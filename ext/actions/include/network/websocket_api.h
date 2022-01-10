/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file websocket interface 
 */

#ifndef __WEBSOCKET_API_H__
#define __WEBSOCKET_API_H__

#include <net/net_app.h>

/* Websocket packet type */
enum websocket_packet {
	WEBSOCKET_FRAME_CONTINUE,
	WEBSOCKET_FRAME_TEXT,
	WEBSOCKET_FRAME_BINARY,
	WEBSOCKET_FRAME_CLOSE = 8,
	WEBSOCKET_FRAME_PING,
	WEBSOCKET_FRAME_PONG,
	WEBSOCKET_FRAME_INVALID = 0xFF,
};

/* websocket context structure */
struct websocket_ctx {
	/** Net app context structure */
	struct net_app_ctx net_app_ctx;
	s32_t net_init_timeout;
	s32_t net_timeout;

	/** Connectivity */
	char *peer_addr_str;
	u16_t peer_port;

	/* Callback when meet error */
	int (*errcb)(struct websocket_ctx *ctx, u16_t pkt_type, struct net_pkt *pkt);
	/* Callback when receive frame */
	int (*rxframe)(struct websocket_ctx *ctx, u8_t *msg, u16_t len, enum websocket_packet type);
	/* Callback when receive pong frame */
	int (*pingpong)(struct websocket_ctx *ctx, u8_t *msg, u16_t len, enum websocket_packet type);
	/* Callback when receive connect */
	int (*connect)(struct websocket_ctx *ctx, u8_t *msg, u16_t len, int8_t state);
	/* Callback when receive disconnect */
	int (*disconnect)(struct websocket_ctx *ctx, u8_t *msg, u16_t len);

	/* Internal use only */
	int (*rcv)(struct websocket_ctx *ctx, struct net_pkt *);
	int waitRxLength;
	u8_t waitRxType;

	/* websocket state */
	u8_t state;

	/* Application type */
	u8_t app_type;
};

typedef int (*websocket_callback)(const void *usrdata, const void *message, int size);

/* websocket agency structure */
typedef struct websocket_agency {
	struct websocket_ctx client_ctx;
	struct k_sem connect_sem;
	struct k_sem disconnect_sem;
	struct k_delayed_work ping_timeout;
	websocket_callback cb;
	void *usrdata;
}websocket_agency_t;

/*
 * Open websocket
 *
 * @param [in] srv_addr websocket connect addr
 * @param [in] srv_port websocket connect port
 * @param [in] origin websocket server domain name or can be NULL
 *
 * @retval websocket_agency_t *, websocket_agency pointer or NULL
 */
websocket_agency_t *websocket_open(char *srv_addr, u16_t srv_port, char *pos, char *origin);


/*
 * Close websocket
 *
 * @param [in] websk websocket agency
 * @param [in] msg message to send, msg can be NULL
 * @param [in] len message length, length can be 0
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_close(websocket_agency_t *websk, u8_t *msg, u16_t len);

/*
 * send websocket fram
 *
 * @param [in] websk websocket agency
 * @param [in] msg message to send
 * @param [in] len message length
 * @param [in] type test or binary type
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_send_frame(websocket_agency_t *websk, u8_t *msg, u16_t len, u8_t type);

/*
 * Send websocket ping
 *
 * @param [in] websk websocket agency
 * @param [in] msg message to send, msg can be NULL
 * @param [in] len message length, length can be 0
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_send_ping(websocket_agency_t *websk, u8_t *msg, u16_t len);

/*
 * register websocket receive callback
 *
 * @param [in] websk websocket agency
 * @param [in] callback callback function
 * @param [in] usrdate callback parameter
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_register_rxcb(websocket_agency_t *websk, websocket_callback callback, void *usrdate);

#endif //__WEBSOCKET_API_H__