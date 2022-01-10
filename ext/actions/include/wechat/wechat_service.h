/** @file
 *  @brief Wecaht Service sample
 */

/*
 * Copyright (c) 2017-2018 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: lipeng<lipeng@actions-semi.com>
 *
 * Change log:
 *	2017/5/5: Created by lipeng.
 */
#ifndef _WECHAT_SERVICE_H_
#define _WECHAT_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*wechat_recv_cb_t)(void *buf, int len);
typedef void (*wechat_ind_status_cb_t)(bool onoff);

int wechat_init(wechat_recv_cb_t recv_cb, wechat_ind_status_cb_t status_cb);
int wechat_advertise(bool on);
void wechat_disconnect(void);

int wechat_send_data(void *buf, int len);
#ifdef __cplusplus
}
#endif

#endif	/* _WECHAT_SERVICE_H_ */
