/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record app event
 */

#include "record.h"
#include "tts_manager.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#include <hotplug_manager.h>

#define SEEK_SPEED_LEVEL	(10)
#define SYNC_RECORD_DATA_PERIOD (30)
static u32_t start_seek_time;
extern char record_cache_read_temp[0x800];

static bool _check_disk_plugout(struct recorder_app_t *record)
{
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	if (strncmp(record->dir, "USB:RECORD", strlen("USB:RECORD")) == 0) {
		if (!fs_manager_get_volume_state("USB:"))
			return true;
	} else if (strncmp(record->dir, "SD:RECORD", strlen("SD:RECORD")) == 0) {
		if (!fs_manager_get_volume_state("SD:"))
			return true;
	}
#endif
	return false;
}

static void _recplayer_clear_breakpoint(struct recorder_app_t *record)
{
	record->recplayer_bp_info.bp_info.file_offset = 0;
	record->recplayer_bp_info.bp_info.time_offset = 0;
}

static void _recplayer_force_play_next(struct recorder_app_t *record, bool force_switch)
{
	struct app_msg msg = {
		.type = MSG_RECORDER_EVENT,
		.cmd = MSG_RECPLAYER_AUTO_PLAY_NEXT,
	};

	SYS_LOG_INF("%d\n", force_switch);

	switch (record->mplayer_mode) {
	case RECPLAYER_REPEAT_ONE_MODE:
		if (!force_switch)
			msg.cmd = MSG_RECPLAYER_PLAY_CUR;
		break;
	default:
		break;
	}

	if (record->prev_music_state) {
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_RECPLAYER_PLAY_PREV;
		record->filter_prev_tts = 1;
	}
	/*fix app message lost:decoder thread sleep 20ms to reduce app message*/
	os_sleep(20);
	send_async_msg(APP_ID_RECORDER, &msg);
}

static void _recplayer_play_event_callback(u32_t event, void *data, u32_t len, void *user_data)
{
	struct recorder_app_t *record = recorder_get_app();

	if (!record || !record->record_or_play)
		return;

	SYS_LOG_INF("event %d\n", event);

	switch (event) {
	case PLAYBACK_EVENT_STOP_ERROR:
		if (record->mplayer_state != RECPLAYER_STATE_NORMAL)
			record->mplayer_state = RECPLAYER_STATE_ERROR;
		record->music_state = RECFILE_STATUS_ERROR;
		_recplayer_force_play_next(record, true);
		break;
	case PLAYBACK_EVENT_STOP_COMPLETE:
		record->prev_music_state = 0;
		record->mplayer_state = RECPLAYER_STATE_NORMAL;
		record->restart_iterator_times = 0;
		_recplayer_force_play_next(record, false);
		break;
	default:

		break;
	}
}

static void _recplayer_seek_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct app_msg msg = {
		.type = MSG_RECORDER_EVENT,
	};
	struct recorder_app_t *record = recorder_get_app();

	if (!record || !record->seek_direction)
		return;
	/*need to clear bp after tts play */
	_recplayer_clear_breakpoint(record);

	SYS_LOG_INF("%d\n", record->seek_direction);
	record->mplayer_state = RECPLAYER_STATE_NORMAL;
	record->prev_music_state = 0;
	if (record->seek_direction == SEEK_FORWARD) {
		_recplayer_force_play_next(record, false);
	} else {
		msg.cmd = MSG_RECPLAYER_PLAY_CUR;
		send_async_msg(APP_ID_RECORDER, &msg);
	}
	record->seek_direction = SEEK_NULL;
}
static void _recorder_esd_save_bp_info(struct recorder_app_t *record)
{
#ifdef CONFIG_ESD_MANAGER
	esd_manager_save_scene(TAG_BP_INFO, (u8_t *)&record->recplayer_bp_info, sizeof(struct recplayer_bp_info_t));
#endif
}

static void _recorder_disk_check(struct recorder_app_t *record)
{
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	struct app_msg msg = {0};

	if (++record->check_disk_plug_time < 5) {
		if (fs_manager_get_volume_state("USB:")) {
			recorder_prepare("USB:RECORD");
			record->is_disk_check = 0;
			record->check_disk_plug_time = 0;
		} else if (fs_manager_get_volume_state("SD:")) {
			recorder_prepare("SD:RECORD");
			record->is_disk_check = 0;
			record->check_disk_plug_time = 0;
		} else {
			thread_timer_start(&record->monitor_timer, 1000, 0);
		}
	} else {
		SYS_LOG_ERR("disk check timer out\n");
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		msg.ptr = NULL;
		/* change to next app */
		send_async_msg(APP_ID_MAIN, &msg);
	}
#else
	record->is_disk_check = 0;
	recorder_prepare("USB:RECORD");
#endif
}
static void _recorder_monitor_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct recorder_app_t *record = recorder_get_app();
	struct app_msg msg = {0};

	if (!record)
		return;

	if (record->is_disk_check) {
		_recorder_disk_check(record);
		return;
	}

	if (!record->record_or_play) {
		if (record->start_reason == RECORDER_RECORD_INIT) {
			record->start_reason = RECORDER_NULL;
			msg.type = MSG_RECORDER_EVENT;
			msg.cmd = MSG_RECORDER_START;
			send_async_msg(APP_ID_RECORDER, &msg);
			return;
		}

		if (record->recording != RECORDER_STATUS_START || !record->recorder_stream)
			return;
		record->record_time++;
		/*sync data to disk*/
		if (!(record->record_time % SYNC_RECORD_DATA_PERIOD)) {
			if (stream_flush(record->recorder_stream)) {
				SYS_LOG_ERR("sync fail\n");
			} else {
				printk("sync ok\n");
			}
		}
		/*display record time*/
		if (record->record_time >= 3600)
			record->record_time = 0;
		recorder_display_record_time(record->record_time);
	} else {
		if (record->start_reason == RECORDER_PLAY_INIT) {
			record->start_reason = RECORDER_NULL;
			msg.type = MSG_RECORDER_EVENT;
			msg.cmd = MSG_RECPLAYER_PLAY_URL;
			send_async_msg(APP_ID_RECORDER, &msg);
			return;
		}
		/*display seek progress*/
		if (record->music_state == RECFILE_STATUS_SEEK) {
			if (record->seek_direction == SEEK_FORWARD) {
				recplayer_display_play_time(record, SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
			} else if (record->seek_direction == SEEK_BACKWARD) {
				recplayer_display_play_time(record, -SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
			}
		}

		if (!record->lcplayer || (record->music_state != RECFILE_STATUS_PLAYING))
			return;
		if (record->is_play_in5s) {
			if (++record->play_time > 10) {
				record->is_play_in5s = 0;
				record->play_time = 0;
				SYS_LOG_INF("play 5s\n");
			}
		}
		mplayer_update_breakpoint(record->lcplayer, &record->recplayer_bp_info.bp_info);
		recplayer_display_play_time(record, 0);

		_recorder_esd_save_bp_info(record);
	}
}

static void _recorder_read_cache_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	int len = 0;
	int sum_len = 0;
	struct recorder_app_t *record = recorder_get_app();

	if (!record || !record->recorder_stream || !record->cache_stream)
		return;
	u32_t begin = k_cycle_get_32();

	while (stream_get_length(record->cache_stream) >= sizeof(record_cache_read_temp)) {
		len = stream_read(record->cache_stream, record_cache_read_temp, sizeof(record_cache_read_temp));
		if (len < 0) {
			SYS_LOG_ERR("error\n");
			return;
		}

		if (len != sizeof(record_cache_read_temp)) {
			SYS_LOG_WRN("len:%d\n", len);
		}
		stream_write(record->recorder_stream, record_cache_read_temp, len);
		sum_len += len;
	}
	u32_t cost = k_cycle_get_32();

	if (cost - begin > 24000 * 50)
		printk("case %d us  len %d\n", (cost - begin)/24, sum_len);
}

void recorder_init_timer(struct recorder_app_t *record)
{
	thread_timer_init(&record->seek_timer, _recplayer_seek_timer, NULL);
	thread_timer_init(&record->monitor_timer, _recorder_monitor_timer, NULL);
	thread_timer_init(&record->read_cache_timer, _recorder_read_cache_timer, NULL);
}

void recplayer_bp_save(struct recorder_app_t *record)
{
	if ((record->music_state == RECFILE_STATUS_ERROR) || (record->music_state == RECFILE_STATUS_NULL))
		return;
	mplayer_update_breakpoint(record->lcplayer, &record->recplayer_bp_info.bp_info);
	recplayer_bp_info_save(record->dir, record->recplayer_bp_info);
}

static void _recplayer_start(struct recorder_app_t *record, const char *url, int seek_time)
{
	struct lcplay_param play_param;

	if (!record->record_or_play)
		return;

	memset(&play_param, 0, sizeof(struct lcplay_param));
	play_param.url = url;
	play_param.seek_time = seek_time;
	play_param.play_event_callback = _recplayer_play_event_callback;
	play_param.bp.time_offset = record->recplayer_bp_info.bp_info.time_offset;
	play_param.bp.file_offset = record->recplayer_bp_info.bp_info.file_offset;

	record->lcplayer = mplayer_start_play(&play_param);
	if (!record->lcplayer) {
		record->music_state = RECFILE_STATUS_ERROR;
		if (record->mplayer_state != RECPLAYER_STATE_NORMAL)
			record->mplayer_state = RECPLAYER_STATE_ERROR;
		_recplayer_force_play_next(record, true);
	} else {
		record->is_play_in5s = 1;
		record->play_time = 0;
		record->music_state = RECFILE_STATUS_PLAYING;
		recorder_store_play_state();
		recplayer_display_icon_play();

		thread_timer_start(&record->monitor_timer, 0, MONITOR_TIME_PERIOD / 2);

	}
}
void recplayer_stop(struct recorder_app_t *record, bool need_updata_bp)
{
	_recplayer_clear_breakpoint(record);

	if (!record->lcplayer)
		return;

	if (need_updata_bp) {
		mplayer_update_breakpoint(record->lcplayer, &record->recplayer_bp_info.bp_info);
		mplayer_update_media_info(record->lcplayer, &record->media_info);
	}

	if (record->music_state != RECFILE_STATUS_SEEK) {
		if (thread_timer_is_running(&record->seek_timer))
			thread_timer_stop(&record->seek_timer);
	}

	if (thread_timer_is_running(&record->monitor_timer))
		thread_timer_stop(&record->monitor_timer);

	mplayer_stop_play(record->lcplayer);

	record->lcplayer = NULL;

	if (record->music_state == RECFILE_STATUS_PLAYING)
		record->music_state = RECFILE_STATUS_STOP;
}

static void _recplayer_switch_app_check(struct recorder_app_t *record)
{
	struct app_msg msg = {0};

	if (record->mplayer_state == RECPLAYER_STATE_ERROR) {
		msg.reserve = 0;
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		if (_check_disk_plugout(record)) {
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
		recplayer_display_play_time(record, 0);
		record->music_state = RECFILE_STATUS_ERROR;
		SYS_LOG_INF("not switch\n");
	}

}
void recorder_tts_event_proc(struct app_msg *msg)
{
	struct recorder_app_t *record = recorder_get_app();

	if (!record)
		return;

	SYS_LOG_INF("msg value =%d\n", msg->value);

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (!record->record_or_play) {
			/*record not control tts*/
			if (record->recording == RECORDER_STATUS_START)
				record->need_resume = 1;
			recorder_stop(record);
		} else {
			if (record->music_state == RECFILE_STATUS_PLAYING)
				record->need_resume = 1;
			if (record->lcplayer)
				recplayer_stop(record, true);
		}
		break;
	case TTS_EVENT_STOP_PLAY:
		if (!record->record_or_play) {
			if (record->need_resume && !record->recording) {
				recorder_start(record);
			}
		} else {
			if (record->need_resume && (record->music_state != RECFILE_STATUS_PLAYING) && (record->music_state != RECFILE_STATUS_ERROR)) {
				_recplayer_start(record, record->recplayer_bp_info.file_path, 0);
			}
		}
		record->need_resume = 0;
		break;
	default:
		break;
	}
}

void recorder_input_event_proc(struct app_msg *msg)
{
	struct recorder_app_t *record = recorder_get_app();
	struct app_msg new_msg = {0};
	u8_t temp_mode = 0;

	if (!record)
		return;

	SYS_LOG_INF("msg cmd =%d\n", msg->cmd);
	switch (msg->cmd) {
	case MSG_RECORDER_INIT:
		if (record->record_or_play) {
			recplayer_stop(record, false);
			record->record_or_play = 0;
			recorder_display_record_string();
		}
		record->start_reason = RECORDER_RECORD_INIT;
		thread_timer_start(&record->monitor_timer, MONITOR_TIME_PERIOD, 0);
		break;
	case MSG_RECORDER_SAVE_AND_START_NEXT:
		recorder_stop(record);
		recorder_start(record);
		break;
	case MSG_RECORDER_PAUSE_RESUME:
		if (record->recording == RECORDER_STATUS_START) {
			recorder_pause(record);
		} else {
			recorder_resume(record);
			if (record->recording != RECORDER_STATUS_START) {
			/*stop state switch app or esd,need to start*/
				recorder_start(record);
			}
		}
		break;
	case MSG_RECPLAYER_INIT:
		if (!record->record_or_play) {
			record->record_or_play = 1;
			recorder_stop(record);
			recplayer_display_icon_disk(record);
			recplayer_stop(record, false);
		}
		/*play tts first*/
		record->start_reason = RECORDER_PLAY_INIT;
		thread_timer_start(&record->monitor_timer, MONITOR_TIME_PERIOD, 0);
		break;
	case MSG_RECPLAYER_PLAY_NEXT:
		if (!record->record_or_play)
			break;
		record->prev_music_state = 0;
		recplayer_stop(record, false);
		sys_event_notify(SYS_EVENT_PLAY_NEXT);
		temp_mode = record->mplayer_mode;/* save current mplay mode */
		if (record->mplayer_mode != RECPLAYER_REPEAT_ALL_MODE) {
			record->mplayer_mode = RECPLAYER_REPEAT_ALL_MODE;
		}

		if (recplayer_play_next_url(record, true)) {
			/*play tts first*/
			new_msg.type = MSG_RECORDER_EVENT;
			new_msg.cmd = MSG_RECPLAYER_PLAY_URL;
			send_async_msg(APP_ID_RECORDER, &new_msg);
		} else {
			_recplayer_switch_app_check(record);
		}
		record->mplayer_mode = temp_mode;/* resume mplayer mode */
		break;
	case MSG_RECPLAYER_PLAY_PREV:
		record->prev_music_state = 1;
		if (record->is_play_in5s) {
			record->is_play_in5s = 0;
			recplayer_stop(record, false);
			sys_event_notify(SYS_EVENT_PLAY_PREVIOUS);

			if (recplayer_play_prev_url(record)) {
				/*play tts first*/
				new_msg.type = MSG_RECORDER_EVENT;
				new_msg.cmd = MSG_RECPLAYER_PLAY_URL;
				send_async_msg(APP_ID_RECORDER, &new_msg);
			} else {
				_recplayer_switch_app_check(record);
			}
		} else {
			if ((record->music_state == RECFILE_STATUS_PLAYING) || (record->music_state == RECFILE_STATUS_STOP)) {
				recplayer_stop(record, false);
				_recplayer_start(record, record->recplayer_bp_info.file_path, 0);
			} else {
				recplayer_stop(record, false);
				if (!record->filter_prev_tts) {
					sys_event_notify(SYS_EVENT_PLAY_PREVIOUS);
				}
				record->filter_prev_tts = 0;
				if (recplayer_play_prev_url(record)) {
					_recplayer_start(record, record->recplayer_bp_info.file_path, 0);
				} else {
					_recplayer_switch_app_check(record);
				}
			}
		}
		break;
	case MSG_RECPLAYER_PLAY_VOLUP:
		system_volume_up(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;
	case MSG_RECPLAYER_PLAY_VOLDOWN:
		system_volume_down(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;
	case MSG_RECPLAYER_PLAY_PAUSE:
		if (record->music_state == RECFILE_STATUS_PLAYING) {
			recplayer_stop(record, true);
			recorder_store_play_state();
			sys_event_notify(SYS_EVENT_PLAY_PAUSE);
			recplayer_bp_save(record);
		}
		break;
	case MSG_RECPLAYER_PLAY_PLAYING:
		sys_event_notify(SYS_EVENT_PLAY_START);

		new_msg.type = MSG_RECORDER_EVENT;
		new_msg.cmd = MSG_RECPLAYER_PLAY_URL;
		send_async_msg(APP_ID_RECORDER, &new_msg);
		break;
	case MSG_RECPLAYER_PLAY_SEEK_FORWARD:
		if (thread_timer_is_running(&record->seek_timer))
			thread_timer_stop(&record->seek_timer);

		record->seek_direction = SEEK_NULL;
		if (record->music_state == RECFILE_STATUS_SEEK) {
			if (thread_timer_is_running(&record->monitor_timer))
				thread_timer_stop(&record->monitor_timer);

			_recplayer_start(record, record->recplayer_bp_info.file_path, SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
		}
		break;
	case MSG_RECPLAYER_PLAY_SEEK_BACKWARD:
		if (thread_timer_is_running(&record->seek_timer))
			thread_timer_stop(&record->seek_timer);

		record->seek_direction = SEEK_NULL;
		if (record->music_state == RECFILE_STATUS_SEEK) {
			if (thread_timer_is_running(&record->monitor_timer))
				thread_timer_stop(&record->monitor_timer);

			_recplayer_start(record, record->recplayer_bp_info.file_path, -(SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time)));
		}
		break;
	case MSG_RECPLAYER_SEEKFW_START_CTIME:
		record->music_state = RECFILE_STATUS_SEEK;
		record->seek_direction = SEEK_FORWARD;
		start_seek_time = os_uptime_get_32();
		recplayer_stop(record, true);
		thread_timer_start(&record->monitor_timer, MONITOR_TIME_PERIOD / 10, MONITOR_TIME_PERIOD / 10);
		if (record->media_info.total_time > record->recplayer_bp_info.bp_info.time_offset) {
			thread_timer_start(&record->seek_timer,
				(record->media_info.total_time - record->recplayer_bp_info.bp_info.time_offset) / SEEK_SPEED_LEVEL, 0);
		}
		break;
	case MSG_RECPLAYER_SEEKBW_START_CTIME:
		record->music_state = RECFILE_STATUS_SEEK;
		record->seek_direction = SEEK_BACKWARD;
		start_seek_time = os_uptime_get_32();
		recplayer_stop(record, true);
		thread_timer_start(&record->monitor_timer, MONITOR_TIME_PERIOD / 10, MONITOR_TIME_PERIOD / 10);
		thread_timer_start(&record->seek_timer,
			record->recplayer_bp_info.bp_info.time_offset / SEEK_SPEED_LEVEL, 0);
		break;
	case MSG_RECPLAYER_SET_PLAY_MODE:
		if (++record->mplayer_mode >= RECPLAYER_NUM_PLAY_MODES)
			record->mplayer_mode = 0;
		SYS_LOG_INF("set mplayer_mode :%d\n", record->mplayer_mode);
		break;

	default:
		break;
	}
}

void recorder_event_proc(struct app_msg *msg)
{
	struct recorder_app_t *record = recorder_get_app();

	if (!record)
		return;

	SYS_LOG_INF("msg cmd =%d\n", msg->cmd);
	switch (msg->cmd) {
	case MSG_RECORDER_START:
		recorder_start(record);
		break;
	case MSG_RECPLAYER_AUTO_PLAY_NEXT:
		if (!record->record_or_play)
			break;
		recplayer_stop(record, false);
		if (recplayer_play_next_url(record, false)) {
			_recplayer_start(record, record->recplayer_bp_info.file_path, 0);
		} else {
			_recplayer_switch_app_check(record);
		}
		break;
	case MSG_RECPLAYER_PLAY_URL:
		if (record->lcplayer) {
			recplayer_stop(record, false);
		}
		_recplayer_start(record, record->recplayer_bp_info.file_path, 0);
		break;
	case MSG_RECPLAYER_PLAY_CUR:
		if (!record->record_or_play)
			break;
		recplayer_stop(record, false);
		_recplayer_start(record, record->recplayer_bp_info.file_path, 0);
		break;
	default:
		break;
	}
}

