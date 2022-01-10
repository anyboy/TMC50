/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call main.
 */

#include "btcall.h"
#include "tts_manager.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
static struct btcall_app_t *p_btcall_app;

static int _btcall_init(void)
{
	int ret = 0;

	p_btcall_app = app_mem_malloc(sizeof(struct btcall_app_t));
	if (!p_btcall_app) {
	    SYS_LOG_ERR("malloc fail!\n");
		ret = -ENOMEM;
		return ret;
	}

	bt_manager_set_stream_type(AUDIO_STREAM_VOICE);

	btcall_view_init();

	btcall_ring_manager_init();

	SYS_LOG_INF("finished\n");
	return ret;
}

static void _btcall_exit(void)
{
	if (!p_btcall_app)
   		goto exit;

	bt_call_stop_play();

	btcall_ring_manager_deinit();

	btcall_view_deinit();

    app_mem_free(p_btcall_app);
	p_btcall_app = NULL;
	
#ifdef CONFIG_PROPERTY
    property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(APP_ID_BTCALL);
	SYS_LOG_INF(" finished \n");
}


static void btcall_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;

	if (_btcall_init()) {
		SYS_LOG_ERR(" init failed");
		_btcall_exit();
		return;
	}

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n",msg.type, msg.value);
			switch (msg.type) {
				case MSG_EXIT_APP:
					_btcall_exit();
					terminaltion = true;
					break;
				case MSG_BT_EVENT:
				    btcall_bt_event_proc(&msg);
					break;
				case MSG_INPUT_EVENT:
				    btcall_input_event_proc(&msg);
					break;
				case MSG_TTS_EVENT:
					btcall_tts_event_proc(&msg);
					break;
				default:
					SYS_LOG_ERR("error: 0x%x \n", msg.type);
					continue;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}
}

struct btcall_app_t *btcall_get_app(void)
{
	return p_btcall_app;
}

APP_DEFINE(btcall, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   btcall_main_loop, NULL);
