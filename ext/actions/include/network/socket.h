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

 * Author: wh<wanghui@actions-semi.com>
 *
 * Change log:
 * 2018/7/27 Created by wanghui
 */

#ifndef _ACT_SOCKET_H_
#define _ACT_SOCKET_H_

#include <zephyr.h>
#include <net/net_context.h>
#include <sys/types.h>

#ifdef CONFIG_FILE_SYSTEM
#include <fs.h>
#endif
/*socket*/

typedef void (*data_notify)(void* parm);

typedef struct _socket{
	struct net_context* ctx;
	/* Local sock address */
	struct sockaddr local;

	/** Remote (server) socket address */
	struct sockaddr remote;

	data_notify notify_func;
	void *notify_param;
	//reserve
	struct k_sem data_response_sem;

	u8_t context_release;
}zpy_socket_t;

struct zpy_sock_pollfd {
	unsigned int fd;
	short events;
	short revents;
};

/* Values are compatible with Linux */
#define ZSOCK_POLLIN 1
#define ZSOCK_POLLOUT 4

unsigned int zpy_sock_socket(int family, int type, int proto);
int zpy_sock_destory(unsigned int sock);
int zpy_sock_close(unsigned int sock);
int zpy_sock_connect(unsigned int sock, void *sockaddr, unsigned int addrlen);
int zpy_sock_connect_by_host(unsigned int sock, void *hostaddr,unsigned short server_port);
//int zpy_sock_listen(int sock, int backlog);
//int zpy_sock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen);
int zpy_sock_send(unsigned int sock, const void *buf, unsigned int len, int flags);
int zpy_sock_recv(unsigned int sock, void *buf, unsigned int max_len, int flags);
int zpy_sock_fcntl(unsigned int sock, int cmd, int flags);
void zpy_sock_set_notify_func(unsigned int sock,data_notify func,void *notify_param);

#ifdef CONFIG_POLL
int zpy_sock_poll(struct zpy_sock_pollfd *fds, int nfds, int timeout);
#endif

int zpy_sock_inet_pton(unsigned int family, const char *src, void *dst);

#endif /* ZPY_INTERFACE_H_ */
