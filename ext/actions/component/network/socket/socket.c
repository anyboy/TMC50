/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: jiangbin<jiangbin@actions-semi.com>
 *
 * Change log:
 * 2017/12/22 Created by jiangbin
 */
#include <kernel.h>
#include <logging/sys_log.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <mem_manager.h>

#include "socket.h"
#define SOCK_EOF 1
#define SOCK_NONBLOCK 2

#define SET_ERRNO(x) \
	{ int _err = x; if (_err < 0) { errno = -_err; return -1; } }

#define sock_is_eof(ctx) sock_get_flag(ctx, SOCK_EOF)
#define sock_is_nonblock(ctx) sock_get_flag(ctx, SOCK_NONBLOCK)
#define TCP_CB_PKT_LEN 25

static struct k_poll_event poll_events[3];

static void zpy_sock_received_cb(struct net_context *ctx, struct net_pkt *pkt,
			      int status, void *user_data);

static inline void sock_set_flag(struct net_context *ctx, unsigned int mask,
				 unsigned int flag)
{
	unsigned int val = POINTER_TO_INT(ctx->user_data);
	val = (val & mask) | flag;
	(ctx)->user_data = UINT_TO_POINTER(val);
}

static inline unsigned int sock_get_flag(struct net_context *ctx, unsigned int mask)
{
	return POINTER_TO_INT(ctx->user_data) & mask;
}

static inline int _k_fifo_wait_non_empty(struct k_fifo *fifo, int32_t timeout)
{
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY, fifo),
	};

	return k_poll(events, ARRAY_SIZE(events), timeout);
}

static void zpy_sock_flush_queue(zpy_socket_t *socket, struct k_fifo* recv_q)
{
	SYS_LOG_DBG("flush_queue");
	void *p;

	/* recv_q and accept_q are shared via a union */
	while ((p = k_fifo_get(recv_q, K_NO_WAIT)) != NULL) {
		SYS_LOG_DBG("discarding pkt %p", p);
		net_pkt_unref(p);
	}
}


static int init_local_addr(zpy_socket_t *socket)
{

	if (socket->remote.sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		struct in6_addr *dst = &net_sin6(&socket->remote)->sin6_addr;

		net_ipaddr_copy(&net_sin6(&socket->local)->sin6_addr,
				net_if_ipv6_select_src_addr(NULL, dst));
#else
		return -EPFNOSUPPORT;
#endif
	} else if (socket->remote.sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_if_get_default();

		/* For IPv4 we take the first address in the interface */
		net_ipaddr_copy(&net_sin(&socket->local)->sin_addr,
				&iface->ipv4.unicast[0].address.in_addr);
#else
		return -EPFNOSUPPORT;
#endif
	}
	socket->local.sa_family = socket->remote.sa_family;
	return 0;

}


unsigned int zpy_sock_socket(int family, int type, int proto)
{
	int ret = 0;
	unsigned int fd = 0;
	zpy_socket_t *socket = mem_malloc(sizeof(zpy_socket_t));

	if (socket == NULL) {
		return -ENOMEM;
	}

	SYS_LOG_DBG("socket=%p uint=%u",socket,POINTER_TO_UINT(socket));

	ret = net_context_get(family, type, proto, &socket->ctx);
	if (!ret) {
		k_fifo_init(&socket->ctx->recv_q);
		fd = POINTER_TO_UINT(socket);
		socket->context_release = 0;
	} else {
		SYS_LOG_ERR("net_context_get error %d",ret);
	}

	SYS_LOG_DBG("tcp =%p fd=%u fifo=%p",socket->ctx,fd,&socket->ctx->recv_q);
	return fd;
}

int zpy_sock_destory(unsigned int sock)
{
	int ret = 0;
	zpy_socket_t *socket  = UINT_TO_POINTER(sock);
	SYS_LOG_DBG("socket=%p uint=%u",socket,sock);

	if (socket != NULL) {
		SYS_LOG_DBG("destory");
		mem_free(socket);
	}
	return ret;
}

static void zpy_sock_received_cb_free(struct net_context *ctx, struct net_pkt *pkt ,
			      int status, void *user_data)
{
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

int zpy_sock_close(unsigned int sock)
{
	int ret = 0, prio;
	zpy_socket_t *socket  = UINT_TO_POINTER(sock);
	SYS_LOG_DBG("socket=%p uint=%u",socket,sock);
	/* Reset callbacks to avoid any race conditions while
	 * flushing queues. No need to check return values here,
	 * as these are fail-mem_free operations and we're closing
	 * socket anyway.
	 */
	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	if (!socket->context_release) {
		net_context_recv(socket->ctx, zpy_sock_received_cb_free, K_NO_WAIT, NULL);
		zpy_sock_flush_queue(socket, &socket->ctx->recv_q);
		ret = net_context_put(socket->ctx);
	}

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	return ret;
}

static void zpy_sock_received_cb(struct net_context *ctx, struct net_pkt *pkt ,
			      int status, void *user_data)
{
	zpy_socket_t *socket = (zpy_socket_t *)(user_data);

	if (!pkt) {
		/** Receive NULL packet,  lower responsible for unref context */
		socket->context_release = 1;
		/** Tcp close, flush context receive queue */
		zpy_sock_flush_queue(socket, &socket->ctx->recv_q);
		return;
	}

	if (net_pkt_appdatalen(pkt) == 0) {
		net_pkt_unref(pkt);
		SYS_LOG_DBG("netbuf datalen = 0 need drop");
		return;
	}

	if (net_pkt_appdatalen(pkt) == TCP_CB_PKT_LEN) {
		net_pkt_unref(pkt);
		SYS_LOG_DBG("cb.drop it");
		return;
	}

	SYS_LOG_DBG("put data");
	net_pktbuf_set_owner(pkt);
	k_fifo_put(&socket->ctx->recv_q, pkt);

	if (socket->notify_func) {
		socket->notify_func(socket->notify_param);
	}
}


int zpy_sock_connect(unsigned int sock, void *sockaddr, unsigned int addrlen)
{
	int ret=0;
	uint16_t timeout=4000;//1s
	zpy_socket_t *socket  = UINT_TO_POINTER(sock);

	init_local_addr(socket);

	ret = net_context_bind(socket->ctx,&socket->local,addrlen);
	if (ret) {
		SYS_LOG_ERR("bind err=%d",ret);
		return ret;
	}

	ret = net_context_connect(socket->ctx, (struct sockaddr *)sockaddr, addrlen, NULL, timeout,INT_TO_POINTER(AF_INET));
	if (!ret) {
		ret = net_context_recv(socket->ctx, zpy_sock_received_cb, K_NO_WAIT, socket);
	} else {
		printk("ERROR: failed net_context_connect, ret:%d\n", ret);
	}

	SYS_LOG_DBG("over ret=%d",ret);
	return ret;
}

int zpy_sock_send(unsigned int sock, const void *buf, unsigned int len, int flags)
{
	int err;
	struct net_pkt * send_pkt = NULL;
	uint16_t timeout=3000;	//3s
	#define SOCKET_SEND_CHECK_TIME	50

	zpy_socket_t *socket  = UINT_TO_POINTER(sock);
	size_t max_len = CONFIG_NET_BUF_DATA_SIZE;
	uint16_t remain = len;
	uint16_t sendlen = 0;

	if (sock_is_nonblock(socket->ctx)) {
		timeout = K_NO_WAIT;
	}

	if (!buf) {
		SYS_LOG_ERR("buf NULL");
		return -1;
	}
	SYS_LOG_DBG("size =%d",len);
	while (remain > 0) {
		if (socket->context_release) {
			errno = EIO;
			return -1;
		}

		sendlen = remain > max_len ? max_len : remain;
		send_pkt = net_pkt_get_tx(socket->ctx, timeout);
		if (!send_pkt) {
			SYS_LOG_WRN("net_pkt_get_tx error");
			errno = EAGAIN;
			return -1;
		}

		if (!net_pkt_append(send_pkt, sendlen, buf, timeout)) {
			net_pkt_unref(send_pkt);
			SYS_LOG_ERR("net_pkt_append error");
			errno = EAGAIN;
			return -1;
		}

		while (true) {
			if (socket->context_release) {
				err = -EIO;
				break;
			}

			err = net_context_send(send_pkt, /*cb*/NULL, timeout, NULL, NULL);
			if (err == -EAGAIN) {
				/* Only return -EAGAIN error can send the packet again */
				if (timeout > 0) {
					timeout = (timeout > SOCKET_SEND_CHECK_TIME)? (timeout - SOCKET_SEND_CHECK_TIME) : 0;
					os_sleep(SOCKET_SEND_CHECK_TIME);
					continue;
				}
			}

			break;
		}

		if (err < 0) {
			SYS_LOG_ERR("send error = %d",err);
			net_pkt_unref(send_pkt);
			errno = -err;
			return -1;
		}
		remain-=sendlen;
		buf+=sendlen;
	}
	return len;
}

static inline ssize_t zpy_sock_recv_stream(zpy_socket_t *socket, void *buf, size_t max_len)
{
	unsigned short recv_len = 0;
	int timeout = K_FOREVER;
	int res;
	unsigned short rx_len;
	uint16_t remain = max_len;

	if (sock_is_nonblock(socket->ctx)) {
		timeout = K_NO_WAIT;
	}
	do {
		struct net_pkt *rx;

		if (socket->context_release || sock_is_eof(socket->ctx)) {
			return 0;
		}

		if (remain == max_len) {
			res = _k_fifo_wait_non_empty(&socket->ctx->recv_q, timeout);
			if (res && res != -EAGAIN) {
				errno = -res;
				SYS_LOG_ERR(" EMPTY");
				goto OUT;
			}
		}

		rx = (struct net_pkt *)k_fifo_peek_head(&socket->ctx->recv_q);
		if (!rx) {
			/* Either timeout expired, or wait was cancelled
			 * due to connection closure by peer.
			 */
			SYS_LOG_DBG("NULL return from fifo");
			if (socket->context_release || sock_is_eof(socket->ctx)) {
				return 0;
			} else {
				errno = EAGAIN;
				goto OUT;
			}
		}

		rx_len = net_pkt_appdatalen(rx);

		if (rx_len == 0) {
			SYS_LOG_ERR("fifo item len =0 ,error");
			goto OUT;
		}

		recv_len = remain;

		if (rx_len < recv_len) {
			recv_len = rx_len;
		}

		/* Actually copy data to application buffer */
		memcpy((buf+(max_len - remain)), net_pkt_appdata(rx), recv_len);

		remain-=recv_len;
		rx_len -=recv_len;

		if (rx_len > 0) {
			net_pkt_set_appdatalen(rx,rx_len);
			net_pkt_set_appdata(rx,(net_pkt_appdata(rx) + recv_len));
		} else {
			k_fifo_get(&socket->ctx->recv_q, K_NO_WAIT); // finished process this nbuf
			net_pkt_unref(rx);
		}

	} while (remain != 0);

	OUT:
	return (max_len - remain);
}

int zpy_sock_recv(unsigned int sock, void *buf, unsigned int max_len, int flags)
{
	ARG_UNUSED(flags);
	SYS_LOG_DBG("reclen = %d",max_len);
	zpy_socket_t *socket  = UINT_TO_POINTER(sock);
	enum net_sock_type sock_type = net_context_get_type(socket->ctx);

	if (sock_type == SOCK_STREAM) {
		return zpy_sock_recv_stream(socket, buf, max_len);
	} else {
		__ASSERT(0, "Unknown socket type");
	}
	return -1;
}

void zpy_sock_set_notify_func(unsigned int sock,data_notify func,void *notify_param)
{
	zpy_socket_t *socket  = UINT_TO_POINTER(sock);
	if (socket) {
		socket->notify_func = func;
		socket->notify_param = notify_param;
	}
}

int zpy_sock_poll(struct zpy_sock_pollfd *fds, int nfds, int timeout)
{
	int i;
	int ret = 0;
	struct zpy_sock_pollfd *pfd;
	struct k_poll_event *pev;
	struct k_poll_event *pev_end = poll_events + ARRAY_SIZE(poll_events);

	if (timeout < 0) {
		timeout = K_FOREVER;
	}
	pev = poll_events;
	for (pfd = fds, i = nfds; i--; pfd++) {

		/* Per POSIX, negative fd's are just ignored */
		if (pfd->fd < 0) {
			continue;
		}

		if (pfd->events & ZSOCK_POLLIN) {
			zpy_socket_t *socket  = UINT_TO_POINTER(pfd->fd);

			if (pev == pev_end) {
				errno = ENOMEM;
				return -1;
			}

			if (socket->context_release) {
				errno = EIO;
				return -1;
			}

			pev->obj = &socket->ctx->recv_q;
			pev->type = K_POLL_TYPE_FIFO_DATA_AVAILABLE;
			pev->mode = K_POLL_MODE_NOTIFY_ONLY;
			pev->state = K_POLL_STATE_NOT_READY;
			pev++;
		}
	}

	ret = k_poll(poll_events, pev - poll_events, timeout);
	if (ret != 0 && ret != -EAGAIN) {
		errno = -ret;
		SYS_LOG_ERR("error ret =%d",ret);
		return -1;
	}

	ret = 0;

	pev = poll_events;
	for (pfd = fds, i = nfds; i--; pfd++) {
		pfd->revents = 0;

		if (pfd->fd < 0) {
			continue;
		}

		/* For now, assume that socket is always writable */
		if (pfd->events & ZSOCK_POLLOUT) {
			pfd->revents |= ZSOCK_POLLOUT;
		}

		if (pfd->events & ZSOCK_POLLIN) {

			if (pev->state != K_POLL_STATE_NOT_READY) {
				pfd->revents |= ZSOCK_POLLIN;
			}
			pev++;
		}

		if (pfd->revents != 0) {
			ret++;
		}
	}
	SYS_LOG_DBG("ret=%d",ret);
	return ret;
}

extern int net_dns_resolve(char *name, struct in_addr * retaddr);
extern void del_dns_cached(char *ip);

static int parse_addr(struct sockaddr *sock_addr, const char *addr, uint16_t server_port, u8_t *ipBuf)
{
	int rc = 0;
	SYS_LOG_DBG("server address: %s  port =%d\n", addr,server_port);
#ifdef CONFIG_NET_IPV6
	net_sin6(sock_addr)->sin6_port = htons(server_port);
	sock_addr->sa_family = AF_INET6;
	rc = net_addr_pton(AF_INET6, addr, ipBuf);
	if(rc)
	{
		SYS_LOG_ERR("Invalid IP6 address: %s\n", addr);
		return rc;
	}
#else
	net_sin(sock_addr)->sin_port = htons(server_port);
	sock_addr->sa_family = AF_INET;
	rc = net_addr_pton(AF_INET, addr, &(net_sin(sock_addr)->sin_addr));
	if (rc) {
		SYS_LOG_DBG("IP host name: %s\n", addr);
		rc = net_dns_resolve((char *)addr, &(net_sin(sock_addr)->sin_addr));
		if (rc == 0) {
			net_addr_ntop(AF_INET, &(net_sin(sock_addr)->sin_addr), ipBuf, NET_IPV4_ADDR_LEN);
			SYS_LOG_DBG("IP address: %s\n", ipBuf);
		}
	} else {
		memcpy(ipBuf, addr, strlen(addr));
	}
#endif
	return rc;
}

int zpy_sock_connect_by_host(unsigned int sock, void *hostaddr,uint16_t server_port)
{
	int ret, prio;
	zpy_socket_t *socket  = UINT_TO_POINTER(sock);
#ifdef CONFIG_NET_IPV6
	u8_t ipBuf[NET_IPV6_ADDR_LEN+1];
#else
	u8_t ipBuf[NET_IPV4_ADDR_LEN+1];
#endif

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	memset(ipBuf, 0, sizeof(ipBuf));
	ret = parse_addr(&socket->remote, hostaddr, server_port, ipBuf);
	if (ret == 0) {
		ret = zpy_sock_connect(sock,&socket->remote,socket->remote.sa_family == AF_INET?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6));
		if (ret) {
			del_dns_cached(ipBuf);
		}
	}

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	return ret;
}

int zpy_sock_inet_pton(unsigned int family, const char *src, void *dst)
{
	if (net_addr_pton(family, src, dst) == 0) {
		return 1;
	} else {
		return 0;
	}
}
