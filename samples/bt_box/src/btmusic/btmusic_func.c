/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music function.
 */

#include "btmusic.h"

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

void bt_music_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	bt_manager_avrcp_play();
	bt_manager_a2dp_check_state();
}

#ifdef CONFIG_ESD_MANAGER
void bt_music_esd_restore(void)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (esd_manager_check_esd()) {
		u8_t play_state = 0;

		esd_manager_restore_scene(TAG_PLAY_STATE, &play_state, 1);

		if (play_state) {
			if (thread_timer_is_running(&btmusic->monitor_timer)) {
				thread_timer_stop(&btmusic->monitor_timer);
			}
			thread_timer_init(&btmusic->monitor_timer, bt_music_delay_resume, NULL);
			thread_timer_start(&btmusic->monitor_timer, 500, 0);
		}
	#ifdef CONFIG_PLAYTTS
		tts_manager_unlock();
	#endif
		esd_manager_reset_finished();
		return;
	}
}
#endif
