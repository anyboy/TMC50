/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music
 */

#ifndef _BT_MUSIC_APP_H_
#define _BT_MUSIC_APP_H_

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "btmusic"
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
#include <thread_timer.h>
#include "btservice_api.h"
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

//#define CONFIG_BT_MUSIC_FREQPOINT_ENERGY_DEMO

#define BT_MUSIC_KEY_FLAG     (0x0 << 6)
#define BT_CALL_KEY_FLAG      (0x1 << 6)

struct btmusic_app_t {
	media_player_t *player;
	struct thread_timer monitor_timer;
	struct thread_timer play_timer;
	u32_t playing : 1;
	u32_t need_resume_play : 1;
	u32_t a2dp_connected : 1;
	u32_t avrcp_connected : 1;
	u32_t media_opened : 1;
	io_stream_t bt_stream;
#ifdef CONFIG_DMA_APP
	io_stream_t upload_stream;
#endif
};

enum {
    /* bt play key message */
	MSG_BT_PLAY_PAUSE_RESUME = MSG_BT_APP_MESSAGE_START,

	MSG_BT_PLAY_NEXT,

	MSG_BT_PLAY_PREVIOUS,

	MSG_BT_PLAY_VOLUP,

	MSG_BT_PLAY_VOLDOWN,

	MSG_BT_PLAY_SEEKFW_START,

	MSG_BT_PLAY_SEEKBW_START,

	MSG_BT_PLAY_SEEKFW_STOP,

	MSG_BT_PLAY_SEEKBW_STOP,

	MSG_BT_PLAY_VOL_SET,

};


enum BT_MUSIC_KEY_FUN {
	BT_MUSIC_KEY_NULL,
	BT_MUSIC_KEY_PLAY_PAUSE,
	BT_MUSIC_KEY_PREVMUSIC,
	BT_MUSIC_KEY_NEXTMUSIC,
	BT_MUSIC_KEY_FFSTART,
	BT_MUSIC_KEY_FFEND,
	BT_MUSIC_KEY_FBSTART,
	BT_MUSIC_KEY_FBEND,
	BT_MUSIC_KEY_VOLUP,
	BT_MUSIC_KEY_VOLDOWN,
};

struct btmusic_app_t *btmusic_get_app(void);

void bt_music_a2dp_stop_play(void);

void bt_music_a2dp_start_play(void);

#ifdef CONFIG_BT_MUSIC_FREQPOINT_ENERGY_DEMO
int bt_music_a2dp_get_freqpoint_energy(media_freqpoint_energy_info_t *info);
#endif

bool btmusic_key_event_handle(int key_event, int event_stage);

void btmusic_bt_event_proc(struct app_msg *msg);

void btmusic_tts_event_proc(struct app_msg *msg);

void btmusic_input_event_proc(struct app_msg *msg);

void bt_music_esd_restore(void);

void bt_music_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg);

void btmusic_view_init(void);

void btmusic_view_deinit(void);

void btmusic_view_show_connected(void);

void btmusic_view_show_disconnected(void);

void btmusic_view_show_play_paused(bool playing);

void btmusic_view_show_tws_connect(bool connect);

void btmusic_view_show_id3_info(struct bt_id3_info *pinfo);

#endif  /* _BT_MUSIC_APP_H */


