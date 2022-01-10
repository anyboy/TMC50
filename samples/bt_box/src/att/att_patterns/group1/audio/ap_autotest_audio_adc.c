/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_audio_adc.c
*/

#include "ap_autotest_audio.h"

static void *adc_in_handle;

s32_t ap_autotest_audio_adc_start(short *adc_in_buffer, u32_t length, int (*callback)(void *cb_data, u32_t reason))
{
	audio_in_init_param_t init_param;

	SYS_LOG_INF("audio adc start...");

	memset(&init_param, 0x0, sizeof(audio_in_init_param_t));
	init_param.need_dma = 1;
	init_param.dma_reload = 1;
	if (stereo_flag == true) {
		init_param.data_mode = AUDIO_MODE_STEREO;
	} else {
		init_param.data_mode = AUDIO_MODE_DEFAULT;
	}

	init_param.sample_rate = MY_SAMPLE_RATE;
	init_param.channel_type = AUDIO_CHANNEL_ADC;
	init_param.data_width = 16;
	
	if (self->test_id == TESTID_LINEIN_CH_TEST) {
		if (g_linein_channel_test_arg.input_channel == 0) {
			init_param.left_input = AUDIO_LINE_IN0;
			init_param.right_input = AUDIO_LINE_IN0;
		} else {
			init_param.left_input = AUDIO_LINE_IN1;
			init_param.right_input = AUDIO_LINE_IN1;
		}
		/* 0dB */
		init_param.adc_gain = 0;
		init_param.input_gain = 0;
	} else if (self->test_id == TESTID_MIC_CH_TEST) {
		if (g_mic_channel_test_arg.input_channel == 0) {
			init_param.left_input = AUDIO_ANALOG_MIC0;
			init_param.right_input = AUDIO_ANALOG_MIC0;
		} else if (g_mic_channel_test_arg.input_channel == 1){
			init_param.left_input = AUDIO_ANALOG_MIC1;
			init_param.right_input = AUDIO_ANALOG_MIC1;
		} else {
			init_param.left_input = AUDIO_ANALOG_MIC0;
			init_param.right_input = AUDIO_ANALOG_MIC1;
		}
		if (g_mic_channel_test_arg.dac_play_flag == 0) {
			/* 45dB for extern sound source */
			init_param.adc_gain = 0;
			init_param.input_gain = 450;
		} else {
			/* 30dB for internal sound source */
    		init_param.adc_gain = 0;
    		init_param.input_gain = 300;
		}
	} else /*TESTID_FM_CH_TEST*/ {
		if (g_fm_rx_test_arg.fm_rx_aux_in_channel == 0) {
			init_param.left_input = AUDIO_LINE_IN0;
			init_param.right_input = AUDIO_LINE_IN0;
		} else {
			init_param.left_input = AUDIO_LINE_IN1;
			init_param.right_input = AUDIO_LINE_IN1;
		}
		/* 6dB */
		init_param.adc_gain = 0;
		init_param.input_gain = 60;
	}

	init_param.reload_len = length;
	init_param.reload_addr = (u8_t *)adc_in_buffer;

	init_param.callback = callback;
	
	adc_in_handle = hal_ain_channel_open(&init_param);
	
	hal_ain_channel_start(adc_in_handle);
	
	return 0;
}

s32_t ap_autotest_audio_adc_stop(void)
{
	hal_ain_channel_stop(adc_in_handle);
	hal_ain_channel_close(adc_in_handle);
	
	return 0;
}

