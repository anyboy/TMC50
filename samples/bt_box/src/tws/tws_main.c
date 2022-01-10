/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tws app main.
 */

#include "tws.h"
#include <dvfs.h>
#include "tts_manager.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

static struct tws_app_t *p_tws_app;

static int _tws_init(void)
{
	int ret = 0;

	p_tws_app = app_mem_malloc(sizeof(struct tws_app_t));
	if (!p_tws_app) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}

#ifdef CONFIG_BT_BLE
	bt_manager_halt_ble();
#endif

	tws_view_init();

	bt_manager_stream_enable(STREAM_TYPE_A2DP, true);

	SYS_LOG_INF("init ok\n");

	if (system_check_low_latencey_mode()) {
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tws");
	#endif
	}

	return ret;
}

static void _tws_exit(void)
{
	if (!p_tws_app) {
		goto exit;
	}

	bt_manager_stream_enable(STREAM_TYPE_A2DP, false);

	tws_a2dp_stop_play();

	tws_view_deinit();

	app_mem_free(p_tws_app);
	p_tws_app = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

#ifdef CONFIG_BT_BLE
	bt_manager_resume_ble();
#endif

exit:
	app_manager_thread_exit(APP_ID_TWS);

	if (system_check_low_latencey_mode()) {
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tws");
	#endif
	}

	SYS_LOG_INF("exit ok\n");
}

static void tws_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;

	if (_tws_init()) {
		SYS_LOG_ERR(" init failed");
		_tws_exit();
		return;
	}

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_EXIT_APP:
				_tws_exit();
				terminaltion = true;
				break;
			case MSG_BT_EVENT:
				tws_bt_event_proc(&msg);
				break;
			case MSG_INPUT_EVENT:
				tws_input_event_proc(&msg);
				break;
			case MSG_TTS_EVENT:
				tws_tts_event_proc(&msg);
				break;
			default:
				SYS_LOG_ERR(" 0x%x!\n", msg.type);
				continue;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}
}

struct tws_app_t *tws_get_app(void)
{
	return p_tws_app;
}

APP_DEFINE(tws, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   tws_main_loop, NULL);
