/** @file
 @brief Network packet buffers for IP stack

 Network data is passed between components using net_pkt.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_NET_PKT)
#define SYS_LOG_DOMAIN "net/pkt"
#define NET_LOG_ENABLED 1

/* This enables allocation debugging but does not print so much output
 * as that can slow things down a lot.
 */
#if !defined(CONFIG_NET_DEBUG_NET_PKT_ALL)
#define NET_SYS_LOG_LEVEL 5
#endif
#endif

#include <kernel.h>
#include <toolchain.h>
#include <string.h>
#include <zephyr/types.h>
#include <sys/types.h>

#include <misc/util.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/buf.h>
#include <net/net_pkt.h>

#include "net_private.h"

/* Available (free) buffers queue */
#define NET_PKT_RX_COUNT	CONFIG_NET_PKT_RX_COUNT
#define NET_PKT_TX_COUNT	CONFIG_NET_PKT_TX_COUNT
#define NET_BUF_RX_COUNT	CONFIG_NET_BUF_RX_COUNT
#define NET_BUF_TX_COUNT	CONFIG_NET_BUF_TX_COUNT
#define NET_BUF_DATA_LEN	CONFIG_NET_BUF_DATA_SIZE
#define NET_BUF_USER_DATA_LEN	CONFIG_NET_BUF_USER_DATA_SIZE

#if defined(CONFIG_NET_BUF_SHARE)
#define NET_BUF_SHARE_COUNT CONFIG_NET_BUF_SHARE_COUNT
#else
#define NET_BUF_SHARE_COUNT		0
#endif

#if defined(CONFIG_NET_NBUF_INNER)
#define NET_BUF_INNER_COUNT	CONFIG_NET_NBUF_INNER_COUNT
#define NET_BUF_INNER_LEN	CONFIG_NET_NBUF_INNER_SIZE
#else
#define NET_BUF_INNER_COUNT		0
#define NET_BUF_INNER_LEN		0
#endif

#define NET_PKT_BUF_FOREVER_TIMEOUT			10000		/* 10s */
#define NET_PKT_BUF_ONE_WAITTIME			500			/* 500ms */

#if defined(CONFIG_NET_TCP)
#define APP_PROTO_LEN NET_TCPH_LEN
#else
#if defined(CONFIG_NET_UDP)
#define APP_PROTO_LEN NET_UDPH_LEN
#else
#define APP_PROTO_LEN 0
#endif /* UDP */
#endif /* TCP */

#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_L2_RAW_CHANNEL)
#define IP_PROTO_LEN NET_IPV6H_LEN
#else
#if defined(CONFIG_NET_IPV4)
#define IP_PROTO_LEN NET_IPV4H_LEN
#else
#error "Either IPv6 or IPv4 needs to be selected."
#endif /* IPv4 */
#endif /* IPv6 */

#define EXTRA_PROTO_LEN NET_ICMPH_LEN

/* Make sure that IP + TCP/UDP header fit into one
 * fragment. This makes possible to cast a protocol header
 * struct into memory area.
 */
#if NET_BUF_DATA_LEN < (IP_PROTO_LEN + APP_PROTO_LEN)
#if defined(STRING2)
#undef STRING2
#endif
#if defined(STRING)
#undef STRING
#endif
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#pragma message "Data len " STRING(NET_BUF_DATA_LEN)
#pragma message "Minimum len " STRING(IP_PROTO_LEN + APP_PROTO_LEN)
#error "Too small net_buf fragment size"
#endif

K_MEM_SLAB_DEFINE(rx_pkts, sizeof(struct net_pkt), NET_PKT_RX_COUNT, 4);
K_MEM_SLAB_DEFINE(tx_pkts, sizeof(struct net_pkt), NET_PKT_TX_COUNT, 4);

#if defined(CONFIG_NET_DEBUG_NET_PKT)
static bool net_pkt_alloc_del(void *alloc_data, const char *func, int line);
#endif

static void net_pkt_buf_destroy(struct net_buf *buf)
{
#if defined(CONFIG_NET_DEBUG_NET_PKT)
	net_pkt_alloc_del(buf, __func__, __LINE__);
#endif
	net_buf_destroy(buf);
}

/* The data fragment pool is for storing network data. */
NET_BUF_POOL_DEFINE(rx_bufs, NET_BUF_RX_COUNT, NET_BUF_DATA_LEN,
		    NET_BUF_USER_DATA_LEN, net_pkt_buf_destroy);

NET_BUF_POOL_DEFINE(tx_bufs, NET_BUF_TX_COUNT, NET_BUF_DATA_LEN,
		    NET_BUF_USER_DATA_LEN, net_pkt_buf_destroy);

#if defined(CONFIG_NET_BUF_SHARE)
NET_BUF_POOL_DEFINE(share_bufs, NET_BUF_SHARE_COUNT, NET_BUF_DATA_LEN,
		    0, net_pkt_buf_destroy);
#endif

#if defined(CONFIG_NET_NBUF_INNER)
NET_BUF_POOL_DEFINE(inner_bufs, NET_BUF_INNER_COUNT, NET_BUF_INNER_LEN,
		    0, net_pkt_buf_destroy);
#endif

#if defined(CONFIG_NET_DYNAMIC_FRAG)
#define MAX_DYNAMIC_COUNT	3
#define DYNAMIC_FRAG_SIZE	(NET_BUF_DATA_LEN + sizeof(struct net_buf))

/* If use malloc frag, no need define dynamic_frag */
static char dynamic_frag[MAX_DYNAMIC_COUNT][DYNAMIC_FRAG_SIZE]	__net_buf_align __noinit;

static struct net_buf *pDynamic_frag[MAX_DYNAMIC_COUNT];
static char dynamic_frag_cnt = 0;
static bool add_frag_flag = false;
static bool release_frag_flag = false;

K_SEM_DEFINE(sem_frag_release, 0, 1);

#else
#define MAX_DYNAMIC_COUNT	0
#define dynamic_frag_cnt	0
#endif

#if defined(CONFIG_NET_DEBUG_NET_PKT)

#define NET_FRAG_CHECK_IF_NOT_IN_USE(frag, ref)				\
	do {								\
		if (!(ref)) {                                           \
			NET_ERR("**ERROR** frag %p not in use (%s:%s():%d)", \
				frag, __FILE__, __func__, __LINE__);     \
		}                                                       \
	} while (0)

struct net_pkt_alloc {
	union {
		struct net_pkt *pkt;
		struct net_buf *buf;
		void *alloc_data;
	};
	const char *func_alloc;
	const char *func_free;
	u16_t line_alloc;
	u16_t line_free;
	u8_t in_use;
	bool is_pkt;
};

#define MAX_NET_PKT_ALLOCS (NET_PKT_RX_COUNT + NET_PKT_TX_COUNT + \
			    NET_BUF_RX_COUNT + NET_BUF_TX_COUNT + \
				NET_BUF_SHARE_COUNT + \
				NET_BUF_INNER_COUNT + \
				MAX_DYNAMIC_COUNT + \
			    CONFIG_NET_DEBUG_NET_PKT_EXTERNALS)

static struct net_pkt_alloc net_pkt_allocs[MAX_NET_PKT_ALLOCS];

static bool net_pkt_alloc_add(void *alloc_data, bool is_pkt,
			      const char *func, int line)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].in_use) {
			continue;
		}

		net_pkt_allocs[i].in_use = true;
		net_pkt_allocs[i].is_pkt = is_pkt;
		net_pkt_allocs[i].alloc_data = alloc_data;
		net_pkt_allocs[i].func_alloc = func;
		net_pkt_allocs[i].line_alloc = line;

		return true;
	}

	return false;
}

static bool net_pkt_alloc_del(void *alloc_data, const char *func, int line)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].in_use &&
		    net_pkt_allocs[i].alloc_data == alloc_data) {
			net_pkt_allocs[i].func_free = func;
			net_pkt_allocs[i].line_free = line;
			net_pkt_allocs[i].in_use = false;

			return true;
		}
	}

	return false;
}

static bool net_pkt_alloc_find(void *alloc_data,
			       const char **func_free,
			       int *line_free)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (!net_pkt_allocs[i].in_use &&
		    net_pkt_allocs[i].alloc_data == alloc_data) {
			*func_free = net_pkt_allocs[i].func_free;
			*line_free = net_pkt_allocs[i].line_free;

			return true;
		}
	}

	return false;
}

void net_pkt_allocs_foreach(net_pkt_allocs_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].in_use) {
			cb(net_pkt_allocs[i].is_pkt ?
			   net_pkt_allocs[i].pkt : NULL,
			   net_pkt_allocs[i].is_pkt ?
			   NULL : net_pkt_allocs[i].buf,
			   net_pkt_allocs[i].func_alloc,
			   net_pkt_allocs[i].line_alloc,
			   net_pkt_allocs[i].func_free,
			   net_pkt_allocs[i].line_free,
			   net_pkt_allocs[i].in_use,
			   user_data);
		}
	}

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (!net_pkt_allocs[i].in_use) {
			cb(net_pkt_allocs[i].is_pkt ?
			   net_pkt_allocs[i].pkt : NULL,
			   net_pkt_allocs[i].is_pkt ?
			   NULL : net_pkt_allocs[i].buf,
			   net_pkt_allocs[i].func_alloc,
			   net_pkt_allocs[i].line_alloc,
			   net_pkt_allocs[i].func_free,
			   net_pkt_allocs[i].line_free,
			   net_pkt_allocs[i].in_use,
			   user_data);
		}
	}
}

const char *net_pkt_slab2str(struct k_mem_slab *slab)
{
	if (slab == &rx_pkts) {
		return "RX";
	} else if (slab == &tx_pkts) {
		return "TX";
	}

	return "EXT";
}

const char *net_pkt_pool2str(struct net_buf_pool *pool)
{
	if (pool == &rx_bufs) {
		return "RDATA";
	} else if (pool == &tx_bufs) {
		return "TDATA";
	}
#if defined(CONFIG_NET_BUF_SHARE)
	else if (pool == &share_bufs) {
		return "SDATA";
	}
#endif
#if defined(CONFIG_NET_BUF_SHARE)
	else if (pool == &inner_bufs) {
		return "IDATA";
	}
#endif

	return "EDATA";
}

static inline s16_t get_frees(struct net_buf_pool *pool)
{
	return pool->avail_count;
}

static inline const char *slab2str(struct k_mem_slab *slab)
{
	return net_pkt_slab2str(slab);
}

static inline const char *pool2str(struct net_buf_pool *pool)
{
	return net_pkt_pool2str(pool);
}

void net_pkt_print_frags(struct net_pkt *pkt)
{
	struct net_buf *frag;
	size_t total = 0;
	int count = -1, frag_size = 0, ll_overhead = 0;

	if (!pkt) {
		NET_INFO("pkt %p", pkt);
		return;
	}

	NET_INFO("pkt %p frags %p", pkt, pkt->frags);

	NET_ASSERT(pkt->frags);

	frag = pkt->frags;
	while (frag) {
		total += frag->len;

		frag_size = frag->size;
		ll_overhead = net_buf_headroom(frag);

		NET_INFO("[%d] frag %p len %d size %d reserve %d "
			 "pool %p [sz %d ud_sz %d]",
			 count, frag, frag->len, frag_size, ll_overhead,
			 net_buf_pool_get(frag->pool_id),
			 net_buf_pool_get(frag->pool_id)->buf_size,
			 net_buf_pool_get(frag->pool_id)->user_data_size);

		count++;

		frag = frag->frags;
	}

	NET_INFO("Total data size %zu, occupied %d bytes, ll overhead %d, "
		 "utilization %zu%%",
		 total, count * frag_size - count * ll_overhead,
		 count * ll_overhead, (total * 100) / (count * frag_size));
}

struct net_pkt *net_pkt_get_reserve_debug(struct k_mem_slab *slab,
					  u16_t reserve_head,
					  s32_t timeout,
					  const char *caller,
					  int line)
#else /* CONFIG_NET_DEBUG_NET_PKT */
struct net_pkt *net_pkt_get_reserve(struct k_mem_slab *slab,
				    u16_t reserve_head,
				    s32_t timeout)
#endif /* CONFIG_NET_DEBUG_NET_PKT */
{
	struct net_pkt *pkt;
	int ret;

	if (k_is_in_isr()) {
		ret = k_mem_slab_alloc(slab, (void **)&pkt, K_NO_WAIT);
	} else {
		if (timeout == K_FOREVER) {
			timeout = NET_PKT_BUF_FOREVER_TIMEOUT;
			ret = k_mem_slab_alloc(slab, (void **)&pkt, timeout);
			if (ret) {
				printk("%s,%d, wait forever return NULL\n", __func__, __LINE__);
			}
		} else {
			ret = k_mem_slab_alloc(slab, (void **)&pkt, timeout);
		}
	}

	if (ret) {
		return NULL;
	}

	memset(pkt, 0, sizeof(struct net_pkt));

	net_pkt_set_ll_reserve(pkt, reserve_head);

	pkt->ref = 1;
	pkt->slab = slab;

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	net_pkt_alloc_add(pkt, true, caller, line);

	NET_DBG("%s [%u] pkt %p reserve %u ref %d (%s():%d)",
		slab2str(slab), k_mem_slab_num_free_get(slab),
		pkt, reserve_head, pkt->ref, caller, line);
#endif
	return pkt;
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
struct net_buf *net_pkt_get_reserve_data_debug(struct net_buf_pool *pool,
					       u16_t reserve_head,
					       s32_t timeout,
					       const char *caller,
					       int line)
#else /* CONFIG_NET_DEBUG_NET_PKT */
struct net_buf *net_pkt_get_reserve_data(struct net_buf_pool *pool,
					 u16_t reserve_head,
					 s32_t timeout)
#endif /* CONFIG_NET_DEBUG_NET_PKT */
{
	struct net_buf *frag = NULL;
	s32_t leavetime;

	/*
	 * The reserve_head variable in the function will tell
	 * the size of the link layer headers if there are any.
	 */

	if (k_is_in_isr()) {
		frag = net_buf_alloc(pool, K_NO_WAIT);
	} else {
		leavetime = (timeout == K_FOREVER)? NET_PKT_BUF_FOREVER_TIMEOUT : timeout;

#if defined(CONFIG_NET_NBUF_INNER)
		if (pool == &inner_bufs) {
			frag = net_buf_alloc(pool, leavetime);
			if (!frag) {
				printk("Can't alloc from inner_bufs, alloc from share_bufs\n");
				frag = net_buf_alloc(&share_bufs, K_NO_WAIT);
			}

			goto get_inner;
		}
#endif

		do {
#if defined(CONFIG_NET_BUF_SHARE)
			if ((pool == &rx_bufs) || (pool == &tx_bufs)) {
				frag = net_buf_alloc(&share_bufs, K_NO_WAIT);
			}
#endif
			if (!frag) {
				if (leavetime > NET_PKT_BUF_ONE_WAITTIME) {
					frag = net_buf_alloc(pool, NET_PKT_BUF_ONE_WAITTIME);
					leavetime -= NET_PKT_BUF_ONE_WAITTIME;
				} else {
					frag = net_buf_alloc(pool, leavetime);
					leavetime = 0;
				}
			}
		} while (leavetime && (!frag));

#if defined(CONFIG_NET_NBUF_INNER)
get_inner:
#endif
		if ((timeout == K_FOREVER) && (!frag)) {
#if defined(CONFIG_NET_DEBUG_NET_PKT)
			printk("%s,%d, pool:%p, wait forever return NULL\n", caller, line, pool);
#else
			printk("%s,%d, pool:%p, wait forever return NULL\n", __func__, __LINE__, pool);
#endif
		}
	}

	if (!frag) {
		return NULL;
	}

	net_buf_reserve(frag, reserve_head);

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	NET_FRAG_CHECK_IF_NOT_IN_USE(frag, frag->ref + 1);

	net_pkt_alloc_add(frag, false, caller, line);

	NET_DBG("%s (%s) [%d] frag %p reserve %u ref %d (%s():%d)",
		pool2str(pool), pool->name, get_frees(pool),
		frag, reserve_head, frag->ref, caller, line);
#endif

	return frag;
}

/* Get a fragment, try to figure out the pool from where to get
 * the data.
 */
#if defined(CONFIG_NET_DEBUG_NET_PKT)
struct net_buf *net_pkt_get_frag_debug(struct net_pkt *pkt,
				       s32_t timeout,
				       const char *caller, int line)
#else
struct net_buf *net_pkt_get_frag(struct net_pkt *pkt,
				 s32_t timeout)
#endif
{
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	struct net_context *context;

	context = net_pkt_context(pkt);
	if (context && context->data_pool) {
#if defined(CONFIG_NET_DEBUG_NET_PKT)
		return net_pkt_get_reserve_data_debug(context->data_pool(),
						      net_pkt_ll_reserve(pkt),
						      timeout, caller, line);
#else
		return net_pkt_get_reserve_data(context->data_pool(),
						net_pkt_ll_reserve(pkt),
						timeout);
#endif /* CONFIG_NET_DEBUG_NET_PKT */
	}
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

	if (pkt->slab == &rx_pkts) {
#if defined(CONFIG_NET_DEBUG_NET_PKT)
		return net_pkt_get_reserve_rx_data_debug(
			net_pkt_ll_reserve(pkt), timeout, caller, line);
#else
		return net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
						   timeout);
#endif
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	return net_pkt_get_reserve_tx_data_debug(net_pkt_ll_reserve(pkt),
						 timeout, caller, line);
#else
	return net_pkt_get_reserve_tx_data(net_pkt_ll_reserve(pkt),
					   timeout);
#endif
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
struct net_pkt *net_pkt_get_reserve_rx_debug(u16_t reserve_head,
					     s32_t timeout,
					     const char *caller, int line)
{
	return net_pkt_get_reserve_debug(&rx_pkts, reserve_head, timeout,
					 caller, line);
}

struct net_pkt *net_pkt_get_reserve_tx_debug(u16_t reserve_head,
					     s32_t timeout,
					     const char *caller, int line)
{
	return net_pkt_get_reserve_debug(&tx_pkts, reserve_head, timeout,
					 caller, line);
}

struct net_buf *net_pkt_get_reserve_rx_data_debug(u16_t reserve_head,
						  s32_t timeout,
						  const char *caller, int line)
{
	return net_pkt_get_reserve_data_debug(&rx_bufs, reserve_head,
					      timeout, caller, line);
}

struct net_buf *net_pkt_get_reserve_tx_data_debug(u16_t reserve_head,
						  s32_t timeout,
						  const char *caller, int line)
{
	return net_pkt_get_reserve_data_debug(&tx_bufs, reserve_head,
					      timeout, caller, line);
}

#if defined(CONFIG_NET_NBUF_INNER)
struct net_buf *net_pkt_get_reserve_inner_data_debug(u16_t reserve_head,
						  s32_t timeout,
						  const char *caller, int line)
{
	return net_pkt_get_reserve_data_debug(&inner_bufs, reserve_head,
					      timeout, caller, line);
}
#endif
#else /* CONFIG_NET_DEBUG_NET_PKT */

struct net_pkt *net_pkt_get_reserve_rx(u16_t reserve_head,
				       s32_t timeout)
{
	return net_pkt_get_reserve(&rx_pkts, reserve_head, timeout);
}

struct net_pkt *net_pkt_get_reserve_tx(u16_t reserve_head,
				       s32_t timeout)
{
	return net_pkt_get_reserve(&tx_pkts, reserve_head, timeout);
}

struct net_buf *net_pkt_get_reserve_rx_data(u16_t reserve_head,
					    s32_t timeout)
{
	return net_pkt_get_reserve_data(&rx_bufs, reserve_head, timeout);
}

struct net_buf *net_pkt_get_reserve_tx_data(u16_t reserve_head,
					    s32_t timeout)
{
	return net_pkt_get_reserve_data(&tx_bufs, reserve_head, timeout);
}

#if defined(CONFIG_NET_NBUF_INNER)
struct net_buf *net_pkt_get_reserve_inner_data(u16_t reserve_head,
					    s32_t timeout)
{
	return net_pkt_get_reserve_data(&inner_bufs, reserve_head, timeout);
}
#endif
#endif /* CONFIG_NET_DEBUG_NET_PKT */


#if defined(CONFIG_NET_DEBUG_NET_PKT)
static struct net_pkt *net_pkt_get_debug(struct k_mem_slab *slab,
					 struct net_context *context,
					 s32_t timeout,
					 const char *caller, int line)
#else
static struct net_pkt *net_pkt_get(struct k_mem_slab *slab,
				   struct net_context *context,
				   s32_t timeout)
#endif /* CONFIG_NET_DEBUG_NET_PKT */
{
	struct in6_addr *addr6 = NULL;
	struct net_if *iface;
	struct net_pkt *pkt;

	if (!context) {
		return NULL;
	}

	iface = net_context_get_iface(context);

	NET_ASSERT(iface);

	if (net_context_get_family(context) == AF_INET6) {
		addr6 = &((struct sockaddr_in6 *) &context->remote)->sin6_addr;
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	pkt = net_pkt_get_reserve_debug(slab,
					net_if_get_ll_reserve(iface, addr6),
					timeout, caller, line);
#else
	pkt = net_pkt_get_reserve(slab, net_if_get_ll_reserve(iface, addr6),
				  timeout);
#endif
	if (pkt) {
		net_pkt_set_context(pkt, context);
		net_pkt_set_iface(pkt, iface);

		if (context) {
			net_pkt_set_family(pkt,
					   net_context_get_family(context));
		}
	}

	return pkt;
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
static struct net_buf *_pkt_get_data_debug(struct net_buf_pool *pool,
					   struct net_context *context,
					   s32_t timeout,
					   const char *caller, int line)
#else
static struct net_buf *_pkt_get_data(struct net_buf_pool *pool,
				     struct net_context *context,
				     s32_t timeout)
#endif /* CONFIG_NET_DEBUG_NET_PKT */
{
	struct in6_addr *addr6 = NULL;
	struct net_if *iface;
	struct net_buf *frag;

	if (!context) {
		return NULL;
	}

	iface = net_context_get_iface(context);

	NET_ASSERT(iface);

	if (net_context_get_family(context) == AF_INET6) {
		addr6 = &((struct sockaddr_in6 *) &context->remote)->sin6_addr;
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	frag = net_pkt_get_reserve_data_debug(pool,
					      net_if_get_ll_reserve(iface,
								    addr6),
					      timeout, caller, line);
#else
	frag = net_pkt_get_reserve_data(pool,
					net_if_get_ll_reserve(iface, addr6),
					timeout);
#endif
	return frag;
}


#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
static inline struct k_mem_slab *get_tx_slab(struct net_context *context)
{
	if (context->tx_slab) {
		return context->tx_slab();
	}

	return NULL;
}

static inline struct net_buf_pool *get_data_pool(struct net_context *context)
{
	if (context->data_pool) {
		return context->data_pool();
	}

	return NULL;
}
#else
#define get_tx_slab(...) NULL
#define get_data_pool(...) NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_DEBUG_NET_PKT)
struct net_pkt *net_pkt_get_rx_debug(struct net_context *context,
				     s32_t timeout,
				     const char *caller, int line)
{
	return net_pkt_get_debug(&rx_pkts, context, timeout, caller, line);
}

struct net_pkt *net_pkt_get_tx_debug(struct net_context *context,
				     s32_t timeout,
				     const char *caller, int line)
{
	struct k_mem_slab *slab = get_tx_slab(context);

	return net_pkt_get_debug(slab ? slab : &tx_pkts, context,
				 timeout, caller, line);
}

struct net_buf *net_pkt_get_data_debug(struct net_context *context,
				       s32_t timeout,
				       const char *caller, int line)
{
	struct net_buf_pool *pool = get_data_pool(context);

	return _pkt_get_data_debug(pool ? pool : &tx_bufs, context,
				   timeout, caller, line);
}

#if defined(CONFIG_NET_NBUF_INNER)
struct net_buf *net_pkt_get_inner_debug(struct net_context *context,
				       s32_t timeout,
				       const char *caller, int line)
{
	return _pkt_get_data_debug(&inner_bufs, context,
				   timeout, caller, line);
}
#endif

#else /* CONFIG_NET_DEBUG_NET_PKT */

struct net_pkt *net_pkt_get_rx(struct net_context *context, s32_t timeout)
{
	NET_ASSERT_INFO(context, "RX context not set");

	return net_pkt_get(&rx_pkts, context, timeout);
}

struct net_pkt *net_pkt_get_tx(struct net_context *context, s32_t timeout)
{
	struct k_mem_slab *slab;

	NET_ASSERT_INFO(context, "TX context not set");

	slab = get_tx_slab(context);

	return net_pkt_get(slab ? slab : &tx_pkts, context, timeout);
}

struct net_buf *net_pkt_get_data(struct net_context *context, s32_t timeout)
{
	struct net_buf_pool *pool;

	NET_ASSERT_INFO(context, "Data context not set");

	pool = get_data_pool(context);

	/* The context is not known in RX path so we can only have TX
	 * data here.
	 */
	return _pkt_get_data(pool ? pool : &tx_bufs, context, timeout);
}

#if defined(CONFIG_NET_NBUF_INNER)
struct net_buf *net_pkt_get_inner(struct net_context *context, s32_t timeout)
{
	return _pkt_get_data(&inner_bufs, context, timeout);
}
#endif

#endif /* CONFIG_NET_DEBUG_NET_PKT */


#if defined(CONFIG_NET_DEBUG_NET_PKT)
void net_pkt_unref_debug(struct net_pkt *pkt, const char *caller, int line)
{
	struct net_buf *frag;

#else
void net_pkt_unref(struct net_pkt *pkt)
{
#endif /* CONFIG_NET_DEBUG_NET_PKT */
	if (!pkt) {
		NET_ERR("*** ERROR *** pkt %p (%s():%d)", pkt, caller, line);
		return;
	}

	if (!pkt->ref) {
#if defined(CONFIG_NET_DEBUG_NET_PKT)
		const char *func_freed;
		int line_freed;

		if (net_pkt_alloc_find(pkt, &func_freed, &line_freed)) {
			NET_ERR("*** ERROR *** pkt %p is freed already by "
				"%s():%d (%s():%d)",
				pkt, func_freed, line_freed, caller, line);
		} else {
			NET_ERR("*** ERROR *** pkt %p is freed already "
				"(%s():%d)", pkt, caller, line);
		}
#endif
		return;
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	NET_DBG("%s [%d] pkt %p ref %d frags %p (%s():%d)",
		slab2str(pkt->slab), k_mem_slab_num_free_get(pkt->slab),
		pkt, pkt->ref - 1, pkt->frags, caller, line);

	if (pkt->ref > 1) {
		goto done;
	}

	frag = pkt->frags;
	while (frag) {
		NET_DBG("%s (%s) [%d] frag %p ref %d frags %p (%s():%d)",
			pool2str(net_buf_pool_get(frag->pool_id)),
			net_buf_pool_get(frag->pool_id)->name,
			get_frees(net_buf_pool_get(frag->pool_id)), frag,
			frag->ref - 1, frag->frags, caller, line);

		if (!frag->ref) {
			const char *func_freed;
			int line_freed;

			if (net_pkt_alloc_find(frag,
					       &func_freed, &line_freed)) {
				NET_ERR("*** ERROR *** frag %p is freed "
					"already by %s():%d (%s():%d)",
					frag, func_freed, line_freed,
					caller, line);
			} else {
				NET_ERR("*** ERROR *** frag %p is freed "
					"already (%s():%d)",
					frag, caller, line);
			}
		}

		frag = frag->frags;
	}
done:
#endif /* CONFIG_NET_DEBUG_NET_PKT */

	if (--pkt->ref > 0) {
		return;
	}

	if (pkt->frags) {
		net_pkt_frag_unref(pkt->frags);
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	net_pkt_alloc_del(pkt, caller, line);
#endif
	k_mem_slab_free(pkt->slab, (void **)&pkt);
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
struct net_pkt *net_pkt_ref_debug(struct net_pkt *pkt, const char *caller,
				  int line)
#else
struct net_pkt *net_pkt_ref(struct net_pkt *pkt)
#endif /* CONFIG_NET_DEBUG_NET_PKT */
{
	if (!pkt) {
		NET_ERR("*** ERROR *** pkt %p (%s():%d)", pkt, caller, line);
		return NULL;
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	NET_DBG("%s [%d] pkt %p ref %d (%s():%d)",
		slab2str(pkt->slab), k_mem_slab_num_free_get(pkt->slab),
		pkt, pkt->ref + 1, caller, line);
#endif

	pkt->ref++;

	return pkt;
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
struct net_buf *net_pkt_frag_ref_debug(struct net_buf *frag,
				       const char *caller, int line)
#else
struct net_buf *net_pkt_frag_ref(struct net_buf *frag)
#endif /* CONFIG_NET_DEBUG_NET_PKT */
{
	if (!frag) {
		NET_ERR("*** ERROR *** frag %p (%s():%d)", frag, caller, line);
		return NULL;
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	NET_DBG("%s (%s) [%d] frag %p ref %d (%s():%d)",
		pool2str(net_buf_pool_get(frag->pool_id)),
		net_buf_pool_get(frag->pool_id)->name,
		get_frees(net_buf_pool_get(frag->pool_id)),
		frag, frag->ref + 1, caller, line);
#endif

	return net_buf_ref(frag);
}


#if defined(CONFIG_NET_DEBUG_NET_PKT)
void net_pkt_frag_unref_debug(struct net_buf *frag,
			      const char *caller, int line)
#else
void net_pkt_frag_unref(struct net_buf *frag)
#endif /* CONFIG_NET_DEBUG_NET_PKT */
{
	if (!frag) {
		NET_ERR("*** ERROR *** frag %p (%s():%d)", frag, caller, line);
		return;
	}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	NET_DBG("%s (%s) [%d] frag %p ref %d (%s():%d)",
		pool2str(net_buf_pool_get(frag->pool_id)),
		net_buf_pool_get(frag->pool_id)->name,
		get_frees(net_buf_pool_get(frag->pool_id)),
		frag, frag->ref - 1, caller, line);
#endif
	net_buf_unref(frag);
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
struct net_buf *net_pkt_frag_del_debug(struct net_pkt *pkt,
				       struct net_buf *parent,
				       struct net_buf *frag,
				       const char *caller, int line)
#else
struct net_buf *net_pkt_frag_del(struct net_pkt *pkt,
				 struct net_buf *parent,
				 struct net_buf *frag)
#endif
{
#if defined(CONFIG_NET_DEBUG_NET_PKT)
	NET_DBG("pkt %p parent %p frag %p ref %u (%s:%d)",
		pkt, parent, frag, frag->ref, caller, line);
#endif

	if (pkt->frags == frag && !parent) {
		struct net_buf *tmp;

		tmp = net_buf_frag_del(NULL, frag);
		pkt->frags = tmp;

		return tmp;
	}

	return net_buf_frag_del(parent, frag);
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
void net_pkt_frag_add_debug(struct net_pkt *pkt, struct net_buf *frag,
			    const char *caller, int line)
#else
void net_pkt_frag_add(struct net_pkt *pkt, struct net_buf *frag)
#endif
{
	NET_DBG("pkt %p frag %p (%s:%d)", pkt, frag, caller, line);

	/* We do not use net_buf_frag_add() as this one will refcount
	 * the frag once more if !pkt->frags
	 */
	if (!pkt->frags) {
		pkt->frags = frag;
		return;
	}

	net_buf_frag_insert(net_buf_frag_last(pkt->frags), frag);
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
void net_pkt_frag_insert_debug(struct net_pkt *pkt, struct net_buf *frag,
			       const char *caller, int line)
#else
void net_pkt_frag_insert(struct net_pkt *pkt, struct net_buf *frag)
#endif
{
	NET_DBG("pkt %p frag %p (%s:%d)", pkt, frag, caller, line);

	net_buf_frag_last(frag)->frags = pkt->frags;
	pkt->frags = frag;
}

struct net_buf *net_pkt_copy(struct net_pkt *pkt, size_t amount,
			     size_t reserve, s32_t timeout)
{
	struct net_buf *frag, *first, *orig;
	u8_t *orig_data;
	size_t orig_len;

	orig = pkt->frags;

	frag = net_pkt_get_frag(pkt, timeout);
	if (!frag) {
		return NULL;
	}

	if (reserve > net_buf_tailroom(frag)) {
		NET_ERR("Reserve %zu is too long, max is %zu",
			reserve, net_buf_tailroom(frag));
		net_pkt_frag_unref(frag);
		return NULL;
	}

	net_buf_add(frag, reserve);

	first = frag;

	NET_DBG("Copying frag %p with %zu bytes and reserving %zu bytes",
		first, amount, reserve);

	if (!orig->len) {
		/* No data in the first fragment in the original message */
		NET_DBG("Original fragment empty!");
		return frag;
	}

	orig_len = orig->len;
	orig_data = orig->data;

	while (orig && amount) {
		int left_len = net_buf_tailroom(frag);
		int copy_len;

		if (amount > orig_len) {
			copy_len = orig_len;
		} else {
			copy_len = amount;
		}

		if ((copy_len - left_len) >= 0) {
			/* Just copy the data from original fragment
			 * to new fragment. The old data will fit the
			 * new fragment and there could be some space
			 * left in the new fragment.
			 */
			amount -= left_len;

			memcpy(net_buf_add(frag, left_len), orig_data,
			       left_len);

			if (!net_buf_tailroom(frag)) {
				/* There is no space left in copy fragment.
				 * We must allocate a new one.
				 */
				struct net_buf *new_frag =
					net_pkt_get_frag(pkt, timeout);
				if (!new_frag) {
					net_pkt_frag_unref(first);
					return NULL;
				}

				net_buf_frag_add(frag, new_frag);

				frag = new_frag;
			}

			orig_len -= left_len;
			orig_data += left_len;

			continue;
		} else {
			/* We should be at the end of the original buf
			 * fragment list.
			 */
			amount -= copy_len;

			memcpy(net_buf_add(frag, copy_len), orig_data,
			       copy_len);
		}

		orig = orig->frags;
		if (orig) {
			orig_len = orig->len;
			orig_data = orig->data;
		}
	}

	return first;
}

int net_frag_linear_copy(struct net_buf *dst, struct net_buf *src,
			 u16_t offset, u16_t len)
{
	u16_t to_copy;
	u16_t copied;

	if (dst->size < len) {
		return -ENOMEM;
	}

	/* find the right fragment to start copying from */
	while (src && offset >= src->len) {
		offset -= src->len;
		src = src->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (src && len > 0) {
		to_copy = min(len, src->len - offset);
		memcpy(dst->data + copied, src->data + offset, to_copy);

		copied += to_copy;
		/* to_copy is always <= len */
		len -= to_copy;
		src = src->frags;
		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	if (len > 0) {
		return -ENOMEM;
	}

	dst->len = copied;

	return 0;
}

int net_frag_linearize(u8_t *dst, size_t dst_len, struct net_pkt *src,
			 u16_t offset, u16_t len)
{
	struct net_buf *frag;
	u16_t to_copy;
	u16_t copied;

	if (dst_len < (size_t)len) {
		return -ENOMEM;
	}

	frag = src->frags;

	/* find the right fragment to start copying from */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (frag && len > 0) {
		to_copy = min(len, frag->len - offset);
		memcpy(dst + copied, frag->data + offset, to_copy);

		copied += to_copy;

		/* to_copy is always <= len */
		len -= to_copy;
		frag = frag->frags;

		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	if (len > 0) {
		return -ENOMEM;
	}

	return copied;
}

#if defined(CONFIG_NET_NBUF_INNER)
/* When define CONFIG_NET_NBUF_INNER
  * will use inner small frag for ip/tcp header,
  * in compact, need move data to last frag
  */
bool net_pkt_compact(struct net_pkt *pkt)
{
	struct net_buf *frag, *next_frag, *first_frag, *last_frag;
	size_t total_len, len, reserve = 0;
	u8_t *dst, *src;

	NET_DBG("Compacting data in pkt %p", pkt);

	first_frag = pkt->frags;
	if ((!first_frag) || (!first_frag->frags)) {
		return true;
	}

	last_frag = net_buf_frag_last(first_frag);
	total_len = net_buf_frags_len(first_frag);

	if (net_buf_headroom(first_frag) > net_buf_headroom(last_frag)) {
		reserve = net_buf_headroom(first_frag) - net_buf_headroom(last_frag);
	}

	len = last_frag->len;
	dst = last_frag->data + total_len - 1 + reserve;
	src = last_frag->data + len - 1;
	while (len-- > 0) {
		*dst-- = *src--;
	}

	net_buf_reserve(last_frag, (net_buf_headroom(last_frag) + reserve));

	len = 0;
	frag = first_frag;
	while (frag != last_frag) {
		memcpy(last_frag->data + len, frag->data, frag->len);
		len += frag->len;

		next_frag = frag->frags;
		frag->frags = NULL;
		net_pkt_frag_unref(frag);
		frag = next_frag;
	}

	net_buf_add(last_frag, len);
	pkt->frags = last_frag;

	return true;
}
#else
bool net_pkt_compact(struct net_pkt *pkt)
{
	struct net_buf *frag, *prev;

	NET_DBG("Compacting data in pkt %p", pkt);

	frag = pkt->frags;
	prev = NULL;

	while (frag) {
		if (frag->frags) {
			/* Copy amount of data from next fragment to this
			 * fragment.
			 */
			size_t copy_len;

			copy_len = frag->frags->len;
			if (copy_len > net_buf_tailroom(frag)) {
				copy_len = net_buf_tailroom(frag);
			}

			memcpy(net_buf_tail(frag), frag->frags->data, copy_len);
			net_buf_add(frag, copy_len);

			memmove(frag->frags->data,
				frag->frags->data + copy_len,
				frag->frags->len - copy_len);

			frag->frags->len -= copy_len;

			/* Is there any more space in this fragment */
			if (net_buf_tailroom(frag)) {
				/* There is. This also means that the next
				 * fragment is empty as otherwise we could
				 * not have copied all data. Remove next
				 * fragment as there is no data in it any more.
				 */
				net_pkt_frag_del(pkt, frag, frag->frags);

				/* Then check next fragment */
				continue;
			}
		} else {
			if (!frag->len) {
				/* Remove the last fragment because there is no
				 * data in it.
				 */
				net_pkt_frag_del(pkt, prev, frag);

				break;
			}
		}

		prev = frag;
		frag = frag->frags;
	}

	return true;
}
#endif

/* This helper routine will append multiple bytes, if there is no place for
 * the data in current fragment then create new fragment and add it to
 * the buffer. It assumes that the buffer has at least one fragment.
 */
static inline u16_t net_pkt_append_bytes(struct net_pkt *pkt,
					 const u8_t *value,
					 u16_t len, s32_t timeout)
{
	struct net_buf *frag = net_buf_frag_last(pkt->frags);
	u16_t added_len = 0;

	do {
		u16_t count = min(len, net_buf_tailroom(frag));
		void *data = net_buf_add(frag, count);

		memcpy(data, value, count);
		len -= count;
		added_len += count;
		value += count;

		if (len == 0) {
			return added_len;
		}

		frag = net_pkt_get_frag(pkt, timeout);
		if (!frag) {
			return added_len;
		}

		net_pkt_frag_add(pkt, frag);
	} while (1);

	/* Unreachable */
	return 0;
}

u16_t net_pkt_append(struct net_pkt *pkt, u16_t len, const u8_t *data,
		    s32_t timeout)
{
	struct net_buf *frag;

	if (!pkt || !data) {
		return 0;
	}

	if (!pkt->frags) {
		frag = net_pkt_get_frag(pkt, timeout);
		if (!frag) {
			return 0;
		}

		net_pkt_frag_add(pkt, frag);
	}

	return net_pkt_append_bytes(pkt, data, len, timeout);
}

/* Helper routine to retrieve single byte from fragment and move
 * offset. If required byte is last byte in framgent then return
 * next fragment and set offset = 0.
 */
static inline struct net_buf *net_frag_read_byte(struct net_buf *frag,
						 u16_t offset,
						 u16_t *pos,
						 u8_t *data)
{
	if (data) {
		*data = frag->data[offset];
	}

	*pos = offset + 1;

	if (*pos >= frag->len) {
		*pos = 0;

		return frag->frags;
	}

	return frag;
}

/* Helper function to adjust offset in net_frag_read() call
 * if given offset is more than current fragment length.
 */
static inline struct net_buf *adjust_offset(struct net_buf *frag,
					    u16_t offset, u16_t *pos)
{
	if (!frag) {
		return NULL;
	}

	while (frag) {
		if (offset == frag->len) {
			*pos = 0;

			return frag->frags;
		} else if (offset < frag->len) {
			*pos = offset;

			return frag;
		}

		offset -= frag->len;
		frag = frag->frags;
	}

	NET_ERR("Invalid offset (%u), failed to adjust", offset);

	return NULL;
}

struct net_buf *net_frag_read(struct net_buf *frag, u16_t offset,
			      u16_t *pos, u16_t len, u8_t *data)
{
	u16_t copy = 0;

	frag = adjust_offset(frag, offset, pos);
	if (!frag) {
		goto error;
	}

	while (len-- > 0 && frag) {
		if (data) {
			frag = net_frag_read_byte(frag, *pos,
						  pos, data + copy++);
		} else {
			frag = net_frag_read_byte(frag, *pos, pos, NULL);
		}

		/* Error: Still reamining length to be read, but no data. */
		if (!frag && len) {
			NET_ERR("Not enough data to read");
			goto error;
		}
	}

	return frag;

error:
	*pos = 0xffff;

	return NULL;
}

struct net_buf *net_frag_read_be16(struct net_buf *frag, u16_t offset,
				   u16_t *pos, u16_t *value)
{
	struct net_buf *ret_frag;
	u8_t v16[2];

	ret_frag = net_frag_read(frag, offset, pos, sizeof(u16_t), v16);

	*value = v16[0] << 8 | v16[1];

	return ret_frag;
}

struct net_buf *net_frag_read_be32(struct net_buf *frag, u16_t offset,
				   u16_t *pos, u32_t *value)
{
	struct net_buf *ret_frag;
	u8_t v32[4];

	ret_frag = net_frag_read(frag, offset, pos, sizeof(u32_t), v32);

	*value = v32[0] << 24 | v32[1] << 16 | v32[2] << 8 | v32[3];

	return ret_frag;
}

static inline struct net_buf *check_and_create_data(struct net_pkt *pkt,
						    struct net_buf *data,
						    s32_t timeout)
{
	struct net_buf *frag;

	if (data) {
		return data;
	}

	frag = net_pkt_get_frag(pkt, timeout);
	if (!frag) {
		return NULL;
	}

	net_pkt_frag_add(pkt, frag);

	return frag;
}

static inline struct net_buf *adjust_write_offset(struct net_pkt *pkt,
						  struct net_buf *frag,
						  u16_t offset,
						  u16_t *pos,
						  s32_t timeout)
{
	u16_t tailroom;

	do {
		frag = check_and_create_data(pkt, frag, timeout);
		if (!frag) {
			return NULL;
		}

		/* Offset is less than current fragment length, so new data
		 *  will start from this "offset".
		 */
		if (offset < frag->len) {
			*pos = offset;

			return frag;
		}

		/* Offset is equal to fragment length. If some tailtoom exists,
		 * offset start from same fragment otherwise offset starts from
		 * beginning of next fragment.
		 */
		if (offset == frag->len) {
			if (net_buf_tailroom(frag)) {
				*pos = offset;

				return frag;
			}

			*pos = 0;

			return check_and_create_data(pkt, frag->frags,
						     timeout);
		}

		/* If the offset is more than current fragment length, remove
		 * current fragment length and verify with tailroom in the
		 * current fragment. From here on create empty (space/fragments)
		 * to reach proper offset.
		 */
		if (offset > frag->len) {

			offset -= frag->len;
			tailroom = net_buf_tailroom(frag);

			if (offset < tailroom) {
				/* Create empty space */
				net_buf_add(frag, offset);

				*pos = frag->len;

				return frag;
			}

			if (offset == tailroom) {
				/* Create empty space */
				net_buf_add(frag, tailroom);

				*pos = 0;

				return check_and_create_data(pkt,
							     frag->frags,
							     timeout);
			}

			if (offset > tailroom) {
				/* Creating empty space */
				net_buf_add(frag, tailroom);
				offset -= tailroom;

				frag = check_and_create_data(pkt,
							     frag->frags,
							     timeout);
			}
		}

	} while (1);

	return NULL;
}

struct net_buf *net_pkt_write(struct net_pkt *pkt, struct net_buf *frag,
			      u16_t offset, u16_t *pos,
			      u16_t len, u8_t *data,
			      s32_t timeout)
{
	if (!pkt) {
		NET_ERR("Invalid packet");
		goto error;
	}

	frag = adjust_write_offset(pkt, frag, offset, &offset, timeout);
	if (!frag) {
		NET_DBG("Failed to adjust offset (%u)", offset);
		goto error;
	}

	do {
		u16_t space = frag->size - net_buf_headroom(frag) - offset;
		u16_t count = min(len, space);
		int size_to_add;

		memcpy(frag->data + offset, data, count);

		/* If we are overwriting on already available space then need
		 * not to update the length, otherwise increase it.
		 */
		size_to_add = offset + count - frag->len;
		if (size_to_add > 0) {
			net_buf_add(frag, size_to_add);
		}

		len -= count;
		if (len == 0) {
			*pos = offset + count;

			return frag;
		}

		data += count;
		offset = 0;
		frag = frag->frags;

		if (!frag) {
			frag = net_pkt_get_frag(pkt, timeout);
			if (!frag) {
				goto error;
			}

			net_pkt_frag_add(pkt, frag);
		}
	} while (1);

error:
	*pos = 0xffff;

	return NULL;
}

static inline bool insert_data(struct net_pkt *pkt, struct net_buf *frag,
			       struct net_buf *temp, u16_t offset,
			       u16_t len, u8_t *data,
			       s32_t timeout)
{
	struct net_buf *insert;

	do {
		u16_t count = min(len, net_buf_tailroom(frag));

		/* Copy insert data */
		memcpy(frag->data + offset, data, count);
		net_buf_add(frag, count);

		len -= count;
		if (len == 0) {
			/* Once insertion is done, then add the data if
			 * there is anything after original insertion
			 * offset.
			 */
			if (temp) {
				net_buf_frag_insert(frag, temp);
			}

			/* As we are creating temporary buffers to cache,
			 * compact the fragments to save space.
			 */
			net_pkt_compact(pkt);

			return true;
		}

		data += count;
		offset = 0;

		insert = net_pkt_get_frag(pkt, timeout);
		if (!insert) {
			return false;
		}

		net_buf_frag_insert(frag, insert);
		frag = insert;

	} while (1);

	return false;
}

static inline struct net_buf *adjust_insert_offset(struct net_buf *frag,
						   u16_t offset,
						   u16_t *pos)
{
	if (!frag) {
		NET_ERR("Invalid fragment");
		return NULL;
	}

	while (frag) {
		if (offset == frag->len) {
			*pos = 0;

			return frag->frags;
		}

		if (offset < frag->len) {
			*pos = offset;

			return frag;
		}

		if (offset > frag->len) {
			if (frag->frags) {
				offset -= frag->len;
				frag = frag->frags;
			} else {
				return NULL;
			}
		}
	}

	NET_ERR("Invalid offset, failed to adjust");

	return NULL;
}

bool net_pkt_insert(struct net_pkt *pkt, struct net_buf *frag,
		    u16_t offset, u16_t len, u8_t *data,
		    s32_t timeout)
{
	struct net_buf *temp = NULL;
	u16_t bytes;

	if (!pkt) {
		return false;
	}

	frag = adjust_insert_offset(frag, offset, &offset);
	if (!frag) {
		return false;
	}

	/* If there is any data after offset, store in temp fragment and
	 * add it after insertion is completed.
	 */
	bytes = frag->len - offset;
	if (bytes) {
		temp = net_pkt_get_frag(pkt, timeout);
		if (!temp) {
			return false;
		}

		memcpy(net_buf_add(temp, bytes), frag->data + offset, bytes);

		frag->len -= bytes;
	}

	/* Insert data into current(frag) fragment from "offset". */
	return insert_data(pkt, frag, temp, offset, len, data, timeout);
}

int net_pkt_split(struct net_pkt *pkt, struct net_buf *orig_frag,
		  u16_t len, struct net_buf **fragA,
		  struct net_buf **fragB, s32_t timeout)
{
	if (!pkt || !orig_frag) {
		return -EINVAL;
	}

	NET_ASSERT(fragA && fragB);

	if (len == 0) {
		*fragA = NULL;
		*fragB = NULL;
		return 0;
	}

	if (len > orig_frag->len) {
		*fragA = net_pkt_get_frag(pkt, timeout);
		if (!*fragA) {
			return -ENOMEM;
		}

		memcpy(net_buf_add(*fragA, orig_frag->len), orig_frag->data,
		       orig_frag->len);

		*fragB = NULL;
		return 0;
	}

	*fragA = net_pkt_get_frag(pkt, timeout);
	if (!*fragA) {
		return -ENOMEM;
	}

	*fragB = net_pkt_get_frag(pkt, timeout);
	if (!*fragB) {
		net_pkt_frag_unref(*fragA);
		*fragA = NULL;
		return -ENOMEM;
	}

	memcpy(net_buf_add(*fragA, len), orig_frag->data, len);
	memcpy(net_buf_add(*fragB, orig_frag->len - len),
	       orig_frag->data + len, orig_frag->len - len);

	return 0;
}

void net_pkt_get_info(struct k_mem_slab **rx,
		      struct k_mem_slab **tx,
		      struct net_buf_pool **rx_data,
		      struct net_buf_pool **tx_data)
{
	if (rx) {
		*rx = &rx_pkts;
	}

	if (tx) {
		*tx = &tx_pkts;
	}

	if (rx_data) {
		*rx_data = &rx_bufs;
	}

	if (tx_data) {
		*tx_data = &tx_bufs;
	}
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
void net_pkt_print_info(u32_t flag)
{
	int i;
	struct k_mem_slab *slab;
	struct net_buf_pool *pool;

	printk("Fragment length %d bytes\n", CONFIG_NET_BUF_DATA_SIZE);
	printk("Network buffer pools:\n");

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	printk("Address\t\tSize\tCount\tAvail\tName\n");

	slab = &rx_pkts;
	printk("%p\t%zu\t%d\t%u\tRX\n",
		   slab, slab->num_blocks * slab->block_size,
		   slab->num_blocks, k_mem_slab_num_free_get(slab));

	slab = &tx_pkts;
	printk("%p\t%zu\t%d\t%u\tTX\n",
		   slab, slab->num_blocks * slab->block_size,
		   slab->num_blocks, k_mem_slab_num_free_get(slab));

	pool = &rx_bufs;
	printk("%p\t%d\t%d(%d)\t%d\tDATA(%s)\n",
		   pool, pool->pool_size, pool->buf_count,
		   pool->uninit_count, pool->avail_count, pool->name);

	pool = &tx_bufs;
	printk("%p\t%d\t%d(%d)\t%d\tDATA(%s)\n",
		   pool, pool->pool_size, pool->buf_count,
		   pool->uninit_count, pool->avail_count, pool->name);

#if defined(CONFIG_NET_BUF_SHARE)
	pool = &share_bufs;
	printk("%p\t%d\t%d+%d(%d)\t%d\tDATA(%s)\n",
		   pool, pool->pool_size, pool->buf_count, dynamic_frag_cnt,
		   pool->uninit_count, pool->avail_count, pool->name);
#endif

#if defined(CONFIG_NET_NBUF_INNER)
	pool = &inner_bufs;
	printk("%p\t%d\t%d(%d)\t%d\tDATA(%s)\n",
		   pool, pool->pool_size, pool->buf_count,
		   pool->uninit_count, pool->avail_count, pool->name);
#endif
#endif

	if (flag) {
		printk("\nPkt from rx/tx\n");
		for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
			if ((!net_pkt_allocs[i].in_use) || (!net_pkt_allocs[i].is_pkt)) {
				continue;
			}

			printk("pkt:%p, pkt->frag:%p, appdatalen:%d,\talloc:%s, line:%d\n",
					net_pkt_allocs[i].pkt, net_pkt_allocs[i].pkt->frags,
					net_pkt_allocs[i].pkt->appdatalen, net_pkt_allocs[i].func_alloc,
					net_pkt_allocs[i].line_alloc);
		}

		printk("\nfrag from rx/tx/share/inner\n");
		for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
			if ((!net_pkt_allocs[i].in_use) || net_pkt_allocs[i].is_pkt) {
				continue;
			}

			printk("frag:%p, %s,\tref:%d, flags:%d, frags:%p, alloc:%s, line:%d\n",
					net_pkt_allocs[i].buf, net_buf_pool_get(net_pkt_allocs[i].buf->pool_id)->name,
					net_pkt_allocs[i].buf->ref, net_pkt_allocs[i].buf->flags, net_pkt_allocs[i].buf->frags,
					net_pkt_allocs[i].func_alloc, net_pkt_allocs[i].line_alloc);
		}
		printk("\n");
	}
}

static void net_pkt_buf_set_caller_line(void *alloc_data, const char *caller,
				  int line)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].alloc_data != alloc_data) {
			continue;
		}

		if (!net_pkt_allocs[i].in_use) {
			NET_ERR("*** ERROR *** %s %p is freed "
				"already by %s():%d (%s():%d)", (net_pkt_allocs[i].is_pkt? "pkt" : "frag"),
				alloc_data, net_pkt_allocs[i].func_free, net_pkt_allocs[i].line_free,
				caller, line);
		} else {
			net_pkt_allocs[i].func_alloc = caller;
			net_pkt_allocs[i].line_alloc = line;
		}

		break;
	}

	if (i == MAX_NET_PKT_ALLOCS) {
		NET_ERR("*** ERROR *** Can't find pkt/frag:%p", alloc_data);
	}
}

void net_pktbuf_set_caller(void *alloc_data, const char *caller,
				  int line)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].alloc_data != alloc_data) {
			continue;
		}

		net_pkt_buf_set_caller_line(alloc_data, caller, line);

		if (net_pkt_allocs[i].in_use && net_pkt_allocs[i].is_pkt &&
			(net_pkt_allocs[i].pkt->frags != NULL)) {
			net_pkt_buf_set_caller_line(net_pkt_allocs[i].pkt->frags, caller, line);
		}

		break;
	}

	if (i == MAX_NET_PKT_ALLOCS) {
		NET_ERR("*** ERROR *** Can't find pkt/frag:%p", alloc_data);
	}
}

void net_pkt_print(void)
{
	NET_DBG("TX %u RX %u RDATA %d TDATA %d",
		k_mem_slab_num_free_get(&tx_pkts),
		k_mem_slab_num_free_get(&rx_pkts),
		rx_bufs.avail_count, tx_bufs.avail_count);
}
#endif /* CONFIG_NET_DEBUG_NET_PKT */

struct net_buf *net_frag_get_pos(struct net_pkt *pkt,
				 u16_t offset,
				 u16_t *pos)
{
	struct net_buf *frag;

	frag = net_frag_skip(pkt->frags, offset, pos, 0);
	if (!frag) {
		return NULL;
	}

	return frag;
}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
static void too_short_msg(char *msg, struct net_pkt *pkt, u16_t offset,
			  size_t extra_len)
{
	size_t total_len = net_buf_frags_len(pkt->frags);
	size_t hdr_len = net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt);

	if (total_len != (hdr_len + extra_len)) {
		/* Print info how many bytes past the end we tried to print */
		NET_ERR("%s: IP hdr %d ext len %d offset %d pos %zd total %zd",
			msg, net_pkt_ip_hdr_len(pkt),
			net_pkt_ipv6_ext_len(pkt),
			offset, hdr_len + extra_len, total_len);
	}
}
#else
#define too_short_msg(...)
#endif

struct net_icmp_hdr *net_pkt_icmp_data(struct net_pkt *pkt)
{
	struct net_buf *frag;
	u16_t offset;

	frag = net_frag_get_pos(pkt,
				net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt),
				&offset);
	if (!frag) {
		/* We tried to read past the end of the data */
		too_short_msg("icmp data", pkt, offset, 0);
		return NULL;
	}

	return (struct net_icmp_hdr *)(frag->data + offset);
}

u8_t *net_pkt_icmp_opt_data(struct net_pkt *pkt, size_t opt_len)
{
	struct net_buf *frag;
	u16_t offset;

	frag = net_frag_get_pos(pkt,
				net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) + opt_len,
				&offset);
	if (!frag) {
		/* We tried to read past the end of the data */
		too_short_msg("icmp opt data", pkt, offset, opt_len);
		return NULL;
	}

	return frag->data + offset;
}

struct net_udp_hdr *net_pkt_udp_data(struct net_pkt *pkt)
{
	struct net_buf *frag;
	u16_t offset;

	frag = net_frag_get_pos(pkt,
				net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt),
				&offset);
	if (!frag) {
		/* We tried to read past the end of the data */
		too_short_msg("udp data", pkt, offset, 0);
		return NULL;
	}

	return (struct net_udp_hdr *)(frag->data + offset);
}

struct net_tcp_hdr *net_pkt_tcp_data(struct net_pkt *pkt)
{
	struct net_buf *frag;
	u16_t offset;

	frag = net_frag_get_pos(pkt,
				net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt),
				&offset);
	if (!frag) {
		/* We tried to read past the end of the data */
		too_short_msg("tcp data", pkt, offset, 0);
		return NULL;
	}

	return (struct net_tcp_hdr *)(frag->data + offset);
}

struct net_pkt *net_pkt_clone(struct net_pkt *pkt, s32_t timeout)
{
	struct net_pkt *clone;
	struct net_buf *frag;
	u16_t pos;

	if (!pkt) {
		return NULL;
	}

	clone = net_pkt_get_reserve_tx(0, timeout);
	if (!clone) {
		return NULL;
	}

	clone->frags = net_pkt_copy_all(pkt, 0, timeout);
	if (!clone->frags) {
		net_pkt_unref(clone);
		return NULL;
	}

	clone->slab = pkt->slab;
	clone->context = pkt->context;
	clone->token = pkt->token;
	clone->iface = pkt->iface;

	frag = net_frag_get_pos(clone, net_pkt_ip_hdr_len(pkt), &pos);

	net_pkt_set_appdata(clone, frag->data + pos);
	net_pkt_set_appdatalen(clone, net_pkt_appdatalen(pkt));
	net_pkt_set_next_hdr(clone, NULL);
	net_pkt_set_ip_hdr_len(clone, net_pkt_ip_hdr_len(pkt));

	memcpy(&clone->lladdr_src, &pkt->lladdr_src, sizeof(clone->lladdr_src));
	memcpy(&clone->lladdr_dst, &pkt->lladdr_dst, sizeof(clone->lladdr_dst));

	net_pkt_set_family(clone, net_pkt_family(pkt));

#if defined(CONFIG_NET_IPV6)
	clone->ipv6_hop_limit = pkt->ipv6_hop_limit;
	clone->ipv6_ext_len = pkt->ipv6_ext_len;
	clone->ipv6_ext_opt_len = pkt->ipv6_ext_opt_len;
	clone->ipv6_prev_hdr_start = pkt->ipv6_prev_hdr_start;
#endif

	NET_DBG("Cloned %p to %p", pkt, clone);

	return clone;
}

void net_pkt_init(void)
{
	NET_DBG("Allocating %u RX (%zu bytes), %u TX (%zu bytes), "
		"%d RX data (%u bytes) and %d TX data (%u bytes) buffers",
		k_mem_slab_num_free_get(&rx_pkts),
		(size_t)(k_mem_slab_num_free_get(&rx_pkts) *
			 sizeof(struct net_pkt)),
		k_mem_slab_num_free_get(&tx_pkts),
		(size_t)(k_mem_slab_num_free_get(&tx_pkts) *
			 sizeof(struct net_pkt)),
		get_frees(&rx_bufs), rx_bufs.pool_size,
		get_frees(&tx_bufs), tx_bufs.pool_size);
}

#define NET_PKT_ENOUTH_BUF_CNT		3

bool net_pkt_enough_buf(void)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
#if defined(CONFIG_NET_BUF_SHARE)
	struct net_buf_pool *pool= &share_bufs;

	if (pool->avail_count > NET_PKT_ENOUTH_BUF_CNT) {
		return true;
	} else {
		return false;
	}
#else
	return true;
#endif
#else
	return true;
#endif
}

#if defined(CONFIG_NET_DYNAMIC_FRAG)
bool net_nbuf_add_dynamic_frag(void)
{
	uint8_t i;
	unsigned int key;
	struct net_buf *frag;
	struct net_buf_pool *pool = &share_bufs;

	if (add_frag_flag) {
		printk("Dynamic frag already add!\n");
		return false;
	}

	add_frag_flag = true;

	key = irq_lock();
	release_frag_flag = false;
	dynamic_frag_cnt = 0;

	for (i=0; i<MAX_DYNAMIC_COUNT; i++) {
		/* buf = mem_malloc(DYNAMIC_NBUF_SIZE) */
		frag = (struct net_buf *)dynamic_frag[i];
		if (!frag) {
			break;
		}

		memset(frag, 0, DYNAMIC_FRAG_SIZE);
		frag->pool_id = net_buf_pool_id(pool);
		frag->size = pool->buf_size;

		/* Add buf to data buffers pool lifo */
		pDynamic_frag[i] = frag;
		dynamic_frag_cnt++;
#if defined(CONFIG_NET_DEBUG_NET_PKT)
		pool->avail_count++;
#endif
		net_buf_destroy(frag);
	}

	for (; i<MAX_DYNAMIC_COUNT; i++) {
		pDynamic_frag[i] = NULL;
	}

	irq_unlock(key);

	printk("Add dynamic frag:%d!!!\n", dynamic_frag_cnt);
	return true;
}

bool free_frags_func_hook(struct net_buf *frag)
{
	uint8_t i;
	unsigned int key;
#if defined(CONFIG_NET_DEBUG_NET_PKT)
	struct net_buf_pool *pool = &share_bufs;
#endif

	if (!release_frag_flag || (frag == NULL)) {
		return false;
	}

	for (i=0; i<MAX_DYNAMIC_COUNT; i++) {
		if (pDynamic_frag[i] == frag) {
			break;
		}
	}

	if (i == MAX_DYNAMIC_COUNT) {
		return false;
	}

	key = irq_lock();

	/* release the buf */
	dynamic_frag_cnt--;
	pDynamic_frag[i] = NULL;
#if defined(CONFIG_NET_DEBUG_NET_PKT)
	pool->avail_count--;
#endif
	// mem_free(frag);

	if (dynamic_frag_cnt == 0) {
		release_frag_flag = false;
		k_sem_give(&sem_frag_release);
	}

	irq_unlock(key);

	return true;
}

bool net_nbuf_release_dynamic_frag(void)
{
	uint8_t i;
	struct net_buf *frag;
	unsigned int key;
#if defined(CONFIG_NET_DEBUG_NET_PKT)
	struct net_buf_pool *pool = &share_bufs;
#endif

	if (!add_frag_flag) {
		printk("Dynamic frag already release\n");
		return false;
	}

	add_frag_flag = false;

	key = irq_lock();
	release_frag_flag = true;
	for (i=0; i<MAX_DYNAMIC_COUNT; i++) {
		frag = pDynamic_frag[i];
		if (frag && k_lifo_remove(&net_buf_pool_get(frag->pool_id)->free, frag)) {
			/* release the buf */
			dynamic_frag_cnt--;
			pDynamic_frag[i] = NULL;
#if defined(CONFIG_NET_DEBUG_NET_PKT)
			pool->avail_count--;
#endif
			// mem_free(frag);
		}
	}

	if (dynamic_frag_cnt == 0) {
		release_frag_flag = false;
		k_sem_give(&sem_frag_release);
	} else {
		k_sem_reset(&sem_frag_release);
	}

	irq_unlock(key);

	k_sem_take(&sem_frag_release, K_FOREVER);
	printk("All dynamic frag release!!!\n");

	return true;
}
#endif
