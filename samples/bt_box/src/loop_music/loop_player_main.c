/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief loop play app main.
 */

#include "loop_player.h"
#include "tts_manager.h"
#include "fs_manager.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif


static struct loop_play_app_t *p_loop_player = NULL;

int loopplay_get_status(void)
{
	if (!p_loop_player)
		return LOOP_STATUS_NULL;
	return p_loop_player->music_status;
}

static void loopplay_disk_check_timer(void)
{
	p_loop_player->is_disk_check = 1;
	thread_timer_start(&p_loop_player->monitor_timer, 100, 0);
}

static int _loopplay_init(void)
{
	if (p_loop_player)
		return 0;

	p_loop_player = app_mem_malloc(sizeof(struct loop_play_app_t));
	if (!p_loop_player) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

	memset(p_loop_player, 0, sizeof(struct loop_play_app_t));

	bt_manager_set_stream_type(AUDIO_STREAM_LOCAL_MUSIC);

	/* don't play tts */
	tts_manager_lock();

	loopplay_view_init();
	loopplay_thread_timer_init(p_loop_player);

	SYS_LOG_INF("init ok\n");
	return 0;
}
static void _loopplay_exit(void)
{
	if (!p_loop_player)
		goto exit;

#ifdef CONFIG_FS_MANAGER
	if (memcmp(p_loop_player->cur_dir, "SD:", strlen("SD:")) == 0)
		fs_manager_sdcard_exit_high_speed();
#endif

	loopplay_bp_info_save(p_loop_player);

	loopplay_stop_play(p_loop_player, false);

	loopplay_exit_iterator();

	loopplay_view_deinit();

	/* tts resume */
	tts_manager_unlock();

	app_mem_free(p_loop_player);

	p_loop_player = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(APP_ID_LOOP_PLAYER);

	SYS_LOG_INF("exit ok\n");
}

static void loopplay_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_loopplay_init()) {
		SYS_LOG_ERR("init failed");
		_loopplay_exit();
		return;
	} else {
		loopplay_disk_check_timer();
	}

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				loopplay_input_event_proc(&msg);
				break;

			case MSG_LOOPPLAY_EVENT:
				loopplay_event_proc(&msg);
				break;

			case MSG_EXIT_APP:
				_loopplay_exit();
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
struct loop_play_app_t *loopplay_get_app(void)
{
	return p_loop_player;
}

APP_DEFINE(loop_player, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	   loopplay_main_loop, NULL);
