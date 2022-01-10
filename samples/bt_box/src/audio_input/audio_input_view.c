/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input app view
 */

#include "audio_input.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static const ui_key_map_t audio_input_keymap[] = {

	{ KEY_VOLUMEUP,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, AUDIO_INPUT_STATUS_ALL, MSG_AUDIO_INPUT_PLAY_VOLUP},
	{ KEY_VOLUMEDOWN,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, AUDIO_INPUT_STATUS_ALL, MSG_AUDIO_INPUT_PLAY_VOLDOWN},
	{ KEY_POWER,		KEY_TYPE_SHORT_UP,							 AUDIO_INPUT_STATUS_ALL, MSG_AUDIO_INPUT_PLAY_PAUSE_RESUME},
	{ KEY_RESERVED,	0,	0,	0}
};

static int _audio_input_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
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

void audio_input_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _audio_input_view_proc;
	view_info.view_key_map = audio_input_keymap;
	view_info.view_get_state = audio_input_get_status;
	view_info.order = 1;
	view_info.app_id = app_manager_get_current_app();

	ui_view_create(AUDIO_INPUT_VIEW, &view_info);

	audio_input_view_clear_screen();

#ifdef CONFIG_SEG_LED_MANAGER
	if (memcmp(app_manager_get_current_app(), APP_ID_LINEIN, strlen(APP_ID_LINEIN)) == 0) {
		seg_led_display_icon(SLED_AUX, true);
		seg_led_display_string(SLED_NUMBER1, "LINE", true);
	} else if (memcmp(app_manager_get_current_app(), APP_ID_MIC_IN, strlen(APP_ID_MIC_IN)) == 0) {
		seg_led_display_string(SLED_NUMBER1, "ICIN", true);
	} else if (memcmp(app_manager_get_current_app(), APP_ID_SPDIF_IN, strlen(APP_ID_SPDIF_IN)) == 0) {
		seg_led_display_string(SLED_NUMBER1, "SPDF", true);
	} else if (memcmp(app_manager_get_current_app(), APP_ID_I2SRX_IN, strlen(APP_ID_I2SRX_IN)) == 0) {
		seg_led_display_string(SLED_NUMBER1, "I2S", true);
	}

#endif

	SYS_LOG_INF(" ok\n");
}

void audio_input_view_deinit(void)
{
	ui_view_delete(AUDIO_INPUT_VIEW);

	SYS_LOG_INF("ok\n");
}

void audio_input_show_play_status(bool status)
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
void audio_input_view_clear_screen(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif
}

