/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_HELPER_H_
#define _HTTP_HELPER_H_

#include <net/http_parser.h>
#include "tcp_client.h"

/* rx tx timeout */
#define HTTP_NETWORK_TIMEOUT	1000
#define HTTP_TCP_CONNECT_RETRY	5

#define HTTP_STATUS_STR_SIZE     32
#define HTTP_MAX_URL_LEN		512

#define HTTP_SERVICE_PORT		80

/* It seems enough to hold 'Content-Length' and its value */
#define CON_LEN_SIZE	48

#define CONTENT_TYPE "\r\nContent-Type: "
#define CONTENT_LENGTH "\r\nContent-Length: "
#define HOST "Host: "
#define PROTOCOL " HTTP/1.1\r\n"

#define CRLF "\r\n"

#define CHUNKED "\r\nTransfer-Encoding: chunked"

/* Default HTTP Header Field values for HTTP Requests */
#define ACCEPT		"text/plain"
#define ACCEPT_ENC	"identity"
#define ACCEPT_LANG	"en-US"
#define CONNECTION	"Close"
#define USER_AGENT	"Mozilla/4.0 (compatible; Totalcmd; iot)"
#define HOST_NAME	"192.168.43.1" /* or example.com, www.example.com */

#define HEADER_FIELDS	"\r\nUser-Agent: "USER_AGENT"\r\n" \
			"Connection: "CONNECTION"\r\n"
#define HTTP_RANGE		"Range: bytes=%d-\r\n"
#define HTTP_END_LINE	"\r\n"

typedef int (*http_receive_cb)(char * buffer, int buffer_len);

struct request_info {
	char * url;
	char * http_head;
	uint32_t seek_offset;
	uint8_t chunked;
	uint8_t send_post_immediately;
	http_receive_cb receive_cb;
};

struct http_client_ctx {
	struct http_parser parser;
	struct http_parser_settings settings;
	/** net buffer fifo */
	os_fifo netbuffifo;
	/** user receive call back*/
	int (*receive_cb)(char * buffer, int len);
	/** current useing net buffer*/
	struct net_pkt * cur_nbuf;
	/** http chunked mode*/
	int cur_off;
	/** sem used for net data sync */
	os_sem net_data_sem;

	struct tcp_client_ctx *tcp_ctx;

	uint32_t content_length;
	uint32_t received_len;
	uint32_t processed;
	char http_status[HTTP_STATUS_STR_SIZE];
	char * http_location;

	uint8_t isworking:1;
	uint8_t cl_present:1;
	uint8_t body_found:1;
	uint8_t location_present:1;
	uint8_t responsed:1;
	uint8_t finished:1;
};

struct http_client_ctx *http_init(void);
void http_deinit(struct http_client_ctx *http_ctx);

int http_reset_ctx(struct http_client_ctx *http_ctx);

/* Reception callback executed by the IP stack */
//void http_receive_cb(struct tcp_client_ctx *tcp_ctx, struct net_buf *rx);

int http_send_request(struct http_client_ctx *http_ctx, const char *method, struct request_info * request);

int http_send_data(struct http_client_ctx *http_ctx, u8_t *data, u32_t len,bool chunked, bool last_block);

int http_receive_data(struct http_client_ctx *http_ctx, u8_t *data, u32_t len, u32_t timeout);

/* Sends an HTTP GET request for URL url */
#define http_send_get(ctx,request) http_send_request(ctx,"GET ",request)

/* Sends an HTTP HEAD request for URL url */
#define http_send_head(ctx,request) http_send_request(ctx,"HEAD ",request)

/* Sends an HTTP OPTIONS request for URL url. From RFC 2616:
 * If the OPTIONS request includes an entity-body (as indicated by the
 * presence of Content-Length or Transfer-Encoding), then the media type
 * MUST be indicated by a Content-Type field.
 * Note: Transfer-Encoding is not yet supported.
 */
#define http_send_options(ctx,request) http_send_request(ctx,"OPTIONS ",request)

/* Sends an HTTP POST request for URL url with payload as content */
#define http_send_post(ctx,request) http_send_request(ctx,"POST ",request)

int http_resp_parse_headers(struct tcp_client_ctx *tcp_ctx,
			 struct net_pkt * nbuf);

int http_parse_url(struct http_parser_url * u,const char *url, char * ip_addr ,uint16_t * port);

int stop_all_http_connectting(void);

int http_wait_response_code(struct http_client_ctx *http_ctx, int timeout);

int http_release_resource(struct http_client_ctx *http_ctx);

bool http_support_chunked(void);
#endif
