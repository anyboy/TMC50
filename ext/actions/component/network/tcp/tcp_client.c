/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <os_common_api.h>

#ifdef CONFIG_NET_TCP
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_if.h>
#include <net/net_ip.h>
#include <net/net_mgmt.h>
#include <mem_manager.h>
#include <misc/printk.h>
#include "tcp_client.h"

#ifdef CONFIG_TCP_KEEP_CONTEXT
#define TCP_KEEP_MAX			3
#define TCP_KEEP_RETYR_MAX		5
#define TCP_CONNECT_TIMEOUT		1000		/* 1000ms */
#define TCP_CONNECT_MAX_TIMEOUT	(1000*90)	/* 90s */
#define TCP_CONNECT_UNUSED_KEEPTIME		(1000*60*5)	/* 300s */

static struct tcp_keep_tcp tcpkeep[TCP_KEEP_MAX];
static OS_MUTEX_DEFINE(tcp_keep_lock);
static os_delayed_work tcp_keep_work;
#endif

u8_t ipReady = false;
static struct net_mgmt_event_callback tcp_client_mgmt_cb;

static void recv_cb(struct net_app_ctx *ctx,
				  struct net_pkt *pkt, int status, void *user_data)
{
	struct tcp_client_ctx *tcp_ctx = (struct tcp_client_ctx *)user_data;

	ARG_UNUSED(ctx);
	ARG_UNUSED(status);

	if (!pkt || net_pkt_appdatalen(pkt) == 0) {
		if (pkt) {
			net_pkt_unref(pkt);
		}
		return;
	}

	/* receive_cb must take ownership of the rx buffer */
	if (tcp_ctx->receive_cb) {
		tcp_ctx->receive_cb(tcp_ctx, pkt);
		return;
	}

	net_pkt_unref(pkt);
}

int tcp_connect_client(struct tcp_client_ctx *ctx, const char *server_addr,
		uint16_t server_port)
{
	int ret;

	ret = net_app_init_tcp_client(&ctx->app_ctx, NULL, NULL, server_addr, server_port,
				      ctx->timeout, ctx);
	if (ret < 0) {
		SYS_LOG_INF("Cannot init TCP client (%d)", ret);
		goto fail;
	}

	ret = net_app_set_cb(&ctx->app_ctx, NULL, recv_cb, NULL, NULL);
	if (ret < 0) {
		SYS_LOG_INF("Cannot set callbacks (%d)", ret);
		goto fail;
	}

	ret = net_app_connect(&ctx->app_ctx, ctx->timeout);
	if (ret < 0) {
		SYS_LOG_INF("Cannot connect TCP (%d)", ret);
		goto fail;
	}

	return ret;

fail:
	net_app_close(&ctx->app_ctx);
	net_app_release(&ctx->app_ctx);
	return ret;
}

int tcp_disconnect_client(struct tcp_client_ctx *ctx)
{
	int ret, prio;

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	ret = net_app_close(&ctx->app_ctx);
	ret |= net_app_release(&ctx->app_ctx);
	if (ret) {
		SYS_LOG_ERR("net app close or release error:%d\n", ret);
	}

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	return ret;
}

int tcp_connect(struct tcp_client_ctx **ppctx, const char *server_addr,
		uint16_t server_port)
{
	int rc, prio;
#ifdef CONFIG_TCP_KEEP_CONTEXT
	int i;
	struct sockaddr remote;
#endif

	if (!ipReady) {
		return -EIO;
	}

#ifdef CONFIG_TCP_KEEP_CONTEXT
	remote.sa_family = AF_INET;
	net_sin(&remote)->sin_port = htons(server_port);
	rc = net_addr_pton(AF_INET, server_addr,
					&net_sin(&remote)->sin_addr);
	if (rc < 0) {
		goto exit_cmp;
	}

	os_mutex_lock(&tcp_keep_lock, K_FOREVER);
	for (i=0; i<TCP_KEEP_MAX; i++) {
		if ((tcpkeep[i].state == TCP_KEEP_CONNECTED) &&
			!memcmp(&remote, &tcpkeep[i].remote, sizeof(struct sockaddr)) &&
			!memcmp(&remote, &tcpkeep[i].tcp_ctx->app_ctx.default_ctx->remote, sizeof(struct sockaddr))) {
			if (net_app_set_cb(&tcpkeep[i].tcp_ctx->app_ctx, NULL, recv_cb, NULL, NULL)) {
				tcp_disconnect_client(tcpkeep[i].tcp_ctx);
				mem_free(tcpkeep[i].tcp_ctx);
				memset(&tcpkeep[i], 0, sizeof(struct tcp_keep_tcp));
			} else {
				tcpkeep[i].tcp_ctx->pIOhandle = (*ppctx)->pIOhandle;
				tcpkeep[i].tcp_ctx->timeout = (*ppctx)->timeout;
				tcpkeep[i].tcp_ctx->receive_cb = (*ppctx)->receive_cb;

				mem_free(*ppctx);
				*ppctx = tcpkeep[i].tcp_ctx;
				memset(&tcpkeep[i], 0, sizeof(struct tcp_keep_tcp));

				os_mutex_unlock(&tcp_keep_lock);
				return 0;
			}
			break;
		} else if ((tcpkeep[i].state == TCP_KEEP_CONNECTED) &&
				memcmp(&tcpkeep[i].remote, &tcpkeep[i].tcp_ctx->app_ctx.default_ctx->remote, sizeof(struct sockaddr))) {
			printk("Diff keep ip: 0x%x, 0x%x\n", net_sin(&tcpkeep[i].remote)->sin_addr.s_addr,
					net_sin(&tcpkeep[i].tcp_ctx->app_ctx.default_ctx->remote)->sin_addr.s_addr);
		}
	}
	os_mutex_unlock(&tcp_keep_lock);
exit_cmp:
#endif

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	rc = tcp_connect_client(*ppctx, server_addr, server_port);

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	if (rc) {
		return rc;
	}

	return 0;
}

int tcp_disconnect(struct tcp_client_ctx *ctx)
{
	int ret;
#ifdef CONFIG_TCP_KEEP_CONTEXT
	uint8_t i;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->app_ctx.is_init) {
		return -ENOENT;
	}

	os_mutex_lock(&tcp_keep_lock, K_FOREVER);
	for (i=0; i<TCP_KEEP_MAX; i++) {
		if ((tcpkeep[i].state == TCP_KEEP_UNUSED) ||
			(tcpkeep[i].state == TCP_KEEP_DISCONNECT)) {
			continue;
		}

		if (!memcmp(&tcpkeep[i].remote, &ctx->app_ctx.default_ctx->remote,
				sizeof(struct sockaddr))) {
			/* Have one same connect in process, not need keep anothter */
			goto exit_keep;
		}
	}

	for (i=0; i<TCP_KEEP_MAX; i++) {
		if (tcpkeep[i].state != TCP_KEEP_UNUSED) {
			continue;
		}

		tcpkeep[i].state = TCP_KEEP_WAIT_CONNECT;
		memcpy(&tcpkeep[i].remote, &ctx->app_ctx.default_ctx->remote,
				sizeof(struct sockaddr));
		tcpkeep[i].lastUsetime = k_uptime_get_32();
		break;
	}

exit_keep:
	os_mutex_unlock(&tcp_keep_lock);

	os_delayed_work_cancel(&tcp_keep_work);
	os_delayed_work_submit(&tcp_keep_work, 10);
#endif

	ret = tcp_disconnect_client(ctx);
	return ret;
}

#ifdef CONFIG_TCP_KEEP_CONTEXT
static void tecp_keep_close_cb(struct net_app_ctx *ctx,
				   int status, void *user_data)
{
	struct tcp_client_ctx *tcp_ctx = (struct tcp_client_ctx *)user_data;
	struct tcp_keep_tcp *ktcp = tcp_ctx->pIOhandle;

	os_mutex_lock(&tcp_keep_lock, K_FOREVER);
	ktcp->state = TCP_KEEP_DISCONNECT;
	os_mutex_unlock(&tcp_keep_lock);
	os_delayed_work_cancel(&tcp_keep_work);
	os_delayed_work_submit(&tcp_keep_work, 10);
}

static void tcp_keep_connect_cb(struct net_app_ctx *ctx,
				     int status,
				     void *user_data)
{
	struct tcp_client_ctx *tcp_ctx = (struct tcp_client_ctx *)user_data;
	struct tcp_keep_tcp *ktcp = tcp_ctx->pIOhandle;

	ARG_UNUSED(ctx);

	if (status == 0) {
		os_mutex_lock(&tcp_keep_lock, K_FOREVER);
		ktcp->state = TCP_KEEP_CONNECTED;
		ktcp->rectime = os_uptime_get_32();
		os_mutex_unlock(&tcp_keep_lock);
		return;
	}
}

static int tcp_keep_connect(struct tcp_keep_tcp *ktcp)
{

	int ret;
	struct tcp_client_ctx *ctx;
	char buf[16];

	if (!ipReady) {
		return -EIO;
	}

	ctx = mem_malloc(sizeof(struct tcp_client_ctx));
	if (!ctx) {
		return -ENOMEM;
	}

	ctx->timeout = TCP_CONNECT_TIMEOUT;
	memset(buf, 0, sizeof(buf));
	net_addr_ntop(AF_INET, &net_sin(&ktcp->remote)->sin_addr, (char *)buf, sizeof(buf));

	ret = net_app_init_tcp_client(&ctx->app_ctx, NULL, NULL, buf,
					ntohs(net_sin(&ktcp->remote)->sin_port), ctx->timeout, ctx);
	if (ret < 0) {
		SYS_LOG_INF("Cannot init TCP client (%d)", ret);
		goto fail;
	}

	ret = net_app_set_cb(&ctx->app_ctx, tcp_keep_connect_cb, NULL, NULL, tecp_keep_close_cb);
	if (ret < 0) {
		SYS_LOG_INF("Cannot set callbacks (%d)", ret);
		goto fail;
	}

	ret = net_app_connect(&ctx->app_ctx, 0);
	if (ret < 0) {
		SYS_LOG_INF("Cannot connect TCP (%d)", ret);
		goto fail;
	}

	ctx->pIOhandle = ktcp;
	ktcp->tcp_ctx = ctx;
	return ret;

fail:
	net_app_close(&ctx->app_ctx);
	net_app_release(&ctx->app_ctx);
	mem_free(ctx);
	return ret;
}

static void tcp_keep_delaywork(os_work *work)
{
	uint8_t i;
	int rc;
	int32_t interval = TCP_CONNECT_MAX_TIMEOUT;

	os_mutex_lock(&tcp_keep_lock, K_FOREVER);
	for (i=0; i<TCP_KEEP_MAX; i++) {
		switch (tcpkeep[i].state) {
			case TCP_KEEP_DISCONNECT:
				if (tcpkeep[i].tcp_ctx) {
					tcp_disconnect_client(tcpkeep[i].tcp_ctx);
					mem_free(tcpkeep[i].tcp_ctx);
					tcpkeep[i].tcp_ctx = NULL;
				}

				tcpkeep[i].tryCnt = 0;
				tcpkeep[i].rectime = 0;
				tcpkeep[i].state = TCP_KEEP_WAIT_CONNECT;
				interval = min(interval, 10);		/* Start connect 10ms later */
				break;

			case TCP_KEEP_WAIT_CONNECT:
				rc = tcp_keep_connect(&tcpkeep[i]);
				if (rc) {
					if (tcpkeep[i].tryCnt++ > TCP_KEEP_RETYR_MAX) {
						memset(&tcpkeep[i], 0, sizeof(struct tcp_keep_tcp));
					}
				} else {
					tcpkeep[i].state = TCP_KEEP_CONNECTING;
					tcpkeep[i].rectime = os_uptime_get_32();
				}
				interval = min(interval, 1000);		/* Check every 1000ms */
				break;

			case TCP_KEEP_CONNECTING:
				if ((os_uptime_get_32() - tcpkeep[i].rectime) > TCP_CONNECT_TIMEOUT) {
					if (tcpkeep[i].tcp_ctx) {
						tcp_disconnect_client(tcpkeep[i].tcp_ctx);
						mem_free(tcpkeep[i].tcp_ctx);
						tcpkeep[i].tcp_ctx = NULL;
					}

					if (tcpkeep[i].tryCnt++ > TCP_KEEP_RETYR_MAX) {
						memset(&tcpkeep[i], 0, sizeof(struct tcp_keep_tcp));
					} else {
						tcpkeep[i].state = TCP_KEEP_WAIT_CONNECT;
					}
				}
				interval = min(interval, 1000);		/* Check every 1000ms */
				break;

			case TCP_KEEP_CONNECTED:
				if ((os_uptime_get_32() - tcpkeep[i].lastUsetime) > TCP_CONNECT_UNUSED_KEEPTIME) {
					/* Keep tcp connect never used after 60*5s, release */
					if (tcpkeep[i].tcp_ctx) {
						tcp_disconnect_client(tcpkeep[i].tcp_ctx);
						mem_free(tcpkeep[i].tcp_ctx);
						tcpkeep[i].tcp_ctx = NULL;
					}

					memset(&tcpkeep[i], 0, sizeof(struct tcp_keep_tcp));
					interval = min(interval, 10000);	/* Checkout connect timeout every 10s */
				} else if ((os_uptime_get_32() - tcpkeep[i].rectime) > TCP_CONNECT_MAX_TIMEOUT) {
					if (tcpkeep[i].tcp_ctx) {
						tcp_disconnect_client(tcpkeep[i].tcp_ctx);
						mem_free(tcpkeep[i].tcp_ctx);
						tcpkeep[i].tcp_ctx = NULL;
					}

					tcpkeep[i].tryCnt = 0;
					tcpkeep[i].rectime = 0;
					tcpkeep[i].state = TCP_KEEP_WAIT_CONNECT;
					interval = min(interval, 10);
				} else {
					interval = min(interval, 10000);	/* Checkout connect timeout every 10s */
				}
				break;

			default:
				break;
		}
	}
	os_mutex_unlock(&tcp_keep_lock);

	os_delayed_work_submit(&tcp_keep_work, interval);
}
#endif

static void tcp_mgmt_event_handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if ((mgmt_event == NET_EVENT_IPV4_ADDR_DEL) ||
		(mgmt_event == NET_EVENT_IF_DOWN)) {
		ipReady = false;

#ifdef CONFIG_TCP_KEEP_CONTEXT
		uint8_t i;

		os_mutex_lock(&tcp_keep_lock, K_FOREVER);
		for (i=0; i<TCP_KEEP_MAX; i++) {
			if (tcpkeep[i].state == TCP_KEEP_CONNECTED) {
				if (tcpkeep[i].tcp_ctx) {
					tcp_disconnect_client(tcpkeep[i].tcp_ctx);
					mem_free(tcpkeep[i].tcp_ctx);
				}
				memset(&tcpkeep[i], 0, sizeof(struct tcp_keep_tcp));
			}
		}
		os_mutex_unlock(&tcp_keep_lock);
#endif
	} else if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		ipReady = true;
	}
}

void tcp_client_init(void)
{
#ifdef CONFIG_TCP_KEEP_CONTEXT
	uint8_t i;

	os_mutex_lock(&tcp_keep_lock, K_FOREVER);
	for (i=0; i<TCP_KEEP_MAX; i++) {
		memset(&tcpkeep[i], 0, sizeof(struct tcp_keep_tcp));
	}
	os_mutex_unlock(&tcp_keep_lock);

	os_delayed_work_init(&tcp_keep_work, tcp_keep_delaywork);
#endif

	net_mgmt_init_event_callback(&tcp_client_mgmt_cb, tcp_mgmt_event_handler,
					(NET_EVENT_IPV4_ADDR_DEL | NET_EVENT_IF_DOWN | NET_EVENT_IPV4_ADDR_ADD));
	net_mgmt_add_event_callback(&tcp_client_mgmt_cb);
}
#else
int tcp_set_local_addr(struct tcp_client_ctx *ctx, const char *local_addr)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int tcp_connect(struct tcp_client_ctx **ppctx, const char *server_addr,
		uint16_t server_port)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int tcp_disconnect(struct tcp_client_ctx *ctx)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
void tcp_client_init(void)
{
	SYS_LOG_WRN("this function not support");
}
#endif