/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio HAL
 */


#ifndef __AUDIO_HAL_H__
#define __AUDIO_HAL_H__
#include <os_common_api.h>
#include <assert.h>
#include <string.h>
#include <audio_out.h>
#include <audio_in.h>
#include <dma.h>

/**
**  audio out hal context struct
**/
typedef struct {
	struct  device *aout_dev;
} hal_audio_out_context_t;

/**
**  audio in hal context struct
**/
typedef struct {
	struct  device *ain_dev;
} hal_audio_in_context_t;

/**
 *  audio out init param
 **/
typedef struct {
	u8_t aa_mode:1;
	u8_t need_dma:1;
	u8_t dma_reload:1;
	u8_t out_to_pa:1;
	u8_t out_to_i2s:1;
	u8_t out_to_spdif:1;
	u8_t sample_cnt_enable:1;

	u8_t sample_rate;
	u8_t channel_type;
	u8_t channel_id;
	u8_t channel_mode;
	u8_t data_width;
	u16_t reload_len;
	u8_t *reload_addr;
	int left_volume;
	int right_volume;

	int (*callback)(void *cb_data, u32_t reason);
	void *callback_data;
}audio_out_init_param_t;


/**
**  audio in init param
**/
typedef struct {
	uint8_t aa_mode:1;
	uint8_t need_dma:1;
	uint8_t need_asrc:1;
	uint8_t need_dsp:1;
	uint8_t reserved_1:1;
	uint8_t dma_reload:1;
	uint8_t data_mode;

	u8_t sample_rate;
	u8_t channel_type;
	u8_t left_input;
	u8_t right_input;
	u8_t data_width;
	s16_t adc_gain;
	s16_t input_gain;
	u8_t boost_gain:1;

	u16_t reload_len;
	u8_t *reload_addr;

	int (*callback)(void *cb_data, u32_t reason);
	void *callback_data;
}audio_in_init_param_t;

int hal_audio_out_init(void);
void* hal_aout_channel_open(audio_out_init_param_t *init_param);
int hal_aout_channel_start(void* aout_channel_handle);
int hal_aout_channel_write_data(void* aout_channel_handle, u8_t *data, u32_t data_size);
int hal_aout_channel_stop(void* aout_channel_handle);
int hal_aout_channel_close(void* aout_channel_handle);
int hal_aout_channel_set_aps(void *aout_channel_handle, unsigned int aps_level, unsigned int aps_mode);
u32_t hal_aout_channel_get_sample_cnt(void *aout_channel_handle);
int hal_aout_channel_reset_sample_cnt(void *aout_channel_handle);
int hal_aout_channel_enable_sample_cnt(void *aout_channel_handle, bool enable);
int hal_aout_channel_check_fifo_underflow(void *aout_channel_handle);
int hal_aout_channel_mute_ctl(void *aout_channel_handle, u8_t mode);
int hal_aout_channel_set_pa_vol_level(void *aout_channel_handle, int vol_level);
int hal_aout_set_pcm_threshold(void *aout_channel_handle, int he_thres, int hf_thres);
u32_t hal_aout_channel_get_remain_pcm_samples(void *aout_channel_handle);
int hal_aout_open_pa(void);
int hal_aout_close_pa(void);
int hal_aout_pa_class_select(u8_t pa_mode);


int hal_audio_in_init(void);
void* hal_ain_channel_open(audio_in_init_param_t *init_param);
int hal_ain_channel_start(void* ain_channel_handle);
int hal_ain_channel_read_data(void* ain_channel_handle, u8_t *data, u32_t data_size);
int hal_ain_channel_stop(void* ain_channel_handle);
int hal_ain_channel_close(void* ain_channel_handle);
int hal_ain_channel_set_volume(void* ain_channel_handle, adc_gain *adc_volume);
int hal_ain_channel_get_info(void *ain_channel_handle, u8_t cmd, void *param);
typedef int (*srd_callback)(void *cb_data, u32_t cmd, void *param);
int hal_ain_set_contrl_callback(u8_t channel_type, srd_callback callback);

#endif

