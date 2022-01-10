/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_bt_link.h
*/

#ifndef _AP_AUTOTEST_BT_LINK_H_
#define _AP_AUTOTEST_BT_LINK_H_

#include "att_pattern_header.h"
#include "bt_manager.h"

extern void bt_manager_startup_reconnect(void);  //bt_manager_inner.h

typedef struct
{
    u8_t bt_transmitter_addr[6];
    u8_t bt_test_mode;
    u8_t bt_fast_mode;
} btplay_test_arg_t;

#endif

bool att_btcall_start_play(void);
void att_btcall_stop_play(void);