/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief usound function.
 */


#ifndef _USOUND_APP_H
#define _USOUND_APP_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "usound"

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
#include <usb_audio_hal.h>

#include <dvfs.h>
#include <thread_timer.h>


#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

enum {
	MSG_USOUND_EVENT = MSG_APP_MESSAGE_START,
};

enum {
	MSG_USOUND_PLAY_PAUSE_RESUME = 1,
	MSG_USOUND_PLAY_VOLUP,
	MSG_USOUND_PLAY_VOLDOWN,
	MSG_USOUND_PLAY_NEXT,
	MSG_USOUND_PLAY_PREV,
};

enum {
	MSG_USOUND_STREAM_START = 1,
	MSG_USOUND_STREAM_STOP,
	MSG_USOUND_STREAM_RESTART,
};

enum USOUND_PLAY_STATUS {
	USOUND_STATUS_NULL = 0x0000,
	USOUND_STATUS_PLAYING = 0x0001,
	USOUND_STATUS_PAUSED = 0x0002,
};

enum USOUND_VOLUME_REQ_TYPE {
	USOUND_VOLUME_NONE = 0x0000,
	USOUND_VOLUME_DEC = 0x0001,
	USOUND_VOLUME_INC = 0x0002,
};

#define USOUND_STATUS_ALL  (USOUND_STATUS_PLAYING | USOUND_STATUS_PAUSED)

struct usound_app_t {
	media_player_t *player;
	struct thread_timer monitor_timer;
	u16_t playing : 1;
	u16_t need_resume_play : 1;
	u16_t media_opened : 1;
	u16_t volume_req_type : 2;
	u8_t mic_upload;
	short volume_req_level : 8;
	short current_volume_level : 8;
	io_stream_t usound_stream;
	io_stream_t usound_upload_stream;
	io_stream_t ext_stream;

	u8_t uac_status;
};
void usound_view_init(void);
void usound_view_deinit(void);
struct usound_app_t *usound_get_app(void);
int usound_get_status(void);
void usound_start_play(void);
void usound_stop_play(void);
void usound_input_event_proc(struct app_msg *msg);
void usound_tts_event_proc(struct app_msg *msg);
void usound_bt_event_proc(struct app_msg *msg);
u32_t usound_get_audio_stream_type(char *app_name);
void usound_event_proc(struct app_msg *msg);
void usound_show_play_status(bool status);
void usound_view_clear_screen(void);
void usound_view_volume_show(int volume_value);
#endif  /* _USOUND_APP_H */

