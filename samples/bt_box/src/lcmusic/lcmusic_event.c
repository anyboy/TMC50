/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lcmusic app event
 */

#include "lcmusic.h"
#include "tts_manager.h"
#include <fs_manager.h>

#define MONITOR_TIME_PERIOD	OS_MSEC(500)/* 500ms */

#ifdef CONFIG_SOUNDBAR_SAMPLE_RATE_FILTER
#define LCMUSIC_SOUNDBAR_MIN_SAMPLE_RATE	(44)/* kHz */
#endif

#define SEEK_SPEED_LEVEL	(10)
static u32_t start_seek_time;

static bool _check_disk_plugout(void)
{
#ifdef CONFIG_SOUNDBAR_LCMUSIC
	if (!fs_manager_get_volume_state("USB:"))
		return true;
#else
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	if (strncmp(app_manager_get_current_app(), APP_ID_UHOST_MPLAYER, strlen(APP_ID_UHOST_MPLAYER)) == 0) {
		if (!fs_manager_get_volume_state("USB:"))
			return true;
	} else if (strncmp(app_manager_get_current_app(), APP_ID_SD_MPLAYER, strlen(APP_ID_SD_MPLAYER)) == 0) {
		if (!fs_manager_get_volume_state("SD:"))
			return true;
	}
#endif
#endif
	return false;

}
static void _lcmusic_force_play_next(struct lcmusic_app_t *lcmusic, bool force_switch)
{
	struct app_msg msg = {
		.type = MSG_LCMUSIC_EVENT,
		.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT,
	};

	SYS_LOG_INF("%d\n", force_switch);

	switch (lcmusic->mplayer_mode) {
	case MPLAYER_REPEAT_ONE_MODE:
		if (!force_switch)
			msg.cmd = MSG_LCMPLAYER_PLAY_CUR;
		break;
	default:
		break;
	}
	if (lcmusic->prev_music_state) {
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_LCMPLAYER_PLAY_PREV;
		lcmusic->filter_prev_tts = 1;
	}
	/*fix app message lost:decoder thread sleep 20ms to reduce app message*/
	os_sleep(20);
	send_async_msg(app_manager_get_current_app(), &msg);
}

static void _lcmusic_clear_breakpoint(struct lcmusic_app_t *lcmusic)
{
	lcmusic->music_bp_info.bp_info.file_offset = 0;
	lcmusic->music_bp_info.bp_info.time_offset = 0;
}

static void _lcmusic_play_event_callback(u32_t event, void *data, u32_t len, void *user_data)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic)
		return;
	SYS_LOG_INF("event %d\n", event);

	switch (event) {
	case PLAYBACK_EVENT_STOP_ERROR:
		if (lcmusic->mplayer_state != MPLAYER_STATE_NORMAL)
			lcmusic->mplayer_state = MPLAYER_STATE_ERROR;
		lcmusic->music_state = LCMUSIC_STATUS_ERROR;
		_lcmusic_force_play_next(lcmusic, true);
		break;
	case PLAYBACK_EVENT_STOP_COMPLETE:
		lcmusic->prev_music_state = 0;
		lcmusic->mplayer_state = MPLAYER_STATE_NORMAL;
		lcmusic->full_cycle_times = 0;
		_lcmusic_force_play_next(lcmusic, false);
		break;
	default:

		break;
	}
}

void lcmusic_bp_info_save(struct lcmusic_app_t *lcmusic)
{
	if (lcmusic->music_state == LCMUSIC_STATUS_NULL)
		return;
	mplayer_update_breakpoint(lcmusic->lcplayer, &lcmusic->music_bp_info.bp_info);
	_lcmusic_bpinfo_save(lcmusic->cur_dir, lcmusic->music_bp_info);
}

void lcmusic_stop_play(struct lcmusic_app_t *lcmusic, bool need_updata_bp)
{
	_lcmusic_clear_breakpoint(lcmusic);

	if (!lcmusic->lcplayer)
		return;

	if (need_updata_bp) {
		mplayer_update_breakpoint(lcmusic->lcplayer, &lcmusic->music_bp_info.bp_info);
		mplayer_update_media_info(lcmusic->lcplayer, &lcmusic->media_info);
	}

	if (thread_timer_is_running(&lcmusic->monitor_timer))
		thread_timer_stop(&lcmusic->monitor_timer);

	if (lcmusic->music_state != LCMUSIC_STATUS_SEEK) {
		if (thread_timer_is_running(&lcmusic->seek_timer))
			thread_timer_stop(&lcmusic->seek_timer);
	}

	mplayer_stop_play(lcmusic->lcplayer);

	lcmusic->lcplayer = NULL;

	if (lcmusic->music_state == LCMUSIC_STATUS_PLAYING)
		lcmusic->music_state = LCMUSIC_STATUS_STOP;
}

static int _lcmusic_esd_restore_bp_info(struct lcmusic_app_t *lcmusic, u8_t *play_state)
{
	int res = -EINVAL;

#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		res = 0;
		esd_manager_restore_scene(TAG_PLAY_STATE, play_state, 1);
		esd_manager_restore_scene(TAG_BP_INFO, (u8_t *)&lcmusic->music_bp_info, sizeof(lcmusic->music_bp_info));

		SYS_LOG_INF("music_bp_info:time_offset=%d,file_offset=%d\n",
			lcmusic->music_bp_info.bp_info.time_offset, lcmusic->music_bp_info.bp_info.file_offset);

		//print_buffer(&lcmusic->music_bp_info, 1, sizeof(lcmusic->music_bp_info), 16, -1);
	}
#endif
	return res;
}

static void _lcmusic_esd_save_bp_info(struct lcmusic_app_t *lcmusic, u8_t is_need_update_cluster)
{
	mplayer_update_breakpoint(lcmusic->lcplayer, &lcmusic->music_bp_info.bp_info);
	if (is_need_update_cluster) {
		if (lcmusic_get_cluster_by_url(lcmusic))
			SYS_LOG_INF("get cluster faild\n");
		//print_buffer(&lcmusic->music_bp_info, 1, sizeof(lcmusic->music_bp_info), 16, -1);
	}
#ifdef CONFIG_ESD_MANAGER
	esd_manager_save_scene(TAG_BP_INFO, (u8_t *)&lcmusic->music_bp_info, sizeof(lcmusic->music_bp_info));
#endif
}

static void _lcmusic_monitor_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();
	struct app_msg msg = { 0 };

	if (!lcmusic)
		return;
	if (lcmusic->is_init) {
		if (strncmp(app_manager_get_current_app(), APP_ID_UHOST_MPLAYER, strlen(APP_ID_UHOST_MPLAYER)) == 0) {
			if (++lcmusic->check_usb_plug_time < 5) {
				if (!fs_manager_get_volume_state("USB:")) {
					thread_timer_start(&lcmusic->monitor_timer, 1000, 0);
					return;
				}
			}
		}

		lcmusic->check_usb_plug_time = 0;
		lcmusic->is_init = 0;
		lcmusic->music_state = LCMUSIC_STATUS_STOP;
		msg.type = MSG_LCMUSIC_EVENT;
		msg.cmd = MSG_LCMPLAYER_INIT;
		send_async_msg(app_manager_get_current_app(), &msg);
		return;
	}
	/*display seek progress*/
	if (lcmusic->music_state == LCMUSIC_STATUS_SEEK) {
		if (lcmusic->seek_direction == SEEK_FORWARD) {
			lcmusic_display_play_time(lcmusic, SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
		} else if (lcmusic->seek_direction == SEEK_BACKWARD) {
			lcmusic_display_play_time(lcmusic, -SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
		}
	}

#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
	if (lcmusic->num_play) {
		/*no display bp track_no dearing num play*/
		lcmusic->filt_track_no = 1;
		if (++lcmusic->num_play_time > 8000 / MONITOR_TIME_PERIOD) {
			lcmusic->num_play_time = 0;
			lcmusic->num_play = 0;
			msg.type = MSG_LCMUSIC_EVENT;
			msg.cmd = MSG_LCMPLAYER_PLAY_TRACK_NO;
			send_async_msg(app_manager_get_current_app(), &msg);
		}
	}
#endif

	if (!lcmusic->lcplayer || lcmusic->music_state != LCMUSIC_STATUS_PLAYING)
		return;

	if (lcmusic->is_play_in5s) {
		if (++lcmusic->play_time > 5000 / MONITOR_TIME_PERIOD) {
			lcmusic->is_play_in5s = 0;
			lcmusic->play_time = 0;
			SYS_LOG_INF("play 5s\n");
		}
	}
	_lcmusic_esd_save_bp_info(lcmusic, 0);
	lcmusic_display_play_time(lcmusic, 0);
}
static void _lcmusic_seek_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct app_msg msg = {
		.type = MSG_LCMUSIC_EVENT,
	};
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic || !lcmusic->seek_direction)
		return;

	/* replay current url from beginning */
	_lcmusic_clear_breakpoint(lcmusic);

	SYS_LOG_INF("%d\n", lcmusic->seek_direction);

	lcmusic->mplayer_state = MPLAYER_STATE_NORMAL;
	lcmusic->prev_music_state = 0;
	if (lcmusic->seek_direction == SEEK_FORWARD) {
		_lcmusic_force_play_next(lcmusic, false);
	} else {
		msg.cmd = MSG_LCMPLAYER_PLAY_CUR;
		send_async_msg(app_manager_get_current_app(), &msg);
	}
	lcmusic->seek_direction = SEEK_NULL;
}

#ifdef CONFIG_SOUNDBAR_LCMUSIC
static void _lcmusic_soundbar_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	/* disk in but no file or music paused, keep checking if disk pluged out */
	if (lcmusic->soundbar_check_disk_out || lcmusic->music_state == LCMUSIC_STATUS_STOP) {
		if (_check_disk_plugout()) {
			lcmusic->is_init = 1;	/* disk out, need reinitialize */
			lcmusic_exit_iterator();
			lcmusic->soundbar_check_disk_out = 0;
			lcmusic->music_state = LCMUSIC_STATUS_ERROR;
		}

		thread_timer_start(&lcmusic->soundbar_disk_timer, 1000, 0);
		return;
	}

	if (_check_disk_plugout() == false) {
		/* disk in, start monitor timer to initialize */
		thread_timer_start(&lcmusic->monitor_timer, 0, 0);
		return;
	}

	/* disk out, keep checking if disk pluged in */
	thread_timer_start(&lcmusic->soundbar_disk_timer, 1000, 0);
}
#endif

void lcmusic_thread_timer_init(struct lcmusic_app_t *lcmusic)
{
	thread_timer_init(&lcmusic->monitor_timer, _lcmusic_monitor_timer, NULL);
	thread_timer_init(&lcmusic->seek_timer, _lcmusic_seek_timer, NULL);
#ifdef CONFIG_SOUNDBAR_LCMUSIC
	thread_timer_init(&lcmusic->soundbar_disk_timer, _lcmusic_soundbar_timer, NULL);
#endif
}

static void _lcmusic_start_play(struct lcmusic_app_t *lcmusic, const char *url, int seek_time)
{
	struct lcplay_param play_param;

	memset(&play_param, 0, sizeof(struct lcplay_param));
	play_param.url = url;
	play_param.seek_time = seek_time;
	play_param.play_event_callback = _lcmusic_play_event_callback;
	play_param.bp.file_offset = lcmusic->music_bp_info.bp_info.file_offset;
	play_param.bp.time_offset = lcmusic->music_bp_info.bp_info.time_offset;
#ifdef CONFIG_SOUNDBAR_SAMPLE_RATE_FILTER
	play_param.min_sample_rate_khz = LCMUSIC_SOUNDBAR_MIN_SAMPLE_RATE;
#endif

	lcmusic->lcplayer = mplayer_start_play(&play_param);
	if (!lcmusic->lcplayer) {
		lcmusic->music_state = LCMUSIC_STATUS_ERROR;
		if (lcmusic->mplayer_state != MPLAYER_STATE_NORMAL)
			lcmusic->mplayer_state = MPLAYER_STATE_ERROR;
		_lcmusic_force_play_next(lcmusic, true);
	} else {
		lcmusic->is_play_in5s = 1;
		lcmusic->play_time = 0;
		lcmusic->music_state = LCMUSIC_STATUS_PLAYING;
		lcmusic_store_play_state(lcmusic->cur_dir);
		lcmusic_display_icon_play();

		if (!lcmusic->filt_track_no)
			lcmusic_display_track_no(lcmusic->music_bp_info.track_no, 3000);
		lcmusic->filt_track_no = 0;

		thread_timer_start(&lcmusic->monitor_timer, 0, MONITOR_TIME_PERIOD);

		_lcmusic_esd_save_bp_info(lcmusic, 1);
	}
}
static void _lcmusic_switch_app_check(struct lcmusic_app_t *lcmusic)
{
#ifndef CONFIG_SOUNDBAR_LCMUSIC
	struct app_msg msg = {0};

	if (lcmusic->mplayer_state == MPLAYER_STATE_ERROR) {
		msg.reserve = 0;
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		if (_check_disk_plugout()) {
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
		lcmusic_display_play_time(lcmusic, 0);
		lcmusic->music_state = LCMUSIC_STATUS_ERROR;
		SYS_LOG_INF("not switch\n");
	}
#else
	lcmusic->music_state = LCMUSIC_STATUS_ERROR;

	if (_check_disk_plugout()) {
		/* disk out */
		lcmusic->is_init = 1;
		lcmusic_exit_iterator();
		lcmusic->soundbar_check_disk_out = 0;
	} else {
		/* disk in but no file */
		lcmusic->soundbar_check_disk_out = 1;
	}

	thread_timer_start(&lcmusic->soundbar_disk_timer, 1000, 0);
#endif
}

#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
static void _lcmusic_deal_play_num(struct lcmusic_app_t *lcmusic, u8_t num)
{
	if (lcmusic->seek_direction)
		return;

	if (lcmusic->track_no > 999)
		lcmusic->track_no = 0;
	lcmusic->track_no = lcmusic->track_no * 10 + num;
	lcmusic_display_track_no(lcmusic->track_no, 8000);
	lcmusic->num_play = 1;
	lcmusic->num_play_time = 0;
	/*no display bp track_no dearing num play*/
	lcmusic->filt_track_no = 1;
}
#endif

static void _lcmusic_cancel_num_input_check(struct lcmusic_app_t *lcmusic)
{
#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
	if (lcmusic->num_play) {
		lcmusic->num_play = 0;
		lcmusic->num_play_time = 0;
		lcmusic->track_no = 0;
		lcmusic->filt_track_no = 0;
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_manager_set_timeout_event(0, NULL);
	#endif
	}
#endif
}


static void _lcmusic_tts_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();
	if ((lcmusic->music_state != LCMUSIC_STATUS_PLAYING)
		&& (lcmusic->music_state != LCMUSIC_STATUS_ERROR)) {
		lcmusic->filt_track_no = 1;
		_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
	}
	lcmusic->need_resume_play = 0;
}

void lcmusic_tts_event_proc(struct app_msg *msg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic)
		return;
	SYS_LOG_INF("msg->value=%d,music_state=%d,need_resume_play=%d\n",
		msg->value, lcmusic->music_state, lcmusic->need_resume_play);

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (lcmusic->music_state == LCMUSIC_STATUS_PLAYING)
			lcmusic->need_resume_play = 1;
		if (lcmusic->lcplayer)
			lcmusic_stop_play(lcmusic, true);
		break;
	case TTS_EVENT_STOP_PLAY:
		if (lcmusic->need_resume_play) {
			if (thread_timer_is_running(&lcmusic->tts_resume_timer)) {
				thread_timer_stop(&lcmusic->tts_resume_timer);
			}
			thread_timer_init(&lcmusic->tts_resume_timer, _lcmusic_tts_delay_resume, NULL);
			thread_timer_start(&lcmusic->tts_resume_timer, 2000, 0);
		}
		break;
	}
}

void lcmusic_input_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	SYS_LOG_INF("msg cmd =%d\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_LCMPLAYER_PLAY_NEXT:
		_lcmusic_cancel_num_input_check(lcmusic);

		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;

		lcmusic->prev_music_state = 0;
		lcmusic->filt_track_no = 0;
		lcmusic_stop_play(lcmusic, false);
		sys_event_notify(SYS_EVENT_PLAY_NEXT);

		if (lcmusic_play_next_url(lcmusic, true)) {
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
	case MSG_LCMPLAYER_PLAY_PREV:
		_lcmusic_cancel_num_input_check(lcmusic);

		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;

		lcmusic->prev_music_state = 1;
		if (lcmusic->is_play_in5s) {
			lcmusic->is_play_in5s = 0;
			lcmusic->filt_track_no = 0;
			lcmusic_stop_play(lcmusic, false);
			sys_event_notify(SYS_EVENT_PLAY_PREVIOUS);
			if (!lcmusic_play_prev_url(lcmusic)) {
				SYS_LOG_INF("prev url is null\n");
				if (lcmusic->music_state == LCMUSIC_STATUS_ERROR) {
					new_msg.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT;
				} else {
					new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
				}
			} else {
				new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			}
			new_msg.type = MSG_LCMUSIC_EVENT;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			if ((lcmusic->music_state == LCMUSIC_STATUS_PLAYING) || (lcmusic->music_state == LCMUSIC_STATUS_STOP)) {
				lcmusic_stop_play(lcmusic, false);
				_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
			} else {
				lcmusic_stop_play(lcmusic, false);
				if (!lcmusic->filter_prev_tts) {
					sys_event_notify(SYS_EVENT_PLAY_PREVIOUS);
				}
				lcmusic->filter_prev_tts = 0;
				if (lcmusic_play_prev_url(lcmusic)) {
					_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
				} else {
					SYS_LOG_INF("prev url is null\n");
					if (lcmusic->music_state == LCMUSIC_STATUS_ERROR) {
						new_msg.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT;
					} else {
						new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
					}
					new_msg.type = MSG_LCMUSIC_EVENT;
					send_async_msg(app_manager_get_current_app(), &new_msg);
				}
			}
		}
		break;
	case MSG_LCMPLAYER_PLAY_VOLUP:
		_lcmusic_cancel_num_input_check(lcmusic);

		system_volume_up(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;
	case MSG_LCMPLAYER_PLAY_VOLDOWN:
		_lcmusic_cancel_num_input_check(lcmusic);

		system_volume_down(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;
	case MSG_LCMPLAYER_PLAY_PAUSE:
	#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
		if (lcmusic->num_play) {
			lcmusic->num_play = 0;
			lcmusic->num_play_time = 0;
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_TRACK_NO;
			send_async_msg(app_manager_get_current_app(), &new_msg);
			break;
		}
	#endif
		if (lcmusic->music_state == LCMUSIC_STATUS_PLAYING) {
			lcmusic->filt_track_no = 1;
			lcmusic_stop_play(lcmusic, true);
			lcmusic_store_play_state(lcmusic->cur_dir);
			sys_event_notify(SYS_EVENT_PLAY_PAUSE);
			lcmusic_bp_info_save(lcmusic);
		#ifdef CONFIG_SOUNDBAR_LCMUSIC
			/* to avoid disk pluged out when paused */
			thread_timer_start(&lcmusic->soundbar_disk_timer, 1000, 0);
		#endif
		}
		break;
	case MSG_LCMPLAYER_PLAY_PLAYING:
	#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
		if (lcmusic->num_play) {
			lcmusic->num_play = 0;
			lcmusic->num_play_time = 0;
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_TRACK_NO;
			send_async_msg(app_manager_get_current_app(), &new_msg);
			break;
		}
	#endif
		sys_event_notify(SYS_EVENT_PLAY_START);

		new_msg.type = MSG_LCMUSIC_EVENT;
		new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
		send_async_msg(app_manager_get_current_app(), &new_msg);
		break;
	case MSG_LCMPLAYER_PLAY_SEEK_FORWARD:
		if (thread_timer_is_running(&lcmusic->seek_timer))
			thread_timer_stop(&lcmusic->seek_timer);

		lcmusic->seek_direction = SEEK_NULL;
		if (lcmusic->music_state == LCMUSIC_STATUS_SEEK) {
			if (thread_timer_is_running(&lcmusic->monitor_timer))
				thread_timer_stop(&lcmusic->monitor_timer);

			lcmusic->filt_track_no = 1;
			_lcmusic_start_play(lcmusic, lcmusic->cur_url, SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
		}
		break;
	case MSG_LCMPLAYER_PLAY_SEEK_BACKWARD:
		if (thread_timer_is_running(&lcmusic->seek_timer))
			thread_timer_stop(&lcmusic->seek_timer);

		lcmusic->seek_direction = SEEK_NULL;
		if (lcmusic->music_state == LCMUSIC_STATUS_SEEK) {
			if (thread_timer_is_running(&lcmusic->monitor_timer))
				thread_timer_stop(&lcmusic->monitor_timer);

			lcmusic->filt_track_no = 1;
			_lcmusic_start_play(lcmusic, lcmusic->cur_url, -(SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time)));
		}
		break;
	case MSG_LCMPLAYER_SEEKFW_START_CTIME:
		_lcmusic_cancel_num_input_check(lcmusic);

		lcmusic->music_state = LCMUSIC_STATUS_SEEK;
		lcmusic->seek_direction = SEEK_FORWARD;
		start_seek_time = os_uptime_get_32();
		lcmusic_stop_play(lcmusic, true);
		thread_timer_start(&lcmusic->monitor_timer, 1000 / SEEK_SPEED_LEVEL, 1000 / SEEK_SPEED_LEVEL);
		if (lcmusic->media_info.total_time > lcmusic->music_bp_info.bp_info.time_offset) {
			thread_timer_start(&lcmusic->seek_timer,
							(lcmusic->media_info.total_time - lcmusic->music_bp_info.bp_info.time_offset) / SEEK_SPEED_LEVEL, 0);
		}
		break;
	case MSG_LCMPLAYER_SEEKBW_START_CTIME:
		_lcmusic_cancel_num_input_check(lcmusic);

		lcmusic->music_state = LCMUSIC_STATUS_SEEK;
		lcmusic->seek_direction = SEEK_BACKWARD;
		start_seek_time = os_uptime_get_32();
		lcmusic_stop_play(lcmusic, true);
		thread_timer_start(&lcmusic->monitor_timer, 1000 / SEEK_SPEED_LEVEL, 1000 / SEEK_SPEED_LEVEL);
		thread_timer_start(&lcmusic->seek_timer,
						lcmusic->music_bp_info.bp_info.time_offset / SEEK_SPEED_LEVEL, 0);
		break;
	case MSG_LCMPLAYER_SET_PLAY_MODE:
		_lcmusic_cancel_num_input_check(lcmusic);

		if (++lcmusic->mplayer_mode >= MPLAYER_NUM_PLAY_MODES)
			lcmusic->mplayer_mode = 0;
		lcmusic_play_set_mode(lcmusic, lcmusic->mplayer_mode);
		SYS_LOG_INF("set mplayer_mode :%d\n", lcmusic->mplayer_mode);
		break;
	case MSG_LCMPLAYER_PLAY_NEXT_DIR:
		_lcmusic_cancel_num_input_check(lcmusic);

		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;
		lcmusic->prev_music_state = 0;
		lcmusic->filt_track_no = 0;
		lcmusic_stop_play(lcmusic, false);

		if (lcmusic_play_next_folder_url(lcmusic)) {
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
	case MSG_LCMPLAYER_PLAY_PREV_DIR:
		_lcmusic_cancel_num_input_check(lcmusic);

		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;

		lcmusic->filt_track_no = 0;
		lcmusic_stop_play(lcmusic, false);
		if (lcmusic_play_prev_folder_url(lcmusic)) {
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
	case MSG_LCMPLAYER_SET_NUM0:
		_lcmusic_deal_play_num(lcmusic, 0);
		break;
	case MSG_LCMPLAYER_SET_NUM1:
		_lcmusic_deal_play_num(lcmusic, 1);
		break;
	case MSG_LCMPLAYER_SET_NUM2:
		_lcmusic_deal_play_num(lcmusic, 2);
		break;
	case MSG_LCMPLAYER_SET_NUM3:
		_lcmusic_deal_play_num(lcmusic, 3);
		break;
	case MSG_LCMPLAYER_SET_NUM4:
		_lcmusic_deal_play_num(lcmusic, 4);
		break;
	case MSG_LCMPLAYER_SET_NUM5:
		_lcmusic_deal_play_num(lcmusic, 5);
		break;
	case MSG_LCMPLAYER_SET_NUM6:
		_lcmusic_deal_play_num(lcmusic, 6);
		break;
	case MSG_LCMPLAYER_SET_NUM7:
		_lcmusic_deal_play_num(lcmusic, 7);
		break;
	case MSG_LCMPLAYER_SET_NUM8:
		_lcmusic_deal_play_num(lcmusic, 8);
		break;
	case MSG_LCMPLAYER_SET_NUM9:
		_lcmusic_deal_play_num(lcmusic, 9);
		break;
#endif
	default:
		break;
	}
}
/*need to clear bp when filed*/
static int _lcmusic_get_bp_url(struct lcmusic_app_t *lcmusic)
{
	return lcmusic_get_url_by_cluster(lcmusic);
}

void lcmusic_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();
	int res = 0;
	u8_t init_play_state = 0;

	SYS_LOG_INF("msg cmd =%d\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_LCMPLAYER_INIT:
		init_play_state = lcmusic_get_play_state(lcmusic->cur_dir);

	#ifdef CONFIG_SOUNDBAR_LCMUSIC
		/* auto start play when disk plug in */
		init_play_state = LCMUSIC_STATUS_PLAYING;
	#endif

		res = _lcmusic_esd_restore_bp_info(lcmusic, &init_play_state);/*need to consider reset when geting bp*/
		if (res)
			_lcmusic_bpinfo_resume(lcmusic->cur_dir, &lcmusic->music_bp_info);

		res = _lcmusic_get_bp_url(lcmusic);

		lcmusic_scan_disk();

		if (init_play_state != LCMUSIC_STATUS_STOP) {
			if (!res) {
				new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			} else {
				new_msg.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT;
			}
			new_msg.type = MSG_LCMUSIC_EVENT;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			lcmusic->music_state = LCMUSIC_STATUS_STOP;
			if (res)
				lcmusic_play_next_url(lcmusic, false);
			lcmusic_display_icon_pause();
			lcmusic_display_play_time(lcmusic, 0);
		}
		break;

	case MSG_LCMPLAYER_AUTO_PLAY_NEXT:
	#ifdef CONFIG_SOUNDBAR_LCMUSIC
		if (_check_disk_plugout()) {
			/* disk out, play error */
			lcmusic_stop_play(lcmusic, true);
			_lcmusic_bpinfo_save(lcmusic->cur_dir, lcmusic->music_bp_info);
			_lcmusic_switch_app_check(lcmusic);
			break;
		}
	#endif

		/* avoid wrong track_no display */
		lcmusic->filt_track_no = 0;

		lcmusic_stop_play(lcmusic, false);
		if (lcmusic_play_next_url(lcmusic, false)) {
			_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
	case MSG_LCMPLAYER_PLAY_URL:
		if (lcmusic->lcplayer) {
			lcmusic_stop_play(lcmusic, false);
		}
		_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
		break;
	case MSG_LCMPLAYER_PLAY_CUR:
		lcmusic_stop_play(lcmusic, false);
		_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
		break;
#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
	case MSG_LCMPLAYER_PLAY_TRACK_NO:
		lcmusic->filt_track_no = 0;
		if (lcmusic->disk_scanning || lcmusic->track_no == 0
			|| lcmusic->track_no > lcmusic->sum_track_no) {
			lcmusic->track_no = 0;
		#ifdef CONFIG_SEG_LED_MANAGER
			seg_led_manager_set_timeout_event(0, NULL);
		#endif
			break;
		}

		lcmusic->music_bp_info.track_no = lcmusic->track_no;
		lcmusic->track_no = 0;
		lcmusic_stop_play(lcmusic, false);
		if (lcmusic_play_set_track_no(lcmusic, lcmusic->music_bp_info.track_no)) {
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
#endif
	default:
		break;
	}
}

void lcmusic_bt_event_proc(struct app_msg *msg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();
	if (msg->cmd == BT_REQ_RESTART_PLAY) {
		if (lcmusic->lcplayer)
			lcmusic_stop_play(lcmusic, true);
		os_sleep(200);
		lcmusic->filt_track_no = 1;
		_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
	}
	if (msg->cmd == BT_TWS_DISCONNECTION_EVENT) {
		if (lcmusic->lcplayer && lcmusic->lcplayer->player) {
			media_player_set_output_mode(lcmusic->lcplayer->player, MEDIA_OUTPUT_MODE_DEFAULT);
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

