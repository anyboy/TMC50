/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input app main.
 */

#include "audio_input.h"
#include "tts_manager.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
#include <audio_hal.h>

static struct audio_input_app_t *p_audio_input = NULL;

static u8_t linein_init_state = AUDIO_INPUT_STATUS_PLAYING;
static u8_t spdif_in_init_state = AUDIO_INPUT_STATUS_PLAYING;
static u8_t mic_in_init_state = AUDIO_INPUT_STATUS_PLAYING;
static u8_t i2srx_in_init_state = AUDIO_INPUT_STATUS_PLAYING;

static u8_t temp_sample_rate = 0;

int audio_input_spdifrx_event_cb(void *cb_data, u32_t cmd, void *param)
{
	if (!p_audio_input)
		return 0;

	if (cmd == SPDIFRX_SRD_FS_CHANGE) {
		temp_sample_rate = *(u8_t *)param;
		os_delayed_work_submit(&p_audio_input->spdifrx_cb_work, 2);
		p_audio_input->spdifrx_srd_event = SPDIFRE_SAMPLE_RATE_CHANGE;
#ifndef CONFIG_SOUNDBAR_AUDIOIN_STAY
	} else if (cmd == SPDIFRX_SRD_TIMEOUT) {
		p_audio_input->spdifrx_srd_event = SPDIFRE_TIME_OUT;
		os_delayed_work_submit(&p_audio_input->spdifrx_cb_work, 2);
#endif
	}
	SYS_LOG_INF("cmd=%d, param=%d\n", cmd,  *(u8_t *)param);
	return 0;
}

int audio_input_i2srx_event_cb(void *cb_data, u32_t cmd, void *param)
{
	if (!p_audio_input)
		return 0;
	SYS_LOG_INF("cmd=%d, param=%d\n", cmd,  *(u8_t *)param);
	if (cmd == I2SRX_SRD_FS_CHANGE) {
		temp_sample_rate = *(u8_t *)param;
		os_delayed_work_submit(&p_audio_input->i2srx_cb_work, 2);
		p_audio_input->i2srx_srd_event = I2SRX_SAMPLE_RATE_CHANGE;
#ifndef CONFIG_SOUNDBAR_AUDIOIN_STAY
	} else if (cmd == SPDIFRX_SRD_TIMEOUT) {
		p_audio_input->i2srx_srd_event = I2SRX_TIME_OUT;
		os_delayed_work_submit(&p_audio_input->i2srx_cb_work, 2);
#endif
	}
	return 0;
}


static void _spdifrx_isr_event_callback_work(struct k_work *work)
{
	struct app_msg msg = { 0 };

	if (!p_audio_input)
		return;

	if (p_audio_input->ain_handle) {
		hal_ain_channel_close(p_audio_input->ain_handle);
		p_audio_input->ain_handle = NULL;
	}

	if (p_audio_input->spdifrx_srd_event == SPDIFRE_SAMPLE_RATE_CHANGE) {
		if (temp_sample_rate != p_audio_input->spdif_sample_rate || !p_audio_input->player) {
			p_audio_input->spdif_sample_rate = temp_sample_rate;
			msg.type = MSG_INPUT_EVENT;
			msg.cmd = MSG_AUDIO_INPUT_SAMPLE_RATE_CHANGE;
			send_async_msg(app_manager_get_current_app(), &msg);
		}
#ifndef CONFIG_SOUNDBAR_AUDIOIN_STAY
	} else if (p_audio_input->spdifrx_srd_event == SPDIFRE_TIME_OUT) {
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		msg.ptr = APP_ID_DEFAULT;
		send_async_msg(APP_ID_MAIN, &msg);
#endif
	}
	SYS_LOG_INF("\n");
}

static void _i2srx_isr_event_callback_work(struct k_work *work)
{
	struct app_msg msg = { 0 };

	if (!p_audio_input)
		return;
	if (p_audio_input->ain_handle) {
		hal_ain_channel_close(p_audio_input->ain_handle);
		p_audio_input->ain_handle = NULL;
	}

	if (p_audio_input->i2srx_srd_event == I2SRX_SAMPLE_RATE_CHANGE) {
		if (temp_sample_rate != p_audio_input->i2srx_sample_rate || !p_audio_input->player) {
			p_audio_input->i2srx_sample_rate = temp_sample_rate;
			msg.type = MSG_INPUT_EVENT;
			msg.cmd = MSG_AUDIO_INPUT_SAMPLE_RATE_CHANGE;
			send_async_msg(app_manager_get_current_app(), &msg);
		}
#ifndef CONFIG_SOUNDBAR_AUDIOIN_STAY
	} else if (p_audio_input->i2srx_srd_event == I2SRX_TIME_OUT) {
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		msg.ptr = APP_ID_DEFAULT;
		send_async_msg(APP_ID_MAIN, &msg);
#endif
	}
	SYS_LOG_INF("\n");
}


void audio_input_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	if (memcmp(app_manager_get_current_app(), APP_ID_SPDIF_IN, strlen(APP_ID_SPDIF_IN)) == 0) {
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "spdif");
#endif
		audio_in_init_param_t init_param = { 0 };

		init_param.channel_type = AUDIO_CHANNEL_SPDIFRX;
		p_audio_input->ain_handle = hal_ain_channel_open(&init_param);
	} else {
		audio_input_start_play();
	}
	p_audio_input->need_resume_play = 0;
}

static void _audio_input_restore_play_state(u8_t init_play_state)
{
#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		esd_manager_restore_scene(TAG_PLAY_STATE, &init_play_state, 1);
	}
#endif

	if (init_play_state == AUDIO_INPUT_STATUS_PLAYING) {
		p_audio_input->playing = 1;
		if (thread_timer_is_running(&p_audio_input->monitor_timer)) {
			thread_timer_stop(&p_audio_input->monitor_timer);
		}
		thread_timer_init(&p_audio_input->monitor_timer, audio_input_delay_resume, NULL);
		thread_timer_start(&p_audio_input->monitor_timer, 800, 0);
		audio_input_show_play_status(true);
	} else {
		p_audio_input->playing = 0;
		audio_input_show_play_status(false);
	}
	audio_input_store_play_state();

	SYS_LOG_INF("%d\n", init_play_state);
}

void audio_input_store_play_state(void)
{
	if (p_audio_input->playing) {
		if (memcmp(app_manager_get_current_app(), APP_ID_LINEIN, strlen(APP_ID_LINEIN)) == 0)
			linein_init_state = AUDIO_INPUT_STATUS_PLAYING;
		else if (memcmp(app_manager_get_current_app(), APP_ID_MIC_IN, strlen(APP_ID_MIC_IN)) == 0)
			mic_in_init_state = AUDIO_INPUT_STATUS_PLAYING;
		else if (memcmp(app_manager_get_current_app(), APP_ID_SPDIF_IN, strlen(APP_ID_SPDIF_IN)) == 0)
			spdif_in_init_state = AUDIO_INPUT_STATUS_PLAYING;
		else if (memcmp(app_manager_get_current_app(), APP_ID_I2SRX_IN, strlen(APP_ID_I2SRX_IN)) == 0)
			i2srx_in_init_state = AUDIO_INPUT_STATUS_PLAYING;
	} else {
	linein_init_state = AUDIO_INPUT_STATUS_PAUSED;
		if (memcmp(app_manager_get_current_app(), APP_ID_LINEIN, strlen(APP_ID_LINEIN)) == 0)
			linein_init_state = AUDIO_INPUT_STATUS_PAUSED;
		else if (memcmp(app_manager_get_current_app(), APP_ID_MIC_IN, strlen(APP_ID_MIC_IN)) == 0)
			mic_in_init_state = AUDIO_INPUT_STATUS_PAUSED;
		else if (memcmp(app_manager_get_current_app(), APP_ID_SPDIF_IN, strlen(APP_ID_SPDIF_IN)) == 0)
			spdif_in_init_state = AUDIO_INPUT_STATUS_PAUSED;
		else if (memcmp(app_manager_get_current_app(), APP_ID_I2SRX_IN, strlen(APP_ID_I2SRX_IN)) == 0)
			i2srx_in_init_state = AUDIO_INPUT_STATUS_PAUSED;
	}
}

#ifdef CONFIG_AUDIO_INPUT_ENERGY_DETECT_DEMO
void audio_input_energy_detect(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();
	if (audio_input->player && audio_input->playing) {
		energy_detect_info_t energy_detect_info;
		media_player_get_energy_detect(audio_input->player, &energy_detect_info);
		SYS_LOG_INF("energy:%d,%d,%d\n", energy_detect_info.energy_stat_average, \
			energy_detect_info.energy_stat_duration_ms, \
			energy_detect_info.energy_lowpower_duration_ms);
	}
}
#endif

static int _audio_input_init(void)
{
	if (p_audio_input)
		return 0;

	p_audio_input = app_mem_malloc(sizeof(struct audio_input_app_t));
	if (!p_audio_input) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}
#ifdef CONFIG_TWS
#ifndef CONFIG_TWS_BACKGROUND_BT
	bt_manager_halt_phone();
#else
	if (system_check_low_latencey_mode()) {
		bt_manager_halt_phone();
	}
#endif
#endif

	memset(p_audio_input, 0, sizeof(struct audio_input_app_t));

	audio_input_view_init();
	if (memcmp(app_manager_get_current_app(), APP_ID_SPDIF_IN, strlen(APP_ID_SPDIF_IN)) == 0) {
		hal_ain_set_contrl_callback(AUDIO_CHANNEL_SPDIFRX, audio_input_spdifrx_event_cb);
		os_delayed_work_init(&p_audio_input->spdifrx_cb_work, _spdifrx_isr_event_callback_work);
	}

	if (memcmp(app_manager_get_current_app(), APP_ID_I2SRX_IN, strlen(APP_ID_I2SRX_IN)) == 0) {
		hal_ain_set_contrl_callback(AUDIO_CHANNEL_I2SRX, audio_input_i2srx_event_cb);
		os_delayed_work_init(&p_audio_input->i2srx_cb_work, _i2srx_isr_event_callback_work);
	}

#ifdef CONFIG_AUDIO_INPUT_ENERGY_DETECT_DEMO
	thread_timer_init(&p_audio_input->energy_detect_timer, audio_input_energy_detect, NULL);
	thread_timer_start(&p_audio_input->energy_detect_timer, 1000, 1000);
#endif

	SYS_LOG_INF("ok\n");
	return 0;
}
void _audio_input_exit(void)
{
	if (!p_audio_input)
		goto exit;

#ifdef CONFIG_AUDIO_INPUT_ENERGY_DETECT_DEMO
	if (thread_timer_is_running(&p_audio_input->energy_detect_timer))
		thread_timer_stop(&p_audio_input->energy_detect_timer);
#endif

	audio_input_stop_play();

	if (p_audio_input->ain_handle) {
		hal_ain_channel_close(p_audio_input->ain_handle);
		p_audio_input->ain_handle = NULL;
		os_delayed_work_cancel(&p_audio_input->spdifrx_cb_work);
	}

	audio_input_view_deinit();

	app_mem_free(p_audio_input);

	p_audio_input = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif


exit:
	app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF("ok\n");
}

static void _audio_input_main_prc(u8_t init_play_state)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_audio_input_init()) {
		SYS_LOG_ERR("init failed");
		_audio_input_exit();
		return;
	}
	_audio_input_restore_play_state(init_play_state);

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				audio_input_input_event_proc(&msg);
				break;

			case MSG_TTS_EVENT:
				audio_input_tts_event_proc(&msg);
				break;
			case MSG_BT_EVENT:
				audio_input_bt_event_proc(&msg);
				break;
			case MSG_EXIT_APP:
				_audio_input_exit();
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
struct audio_input_app_t *audio_input_get_app(void)
{
	return p_audio_input;
}

static void audio_input_linein_main_loop(void *parama1, void *parama2, void *parama3)
{
	u8_t init_play_state = *(u8_t *) parama1;

	SYS_LOG_INF("Enter\n");

	sys_event_notify(SYS_EVENT_ENTER_LINEIN);

	bt_manager_set_stream_type(AUDIO_STREAM_LINEIN);

	_audio_input_main_prc(init_play_state);

	SYS_LOG_INF("Exit\n");
}

APP_DEFINE(linein, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, &linein_init_state, NULL, NULL,
	   audio_input_linein_main_loop, NULL);

static void audio_input_spdif_in_main_loop(void *parama1, void *parama2, void *parama3)
{
	u8_t init_play_state = *(u8_t *) parama1;

	SYS_LOG_INF("Enter\n");
	sys_event_notify(SYS_EVENT_ENTER_SPDIF_IN);

	bt_manager_set_stream_type(AUDIO_STREAM_SPDIF_IN);

	_audio_input_main_prc(init_play_state);
	SYS_LOG_INF("Exit\n");
}
APP_DEFINE(spdif_in, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, &spdif_in_init_state, NULL, NULL,
	   audio_input_spdif_in_main_loop, NULL);

static void audio_input_i2srx_in_main_loop(void *parama1, void *parama2, void *parama3)
{
   u8_t init_play_state = *(u8_t *) parama1;

   SYS_LOG_INF("Enter\n");
   sys_event_notify(SYS_EVENT_ENTER_I2SRX_IN);

   bt_manager_set_stream_type(AUDIO_STREAM_I2SRX_IN);

   _audio_input_main_prc(init_play_state);
   SYS_LOG_INF("Exit\n");
}
APP_DEFINE(i2srx_in, share_stack_area, sizeof(share_stack_area),
	  CONFIG_APP_PRIORITY, FOREGROUND_APP, &spdif_in_init_state, NULL, NULL,
	  audio_input_i2srx_in_main_loop, NULL);

static void audio_input_mic_in_main_loop(void *parama1, void *parama2, void *parama3)
{
	u8_t init_play_state = *(u8_t *) parama1;

	SYS_LOG_INF("Enter\n");
	sys_event_notify(SYS_EVENT_ENTER_MIC_IN);

	bt_manager_set_stream_type(AUDIO_STREAM_MIC_IN);

	_audio_input_main_prc(init_play_state);
	SYS_LOG_INF("Exit\n");
}
APP_DEFINE(mic_in, share_stack_area, sizeof(share_stack_area),
	CONFIG_APP_PRIORITY, FOREGROUND_APP, &mic_in_init_state, NULL, NULL,
	audio_input_mic_in_main_loop, NULL);

