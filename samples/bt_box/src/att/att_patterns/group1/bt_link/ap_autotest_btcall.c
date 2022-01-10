/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief autotest bt call
 */


#include "ap_autotest_bt_link.h"
#include "media_player.h"
#include "media_mem.h"
#include "audio_system.h"
#include "audio_policy.h"
#include "ringbuff_stream.h"

io_stream_t att_upload_stream;
io_stream_t att_input_stream;
media_player_t *att_player;

static io_stream_t att_btcall_sco_create_inputstream(void)
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
	SYS_LOG_INF(" %p ",input_stream);
	return 	input_stream;
}

bool att_btcall_start_play(void)
{
    bool ret = false;
	media_init_param_t init_param;
	uint8_t codec_id = bt_manager_sco_get_codecid();
	uint8_t sample_rate = bt_manager_sco_get_sample_rate();

	memset(&init_param, 0, sizeof(media_init_param_t));
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;

    att_upload_stream = sco_upload_stream_create(codec_id);
    if(!att_upload_stream) {
        SYS_LOG_INF("stream create failed!\n");
        goto err_exit;
    }

    if(stream_open(att_upload_stream, MODE_OUT)) {
        stream_destroy(att_upload_stream);
        att_upload_stream = NULL;
        SYS_LOG_INF("stream open failed\n");
        goto err_exit;
    }
    
	if (codec_id == MSBC_TYPE) {
		init_param.capture_bit_rate = 26;
	}

	SYS_LOG_INF("codec_id: %d, sample rate: %d",codec_id, sample_rate);
    att_input_stream = att_btcall_sco_create_inputstream();
	init_param.stream_type = AUDIO_STREAM_VOICE;
	init_param.efx_stream_type = AUDIO_STREAM_VOICE;
	init_param.format = codec_id;
	init_param.sample_rate = sample_rate;
    init_param.input_stream = att_input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.capture_format = codec_id;
	init_param.capture_sample_rate_input = sample_rate;
	init_param.capture_sample_rate_output = sample_rate;
	init_param.capture_input_stream = NULL;
    init_param.capture_output_stream = att_upload_stream;
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

    bt_manager_set_stream(STREAM_TYPE_SCO, att_input_stream);

	att_player = media_player_open(&init_param);
	if (!att_player) {
		goto err_exit;
	}

	media_player_fade_in(att_player, 60);

	media_player_play(att_player);

	SYS_LOG_INF("open sucessed %p ",att_player);

    ret = true;
    return ret;

err_exit:
    if(att_upload_stream) {
        stream_close(att_upload_stream);
        stream_destroy(att_upload_stream);
        att_upload_stream = NULL;
    }

	SYS_LOG_ERR("open failed \n");
	return ret;
}

void att_btcall_stop_play(void)
{

    media_player_fade_out(att_player, 100);

	os_sleep(100);

	bt_manager_set_stream(STREAM_TYPE_SCO, NULL);


	if (att_input_stream) {
		stream_close(att_input_stream);
	}

	media_player_stop(att_player);

	media_player_close(att_player);

	if (att_upload_stream) {
		stream_close(att_upload_stream);
		stream_destroy(att_upload_stream);
		att_upload_stream = NULL;
	}

	if (att_input_stream) {
		stream_destroy(att_input_stream);
		att_input_stream = NULL;
	}

	SYS_LOG_INF(" %p ok \n",att_player);

	att_player = NULL;

}
