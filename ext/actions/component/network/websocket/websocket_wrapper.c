/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <net/net_context.h>
#include <net/net_pkt.h>
#include <misc/printk.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "mem_manager.h"
#include "dns_client.h"
#include "websocket_wrapper.h"


static int websocket_errcb(struct websocket_ctx *ctx, u16_t pkt_type, struct net_pkt *pkt)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);

	SYS_LOG_ERR("Error callback pkt_type:%d", pkt_type);
	/* rfc6455, if receive any err, will close websocket */

	/* Notify app to close websocket */

	return 0;
}

static int websocket_rxframe_cb(struct websocket_ctx *ctx, u8_t *msg,
					u16_t len, enum websocket_packet type)
{
	u8_t tmp;
	websocket_agency_t *websk = CONTAINER_OF(ctx, websocket_agency_t, client_ctx);

	tmp = msg[len];
	msg[len] = 0x00;
	SYS_LOG_DBG("msg len:%d, type:%d, msg:%s", len, type, msg);

	/* send msg to app
	 * app can use this msg, but can't modify memory after this msg
	 */
	if (websk->cb) {
		websk->cb(websk->usrdata, (void *)msg, len);
	}

	msg[len] = tmp;

	return 0;
}

static int websocket_pingpong_cb(struct websocket_ctx *ctx, u8_t *msg,
					u16_t len, enum websocket_packet type)
{
	u8_t tmp;

	if (len) {
		tmp = msg[len];
		msg[len] = 0x00;
		SYS_LOG_DBG("msg len:%d, type:%d, msg:%s", len, type, msg);
		msg[len] = tmp;
	}

	SYS_LOG_DBG("Recieve %s frame", (type == WEBSOCKET_FRAME_PING)? "ping" : "pong");

	if (type == WEBSOCKET_FRAME_PING) {
		/* Send msg to app if have msg */

		/* Respone pong frame */
		websocket_tx_pong(ctx, msg, len);
	}

	return 0;
}

static int websocket_connect_cb(struct websocket_ctx *ctx, u8_t *msg, u16_t len, int8_t state)
{
	u8_t tmp;
	websocket_agency_t *websk = CONTAINER_OF(ctx, websocket_agency_t, client_ctx);

	SYS_LOG_DBG("state:%d", state);
	if (len) {
		tmp = msg[len];
		msg[len] = 0x00;
		SYS_LOG_DBG("msg: %s", msg);
		msg[len] = tmp;
	}

	if (state == 0) {
		k_delayed_work_submit(&websk->ping_timeout, WEBSOCKET_PING_TIMEOUT);
	}

	k_sem_give(&websk->connect_sem);
	return 0;
}

static int websocket_disconnect_cb(struct websocket_ctx *ctx, u8_t *msg, u16_t len)
{
	u8_t tmp;
	websocket_agency_t *websk = CONTAINER_OF(ctx, websocket_agency_t, client_ctx);

	SYS_LOG_INF("Disconnect callback");
	if (len) {
		tmp = msg[len];
		msg[len] = 0x00;
		SYS_LOG_DBG("msg: %s", msg);
		msg[len] = tmp;
	}

	/* Notify app websocket disconnect */

	k_delayed_work_cancel(&websk->ping_timeout);
	k_sem_give(&websk->disconnect_sem);

	return 0;
}

int websocket_send_frame(websocket_agency_t *websk, u8_t *msg, u16_t len, u8_t type)
{
	int rc;

	if (websk == NULL) {
		return -ENOTCONN;
	}

	rc = websocket_tx_frame(&websk->client_ctx, msg, len, type);

	if (rc == 0) {
		k_delayed_work_cancel(&websk->ping_timeout);
		k_delayed_work_submit(&websk->ping_timeout, WEBSOCKET_PING_TIMEOUT);
	}

	return rc;
}

int websocket_send_ping(websocket_agency_t *websk, u8_t *msg, u16_t len)
{
	int rc = -ENOTCONN;

	if (websk == NULL) {
		return rc;
	}

	if (websk->client_ctx.state == WEBSOCKET_STATE_OPEN) {
		k_delayed_work_cancel(&websk->ping_timeout);
		k_delayed_work_submit(&websk->ping_timeout, WEBSOCKET_PING_TIMEOUT);
		rc = websocket_tx_ping(&websk->client_ctx, msg, len);
	}

	return rc;
}

static void websocket_ping_timeout(struct k_work *work)
{
	SYS_LOG_DBG("ping timeout");

	/* Notify app ping timeout */
}




websocket_agency_t *websocket_open(char *srv_host, u16_t srv_port, char *pos, char *origin)
{
	int i, prio, rc;
	websocket_agency_t *websk;
	struct in_addr  in_addr_t = { { { 0 } } };
	char srv_addr[NET_IPV4_ADDR_LEN + 1];

	memset(srv_addr, 0, (NET_IPV4_ADDR_LEN + 1));
	rc = net_addr_pton(AF_INET, srv_host, &in_addr_t);
	if (rc < 0)
	{
		rc = net_dns_resolve(srv_host, &in_addr_t);
		if (rc == 0) {
				net_addr_ntop(AF_INET, &in_addr_t,
					srv_addr, NET_IPV4_ADDR_LEN);
		}
		else
		{
			SYS_LOG_ERR("websocket dns error :%d", rc);
			return NULL;
		}
	} else {
		memcpy(srv_addr, srv_host, strlen(srv_host));
	}

	websk = (websocket_agency_t *)mem_malloc(sizeof(websocket_agency_t));
	if (websk == NULL) {
		SYS_LOG_ERR("Cant't mem_malloc memory!");
		return websk;
	}

	memset(websk, 0, sizeof(websocket_agency_t));

	websk->client_ctx.net_init_timeout = WEBSOCKET_TCP_CONNECT_TIMEOUT;
	websk->client_ctx.net_timeout = WEBSOCKET_TCP_CONNECT_TIMEOUT;
	websk->client_ctx.peer_addr_str = srv_addr;
	websk->client_ctx.peer_port = srv_port;

	websk->client_ctx.errcb = websocket_errcb;
	websk->client_ctx.connect = websocket_connect_cb;
	websk->client_ctx.disconnect = websocket_disconnect_cb;
	websk->client_ctx.pingpong = websocket_pingpong_cb;
	websk->client_ctx.rxframe = websocket_rxframe_cb;
	k_sem_init(&websk->connect_sem, 0, 1);
	k_sem_init(&websk->disconnect_sem, 0, 1);
	k_delayed_work_init(&websk->ping_timeout, websocket_ping_timeout);

	websocket_init(&websk->client_ctx, WEBSOCKET_APP_CLIENT);

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);
	
	for (i=0; i<WEBSOCKET_TCP_CONNECT_TRIES; i++) {	
		if (0 == websocket_connect(&websk->client_ctx)) {
			break;
		}

		k_sleep(10);
	}

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	if (i == WEBSOCKET_TCP_CONNECT_TRIES) {
		goto open_exit;
	}

	if (websocket_tx_connect(&websk->client_ctx, srv_host, pos, origin)) {
		SYS_LOG_ERR("Connect error!");
		goto connect_exit;
	}

	if (k_sem_take(&websk->connect_sem, WEBSOCKET_CONNECT_TIMEOUT)) {
		SYS_LOG_ERR("Wait connect tiemout!");
		goto connect_exit;
	}

	if (websk->client_ctx.state != WEBSOCKET_STATE_OPEN) {
		SYS_LOG_ERR("Connect not open!");
		goto connect_exit;
	}

	return websk;

connect_exit:
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	websocket_deinit(&websk->client_ctx);

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

open_exit:
	mem_free(websk);
	return NULL;
}

int websocket_close(websocket_agency_t *websk, u8_t *msg, u16_t len)
{
	int prio;

	if (websk == NULL) {
		return 0;
	}

	k_delayed_work_cancel(&websk->ping_timeout);

	if (websk->client_ctx.state == WEBSOCKET_STATE_OPEN) {
		k_sem_reset(&websk->disconnect_sem);
		if (0 == websocket_tx_close(&websk->client_ctx, msg, len)) {
			k_sem_take(&websk->disconnect_sem, WEBSOCKET_DISCONNECT_WAIT);
		}
	}

	k_sleep(10);

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	websocket_deinit(&websk->client_ctx);

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	mem_free(websk);
	return 0;
}

int websocket_register_rxcb(websocket_agency_t *websk, websocket_callback callback, void *usrdate)
{
	if ((websk == NULL) || (websk->client_ctx.state != WEBSOCKET_STATE_OPEN)) {
		SYS_LOG_ERR("websk is NULL or not open!");
		return -1;
	}

	websk->cb = callback;
	websk->usrdata = usrdate;
	return 0;
}
