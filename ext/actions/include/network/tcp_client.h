/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include <net/net_app.h>

struct tcp_client_ctx {
	/* Net application context */
	void *pIOhandle;
	struct net_app_ctx app_ctx;
	/* Network timeout */
	int32_t timeout;
	/* User defined call back*/
	void (*receive_cb)(struct tcp_client_ctx *ctx, struct net_pkt *rx);
};

typedef enum TCP_KEEP_STATE {
	TCP_KEEP_UNUSED,
	TCP_KEEP_WAIT_CONNECT,
	TCP_KEEP_DISCONNECT,
	TCP_KEEP_CONNECTING,
	TCP_KEEP_CONNECTED,
}tcpkeepState;

struct tcp_keep_tcp {
	uint8_t			state;
	uint8_t			tryCnt;
	uint32_t		rectime;
	uint32_t		lastUsetime;
	struct tcp_client_ctx *tcp_ctx;
	struct sockaddr remote;
};

int tcp_set_local_addr(struct tcp_client_ctx *ctx, const char *local_addr);

int tcp_connect(struct tcp_client_ctx **ppctx, const char *server_addr,
		uint16_t server_port);

int tcp_disconnect(struct tcp_client_ctx *ctx);
void tcp_client_init(void);

#endif
