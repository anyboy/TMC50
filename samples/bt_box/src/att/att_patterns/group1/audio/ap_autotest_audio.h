/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_audio.h
*/

#ifndef _AP_AUTOTEST_AUDIO_H_
#define _AP_AUTOTEST_AUDIO_H_

#include "att_pattern_header.h"

#include "audio_hal.h"
#include "fft.h"
#include "audio_system.h"

#define MY_SAMPLE_RATE  48

struct channel_arg 
{
    u8_t test_ch;
    s16_t ch_power_th;
    u8_t test_ch_spectrum;
    s16_t ch_spectrum_snr_th;
};

/* test args for mic channel and aux channel*/
typedef struct
{
    u8_t dac_play_flag;
	s8_t dac_play_volume_db;
	u8_t input_channel;

	struct channel_arg left;
    struct channel_arg right;
} aux_mic_channel_test_arg_t;

/* test args for fm rx channel */
typedef struct
{
    u16_t fm_rx_channel;
	u8_t fm_rx_aux_in_channel;

	struct channel_arg left;
    struct channel_arg right;
} fm_rx_test_arg_t;

/* test result for audio channel */
typedef struct
{
	u8_t left_ch_result;
	s16_t left_ch_power;
	s16_t left_ch_spectrum_snr;
	
	u8_t right_ch_result;
	s16_t right_ch_power;
	s16_t right_ch_spectrum_snr;
} audio_channel_result_t;

extern struct att_env_var *self;
extern aux_mic_channel_test_arg_t g_linein_channel_test_arg;
extern aux_mic_channel_test_arg_t g_mic_channel_test_arg;
extern fm_rx_test_arg_t g_fm_rx_test_arg;
extern bool stereo_flag;

void energy_calc(short *frame_buf, u16_t sample_cnt, short *p_cur_power_max, short *p_cur_power_rms);

s32_t math_exp_fixed(s32_t db); /*dB -> x/32768*/
s32_t math_calc_db(s32_t value); /* x/32768 -> dB */

s32_t ap_autotest_audio_dac_fill(void);
s32_t ap_autotest_audio_dac_start(s32_t db);
s32_t ap_autotest_audio_dac_stop(void);

s32_t ap_autotest_audio_adc_start(short *adc_in_buffer, u32_t length, int (*callback)(void *cb_data, u32_t reason));
s32_t ap_autotest_audio_adc_stop(void);

#endif
