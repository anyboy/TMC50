/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call status
 */
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <dvfs.h>
#include "btcall.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "audio_system.h"
#include "audio_policy.h"

static void _bt_call_hfp_disconnected(void)
{
	SYS_LOG_INF(" enter\n");
}

static void _bt_call_sco_connected(io_stream_t upload_stream)
{
	struct btcall_app_t *btcall = btcall_get_app();

	btcall->mic_mute = false;
	btcall->upload_stream = upload_stream;
	btcall->upload_stream_outer = upload_stream ? 1 : 0;
#ifdef CONFIG_BT_CALL_FORCE_PLAY_PHONE_NUM
	if (btcall->phonenum_played) {
		bt_call_start_play();
		btcall->playing = 1;
	}
#else
	bt_call_start_play();
	btcall->playing = 1;
#endif
}

static void _bt_call_sco_disconnected(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	bt_call_stop_play();
	btcall->playing = 0;
}

static void _bt_call_hfp_active_dev_changed(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	if (btcall->playing) {
		bt_call_stop_play();
		btcall->playing = 0;
	}
}

void btcall_bt_event_proc(struct app_msg *msg)
{
	struct btcall_app_t *btcall = btcall_get_app();

	switch (msg->cmd) {
	case BT_HFP_CALL_RING_STATR_EVENT:
		if (!btcall->phonenum_played) {
			btcall_ring_start(msg->ptr, strlen(msg->ptr));
			btcall->phonenum_played = 1;
		}
		break;
	case BT_HFP_CALL_RING_STOP_EVENT:
		btcall_ring_stop();
	#ifdef CONFIG_BT_CALL_FORCE_PLAY_PHONE_NUM
		if (btcall->sco_established) {
			bt_call_start_play();
			btcall->playing = 1;
		}
	#endif
		break;

	case BT_HFP_ACTIVEDEV_CHANGE_EVENT:
	{
		_bt_call_hfp_active_dev_changed();
		break;
	}

	case BT_HFP_CALL_OUTGOING:
	{
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_string(SLED_NUMBER1, "C", true);
		seg_led_display_string(SLED_NUMBER3, "OU", true);
	#endif
		btcall->phonenum_played = 1;
		break;
	}
	case BT_HFP_CALL_INCOMING:
	{
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_string(SLED_NUMBER1, "C", true);
		seg_led_display_string(SLED_NUMBER3, "IN", true);
	#endif
		break;
	}
	case BT_HFP_CALL_ONGOING:
	{
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_string(SLED_NUMBER1, "C", true);
		if	(btcall->playing) {
			seg_led_display_string(SLED_NUMBER3, "HF", true);
		} else {
			seg_led_display_string(SLED_NUMBER3, "AG", true);
		}
		btcall->hfp_ongoing = 1;
		btcall->phonenum_played = 1;
	#endif
	#ifdef CONFIG_LED_MANAGER
		led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
		led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
	#endif
		btcall_ring_stop();
		break;
	}
	case BT_HFP_CALL_SIRI_MODE:
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_manager_clear_screen(LED_CLEAR_ALL);
		seg_led_display_string(SLED_NUMBER1, "SIRI", true);
		btcall->siri_mode = 1;
	#endif
		btcall->phonenum_played = 1;
		SYS_LOG_INF("siri_mode %d\n", btcall->siri_mode);
		break;
	case BT_HFP_CALL_HUNGUP:
	{
		break;
	}

	case BT_HFP_DISCONNECTION_EVENT:
	{
	    btcall->need_resume_play = 0;
		_bt_call_hfp_disconnected();
		break;
	}
	case BT_HFP_ESCO_ESTABLISHED_EVENT:
	{
		io_stream_t upload_stream = NULL;

		if (msg->ptr)
			memcpy(&upload_stream, msg->ptr, sizeof(upload_stream));
	#ifdef CONFIG_SEG_LED_MANAGER
		SYS_LOG_INF("..siri_mode %d\n", btcall->siri_mode);
		if (!btcall->siri_mode && btcall->hfp_ongoing) {
			seg_led_display_string(SLED_NUMBER1, "C", true);
			seg_led_display_string(SLED_NUMBER3, "HF", true);
		}
	#endif
		btcall->sco_established = 1;
		_bt_call_sco_connected(upload_stream);
		break;
	}
	case BT_HFP_ESCO_RELEASED_EVENT:
	{
	    _bt_call_sco_disconnected();
	#ifdef CONFIG_SEG_LED_MANAGER
		if (!btcall->siri_mode && btcall->hfp_ongoing) {
			seg_led_display_string(SLED_NUMBER1, "C", true);
			seg_led_display_string(SLED_NUMBER3, "AG", true);
		}
	#endif
		btcall->sco_established = 0;
		break;
	}

	case BT_RMT_VOL_SYNC_EVENT:
	{
		audio_system_set_stream_volume(AUDIO_STREAM_VOICE, msg->value);

		if (btcall && btcall->player) {
			media_player_set_volume(btcall->player, msg->value, msg->value);
		}
		break;
	}

	case BT_REQ_RESTART_PLAY:
	{
		if(btcall->playing){
			SYS_LOG_INF("BT_REQ_RESTART_PLAY\n");
			bt_call_stop_play();
			os_sleep(200);
			bt_call_start_play();
		}
		break;
	}
	
	default:
		break;
	}
}


static void _btcall_key_func_switch_mic_mute(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	btcall->mic_mute ^= 1;

	audio_system_mute_microphone(btcall->mic_mute);

	if (btcall->mic_mute)
		sys_event_notify(SYS_EVENT_MIC_MUTE_ON);
	else
		sys_event_notify(SYS_EVENT_MIC_MUTE_OFF);
}

static void _btcall_key_func_volume_adjust(int updown)
{
	int volume = 0;
	struct btcall_app_t *btcall = btcall_get_app();

	if (updown) {
		volume = system_volume_up(AUDIO_STREAM_VOICE, 1);
	} else {
		volume = system_volume_down(AUDIO_STREAM_VOICE, 1);
	}

	if (btcall->player) {
		media_player_set_volume(btcall->player, volume, volume);
	}
}

void btcall_input_event_proc(struct app_msg *msg)
{
	switch (msg->cmd) {
	case MSG_BT_CALL_VOLUP:
	{
		_btcall_key_func_volume_adjust(1);
		break;
	}
	case MSG_BT_CALL_VOLDOWN:
	{
		_btcall_key_func_volume_adjust(0);
		break;
	}
	case MSG_BT_ACCEPT_CALL:
	{
	    bt_manager_hfp_accept_call();
		break;
	}
	case MSG_BT_REJECT_CALL:
	{
	    bt_manager_hfp_reject_call();
		break;
	}
	case MSG_BT_HANGUP_CALL:
	{
	    bt_manager_hfp_hangup_call();
		break;
	}
	case MSG_BT_HANGUP_ANOTHER:
	{
		bt_manager_hfp_hangup_another_call();
	break;
	}
	case MSG_BT_HOLD_CURR_ANSWER_ANOTHER:
	{
		bt_manager_hfp_holdcur_answer_call();
		break;
	}
	case MSG_BT_HANGUP_CURR_ANSER_ANOTHER:
	{
		bt_manager_hfp_hangupcur_answer_call();
		break;
	}
	case MSG_BT_CALL_SWITCH_CALLOUT:
	{
	#ifdef CONFIG_SEG_LED_MANAGER
		struct btcall_app_t *btcall = btcall_get_app();
		seg_led_display_string(SLED_NUMBER1, "C", true);
		if	(btcall->playing) {
			seg_led_display_string(SLED_NUMBER3, "AG", true);
		} else {
			seg_led_display_string(SLED_NUMBER3, "HF", true);
		}
	#endif
		bt_manager_hfp_switch_sound_source();
		break;
	}
	case MSG_BT_CALL_SWITCH_MICMUTE:
	{
	    _btcall_key_func_switch_mic_mute();
		break;
	}
	case MSG_BT_CALL_LAST_NO:
	{
		bt_manager_hfp_dial_number(NULL);
		break;
	}
	case MSG_BT_SIRI_STOP:
	{
		bt_manager_hfp_stop_siri();
		break;
	}
	case MSG_BT_HID_START:
	{
		break;
	}
	}
}

void btcall_tts_event_proc(struct app_msg *msg)
{
	struct btcall_app_t *btcall = btcall_get_app();

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
	if (btcall->player) {
		btcall->need_resume_play = 1;
		bt_call_stop_play();
	}
	break;
	case TTS_EVENT_STOP_PLAY:
	if (btcall->need_resume_play) {
		btcall->need_resume_play = 0;
		bt_call_start_play();
	}
	break;
	}
}

