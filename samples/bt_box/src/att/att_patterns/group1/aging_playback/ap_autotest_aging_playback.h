/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_aging_playback.h
*/

#ifndef _AP_AUTOTEST_AGING_PLAYBACK_H_
#define _AP_AUTOTEST_AGING_PLAYBACK_H_

#include "att_pattern_header.h"
#include "app_aging_playback.h"

typedef struct {
	u8_t aging_playback_source; /* 0-pink noise, 1-white noise, 2-sine wave */
	u8_t aging_playback_volume; /* volume level 0 ~ 31 */
	u16_t aging_playback_duration; /* unit 1 minute */
} aging_playback_start_arg_t;

typedef struct {
	u8_t cmp_aging_playback_status; /* if match status, it require passed status */
	u8_t stop_aging_playback; /* if force stop playback */
} aging_playback_check_arg_t;

typedef struct {
	u8_t aging_playback_status; /* 0-disable, 1-aging doing, 2-aging passed, >= 3 -aging error */
} aging_playback_result_t;

#endif
