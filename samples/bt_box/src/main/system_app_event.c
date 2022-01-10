/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system app event.
 */

#include <mem_manager.h>
#include <msg_manager.h>
#include <sys_event.h>
#include <bt_manager.h>
#include "app_ui.h"
#include "system_app.h"
#ifdef CONFIG_USB_MASS_STORAGE
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#ifdef CONFIG_FS_MANAGER
#include <fs_manager.h>
#endif
#include <usb/class/usb_msc.h>
#endif

#ifdef CONFIG_USB_UART_CONSOLE
#include <drivers/console/uart_usb.h>
#endif

extern void recorder_service_start_stop(u8_t is_stop);
extern void recorder_service_check_disk(const char *disk);
extern void alarm_entry_exit(u8_t force_exit);
extern int alarm_input_event_proc(struct app_msg *msg);
#ifdef CONFIG_GMA_APP
extern uint8_t gma_dev_sent_record_start_stop(uint8_t cmd);
#endif
void system_key_event_handle(struct app_msg *msg)
{
	SYS_LOG_INF("msg->cmd 0x%x\n", msg->cmd);
	switch (msg->cmd) {
	case MSG_KEY_POWER_OFF:
	#if CONFIG_RECORD_SERVICE
		recorder_service_start_stop(0x01);
	#endif
		sys_event_notify(SYS_EVENT_POWER_OFF);
		break;
	case MSG_FACTORY_DEFAULT:
		sys_event_notify(SYS_EVENT_FACTORY_DEFAULT);
		system_restore_factory_config();
		break;
	case MSG_SWITCH_APP:
		/*模式切换*/
		app_switch((char *)msg->ptr, APP_SWITCH_NEXT, false);
		break;
	case MSG_ENTER_PAIRING_MODE:
		/* Design for entering pairing mode, wait to connect by phone */
		break;
#ifdef CONFIG_TWS
	case MSG_BT_PLAY_TWS_PAIR:
		bt_manager_tws_wait_pair();
		break;
	case MSG_BT_PLAY_DISCONNECT_TWS_PAIR:
		bt_manager_tws_disconnect_and_wait_pair();
		break;
	case MSG_BT_TWS_SWITCH_MODE:
		bt_manager_tws_channel_mode_switch();
		break;
#endif
#ifdef CONFIG_BT_MANAGER
	case MSG_BT_PLAY_CLEAR_LIST:
		bt_manager_clear_list(BTSRV_DEVICE_ALL);
		break;
	case MSG_BT_SIRI_STOP:
		bt_manager_hfp_stop_siri();
		break;
	case MSG_BT_SIRI_START:
		bt_manager_hfp_start_siri();
		break;
	case MSG_BT_CALL_LAST_NO:
		bt_manager_hfp_dial_last_number();
		break;
#endif
#ifdef CONFIG_BT_HID
	case MSG_BT_HID_START:
		bt_manager_hid_take_photo();
		/* bt_manager_hid_key_func(KEY_FUNC_HID_CUSTOM_KEY); */
		break;
#endif
	case MSG_RECORDER_START_STOP:
	#ifdef CONFIG_RECORD_SERVICE
		recorder_service_start_stop(msg->reserve);
	#endif
		break;
#ifdef CONFIG_GMA_APP
	case MSG_GMA_RECORDER_START:
		gma_dev_sent_record_start_stop(0);
#endif
		break;
#ifdef CONFIG_ALARM_APP
	case MSG_ALARM_ENTRY_EXIT:
		alarm_entry_exit(msg->reserve);
		break;
#endif

	default:
	#ifdef CONFIG_ALARM_APP
		if (alarm_input_event_proc(msg))
			SYS_LOG_ERR("error: 0x%x!\n", msg->cmd);
	#endif
		break;
	}
}
#ifdef CONFIG_HOTPLUG_MANAGER
#if (defined(CONFIG_LINE_IN_APP) || defined(CONFIG_USOUND_APP) || defined(CONFIG_RECORD_APP) || defined(CONFIG_LCMUSIC_APP) || defined(CONFIG_LOOP_PLAYER_APP))

static void _system_remove_app(const char *target_app_name)
{
	bool need_switch = true;
	void *fg_app = app_manager_get_current_app();

	if (fg_app && !strcmp(fg_app, target_app_name)) {
		if (!strcmp(target_app_name, APP_ID_RECORDER) || !strcmp(target_app_name, APP_ID_LOOP_PLAYER)) {
			/*USB has a higher priority than card,in record app and loop player app*/
			/*plug out USB must switch app*/
			/*plug out card,not need to switch app,when USB plug in*/
			/*plug out card,must switch app,when USB not plug in*/
			if ((hotplug_manager_get_state(HOTPLUG_SDCARD) == HOTPLUG_IN)
				|| ((hotplug_manager_get_state(HOTPLUG_USB_HOST) != HOTPLUG_IN)
				&& (hotplug_manager_get_state(HOTPLUG_SDCARD) != HOTPLUG_IN))) {
				app_switch(APP_ID_DEFAULT, APP_SWITCH_PREV, false);
			}
		} else {
			app_switch(APP_ID_DEFAULT, APP_SWITCH_PREV, false);
		}
	}

	if (!strcmp(target_app_name, APP_ID_RECORDER) || !strcmp(target_app_name, APP_ID_LOOP_PLAYER)) {
		if (hotplug_manager_get_state(HOTPLUG_USB_HOST) == HOTPLUG_IN
			|| hotplug_manager_get_state(HOTPLUG_SDCARD) == HOTPLUG_IN) {
			need_switch = false;
		}
	}

	if (need_switch) {
		app_switch_remove_app(target_app_name);
	}
}
#endif
#endif //CONFIG_HOTPLUG_MANAGER

void system_hotplug_event_handle(struct app_msg *msg)
{

#ifdef CONFIG_HOTPLUG_MANAGER
	switch (msg->cmd) {
	case HOTPLUG_LINEIN:
	#ifdef CONFIG_LINE_IN_APP
		if (msg->value == HOTPLUG_IN) {
			app_switch_add_app(APP_ID_LINEIN);
			app_switch(APP_ID_LINEIN, APP_SWITCH_NEXT, false);
		} else if (msg->value == HOTPLUG_OUT) {
			_system_remove_app(APP_ID_LINEIN);
		}
	#endif
		break;
	case HOTPLUG_SDCARD:
		if (msg->value == HOTPLUG_IN) {
	#ifndef CONFIG_SOUNDBAR_LCMUSIC
		#ifdef CONFIG_LCMUSIC_APP
			app_switch_add_app(APP_ID_SD_MPLAYER);
		#endif
	#endif
		#ifdef CONFIG_LOOP_PLAYER_APP
			app_switch_add_app(APP_ID_LOOP_PLAYER);
		#endif
		#ifdef CONFIG_RECORD_APP
			app_switch_add_app(APP_ID_RECORDER);
		#endif
	#ifndef CONFIG_SOUNDBAR_LCMUSIC
		#ifdef CONFIG_LCMUSIC_APP
		#ifdef CONFIG_LOOP_PLAYER_APP
			/* don't switch app if current app is loop player */
			if (strcmp(app_manager_get_current_app(), APP_ID_LOOP_PLAYER) != 0)
		#endif
			app_switch(APP_ID_SD_MPLAYER, APP_SWITCH_NEXT, false);
		#else
		#ifdef CONFIG_LOOP_PLAYER_APP
			/* switch to loop player if not define lcmusic */
			app_switch(APP_ID_LOOP_PLAYER, APP_SWITCH_NEXT, false);
		#endif
		#endif
	#endif
		} else if (msg->value == HOTPLUG_OUT) {
		#if CONFIG_RECORD_SERVICE
			recorder_service_check_disk("SD:");
		#endif
		#ifdef CONFIG_RECORD_APP
			_system_remove_app(APP_ID_RECORDER);
		#endif
	#ifndef CONFIG_SOUNDBAR_LCMUSIC
		#ifdef CONFIG_LCMUSIC_APP
			_system_remove_app(APP_ID_SD_MPLAYER);
		#endif
	#endif
		#ifdef CONFIG_LOOP_PLAYER_APP
			_system_remove_app(APP_ID_LOOP_PLAYER);
		#endif
		}
		break;
	case HOTPLUG_USB_DEVICE:
		if (msg->value == HOTPLUG_IN) {
#ifdef CONFIG_USB_UART_CONSOLE
			if (uart_usb_is_enabled()) {
				uart_usb_init();
			}
#endif

#ifdef CONFIG_USOUND_APP
			app_switch_add_app(APP_ID_USOUND);
		#ifdef CONFIG_USB_UART_CONSOLE
			if (!uart_usb_is_enabled())
		#endif
			{
				//if (system_boot_time() > 5000) {
					app_switch(APP_ID_USOUND, APP_SWITCH_NEXT, false);
				#ifdef CONFIG_CASCADE
					if (bt_manager_get_connected_dev_num() == 0) {
						bt_manager_tws_disconnect_and_wait_pair();
					}
				#endif
				//}
			}
#endif
		} else if (msg->value == HOTPLUG_OUT) {
#ifdef CONFIG_USB_UART_CONSOLE
			if (uart_usb_is_enabled()) {
				uart_usb_uninit();
			}
#endif

#ifdef CONFIG_CASCADE
			if (!strcmp(APP_ID_USOUND, app_manager_get_current_app()))
				bt_manager_tws_disconnect();
#endif

#ifdef CONFIG_USOUND_APP
			_system_remove_app(APP_ID_USOUND);

#endif
		}

		break;
	case HOTPLUG_USB_HOST:
		if (msg->value == HOTPLUG_IN) {
	#ifndef CONFIG_SOUNDBAR_LCMUSIC
		#ifdef CONFIG_LCMUSIC_APP
			app_switch_add_app(APP_ID_UHOST_MPLAYER);
		#endif
	#endif
		#ifdef CONFIG_LOOP_PLAYER_APP
			app_switch_add_app(APP_ID_LOOP_PLAYER);
		#endif
		#ifdef CONFIG_RECORD_APP
			app_switch_add_app(APP_ID_RECORDER);
		#endif
	#ifndef CONFIG_SOUNDBAR_LCMUSIC
		#ifdef CONFIG_LCMUSIC_APP
			if (system_boot_time() > 5000 && system_wakeup_time() > 5000) {
				app_switch(APP_ID_UHOST_MPLAYER, APP_SWITCH_NEXT, false);
			}
		#else
		#ifdef CONFIG_LOOP_PLAYER_APP
			/* switch to loop player if not define lcmusic */
			app_switch(APP_ID_LOOP_PLAYER, APP_SWITCH_NEXT, false);
		#endif
		#endif
	#endif
		} else if (msg->value == HOTPLUG_OUT) {
		#if CONFIG_RECORD_SERVICE
			recorder_service_check_disk("USB:");
		#endif
		#ifdef CONFIG_RECORD_APP
			_system_remove_app(APP_ID_RECORDER);
		#endif
	#ifndef CONFIG_SOUNDBAR_LCMUSIC
		#ifdef CONFIG_LCMUSIC_APP
			if(system_wakeup_time() > 1000) {
				_system_remove_app(APP_ID_UHOST_MPLAYER);
			} else {
				app_switch_remove_app(APP_ID_UHOST_MPLAYER);
			}
		#endif
	#endif
		#ifdef CONFIG_LOOP_PLAYER_APP
			_system_remove_app(APP_ID_LOOP_PLAYER);
		#endif
		}
		break;
	default:
		SYS_LOG_ERR("error: 0x%x!\n", msg->cmd);
		break;
	}
#endif
}

