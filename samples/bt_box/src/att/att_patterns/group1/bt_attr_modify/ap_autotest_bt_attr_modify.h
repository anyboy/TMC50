/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_bt_attr_modify.h
*/

#ifndef _AP_AUTOTEST_BT_ATTR_MODIFY_H_
#define _AP_AUTOTEST_BT_ATTR_MODIFY_H_

#include "att_pattern_header.h"

#define TEST_BTNAME_MAX_LEN     (18*3 + 2)

typedef struct
{
    u8_t bt_name[TEST_BTNAME_MAX_LEN];
} bt_name_arg_t;

#define TEST_BTBLENAME_MAX_LEN  (29)

typedef struct
{
    u8_t bt_ble_name[TEST_BTBLENAME_MAX_LEN];
} ble_name_arg_t;

typedef struct
{
    u8_t bt_addr[6];
    u8_t bt_addr_add_mode;
    u8_t bt_burn_mode;
    u8_t reserved;
} bt_addr_arg_t;

#endif
