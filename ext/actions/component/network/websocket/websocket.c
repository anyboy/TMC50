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
#include "websocket.h"


/* base64 encode/decode, from base64.h */
#define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL               -0x002A  /**< Output buffer too small. */
#define MBEDTLS_ERR_BASE64_INVALID_CHARACTER              -0x002C  /**< Invalid character in input. */

static const unsigned char base64_enc_map[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
};

static const unsigned char base64_dec_map[128] =
{
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
     54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
      5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
     25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
     29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
     39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
     49,  50,  51, 127, 127, 127, 127, 127
};

#define BASE64_SIZE_T_MAX   ( (size_t) -1 ) /* SIZE_T_MAX is not standard */

/*
 * Encode a buffer into base64 format
 */
int mbedtls_base64_encode( unsigned char *dst, u32_t dlen, u32_t *olen,
                   const unsigned char *src, u32_t slen )
{
    u32_t i, n;
    int C1, C2, C3;
    unsigned char *p;

    if( slen == 0 )
    {
        *olen = 0;
        return( 0 );
    }

    n = slen / 3 + ( slen % 3 != 0 );

    if( n > ( BASE64_SIZE_T_MAX - 1 ) / 4 )
    {
        *olen = BASE64_SIZE_T_MAX;
        return( MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL );
    }

    n *= 4;

    if( ( dlen < n + 1 ) || ( NULL == dst ) )
    {
        *olen = n + 1;
        return( MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL );
    }

    n = ( slen / 3 ) * 3;

    for( i = 0, p = dst; i < n; i += 3 )
    {
        C1 = *src++;
        C2 = *src++;
        C3 = *src++;

        *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map[(((C1 &  3) << 4) + (C2 >> 4)) & 0x3F];
        *p++ = base64_enc_map[(((C2 & 15) << 2) + (C3 >> 6)) & 0x3F];
        *p++ = base64_enc_map[C3 & 0x3F];
    }

    if( i < slen )
    {
        C1 = *src++;
        C2 = ( ( i + 1 ) < slen ) ? *src++ : 0;

        *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];

        if( ( i + 1 ) < slen )
             *p++ = base64_enc_map[((C2 & 15) << 2) & 0x3F];
        else *p++ = '=';

        *p++ = '=';
    }

    *olen = p - dst;
    *p = 0;

    return( 0 );
}

/*
 * Decode a base64-formatted buffer
 */
int mbedtls_base64_decode( unsigned char *dst, u32_t dlen, u32_t *olen,
                   const unsigned char *src, u32_t slen )
{
    u32_t i, n;
    u32_t j, x;
    unsigned char *p;

    /* First pass: check for validity and get output length */
    for( i = n = j = 0; i < slen; i++ )
    {
        /* Skip spaces before checking for EOL */
        x = 0;
        while( i < slen && src[i] == ' ' )
        {
            ++i;
            ++x;
        }

        /* Spaces at end of buffer are OK */
        if( i == slen )
            break;

        if( ( slen - i ) >= 2 &&
            src[i] == '\r' && src[i + 1] == '\n' )
            continue;

        if( src[i] == '\n' )
            continue;

        /* Space inside a line is an error */
        if( x != 0 )
            return( MBEDTLS_ERR_BASE64_INVALID_CHARACTER );

        if( src[i] == '=' && ++j > 2 )
            return( MBEDTLS_ERR_BASE64_INVALID_CHARACTER );

        if( src[i] > 127 || base64_dec_map[src[i]] == 127 )
            return( MBEDTLS_ERR_BASE64_INVALID_CHARACTER );

        if( base64_dec_map[src[i]] < 64 && j != 0 )
            return( MBEDTLS_ERR_BASE64_INVALID_CHARACTER );

        n++;
    }

    if( n == 0 )
    {
        *olen = 0;
        return( 0 );
    }

    n = ( ( n * 6 ) + 7 ) >> 3;
    n -= j;

    if( dst == NULL || dlen < n )
    {
        *olen = n;
        return( MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL );
    }

   for( j = 3, n = x = 0, p = dst; i > 0; i--, src++ )
   {
        if( *src == '\r' || *src == '\n' || *src == ' ' )
            continue;

        j -= ( base64_dec_map[*src] == 64 );
        x  = ( x << 6 ) | ( base64_dec_map[*src] & 0x3F );

        if( ++n == 4 )
        {
            n = 0;
            if( j > 0 ) *p++ = (unsigned char)( x >> 16 );
            if( j > 1 ) *p++ = (unsigned char)( x >>  8 );
            if( j > 2 ) *p++ = (unsigned char)( x       );
        }
    }

    *olen = p - dst;

    return( 0 );
}

static int websocket_pkg_head(u8_t *head, u16_t msg_len,
					u8_t opcode, u8_t *outlen, u8_t *mask_index)
{
	u8_t len = 0;
	u8_t index;
	u32_t key;

	head[0] = 0x80 | (opcode&0x0F);
	head[1] = 0x80;
	len += 2;

	if (msg_len <= 125) {
		head[1] |= (u8_t)msg_len;
		index = 2;
		len += 4;
	} else if (msg_len <= 65535) {
		head[1] |= 126;
		head[2] = (u8_t)(msg_len >> 8);
		head[3] = (u8_t)msg_len;
		index = 4;
		len += 6;
	} else {
		return -ENOPROTOOPT;
	}

	key = sys_rand32_get();
	head[index] = (u8_t)key;
	head[index + 1] = (u8_t)(key >> 8);
	head[index + 2] = (u8_t)(key >> 16);
	head[index + 3] = (u8_t)(key >> 24);

	*outlen = len;
	*mask_index = index;

	return 0;
}

static int websocket_pkg_send(struct websocket_ctx *ctx, u8_t *msg,
						u16_t len, u8_t opcode)
{
	u8_t head[8], mask_index, outlen;
	u8_t *mask_key;
	int rc = 0, i;
	struct net_pkt *pkt = NULL;

	pkt = net_app_get_net_pkt(&ctx->net_app_ctx, AF_UNSPEC, K_FOREVER);
	if (!pkt) {
		rc = -ENOBUFS;
		goto exit_tx;
	}

	if (websocket_pkg_head(head, len, opcode, &outlen, &mask_index)) {
		rc = -ENOPROTOOPT;
		goto exit_tx;
	}

	if (!net_pkt_append(pkt, outlen, (u8_t *)head,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_tx;
	}

	if (len != 0) {
		mask_key = &head[mask_index];
		for (i=0; i<len; i++) {
			msg[i] = msg[i]^(mask_key[i%4]);
		}

		if (!net_pkt_append(pkt, len, (u8_t *)msg,
					K_FOREVER)) {
			rc = -ENOBUFS;
			goto exit_tx;
		}
	}

	rc = net_app_send_pkt(&ctx->net_app_ctx, pkt, NULL, 0, ctx->net_timeout, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_tx;
	}

	pkt = NULL;
	rc = 0;

exit_tx:
	if (pkt) {
		net_pkt_unref(pkt);
	}
	return rc;
}

int websocket_tx_connect(struct websocket_ctx *ctx, char *host, char *pos, char *origin)
{
	struct net_pkt *pkt = NULL;
	u8_t	*pDst = NULL, *pSrc = NULL;
	int rc, i;
	u32_t len;

	pDst = (u8_t *)mem_malloc(64);
	if (pDst == NULL) {
		rc = -ENOMEM;
		goto exit_connect;
	}

	pSrc = (u8_t *)mem_malloc(64);
	if (pSrc == NULL) {
		rc = -ENOMEM;
		goto exit_connect;
	}

	pkt = net_app_get_net_pkt(&ctx->net_app_ctx, AF_UNSPEC, K_FOREVER);
	if (!pkt) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* Append: GET / HTTP/1.1 */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_GET1), (u8_t *)WEBSOCKET_HTTP_GET1,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	if (pos == NULL) {
		if (!net_pkt_append(pkt, 1, "/",
					K_FOREVER)) {
			rc = -ENOBUFS;
			goto exit_connect;
		}
	} else {
		if (!net_pkt_append(pkt, strlen(pos), (u8_t *)pos,
					K_FOREVER)) {
			rc = -ENOBUFS;
			goto exit_connect;
		}
	}

	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_GET2), (u8_t *)WEBSOCKET_HTTP_GET2,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* Append http host */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_HOST), (u8_t *)WEBSOCKET_HTTP_HOST,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	if (!net_pkt_append(pkt, strlen(host), (u8_t *)host,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_ENDLINE), (u8_t *)WEBSOCKET_HTTP_ENDLINE,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* Append: Connection: Upgrade */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_CONNECTING), (u8_t *)WEBSOCKET_HTTP_CONNECTING,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* Append: Upgrade: websocket */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_UPGRADE), (u8_t *)WEBSOCKET_HTTP_UPGRADE,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	if (origin != NULL) {
		/* Append origin if need */
		if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_ORIGIN), (u8_t *)WEBSOCKET_HTTP_ORIGIN,
					K_FOREVER)) {
			rc = -ENOBUFS;
			goto exit_connect;
		}

		if (!net_pkt_append(pkt, strlen(origin), (u8_t *)origin,
					K_FOREVER)) {
			rc = -ENOBUFS;
			goto exit_connect;
		}

		if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_ENDLINE), (u8_t *)WEBSOCKET_HTTP_ENDLINE,
					K_FOREVER)) {
			rc = -ENOBUFS;
			goto exit_connect;
		}
	}

	/* Append User-Agent */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_USER), (u8_t *)WEBSOCKET_HTTP_USER,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* Append Sec-WebSocket-Key */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_KEY), (u8_t *)WEBSOCKET_HTTP_KEY,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	for (i=0; i<16; i++) {
		pSrc[i] = (u8_t)sys_rand32_get();
	}

	if (0 != mbedtls_base64_encode(pDst, 64, &len, pSrc, 16)) {
		rc = -ENOPROTOOPT;
		goto exit_connect;
	}

	if (!net_pkt_append(pkt, len, (u8_t *)pDst,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_ENDLINE), (u8_t *)WEBSOCKET_HTTP_ENDLINE,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* Append Sec-WebSocket-Version */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_VERSION), (u8_t *)WEBSOCKET_HTTP_VERSION,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* Append Sec-WebSocket-Protocol */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_PROTOCOL), (u8_t *)WEBSOCKET_HTTP_PROTOCOL,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	/* HTTP last endline */
	if (!net_pkt_append(pkt, strlen(WEBSOCKET_HTTP_ENDLINE), (u8_t *)WEBSOCKET_HTTP_ENDLINE,
				K_FOREVER)) {
		rc = -ENOBUFS;
		goto exit_connect;
	}

	ctx->state = WEBSOCKET_STATE_CONNECTING;
	rc = net_app_send_pkt(&ctx->net_app_ctx, pkt, NULL, 0, ctx->net_timeout, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_connect;
	}

	pkt = NULL;
	rc = 0;

exit_connect:
	if (pkt) {
		net_pkt_unref(pkt);
	}
	if (pDst) {
		mem_free(pDst);
	}
	if (pSrc) {
		mem_free(pSrc);
	}
	return rc;
}

static int websocket_connecting_rx(struct websocket_ctx *ctx, u8_t *msg, u16_t len)
{
	int rc;
	int8_t	state;

	msg[len] = 0x00;

	if (strstr(msg, WEBSOCKET_HTTP_UPGRADE) == NULL) {
		state = -1;
		rc = -EIO;
	} else {
		ctx->state = WEBSOCKET_STATE_OPEN;
		state = 0;
		rc = 0;
	}

	ctx->connect(ctx, msg, len, state);
	return rc;
}

int websocket_tx_frame(struct websocket_ctx *ctx, u8_t *msg, u16_t len, u8_t type)
{
	if (ctx->state != WEBSOCKET_STATE_OPEN) {
		return -ENOTCONN;
	}

	return websocket_pkg_send(ctx, msg, len, type);
}

int websocket_tx_ping(struct websocket_ctx *ctx, u8_t *msg, u16_t len)
{
	if (ctx->state != WEBSOCKET_STATE_OPEN) {
		return -ENOTCONN;
	}

	return websocket_pkg_send(ctx, msg, len, WEBSOCKET_FRAME_PING);
}

int websocket_tx_pong(struct websocket_ctx *ctx, u8_t *msg, u16_t len)
{
	if (ctx->state != WEBSOCKET_STATE_OPEN) {
		return -ENOTCONN;
	}

	return websocket_pkg_send(ctx, msg, len, WEBSOCKET_FRAME_PONG);
}

int websocket_tx_close(struct websocket_ctx *ctx, u8_t *msg, u16_t len)
{
	if (ctx->state != WEBSOCKET_STATE_OPEN) {
		return -ENOTCONN;
	}

	return websocket_pkg_send(ctx, msg, len, WEBSOCKET_FRAME_CLOSE);
}

static int websocket_client_rx(struct websocket_ctx *ctx, struct net_pkt *pkt)
{
	u16_t pkt_type = WEBSOCKET_FRAME_INVALID;
	u16_t data_len, offset, payload_len, head_len;
	struct net_buf *frag = NULL;
	int rc = 0;

	data_len = net_pkt_appdatalen(pkt);
	offset = net_buf_frags_len(pkt->frags) - data_len;

	frag = pkt->frags;
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	frag->data += offset;
	frag->len -= offset;

	switch (ctx->state) {
		case WEBSOCKET_STATE_CONNECTING:
			return websocket_connecting_rx(ctx, frag->data, frag->len);
		case WEBSOCKET_STATE_CLOSING:
			/* Do nothing */
			return 0;
		case WEBSOCKET_STATE_OPEN:
			break;
		default:
			return -EIO;
	}

	while (frag->len > 0) {
		if (ctx->waitRxLength == 0) {
			pkt_type = WEBSOCKET_PACKET_TYPE(frag->data[0]);

			if ((WEBSOCKET_FIN_BIT(frag->data[0]) == 0) ||
				(WEBSOCKET_MASK_BIT(frag->data[1]) == 1)) {
				/* Only support first fragment also be the final fragment
				 *  Server to client fragment, mask bit is 0
				 */
				return ctx->errcb(ctx, pkt_type, pkt);
			}

			if (WEBSOCKET_PAYLEN_7BIT(frag->data[1]) < 126) {
				payload_len = WEBSOCKET_PAYLEN_7BIT(frag->data[1]);
				head_len = 2;
			} else if (WEBSOCKET_PAYLEN_7BIT(frag->data[1]) == 126) {
				payload_len = (frag->data[2] << 8) | frag->data[3];
				head_len = 4;
			} else {
				/* Not support payload length large than 65535 */
				return ctx->errcb(ctx, pkt_type, pkt);
			}

			if ((head_len + payload_len) > frag->len) {
				ctx->waitRxLength = payload_len - (frag->len - head_len);
				ctx->waitRxType = pkt_type;
				payload_len = frag->len - head_len;
			}
		} else {
			pkt_type = ctx->waitRxType;
			head_len = 0;
			payload_len = (ctx->waitRxLength > frag->len)? frag->len : ctx->waitRxLength;
			ctx->waitRxLength -= payload_len;
			if (ctx->waitRxLength == 0) {
				ctx->waitRxType = WEBSOCKET_FRAME_INVALID;
			}
		}

		switch (pkt_type) {
			case WEBSOCKET_FRAME_TEXT:
			case WEBSOCKET_FRAME_BINARY:
				rc = ctx->rxframe(ctx, &frag->data[head_len], payload_len, pkt_type);
				break;
			case WEBSOCKET_FRAME_PING:
			case WEBSOCKET_FRAME_PONG:
				rc = ctx->pingpong(ctx, (payload_len? &frag->data[head_len] : NULL),
									payload_len, pkt_type);
				break;
			case WEBSOCKET_FRAME_CLOSE:
				rc = ctx->disconnect(ctx, (payload_len? &frag->data[head_len] : NULL),
									payload_len);
				ctx->state = WEBSOCKET_STATE_CLOSING;
				break;
			default:
				rc = ctx->errcb(ctx, pkt_type, pkt);
				break;
		}

		frag->data += (head_len + payload_len);
		frag->len -= (head_len + payload_len);
	}

	return rc;
}

static int websocket_server_rx(struct websocket_ctx *ctx, struct net_pkt *pkt)
{
	/* Wait todo! */
	return -EIO;
}

static void app_connected(struct net_app_ctx *ctx, int status, void *data)
{
	struct websocket_ctx *websk = (struct websocket_ctx *)data;

	ARG_UNUSED(ctx);

	if (!websk) {
		return;
	}
}

static void app_recv(struct net_app_ctx *ctx, struct net_pkt *pkt, int status,
	       	void *data)
{
	struct websocket_ctx *websk = (struct websocket_ctx *)data;

	/* net_ctx is already referenced to by the websocket_ctx struct */
	ARG_UNUSED(ctx);

	if (status || !pkt) {
		return;
	}

	if (!pkt) {
		goto recv_exit;
	}

	if (net_pkt_appdatalen(pkt) == 0) {
		goto recv_exit;
	}

	websk->rcv(websk, pkt);

recv_exit:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

int websocket_connect(struct websocket_ctx *ctx)
{
	int rc = 0;

	if (!ctx) {
		return -EFAULT;
	}

	rc = net_app_init_tcp_client(&ctx->net_app_ctx,
			NULL,
			NULL,
			ctx->peer_addr_str,
			ctx->peer_port,
			ctx->net_init_timeout,
			ctx);
	if (rc < 0) {
		goto error_connect;
	}

	rc = net_app_set_cb(&ctx->net_app_ctx,
			app_connected,
			app_recv,
			NULL,
			NULL);
	if (rc < 0) {
		goto error_connect;
	}

	rc = net_app_connect(&ctx->net_app_ctx, ctx->net_timeout);
	if (rc < 0) {
		goto error_connect;
	}

	return rc;

error_connect:
	/* clean net app context, so websocket_connect() can be called repeatedly */
	net_app_close(&ctx->net_app_ctx);
	net_app_release(&ctx->net_app_ctx);

	return rc;
}

int websocket_init(struct websocket_ctx *ctx, enum websocket_app app_type)
{
	ctx->app_type = app_type;
	ctx->state = WEBSOCKET_STATE_CLOSE;
	ctx->waitRxLength = 0;
	ctx->waitRxType = WEBSOCKET_FRAME_INVALID;

	switch (ctx->app_type) {
	case WEBSOCKET_APP_CLIENT:
		ctx->rcv = websocket_client_rx;
		break;
	case WEBSOCKET_APP_SERVER:
		ctx->rcv = websocket_server_rx;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int websocket_deinit(struct websocket_ctx *ctx)
{
	if (!ctx) {
		return -EFAULT;
	}

	if (ctx->net_app_ctx.is_init) {
		net_app_close(&ctx->net_app_ctx);
		net_app_release(&ctx->net_app_ctx);
	}

	return 0;
}
