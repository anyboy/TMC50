/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm app view
 */

#include "alarm.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static const ui_key_map_t alarm_keymap[] = {

	{ KEY_MENU,		KEY_TYPE_SHORT_UP,	ALARM_CHANGE_MODE_STATUS,	MSG_ALARM_CHANGE_MODE},
	{ KEY_POWER,		KEY_TYPE_SHORT_UP,	ALARM_CHANGE_MODE_STATUS,	MSG_ALARM_SETTING_CONFIRM},
	{ KEY_VOLUMEUP,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	ALARM_CHANGE_MODE_STATUS,	MSG_ALARM_VALUE_ADD},
	{ KEY_NEXTSONG,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD | KEY_TYPE_LONG_DOWN, ALARM_CHANGE_MODE_STATUS, MSG_ALARM_VALUE_ADD},
	{ KEY_VOLUMEDOWN,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	ALARM_CHANGE_MODE_STATUS,	MSG_ALARM_VALUE_SUB},
	{ KEY_PREVIOUSSONG,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD | KEY_TYPE_LONG_DOWN, ALARM_CHANGE_MODE_STATUS, MSG_ALARM_VALUE_SUB},
	/*short or long press mode key,will stop alarm,when alarm playing*/
	{ KEY_MENU,		KEY_TYPE_SHORT_UP | KEY_TYPE_LONG_DOWN,	ALARM_STATUS_RING_PLAYING,	MSG_ALARM_RING_STOP},
	/*short press power/next/prev/F1/vol+/vol- key,will snooze alarm,when alarm playing*/
	{ KEY_POWER,		KEY_TYPE_SHORT_UP,	ALARM_STATUS_RING_PLAYING,	MSG_ALARM_RING_SNOOZE},
	{ KEY_NEXTSONG,		KEY_TYPE_SHORT_UP | KEY_TYPE_LONG_DOWN,	ALARM_STATUS_RING_PLAYING,	MSG_ALARM_RING_SNOOZE},
	{ KEY_PREVIOUSSONG,		KEY_TYPE_SHORT_UP | KEY_TYPE_LONG_DOWN,	ALARM_STATUS_RING_PLAYING,	MSG_ALARM_RING_SNOOZE},
	{ KEY_TBD,		KEY_TYPE_SHORT_UP | KEY_TYPE_LONG_DOWN,	ALARM_STATUS_RING_PLAYING,	MSG_ALARM_RING_SNOOZE},
	{ KEY_VOLUMEUP,		KEY_TYPE_SHORT_UP | KEY_TYPE_LONG_DOWN,	ALARM_STATUS_RING_PLAYING,	MSG_ALARM_RING_SNOOZE},
	{ KEY_VOLUMEDOWN,		KEY_TYPE_SHORT_UP | KEY_TYPE_LONG_DOWN,	ALARM_STATUS_RING_PLAYING,	MSG_ALARM_RING_SNOOZE},
	{ KEY_RESERVED,	0,	0,	0}
};

static int _alarm_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
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
		break;
	}
	case MSG_VIEW_PAINT:
	{
		break;
	}
	}
	return 0;
}

void alarm_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));
	view_info.view_proc = _alarm_view_proc;
	view_info.view_key_map = alarm_keymap;
	view_info.view_get_state = alarm_get_status;
	if (memcmp(app_manager_get_current_app(), APP_ID_ALARM, strlen(APP_ID_ALARM)) == 0) {
		view_info.order = 1;
		view_info.app_id = APP_ID_ALARM;
	} else {
		view_info.order = 2;
		view_info.app_id = APP_ID_MAIN;
	}

	ui_view_create(ALARM_VIEW, &view_info);
#ifdef CONFIG_SEG_LED_MANAGER
	if (memcmp(app_manager_get_current_app(), APP_ID_ALARM, strlen(APP_ID_ALARM)) == 0) {
		seg_led_manager_clear_screen(LED_CLEAR_ALL);
	}
#endif

	SYS_LOG_INF("ok\n");
}

void alarm_view_deinit(void)
{
	ui_view_delete(ALARM_VIEW);
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_set_timeout_event(0, NULL);
#endif

	SYS_LOG_INF("ok\n");
}

void alarm_calendar_year_display(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, false);
	snprintf(diplay_str, sizeof(diplay_str), "%04u", alarm->tm.tm_year);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}
void alarm_calendar_mon_day_display(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, false);
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_mon, alarm->tm.tm_mday);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}
void alarm_calendar_mon_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_mon);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}
void alarm_calendar_day_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_mday);
	seg_led_display_string(SLED_NUMBER3, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_clock_set_display(struct alarm_app_t *alarm)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, true);
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_hour, alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}
void alarm_clock_display(struct alarm_app_t *alarm, bool need_update)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_P1, false);
	if (need_update)
		alarm_manager_get_time(&alarm->tm);

	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_hour, alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);

	if (alarm->reflash_counter % 2)
		seg_led_display_icon(SLED_COL, false);
	else
		seg_led_display_icon(SLED_COL, true);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}
void alarm_clock_hour_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_hour);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}
void alarm_clock_min_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER3, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}
void alarm_onoff_display(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, false);
	if (alarm->alarm_is_on) {
		seg_led_display_string(SLED_NUMBER1, " ON ", is_light);
	} else {
		seg_led_display_string(SLED_NUMBER1, " OFF", is_light);
	}
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_ringing_display(struct alarm_app_t *alarm)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PLAY, true);
	if (memcmp(alarm->dir, "SD:ALARM", strlen("SD:ALARM")) == 0) {
		seg_led_display_icon(SLED_SD, true);
		seg_led_display_icon(SLED_USB, false);
	} else if (memcmp(alarm->dir, "USB:ALARM", strlen("USB:ALARM")) == 0) {
		seg_led_display_icon(SLED_USB, true);
		seg_led_display_icon(SLED_SD, false);
	}
	alarm_clock_display(alarm, true);
	seg_led_manager_set_timeout_event(0, NULL);
	alarm->set_ok = 0;
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
#endif
}
void alarm_ringing_clock_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	alarm_manager_get_time(&alarm->tm);
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_hour, alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_display_icon(SLED_COL, is_light);
#endif
}

