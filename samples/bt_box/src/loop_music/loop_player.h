/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief loop player app
 */

#ifndef _LOOP_PLAYER_H
#define _LOOP_PLAYER_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "loop_player"

#include <logging/sys_log.h>
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <stream.h>
#include <file_stream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <volume_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <global_mem.h>
#include <thread_timer.h>
#include "btservice_api.h"
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#include <iterator/file_iterator.h>
#include <fs.h>
#include "../local_player/local_player.h"
#include <ui_manager.h>


#define SPECIFY_DIR_NAME	(10)
#define LOOP_STATUS_ALL  (LOOP_STATUS_PLAYING | LOOP_STATUS_ERROR | LOOP_STATUS_STOP)

/* energy limit (time in millionsecond) */
#define ENERGY_LIMITER_THRESHOLD	(100)
#define ENERGY_LIMITER_ATTACK_TIME	(20)
#define ENERGY_LIMITER_RELEASE_TIME	(20)


/* player state */
enum LOOP_PLAYER_STATE {
	LPPLAYER_STATE_INIT = 0,
	LPPLAYER_STATE_NORMAL,
	LPPLAYER_STATE_ERROR,
};

/* playing status */
enum LOOP_PLAY_STATUS {
	LOOP_STATUS_NULL	= 0x0000,
	LOOP_STATUS_PLAYING = 0x0001,
	LOOP_STATUS_ERROR   = 0x0002,
	LOOP_STATUS_STOP	= 0x0004,
};

/* repeat mode, loopplay repeats current song always */
enum LOOP_PLAYER_MODE {
	LPPLAYER_REPEAT_ALL_MODE = 0,

	LPPLAYER_NUM_PLAY_MODES,
};

enum LOOP_MSG_EVENT_TYPE {
	MSG_LOOPPLAY_EVENT = MSG_APP_MESSAGE_START,
};

enum LOOP_MSG_EVENT_CMD {
	MSG_LOOPPLAYER_INIT = 0,
	MSG_LOOPPLAYER_PLAY_URL,
	MSG_LOOPPLAYER_PLAY_NEXT,
	MSG_LOOPPLAYER_PLAY_PREV,
	MSG_LOOPPLAYER_AUTO_PLAY_NEXT,/*4*/

	MSG_LOOPPLAYER_PLAY_VOLUP,
	MSG_LOOPPLAYER_PLAY_VOLDOWN,
	MSG_LOOPPLAYER_PLAY_PAUSE,
	MSG_LOOPPLAYER_PLAY_PLAYING,/*8*/

	/* set energy filter param */
	MSG_LOOPPLAYER_SET_ENERGY_FILTER,

	MSG_LOOPPLAYER_SET_PLAY_MODE,
};


struct loop_play_app_t {
	u16_t track_no;
	u16_t sum_track_no;
	u8_t music_status;
	u32_t full_cycle_times : 3;
	u32_t mplayer_state : 2;
	u32_t mplayer_mode : 3;
	u32_t need_resume_play : 1;
	u32_t is_disk_check : 1;
	u32_t show_track_or_loop : 2;   /* 0-show time, 1-show loop, 2-show track */
	u32_t check_disk_plug_time : 3;
	char cur_dir[SPECIFY_DIR_NAME + 1];
	char cur_url[MAX_URL_LEN + 1];

	media_breakpoint_info_t cur_time;	/* save the current played time */
	struct local_player_t *localplayer;
	energy_filter_t energy_filt;

	struct thread_timer monitor_timer;
};


int loopplay_get_status(void);
struct loop_play_app_t *loopplay_get_app(void);
void loopplay_tts_event_proc(struct app_msg *msg);

void loopplay_bp_info_save(struct loop_play_app_t *lpplayer);
void loopplay_stop_play(struct loop_play_app_t *lpplayer, bool need_update_time);
void loopplay_thread_timer_init(struct loop_play_app_t *lpplayer);
void loopplay_tts_event_proc(struct app_msg *msg);
void loopplay_input_event_proc(struct app_msg *msg);
void loopplay_event_proc(struct app_msg *msg);

int loopplay_init_iterator(struct loop_play_app_t *lpplayer);
void loopplay_exit_iterator(void);
const char *loopplay_set_track_no(struct loop_play_app_t *lpplayer);
const char *loopplay_next_url(struct loop_play_app_t *lpplayer, bool force_switch);
const char *loopplay_prev_url(struct loop_play_app_t *lpplayer);
int loopplay_set_mode(struct loop_play_app_t *lpplayer, u8_t mode);

void loopplay_view_init(void);
void loopplay_view_deinit(void);
void loopplay_display_icon_disk(char dir[]);
void loopplay_display_icon_play(void);
void loopplay_display_icon_pause(void);
void loopplay_display_play_time(int time, int seek_time);
void loopplay_display_track_no(u16_t track_no);
void loopplay_display_loop_str();
void loopplay_display_none_str();

void _loopplayer_bpinfo_save(char dir[], u16_t track_no);
int _loopplayer_bpinfo_resume(char dir[], u16_t *track_no);

void loopplay_set_stream_info(struct loop_play_app_t *lpplayer, void *info, u8_t info_id);

#endif  /* _LOOP_PLAYER_H */
