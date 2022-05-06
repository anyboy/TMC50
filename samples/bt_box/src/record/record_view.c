/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record app view
 */

#include "record.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static const ui_key_map_t recorder_keymap[] = {

	{ KEY_TBD, KEY_TYPE_LONG_DOWN, RECORDER_STATUS_ALL, MSG_RECPLAYER_INIT},
	{ KEY_TBD, KEY_TYPE_LONG_DOWN, RECFILE_STATUS_ALL, MSG_RECORDER_INIT},
	{ KEY_POWER, KEY_TYPE_SHORT_UP, RECORDER_STATUS_ALL, MSG_RECORDER_PAUSE_RESUME},
	{ KEY_NEXTSONG, KEY_TYPE_SHORT_UP, RECORDER_STATUS_ALL, MSG_RECORDER_SAVE_AND_START_NEXT},

	{ KEY_VOLUMEUP,     KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	RECFILE_STATUS_ALL, MSG_RECPLAYER_PLAY_VOLUP},
	{ KEY_VOLUMEDOWN,   KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	RECFILE_STATUS_ALL, MSG_RECPLAYER_PLAY_VOLDOWN},
	{ KEY_POWER,        KEY_TYPE_SHORT_UP,	RECFILE_STATUS_PLAYING,	MSG_RECPLAYER_PLAY_PAUSE},
	{ KEY_POWER,        KEY_TYPE_SHORT_UP,	RECFILE_STATUS_STOP, MSG_RECPLAYER_PLAY_PLAYING},
	{ KEY_NEXTSONG,     KEY_TYPE_SHORT_UP,	RECFILE_STATUS_ALL, MSG_RECPLAYER_PLAY_NEXT},
	{ KEY_PREVIOUSSONG, KEY_TYPE_SHORT_UP,	RECFILE_STATUS_ALL, MSG_RECPLAYER_PLAY_PREV},
	{ KEY_NEXTSONG,	KEY_TYPE_LONG_DOWN,	RECFILE_STATUS_PLAYING, MSG_RECPLAYER_SEEKFW_START_CTIME},
	{ KEY_PREVIOUSSONG, KEY_TYPE_LONG_DOWN,	RECFILE_STATUS_PLAYING, MSG_RECPLAYER_SEEKBW_START_CTIME},
	{ KEY_NEXTSONG,     KEY_TYPE_LONG_UP,	RECFILE_STATUS_SEEK, MSG_RECPLAYER_PLAY_SEEK_FORWARD},
	{ KEY_PREVIOUSSONG, KEY_TYPE_LONG_UP,	RECFILE_STATUS_SEEK, MSG_RECPLAYER_PLAY_SEEK_BACKWARD},
	{ KEY_TBD,	    KEY_TYPE_SHORT_UP,	RECFILE_STATUS_ALL,	MSG_RECPLAYER_SET_PLAY_MODE},
	{ KEY_RESERVED,	0,	0,	0}
};

static int _recorder_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
	{
		SYS_LOG_INF("CREATE\n");
		break;
	}
	case MSG_VIEW_DELETE:
	{
		SYS_LOG_INF("DELETE\n");
	#ifdef CONFIG_LED_MANAGER
		// led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);
		// led_manager_set_display(1, LED_OFF, OS_FOREVER, NULL);
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

void recorder_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _recorder_view_proc;
	view_info.view_key_map = recorder_keymap;
	view_info.view_get_state = recorder_get_status;
	view_info.order = 1;
	view_info.app_id = APP_ID_RECORDER;

	ui_view_create(RECORDER_VIEW, &view_info);
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif

	SYS_LOG_INF(" ok\n");
}

void recorder_view_deinit(void)
{
	ui_view_delete(RECORDER_VIEW);

	SYS_LOG_INF("ok\n");
}

void recorder_display_record_string(void)
{
	sys_event_notify(SYS_EVENT_ENTER_RECORD);
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
	seg_led_display_string(SLED_NUMBER2, "REC", true);
#endif
}

void recorder_display_record_time(u16_t record_time)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", record_time / 60, record_time % 60);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_display_icon(SLED_COL, true);
#endif

}
void recorder_display_record_start(struct recorder_app_t *record)
{
#ifdef CONFIG_SEG_LED_MANAGER
	if (strncmp(record->dir, "USB:RECORD", strlen("USB:RECORD")) == 0) {
		seg_led_display_set_flash(FLASH_ITEM(SLED_USB), 1000, FLASH_FOREVER, NULL);
	} else {
		seg_led_display_set_flash(FLASH_ITEM(SLED_SD), 1000, FLASH_FOREVER, NULL);
	}
	seg_led_display_icon(SLED_PLAY, true);
	seg_led_display_icon(SLED_PAUSE, false);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
	led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
#endif
}

void recorder_display_record_pause(struct recorder_app_t *record)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_set_flash(0, 1000, FLASH_FOREVER, NULL);
	if (strncmp(record->dir, "USB:RECORD", strlen("USB:RECORD")) == 0) {
		seg_led_display_icon(SLED_USB, true);
	} else {
		seg_led_display_icon(SLED_SD, true);
	}
	seg_led_display_icon(SLED_PLAY, false);
	seg_led_display_icon(SLED_PAUSE, true);
#endif
#ifdef CONFIG_LED_MANAGER
	// led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
	// led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif
}

void recplayer_display_icon_disk(struct recorder_app_t *record)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif
	if (strncmp(record->dir, "USB:RECORD", strlen("USB:RECORD")) == 0) {
		sys_event_notify(SYS_EVENT_ENTER_UPLAYBACK);
#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_icon(SLED_USB, true);
#endif
	} else {
		sys_event_notify(SYS_EVENT_ENTER_SDPLAYBACK);
#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_icon(SLED_SD, true);
#endif
	}
}

void recplayer_display_play_time(struct recorder_app_t *record, int seek_time)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};
	/* remove hours */
	int temp_time = (record->recplayer_bp_info.bp_info.time_offset + seek_time) / 1000 % 3600;

	if (temp_time < 0)
		temp_time = 0;
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", temp_time / 60, temp_time % 60);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_display_icon(SLED_COL, true);
#endif
}

void recplayer_display_icon_play(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PAUSE, false);
	seg_led_display_icon(SLED_PLAY, true);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
	led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
#endif

}
void recplayer_display_icon_pause(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PAUSE, true);
	seg_led_display_icon(SLED_PLAY, false);
#endif
#ifdef CONFIG_LED_MANAGER
	// led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
	// led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif

}
