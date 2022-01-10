/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app input
 */

#include <misc/util.h>
#include <soc.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <sys_monitor.h>
#include "sys_wakelock.h"
#include "system_app.h"

#ifdef CONFIG_ESD_MANAGER
#include <esd_manager.h>
#endif

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifdef CONFIG_INPUT_MANAGER
void _system_key_event_cb(u32_t key_value, uint16_t type)
{
	if ((key_value & KEY_TYPE_DOUBLE_CLICK) == KEY_TYPE_DOUBLE_CLICK
		|| (key_value & KEY_TYPE_TRIPLE_CLICK) == KEY_TYPE_TRIPLE_CLICK
		|| (key_value & KEY_TYPE_SHORT_UP) == KEY_TYPE_SHORT_UP
		|| (key_value & KEY_TYPE_LONG_DOWN) == KEY_TYPE_LONG_DOWN) {
		/** need process key tone */
	#ifdef CONFIG_PLAY_KEYTONE
		//key_tone_play();
	#endif
	}

	/*filter onoff key for power on */
	if (((key_value & ~KEY_TYPE_ALL) == KEY_POWER)
		&& (k_uptime_get() < 1800))	{
		input_manager_filter_key_itself();
	}

}

void system_sr_input_event_handle(void *value)
{
	void *fg_app = app_manager_get_current_app();

	if (fg_app) {
		struct app_msg  msg = {0};

		msg.type = MSG_SR_INPUT;
		msg.value = *(int *)value;
		send_async_msg(fg_app, &msg);
	}
}

void system_input_event_handle(u32_t key_event)
{
	/**input event means esd proecess finished*/
#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
	#ifdef CONFIG_PLAYTTS
		tts_manager_unlock();
	#endif
		esd_manager_reset_finished();
	}
#endif

	sys_wake_lock(WAKELOCK_INPUT);

#ifdef CONFIG_PLAYTTS
	if ((key_event & KEY_TYPE_DOUBLE_CLICK) == KEY_TYPE_DOUBLE_CLICK
		|| (key_event & KEY_TYPE_TRIPLE_CLICK) == KEY_TYPE_TRIPLE_CLICK
		|| (key_event & KEY_TYPE_SHORT_UP) == KEY_TYPE_SHORT_UP
		|| (key_event & KEY_TYPE_LONG_DOWN) == KEY_TYPE_LONG_DOWN) {
		tts_manager_stop(NULL); 
	}
#endif

	SYS_LOG_INF(" 0x%x\n", key_event);

	/**drop fisrt key event when resume*/
	if (system_wakeup_time() > 600) {
	#ifdef CONFIG_UI_MANAGER
		ui_manager_dispatch_key_event(key_event);
	#endif
	} else {
		SYS_LOG_INF("drop key workup %d \n",system_wakeup_time());
	}

	sys_wake_unlock(WAKELOCK_INPUT);
}
#endif

void system_input_handle_init(void)
{
#ifdef CONFIG_INPUT_MANAGER
	input_manager_init(_system_key_event_cb);
	/** input init is locked ,so we must unlock*/
	input_manager_unlock();
#endif
}
