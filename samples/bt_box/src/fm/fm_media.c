/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief fm media
 */


#include "fm.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
#include "seg_led_manager.h"

int fm_get_status(void)
{
	struct fm_app_t *fm = fm_get_app();

	if (!fm)
		return FM_STATUS_NULL;

	return FM_STATUS_ALL;

	if (fm->playing)
		return FM_STATUS_PLAYING;
	else
		return FM_STATUS_PAUSED;
}
static io_stream_t _fm_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = NULL;

	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_FM),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_FM));
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

void fm_delay_start_play(struct fm_app_t *fm)
{
	fm_view_play_freq(fm->current_freq);
}

void fm_start_play(void)
{
	struct fm_app_t *fm = fm_get_app();
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;

	if (!fm)
		return;

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	if (fm->player) {
		SYS_LOG_INF(" player already open ");
		return;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	input_stream = _fm_create_inputstream();
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_FM;
	init_param.efx_stream_type = init_param.stream_type;
	init_param.format = PCM_TYPE;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.dumpable = 1;

	init_param.capture_format = PCM_TYPE;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = input_stream;

	init_param.support_tws = 1;
	init_param.channels = 2;
	init_param.sample_rate = 48;
	init_param.capture_channels = 2;
	init_param.capture_sample_rate_input = 48;
	init_param.capture_sample_rate_output = 48;

	fm->player = media_player_open(&init_param);
	if (!fm->player) {
		SYS_LOG_ERR("player open failed\n");
		goto err_exit;
	}

	/*set adc gain*/
	audio_system_set_microphone_volume(AUDIO_STREAM_FM, 2);

	fm->fm_stream = init_param.input_stream;

	media_player_fade_in(fm->player, 60);

	media_player_play(fm->player);

	fm->playing = 1;

	if (!fm->mute_flag) {
		fm_view_show_play_status(false);
	}

	SYS_LOG_INF("player open sucessed %p ", fm->player);
	return;

err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
	}

}

void fm_stop_play(void)
{
	struct fm_app_t *fm = fm_get_app();

	if (!fm)
		return;

	if (!fm->player)
		return;

	media_player_fade_out(fm->player, 60);

	/** reserve time to fade out*/
	os_sleep(60);

	if (fm->fm_stream)
		stream_close(fm->fm_stream);

	media_player_stop(fm->player);

	media_player_close(fm->player);
	fm->playing = 0;

	if (fm->mute_flag) {
		fm_view_show_play_status(true);
	}

	fm->player = NULL;
	if (fm->fm_stream) {
		stream_destroy(fm->fm_stream);
		fm->fm_stream = NULL;
	}

	SYS_LOG_INF("stopped\n");

}

