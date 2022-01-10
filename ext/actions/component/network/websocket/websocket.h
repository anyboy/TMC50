/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <net/net_app.h>

/* Websocket connect http header */
#define WEBSOCKET_HTTP_GET1			"GET "
#define WEBSOCKET_HTTP_GET2			" HTTP/1.1\r\n"
#define WEBSOCKET_HTTP_HOST			"Host: "
#define WEBSOCKET_HTTP_CONNECTING	"Connection: Upgrade\r\n"
#define WEBSOCKET_HTTP_UPGRADE		"Upgrade: websocket\r\n"
#define WEBSOCKET_HTTP_ORIGIN		"Origin: "
#define WEBSOCKET_HTTP_VERSION		"Sec-WebSocket-Version: 13\r\n"
#define WEBSOCKET_HTTP_KEY			"Sec-WebSocket-Key: "
#define WEBSOCKET_HTTP_PROTOCOL		"Sec-WebSocket-Protocol: chat, superchat\r\n"
#define WEBSOCKET_HTTP_EXTENSIONS	"Sec-WebSocket-Extensions: deflate-stream\r\n"
#define WEBSOCKET_HTTP_USER			"User-Agent: Zephyr/1.70 (iot)\r\n"
#define WEBSOCKET_HTTP_ENDLINE		"\r\n"

/* Websocket application type */
enum websocket_app {
	/** Websocket client */
	WEBSOCKET_APP_CLIENT,
	/** Websocket Server */
	WEBSOCKET_APP_SERVER
};

/* Websocket state type */
enum websocket_state {
	WEBSOCKET_STATE_CLOSE,
	WEBSOCKET_STATE_CONNECTING,
	WEBSOCKET_STATE_OPEN,
	WEBSOCKET_STATE_CLOSING
};

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

/* Websocke head field */
#define WEBSOCKET_FIN_BIT(first_byte)		(((first_byte) & 0x80) >> 7)
#define WEBSOCKET_PACKET_TYPE(first_byte)	((first_byte) & 0x0F)
#define WEBSOCKET_MASK_BIT(second_byte)		(((second_byte) & 0x80) >> 7)
#define WEBSOCKET_PAYLEN_7BIT(second_byte)	((second_byte) & 0x7F)

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

/*
 * Send the websocket connect request
 *
 * @param [in] ctx websocket context structure
 * @param [in] host connect host "xx.xx.xx.xx:port"
 * @param [in] origin host domain name
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_tx_connect(struct websocket_ctx *ctx, char *host, char *pos, char *origin);

/*
 * Send the websocket frame
 *
 * @param [in] ctx websocket context structure
 * @param [in] msg message to send
 * @param [in] len message length
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_tx_frame(struct websocket_ctx *ctx, u8_t *msg, u16_t len, u8_t type);

/*
 * Send the websocket ping
 *
 * @param [in] ctx websocket context structure
 * @param [in] msg message to send, msg can be NULL
 * @param [in] len message length, length can be 0
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_tx_ping(struct websocket_ctx *ctx, u8_t *msg, u16_t len);

/*
 * Send the websocket pong
 *
 * @param [in] ctx websocket context structure
 * @param [in] msg message to send, msg can be NULL
 * @param [in] len message length, length can be 0
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_tx_pong(struct websocket_ctx *ctx, u8_t *msg, u16_t len);

/*
 * Send the websocket close
 *
 * @param [in] ctx websocket context structure
 * @param [in] msg message to send, msg can be NULL
 * @param [in] len message length, length can be 0
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_tx_close(struct websocket_ctx *ctx, u8_t *msg, u16_t len);

/*
 * Initialize websocket start tcp connect
 *
 * @param [in] ctx websocket context structure
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_connect(struct websocket_ctx *ctx);

/*
 * Initialize websocket
 *
 * @param [in] ctx websocket context structure
 * @param [in] app_type websocket type, client or server
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_init(struct websocket_ctx *ctx, enum websocket_app app_type);

/*
 * Deinitialize websocket
 *
 * @param [in] ctx websocket context structure
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_deinit(struct websocket_ctx *ctx);

#endif //__WEBSOCKET_H__
