/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief loop player app view
 */

#include "loop_player.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static const ui_key_map_t loopplay_keymap[] = {

	{ KEY_VOLUMEUP,	 	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	LOOP_STATUS_ALL, MSG_LOOPPLAYER_PLAY_VOLUP},
	{ KEY_VOLUMEDOWN,   KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	LOOP_STATUS_ALL, MSG_LOOPPLAYER_PLAY_VOLDOWN},
	{ KEY_POWER,		KEY_TYPE_SHORT_UP,								LOOP_STATUS_PLAYING, MSG_LOOPPLAYER_PLAY_PAUSE},
	{ KEY_POWER,		KEY_TYPE_SHORT_UP,								LOOP_STATUS_STOP, MSG_LOOPPLAYER_PLAY_PLAYING},
	{ KEY_NEXTSONG,	 	KEY_TYPE_SHORT_UP,								LOOP_STATUS_ALL, MSG_LOOPPLAYER_PLAY_NEXT},
	{ KEY_PREVIOUSSONG, KEY_TYPE_SHORT_UP,								LOOP_STATUS_ALL, MSG_LOOPPLAYER_PLAY_PREV},
	{ KEY_TBD,			KEY_TYPE_SHORT_UP,								LOOP_STATUS_ALL, MSG_LOOPPLAYER_SET_PLAY_MODE},
	{ KEY_RESERVED,	 0,  0, 0}
};

static int _loopplay_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		SYS_LOG_INF("CREATE\n");
		break;

	case MSG_VIEW_DELETE:
		SYS_LOG_INF("DELETE\n");
	#ifdef CONFIG_LED_MANAGER
		// led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);
		// led_manager_set_display(1, LED_OFF, OS_FOREVER, NULL);
	#endif
		break;

	case MSG_VIEW_PAINT:
		break;
	}
	return 0;
}

void loopplay_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _loopplay_view_proc;
	view_info.view_key_map = loopplay_keymap;
	view_info.view_get_state = loopplay_get_status;
	view_info.order = 1;
	view_info.app_id = APP_ID_LOOP_PLAYER;
	ui_view_create(LOOP_PLAYER_VIEW, &view_info);
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif
	SYS_LOG_INF(" ok\n");
}

void loopplay_view_deinit(void)
{
	ui_view_delete(LOOP_PLAYER_VIEW);

	SYS_LOG_INF("ok\n");
}

void loopplay_display_icon_disk(char dir[])
{
	SYS_LOG_INF("dir:%s\n", dir);
#ifdef CONFIG_SEG_LED_MANAGER
	if (memcmp(dir, "SD:", strlen("SD:")) == 0) {
		seg_led_display_icon(SLED_SD, true);
	} else if (memcmp(dir, "USB:", strlen("USB:")) == 0) {
		seg_led_display_icon(SLED_USB, true);
	}
#endif
}

void loopplay_display_icon_play(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PLAY, true);
	seg_led_display_icon(SLED_PAUSE, false);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
	led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
#endif
}

void loopplay_display_icon_pause(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PLAY, false);
	seg_led_display_icon(SLED_PAUSE, true);
#endif
#ifdef CONFIG_LED_MANAGER
	// led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
	// led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif
}

void loopplay_display_play_time(int time, int seek_time)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	/* remove hours */
	int temp_time = (time + seek_time) / 1000 % 3600;

	if (temp_time < 0)
		temp_time = 0;

	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", temp_time / 60, temp_time % 60);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_display_icon(SLED_COL, true);
#endif
}

void loopplay_display_track_no(u16_t track_no)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};
	snprintf(diplay_str, sizeof(diplay_str), "%04u", track_no % 10000);
	if (track_no > 9999)
		diplay_str[0] = 'A';
	seg_led_manager_clear_screen(LED_CLEAR_NUM);
	seg_led_display_icon(SLED_COL, false);
	seg_led_manager_set_timeout_event_start();
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_manager_set_timeout_event(3000, NULL);/*track no display 3s*/
#endif
}

void loopplay_display_loop_str()
{
#ifdef CONFIG_SEG_LED_MANAGER
	char loop_str[5] = "LOOP";
	seg_led_manager_set_timeout_event_start();
	seg_led_display_string(SLED_NUMBER1, loop_str, true);
	seg_led_display_icon(SLED_COL, false);
	seg_led_manager_set_timeout_event(3000, NULL);
#endif
}

void loopplay_display_none_str()
{
#ifdef CONFIG_SEG_LED_MANAGER
	char loop_str[5] = "NONE";
	seg_led_manager_clear_screen(LED_CLEAR_NUM);
	seg_led_display_icon(SLED_PLAY, false);
	seg_led_display_icon(SLED_PAUSE, false);

	seg_led_display_string(SLED_NUMBER1, loop_str, true);
#endif
}