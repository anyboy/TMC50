/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio policy.
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <audio_device.h>
#include <btservice_api.h>
#include <bt_manager.h>
#include <media_type.h>

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio policy"
#include <logging/sys_log.h>

static const struct audio_policy_t *user_policy;

int audio_policy_get_out_channel_type(u8_t stream_type)
{
	int out_channel = AUDIO_CHANNEL_DAC;

	if (user_policy)
		out_channel = user_policy->audio_out_channel;

	return out_channel;
}

int audio_policy_get_out_channel_id(u8_t stream_type)
{
	int out_channel_id = AOUT_FIFO_DAC0;

	return out_channel_id;
}

int audio_policy_get_out_channel_mode(u8_t stream_type)
{
	int out_channel_mode = AUDIO_DMA_MODE;

	switch (stream_type) {
	case AUDIO_STREAM_TTS:
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
		out_channel_mode = AUDIO_DMA_MODE;
		break;
	}
	return out_channel_mode;
}

int audio_policy_get_out_pcm_channel_num(u8_t stream_type)
{
	int channel_num = 0;

	switch (stream_type) {
	case AUDIO_STREAM_TTS:
		channel_num = 2;
		break;
	case AUDIO_STREAM_VOICE:
		channel_num = 1;
		break;
	case AUDIO_STREAM_MIC_IN:
		break;
	default: /* decoder decided */
		channel_num = 2;
		break;
	}

	if ((user_policy && (user_policy->audio_out_channel & AUDIO_CHANNEL_I2STX))
	|| (user_policy && (user_policy->audio_out_channel & AUDIO_CHANNEL_SPDIFTX))) {
		if (stream_type != AUDIO_STREAM_VOICE)
			channel_num = 2;
	}

	return channel_num;
}

int audio_policy_get_out_pcm_frame_size(u8_t stream_type)
{
	int frame_size = 512;

	switch (stream_type) {
	case AUDIO_STREAM_TTS:
		frame_size = 4 * 960;
		break;
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
		frame_size = 2048;
		break;
	case AUDIO_STREAM_VOICE:
		frame_size = 512;
		break;
	default:
		break;
	}

	return frame_size;
}

int audio_policy_get_out_audio_mode(u8_t stream_type)
{
	int audio_mode = AUDIO_MODE_STEREO;

	switch (stream_type) {
	/**283D tts is mono ,but zs285A tts is stereo*/
	/* case AUDIO_STREAM_TTS: */
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_MIC_IN:
		audio_mode = AUDIO_MODE_MONO;
		break;
	}

	return audio_mode;
}
u8_t audio_policy_get_out_effect_type(u8_t stream_type,
			u8_t efx_stream_type, bool is_tws)
{
	switch (stream_type) {
	case AUDIO_STREAM_LOCAL_MUSIC:
	#if (!defined CONFIG_LCMUSIC_TWS_DRC_EFFECT || defined CONFIG_DECODER_APE || defined CONFIG_DECODER_FLAC)
		if (is_tws)
			efx_stream_type = AUDIO_STREAM_TWS;
	#endif
		break;
	case AUDIO_STREAM_MUSIC:
		if (is_tws && efx_stream_type == AUDIO_STREAM_LINEIN) {
			efx_stream_type = AUDIO_STREAM_LINEIN;
		}
		break;
	default:
		break;
	}

	return efx_stream_type;
}

u8_t audio_policy_get_record_effect_type(u8_t stream_type,
			u8_t efx_stream_type)
{
	return efx_stream_type;
}

int audio_policy_get_record_audio_mode(u8_t stream_type)
{
	int audio_mode = AUDIO_MODE_STEREO;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_MIC_IN:
		audio_mode = AUDIO_MODE_MONO;
		break;
	}

	return audio_mode;
}

int audio_policy_get_record_left_channel_id(u8_t stream_type)
{
	int channel_id = AUDIO_ANALOG_MIC0;

	switch (stream_type) {
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
		channel_id = AUDIO_ANALOG_MIC0;
		break;
	case AUDIO_STREAM_LINEIN:
		channel_id = AUDIO_LINE_IN0;
		break;
	case AUDIO_STREAM_FM:
		channel_id = AUDIO_ANALOG_FM0;
		break;
	}

	return channel_id;
}

int audio_policy_get_record_right_channel_id(u8_t stream_type)
{
	int channel_id = AUDIO_ANALOG_MIC0;

	switch (stream_type) {
	case AUDIO_STREAM_LINEIN:
		channel_id = AUDIO_LINE_IN0;
		break;
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
		channel_id = AUDIO_ANALOG_MIC0;
		break;
	case AUDIO_STREAM_FM:
		channel_id = AUDIO_ANALOG_FM0;
		break;
	}

	return channel_id;
}

int audio_policy_get_record_input_gain(u8_t stream_type)
{
	int gain = 0;/* odb */

	switch (stream_type) {
	case AUDIO_STREAM_LINEIN:
		if (user_policy)
			gain = user_policy->audio_in_linein_gain;
	break;
	case AUDIO_STREAM_FM:
		if (user_policy)
			gain = user_policy->audio_in_fm_gain;
	break;
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_AI:
		if (user_policy->asqt_ext_info &&
			user_policy->asqt_ext_info->asqt_para_bin->mic_gain) {
			gain = user_policy->asqt_ext_info->asqt_para_bin->mic_gain
					+ user_policy->asqt_ext_info->asqt_para_bin->adc_gain;
		} else {
			gain = user_policy->audio_in_mic_gain;
		}
		break;
	}

	return gain;
}

int audio_policy_get_record_channel_mode(u8_t stream_type)
{
	int channel_mode =  AUDIO_DMA_MODE | AUDIO_DMA_RELOAD_MODE;

	switch (stream_type) {

	}

	return channel_mode;
}

int audio_policy_get_record_channel_support_aec(u8_t stream_type)
{
	int support_aec = false;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		support_aec = true;
		break;
	}

	return support_aec;
}

int audio_policy_get_record_channel_aec_tail_length(u8_t stream_type, u8_t sample_rate, bool in_debug)
{
	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		if (in_debug) {
			return (sample_rate > 8) ? 32 : 64;
		} else if (user_policy) {
			return (sample_rate > 8) ?
				user_policy->voice_aec_tail_length_16k :
				user_policy->voice_aec_tail_length_8k;
		} else {
			return (sample_rate > 8) ? 48 : 96;
		}
	default:
		return 0;
	}
}

int audio_policy_get_channel_resample(u8_t stream_type)
{
	int resample = false;

	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		/* resample = true; */
		break;
	}

	return resample;
}

int audio_policy_get_output_support_multi_track(u8_t stream_type)
{
	int support_multi_track = false;

#ifdef CONFIG_AUDIO_SUBWOOFER
	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_USOUND:
		support_multi_track = true;
		break;
	}
#endif

	return support_multi_track;
}

int audio_policy_get_record_channel_type(u8_t stream_type)
{
	int channel_type = AUDIO_CHANNEL_ADC;

	switch (stream_type) {
	case AUDIO_STREAM_SPDIF_IN:
		channel_type = AUDIO_CHANNEL_SPDIFRX;
		break;

	case AUDIO_STREAM_I2SRX_IN:
		channel_type = AUDIO_CHANNEL_I2SRX;
		break;

	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_USOUND:
		channel_type = AUDIO_CHANNEL_I2SRX;
		break;
	}
	return channel_type;
}

int audio_policy_get_pa_volume(u8_t stream_type, u8_t volume_level)
{
	int pa_volume = volume_level;

	if (!user_policy)
		goto exit;

	if (volume_level > user_policy->audio_out_volume_level) {
		volume_level = user_policy->audio_out_volume_level;
	}

	if (volume_level < 0) {
		volume_level = 0;
	}

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		if (user_policy->asqt_ext_info &&
			user_policy->asqt_ext_info->asqt_para_bin->max_pa_volume) {
			pa_volume = user_policy->asqt_ext_info->asqt_para_bin->max_pa_volume;
		} else {
			pa_volume = user_policy->voice_pa_table[volume_level];
		}
		break;
	case AUDIO_STREAM_USOUND:
		if (volume_level > 15) {
			volume_level = 15;
		}
		pa_volume = user_policy->usound_pa_table[volume_level];
		break;
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_TTS:
		if (user_policy->aset_volume_table &&
			user_policy->aset_volume_table->bEnable) {
			 /* aset volume unit: 0.001 dB */
			pa_volume = user_policy->aset_volume_table->nPAVal[volume_level];
		} else {
			pa_volume = user_policy->music_pa_table[volume_level];
		}
		break;
	}

exit:
	return pa_volume;
}

int audio_policy_get_da_volume(u8_t stream_type, u8_t volume_level)
{
	int da_volume = volume_level;

	if (!user_policy)
		goto exit;

	if (volume_level > user_policy->audio_out_volume_level) {
		volume_level = user_policy->audio_out_volume_level;
	}

	if (volume_level < 0) {
		volume_level = 0;
	}

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		da_volume = user_policy->voice_da_table[volume_level];
		break;
	case AUDIO_STREAM_USOUND:
		if (volume_level > 15) {
			volume_level = 15;
		}
		da_volume = user_policy->usound_da_table[volume_level];
		break;
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_TTS:
		if (user_policy->aset_volume_table &&
			user_policy->aset_volume_table->bEnable) {
			da_volume = user_policy->aset_volume_table->sDAVal[volume_level];
		} else {
			da_volume = user_policy->music_da_table[volume_level];
		}
		break;
	}

exit:
	return da_volume;
}

int audio_policy_get_volume_level_by_db(u8_t stream_type, int volume_db)
{
	int volume_level = 0;
	int max_level = user_policy->audio_out_volume_level;
	const int *volume_table = user_policy->music_pa_table;

	if (!user_policy)
		goto exit;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		volume_table = user_policy->music_pa_table;
		break;
	case AUDIO_STREAM_USOUND:
		volume_table = user_policy->usound_pa_table;
		max_level = 16;
		break;
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_TTS:
		if (user_policy->aset_volume_table &&
			user_policy->aset_volume_table->bEnable) {
			volume_table = user_policy->aset_volume_table->nPAVal;
		} else {
			volume_table = user_policy->music_pa_table;
		}
		break;
	}

	/* to 0.001 dB */
	volume_db *= 100;

	if (volume_db == volume_table[max_level - 1]) {
		volume_level = max_level;
	} else {
		for (int i = 0; i < max_level; i++) {
			if (volume_db < volume_table[i]) {
				volume_level = i;
				break;
			}
		}
	}

exit:
	return volume_level;
}


int audio_policy_get_volume_level(void)
{
	if (user_policy)
		return user_policy->audio_out_volume_level;

	return 16;
}

int audio_policy_check_tts_fixed_volume(void)
{
	if (user_policy)
		return user_policy->tts_fixed_volume;

	return 1;
}

aset_volume_table_v2_t *audio_policy_get_aset_volume_table(void)
{
	if (user_policy)
		return user_policy->aset_volume_table;

	return NULL;
}

asqt_ext_info_t *audio_policy_get_asqt_ext_info(void)
{
	if (user_policy)
		return user_policy->asqt_ext_info;

	return NULL;
}


int audio_policy_check_save_volume_to_nvram(void)
{
	if (user_policy)
		return user_policy->volume_saveto_nvram;

	return 1;
}

int audio_policy_get_upload_start_threshold(u8_t stream_type)
{
	int start_threshold = 0;

	switch(stream_type) {
	case AUDIO_STREAM_VOICE:
		start_threshold = 5;
		break;

	case AUDIO_STREAM_USOUND:
		start_threshold = 8;
		break;

	default:
		break;
	}

	return start_threshold;
}
int audio_policy_get_out_input_start_threshold(u8_t stream_type, u8_t exf_stream_type)
{
	int start_threshold = 0;

	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		/**btmusic master or single */
		if (btif_tws_get_dev_role() != BTSRV_TWS_SLAVE) {
				if (system_check_low_latencey_mode())
					start_threshold = 40;
				else
					start_threshold = 200;
		} else {
			/**only for slave*/
			if (exf_stream_type == AUDIO_STREAM_MUSIC) {
				if (!bt_manager_tws_check_is_woodpecker()) {
					start_threshold = 6;
				} else {
					if (system_check_low_latencey_mode())
						start_threshold = 40;
					else
						start_threshold = 60;
				}
			} else if (exf_stream_type == AUDIO_STREAM_LOCAL_MUSIC) {
				start_threshold = 60;
			} else {
				/**linein, micin, fm ,usound, spdifin, i2s in*/
				start_threshold = 6;
			}
		}
		break;
	case AUDIO_STREAM_VOICE:
		if (system_check_low_latencey_mode())
			start_threshold = 16;
		else
			if (bt_manager_tws_is_hfp_mode()) {
				start_threshold = 0;
			} else {
				start_threshold = 52;
			}
		break;
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
		start_threshold = 2;
		break;

	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_SPDIF_IN:
		start_threshold = 5;
		break;

	case AUDIO_STREAM_USOUND:
		if (bt_manager_tws_is_hfp_mode()) {
			start_threshold = 0;
		} else {
			start_threshold = 8;
		}
		break;

	case AUDIO_STREAM_TTS:
	default:
		start_threshold = 0;
		break;
	}

	return start_threshold;
}

int audio_policy_get_out_input_stop_threshold(u8_t stream_type,u8_t exf_stream_type)
{
	return 0;
}

int audio_policy_get_download_reduce_threshold(u8_t stream_type, u8_t exf_stream_type)
{
	int start_threshold = audio_policy_get_out_input_start_threshold(stream_type, exf_stream_type);
	int reduce_threshold = 0;

	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		if (system_check_low_latencey_mode())
			reduce_threshold = start_threshold - start_threshold / 4;
		else
			reduce_threshold = start_threshold - start_threshold / 4;
		break;
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_SPDIF_IN:
		if (btif_tws_get_dev_role() != BTSRV_TWS_NONE && !bt_manager_tws_is_hfp_mode()) {
			reduce_threshold = 45;
		} else {
			reduce_threshold = 10;
		}
		break;
	case AUDIO_STREAM_VOICE:
		if (system_check_low_latencey_mode())
			reduce_threshold = start_threshold - start_threshold / 2;
		else
			reduce_threshold = start_threshold - start_threshold / 4;
		reduce_threshold = 44;
		break;
	}

	return reduce_threshold;
}

int audio_policy_get_upload_reduce_threshold(u8_t stream_type, u8_t exf_stream_type)
{
	int start_threshold = audio_policy_get_upload_start_threshold(stream_type);

	int reduce_threshold = 0;

	switch (stream_type) {
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
		reduce_threshold = start_threshold - start_threshold / 2;
		break;
	default:
		break;
	}

	return reduce_threshold;

}
int audio_policy_get_reduce_threshold(u8_t stream_type, u8_t exf_stream_type, int monitor_type)
{

	if (monitor_type == APS_MONITOR_TYPE_DOWNLOAD) {
		return audio_policy_get_download_reduce_threshold(stream_type, exf_stream_type);
	} else if (monitor_type == APS_MONITOR_TYPE_UPLOAD) {
		return audio_policy_get_upload_reduce_threshold(stream_type, exf_stream_type);
	}

	return 0;
}

int audio_policy_get_download_increase_threshold(u8_t stream_type, u8_t exf_stream_type)
{
	int start_threshold = audio_policy_get_out_input_start_threshold(stream_type, exf_stream_type);
	int increase_threshold = 0;

	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		if (system_check_low_latencey_mode())
			increase_threshold = start_threshold + start_threshold / 4;
		else
			increase_threshold = start_threshold + start_threshold / 4;
		break;
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_SPDIF_IN:
		if (btif_tws_get_dev_role() != BTSRV_TWS_NONE && !bt_manager_tws_is_hfp_mode()) {
			increase_threshold = 65;
		} else {
			increase_threshold = 20;
		}

		break;
	case AUDIO_STREAM_VOICE:
		if (system_check_low_latencey_mode())
			increase_threshold = start_threshold + start_threshold / 2;
		else
			increase_threshold = start_threshold + start_threshold / 3;
		increase_threshold = 66;
		break;
	}
	return increase_threshold;

}

int audio_policy_get_upload_increase_threshold(u8_t stream_type, u8_t exf_stream_type)
{
	int start_threshold = audio_policy_get_upload_start_threshold(stream_type);

	int reduce_threshold = 0;

	switch (stream_type) {
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
		reduce_threshold = start_threshold + start_threshold / 2;
		break;
	default:
		break;
	}

	return reduce_threshold;

}

int audio_policy_get_increase_threshold(u8_t stream_type, u8_t exf_stream_type, int monitor_type)
{
	if (monitor_type == APS_MONITOR_TYPE_DOWNLOAD) {
		return audio_policy_get_download_increase_threshold(stream_type, exf_stream_type);
	} else if (monitor_type == APS_MONITOR_TYPE_UPLOAD) {
		return audio_policy_get_upload_increase_threshold(stream_type, exf_stream_type);
	}

	return 0;
}

int audio_policy_register(const struct audio_policy_t *policy)
{
	int size = sizeof(aset_para_bin_t);

	user_policy = policy;

	/**only for init default volume table*/
	media_effect_load("musicdrc.efx", &size, AUDIO_STREAM_MUSIC);

	return 0;
}


int audio_policy_get_tws_bitpool(void)
{
	return CONFIG_TWS_A2DP_BITPOOL;
}

int audio_policy_get_aps_mode(void)
{
#ifdef CONFIG_SF_APS
	return 1;
#else
	return 0;
#endif
}

