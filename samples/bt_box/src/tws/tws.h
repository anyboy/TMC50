/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt tws
 */

#ifndef _BT_MUSIC_APP_H_
#define _BT_MUSIC_APP_H_

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "tws"
#include <logging/sys_log.h>
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <stream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <volume_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <global_mem.h>
#include <dvfs.h>
#include <thread_timer.h>
#include "btservice_api.h"
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

struct tws_app_t {
	media_player_t *player;
	u32_t playing : 1;
	u32_t sco_playing : 1;
	u32_t need_resume_play : 1;
	u32_t a2dp_connected : 1;
	u32_t avrcp_connected : 1;
	u32_t media_opened : 1;
	u32_t stream_type:8;
	u32_t codec_id:8;
	u32_t sample_rate:8;
	io_stream_t bt_stream;
	io_stream_t upload_stream;
};

struct tws_app_t *tws_get_app(void);

void tws_a2dp_stop_play(void);

void tws_a2dp_start_play(u8_t stream_type, u8_t codec_id, u8_t sample_rate, u8_t volume);

void tws_sco_start_play(void);

void tws_sco_stop_play(void);

bool tws_key_event_handle(int key_event, int event_stage);

void tws_input_event_proc(struct app_msg *msg);

void tws_tts_event_proc(struct app_msg *msg);

void tws_bt_event_proc(struct app_msg *msg);

void tws_view_init(void);

void tws_view_deinit(void);

void tws_view_show_play_status(bool status);

#endif  /* _BT_MUSIC_APP_H */


