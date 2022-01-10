/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief loop play app event
 */

#include "loop_player.h"
#include <fs_manager.h>


#define MONITOR_TIME_PERIOD	OS_MSEC(1000)/* 1s */

static bool _check_disk_plugout(char dir[])
{
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	if (strncmp(dir, "USB:", strlen("USB:")) == 0) {
		if (!fs_manager_get_volume_state("USB:"))
			return true;
	} else if (strncmp(dir, "SD:", strlen("SD:")) == 0){
		if (!fs_manager_get_volume_state("SD:"))
			return true;
	}
#endif
	return false;
}

static void _check_disk_plugin(struct loop_play_app_t * lpplayer)
{
	struct app_msg msg = { 0 };

#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	if (++lpplayer->check_disk_plug_time < 5) {
		if (fs_manager_get_volume_state("USB:")) {
			strncpy(lpplayer->cur_dir, "USB:LOOP", sizeof(lpplayer->cur_dir));
			lpplayer->is_disk_check = 0;
			lpplayer->check_disk_plug_time = 0;
		} else if (fs_manager_get_volume_state("SD:")) {
			strncpy(lpplayer->cur_dir, "SD:LOOP", sizeof(lpplayer->cur_dir));
			lpplayer->is_disk_check = 0;
			lpplayer->check_disk_plug_time = 0;
		#ifdef CONFIG_FS_MANAGER
			fs_manager_sdcard_enter_high_speed();
		#endif
		} else {
		#ifdef CONFIG_DISK_MUSIC_APP
			/* start from NOR, if supported */
			strncpy(lpplayer->cur_dir, "NOR:LOOP", sizeof(lpplayer->cur_dir));
			lpplayer->is_disk_check = 0;
			lpplayer->check_disk_plug_time = 0;
		#else
			/* check again 1s later */
			thread_timer_start(&lpplayer->monitor_timer, 1000, 0);
			return;
		#endif
		}
		msg.type = MSG_LOOPPLAY_EVENT;
		msg.cmd = MSG_LOOPPLAYER_INIT;
		send_async_msg(APP_ID_LOOP_PLAYER, &msg);
	} else {
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		send_async_msg(APP_ID_MAIN, &msg);
	}
#else
	/* default play from Uhost */
	strncpy(lpplayer->cur_dir, "USB:LOOP", sizeof(lpplayer->cur_dir));
	lpplayer->is_disk_check = 0;
#endif
}

static void _loopplay_clear_cur_time(struct loop_play_app_t *lpplayer)
{
	lpplayer->cur_time.file_offset = 0;
	lpplayer->cur_time.time_offset = 0;
}

static void _loopplay_force_play_next(bool force_switch)
{
	struct app_msg msg = {
		.type = MSG_LOOPPLAY_EVENT,
		.cmd = MSG_LOOPPLAYER_AUTO_PLAY_NEXT,
	};

	/*fix app message lost:decoder thread sleep 20ms to reduce app message*/
	os_sleep(20);
	send_async_msg(APP_ID_LOOP_PLAYER, &msg);
}

static void _loopplay_stream_energy_filter_callback(u8_t energy_filt_enable)
{
	struct app_msg new_msg = {0};
	struct loop_play_app_t *lpplayer = loopplay_get_app();

	/* do if to reduce msg */
	if (lpplayer->energy_filt.enable != energy_filt_enable) {
		lpplayer->energy_filt.enable = energy_filt_enable;
		new_msg.type = MSG_LOOPPLAY_EVENT;
		new_msg.cmd = MSG_LOOPPLAYER_SET_ENERGY_FILTER;
		send_async_msg(APP_ID_LOOP_PLAYER, &new_msg);
	}
}

static void _loopplay_play_event_callback(u32_t event, void *data, u32_t len, void *user_data)
{
	struct loop_play_app_t *lpplayer = loopplay_get_app();

	if (!lpplayer)
		return;

	switch (event) {
	case PLAYBACK_EVENT_STOP_ERROR:
		if (lpplayer->mplayer_state != LPPLAYER_STATE_NORMAL)
			lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		lpplayer->music_status = LOOP_STATUS_ERROR;
		_loopplay_force_play_next(true);
		break;

	case PLAYBACK_EVENT_STOP_COMPLETE:
		lpplayer->mplayer_state = LPPLAYER_STATE_NORMAL;
		lpplayer->full_cycle_times = 0;
		_loopplay_force_play_next(false);
		break;

	default:
		break;
	}
}

void loopplay_bp_info_save(struct loop_play_app_t *lpplayer)
{
	if ((lpplayer->music_status == LOOP_STATUS_ERROR) || (lpplayer->music_status == LOOP_STATUS_NULL)) {
		return;
	}

	_loopplayer_bpinfo_save(lpplayer->cur_dir, lpplayer->track_no);
}

void loopplay_stop_play(struct loop_play_app_t *lpplayer, bool need_update_time)
{
	/* clear current played time to play from start(or from breakpoint saved) */
	_loopplay_clear_cur_time(lpplayer);

	if (!lpplayer->localplayer) {
		return;
	}

	if (need_update_time) {
		/* updata current played time to continue playing when paused */
		mplayer_update_breakpoint(lpplayer->localplayer, &lpplayer->cur_time);
	}

	if (thread_timer_is_running(&lpplayer->monitor_timer))
		thread_timer_stop(&lpplayer->monitor_timer);

	mplayer_stop_play(lpplayer->localplayer);

	lpplayer->localplayer = NULL;

	if (lpplayer->music_status == LOOP_STATUS_PLAYING)
		lpplayer->music_status = LOOP_STATUS_STOP;
}

static void _loopplay_monitor_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct loop_play_app_t *lpplayer = loopplay_get_app();

	if (!lpplayer)
		return;

	/* check disk when init */
	if (lpplayer->is_disk_check) {
		_check_disk_plugin(lpplayer);
		return;
	}

	if (!lpplayer->localplayer || lpplayer->music_status != LOOP_STATUS_PLAYING) {
		return;
	}

	mplayer_update_breakpoint(lpplayer->localplayer, &lpplayer->cur_time);
	loopplay_display_play_time(lpplayer->cur_time.time_offset, 0);
}

void loopplay_thread_timer_init(struct loop_play_app_t *lpplayer)
{
	thread_timer_init(&lpplayer->monitor_timer, _loopplay_monitor_timer, NULL);
}

static void _loopplay_switch_app_check(struct loop_play_app_t *lpplayer)
{
	struct app_msg msg = {0};

	if (lpplayer->mplayer_state == LPPLAYER_STATE_ERROR) {
		msg.reserve = 0;
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		if (_check_disk_plugout(lpplayer->cur_dir)) {
			msg.ptr = APP_ID_DEFAULT;
			SYS_LOG_WRN("disk plug out,exit app\n");
		} else {
			msg.ptr = NULL;
			SYS_LOG_WRN("no music,exit app\n");
		}
		/* change to next app */
		send_async_msg(APP_ID_MAIN, &msg);
	} else {
		/*update play time to 00:00*/
		loopplay_display_play_time(lpplayer->cur_time.time_offset, 0);
		lpplayer->music_status = LOOP_STATUS_ERROR;
		SYS_LOG_INF("not switch\n");
	}
}


static void _loopplay_start_play(struct loop_play_app_t *lpplayer, const char *url, int seek_time)
{
	struct lcplay_param play_param;

	memset(&play_param, 0, sizeof(struct lcplay_param));
	play_param.url = url;
	play_param.seek_time = seek_time;
	play_param.play_mode = 1;	/* loop play mode */
	play_param.play_event_callback = _loopplay_play_event_callback;
#if 0
	/* breakpoint info to continue playing from pause */
	play_param.bp.time_offset = lpplayer->cur_time.time_offset;
	play_param.bp.file_offset = lpplayer->cur_time.file_offset;
#endif

	lpplayer->localplayer = mplayer_start_play(&play_param);
	if (!lpplayer->localplayer) {
		lpplayer->music_status = LOOP_STATUS_ERROR;
		if (lpplayer->mplayer_state != LPPLAYER_STATE_NORMAL)
			lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;

		memset(lpplayer->cur_url, 0, sizeof(lpplayer->cur_url));
		_loopplay_force_play_next(true);
	} else {
		lpplayer->music_status = LOOP_STATUS_PLAYING;

		/* show "loop" only when first enter app */
		loopplay_display_icon_play();
		if (lpplayer->show_track_or_loop == 1) {
			loopplay_display_loop_str();
		} else if (lpplayer->show_track_or_loop == 2){
			loopplay_display_track_no(lpplayer->track_no);
		}
		lpplayer->show_track_or_loop = 0;

		/* set loop_fstream file info and energy filter callback */
		loopplay_set_stream_info(lpplayer, NULL, LOOPFS_FILE_INFO);
		/* loop_fstream callback to set energy limiter */
		loopplay_set_stream_info(lpplayer, (void *)_loopplay_stream_energy_filter_callback, LOOPFS_CALLBACK);

		thread_timer_start(&lpplayer->monitor_timer, 0, MONITOR_TIME_PERIOD / 2);
		loopplay_bp_info_save(lpplayer);
	}
}

void loopplay_input_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct loop_play_app_t *lpplayer = loopplay_get_app();
	u8_t temp_mode = 0;

	SYS_LOG_INF("msg cmd =%d\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_LOOPPLAYER_PLAY_NEXT:
		loopplay_stop_play(lpplayer, false);
		lpplayer->show_track_or_loop = 2;
		temp_mode = lpplayer->mplayer_mode;/* save current mplay mode */
		if (lpplayer->mplayer_mode != LPPLAYER_REPEAT_ALL_MODE) {
			lpplayer->mplayer_mode = LPPLAYER_REPEAT_ALL_MODE;
		}

		if (loopplay_next_url(lpplayer, true)) {
			new_msg.type = MSG_LOOPPLAY_EVENT;
			new_msg.cmd = MSG_LOOPPLAYER_PLAY_URL;
			send_async_msg(APP_ID_LOOP_PLAYER, &new_msg);
		} else {
			_loopplay_switch_app_check(lpplayer);
		}
		lpplayer->mplayer_mode = temp_mode;/* resume mplayer mode */
		break;

	case MSG_LOOPPLAYER_PLAY_PREV:
		loopplay_stop_play(lpplayer, false);
		lpplayer->show_track_or_loop = 2;
		if (!loopplay_prev_url(lpplayer)) {
			if (lpplayer->music_status == LOOP_STATUS_ERROR) {
				new_msg.cmd = MSG_LOOPPLAYER_AUTO_PLAY_NEXT;
			} else {
				/* cur_url that playing is valid */
				new_msg.cmd = MSG_LOOPPLAYER_PLAY_URL;
			}
		} else {
			new_msg.cmd = MSG_LOOPPLAYER_PLAY_URL;
		}
		new_msg.type = MSG_LOOPPLAY_EVENT;
		send_async_msg(APP_ID_LOOP_PLAYER, &new_msg);
		break;

	case MSG_LOOPPLAYER_PLAY_VOLUP:
		system_volume_up(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;

	case MSG_LOOPPLAYER_PLAY_VOLDOWN:
		system_volume_down(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;

	case MSG_LOOPPLAYER_PLAY_PAUSE:
		if (lpplayer->music_status == LOOP_STATUS_PLAYING) {
			/* need_update_time true to save breakpoint info */
			loopplay_stop_play(lpplayer, true);
			loopplay_bp_info_save(lpplayer);
			loopplay_display_icon_pause();
		}
		break;

	case MSG_LOOPPLAYER_PLAY_PLAYING:
		new_msg.type = MSG_LOOPPLAY_EVENT;
		new_msg.cmd = MSG_LOOPPLAYER_PLAY_URL;
		send_async_msg(APP_ID_LOOP_PLAYER, &new_msg);
		break;

	case MSG_LOOPPLAYER_SET_PLAY_MODE:
		if (++lpplayer->mplayer_mode >= LPPLAYER_NUM_PLAY_MODES)
			lpplayer->mplayer_mode = 0;
		loopplay_set_mode(lpplayer, lpplayer->mplayer_mode);
		SYS_LOG_INF("set mplayer_mode :%d\n", lpplayer->mplayer_mode);
		break;

	default:
		break;
	}
}

void loopplay_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct loop_play_app_t *lpplayer = loopplay_get_app();
	int res = 0;

	SYS_LOG_INF("msg cmd =%d\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_LOOPPLAYER_INIT:
		lpplayer->show_track_or_loop = 1;
		loopplay_display_icon_disk(lpplayer->cur_dir);

		/* resume track_no only */
		_loopplayer_bpinfo_resume(lpplayer->cur_dir, &lpplayer->track_no);

		/* no LOOP folder */
		if (loopplay_init_iterator(lpplayer)) {
			res = fs_mkdir(lpplayer->cur_dir);
			if (res) {
				/* LOOP folder create fail */
				new_msg.cmd = MSG_SWITCH_APP;
				new_msg.type = MSG_INPUT_EVENT;
				send_async_msg(APP_ID_MAIN, &new_msg);
				break;
			}
			new_msg.type = MSG_LOOPPLAY_EVENT;
			new_msg.cmd = MSG_LOOPPLAYER_INIT;
			send_async_msg(APP_ID_LOOP_PLAYER, &new_msg);
			break;
		}

		/* no mp3 file in LOOP folder */
		if (lpplayer->sum_track_no == 0) {
			loopplay_display_none_str();
			new_msg.cmd = MSG_LOOPPLAYER_PLAY_PAUSE;
			new_msg.type = MSG_INPUT_EVENT;
			send_async_msg(APP_ID_LOOP_PLAYER, &new_msg);
			break;
		}

		/* get the url */
		if (lpplayer->track_no > 0)
			loopplay_set_track_no(lpplayer);

		new_msg.cmd = MSG_LOOPPLAYER_AUTO_PLAY_NEXT;
		new_msg.type = MSG_LOOPPLAY_EVENT;
		send_async_msg(APP_ID_LOOP_PLAYER, &new_msg);
		break;

	case MSG_LOOPPLAYER_AUTO_PLAY_NEXT:
		/* this branch runs in so many cases that is necessary to check url  */
		loopplay_stop_play(lpplayer, false);
		if (lpplayer->cur_url[0] == 0) {
			if (!loopplay_next_url(lpplayer, false))
				_loopplay_switch_app_check(lpplayer);
		}
		_loopplay_start_play(lpplayer, lpplayer->cur_url, 0);
		break;

	case MSG_LOOPPLAYER_PLAY_URL:
		if (lpplayer->localplayer) {
			loopplay_stop_play(lpplayer, false);
		}
		_loopplay_start_play(lpplayer, lpplayer->cur_url, 0);
		break;

	case MSG_LOOPPLAYER_SET_ENERGY_FILTER:
		lpplayer->energy_filt.threshold = ENERGY_LIMITER_THRESHOLD;
		lpplayer->energy_filt.attack_time = ENERGY_LIMITER_ATTACK_TIME;
		lpplayer->energy_filt.release_time = ENERGY_LIMITER_RELEASE_TIME;

		media_player_set_energy_filter(lpplayer->localplayer->player, &lpplayer->energy_filt);
		break;
	
	default:
		break;
	}
}
