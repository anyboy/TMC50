/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio input app
 */

#ifndef _AUDIO_INPUT_APP_H
#define _AUDIO_INPUT_APP_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio_input"
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
#include <ui_manager.h>

#define AUDIO_INPUT_CODEC_ID     (0)

#define AUDIO_INPUT_SAMPLERATE   (SAMPLE_44KHZ)

//#define CONFIG_AUDIO_INPUT_ENERGY_DETECT_DEMO

enum {
    /* audio_input key message */
	MSG_AUDIO_INPUT_PLAY_PAUSE_RESUME = MSG_APP_MESSAGE_START,
	MSG_AUDIO_INPUT_PLAY_VOLUP,
	MSG_AUDIO_INPUT_PLAY_VOLDOWN,
	MSG_AUDIO_INPUT_SAMPLE_RATE_CHANGE,
};

enum AUDIO_INPUT_PLAY_STATUS {
	AUDIO_INPUT_STATUS_NULL      = 0x0000,
	AUDIO_INPUT_STATUS_PLAYING    = 0x0001,
	AUDIO_INPUT_STATUS_PAUSED = 0x0002,
};

enum AUDIO_INPUT_SPDIFRX_EVENT {
	SPDIFRE_SAMPLE_RATE_CHANGE = 1,
	SPDIFRE_TIME_OUT = 2,
};

enum AUDIO_INPUT_I2SRX_EVENT {
	I2SRX_SAMPLE_RATE_CHANGE = 1,
	I2SRX_TIME_OUT = 2,
};


#define AUDIO_INPUT_STATUS_ALL  (AUDIO_INPUT_STATUS_PLAYING | AUDIO_INPUT_STATUS_PAUSED)

struct audio_input_app_t {
	media_player_t *player;
	struct thread_timer monitor_timer;
#ifdef CONFIG_AUDIO_INPUT_ENERGY_DETECT_DEMO
	struct thread_timer energy_detect_timer;
#endif
	u32_t playing : 1;
	u32_t need_resume_play : 1;
	u32_t media_opened : 1;
	u32_t need_resample : 1;
	u32_t spdif_sample_rate : 8;
	u32_t i2srx_sample_rate : 8;
	u32_t spdifrx_srd_event : 2;/*1--sample rate event,2--timeout*/
	u32_t i2srx_srd_event : 2;/*1--sample rate event,2--timeout*/
	io_stream_t audio_input_stream;
	void *ain_handle;
	os_delayed_work spdifrx_cb_work;
	os_delayed_work i2srx_cb_work;
};
void audio_input_view_init(void);
void audio_input_view_deinit(void);
struct audio_input_app_t *audio_input_get_app(void);
int audio_input_get_status(void);
void audio_input_start_play(void);
void audio_input_stop_play(void);
void audio_input_input_event_proc(struct app_msg *msg);
void audio_input_tts_event_proc(struct app_msg *msg);
u32_t audio_input_get_audio_stream_type(char *app_name);
void audio_input_bt_event_proc(struct app_msg *msg);
void audio_input_store_play_state(void);
void audio_input_show_play_status(bool status);
void audio_input_view_clear_screen(void);
void audio_input_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg);
int audio_input_spdifrx_event_cb(void *cb_data, u32_t cmd, void *param);
#endif  /* _AUDIO_INPUT_APP_H */

