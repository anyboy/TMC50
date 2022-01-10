/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief card reader app view
*/

#include "card_reader.h"
#include "hotplug_manager.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static int _card_reader_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
{
    switch (msg_id)
    {
        case MSG_VIEW_CREATE:
        {
			SYS_LOG_INF("CREATE \n");
            break;
        }
        case MSG_VIEW_DELETE:
		{
			SYS_LOG_INF(" DELETE \n");
			break;
		}
		case MSG_VIEW_PAINT:
		{
			break;
		}
    }
    return 0;
}

void card_reader_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));
	view_info.view_proc = _card_reader_view_proc;
	view_info.order = 1;
	view_info.app_id = APP_ID_MAIN;

	ui_view_create(CARD_READER_VIEW, &view_info);

#ifdef CONFIG_SEG_LED_MANAGER
#ifdef CONFIG_HOTPLUG
	seg_led_display_string(SLED_NUMBER1, "USB ", true);
	if (hotplug_manager_get_state(HOTPLUG_SDCARD) == HOTPLUG_IN) {
		seg_led_display_icon(SLED_SD, true);
	}
#endif
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
	led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif

	SYS_LOG_INF("ok\n");
}

void card_reader_view_deinit(void)
{
	ui_view_delete(CARD_READER_VIEW);

	SYS_LOG_INF("ok\n");
}

void card_reader_show_storage_working(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_set_flash(FLASH_ITEM(SLED_SD), 400, FLASH_FOREVER, NULL);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_blink(0, 200, 100, 500, LED_START_STATE_ON, NULL);
	led_manager_set_blink(1, 200, 100, 500, LED_START_STATE_OFF, NULL);
#endif
}

void card_reader_show_card_plugin(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_SD, true);
#endif
}
