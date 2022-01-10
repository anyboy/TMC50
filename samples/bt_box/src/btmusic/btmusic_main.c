/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music app main.
 */

#include "btmusic.h"
#include "tts_manager.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

static struct btmusic_app_t *p_btmusic_app;

#ifdef CONFIG_BT_MUSIC_FREQPOINT_ENERGY_DEMO
static void btmusic_display_freqpoint_energy(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	media_freqpoint_energy_info_t temp_info;
	if (bt_music_a2dp_get_freqpoint_energy(&temp_info) == 0) {
		SYS_LOG_INF("fp energy:%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
			temp_info.values[0],
			temp_info.values[1],
			temp_info.values[2],
			temp_info.values[3],
			temp_info.values[4],
			temp_info.values[5],
			temp_info.values[6],
			temp_info.values[7],
			temp_info.values[8]);
	}
}
#endif

void bt_music_delay_start(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	if (p_btmusic_app->player)
		return;

	bt_manager_a2dp_check_state();
}

static int _btmusic_init(void)
{
	int ret = 0;

	p_btmusic_app = app_mem_malloc(sizeof(struct btmusic_app_t));
	if (!p_btmusic_app) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}

	btmusic_view_init();

	bt_manager_stream_enable(STREAM_TYPE_A2DP, true);

	bt_manager_set_stream_type(AUDIO_STREAM_MUSIC);

#ifdef CONFIG_BT_MUSIC_FREQPOINT_ENERGY_DEMO
	thread_timer_init(&p_btmusic_app->monitor_timer, btmusic_display_freqpoint_energy, NULL);
	thread_timer_start(&p_btmusic_app->monitor_timer, 100, 100);
#endif

	thread_timer_init(&p_btmusic_app->play_timer, bt_music_delay_start, NULL);
	thread_timer_start(&p_btmusic_app->play_timer, 800, 0);

	SYS_LOG_INF("init finished\n");

#if 0
#ifndef CONFIG_TWS_BACKGROUND_BT
	bt_manager_resume_phone();
#else
	if (system_check_low_latencey_mode()) {
		bt_manager_resume_phone();
	}
#endif
#endif

	return ret;
}

static void _btmusic_exit(void)
{
	if (!p_btmusic_app)
		goto exit;

	if (thread_timer_is_running(&p_btmusic_app->monitor_timer))
		thread_timer_stop(&p_btmusic_app->monitor_timer);

	if (thread_timer_is_running(&p_btmusic_app->play_timer))
		thread_timer_stop(&p_btmusic_app->play_timer);

	bt_manager_stream_enable(STREAM_TYPE_A2DP, false);

	bt_music_a2dp_stop_play();

	btmusic_view_deinit();

	app_mem_free(p_btmusic_app);
	p_btmusic_app = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(APP_ID_BTMUSIC);

	SYS_LOG_INF("exit finished\n");
}

static void btmusic_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;

	if (_btmusic_init()) {
		SYS_LOG_ERR("init failed");
		_btmusic_exit();
		return;
	}

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_EXIT_APP:
				_btmusic_exit();
				terminaltion = true;
				break;
			case MSG_BT_EVENT:
				btmusic_bt_event_proc(&msg);
				break;
			case MSG_INPUT_EVENT:
				btmusic_input_event_proc(&msg);
				break;
			case MSG_TTS_EVENT:
				btmusic_tts_event_proc(&msg);
				break;
			default:
				SYS_LOG_ERR("error: 0x%x!\n", msg.type);
				continue;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}
}

struct btmusic_app_t *btmusic_get_app(void)
{
	return p_btmusic_app;
}

APP_DEFINE(btmusic, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
	   btmusic_main_loop, NULL);
