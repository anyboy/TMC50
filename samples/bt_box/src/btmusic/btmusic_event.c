/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music event
 */
#include "btmusic.h"
#include <ui_manager.h>
#include <esd_manager.h>
#include "tts_manager.h"
static bool avrcp_connected = false;

void bt_music_tts_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	btmusic->need_resume_play = 0;
	bt_manager_a2dp_check_state();
}

void btmusic_bt_event_proc(struct app_msg *msg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
#ifdef CONFIG_ESD_MANAGER
	u8_t playing = 0;
#endif
	SYS_LOG_INF("bt event %d\n", msg->cmd);
	switch (msg->cmd) {

	case BT_CONNECTION_EVENT:
	{
		btmusic_view_show_connected();
		break;
	}

	case BT_DISCONNECTION_EVENT:
	{
		btmusic_view_show_disconnected();
		break;
	}
	case BT_TWS_CONNECTION_EVENT:
		btmusic_view_show_tws_connect(true);
		break;
	case BT_TWS_DISCONNECTION_EVENT:
		btmusic_view_show_tws_connect(false);
		if (btmusic->player) {
			media_player_set_output_mode(btmusic->player, MEDIA_OUTPUT_MODE_DEFAULT);
		}
		break;
	case BT_A2DP_STREAM_START_EVENT:
	{
		SYS_LOG_INF("STREAM_START\n");
		bt_music_a2dp_start_play();
		btmusic->playing = 1;
	#ifdef CONFIG_ESD_MANAGER
		playing = btmusic->playing;
		esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
	#endif
		break;
	}

	case BT_A2DP_STREAM_DATA_IND_EVENT:
	{
		btmusic->playing = 1;
		break;
	}

	case BT_A2DP_STREAM_SUSPEND_EVENT:
	{
		SYS_LOG_INF("STREAM_SUSPEND\n");
		bt_music_a2dp_stop_play();
		btmusic->playing = 0;
	#ifdef CONFIG_ESD_MANAGER
		playing = btmusic->playing;
		esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
	#endif
		break;
	}

	case BT_AVRCP_CONNECTION_EVENT:
	{
	#ifdef CONFIG_ESD_MANAGER
		bt_music_esd_restore();
	#endif
		avrcp_connected = 1;
		break;
	}

	case BT_AVRCP_DISCONNECTION_EVENT:
	{
		avrcp_connected = 0;
		break;
	}

	case BT_AVRCP_UPDATE_ID3_INFO_EVENT:
	{
		struct bt_id3_info *pinfo = (struct bt_id3_info *)msg->ptr;
		btmusic_view_show_id3_info(pinfo);
		break;
	}

	case BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT:
	{
		int param = *(int *)(msg->ptr);

		if (param == BT_STATUS_PAUSED) {
			btmusic->playing = 0;
			//bt_music_a2dp_stop_play();
		} else if (param == BT_STATUS_PLAYING) {
			btmusic->playing = 1;
			bt_manager_a2dp_check_state();
		}
		break;
	}

	case BT_AVRCP_TRACK_CHANGED_EVENT:
	{
		bt_manager_avrcp_get_id3_info();
		break;
	}
	
	case BT_RMT_VOL_SYNC_EVENT:
	{
		audio_system_set_stream_volume(AUDIO_STREAM_MUSIC, msg->value);
		if (btmusic && btmusic->player)
			media_player_set_volume(btmusic->player, msg->value, msg->value);
		break;
	}

	case BT_REQ_RESTART_PLAY:
	{
		bt_music_a2dp_stop_play();
		btmusic->playing = 0;
		os_sleep(200);
		bt_manager_a2dp_check_state();
		break;
	}
	default:
		break;
	}
}

void btmusic_input_event_proc(struct app_msg *msg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

	switch (msg->cmd) {
	case MSG_BT_PLAY_PAUSE_RESUME:
	{
		if (btmusic->playing) {
			bt_manager_avrcp_pause();
			btmusic->playing = 0;
			bt_music_a2dp_stop_play();
			if (thread_timer_is_running(&btmusic->monitor_timer)) {
				thread_timer_stop(&btmusic->monitor_timer);
			}
			thread_timer_init(&btmusic->monitor_timer, bt_music_tts_delay_resume, NULL);
			thread_timer_start(&btmusic->monitor_timer, 3000, 0);
		} else {
			bt_manager_avrcp_play();
			btmusic->playing = 1;
			bt_manager_a2dp_check_state();
		}

		if (avrcp_connected) {
			btmusic_view_show_play_paused(btmusic->playing);
		}

		break;
	}

	case MSG_BT_PLAY_NEXT:
	{
		bt_manager_avrcp_play_next();
		break;
	}

	case MSG_BT_PLAY_PREVIOUS:
	{
		bt_manager_avrcp_play_previous();
		break;
	}

	case MSG_BT_PLAY_VOLUP:
	{
		system_volume_up(AUDIO_STREAM_MUSIC, 1);
		break;
	}

	case MSG_BT_PLAY_VOLDOWN:
	{
		system_volume_down(AUDIO_STREAM_MUSIC, 1);
		break;
	}
	case MSG_BT_PLAY_SEEKFW_START:
	{
		if (btmusic->playing) {
			bt_manager_avrcp_fast_forward(true);
		}
		break;
	}
	case MSG_BT_PLAY_SEEKFW_STOP:
	{
		if (btmusic->playing) {
			bt_manager_avrcp_fast_forward(false);
		}
		break;
	}
	case MSG_BT_PLAY_SEEKBW_START:
	{
		if (btmusic->playing) {
			bt_manager_avrcp_fast_backward(true);
		}
		break;
	}
	case MSG_BT_PLAY_SEEKBW_STOP:
	{
		if (btmusic->playing) {
			bt_manager_avrcp_fast_backward(false);
		}
		break;
	}
	default:
		break;
	}
}

void btmusic_tts_event_proc(struct app_msg *msg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

	if (!btmusic)
		return;

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
	if (btmusic->player) {
		btmusic->need_resume_play = 1;
		bt_music_a2dp_stop_play();
	}
	break;
	case TTS_EVENT_STOP_PLAY:
	if (btmusic->need_resume_play) {
		if (thread_timer_is_running(&btmusic->monitor_timer)) {
			thread_timer_stop(&btmusic->monitor_timer);
		}
		thread_timer_init(&btmusic->monitor_timer, bt_music_tts_delay_resume, NULL);
		thread_timer_start(&btmusic->monitor_timer, 2000, 0);
	}
	break;
	}

}

