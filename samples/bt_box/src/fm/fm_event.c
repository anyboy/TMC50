/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief fm event
 */

#include "fm.h"
#include "tts_manager.h"

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

static void _fm_tts_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct fm_app_t *fm = fm_get_app();
	fm->need_resume_play = 0;
	fm_start_play();
}

static void fm_delay_resume_play(int delay)
{
	struct fm_app_t *fm = fm_get_app();
	if (!fm)
		return;

	if (thread_timer_is_running(&fm->tts_resume_timer)) {
		thread_timer_stop(&fm->tts_resume_timer);
	}
	thread_timer_init(&fm->tts_resume_timer, _fm_tts_delay_resume, NULL);
	thread_timer_start(&fm->tts_resume_timer, delay, 0);
}

void fm_tts_event_proc(struct app_msg *msg)
{
	struct fm_app_t *fm = fm_get_app();
	if (!fm)
		return;

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (fm->player) {
			fm->need_resume_play = 1;
			fm_stop_play();
			if (thread_timer_is_running(&fm->tts_resume_timer)) {
				thread_timer_stop(&fm->tts_resume_timer);
			}
		}
		break;
	case TTS_EVENT_STOP_PLAY:
		if (fm->need_resume_play) {
			fm_delay_resume_play(2000);
		}
		break;
	default:
		break;
	}
}


void fm_input_event_proc(struct app_msg *msg)
{
	struct fm_app_t *fm = fm_get_app();

	assert(fm);

	if (fm->scan_mode != NONE_SCAN) {
		fm->scan_mode = NONE_SCAN;
		fm_radio_station_cancel_scan(fm);
		return;
	}

	switch (msg->cmd) {
	case MSG_FM_PLAY_PAUSE_RESUME:
		if (fm->mute_flag) {
			fm->mute_flag = false;
			sys_event_notify(SYS_EVENT_PLAY_START);
			fm_start_play();
			fm_radio_station_set_mute(fm);
		} else {
			fm->mute_flag = true;
			fm_radio_station_set_mute(fm);
			fm_stop_play();
			sys_event_notify(SYS_EVENT_PLAY_PAUSE);
		}
	#ifdef CONFIG_ESD_MANAGER
		{
			u8_t playing = fm->playing;

			esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
		}
	#endif
		break;
	case MSG_FM_PLAY_VOLUP:
		system_volume_up(AUDIO_STREAM_FM, 1);
		break;
	case MSG_FM_PLAY_VOLDOWN:
		system_volume_down(AUDIO_STREAM_FM, 1);
		break;
	case MSG_FM_PLAY_NEXT:
		fm_radio_station_get_next(fm);
		fm_start_play();
		break;
	case MSG_FM_PLAY_PREV:
		fm_radio_station_get_prev(fm);
		fm_start_play();
		break;
	case MSG_FM_MANUAL_SEARCH_NEXT:
		fm_radio_station_manual_search(fm, NEXT_STATION);
		fm_start_play();
		break;
	case MSG_FM_MANUAL_SEARCH_PREV:
		fm_radio_station_manual_search(fm, PREV_STATION);
		fm_start_play();
		break;
	case MSG_FM_AUTO_SEARCH:
		fm_radio_station_auto_scan(fm);
		fm_start_play();
		break;

	case MSG_FM_MEDIA_START:
		fm_delay_resume_play(500);
		break;

	default:
		break;
	}
}

void fm_bt_event_proc(struct app_msg *msg)
{

	if (msg->cmd == BT_REQ_RESTART_PLAY) {
		fm_stop_play();
		os_sleep(200);
		fm_start_play();
	}
	if (msg->cmd == BT_TWS_DISCONNECTION_EVENT) {
		struct fm_app_t *fm = fm_get_app();
		if (fm->player) {
			media_player_set_output_mode(fm->player, MEDIA_OUTPUT_MODE_DEFAULT);
		}
	}
#ifndef CONFIG_TWS_BACKGROUND_BT
	if (msg->cmd == BT_TWS_CONNECTION_EVENT) {
		bt_manager_halt_phone();
	} else if (msg->cmd == BT_TWS_DISCONNECTION_EVENT) {
		bt_manager_resume_phone();
	}
#endif
}


