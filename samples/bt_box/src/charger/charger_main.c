/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app main
 */
#include <mem_manager.h>
#include <msg_manager.h>
#include <fw_version.h>
#include <sys_event.h>
#include "app_switch.h"
#include "app_ui.h"
#include <bt_manager.h>
#include <hotplug_manager.h>
#include <input_manager.h>
#include <thread_timer.h>
#include <stream.h>
#include <property_manager.h>
#include <usb/usb_device.h>
#include <usb/class/usb_msc.h>
#include <soc.h>
#include <ui_manager.h>
#include <sys_wakelock.h>
#include <stub_hal.h>
#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif

#include "charger.h"

int charger_mode_check(void)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;

	if (!sys_pm_get_power_5v_status()) {
		return 0;
	}

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(WAKELOCK_USB_DEVICE);
#endif

	charger_view_init();

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.type) {
            case MSG_KEY_INPUT:
				terminaltion = true;
				break;
		#ifdef CONFIG_UI_MANAGER
			case MSG_UI_EVENT:
				ui_message_dispatch(msg.sender, msg.cmd, msg.value);
				break;
		#endif
			case MSG_HOTPLUG_EVENT:
				if (msg.value == HOTPLUG_OUT) {
					switch (msg.cmd) {
						case HOTPLUG_CHARGER:
							terminaltion = true;
							sys_pm_poweroff();
							break;
						default :
							break;
					}
				}
				break;
			default:
				SYS_LOG_ERR("error type: 0x%x! \n", msg.type);
				continue;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}

	charger_view_deinit();

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(WAKELOCK_USB_DEVICE);
#endif

	return 0;

}
