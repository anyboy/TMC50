/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input media
 */


#include "audio_input.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

int audio_input_get_status(void)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	if (!audio_input)
		return AUDIO_INPUT_STATUS_NULL;

	if (audio_input->playing)
		return AUDIO_INPUT_STATUS_PLAYING;
	else
		return AUDIO_INPUT_STATUS_PAUSED;
}
static io_stream_t _audio_input_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = NULL;

	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, audio_input_get_audio_stream_type(app_manager_get_current_app())),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, audio_input_get_audio_stream_type(app_manager_get_current_app())));
	if (!input_stream) {
		goto exit;
	}

	ret = stream_open(input_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", input_stream);
	return	input_stream;
}

void audio_input_start_play(void)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;
#ifdef CONFIG_AUDIO_INPUT_ENERGY_DETECT_DEMO
	energy_detect_param_t energy_detect_param;
#endif

	if (!audio_input)
		return;

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	if (audio_input->player) {
		SYS_LOG_INF(" player already open ");
		return;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	input_stream = _audio_input_create_inputstream();
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;
	init_param.stream_type = audio_input_get_audio_stream_type(app_manager_get_current_app());
	init_param.efx_stream_type = init_param.stream_type;
	init_param.format = PCM_TYPE;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.support_tws = 1;
	init_param.dumpable = 1;

	init_param.capture_format = PCM_TYPE;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = input_stream;

	if (init_param.stream_type == AUDIO_STREAM_LINEIN) {
		/* TWS requires sbc encoder which only support 32/44.1/48 kHz */
		init_param.channels = 2;
		init_param.sample_rate = 48;
		init_param.capture_channels = 2;
		init_param.capture_sample_rate_input = 48;
		init_param.capture_sample_rate_output = 48;
	} else if (init_param.stream_type == AUDIO_STREAM_SPDIF_IN) {
		init_param.channels = 2;
		init_param.sample_rate = 48;
		init_param.capture_channels = 2;
		init_param.capture_sample_rate_input = audio_input->spdif_sample_rate;
		init_param.capture_sample_rate_output = 48;
	} else if (init_param.stream_type == AUDIO_STREAM_I2SRX_IN) {
		init_param.channels = 2;
		init_param.sample_rate = 48;
		init_param.capture_channels = 2;
		init_param.capture_sample_rate_input = audio_input->i2srx_sample_rate? audio_input->i2srx_sample_rate:48;
		init_param.capture_sample_rate_output = 48;
	} else if (init_param.stream_type == AUDIO_STREAM_MIC_IN) {
		init_param.channels = 1;
		init_param.sample_rate = 48;
		init_param.capture_channels = 1;
		init_param.capture_sample_rate_input = 16;
		init_param.capture_sample_rate_output = 48;
	} else {
		init_param.channels = 1;
		init_param.sample_rate = 16;
		init_param.capture_channels = 1;
		init_param.capture_sample_rate_input = 16;
		init_param.capture_sample_rate_output = 16;
	}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	if(init_param.capture_sample_rate_input != init_param.capture_sample_rate_output) {
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "media");
		audio_input->need_resample = 1;
	} else {
		audio_input->need_resample = 0;
	}
#endif

	audio_input->player = media_player_open(&init_param);
	if (!audio_input->player) {
		SYS_LOG_ERR("player open failed\n");
		goto err_exit;
	}

	audio_input->audio_input_stream = init_param.input_stream;

#ifdef CONFIG_AUDIO_INPUT_ENERGY_DETECT_DEMO
	energy_detect_param.enable = true;
	energy_detect_param.lowpower_threshold = 5;
	media_player_set_energy_detect(audio_input->player, &energy_detect_param);
#endif

	media_player_fade_in(audio_input->player, 60);

	media_player_play(audio_input->player);

	audio_input->playing = 1;

	SYS_LOG_INF("player open sucessed %p ", audio_input->player);
	return;
err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
	}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	if (audio_input->need_resample) {
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "media");
	}
#endif

}

void audio_input_stop_play(void)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	if (!audio_input)
		return;

	if (!audio_input->player)
		return;

	media_player_fade_out(audio_input->player, 60);

	/** reserve time to fade out*/
	os_sleep(60);

	if (audio_input->audio_input_stream)
		stream_close(audio_input->audio_input_stream);

	media_player_stop(audio_input->player);

	media_player_close(audio_input->player);

	audio_input->playing = 0;

	audio_input->player = NULL;
	if (audio_input->audio_input_stream) {
		stream_destroy(audio_input->audio_input_stream);
		audio_input->audio_input_stream = NULL;
	}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	if (audio_input->need_resample) {
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "media");
	}
#endif

	SYS_LOG_INF("stopped\n");

}

