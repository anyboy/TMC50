/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm app event
 */

#include "alarm.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#include "tts_manager.h"

#define ALARM_PLAY_TIMEOUT (5*60*1000)/* 5min */
#define ALARM_SNOOSE_TIMS	(3)
#define ALARM_SNOOSE_TIMEOUT	(10)/* 10min */
u8_t alarm_snooze_times = 0;

static void _alarm_ring_force_play_next(struct alarm_app_t *alarm, bool force_switch)
{
	struct app_msg msg = {
		.type = MSG_ALARM_EVENT,
		.cmd = MSG_ALARM_RING_PLAY_CUR,
	};

	SYS_LOG_INF("\n");
	if (force_switch)
		msg.cmd = MSG_ALARM_RING_PLAY_NEXT;

	os_sleep(20);
	send_async_msg(app_manager_get_current_app(), &msg);
}

static void _alarm_ring_play_event_callback(u32_t event, void *data, u32_t len, void *user_data)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;

	switch (event) {
	case PLAYBACK_EVENT_STOP_ERROR:
		if (alarm->mplayer_state != MPLAYER_STATE_NORMAL)
			alarm->mplayer_state = MPLAYER_STATE_ERROR;
		alarm->mode = ALARM_STATUS_RING_ERROR;
		_alarm_ring_force_play_next(alarm, true);
		break;
	case PLAYBACK_EVENT_STOP_COMPLETE:
		alarm->mplayer_state = MPLAYER_STATE_NORMAL;
		_alarm_ring_force_play_next(alarm, false);
		break;
	default:

		break;
	}
}
static void _alarm_clear_breakpoint(struct alarm_app_t *alarm)
{
	alarm->bp_info.file_offset = 0;
	alarm->bp_info.time_offset = 0;
}

void alarm_ring_stop(struct alarm_app_t *alarm, bool need_updata_bp)
{
	if (!alarm || !alarm->lcplayer)
		return;
	if (need_updata_bp)
		mplayer_update_breakpoint(alarm->lcplayer, &alarm->bp_info);
	else
		_alarm_clear_breakpoint(alarm);
	mplayer_stop_play(alarm->lcplayer);
	alarm->lcplayer = NULL;
	alarm->mode = ALARM_STATUS_RING_STOP;
}

static void _alarm_ring_start_play(struct alarm_app_t *alarm, const char *url)
{
	struct lcplay_param play_param;

	memset(&play_param, 0, sizeof(struct lcplay_param));
	play_param.url = url;
	play_param.play_event_callback = _alarm_ring_play_event_callback;
	play_param.bp.time_offset = alarm->bp_info.time_offset;
	play_param.bp.file_offset = alarm->bp_info.file_offset;

	alarm->lcplayer = mplayer_start_play(&play_param);
	if (!alarm->lcplayer) {
		alarm->mode = ALARM_STATUS_RING_ERROR;
		if (alarm->mplayer_state != MPLAYER_STATE_NORMAL)
			alarm->mplayer_state = MPLAYER_STATE_ERROR;
		_alarm_ring_force_play_next(alarm, true);
	} else {
		alarm->mode = ALARM_STATUS_RING_PLAYING;
	}
}
static void _alarm_switch_mode(struct alarm_app_t *alarm)
{
	if (thread_timer_is_running(&alarm->monitor_timer)) {
		thread_timer_stop(&alarm->monitor_timer);
		alarm->reflash_counter = 0;
	}
	alarm->set_ok = 0;
	if (!alarm->mode || alarm->mode >= ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm->mode = ALARM_STATUS_CLOCK_DISPLAY;
		alarm_clock_display(alarm, true);
	} else if (alarm->mode == ALARM_STATUS_CLOCK_DISPLAY) {
		alarm->mode = ALARM_STATUS_ALARM_SET_ONOFF;
		bool is_on = false;

		alarm_manager_get_alarm(&alarm->tm, &is_on);
		alarm->alarm_is_on = is_on;
		alarm_onoff_display(alarm, true);
		SYS_LOG_INF(" alarm set mode\n");
	} else if (alarm->mode < ALARM_STATUS_CLOCK_SET_HOUR) {
		alarm->mode = ALARM_STATUS_CLOCK_SET_HOUR;
		alarm_clock_display(alarm, true);
		SYS_LOG_INF(" clock set mode\n");
	} else if (alarm->mode < ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm->mode = ALARM_STATUS_CALENDAR_SET_YEAR;
		alarm_calendar_year_display(alarm, true);
		SYS_LOG_INF(" calendar set mode\n");
	} else {
		;
	}
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
}
static void _alarm_clock_value_adjust(struct alarm_app_t *alarm, bool add)
{
	if (thread_timer_is_running(&alarm->monitor_timer)) {
		thread_timer_stop(&alarm->monitor_timer);
		alarm->reflash_counter = 0;
	}

	if (alarm->set_ok) {
		thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
		return;
	}

	if (alarm->mode == ALARM_STATUS_ALARM_SET_ONOFF) {
		alarm_onoff_adjust(alarm);
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_HOUR) {
		alarm_time_hour_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_MIN) {
		alarm_time_min_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_HOUR) {
		alarm_time_hour_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_MIN) {
		alarm_time_min_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm_calendar_year_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_MONTH) {
		alarm_calendar_month_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_DAY) {
		alarm_calendar_day_adjust(alarm, add);
	} else {
		;
	}
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
}

static void _alarm_confirm_setting(struct alarm_app_t *alarm)
{
	if (thread_timer_is_running(&alarm->monitor_timer)) {
		thread_timer_stop(&alarm->monitor_timer);
		alarm->reflash_counter = 0;
	}

	if (alarm->mode == ALARM_STATUS_ALARM_SET_ONOFF) {
		if (alarm->alarm_is_on) {
			alarm->mode = ALARM_STATUS_ALARM_SET_HOUR;
			alarm_clock_display(alarm, false);
		} else {
			alarm_delete_alarm(alarm);
			alarm_clock_display(alarm, true);
			alarm->set_ok = 1;
		}
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_HOUR) {
		alarm->mode = ALARM_STATUS_ALARM_SET_MIN;
		alarm_clock_display(alarm, false);
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_MIN) {
		alarm_set_alarm(alarm);
		alarm->set_ok = 1;
		alarm_clock_display(alarm, true);
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_HOUR) {
		alarm->mode = ALARM_STATUS_CLOCK_SET_MIN;
		alarm_clock_display(alarm, false);
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_MIN) {
		alarm_set_clock(alarm);
		alarm->set_ok = 1;
		alarm_clock_display(alarm, true);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm->mode = ALARM_STATUS_CALENDAR_SET_MONTH;
		alarm_calendar_mon_day_display(alarm, true);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_MONTH) {
		alarm->mode = ALARM_STATUS_CALENDAR_SET_DAY;
		alarm_calendar_mon_day_display(alarm, true);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_DAY) {
		alarm_set_calendar(alarm);
		alarm->set_ok = 1;
		alarm_clock_display(alarm, true);
	} else {
		;
	}
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
}

static int _alarm_switch_disk(struct alarm_app_t *alarm)
{
	struct app_msg msg = {
		.type = MSG_ALARM_EVENT,
		.cmd = MSG_ALARM_RING_PLAY_INIT,
	};

	if (memcmp(alarm->dir, "USB:ALARM", strlen("USB:ALARM")) == 0) {
		if (fs_manager_get_volume_state("SD:")) {
			strncpy(alarm->dir, "SD:ALARM", sizeof(alarm->dir));
		} else {
		#ifdef CONFIG_DISK_MUSIC_APP
			strncpy(alarm->dir, "NOR:", sizeof(alarm->dir));
		#else
			app_switch_unlock(1);
			msg.type = MSG_START_APP;
			msg.ptr = NULL;
			msg.reserve = APP_SWITCH_LAST;
			send_async_msg(APP_ID_MAIN, &msg);
			return -1;
		#endif
		}
	} else if (memcmp(alarm->dir, "SD:ALARM", strlen("SD:ALARM")) == 0) {
	#ifdef CONFIG_DISK_MUSIC_APP
		strncpy(alarm->dir, "NOR:", sizeof(alarm->dir));
	#else
		app_switch_unlock(1);
		msg.type = MSG_START_APP;
		msg.ptr = NULL;
		msg.reserve = APP_SWITCH_LAST;
		send_async_msg(APP_ID_MAIN, &msg);
		return -1;
	#endif
	} else {
		app_switch_unlock(1);
		msg.type = MSG_START_APP;
		msg.ptr = NULL;
		msg.reserve = APP_SWITCH_LAST;
		send_async_msg(APP_ID_MAIN, &msg);
		return -1;
	}
	send_async_msg(APP_ID_ALARM, &msg);
	return 0;
}
void alarm_tts_event_proc(struct app_msg *msg)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;
	SYS_LOG_INF("msg->value=%d\n", msg->value);
	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (alarm->mode == ALARM_STATUS_RING_PLAYING)
			alarm->need_resume_play = 1;
		alarm_ring_stop(alarm, true);
		break;
	case TTS_EVENT_STOP_PLAY:
		if (alarm->need_resume_play) {
			alarm->need_resume_play = 0;
			_alarm_ring_start_play(alarm, alarm->url);
		} else {
			alarm->need_resume_play = 0;
		}
		break;
	default:
		break;
	}
}

int alarm_input_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return -1;

	switch (msg->cmd) {
	case MSG_ALARM_CHANGE_MODE:
		_alarm_switch_mode(alarm);
		break;
	case MSG_ALARM_VALUE_ADD:
		_alarm_clock_value_adjust(alarm, true);
		break;
	case MSG_ALARM_VALUE_SUB:
		_alarm_clock_value_adjust(alarm, false);
		break;
	case MSG_ALARM_SETTING_CONFIRM:
		_alarm_confirm_setting(alarm);
		break;

	case MSG_ALARM_RING_STOP:
		alarm_snooze_times = 0;
		alarm_ring_stop(alarm, false);
		app_switch_unlock(1);
		new_msg.type = MSG_START_APP;
		new_msg.ptr = NULL;
		new_msg.reserve = APP_SWITCH_LAST;
		send_async_msg(APP_ID_MAIN, &new_msg);
		break;
	case MSG_ALARM_RING_SNOOZE:
		alarm_ring_stop(alarm, false);
		if (alarm_snooze_times < ALARM_SNOOSE_TIMS) {
			alarm_snooze_times++;
			alarm_set_snooze(alarm, ALARM_SNOOSE_TIMEOUT);
		}
		app_switch_unlock(1);
		new_msg.type = MSG_START_APP;
		new_msg.ptr = NULL;
		new_msg.reserve = APP_SWITCH_LAST;
		send_async_msg(APP_ID_MAIN, &new_msg);
		break;
	default:
		return -1;
	}
	return 0;
}

void alarm_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;

	switch (msg->cmd) {
	case MSG_ALARM_RING_PLAY_INIT:
		if (thread_timer_is_running(&alarm->monitor_timer)) {
			thread_timer_stop(&alarm->monitor_timer);
			alarm->reflash_counter = 0;
		}

		if (thread_timer_is_running(&alarm->play_timer)) {
			thread_timer_stop(&alarm->play_timer);
		}

		if (alarm_init_iterator(alarm)) {
		/*need switch disk or exit app*/
			SYS_LOG_ERR("init iterator failed\n");
			if (_alarm_switch_disk(alarm)) {
				alarm_snooze_times = 0;
				SYS_LOG_ERR("no disk\n");
			}
			break;
		}

		if (alarm_ring_play_next_url(alarm)) {
			alarm_ringing_display(alarm);
			_alarm_ring_start_play(alarm, alarm->url);
			thread_timer_start(&alarm->play_timer, ALARM_PLAY_TIMEOUT, 0);
		} else {
		/*need switch disk or exit app*/
			if (_alarm_switch_disk(alarm)) {
				alarm_snooze_times = 0;
				SYS_LOG_ERR("no disk\n");
			}
		}
		break;
	case MSG_ALARM_RING_PLAY_CUR:
		alarm_ring_stop(alarm, false);
		_alarm_ring_start_play(alarm, alarm->url);
		break;
	case MSG_ALARM_RING_PLAY_NEXT:
		alarm_ring_stop(alarm, false);
		if (alarm_ring_play_next_url(alarm)) {
			_alarm_ring_start_play(alarm, alarm->url);
		} else {
		/*alarm music play error,exit app*/
			app_switch_unlock(1);
			new_msg.type = MSG_START_APP;
			new_msg.ptr = NULL;
			new_msg.reserve = APP_SWITCH_LAST;
			send_async_msg(APP_ID_MAIN, &new_msg);
			SYS_LOG_WRN("play error,exit app\n");
		}
		break;
	default:
		break;
	}
}

