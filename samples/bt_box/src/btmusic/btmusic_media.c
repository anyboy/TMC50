/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music media.
 */
#include "btmusic.h"
#include "media_mem.h"
#include <ringbuff_stream.h>
#include "tts_manager.h"
#include "ui_manager.h"

extern io_stream_t dma_upload_stream_create(void);

static io_stream_t _bt_music_a2dp_create_inputstream(void)
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

void bt_music_a2dp_start_play(void)
{
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;
	struct btmusic_app_t *btmusic = btmusic_get_app();
	u8_t codec_id = bt_manager_a2dp_get_codecid();
	u8_t sample_rate = bt_manager_a2dp_get_sample_rate();
#ifdef CONFIG_BT_MUSIC_FREQPOINT_ENERGY_DEMO
	media_freqpoint_energy_info_t freqpoint_energy_info; 
#endif

	if (!btmusic)
		return;

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	if (btmusic->player) {
		if (btmusic->bt_stream) {
			bt_manager_set_codec(codec_id);
			bt_manager_set_stream(STREAM_TYPE_A2DP, btmusic->bt_stream);
			media_player_play(btmusic->player);
			SYS_LOG_INF("already open\n");
			return;
		}
		media_player_stop(btmusic->player);
		media_player_close(btmusic->player);
		os_sleep(200);
	}

	btmusic_view_show_play_paused(true);

	memset(&init_param, 0, sizeof(media_init_param_t));
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;

	SYS_LOG_INF("codec_id: %d sample_rate %d\n", codec_id, sample_rate);

	if (codec_id == 0) {
		init_param.format = SBC_TYPE;
	} else if (codec_id == 2) {
		init_param.format = AAC_TYPE;
	}
	input_stream = _bt_music_a2dp_create_inputstream();
	init_param.stream_type = AUDIO_STREAM_MUSIC;
	init_param.efx_stream_type = 0;
	init_param.sample_rate = sample_rate;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.support_tws = 1;
	init_param.dumpable = 1;

	if (audio_policy_get_out_audio_mode(init_param.stream_type) == AUDIO_MODE_STEREO) {
		init_param.channels = 2;
	} else {
		init_param.channels = 1;
	}

	btmusic->bt_stream = input_stream;
	bt_manager_set_codec(codec_id);
	bt_manager_set_stream(STREAM_TYPE_A2DP, input_stream);

	bt_manager_avrcp_sync_vol_to_remote(audio_system_get_stream_volume(AUDIO_STREAM_MUSIC));

	btmusic->player = media_player_open(&init_param);
	if (!btmusic->player) {
		goto err_exit;
	}

	btmusic->media_opened = 1;

	media_player_fade_in(btmusic->player, 60);

#ifdef CONFIG_BT_MUSIC_FREQPOINT_ENERGY_DEMO
	memset(&freqpoint_energy_info, 0x0, sizeof(freqpoint_energy_info));
	freqpoint_energy_info.num_points = 9;
	freqpoint_energy_info.values[0] = 64;
	freqpoint_energy_info.values[1] = 125;
	freqpoint_energy_info.values[2] = 250;
	freqpoint_energy_info.values[3] = 500;
	freqpoint_energy_info.values[4] = 1000;
	freqpoint_energy_info.values[5] = 2000;
	freqpoint_energy_info.values[6] = 4000;
	freqpoint_energy_info.values[7] = 8000;
	freqpoint_energy_info.values[8] = 16000;
	media_player_set_freqpoint_energy(btmusic->player, &freqpoint_energy_info);
#endif

	media_player_play(btmusic->player);

	SYS_LOG_INF(" %p\n", btmusic->player);

	return;

err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
	}

	SYS_LOG_ERR("open failed\n");

}

void bt_music_a2dp_stop_play(void)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (!btmusic ||	!btmusic->media_opened)
		return;

	if (!btmusic->player)
		return;

	media_player_fade_out(btmusic->player, 60);

	/** reserve time to fade out*/
	os_sleep(60);

	btmusic_view_show_play_paused(false);

	btmusic->media_opened = 0;

	if (btmusic->bt_stream) {
		stream_close(btmusic->bt_stream);
	}

	bt_manager_set_stream(STREAM_TYPE_A2DP, NULL);

	media_player_stop(btmusic->player);
	media_player_close(btmusic->player);

	SYS_LOG_INF(" %p ok\n", btmusic->player);

	btmusic->player = NULL;

	if (btmusic->bt_stream) {
		stream_destroy(btmusic->bt_stream);
		btmusic->bt_stream = NULL;
	}

}

#ifdef CONFIG_BT_MUSIC_FREQPOINT_ENERGY_DEMO
int bt_music_a2dp_get_freqpoint_energy(media_freqpoint_energy_info_t *info)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (!btmusic ||	!btmusic->media_opened)
		return -1;

	if (!btmusic->player)
		return -1;

	return media_player_get_freqpoint_energy(btmusic->player, info);
}
#endif


