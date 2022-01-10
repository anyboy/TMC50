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
 */

#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <shell/shell.h>
#include <stdlib.h>
#include "websocket_api.h"

#define WEBSK_HOST_ADDR		"121.40.165.18" /* or "www.blue-zero.com" */
#define WEBSK_HOST_PORT		8088
#define WEBSK_HOST_GET		"/cn.xxxx"
#define WEBSK_ORIGIN		"http://www.blue-zero.com"		/* origin can be NULL */
websocket_agency_t *websk = NULL;

int shell_test_websk(int argc, char *argv[])
{
	uint8_t msg[10] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};

	if (!strcmp(argv[0], "open")) {
		websk = websocket_open(WEBSK_HOST_ADDR, WEBSK_HOST_PORT, WEBSK_HOST_GET, WEBSK_ORIGIN);
		SYS_LOG_INF("websocket_open %s\n", websk? "success" : "failed");
	} else if (!strcmp(argv[0], "close1")) {
		if (websk) {
			websocket_close(websk, NULL, 0);
			SYS_LOG_INF("websocket_close\n");
		} else {
			SYS_LOG_INF("websk is NULL\n");
		}
	} else if (!strcmp(argv[0], "close2")) {
		if (websk) {
			websocket_close(websk, msg, 10);
			SYS_LOG_INF("websocket_close\n");
		} else {
			SYS_LOG_INF("websk is NULL\n");
		}
	} else if (!strcmp(argv[0], "txtext")) {
		websocket_send_frame(websk, msg, 10, WEBSOCKET_FRAME_TEXT);
		SYS_LOG_INF("websocket_send_frame\n");
	} else if (!strcmp(argv[0], "txbin")) {
		websocket_send_frame(websk, msg, 10, WEBSOCKET_FRAME_BINARY);
		SYS_LOG_INF("websocket_send_frame\n");
	} else if (!strcmp(argv[0], "ping1")) {
		websocket_send_ping(websk, NULL, 0);
		SYS_LOG_INF("websocket_send_ping\n");
	} else if (!strcmp(argv[0], "ping2")) {
		websocket_send_ping(websk, msg, 10);
		SYS_LOG_INF("websocket_send_ping\n");
	} else {
		SYS_LOG_INF("Unknow command:%s\n", argv[0]);
	}

	return 0;
}
