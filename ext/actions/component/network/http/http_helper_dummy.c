/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <stdio.h>

struct http_client_ctx *http_init(void)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
void http_deinit(struct http_client_ctx *http_ctx)
{
	SYS_LOG_WRN("this function not support");
}

int http_reset_ctx(struct http_client_ctx *http_ctx)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int http_send_request(struct http_client_ctx *http_ctx, const char *method, struct request_info * request)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int http_resp_parse_headers(struct tcp_client_ctx *tcp_ctx,
			 struct net_pkt * nbuf)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int http_send_data(struct http_client_ctx *http_ctx, u8_t *data, u32_t len,bool chunked, bool last_block)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int http_receive_data(struct http_client_ctx *http_ctx, u8_t *data, u32_t len, u32_t timeout)
{
	SYS_LOG_WRN("this function not support");
	return 0;	
}

int http_parse_url(struct http_parser_url * u,const char *url, char * ip_addr ,u16_t * port)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int stop_all_http_connectting(void)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int http_wait_response_code(struct http_client_ctx *http_ctx, int timeout)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}

int http_release_resource(struct http_client_ctx *http_ctx)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
bool http_support_chunked(void)
{
	SYS_LOG_WRN("this function not support");
	return false;
}