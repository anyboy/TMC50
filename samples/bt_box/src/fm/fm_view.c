/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief fm app view
 */

#include "fm.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static const ui_key_map_t fm_keymap[] = {

	{ KEY_VOLUMEUP,		KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	FM_STATUS_ALL, MSG_FM_PLAY_VOLUP},
	{ KEY_VOLUMEDOWN,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	FM_STATUS_ALL, MSG_FM_PLAY_VOLDOWN},
	{ KEY_NEXTSONG,     KEY_TYPE_SHORT_UP,									FM_STATUS_ALL, MSG_FM_PLAY_NEXT},
	{ KEY_PREVIOUSSONG, KEY_TYPE_SHORT_UP,									FM_STATUS_ALL, MSG_FM_PLAY_PREV},
	{ KEY_NEXTSONG,     KEY_TYPE_LONG_DOWN,									FM_STATUS_ALL, MSG_FM_MANUAL_SEARCH_NEXT},
	{ KEY_PREVIOUSSONG, KEY_TYPE_LONG_DOWN,									FM_STATUS_ALL, MSG_FM_MANUAL_SEARCH_PREV},
	{ KEY_POWER,		KEY_TYPE_SHORT_UP,									FM_STATUS_ALL, MSG_FM_PLAY_PAUSE_RESUME},
	{ KEY_TBD,			KEY_TYPE_SHORT_UP,									FM_STATUS_ALL, MSG_FM_AUTO_SEARCH},
	{ KEY_RESERVED,	0,	0,	0}
};

static int _fm_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		SYS_LOG_INF("CREATE\n");
		break;
	case MSG_VIEW_DELETE:
		SYS_LOG_INF("DELETE\n");
	#ifdef CONFIG_PLAYTTS
		tts_manager_remove_event_lisener(NULL);
		tts_manager_wait_finished(true);
	#endif
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

void fm_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _fm_view_proc;
	view_info.view_key_map = fm_keymap;
	view_info.view_get_state = fm_get_status;
	view_info.order = 1;
	view_info.app_id = APP_ID_FM;

	ui_view_create(FM_VIEW, &view_info);

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_FM, true);
#endif
	SYS_LOG_INF(" ok\n");
}

void fm_view_deinit(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_set_timeout_event(0, NULL);
#endif

	ui_view_delete(FM_VIEW);

	SYS_LOG_INF("ok\n");
}

int _fm_view_show_station_num_finished(void)
{
	struct fm_app_t *fm = fm_get_app();

	fm_view_show_freq(fm->current_freq);
	return 0;
}

void fm_view_show_station_num(u16_t num)
{

	struct fm_app_t *fm = fm_get_app();

#ifdef CONFIG_SEG_LED_MANAGER
	u8_t freq_num[5];

	seg_led_manager_clear_screen(LED_CLEAR_NUM);
	seg_led_display_icon(SLED_P1, false);
	snprintf(freq_num, sizeof(freq_num), "CH%02u", num);
	seg_led_display_string(SLED_NUMBER1, freq_num, true);
	fm_stop_play();
	fm->need_resume_play = 1;
	seg_led_manager_set_timeout_event(3000, _fm_view_show_station_num_finished);

#endif
#ifdef CONFIG_LED_MANAGER
	//led_manager_set_blink(0, 200, 100, OS_FOREVER, LED_START_STATE_ON, NULL);
	//led_manager_set_blink(1, 200, 100, OS_FOREVER, LED_START_STATE_OFF, NULL);
#endif
	fm_view_play_freq(fm->current_freq);

}

void fm_view_show_freq(u16_t freq)
{
#ifdef CONFIG_SEG_LED_MANAGER
	u8_t freq_num[5];

	if (freq / 10000 > 0) {
		snprintf(freq_num, sizeof(freq_num), "%04u", freq / 10);
		seg_led_display_string(SLED_NUMBER1, freq_num, true);
	} else {
		snprintf(freq_num, sizeof(freq_num), "%03u", freq / 10);
		seg_led_display_number(SLED_NUMBER1, '0', false);
		seg_led_display_string(SLED_NUMBER2, freq_num, true);
	}
	seg_led_display_icon(SLED_P1, true);
#endif
}
#ifdef CONFIG_PLAYTTS
u8_t freq_num[6];
u8_t freq_index = 0;
static int _fm_view_freq_get_file_name(u8_t num, u8_t *file_name, u8_t file_name_len)
{

	if ((num >= '0') && (num <= '9')) {
		snprintf(file_name, file_name_len, "%c.mp3", num);
	} else if (num == '.') {
		snprintf(file_name, file_name_len, "dot.mp3");
	}

	return 0;
}

static void _fm_view_play_freq_event_nodify(u8_t *tts_id, u32_t event)
{
	u8_t file_name[13];
	struct app_msg msg = {0};

	if (event == TTS_EVENT_STOP_PLAY) {

		if (freq_index < strlen(freq_num)) {
			_fm_view_freq_get_file_name(freq_num[freq_index++], file_name, sizeof(file_name));
			tts_manager_play(file_name, 0);
		} else {
			tts_manager_remove_event_lisener(_fm_view_play_freq_event_nodify);
			msg.type = MSG_INPUT_EVENT;
			msg.cmd = MSG_FM_MEDIA_START;
			send_async_msg(APP_ID_FM, &msg);

		}
	}
}
#endif

void fm_view_play_freq(u16_t freq)
{
#ifdef CONFIG_PLAYTTS
	u8_t file_name[13];

	freq_index = 0;

	tts_manager_remove_event_lisener(_fm_view_play_freq_event_nodify);
	tts_manager_wait_finished(true);

	snprintf(freq_num, sizeof(freq_num), "%d.%d", freq / 100, freq % 100 / 10);

	memset(file_name, 0, sizeof(file_name));

	_fm_view_freq_get_file_name(freq_num[freq_index++], file_name, sizeof(file_name));

	tts_manager_add_event_lisener(_fm_view_play_freq_event_nodify);

	tts_manager_play(file_name, 0);
#else
	fm_start_play();
#endif
}

void fm_view_show_play_status(bool status)
{
	/*0 led play 1 led pause */
#ifdef CONFIG_SEG_LED_MANAGER
	if (status) {
		seg_led_display_icon(SLED_PLAY, false);
		seg_led_display_icon(SLED_PAUSE, true);
	} else {
		seg_led_display_icon(SLED_PAUSE, false);
		seg_led_display_icon(SLED_PLAY, true);
	}
#endif
#ifdef CONFIG_LED_MANAGER
	if (!status) {
		led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
		led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
	} else {
		// led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		// led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	}
#endif
}

void fm_view_show_try_listen(void)
{
#ifdef CONFIG_LED_MANAGER
	// led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
	// led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif
}

