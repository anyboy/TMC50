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
#include "system_app.h"
#include <dvfs.h>
#include "app_ui.h"
#include <bt_manager.h>
#include <stream.h>
#include <property_manager.h>
#include "soc_watchdog.h"

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_OTA_UPGRADE
#include "ota_app.h"
#endif

#ifdef CONFIG_AGING_PLAYBACK
#include <app_aging_playback.h>
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif

extern void system_library_version_dump(void);
extern void trace_init(void);
extern int trace_dma_print_set(unsigned int dma_enable);
extern int card_reader_mode_check(void);
extern int charger_mode_check(void);
extern bool alarm_wakeup_source_check(void);
extern void recorder_service_set_media_notify(void);
#ifdef CONFIG_GMA_APP
extern int ais_gma_sppble_init(void);
#endif
#ifdef CONFIG_TEST_BLE_NOTIFY
extern void ble_test_init(void);
#endif

static bool att_enter_bqb_flag = false;

static void main_freq_init(void)
{
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	u32_t current_level = dvfs_get_current_level();

	dvfs_unset_level(current_level, "init");
#ifdef CONFIG_MUSIC_DRC_EFFECT
	dvfs_set_level(DVFS_LEVEL_NORMAL, "init");
#else
#ifdef CONFIG_MUSIC_EFFECT
	dvfs_set_level(DVFS_LEVEL_NORMAL_NO_DRC, "init");
#else
	dvfs_set_level(DVFS_LEVEL_NORMAL_NO_EFX, "init");
#endif
#endif
#endif
}

void main_msg_proc(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;

	if (receive_msg(&msg, thread_timer_next_timeout())) {

		switch (msg.type) {
		case MSG_SYS_EVENT:
			sys_event_process(msg.cmd);
			break;

		#ifdef CONFIG_UI_MANAGER
		case MSG_UI_EVENT:
			ui_message_dispatch(msg.sender, msg.cmd, msg.value);
			break;
		#endif
		#ifdef CONFIG_INPUT_MANAGER
		case MSG_SR_INPUT:
			system_sr_input_event_handle(msg.ptr);
			break;
		#endif

		#ifdef CONFIG_PLAYTTS
		case MSG_TTS_EVENT:
			if (msg.cmd == TTS_EVENT_START_PLAY) {
				tts_manager_play_process();
			}
			break;
		#endif

		case MSG_KEY_INPUT:
			/**input event means esd proecess finished*/
			system_input_event_handle(msg.value);
			break;


		case MSG_INPUT_EVENT:
			system_key_event_handle(&msg);
			break;

		case MSG_HOTPLUG_EVENT:
			system_hotplug_event_handle(&msg);
			break;

		case MSG_VOLUME_CHANGED_EVENT:
			system_app_volume_show(&msg);
			break;

		case MSG_POWER_OFF:
			system_power_off();
			break;
		case MSG_REBOOT:
			system_power_reboot(msg.cmd);
			break;
		case MSG_NO_POWER:
			sys_event_notify(SYS_EVENT_POWER_OFF);
			break;
		case MSG_BT_ENGINE_READY:
			main_freq_init();
		#ifdef CONFIG_ALARM_APP
			if (alarm_wakeup_source_check()) {
				system_app_launch(SYS_INIT_ALARM_MODE);
			} else {
				system_app_launch(SYS_INIT_NORMAL_MODE);
			}
		#else
			system_app_launch(SYS_INIT_NORMAL_MODE);
		#endif
		#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
			ota_app_init_bluetooth();
		#endif
		#ifdef CONFIG_RECORD_SERVICE
			recorder_service_set_media_notify();
		#endif
		#ifdef CONFIG_ACTIONS_ATT
			extern int act_att_notify_bt_engine_ready(void);
			act_att_notify_bt_engine_ready();
		#endif

			if (att_enter_bqb_flag == true) {
				SYS_LOG_INF("ATT Goto BQB TEST");
				k_thread_priority_set(k_current_get(), 0);
				/* disable watchdog */
				soc_watchdog_stop();
				extern void BT_DutTest(int bqb_mode);
				BT_DutTest(1); /*1-classic bqb, 2-ble bqb*/
			}
			break;

		case MSG_START_APP:
			SYS_LOG_INF("start %s\n", (char *)msg.ptr);
			app_switch((char *)msg.ptr, msg.reserve, true);
			break;

		case MSG_EXIT_APP:
			SYS_LOG_DBG("exit %s\n", (char *)msg.ptr);
			app_manager_exit_app((char *)msg.ptr, true);
			break;

		default:
			SYS_LOG_ERR(" error: %d\n", msg.type);
			break;
		}

		if (msg.callback)
			msg.callback(&msg, result, NULL);
	}

	thread_timer_handle_expired();
}

void main(void)
{
	bool play_welcome = true;
	bool init_bt_manager = true;
	u16_t reboot_type = 0;
	u8_t reason = 0;

	system_power_get_reboot_reason(&reboot_type, &reason);

#ifdef CONFIG_FIRMWARE_VERSION
	fw_version_dump_current();
#endif

	system_library_version_dump();

	system_init();

	system_audio_policy_init();

	system_tts_policy_init();

	system_event_map_init();

	system_input_handle_init();

	system_app_launch_init();

#ifdef CONFIG_ALARM_APP
	if (alarm_wakeup_source_check()) {
		play_welcome = false;
	}
#endif

#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()
	#ifdef CONFIG_WDT_MODE_RESET 
		&& reboot_type == REBOOT_TYPE_WATCHDOG) {
	#else 
		&& (reboot_type == REBOOT_TYPE_WATCHDOG || reboot_type == REBOOT_TYPE_HW_RESET)) {
	#endif
		play_welcome = false;
	} else {
		esd_manager_reset_finished();
	}
#endif

	if ((reboot_type == REBOOT_TYPE_SF_RESET) && (reason == REBOOT_REASON_GOTO_BQB_ATT)) {
		play_welcome = false;
		att_enter_bqb_flag = true;
	}

	if (!play_welcome) {
	#ifdef CONFIG_PLAYTTS
		tts_manager_lock();
	#endif
	} else if(reason != REBOOT_REASON_OTA_FINISHED){
		bool enter_stub_tool = false;

		#if (defined CONFIG_TOOL && defined CONFIG_ATT_ACTIVE_CONNECTION)
		if (tool_att_connect_try() == 0) {
			enter_stub_tool = true;
			init_bt_manager = false;
		}
		#endif

		if (enter_stub_tool == false) {
		#ifdef CONFIG_CARD_READER_APP
			if (usb_hotplug_device_mode()
				&& !(reason == REBOOT_REASON_NORMAL)) {
				if (card_reader_mode_check() == NO_NEED_INIT_BT) {
					init_bt_manager = false;
				}
			} else 
		#endif
			{
			#ifdef CONFIG_CHARGER_APP
				if (charger_mode_check()) {

				}
			#endif
			}
		}
	}

	system_app_view_init();

	system_ready();

#ifdef CONFIG_OTA_APP
	ota_app_init();
#ifdef CONFIG_OTA_BACKEND_SDCARD
	ota_app_init_sdcard();
#endif
#endif

#ifdef CONFIG_ACTIONS_TRACE
#ifdef CONFIG_TOOL
	if (tool_get_dev_type() == TOOL_DEV_TYPE_UART0) {
		/* stub uart and trace both use uart0, forbidden trace dma mode */
		trace_dma_print_set(false);
	}
#endif
	trace_init();
#endif

#ifdef CONFIG_AGING_PLAYBACK
	aging_playback_app_try();
#endif

#ifdef CONFIG_BT_CONTROLER_RF_FCC
	if ((reboot_type == REBOOT_TYPE_SF_RESET) && (reason == REBOOT_REASON_GOTO_BQB)) {
		SYS_LOG_INF("Goto BQB TEST");
	} else {
		system_enter_FCC_mode();
	}
#endif

#ifdef CONFIG_BT_MANAGER
	if (init_bt_manager) {
		bt_manager_init();
	}
#endif

#ifdef CONFIG_GMA_APP
	ais_gma_sppble_init();
#endif

#ifdef CONFIG_TEST_BLE_NOTIFY
	ble_test_init();
#endif

	//init
	system_app_uart_init();

	while (1) {
		main_msg_proc(NULL, NULL, NULL);
	}
}

extern char _main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, _main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);

