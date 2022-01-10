/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record app
 */
#ifndef _RECORDER_APP_H
#define _RECORDER_APP_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "record"


#include <logging/sys_log.h>
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <global_mem.h>
#include <stream.h>
#include <file_stream.h>

#include <dvfs.h>
#include <thread_timer.h>
#include "btservice_api.h"


#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#include "app_switch.h"
#include <fs.h>
#ifdef CONFIG_FS_MANAGER
#include <fs_manager.h>
#endif
#include "../local_player/local_player.h"
#include <ui_manager.h>

#define MONITOR_TIME_PERIOD OS_MSEC(1000)
#define RECORD_DIR_LEN	(10)
#define RECORD_FILE_PATH_LEN	(24)
#define MAX_RECORD_FILES (999)
#define FILE_FORMAT	".WAV"

enum RECORDER_MSG_EVENT_TYPE {
    /* record key message */
	MSG_RECORDER_EVENT = MSG_APP_MESSAGE_START,
};

enum RECORDER_MSG_EVENT_CMD {
	MSG_RECORDER_INIT = 0,
	MSG_RECORDER_START,
	MSG_RECORDER_SAVE_AND_START_NEXT,
	MSG_RECORDER_PAUSE_RESUME,

	MSG_RECPLAYER_INIT,
	MSG_RECPLAYER_PLAY_URL,
	MSG_RECPLAYER_PLAY_CUR,
	MSG_RECPLAYER_PLAY_NEXT,
	MSG_RECPLAYER_PLAY_PREV,
	MSG_RECPLAYER_AUTO_PLAY_NEXT,
	MSG_RECPLAYER_PLAY_VOLUP,
	MSG_RECPLAYER_PLAY_VOLDOWN,
	MSG_RECPLAYER_PLAY_PAUSE,
	MSG_RECPLAYER_PLAY_PLAYING,
	MSG_RECPLAYER_PLAY_SEEK_FORWARD,
	MSG_RECPLAYER_PLAY_SEEK_BACKWARD,
	MSG_RECPLAYER_SEEKFW_START_CTIME,
	MSG_RECPLAYER_SEEKBW_START_CTIME,
	MSG_RECPLAYER_SET_PLAY_MODE,
};

enum RECORDER_PLAY_STATUS {
	RECORDER_STATUS_NULL	= 0x0000,
	RECORDER_STATUS_START	= 0x0001,
	RECORDER_STATUS_STOP	= 0x0002,
};

#define RECORDER_STATUS_ALL  (RECORDER_STATUS_START | RECORDER_STATUS_STOP)

#define RECPLAYER_STATE_INIT 0
#define RECPLAYER_STATE_NORMAL 1
#define RECPLAYER_STATE_ERROR 2

#define SPECIFY_DIR_NAME 10
#define RECFILE_STATUS_ALL  (RECFILE_STATUS_PLAYING | RECFILE_STATUS_SEEK | RECFILE_STATUS_ERROR | RECFILE_STATUS_STOP)

enum RECPLAYER_PLAY_MODE {
	RECPLAYER_REPEAT_ALL_MODE = 0,
	RECPLAYER_REPEAT_ONE_MODE,
	RECPLAYER_NORMAL_ALL_MODE,

	RECPLAYER_NUM_PLAY_MODES,
};

enum RECFLIE_PLAY_STATUS {
	RECFILE_STATUS_NULL    = 0x0000,
	RECFILE_STATUS_PLAYING = 0x0004,
	RECFILE_STATUS_SEEK    = 0x0008,
	RECFILE_STATUS_ERROR   = 0x0010,
	RECFILE_STATUS_STOP    = 0x0020,
};

enum RECFLIE_SEEK_STATUS {
	SEEK_NULL = 0,
	SEEK_FORWARD,
	SEEK_BACKWARD,
};

enum RECORDER_INIT_STATUS {
	RECORDER_NULL = 0,
	RECORDER_RECORD_INIT,
	RECORDER_PLAY_INIT,
};

struct recplayer_bp_info_t {
	media_breakpoint_info_t bp_info;
	char file_path[RECORD_FILE_PATH_LEN];
};

struct recorder_app_t {
	media_player_t *player;
	struct thread_timer monitor_timer;
	struct thread_timer seek_timer;
	struct thread_timer read_cache_timer;

	char dir[RECORD_DIR_LEN + 1];
	u32_t file_bitmaps[MAX_RECORD_FILES / 32 + 1];
	u32_t record_or_play : 1;/* 0---record,1---playback; */
	u32_t recording : 2;
	u32_t need_resume : 1;
	u32_t disk_space_less : 1;
	u32_t file_is_full : 1;
	u32_t file_count : 16;
	u32_t prev_music_state : 1;
	u32_t seek_direction : 2;/*1--forward,2--backward*/
	u32_t start_reason : 2;/*1--recorder,2--recplayer*/
	u32_t is_play_in5s : 1;
	u32_t play_time : 4;
	u32_t disk_space_size;/*M byte*/

	io_stream_t recorder_stream;
	io_stream_t cache_stream;

	u8_t restart_iterator_times;
	u8_t music_state;
	u8_t mplayer_state;
	u8_t mplayer_mode;
	u16_t record_time;
	u16_t is_disk_check : 1;
	u16_t check_disk_plug_time : 3;
	u16_t filter_prev_tts : 1;
	struct recplayer_bp_info_t recplayer_bp_info;
	media_info_t media_info;
	struct local_player_t *lcplayer;
};
void recorder_view_init(void);
void recorder_view_deinit(void);
struct recorder_app_t *recorder_get_app(void);
int recorder_get_status(void);
void recorder_start(struct recorder_app_t *record);
void recorder_stop(struct recorder_app_t *record);
void recorder_pause(struct recorder_app_t *record);
void recorder_resume(struct recorder_app_t *record);
void recorder_input_event_proc(struct app_msg *msg);
void recorder_event_proc(struct app_msg *msg);
void recorder_tts_event_proc(struct app_msg *msg);
void recorder_repair_file(struct recorder_app_t *record);
void recorder_get_file_count(struct recorder_app_t *record);
void recplayer_bp_save(struct recorder_app_t *record);
void recplayer_bp_info_save(const char *dir, struct recplayer_bp_info_t recplayer_bp_info);
const char *recplayer_play_next_url(struct recorder_app_t *record, bool force_switch);
int recplayer_bp_info_resume(const char *dir, struct recplayer_bp_info_t *recplayer_bp_info);
const char *recplayer_play_prev_url(struct recorder_app_t *record);
void recplayer_stop(struct recorder_app_t *record, bool need_updata_bp);
void recorder_init_timer(struct recorder_app_t *record);
void recorder_store_play_state(void);
void recorder_display_record_string(void);

void recorder_display_record_time(u16_t record_time);
void recorder_display_record_start(struct recorder_app_t *record);
void recorder_display_record_pause(struct recorder_app_t *record);
void recplayer_display_icon_disk(struct recorder_app_t *record);
void recplayer_display_play_time(struct recorder_app_t *record, int seek_time);
void recplayer_display_icon_play(void);
void recplayer_display_icon_pause(void);
int record_update_file_bitmaps(struct recorder_app_t *record, const char *dir);
int recorder_prepare(char *dir);

#endif  /* _RECORDER_APP_H */

