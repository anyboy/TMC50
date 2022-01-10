/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input event
 */

#include "audio_input.h"
#include "tts_manager.h"

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

void audio_input_tts_event_proc(struct app_msg *msg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	if (!audio_input)
		return;

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (audio_input->player) {
			audio_input->need_resume_play = 1;
			audio_input_stop_play();
		}
		break;
	case TTS_EVENT_STOP_PLAY:
		if (audio_input->need_resume_play && !audio_input->playing) {
			if (thread_timer_is_running(&audio_input->monitor_timer)) {
				thread_timer_stop(&audio_input->monitor_timer);
			}
			thread_timer_init(&audio_input->monitor_timer, audio_input_delay_resume, NULL);
			thread_timer_start(&audio_input->monitor_timer, 2000, 0);
		}
		break;
	default:
		break;
	}
}
u32_t audio_input_get_audio_stream_type(char *app_name)
{
	if (memcmp(app_name, APP_ID_LINEIN, strlen(APP_ID_LINEIN)) == 0)
		return AUDIO_STREAM_LINEIN;
	else if (memcmp(app_name, APP_ID_SPDIF_IN, strlen(APP_ID_SPDIF_IN)) == 0)
		return AUDIO_STREAM_SPDIF_IN;
	else if (memcmp(app_name, APP_ID_MIC_IN, strlen(APP_ID_MIC_IN)) == 0)
		return AUDIO_STREAM_MIC_IN;
	else if (memcmp(app_name, APP_ID_I2SRX_IN, strlen(APP_ID_I2SRX_IN)) == 0)
		return AUDIO_STREAM_I2SRX_IN;

	SYS_LOG_WRN("%s unsupport\n", app_name);
	return 0;
}

void audio_input_input_event_proc(struct app_msg *msg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	if (!audio_input)
		return;
	switch (msg->cmd) {
	case MSG_AUDIO_INPUT_PLAY_PAUSE_RESUME:
		if (audio_input->playing) {
			audio_input_stop_play();
			sys_event_notify(SYS_EVENT_PLAY_PAUSE);
		} else {
			sys_event_notify(SYS_EVENT_PLAY_START);
			if (thread_timer_is_running(&audio_input->monitor_timer)) {
				thread_timer_stop(&audio_input->monitor_timer);
			}
			thread_timer_init(&audio_input->monitor_timer, audio_input_delay_resume, NULL);
			thread_timer_start(&audio_input->monitor_timer, 200, 0);
			audio_input->playing = 1;
		}
		audio_input_store_play_state();
	#ifdef CONFIG_ESD_MANAGER
		{
			u8_t playing = audio_input->playing;

			esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
		}
	#endif
		break;
	case MSG_AUDIO_INPUT_PLAY_VOLUP:
	 system_volume_up(audio_input_get_audio_stream_type(app_manager_get_current_app()), 1);
		break;
	case MSG_AUDIO_INPUT_PLAY_VOLDOWN:
		system_volume_down(audio_input_get_audio_stream_type(app_manager_get_current_app()), 1);
		break;
	case MSG_AUDIO_INPUT_SAMPLE_RATE_CHANGE:
		audio_input_stop_play();
		audio_input_start_play();
		break;
	default:
		break;
	}
}

void audio_input_bt_event_proc(struct app_msg *msg)
{
	if (msg->cmd == BT_REQ_RESTART_PLAY) {
		audio_input_stop_play();
		os_sleep(200);
		audio_input_start_play();
	}
	if (msg->cmd == BT_TWS_DISCONNECTION_EVENT) {
		struct audio_input_app_t *audio_input = audio_input_get_app();
		if (audio_input->player) {
			media_player_set_output_mode(audio_input->player, MEDIA_OUTPUT_MODE_DEFAULT);
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



