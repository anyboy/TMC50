/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <mem_manager.h>
#include "http_client.h"
#include "http_helper_cb.h"


#define MAX_NUM_DIGITS	16

int on_url(struct http_parser *parser, const char *at, size_t length)
{
	ARG_UNUSED(parser);

	//printf("URL: %.*s\n", length, at);

	return 0;
}

int on_status(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx;
	uint16_t len;

	ARG_UNUSED(parser);

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);
	len = min(length, sizeof(ctx->http_status) - 1);
	memcpy(ctx->http_status, at, len);
	ctx->http_status[len] = 0;

	return 0;
}

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	char *content_len = "content-length";
	char *location_len = "location";
	struct http_client_ctx *ctx;
	uint16_t len;
	int i;

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	/* field: content-length */
	if (tolower(at[0]) == 'c') {
		len = strlen(content_len);
		if (length >= len) {
			for (i = 1; i < len; i++) {
				if (tolower(at[i]) != content_len[i])
					break;
			}

			ctx->cl_present = (i == len);
		}

	/* field: location */
	} else if (tolower(at[0]) == 'l') {
		len = strlen(location_len);
		if (length >= len) {
			for (i = 1; i < len; i++) {
				if (tolower(at[i]) != location_len[i])
					break;
			}

			ctx->location_present = (i == len);
		}
	}

	//printf("%.*s: ", length, at);

	return 0;
}

int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx;
	char str[MAX_NUM_DIGITS];

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	if (ctx->cl_present) {
		if (length <= MAX_NUM_DIGITS - 1) {
			long int num;

			memcpy(str, at, length);
			str[length] = 0;
			num = strtol(str, NULL, 10);
			if (num == LONG_MIN || num == LONG_MAX) {
				return -EINVAL;
			}

			ctx->content_length = num;
		}

		ctx->cl_present = 0;
	}

	if (ctx->location_present) {
		if (length <= HTTP_MAX_URL_LEN - 1) {
			if(ctx->http_location == NULL)
			{
				ctx->http_location = mem_malloc(length + 1);
			}
			else
			{
				mem_free(ctx->http_location);
				ctx->http_location = mem_malloc(length + 1);
			}
			memcpy(ctx->http_location, at, length);
			ctx->http_location[length] = 0;
		} else {
			SYS_LOG_ERR("http_location url %zu > %d\n", length, HTTP_MAX_URL_LEN);
		}

		ctx->location_present = 0;
	}

	//printf("%.*s\n", length, at);

	return 0;
}

int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx;

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	ctx->body_found = 1;
	ctx->processed += length;

	return 0;
}

int on_headers_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

int on_message_begin(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	//printf("\n--------- HTTP response (headers) ---------\n");

	return 0;
}

int on_message_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

int on_chunk_header(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

int on_chunk_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}
