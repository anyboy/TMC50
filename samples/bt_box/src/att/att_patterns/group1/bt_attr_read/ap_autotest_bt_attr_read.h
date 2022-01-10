/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_bt_attr_read.h
*/

#ifndef _AP_AUTOTEST_BT_ATTR_READ_H_
#define _AP_AUTOTEST_BT_ATTR_READ_H_

#include "att_pattern_header.h"

#define TEST_BTNAME_MAX_LEN     (18*3 + 2)
#define TEST_BTBLENAME_MAX_LEN  (29)

typedef struct
{
    u8_t cmp_btname_flag;
    u8_t cmp_btname[TEST_BTNAME_MAX_LEN];
    u8_t cmp_blename_flag;
    u8_t cmp_blename[TEST_BTBLENAME_MAX_LEN];
} read_bt_name_arg_t;

#endif
