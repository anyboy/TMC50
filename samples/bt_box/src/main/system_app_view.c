/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app view
 */
#include <mem_manager.h>
#include <msg_manager.h>
#ifdef CONFIG_LED_MANAGER
#include <led_manager.h>
#endif
#include "ugui.h"
#include "ui_manager.h"
#include "power_manager.h"
#include "app_ui.h"
#include "tts_manager.h"
#include "system_app.h"

const ui_key_map_t common_keymap[] = {

	{ KEY_MENU,			KEY_TYPE_SHORT_UP,		1, MSG_SWITCH_APP},
	{ KEY_POWER,        KEY_TYPE_SHORT_UP,		1, MSG_SWITCH_APP},
	{ KEY_POWER,        KEY_TYPE_LONG_DOWN,		1,	MSG_KEY_POWER_OFF},
	//{ KEY_MENU,         KEY_TYPE_LONG_DOWN,		1,	MSG_BT_PLAY_CLEAR_LIST},
	{ KEY_POWER,        KEY_TYPE_DOUBLE_CLICK,	1,	MSG_BT_CALL_LAST_NO},
#ifdef CONFIG_GMA_APP
	{ KEY_TBD,			KEY_TYPE_LONG_DOWN,	1,	MSG_GMA_RECORDER_START},
#else
#ifdef CONFIG_RECORD_SERVICE
	{ KEY_TBD,			KEY_TYPE_LONG_DOWN,	1,	MSG_RECORDER_START_STOP},
#else
	{ KEY_TBD,   		KEY_TYPE_LONG_DOWN,		1,	MSG_BT_SIRI_START},
#endif
#endif
	//{ KEY_TBD,			KEY_TYPE_LONG_DOWN,		1,	MSG_BT_TWS_SWITCH_MODE},
	{ KEY_TBD,			KEY_TYPE_SHORT_UP,		1,	MSG_BT_PLAY_TWS_PAIR},
	{ KEY_TBD,			KEY_TYPE_DOUBLE_CLICK,	1,	MSG_ALARM_ENTRY_EXIT},
#ifdef CONFIG_BT_HID
	{ KEY_NEXTSONG,     KEY_TYPE_DOUBLE_CLICK,  1,	MSG_BT_HID_START},
#endif
	{ KEY_RESERVED,		0,						0,	0}
};

int system_app_ui_event(int event)
{
	SYS_LOG_INF(" %d\n", event);

	ui_message_send_async(MAIN_VIEW, MSG_VIEW_PAINT, event);

	return 0;
}

void system_app_volume_show(struct app_msg *msg)
{
#ifdef CONFIG_SEG_LED_MANAGER
	int volume_value = msg->value;
	u8_t volume[5];

	snprintf(volume, sizeof(volume), "U %02u", volume_value);
	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_P1, false);
	seg_led_display_icon(SLED_COL, false);
	seg_led_display_string(SLED_NUMBER1, volume, true);
	seg_led_manager_set_timeout_event(2000, NULL);
#endif
#ifdef CONFIG_LED_MANAGER
	if (msg->cmd) {
		led_manager_set_timeout_event_start();
		led_manager_set_blink(0, 100, 50, OS_FOREVER, LED_START_STATE_ON, NULL);
		led_manager_set_blink(1, 100, 50, OS_FOREVER, LED_START_STATE_OFF, NULL);
		/*blink 3 times*/
		led_manager_set_timeout_event(100 * 3, NULL);
	}
#endif
}

static void system_app_view_deal(u32_t ui_event)
{

#ifdef CONFIG_PLAYTTS
	tts_manager_process_ui_event(ui_event);
#endif
	switch (ui_event) {
	case UI_EVENT_PLAY_START:
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_icon(SLED_PAUSE, false);
		seg_led_display_icon(SLED_PLAY, true);
	#endif
	#ifdef CONFIG_LED_MANAGER
		led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
		led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
	#endif
	break;
	case UI_EVENT_PLAY_PAUSE:
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_icon(SLED_PAUSE, true);
		seg_led_display_icon(SLED_PLAY, false);
	#endif
	#ifdef CONFIG_LED_MANAGER
		led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	#endif
	break;
	case UI_EVENT_POWER_OFF:
		/**make sure powerdown tts */
	#ifdef CONFIG_PLAYTTS
		tts_manager_wait_finished(false);
	#endif
		sys_event_send_message(MSG_POWER_OFF);
		break;
	case UI_EVENT_NO_POWER:
		sys_event_send_message(MSG_NO_POWER);
		break;
	}

}

static int system_app_view_proc(u8_t view_id, u8_t msg_id, u32_t ui_event)
{
	SYS_LOG_INF(" msg_id %d ui_event %d\n", msg_id, ui_event);
	switch (msg_id) {
	case MSG_VIEW_CREATE:
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_manager_clear_screen(LED_CLEAR_ALL);
		seg_led_display_string(SLED_NUMBER1, "----", true);
	#endif
	#ifdef CONFIG_LED_MANAGER
		led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	#endif
		system_app_view_deal(UI_EVENT_POWER_ON);
		break;
	case MSG_VIEW_PAINT:
	    system_app_view_deal(ui_event);
		break;
	case MSG_VIEW_DELETE:
	#ifdef CONFIG_PLAYTTS
		tts_manager_wait_finished(true);
	#endif

	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_manager_clear_screen(LED_CLEAR_ALL);
	#endif
	#ifdef CONFIG_LED_MANAGER
		led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_OFF, OS_FOREVER, NULL);
	#endif
		break;
	default:
		break;
	}
	return 0;
}

void system_app_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = system_app_view_proc;
	view_info.view_key_map = common_keymap;
	view_info.view_get_state = NULL;
	view_info.order = 0;
	view_info.app_id = APP_ID_MAIN;

#ifdef CONFIG_UI_MANAGER
	ui_view_create(MAIN_VIEW, &view_info);
#endif

	SYS_LOG_INF(" ok\n");
}

void system_app_view_exit(void)
{
#ifdef CONFIG_UI_MANAGER
	ui_view_delete(MAIN_VIEW);
#endif
}



