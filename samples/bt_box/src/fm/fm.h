/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief fm function.
 */
#ifndef _FM_APP_H
#define _FM_APP_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "fm"
#ifdef SYS_LOG_LEVEL
#undef SYS_LOG_LEVEL
#endif

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
#include <fm_manager.h>
#include <dvfs.h>
#include <thread_timer.h>
#include "btservice_api.h"


#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#include "app_switch.h"
#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#define MAX_RADIO_STATION_NUM 40

#define FM_CODEC_ID     (0)

#define FM_SAMPLERATE   (SAMPLE_44KHZ)

enum {
	MSG_FM_EVENT = MSG_APP_MESSAGE_START,
};

enum FM_MSG_EVENT_CMD {
	MSG_FM_PLAY_PAUSE_RESUME,
	MSG_FM_PLAY_VOLUP,
	MSG_FM_PLAY_VOLDOWN,
	MSG_FM_PLAY_NEXT,
	MSG_FM_PLAY_PREV,
	MSG_FM_MANUAL_SEARCH_NEXT,
	MSG_FM_MANUAL_SEARCH_PREV,
	MSG_FM_AUTO_SEARCH,
	MSG_FM_MEDIA_START,
};

enum FM_PLAY_STATUS {
	FM_STATUS_NULL      = 0x0000,
	FM_STATUS_PLAYING   = 0x0001,
	FM_STATUS_PAUSED    = 0x0002,
	FM_STATUS_SCANING   = 0x0003,
};

#define FM_STATUS_ALL  (FM_STATUS_PLAYING | FM_STATUS_PAUSED | FM_STATUS_SCANING)

enum FM_AREA {
	CH_US_AREA,
	JP_AREA,
	EU_AREA,
	MAX_AREA,
};

enum SEARCH_MODE {
	NONE_SCAN,
	AUTO_SCAN,
	NEXT_STATION,
	PREV_STATION,
};

struct fm_radio_station_config {
	u16_t min_freq;
	u16_t max_freq;
	u16_t step;
};

struct fm_radio_station_info {
	u16_t default_freq;
	u16_t history_station[MAX_RADIO_STATION_NUM];
	s16_t current_station_index;
	u16_t station_num;
};

struct fm_app_t {
	media_player_t *player;
	struct thread_timer monitor_timer;
	struct thread_timer tts_resume_timer;
	struct fm_radio_station_info radio_station;
	u32_t playing : 4;
	u32_t try_listening : 4;
	u32_t scan_flag : 1;
	u32_t mute_flag : 1;    /* 1: mute, 0: unmute */
	u32_t need_resume_play : 1;
	u32_t scan_mode : 2;
	u16_t try_listening_count;
	u16_t current_freq;
	u16_t manual_freq;
	struct fm_radio_station_config *auto_scan_config;
	io_stream_t fm_stream;
};

void fm_view_init(void);
void fm_view_deinit(void);
struct fm_app_t *fm_get_app(void);
int fm_get_status(void);
void fm_start_play(void);
void fm_stop_play(void);
void fm_delay_start_play(struct fm_app_t *fm);
void fm_input_event_proc(struct app_msg *msg);
void fm_tts_event_proc(struct app_msg *msg);
void fm_bt_event_proc(struct app_msg *msg);
u32_t fm_get_audio_stream_type(char *app_name);

int fm_radio_station_load(struct fm_app_t *fm);
int fm_radio_station_save(struct fm_app_t *fm);
u16_t fm_radio_station_get_current(struct fm_app_t *fm);
u16_t fm_radio_station_get_next(struct fm_app_t *fm);
u16_t fm_radio_station_get_prev(struct fm_app_t *fm);
int fm_radio_station_manual_search(struct fm_app_t *fm, int mode);
int fm_radio_station_auto_scan(struct fm_app_t *fm);
void fm_view_show_freq(u16_t freq);
void fm_view_show_station_num(u16_t num);
void fm_view_play_freq(u16_t freq);
int fm_radio_station_cancel_scan(struct fm_app_t *fm);
int fm_radio_station_set_mute(struct fm_app_t *fm);
void fm_view_show_play_status(bool status);
int fm_function_init(void);
void fm_view_show_try_listen(void);

#endif  /* _FM_APP_H */

