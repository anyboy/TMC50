/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief fm app main.
 */

#include "fm.h"
#include "tts_manager.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

static struct fm_app_t *p_fm = NULL;

void _fm_delay_start(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	fm_delay_start_play(p_fm);
}


static void _fm_restore_play_state(u8_t init_play_state)
{
#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		esd_manager_restore_scene(TAG_PLAY_STATE, &init_play_state, 1);
	}
#endif

	if (init_play_state == FM_STATUS_PLAYING) {
		if (thread_timer_is_running(&p_fm->monitor_timer)) {
			thread_timer_stop(&p_fm->monitor_timer);
		}
		thread_timer_init(&p_fm->monitor_timer, _fm_delay_start, NULL);
		thread_timer_start(&p_fm->monitor_timer, 800, 0);
	} else if (init_play_state == FM_STATUS_PAUSED) {
		p_fm->playing = 0;
	}

	SYS_LOG_INF(" %d ", init_play_state);
}

static void _fm_store_play_state(void)
{

}


void _fm_exit(void)
{
	if (!p_fm)
		goto exit;

	_fm_store_play_state();

	fm_radio_station_save(p_fm);

	fm_stop_play();

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(true);
#endif

	fm_rx_deinit();

	fm_view_deinit();

	app_mem_free(p_fm);

	p_fm = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF("exit ok\n");
}

static int _fm_init(void)
{

	if (p_fm)
		return 0;

	p_fm = app_mem_malloc(sizeof(struct fm_app_t));
	if (!p_fm) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

#ifdef CONFIG_TWS
#ifndef CONFIG_TWS_BACKGROUND_BT
	bt_manager_halt_phone();
#else
	if (system_check_low_latencey_mode()) {
		if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
			bt_manager_halt_phone();
		}
	}
#endif
#endif

	bt_manager_set_stream_type(AUDIO_STREAM_FM);

	memset(p_fm, 0, sizeof(struct fm_app_t));

	if (fm_function_init() != 0) {
		SYS_LOG_INF("err\n");
		return 1;
	}

	fm_view_init();

	fm_radio_station_load(p_fm);

	sys_event_notify(SYS_EVENT_ENTER_FM);

	SYS_LOG_INF("init ok\n");
	return 0;
}

static void _fm_main_loop(void *parama1, void *parama2, void *parama3)
{
	u8_t init_play_state = FM_STATUS_PLAYING;
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_fm_init()) {
		SYS_LOG_ERR("init failed");
		_fm_exit();
		return;
	}

	_fm_restore_play_state(init_play_state);

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d cmd %d\n", msg.type, msg.value, msg.cmd);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				fm_input_event_proc(&msg);
				break;

			case MSG_TTS_EVENT:
				fm_tts_event_proc(&msg);
				break;

			case MSG_BT_EVENT:
				fm_bt_event_proc(&msg);
				break;

			case MSG_EXIT_APP:
				_fm_exit();
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

struct fm_app_t *fm_get_app(void)
{
	return p_fm;
}

APP_DEFINE(fm, share_stack_area, sizeof(share_stack_area),
	CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	_fm_main_loop, NULL);

