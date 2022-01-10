/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <sys_event.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include "btservice_api.h"

#define A2DP_ENDPOINT_MAX		CONFIG_BT_MAX_BR_CONN

static const u8_t a2dp_sbc_codec[] = {
	0x00,	/* BT_A2DP_AUDIO << 4 */
	0x00,	/* BT_A2DP_SBC */
	0xFF,	/* (SNK optional) 16000, 32000, (SNK mandatory)44100, 48000, mono, dual channel, stereo, join stereo */
	0xFF,	/* (SNK mandatory) Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
	0x02,	/* min bitpool */
	CONFIG_BT_A2DP_MAX_BITPOOL,	/* max bitpool */
};

static const u8_t a2dp_aac_codec[] = {
	0x00,	/* BT_A2DP_AUDIO << 4 */
	0x02,	/* BT_A2DP_MPEG2 */
	0xF0,	/* MPEG2 AAC LC, MPEG4 AAC LC, MPEG AAC LTP, MPEG4 AAC Scalable */
	0x01,	/* Sampling Frequecy 44100 */
	0x8F,	/* Sampling Frequecy 48000, channels 1, channels 2 */
	0xFF,	/* VBR, bit rate */
	0xFF,	/* bit rate */
	0xFF	/* bit rate */
};

static u8_t codec_id = 0;
static u8_t sample_rate = 0;

static void _bt_manager_a2dp_callback(btsrv_a2dp_event_e event, void *packet, int size)
{
	switch (event) {
	case BTSRV_A2DP_STREAM_OPENED:
	{
		SYS_LOG_INF("stream opened\n");
	}
	break;
	case BTSRV_A2DP_STREAM_CLOSED:
	{
		SYS_LOG_INF("stream closed\n");
		bt_manager_event_notify(BT_A2DP_STREAM_SUSPEND_EVENT, NULL, 0);
	}
	break;
	case BTSRV_A2DP_STREAM_STARED:
	{
		SYS_LOG_INF("stream started\n");
		bt_manager_set_status(BT_STATUS_PLAYING);
		bt_manager_event_notify(BT_A2DP_STREAM_START_EVENT, NULL, 0);
	}
	break;
	case BTSRV_A2DP_STREAM_SUSPEND:
	{
		SYS_LOG_INF("stream suspend\n");
		bt_manager_set_status(BT_STATUS_PAUSED);
		bt_manager_event_notify(BT_A2DP_STREAM_SUSPEND_EVENT, NULL, 0);
	}
	break;
	case BTSRV_A2DP_DATA_INDICATED:
	{
		static u8_t print_cnt;
		int ret = 0;

		bt_manager_stream_pool_lock();
		io_stream_t bt_stream = bt_manager_get_stream(STREAM_TYPE_A2DP);

		if (!bt_stream) {
			bt_manager_stream_pool_unlock();
			if (print_cnt == 0) {
				SYS_LOG_INF("stream is null\n");
			}
			print_cnt++;
			break;
		}

		if (stream_get_space(bt_stream) < size) {
			bt_manager_stream_pool_unlock();
			if (print_cnt == 0) {
				SYS_LOG_WRN(" stream is full\n");
			}
			print_cnt++;
			break;
		}

		ret = stream_write(bt_stream, packet, size);
		if (ret != size) {
			if (print_cnt == 0) {
				SYS_LOG_WRN("write %d error %d\n", size, ret);
			}
			print_cnt++;
			bt_manager_stream_pool_unlock();
			break;
		}
		bt_manager_stream_pool_unlock();
		print_cnt = 0;
		break;
	}
	case BTSRV_A2DP_CODEC_INFO:
	{
		u8_t *codec_info = (u8_t *)packet;

		codec_id = codec_info[0];
		sample_rate = codec_info[1];
		break;
	}
	case BTSRV_A2DP_CONNECTED:
		break;
	case BTSRV_A2DP_DISCONNECTED:
		break;
	default:
	break;
	}
}

int bt_manager_a2dp_profile_start(void)
{
	struct btsrv_a2dp_start_param param;

	memset(&param, 0, sizeof(param));
	param.cb = &_bt_manager_a2dp_callback;
	param.sbc_codec = (u8_t *)a2dp_sbc_codec;
	param.sbc_endpoint_num = A2DP_ENDPOINT_MAX;
	if (bt_manager_config_support_a2dp_aac()) {
		param.aac_codec = (u8_t *)a2dp_aac_codec;
		param.aac_endpoint_num = A2DP_ENDPOINT_MAX;
	}

	return btif_a2dp_start((struct btsrv_a2dp_start_param *)&param);
}

int bt_manager_a2dp_profile_stop(void)
{
	return btif_a2dp_stop();
}

int bt_manager_a2dp_check_state(void)
{
	return btif_a2dp_check_state();
}

int bt_manager_a2dp_send_delay_report(u16_t delay_time)
{
	return btif_a2dp_send_delay_report(delay_time);
}

int bt_manager_a2dp_get_codecid(void)
{
	SYS_LOG_INF("codec_id %d\n", codec_id);
	return codec_id;
}

int bt_manager_a2dp_get_sample_rate(void)
{
	//SYS_LOG_INF("sample_rate %d\n", sample_rate);
	return sample_rate;
}

