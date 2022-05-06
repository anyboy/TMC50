/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music app view
 */

#include "btmusic.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>
#ifdef CONFIG_UGUI
#include <resource.h>
#endif

//#include "interactive.h"
extern int bt_connect_flag;   // 0=normal   1=connect   2=disconnect 

static int _btmusic_get_status(void)
{
	return BT_STATUS_A2DP_ALL;
}

static const ui_key_map_t btmusic_keymap[] = {
	{ KEY_VOLUMEUP,     KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	BT_STATUS_A2DP_ALL, MSG_BT_PLAY_VOLUP},
	{ KEY_VOLUMEDOWN,   KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD,	BT_STATUS_A2DP_ALL, MSG_BT_PLAY_VOLDOWN},
	//{ KEY_POWER,        KEY_TYPE_SHORT_UP,									BT_STATUS_A2DP_ALL, MSG_BT_PLAY_PAUSE_RESUME},
	{ KEY_NEXTSONG,     KEY_TYPE_SHORT_UP,									BT_STATUS_A2DP_ALL, MSG_BT_PLAY_NEXT},
	{ KEY_PREVIOUSSONG, KEY_TYPE_SHORT_UP,									BT_STATUS_A2DP_ALL, MSG_BT_PLAY_PREVIOUS},
	{ KEY_NEXTSONG,		KEY_TYPE_LONG_DOWN,									BT_STATUS_A2DP_ALL, MSG_BT_PLAY_SEEKFW_START},
	{ KEY_PREVIOUSSONG, KEY_TYPE_LONG_DOWN,									BT_STATUS_A2DP_ALL, MSG_BT_PLAY_SEEKBW_START},
	{ KEY_NEXTSONG,		KEY_TYPE_LONG_UP,									BT_STATUS_A2DP_ALL, MSG_BT_PLAY_SEEKFW_STOP},
	{ KEY_PREVIOUSSONG, KEY_TYPE_LONG_UP,									BT_STATUS_A2DP_ALL, MSG_BT_PLAY_SEEKBW_STOP},
	{ KEY_RESERVED,     0,  0, 0}
};

static int _btmusic_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
	{
		SYS_LOG_INF("CREATE\n");
		break;
	}
	case MSG_VIEW_DELETE:
	{
		SYS_LOG_INF(" DELETE\n");
	#ifdef CONFIG_LED_MANAGER
		//led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);
		//led_manager_set_display(1, LED_OFF, OS_FOREVER, NULL);
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

//蓝牙链接
void btmusic_view_show_connected(void)
{
	//printk("led\n");
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
	seg_led_display_string(SLED_NUMBER1, " bL ", true);
	seg_led_display_icon(SLED_PAUSE, true);
#if 0
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
		seg_led_display_string(SLED_NUMBER1, "-", true);
	}
#endif
#endif
#ifdef CONFIG_LED_MANAGER
	printk("led_on\n");
	////led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);		//长亮
	//led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif
	bt_connect_flag =1;
}

//蓝牙断开
void btmusic_view_show_disconnected(void)
{
	//printk("led\n");
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_string(SLED_NUMBER2, "bL", true);
	seg_led_display_string(SLED_NUMBER4, " ", false);
	seg_led_display_set_flash(FLASH_2ITEM(SLED_NUMBER2, SLED_NUMBER3), 1000, FLASH_FOREVER, NULL);
	seg_led_display_icon(SLED_PAUSE, false);
	seg_led_display_icon(SLED_PLAY, false);
#if 0
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
		seg_led_display_string(SLED_NUMBER1, "-", true);
	}
#endif
#endif
#ifdef CONFIG_LED_MANAGER
	printk("led_off\n");
	
	////led_manager_set_blink(0, 500, 500, OS_MINUTES(1), LED_START_STATE_OFF, NULL);		//闪烁
	//led_manager_set_blink(1, 200, 100, OS_FOREVER, LED_START_STATE_OFF, NULL);
#endif
	bt_connect_flag =2;
}

void btmusic_view_show_play_paused(bool playing)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PAUSE, !playing);
	seg_led_display_icon(SLED_PLAY, playing);
#endif
#ifdef CONFIG_LED_MANAGER
	if (playing) {
		led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
		led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
	} else {
		// led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		// led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	}
#endif
}

void btmusic_view_show_tws_connect(bool connect)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_string(SLED_NUMBER1, "-", connect);
#endif
}

void btmusic_view_show_id3_info(struct bt_id3_info *pinfo)
{
#ifdef CONFIG_UGUI
	UG_BMP img;

	UG_FontSelect(&FONT_ZH_YAHEI_16X16);
	UG_SetForecolor(C_BLACK);
	UG_SetBackcolor(0x9980);

	if (!pinfo)
		return;

	if (UG_GetImg_Info(MUSIC_BACK_IMG, &img) == UG_RESULT_OK) {
		UG_DrawBMP_Ext(0, 0, UG_GetXDim(), UG_GetYDim(), img.p, img.width);
	} else {
		UG_FillFrame_Ext(0, 0, 320, 240, 0x9980);
	}

	if (pinfo->item[0].data)
		UG_PutString_Ext(10, 10, 310, 60, pinfo->item[0].data, pinfo->item[0].character_id);/*max display 2 line*/
	if (pinfo->item[1].data)
		UG_PutString_Ext(10, 61, 310, 78, pinfo->item[1].data, pinfo->item[1].character_id);/*max display 1 line*/
	if (pinfo->item[2].data)
		UG_PutString_Ext(10, 95, 310, 214, pinfo->item[2].data, pinfo->item[2].character_id);

#endif
}

void btmusic_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _btmusic_view_proc;
	view_info.view_key_map = btmusic_keymap;
	view_info.view_get_state = _btmusic_get_status;
	view_info.order = 1;
	view_info.app_id = APP_ID_BTMUSIC;

	ui_view_create(BTMUSIC_VIEW, &view_info);

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif

	if (bt_manager_get_connected_dev_num() == 0) {
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_string(SLED_NUMBER2, "bL", true);
		seg_led_display_string(SLED_NUMBER4, " ", false);
		seg_led_display_set_flash(FLASH_2ITEM(SLED_NUMBER2, SLED_NUMBER3), 1000, FLASH_FOREVER, NULL);
	#if 0
		if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
			seg_led_display_string(SLED_NUMBER1, "-", true);
		}
	#endif
	#endif
	#ifdef CONFIG_LED_MANAGER
		printk("led_off\n");
		
		////led_manager_set_blink(0, 500, 500, OS_MINUTES(1), LED_START_STATE_OFF, NULL);
		//led_manager_set_blink(1, 200, 100, OS_FOREVER, LED_START_STATE_OFF, NULL);
	#endif
		bt_connect_flag =2;
	} else {
	#ifdef CONFIG_SEG_LED_MANAGER
		seg_led_display_string(SLED_NUMBER2, "bL", true);
		seg_led_display_string(SLED_NUMBER4, " ", false);
	#if 0
		if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
			seg_led_display_string(SLED_NUMBER1, "-", true);
		}
	#endif
		seg_led_display_icon(SLED_PAUSE, true);
		seg_led_display_icon(SLED_PLAY, false);
	#endif
	#ifdef CONFIG_LED_MANAGER
		// led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		// led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	#endif
	}
	/**don't notify tts when switch from btcall gma is the same*/
	if (strncmp(app_manager_get_prev_app(), APP_ID_BTCALL, strlen(APP_ID_BTCALL))
	    && strncmp(app_manager_get_prev_app(), APP_ID_AIS_APP, strlen(APP_ID_AIS_APP))) {
		sys_event_notify(SYS_EVENT_ENTER_BTMUSIC);
		if (bt_manager_get_connected_dev_num() == 0) {
			sys_event_notify(SYS_EVENT_BT_WAIT_PAIR);
		}
	}
	SYS_LOG_INF("ok\n");
}

void btmusic_view_deinit(void)
{
	ui_view_delete(BTMUSIC_VIEW);

	SYS_LOG_INF("ok\n");
}
