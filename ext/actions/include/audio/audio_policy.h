/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio policy.
*/

#ifndef __AUDIO_POLICY_H__
#define __AUDIO_POLICY_H__

#include <audio_hal.h>
#include <media_effect_param.h>

/**
 * @defgroup audio_policy_apis Auido policy APIs
 * @{
 */

struct audio_policy_t {
	u8_t audio_out_channel;
	u8_t tts_fixed_volume:1;
	u8_t volume_saveto_nvram:1;
	u8_t audio_out_volume_level;

	s16_t audio_in_linein_gain;
	s16_t audio_in_fm_gain;
	s16_t audio_in_mic_gain;

	u8_t voice_aec_tail_length_8k;
	u8_t voice_aec_tail_length_16k;

	const short *music_da_table; /* 0.1 dB */
	const int   *music_pa_table; /* 0.001 dB */
	const short *voice_da_table; /* 0.1 dB */
	const int   *voice_pa_table; /* 0.001 dB */
	const short *usound_da_table; /* 0.1 dB */
	const int   *usound_pa_table; /* 0.001 dB */

	aset_volume_table_v2_t *aset_volume_table;
	asqt_ext_info_t        *asqt_ext_info;
};
/**
 * @cond INTERNAL_HIDDEN
 */
int audio_policy_get_out_channel_type(u8_t stream_type);

int audio_policy_get_out_channel_id(u8_t stream_type);

int audio_policy_get_out_channel_mode(u8_t stream_type);

/* return 0 for decoder decided */
int audio_policy_get_out_pcm_channel_num(u8_t stream_type);

int audio_policy_get_out_pcm_frame_size(u8_t stream_type);

int audio_policy_get_out_channel_asrc_alloc_method(u8_t stream_type);

int audio_policy_get_out_input_start_threshold(u8_t stream_type, u8_t exf_stream_type);

int audio_policy_get_out_input_stop_threshold(u8_t stream_type, u8_t exf_stream_type);

int audio_policy_get_out_audio_mode(u8_t stream_type);

u8_t audio_policy_get_out_effect_type(u8_t stream_type,
			u8_t efx_stream_type, bool is_tws);

u8_t audio_policy_get_record_effect_type(u8_t stream_type,
			u8_t efx_stream_type);

int audio_policy_get_record_audio_mode(u8_t stream_type);

int audio_policy_get_record_right_channel_id(u8_t stream_type);

int audio_policy_get_record_left_channel_id(u8_t stream_type);

int audio_policy_get_record_adc_gain(u8_t stream_type);

int audio_policy_get_record_input_gain(u8_t stream_type);

int audio_policy_get_record_channel_mode(u8_t stream_type);

int audio_policy_get_record_channel_type(u8_t stream_type);

int audio_policy_get_volume_level(void);

/* return volume in 0.001 dB */
int audio_policy_get_pa_volume(u8_t stream_type, u8_t volume_level);
int audio_policy_get_pa_class(u8_t stream_type);

/* return volume in 0.1 dB */
int audio_policy_get_da_volume(u8_t stream_type, u8_t volume_level);

int audio_policy_get_record_channel_support_aec(u8_t stream_type);

int audio_policy_get_record_channel_aec_tail_length(u8_t stream_type, u8_t sample_rate, bool in_debug);

int audio_policy_get_channel_resample(u8_t stream_type);

int audio_policy_get_output_support_multi_track(u8_t stream_type);

int audio_policy_check_tts_fixed_volume();

int audio_policy_check_save_volume_to_nvram(void);

int audio_policy_get_increase_threshold(u8_t stream_type, u8_t ext_stream_type, int monitor_type);

int audio_policy_get_reduce_threshold(u8_t stream_type, u8_t ext_stream_type, int monitor_type);

int audio_policy_get_tws_bitpool(void);

/* @volume_db in 0.1 dB */
int audio_policy_get_volume_level_by_db(u8_t stream_type, int volume_db);

aset_volume_table_v2_t *audio_policy_get_aset_volume_table(void);

asqt_ext_info_t *audio_policy_get_asqt_ext_info(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Register user audio policy to audio system
 *
 * This routine Register user audio policy to audio system, must be called when system
 * init. if not set user audio policy ,system may used default policy for audio system.
 *
 * @param user_policy user sudio policy
 *
 * @return 0 excute successed , others failed
 */

int audio_policy_register(const struct audio_policy_t *user_policy);

/**
 * @} end defgroup audio_policy_apis
 */

#endif