/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tws event
 */
#include "tws.h"
#include "bt_manager.h"
#include "tts_manager.h"
#include <input_manager.h>
#include <property_manager.h>

void tws_bt_event_proc(struct app_msg *msg)
{
	struct tws_app_t *tws = (struct tws_app_t *)tws_get_app();

	switch (msg->cmd) {
	case BT_A2DP_STREAM_START_EVENT:
	{
		SYS_LOG_INF("STREAM_START\n");
		/* tws_a2dp_start_play(); */
		/* tws->playing = 1; */
		break;
	}
	case BT_TWS_START_PLAY:
	{
		u8_t *param = msg->ptr;

		SYS_LOG_INF("BT_TWS_START_PLAY %p %d %d %d %d\n",
					msg->ptr, param[1], param[2], param[3], param[4]);
		tws->stream_type = param[2];
		tws->codec_id = param[3];
		tws->sample_rate = param[4];
		if(param[2] != AUDIO_STREAM_VOICE){
			tws_a2dp_start_play(param[2], param[3], param[4], param[5]);
			tws->playing = 1;
		} else {
			audio_system_set_stream_volume(tws->stream_type, param[5]);
		}
		break;
	}
	case BT_TWS_STOP_PLAY:
	{
		if(tws->sco_playing){
			tws_sco_stop_play();
			tws->sco_playing = 0;
		}else if(tws->playing){
			tws_a2dp_stop_play();
			tws->playing = 0;
		}
		break;
	}
	case BT_A2DP_STREAM_DATA_IND_EVENT:
	{
		tws->playing = 1;
		break;
	}

	case BT_A2DP_STREAM_SUSPEND_EVENT:
	{
		tws_a2dp_stop_play();
		tws->playing = 0;
		break;
	}

	case BT_A2DP_CONNECTION_EVENT:
	{
		tws->a2dp_connected = 1;
		break;
	}

	case BT_A2DP_DISCONNECTION_EVENT:
	{
		tws_a2dp_stop_play();
		tws->playing = 0;
		tws->a2dp_connected = 0;
		break;
	}

	case BT_RMT_VOL_SYNC_EVENT:
	{
	#if 0
		if (tws->sco_playing) {
			audio_system_set_stream_volume(AUDIO_STREAM_VOICE, msg->value);
		} else {
			audio_system_set_stream_volume(AUDIO_STREAM_MUSIC, msg->value);
		}
	#else
		printk("BT_RMT_VOL_SYNC_EVENT type:%d, vol:%d\n", tws->stream_type, msg->value);
		audio_system_set_stream_volume(tws->stream_type, msg->value);
	#endif
		if (tws && tws->player)
			media_player_set_volume(tws->player, msg->value, msg->value);
		break;
	}
	case BT_TWS_CHANNEL_MODE_SWITCH:
	{
		u8_t *param = msg->ptr;
		property_set_int("TWS_MODE", param[0]);
		if (tws && tws->player && param[0] != MEDIA_OUTPUT_MODE_DEFAULT) {
			if (param[0] == MEDIA_OUTPUT_MODE_LEFT || param[0] == MEDIA_OUTPUT_MODE_RIGHT) {
				media_player_set_output_mode(tws->player, param[0]);
			}
		}
		SYS_LOG_INF("%d\n", param[0]);
		break;
	}

	case BT_HFP_ESCO_ESTABLISHED_EVENT:
	{
		tws_sco_start_play();
		tws->sco_playing = 1;
		break;
	}

	case BT_HFP_ESCO_RELEASED_EVENT:
	{
		tws_sco_stop_play();
		tws->sco_playing = 0;
		break;
	}

	default:
		break;
	}
}

#ifdef CONFIG_BT_TWS_US281B
static void tws_input_event_proc_to_us281b(struct app_msg *msg)
{
	u8_t tws_event_data[5];

	if (bt_manager_tws_check_is_woodpecker()) {
		memset(tws_event_data, 0, sizeof(tws_event_data));
		tws_event_data[0] = TWS_INPUT_EVENT;
		memcpy(&tws_event_data[1], &msg->value, 4);

	#ifdef CONFIG_TWS
		bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
	#endif
	} else {
		u32_t in_key;
		u32_t key_type = 0;
		u32_t key_value = 0;
		u32_t send_value;

		in_key = (u32_t)msg->value;
		key_type = in_key & KEY_TYPE_ALL;
		key_value = in_key & 0xFF;

		/* zs285a not send short down, but us283a send short down and up */
		if (key_type == KEY_TYPE_SHORT_UP) {
			send_value = KEY_TYPE_SHORT_DOWN | (key_value << 8);
			tws_event_data[0] = TWS_INPUT_EVENT;
			memcpy(&tws_event_data[1], &send_value, 4);
	#ifdef CONFIG_TWS
			bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
	#endif
		}

		if (key_type == KEY_TYPE_LONG_UP) {
			send_value = KEY_TYPE_SHORT_UP | (key_value << 8);
			tws_event_data[0] = TWS_INPUT_EVENT;
			memcpy(&tws_event_data[1], &send_value, 4);
	#ifdef CONFIG_TWS
			bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
	#endif
		}

		if (key_type == KEY_TYPE_DOUBLE_CLICK) {
			tws_event_data[0] = TWS_INPUT_EVENT;
			send_value = KEY_TYPE_SHORT_DOWN | (key_value << 8);
			memcpy(&tws_event_data[1], &send_value, 4);
	#ifdef CONFIG_TWS
			bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
	#endif

			send_value = KEY_TYPE_SHORT_UP | (key_value << 8);
			memcpy(&tws_event_data[1], &send_value, 4);
	#ifdef CONFIG_TWS
			bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
	#endif
			send_value = KEY_TYPE_SHORT_DOWN | (key_value << 8);
			memcpy(&tws_event_data[1], &send_value, 4);
	#ifdef CONFIG_TWS
			bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
	#endif
			key_type = KEY_TYPE_SHORT_UP;
			SYS_LOG_INF("key 0x%x convert to 0x%x\n", in_key, send_value);
		}

		send_value = key_type | (key_value << 8);
		tws_event_data[0] = TWS_INPUT_EVENT;
		memcpy(&tws_event_data[1], &send_value, 4);
	#ifdef CONFIG_TWS
		bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
	#endif
		SYS_LOG_INF("key 0x%x convert to 0x%x\n", in_key, send_value);
	}
}
#endif

void tws_input_event_proc(struct app_msg *msg)
{
#ifdef CONFIG_BT_TWS_US281B
	tws_input_event_proc_to_us281b(msg);
#else
	u8_t tws_event_data[5];

	memset(tws_event_data, 0, sizeof(tws_event_data));
	tws_event_data[0] = TWS_INPUT_EVENT;
	memcpy(&tws_event_data[1], &msg->value, 4);

#ifdef CONFIG_TWS
	bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
#endif
#endif
}

void tws_tts_event_proc(struct app_msg *msg)
{
	struct tws_app_t *tws = (struct tws_app_t *)tws_get_app();

	if (!tws)
		return;

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
	if (tws->playing) {
		tws->need_resume_play = 1;
		tws_a2dp_stop_play();
	}
	break;
	case TTS_EVENT_STOP_PLAY:
	if (tws->playing && tws->need_resume_play) {
		tws->need_resume_play = 0;
		tws_a2dp_start_play(tws->stream_type, tws->codec_id, tws->sample_rate,
							audio_system_get_stream_volume(tws->stream_type));
	}
	break;
	}
}

