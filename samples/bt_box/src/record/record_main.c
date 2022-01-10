/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record app main.
 */

#include "record.h"
#include "tts_manager.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
#include <hotplug_manager.h>

static struct recorder_app_t *p_recorder = NULL;
static u8_t recorder_init_state = RECORDER_STATUS_START;

void recorder_store_play_state(void)
{
	if (!p_recorder->record_or_play) {
		recorder_init_state = p_recorder->recording;
	} else {
		recorder_init_state = p_recorder->music_state;
	}

#ifdef CONFIG_ESD_MANAGER
	u8_t playing = recorder_init_state;

	esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
#endif
}

static int _recorder_get_prev_play_state(u8_t *init_play_state)
{
	int res = 0;
#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		SYS_LOG_INF("esd restart\n");
		esd_manager_restore_scene(TAG_PLAY_STATE, init_play_state, 1);
		if (*init_play_state <= RECORDER_STATUS_STOP) {
			p_recorder->record_or_play = 0;
		} else if (*init_play_state == RECFILE_STATUS_PLAYING || *init_play_state == RECFILE_STATUS_STOP) {
			esd_manager_restore_scene(TAG_BP_INFO, (u8_t *)&p_recorder->recplayer_bp_info, sizeof(struct recplayer_bp_info_t));
			p_recorder->record_or_play = 1;
			SYS_LOG_INF("file_path:%s\n", p_recorder->recplayer_bp_info.file_path);
		}
	} else
#endif
	{
		*init_play_state = recorder_init_state;

		SYS_LOG_INF("record_state = %d\n", *init_play_state);
		if ((*init_play_state == RECFILE_STATUS_PLAYING) || (*init_play_state == RECFILE_STATUS_STOP)) {
			p_recorder->record_or_play = 1;
		} else {
			p_recorder->record_or_play = 0;
		}
		res = 1;
	}
	return res;
}
static void _recorder_restore_play_state(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
	};
	u8_t init_play_state = 0;

	if (_recorder_get_prev_play_state(&init_play_state))
		recplayer_bp_info_resume(p_recorder->dir, &p_recorder->recplayer_bp_info);

	if (p_recorder->record_or_play) {
		recplayer_display_icon_disk(p_recorder);
		if (init_play_state == RECFILE_STATUS_PLAYING) {
			msg.cmd = MSG_RECPLAYER_INIT;
			send_async_msg(APP_ID_RECORDER, &msg);
		} else {
			recplayer_display_icon_pause();
			recplayer_display_play_time(p_recorder, 0);
		}
	} else {
		recorder_display_record_string();
		if (init_play_state == RECORDER_STATUS_STOP) {
			recorder_display_record_pause(p_recorder);
		} else {
			msg.cmd = MSG_RECORDER_INIT;
			send_async_msg(APP_ID_RECORDER, &msg);
		}
	}
}

static int check_file_dir_exists(const char *path)
{
	int res = 0;
	struct fs_dirent *entry = NULL;

	/* Verify fs_stat() */
	entry = app_mem_malloc(sizeof(struct fs_dirent));
	if (entry) {
		res = fs_stat(path, entry);
		if (res) {
			SYS_LOG_WRN("path %s not found (res=%d)\n", path, res);
			app_mem_free(entry);
			return -ENOENT;
		}
		app_mem_free(entry);
	} else {
		SYS_LOG_ERR("malloc faild\n");
		return -ENOENT;
	}
	return res;
}
int recorder_prepare(char *dir)
{
	strncpy(p_recorder->dir, dir, sizeof(p_recorder->dir));
	if (check_file_dir_exists(p_recorder->dir)) {
		int res = fs_mkdir(p_recorder->dir);

		if (res) {
			SYS_LOG_ERR("mkdir:%s [%d]\n", p_recorder->dir, res);
			return -ENOENT;
		}
	}
	recorder_repair_file(p_recorder);
	recorder_get_file_count(p_recorder);

#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	u32_t begin = k_cycle_get_32();
#endif

	record_update_file_bitmaps(p_recorder, p_recorder->dir);
	SYS_LOG_INF("update bitmaps :%d us\n", (k_cycle_get_32() - begin)/24);
	_recorder_restore_play_state();
	return 0;
}

static void _recorder_start_check_disk_timer(void)
{
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	thread_timer_start(&p_recorder->monitor_timer, 100, 0);
	p_recorder->is_disk_check = 1;
#else
	recorder_prepare("USB:RECORD");
#endif
}
static int _recorder_init(void)
{
	if (p_recorder)
		return 0;

	p_recorder = app_mem_malloc(sizeof(struct recorder_app_t));
	if (!p_recorder) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}
	memset(p_recorder, 0, sizeof(struct recorder_app_t));
	recorder_view_init();
	recorder_init_timer(p_recorder);

	SYS_LOG_INF("ok\n");
	return 0;
}
static void _recorder_exit()
{
	if (!p_recorder)
		goto exit;

	if (p_recorder->record_or_play) {
		recplayer_bp_save(p_recorder);
		recplayer_stop(p_recorder, false);
	} else {
		recorder_stop(p_recorder);
	}

	recorder_view_deinit();

	app_mem_free(p_recorder);

	p_recorder = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF(" -- finish--\n");
}

struct recorder_app_t *recorder_get_app(void)
{
	return p_recorder;
}

static void _recorder_main_prc(void)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_recorder_init()) {
		SYS_LOG_ERR("recorder init failed\n");
	} else {
		 _recorder_start_check_disk_timer();
	}
	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.type) {

			case MSG_INPUT_EVENT:
				recorder_input_event_proc(&msg);
				break;
			case MSG_RECORDER_EVENT:
				recorder_event_proc(&msg);
				break;
			case MSG_TTS_EVENT:
				recorder_tts_event_proc(&msg);
				break;
			case MSG_EXIT_APP:
				_recorder_exit();
				terminated = true;
				break;
			default:
				break;
			}
			if (msg.callback)
				msg.callback(&msg, 0, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}
}
static void recorder_main_loop(void *parama1, void *parama2, void *parama3)
{
	SYS_LOG_INF("Enter app\n");
	_recorder_main_prc();
	SYS_LOG_INF("Exit app\n");
}

APP_DEFINE(recorder, share_stack_area, sizeof(share_stack_area),
			CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
			recorder_main_loop, NULL);

