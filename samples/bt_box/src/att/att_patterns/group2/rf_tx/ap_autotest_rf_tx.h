/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_tx.h
*/

#ifndef _AP_AUTOTEST_RF_TX_H_
#define _AP_AUTOTEST_RF_TX_H_

#include "att_pattern_header.h"
#include "fcc_drv_extern.h"
#include "mp_manager_header.h"
#include "compensation.h"

#define INVALID_CFO_VAL         (0x7fff)
#define INVALID_PWR_VAL         (0x7fff)
#define INVALID_RSSI_VAL        (0x7fff)

//cfo max value,If the value is exceeded, the tool will report error
#define MAX_CFO_DIFF_VAL        (5)
#define MAX_RSSI_DIFF_VAL       (10)
#define TEST_RETRY_NUM          (3)
#define MIN_RESULT_NUM          (10)
#define CFO_RESULT_NUM          (10)

typedef enum
{
    BT_PAYLOAD_TYPE_ALL0 = 0,
    BT_PAYLOAD_TYPE_ALL1 = 1,
    BT_PAYLOAD_TYPE_0101 = 2,
    BT_PAYLOAD_TYPE_1010 = 3,
    BT_PAYLOAD_TYPE_0x0_0xF = 4,
    BT_PAYLOAD_TYPE_0000_1111 = 5,
    BT_PAYLOAD_TYPE_1111_0000 = 6,
    BT_PAYLOAD_TYPE_PRBS9 = 7,
    //////////////////////////////////
    BT_PAYLOAD_TYPE_NUM = 8
} BT_PAYLOAD_TYPE;

typedef enum
{
    BT_PKT_1DH1 = 0,
    BT_PKT_1DH3,
    BT_PKT_1DH5,
    BT_PKT_2DH1,
    BT_PKT_2DH3,
    BT_PKT_2DH5,
    BT_PKT_3DH1,
    BT_PKT_3DH3,
    BT_PKT_3DH5,
    BT_PKT_LE,
    //////////////////////////////////////////////////
    BT_PKT_TYPE_NULL,
    BT_PKT_TYPE_NUM
} BT_PKT_TYPE;

typedef struct
{
    u8_t pwr_channel[3];
    s8_t  pwr_thr_low;
    s8_t  pwr_thr_high;
    s16_t rssi_val;
} pwr_test_arg_t;

typedef struct
{
    u8_t mp_type;  // MP type 0 ¨C 5119  1 ¨C 5120
    u8_t sdk_type;  // reserved, default 0
    u8_t rf_channel; // MP test rf channel
	u8_t rf_power; // rf tx power
	u8_t packet_type; // packet type, default DH1 
	u8_t payload_type;// payload type, default 01010101
    u16_t payload_len; // payload length, default 27 
    u16_t timeout;  // Unable to receive enough data within the specified time is invalid data , unit 1ms
    u8_t reserved[6]; 
} bttools_mp_param_t;

bool rf_tx_rssi_calc(s16_t *data_buffer, u32_t result_num, s16_t *rssi_return);
u32_t att_read_trim_cap(u32_t mode);

#endif

