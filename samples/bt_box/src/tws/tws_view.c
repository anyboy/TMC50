/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music app view
*/

#include "tws.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static int _tws_get_status(void)
{
	return TRANSMIT_ALL_KEY_EVENT;
}

static const ui_key_map_t _tws_keymap[] =
{
    { KEY_VOLUMEUP,     KEY_TYPE_ALL, 	TRANSMIT_ALL_KEY_EVENT, 0},
    { KEY_VOLUMEDOWN,   KEY_TYPE_ALL, 	TRANSMIT_ALL_KEY_EVENT, 0},
    { KEY_MENU,         (KEY_TYPE_ALL & (~KEY_TYPE_LONG_DOWN)), TRANSMIT_ALL_KEY_EVENT, 0},
    { KEY_POWER,        KEY_TYPE_ALL, 	TRANSMIT_ALL_KEY_EVENT, 0},
	{ KEY_TBD,   		KEY_TYPE_ALL,	TRANSMIT_ALL_KEY_EVENT, 0},
    { KEY_NEXTSONG,     KEY_TYPE_ALL, 	TRANSMIT_ALL_KEY_EVENT, 0},
    { KEY_PREVIOUSSONG, KEY_TYPE_ALL, 	TRANSMIT_ALL_KEY_EVENT, 0},
    { KEY_RESERVED,     KEY_TYPE_ALL,  0, 0}
};

static int _tws_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
{
    switch (msg_id)
    {
        case MSG_VIEW_CREATE:
        {
			SYS_LOG_INF("CREATE\n");
            break;
        }
        case MSG_VIEW_DELETE:
		{
			SYS_LOG_INF("DELETE\n");
		#ifdef CONFIG_LED_MANAGER
			led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);
			led_manager_set_display(1, LED_OFF, OS_FOREVER, NULL);
		#endif
			break;
		}
		case MSG_VIEW_PAINT:
		{
			break;
		}
    }
    return 0;
}

void tws_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _tws_view_proc;
	view_info.view_key_map = _tws_keymap;
	view_info.view_get_state = _tws_get_status;
	view_info.order = 1;
	view_info.app_id = APP_ID_TWS;

	ui_view_create(TWS_VIEW, &view_info);

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
	seg_led_display_string(SLED_NUMBER2, "bL", true);
	seg_led_display_string(SLED_NUMBER4, " ", false);
	seg_led_display_icon(SLED_PAUSE, true);
	seg_led_display_icon(SLED_PLAY, false);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
	led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif

	SYS_LOG_INF(" ok\n");
}

void tws_view_deinit(void)
{
	ui_view_delete(TWS_VIEW);

	SYS_LOG_INF("ok\n");
}

void tws_view_show_play_status(bool status)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PAUSE, !status);
	seg_led_display_icon(SLED_PLAY, status);
#endif
#ifdef CONFIG_LED_MANAGER
	if (status) {
		led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
		led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
	} else {
		led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	}
#endif
}
