/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music media.
 */
#include "tws.h"
#include "media_mem.h"
#include "ui_manager.h"
#include <ringbuff_stream.h>
#include "tts_manager.h"

extern io_stream_t dma_upload_stream_create(void);

extern u32_t asdp_set_state_sample_rate( u32_t sample_rate);

static io_stream_t _tws_a2dp_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC));

	if (!input_stream) {
		goto exit;
	}

	ret = stream_open(input_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", input_stream);
	return	input_stream;
}

void tws_a2dp_start_play(u8_t stream_type, u8_t codec_id, u8_t sample_rate, u8_t volume)
{
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;
	struct tws_app_t *tws = tws_get_app();

	if (!tws)
		return;

#ifdef CONFIG_USP
	asdp_set_state_sample_rate(sample_rate);
#endif

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif
	if (tws->player) {
		if (tws->bt_stream) {
			bt_manager_set_codec(codec_id);
			bt_manager_set_stream(STREAM_TYPE_A2DP, tws->bt_stream);
			media_player_play(tws->player);
			SYS_LOG_INF("already open\n");
			return;
		}
		media_player_stop(tws->player);
		media_player_close(tws->player);
	}

	tws_view_show_play_status(true);

	memset(&init_param, 0, sizeof(media_init_param_t));
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;

	SYS_LOG_INF("codec_id: %d sample_rate %d\n", codec_id, sample_rate);

	if (codec_id == 0) {
		init_param.format = SBC_TYPE;
	} else if (codec_id == 2) {
		init_param.format = AAC_TYPE;
	}

	input_stream = _tws_a2dp_create_inputstream();
	init_param.stream_type = AUDIO_STREAM_MUSIC;
	init_param.efx_stream_type = stream_type;
	init_param.sample_rate = sample_rate;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.support_tws = 1;
	init_param.dumpable = 1;

#ifdef CONFIG_TWS_MONO_MODE
	init_param.channels = 1;
#else
	init_param.channels = 2;
#endif

	tws->bt_stream = input_stream;
	bt_manager_set_codec(codec_id);
	bt_manager_set_stream(STREAM_TYPE_A2DP, input_stream);

	tws->player = media_player_open(&init_param);
	if (!tws->player) {
		goto err_exit;
	}
	printk("%s st_type:%d vol:%d\n", __FUNCTION__, stream_type, volume);
	audio_system_set_stream_volume(stream_type, volume);

	tws->media_opened = 1;

	media_player_fade_in(tws->player, 60);

	media_player_play(tws->player);

	SYS_LOG_INF(" %p\n", tws->player);

	return;

err_exit:
	if (input_stream) {
		stream_destroy(input_stream);
	}

	SYS_LOG_ERR("open failed\n");

}

void tws_a2dp_stop_play(void)
{
	struct tws_app_t *tws = tws_get_app();


	if (!tws ||	!tws->media_opened)
		return;

	if (!tws->player)
		return;

	media_player_fade_out(tws->player, 100);

	tws_view_show_play_status(false);

	tws->media_opened = 0;

	if (tws->bt_stream) {
		stream_close(tws->bt_stream);
	}

	bt_manager_set_stream(STREAM_TYPE_A2DP, NULL);
#ifdef CONFIG_USP
	asdp_set_state_sample_rate(0);
#endif
	media_player_stop(tws->player);
	media_player_close(tws->player);

	SYS_LOG_INF(" %p ok\n", tws->player);

	tws->player = NULL;

	if (tws->bt_stream) {
		stream_destroy(tws->bt_stream);
		tws->bt_stream = NULL;
	}
}

static io_stream_t tws_sco_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = NULL;

	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_VOICE),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_VOICE));

	if(!input_stream) {
		goto exit;
	}

	ret = stream_open(input_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p \n",input_stream);
	return 	input_stream;
}

void tws_sco_start_play(void)
{
	media_init_param_t init_param;
	uint8_t codec_id = bt_manager_sco_get_codecid();
	uint8_t sample_rate = bt_manager_sco_get_sample_rate();
	struct tws_app_t *tws = tws_get_app();

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(true);
	tts_manager_lock();
#endif

	if (tws->player) {
		tws_sco_stop_play();
		SYS_LOG_INF("already open\n");
	}

	memset(&init_param, 0, sizeof(media_init_param_t));
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;

	if (!tws->upload_stream) {
		tws->upload_stream = sco_upload_stream_create(codec_id);
		if (!tws->upload_stream) {
			SYS_LOG_INF("stream create failed");
			goto err_exit;
		}

		if (stream_open(tws->upload_stream, MODE_OUT)) {
			stream_destroy(tws->upload_stream);
			tws->upload_stream = NULL;
			SYS_LOG_INF("stream open failed ");
			goto err_exit;
		}
	}

	if (codec_id == MSBC_TYPE) {
		init_param.capture_bit_rate = 26;
	}

	SYS_LOG_INF("codec_id %d sample rate: %d\n",codec_id, sample_rate);

	tws->bt_stream = tws_sco_create_inputstream();
	init_param.stream_type = AUDIO_STREAM_VOICE;
	init_param.efx_stream_type = AUDIO_STREAM_VOICE;
	init_param.format = codec_id;
	init_param.sample_rate = sample_rate;
	init_param.input_stream = tws->bt_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.capture_format = codec_id;
	init_param.capture_sample_rate_input = sample_rate;
	init_param.capture_sample_rate_output = sample_rate;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = tws->upload_stream;
	init_param.support_tws = 1;
	init_param.dumpable = true;

	if (audio_policy_get_out_audio_mode(init_param.stream_type) == AUDIO_MODE_STEREO) {
		init_param.channels = 2;
	} else {
		init_param.channels = 1;
	}

	if (audio_policy_get_record_audio_mode(init_param.stream_type) == AUDIO_MODE_STEREO) {
		init_param.capture_channels = 2;
	} else {
		init_param.capture_channels = 1;
	}

	bt_manager_set_stream(STREAM_TYPE_SCO, tws->bt_stream);

	tws->player = media_player_open(&init_param);
	if (!tws->player) {
		goto err_exit;
	}

	int vol = audio_system_get_stream_volume(tws->stream_type);
	audio_system_set_stream_volume(tws->stream_type, vol);

	media_player_fade_in(tws->player, 60);

	media_player_play(tws->player);

	//bt_manager_hfp_sync_vol_to_remote(audio_system_get_stream_volume(AUDIO_STREAM_VOICE));

	SYS_LOG_INF("open sucessed %p ",tws->player);
#ifdef CONFIG_SEG_LED_MANAGER
	if (bt_manager_sco_get_codecid() == MSBC_TYPE
		&& bt_manager_hfp_get_status() != BT_STATUS_SIRI) {
		seg_led_display_string(SLED_NUMBER2, "-", true);
	}
#endif

	return;

err_exit:
	if (tws->upload_stream) {
		stream_close(tws->upload_stream);
		stream_destroy(tws->upload_stream);
		tws->upload_stream = NULL;
	}
#ifdef CONFIG_PLAYTTS
	tts_manager_unlock();
#endif

	SYS_LOG_ERR("open failed \n");
	return;
}

void tws_sco_stop_play(void)
{
	struct tws_app_t *tws = tws_get_app();

	if (!tws->player) {
		/**avoid noise when hang up btcall */
		os_sleep(100);
		return;
	}

	media_player_fade_out(tws->player, 100);

	bt_manager_set_stream(STREAM_TYPE_SCO, NULL);


	if (tws->bt_stream) {
		stream_close(tws->bt_stream);
	}

	media_player_stop(tws->player);

	media_player_close(tws->player);

	if (tws->upload_stream) {
		stream_close(tws->upload_stream);
		stream_destroy(tws->upload_stream);
		tws->upload_stream = NULL;
	}

	if (tws->bt_stream) {
		stream_destroy(tws->bt_stream);
		tws->bt_stream = NULL;
	}

	SYS_LOG_INF(" %p ok \n",tws->player);

	tws->player = NULL;

#ifdef CONFIG_PLAYTTS
	tts_manager_unlock();
#endif

}

