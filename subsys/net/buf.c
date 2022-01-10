/* buf.c - Buffer management */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <misc/byteorder.h>

#include <net/buf.h>

#if defined(CONFIG_NET_BUF_LOG)
#define SYS_LOG_DOMAIN "net/buf"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_NET_BUF_LEVEL
#include <logging/sys_log.h>

#define NET_BUF_DBG(fmt, ...) SYS_LOG_DBG("(%p) " fmt, k_current_get(), \
					  ##__VA_ARGS__)
#define NET_BUF_ERR(fmt, ...) SYS_LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_WARN(fmt, ...) SYS_LOG_WRN(fmt,	##__VA_ARGS__)
#define NET_BUF_INFO(fmt, ...) SYS_LOG_INF(fmt,  ##__VA_ARGS__)
#define NET_BUF_ASSERT(cond) do { if (!(cond)) {			  \
			NET_BUF_ERR("assert: '" #cond "' failed"); \
		} } while (0)
#else

#define NET_BUF_DBG(fmt, ...)
#define NET_BUF_ERR(fmt, ...)
#define NET_BUF_WARN(fmt, ...)
#define NET_BUF_INFO(fmt, ...)
#define NET_BUF_ASSERT(cond)
#endif /* CONFIG_NET_BUF_LOG */

#define BUF_TEST_DEBUG		1
#if BUF_TEST_DEBUG
#define BUF_ASSERT(cond, str)			__ASSERT((cond), (str))
#define BUF_LOG(fmt, ...)				printk(fmt, ##__VA_ARGS__)
#else
#define BUF_ASSERT(cond, str)
#define BUF_LOG(fmt, ...)
#endif

#if CONFIG_NET_BUF_WARN_ALLOC_INTERVAL > 0
#define WARN_ALLOC_INTERVAL K_SECONDS(CONFIG_NET_BUF_WARN_ALLOC_INTERVAL)
#else
#define WARN_ALLOC_INTERVAL K_FOREVER
#endif

/* Linker-defined symbol bound to the static pool structs */
extern struct net_buf_pool _net_buf_pool_list[];
extern struct net_buf_pool _net_buf_pool_list_end;

struct net_buf_pool *net_buf_pool_get(int id)
{
	BUF_ASSERT((id < (&_net_buf_pool_list_end - _net_buf_pool_list)), "pool id error");
	return &_net_buf_pool_list[id];
}

static int pool_id(struct net_buf_pool *pool)
{
	BUF_ASSERT(((pool >= _net_buf_pool_list) &&
				(pool < &_net_buf_pool_list_end)), "pool address error");
	return pool - _net_buf_pool_list;
}

int net_buf_pool_id(struct net_buf_pool *pool)
{
	return pool_id(pool);
}

/* Helpers to access the storage array, since we don't have access to its
 * type at this point anymore.
 */
#define BUF_SIZE(pool) (sizeof(struct net_buf) + \
				ROUND_UP(pool->user_data_size, 4) + \
				ROUND_UP(pool->buf_size, 4))
#define UNINIT_BUF(pool, n) (struct net_buf *)(((u8_t *)(pool->__bufs)) + \
							   ((n) * BUF_SIZE(pool)))

#define BUF_SIZE_USE_DATA_POOL(pool) (sizeof(struct net_buf) + \
			ROUND_UP(pool->user_data_size, 4))
#define UNINIT_BUF_USE_DATA_POOL(pool, n) (struct net_buf *)(((u8_t *)(pool->__bufs)) + \
					       ((n) * BUF_SIZE_USE_DATA_POOL(pool)))

static inline struct net_buf *pool_get_uninit(struct net_buf_pool *pool,
					      u16_t uninit_count)
{
	struct net_buf *buf;

	if (pool->use_data_pool) {
		buf = UNINIT_BUF_USE_DATA_POOL(pool, pool->buf_count - uninit_count);
	} else {
		buf = UNINIT_BUF(pool, pool->buf_count - uninit_count);
	}

	buf->pool_id = pool_id(pool);
	buf->size = pool->buf_size;

	return buf;
}

void net_buf_reset(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf->flags == 0);
	NET_BUF_ASSERT(buf->frags == NULL);

	buf->len   = 0;
	buf->data  = buf->__buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_debug(struct net_buf_pool *pool, s32_t timeout,
				    const char *func, int line)
#else
struct net_buf *net_buf_alloc(struct net_buf_pool *pool, s32_t timeout)
#endif
{
	struct net_buf *buf;
	unsigned int key;

	NET_BUF_ASSERT(pool);

	NET_BUF_DBG("%s():%d: pool %p timeout %d", func, line, pool, timeout);

	/* We need to lock interrupts temporarily to prevent race conditions
	 * when accessing pool->uninit_count.
	 */
	key = irq_lock();

	/* If there are uninitialized buffers we're guaranteed to succeed
	 * with the allocation one way or another.
	 */
	if (pool->uninit_count) {
		u16_t uninit_count;

		/* If this is not the first access to the pool, we can
		 * be opportunistic and try to fetch a previously used
		 * buffer from the LIFO with K_NO_WAIT.
		 */
		if (pool->uninit_count < pool->buf_count) {
			buf = k_lifo_get(&pool->free, K_NO_WAIT);
			if (buf) {
				irq_unlock(key);
				goto success;
			}
		}

		uninit_count = pool->uninit_count--;
		irq_unlock(key);

		buf = pool_get_uninit(pool, uninit_count);
		goto success;
	}

	irq_unlock(key);

#if defined(CONFIG_NET_BUF_LOG) && SYS_LOG_LEVEL >= SYS_LOG_LEVEL_WARNING
	if (timeout == K_FOREVER) {
		u32_t ref = k_uptime_get_32();
		buf = k_lifo_get(&pool->free, K_NO_WAIT);
		while (!buf) {
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s low on buffers.",
				     func, line, pool->name);
#else
			NET_BUF_WARN("%s():%d: Pool %p low on buffers.",
				     func, line, pool);
#endif
			buf = k_lifo_get(&pool->free, WARN_ALLOC_INTERVAL);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s blocked for %u secs",
				     func, line, pool->name,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#else
			NET_BUF_WARN("%s():%d: Pool %p blocked for %u secs",
				     func, line, pool,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#endif
		}
	} else {
		buf = k_lifo_get(&pool->free, timeout);
	}
#else
	buf = k_lifo_get(&pool->free, timeout);
#endif
	if (!buf) {
		NET_BUF_ERR("%s():%d: Failed to get free buffer", func, line);
		return NULL;
	}

success:
	NET_BUF_DBG("allocated buf %p", buf);

	BUF_ASSERT(buf, "buf NULL");
	BUF_ASSERT((buf->pool_id == net_buf_pool_id(pool)), "pool id error");

	if (pool->use_data_pool) {
		buf->__buf = pool->data_pool->opt->malloc(buf, pool->buf_size, timeout);
		if (!buf->__buf) {
			BUF_ASSERT(buf->__buf, "buf->__buf NULL");
			k_lifo_put(&pool->free, buf);
			NET_BUF_ERR("%s():%d: Failed to get free data buffer", func, line);
			return NULL;
		}
	} else {
		buf->__buf = &buf->user_data[ROUND_UP(pool->user_data_size, 4)];
	}

	buf->ref   = 1;
	buf->flags = 0;
	buf->frags = NULL;
	net_buf_reset(buf);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	key = irq_lock();
	pool->avail_count--;
	if ((pool->buf_count - pool->avail_count) > pool->max_used) {
		pool->max_used = pool->buf_count - pool->avail_count;
		BUF_LOG("pool(%s data pool %p), max_used: %d\n", pool->name, pool->data_pool, pool->max_used);
	}
	irq_unlock(key);
	NET_BUF_ASSERT(pool->avail_count >= 0);
#endif

	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_len_debug(struct net_buf_pool *pool, u16_t len, s32_t timeout,
				    const char *func, int line)
#else
struct net_buf *net_buf_alloc_len(struct net_buf_pool *pool, u16_t len, s32_t timeout)
#endif
{
	struct net_buf *buf;
	unsigned int key;

	NET_BUF_ASSERT(pool);
	if (!pool->use_data_pool) {
		NET_BUF_ASSERT(pool->use_data_pool);
		NET_BUF_ASSERT(len);
		return NULL;
	}

	NET_BUF_DBG("%s():%d: pool %p len %d timeout %d", func, line, pool, len, timeout);
	/* We need to lock interrupts temporarily to prevent race conditions
	 * when accessing pool->uninit_count.
	 */
	key = irq_lock();

	/* If there are uninitialized buffers we're guaranteed to succeed
	 * with the allocation one way or another.
	 */
	if (pool->uninit_count) {
		u16_t uninit_count;

		/* If this is not the first access to the pool, we can
		 * be opportunistic and try to fetch a previously used
		 * buffer from the LIFO with K_NO_WAIT.
		 */
		if (pool->uninit_count < pool->buf_count) {
			buf = k_lifo_get(&pool->free, K_NO_WAIT);
			if (buf) {
				irq_unlock(key);
				goto success;
			}
		}

		uninit_count = pool->uninit_count--;
		irq_unlock(key);

		buf = pool_get_uninit(pool, uninit_count);
		goto success;
	}

	irq_unlock(key);

#if defined(CONFIG_NET_BUF_LOG) && SYS_LOG_LEVEL >= SYS_LOG_LEVEL_WARNING
	if (timeout == K_FOREVER) {
		u32_t ref = k_uptime_get_32();
		buf = k_lifo_get(&pool->free, K_NO_WAIT);
		while (!buf) {
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s low on buffers.",
					 func, line, pool->name);
#else
			NET_BUF_WARN("%s():%d: Pool %p low on buffers.",
					 func, line, pool);
#endif
			buf = k_lifo_get(&pool->free, WARN_ALLOC_INTERVAL);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s blocked for %u secs",
					 func, line, pool->name,
					 (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#else
			NET_BUF_WARN("%s():%d: Pool %p blocked for %u secs",
					 func, line, pool,
					 (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#endif
		}
	} else {
		buf = k_lifo_get(&pool->free, timeout);
	}
#else
	buf = k_lifo_get(&pool->free, timeout);
#endif
	if (!buf) {
		NET_BUF_ERR("%s():%d: Failed to get free buffer", func, line);
		return NULL;
	}

success:
	NET_BUF_DBG("allocated buf %p", buf);

	BUF_ASSERT(buf, "buf NULL");
	BUF_ASSERT((buf->pool_id == net_buf_pool_id(pool)), "pool id error");

	buf->__buf = pool->data_pool->opt->malloc(buf, len, timeout);
	if (!buf->__buf) {
		BUF_ASSERT(buf->__buf, "buf->__buf NULL");
		k_lifo_put(&pool->free, buf);
		NET_BUF_ERR("%s():%d: Failed to get free data buffer", func, line);
		return NULL;
	}

	buf->size = len;
	buf->ref   = 1;
	buf->flags = 0;
	buf->frags = NULL;
	net_buf_reset(buf);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	key = irq_lock();
	pool->avail_count--;
	if ((pool->buf_count - pool->avail_count) > pool->max_used) {
		pool->max_used = pool->buf_count - pool->avail_count;
		BUF_LOG("pool(%s data pool %p), max_used: %d\n", pool->name, pool->data_pool, pool->max_used);
	}
	irq_unlock(key);
	NET_BUF_ASSERT(pool->avail_count >= 0);
#endif

	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_get_debug(struct k_fifo *fifo, s32_t timeout,
				  const char *func, int line)
#else
struct net_buf *net_buf_get(struct k_fifo *fifo, s32_t timeout)
#endif
{
	struct net_buf *buf, *frag;

	NET_BUF_DBG("%s():%d: fifo %p timeout %d", func, line, fifo, timeout);

	buf = k_fifo_get(fifo, timeout);
	if (!buf) {
		return NULL;
	}

	NET_BUF_DBG("%s():%d: buf %p fifo %p", func, line, buf, fifo);

	/* Get any fragments belonging to this buffer */
	for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
		frag->frags = k_fifo_get(fifo, K_NO_WAIT);
		NET_BUF_ASSERT(frag->frags);

		/* The fragments flag is only for FIFO-internal usage */
		frag->flags &= ~NET_BUF_FRAGS;
	}

	/* Mark the end of the fragment list */
	frag->frags = NULL;

	return buf;
}

void net_buf_reserve(struct net_buf *buf, size_t reserve)
{
	NET_BUF_ASSERT(buf);
	NET_BUF_ASSERT(buf->len == 0);
	NET_BUF_DBG("buf %p reserve %zu", buf, reserve);

	buf->data = buf->__buf + reserve;
}

void net_buf_put(struct k_fifo *fifo, struct net_buf *buf)
{
	struct net_buf *tail;

	NET_BUF_ASSERT(fifo);
	NET_BUF_ASSERT(buf);

	for (tail = buf; tail->frags; tail = tail->frags) {
		tail->flags |= NET_BUF_FRAGS;
	}

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	buf->timestamp = k_uptime_get_32();
#endif
	k_fifo_put_list(fifo, buf, tail);
}

#if defined (CONFIG_NET_DYNAMIC_FRAG)
extern bool free_frags_func_hook(struct net_buf *buf);
#endif

#if defined(CONFIG_NET_BUF_LOG)
void net_buf_unref_debug(struct net_buf *buf, const char *func, int line)
#else
void net_buf_unref(struct net_buf *buf)
#endif
{
	NET_BUF_ASSERT(buf);

	while (buf) {
		struct net_buf *frags = buf->frags;
		struct net_buf_pool *pool;

#if defined(CONFIG_NET_BUF_LOG)
		if (!buf->ref) {
			NET_BUF_ERR("%s():%d: buf %p double free", func, line,
				    buf);
			return;
		}
#endif
		NET_BUF_DBG("buf %p ref %u pool_id %u frags %p", buf, buf->ref,
			    buf->pool_id, buf->frags);

		if (--buf->ref > 0) {
			return;
		}

		buf->frags = NULL;
		pool = net_buf_pool_get(buf->pool_id);

		buf->data = NULL;
		buf->len = 0;
		if (pool->use_data_pool) {
			pool->data_pool->opt->free(buf);
		}
		buf->__buf = NULL;
		buf->size = pool->buf_size;

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		unsigned int key;

		key = irq_lock();
		pool->avail_count++;
		irq_unlock(key);
#if defined (CONFIG_NET_DYNAMIC_FRAG)
#else
		NET_BUF_ASSERT(pool->avail_count <= pool->buf_count);
#endif
#endif

		if (pool->destroy) {
			pool->destroy(buf);
		} else {
#if defined(CONFIG_NET_DYNAMIC_FRAG)
			if (free_frags_func_hook(buf)) {
				/* Dynamic nbuf release */
			} else
#endif
			{
				net_buf_destroy(buf);
			}
		}

		buf = frags;
	}
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf);

	NET_BUF_DBG("buf %p (old) ref %u pool_id %u",
		    buf, buf->ref, buf->pool_id);
	buf->ref++;
	return buf;
}

struct net_buf *net_buf_clone(struct net_buf *buf, s32_t timeout)
{
	struct net_buf_pool *pool;
	struct net_buf *clone;

	NET_BUF_ASSERT(buf);

	pool = net_buf_pool_get(buf->pool_id);

	if (pool->buf_size == buf->size) {
		clone = net_buf_alloc(pool, timeout);
	} else {
		clone = net_buf_alloc_len(pool, buf->size, timeout);
	}

	if (!clone) {
		return NULL;
	}

	net_buf_reserve(clone, net_buf_headroom(buf));

	/* TODO: Add reference to the original buffer instead of copying it. */
	memcpy(net_buf_add(clone, buf->len), buf->data, buf->len);

	return clone;
}

struct net_buf *net_buf_frag_last(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf);

	while (buf->frags) {
		buf = buf->frags;
	}

	return buf;
}

void net_buf_frag_insert(struct net_buf *parent, struct net_buf *frag)
{
	NET_BUF_ASSERT(parent);
	NET_BUF_ASSERT(frag);

	if (parent->frags) {
		net_buf_frag_last(frag)->frags = parent->frags;
	}
	/* Take ownership of the fragment reference */
	parent->frags = frag;
}

struct net_buf *net_buf_frag_add(struct net_buf *head, struct net_buf *frag)
{
	NET_BUF_ASSERT(frag);

	if (!head) {
		return net_buf_ref(frag);
	}

	net_buf_frag_insert(net_buf_frag_last(head), frag);

	return head;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_frag_del_debug(struct net_buf *parent,
				       struct net_buf *frag,
				       const char *func, int line)
#else
struct net_buf *net_buf_frag_del(struct net_buf *parent, struct net_buf *frag)
#endif
{
	struct net_buf *next_frag;

	NET_BUF_ASSERT(frag);

	if (parent) {
		NET_BUF_ASSERT(parent->frags);
		NET_BUF_ASSERT(parent->frags == frag);
		parent->frags = frag->frags;
	}

	next_frag = frag->frags;

	frag->frags = NULL;

#if defined(CONFIG_NET_BUF_LOG)
	net_buf_unref_debug(frag, func, line);
#else
	net_buf_unref(frag);
#endif

	return next_frag;
}

#if defined(CONFIG_NET_BUF_SIMPLE_LOG)
#define NET_BUF_SIMPLE_DBG(fmt, ...) NET_BUF_DBG(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ERR(fmt, ...) NET_BUF_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_WARN(fmt, ...) NET_BUF_WARN(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_INFO(fmt, ...) NET_BUF_INFO(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ASSERT(cond) NET_BUF_ASSERT(cond)
#else
#define NET_BUF_SIMPLE_DBG(fmt, ...)
#define NET_BUF_SIMPLE_ERR(fmt, ...)
#define NET_BUF_SIMPLE_WARN(fmt, ...)
#define NET_BUF_SIMPLE_INFO(fmt, ...)
#define NET_BUF_SIMPLE_ASSERT(cond)			BUF_ASSERT((cond), "net buf simple")
#endif /* CONFIG_NET_BUF_SIMPLE_LOG */

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len)
{
	u8_t *tail = net_buf_simple_tail(buf);

	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	NET_BUF_SIMPLE_ASSERT(net_buf_simple_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem,
			     size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	return memcpy(net_buf_simple_add(buf, len), mem, len);
}

u8_t *net_buf_simple_add_u8(struct net_buf_simple *buf, u8_t val)
{
	u8_t *u8;

	NET_BUF_SIMPLE_DBG("buf %p val 0x%02x", buf, val);

	u8 = net_buf_simple_add(buf, 1);
	*u8 = val;

	return u8;
}

void net_buf_simple_add_le16(struct net_buf_simple *buf, u16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_le16(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be16(struct net_buf_simple *buf, u16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_be16(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_le32(struct net_buf_simple *buf, u32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_le32(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be32(struct net_buf_simple *buf, u32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_be32(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	NET_BUF_SIMPLE_ASSERT(net_buf_simple_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void net_buf_simple_push_le16(struct net_buf_simple *buf, u16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_le16(val);
	memcpy(net_buf_simple_push(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_push_be16(struct net_buf_simple *buf, u16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_be16(val);
	memcpy(net_buf_simple_push(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_push_u8(struct net_buf_simple *buf, u8_t val)
{
	u8_t *data = net_buf_simple_push(buf, 1);

	*data = val;
}

void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	NET_BUF_SIMPLE_ASSERT(buf->len >= len);

	buf->len -= len;
	return buf->data += len;
}

u8_t net_buf_simple_pull_u8(struct net_buf_simple *buf)
{
	u8_t val;

	val = buf->data[0];
	net_buf_simple_pull(buf, 1);

	return val;
}

u16_t net_buf_simple_pull_le16(struct net_buf_simple *buf)
{
	u16_t val;

	val = UNALIGNED_GET((u16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le16_to_cpu(val);
}

u16_t net_buf_simple_pull_be16(struct net_buf_simple *buf)
{
	u16_t val;

	val = UNALIGNED_GET((u16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be16_to_cpu(val);
}

u32_t net_buf_simple_pull_le32(struct net_buf_simple *buf)
{
	u32_t val;

	val = UNALIGNED_GET((u32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le32_to_cpu(val);
}

u32_t net_buf_simple_pull_be32(struct net_buf_simple *buf)
{
	u32_t val;

	val = UNALIGNED_GET((u32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be32_to_cpu(val);
}

size_t net_buf_simple_headroom(struct net_buf_simple *buf)
{
	return buf->data - buf->__buf;
}

size_t net_buf_simple_tailroom(struct net_buf_simple *buf)
{
	return buf->size - net_buf_simple_headroom(buf) - buf->len;
}

/** Option for continue memory manager */
#define DATA_MEM_POOL_ALIGN		(4)
#define DATA_MEM_POOL_USED		BIT(0)
#define MEM_INIT_CHKSUM			(0x5A)

#define DATA_MEM_USED(x)		((x)&DATA_MEM_POOL_USED)
#define DATA_MEM_FREE(x)		(!DATA_MEM_USED(x))

#define DATA_MEM_SET_USED(x) 	do{ \
			(x) = (x) | DATA_MEM_POOL_USED; \
		} while(0)
#define DATA_MEM_SET_FREE(x)	do{ \
			(x) = (x)&(~DATA_MEM_POOL_USED); \
		} while(0)

#define DATA_MEM_SET_CHKSUM(x)	(((u8_t *)(x))[3] = \
		(((u8_t *)(x))[0])^(((u8_t *)(x))[1])^(((u8_t *)(x))[2])^MEM_INIT_CHKSUM)
#define DATA_MEM_CHKSUM_CORRECT(x) ((((u8_t *)(x))[3]) == \
		((((u8_t *)(x))[0])^(((u8_t *)(x))[1])^(((u8_t *)(x))[2])^MEM_INIT_CHKSUM))

#define MEM_MALLOC_CHECK_INTERVAL	5		/* 5ms */

/* Continue memory manager struct */
struct pool_data_info {
	u16_t len;				/* len include data info */
	u8_t flag;
	u8_t chksum;
};

#define DATA_INFO_SIZE		sizeof(struct pool_data_info)

static void net_data_continue_pool_init(void *data_pool)
{
	struct net_data_pool *pool = data_pool;
	struct pool_data_info *info;

	pool->alloc_pos = pool->data_pool;
	info = (struct pool_data_info *)pool->alloc_pos;

	memset(info, 0, DATA_INFO_SIZE);
	info->len = pool->data_size;
	DATA_MEM_SET_FREE(info->flag);
	DATA_MEM_SET_CHKSUM(info);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	pool->curr_used = 0;
	pool->max_used = 0;
#endif
}

static u8_t *continue_pool_malloc(struct net_data_pool *data_pool, u16_t malloc_size)
{
	unsigned int key;
	u8_t *sPos, *nPos, *mallocPos = NULL;
	struct pool_data_info *sInfo, *nInfo;
	bool merge;

	key = irq_lock();
	sPos = data_pool->alloc_pos;

	do {
		merge = false;
		sInfo = (struct pool_data_info *)sPos;
		BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(sInfo), "chksum error");

		if (DATA_MEM_USED(sInfo->flag)) {
			sPos += sInfo->len;
			BUF_ASSERT(sPos <= (data_pool->data_pool + data_pool->data_size), "address error");

			if (sPos == (data_pool->data_pool + data_pool->data_size)) {
				sPos = data_pool->data_pool;
			}
			continue;
		}

		if (sInfo->len >= malloc_size) {
			mallocPos = sPos;
			break;
		} else {
			nPos = sPos + sInfo->len;
			BUF_ASSERT(nPos <= (data_pool->data_pool + data_pool->data_size), "address error");

			if (nPos == (data_pool->data_pool + data_pool->data_size)) {
				sPos = data_pool->data_pool;
				continue;
			} else {
				nInfo = (struct pool_data_info *)nPos;
				BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(nInfo), "chksum error");

				if (DATA_MEM_USED(nInfo->flag)) {
					sPos = nPos;
					continue;
				} else {
					sInfo->len += nInfo->len;
					DATA_MEM_SET_CHKSUM(sInfo);
					memset(nInfo, 0, DATA_INFO_SIZE);
					merge = true;
					continue;
				}
			}
		}
	} while ((sPos != data_pool->alloc_pos) || merge);

	if (mallocPos) {
		sInfo = (struct pool_data_info *)mallocPos;
		if (sInfo->len > malloc_size) {
			nInfo = (struct pool_data_info *)(mallocPos + malloc_size);
			memset(nInfo, 0, DATA_INFO_SIZE);
			nInfo->len = sInfo->len - malloc_size;
			DATA_MEM_SET_FREE(nInfo->flag);
			DATA_MEM_SET_CHKSUM(nInfo);
		}

		if ((mallocPos + malloc_size) == (data_pool->data_pool + data_pool->data_size)) {
			data_pool->alloc_pos = data_pool->data_pool;
		} else {
			data_pool->alloc_pos = mallocPos + malloc_size;
		}

		sInfo->len = malloc_size;
		DATA_MEM_SET_USED(sInfo->flag);
		DATA_MEM_SET_CHKSUM(sInfo);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
		data_pool->curr_used += malloc_size;
		if (data_pool->curr_used > data_pool->max_used) {
			data_pool->max_used = data_pool->curr_used;
			BUF_LOG("data pool(%p), max_used: %d\n", data_pool, data_pool->max_used);
			BUF_ASSERT((data_pool->max_used <= data_pool->data_size), "Error max used");
		}
#endif
	}

	irq_unlock(key);

	if (mallocPos) {
		return (mallocPos + DATA_INFO_SIZE);
	} else {
		return NULL;
	}
}

static void continue_pool_free(struct net_data_pool *data_pool, u8_t *pData)
{
	unsigned int key;
	struct pool_data_info *info = (struct pool_data_info *)(pData - DATA_INFO_SIZE);

	BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(info), "chksum error");

	key = irq_lock();

	DATA_MEM_SET_FREE(info->flag);
	DATA_MEM_SET_CHKSUM(info);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	data_pool->curr_used -= info->len;
#endif

	irq_unlock(key);
}

#if BUF_TEST_DEBUG
static void dump_continue_pool(struct net_data_pool *pool, u16_t malloc_size)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	u8_t *pos;
	u16_t cal_used_len = 0;
	struct pool_data_info *info;

	k_sched_lock();

	BUF_LOG("%s\n", __func__);
	BUF_LOG("data_pool: %p~%p, malloc_size: %d\n", pool->data_pool,
			(pool->data_pool + pool->data_size), malloc_size);
	BUF_LOG("size: %d, curr_used: %d, max_used: %d, alloc_pos: %p\n",
			pool->data_size, pool->curr_used, pool->max_used, pool->alloc_pos);

	pos = pool->data_pool;
	BUF_LOG("address\t len\t flag\n");
	while (pos < (pool->data_pool + pool->data_size)) {
		info = (struct pool_data_info *)pos;
		BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(info), "chksum error");
		BUF_LOG("%p\t %d\t %d\n", info, info->len, info->flag);
		if (DATA_MEM_USED(info->flag)) {
			cal_used_len += info->len;
		}

		pos += info->len;
	}

	BUF_LOG("cal_used_len: %d\n", cal_used_len);

	k_sched_unlock();
#endif
}
#endif

static u8_t *net_data_continue_pool_malloc(struct net_buf *buf, u16_t size, s32_t timeout)
{
	u8_t *pData;
	u16_t malloc_size;
	s32_t sleep_time, remain_time = timeout;
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct net_data_pool *data_pool = buf_pool->data_pool;

	BUF_ASSERT(buf_pool, "buf pool NULL");
	BUF_ASSERT(data_pool, "data pool NULL");

	malloc_size = size + DATA_INFO_SIZE;
	malloc_size = ROUND_UP(malloc_size, DATA_MEM_POOL_ALIGN);

try_again:
	pData = continue_pool_malloc(data_pool, malloc_size);
	if (pData) {
		return pData;
	}

#if BUF_TEST_DEBUG
	dump_continue_pool(data_pool, malloc_size);
#endif
	if (timeout == K_NO_WAIT) {
		return pData;
	} else if (timeout == K_FOREVER) {
		k_sleep(MEM_MALLOC_CHECK_INTERVAL);
		goto try_again;
	} else if (remain_time) {
		sleep_time = (remain_time > MEM_MALLOC_CHECK_INTERVAL)? MEM_MALLOC_CHECK_INTERVAL : remain_time;
		k_sleep(sleep_time);
		remain_time -= remain_time;
		goto try_again;
	} else {
		return pData;
	}
}

static void net_data_continue_pool_free(struct net_buf *buf)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct net_data_pool *data_pool = buf_pool->data_pool;

	BUF_ASSERT(buf->__buf, "buf->__buf NULL");
	BUF_ASSERT(buf_pool, "buf pool NULL");
	BUF_ASSERT(data_pool, "data pool NULL");

	continue_pool_free(data_pool, buf->__buf);
}

struct data_pool_opt continue_data_pool_opt = {
	.init = net_data_continue_pool_init,
	.malloc = net_data_continue_pool_malloc,
	.free = net_data_continue_pool_free,
};

void net_buf_dumpinfo(void)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	struct net_buf_pool *pool;

	printk("Net pool info\n");
	printk("\t addr\t count\t avail\t max_used\t name\t use_data_pool\n");
	for (pool = &_net_buf_pool_list[0];
		pool < &_net_buf_pool_list_end; pool++) {
		printk("pool: %p\t %d\t %d\t %d\t %s\t %d\n", pool, pool->buf_count,
			pool->avail_count, pool->max_used, pool->name, pool->use_data_pool);
		if (pool->use_data_pool) {
			printk("data_pool: %p\t %d\t %d\t\t %d\n", pool->data_pool,
				pool->data_pool->data_size, pool->data_pool->curr_used, pool->data_pool->max_used);
		}
	}
#else
	printk("CONFIG_NET_BUF_POOL_USAGE not set\n");
#endif
}
