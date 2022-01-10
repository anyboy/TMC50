/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app ui define
 */

#ifndef _APP_UI_EVENT_H_
#define _APP_UI_EVENT_H_
#include <sys_event.h>
#include <msg_manager.h>


enum {
	NEED_INIT_BT,
	NO_NEED_INIT_BT,
};

enum {
    // bt common key message
	MSG_BT_PLAY_TWS_PAIR = MSG_APP_MESSAGE_START,
	MSG_BT_PLAY_DISCONNECT_TWS_PAIR,
    MSG_BT_PLAY_CLEAR_LIST,
    MSG_BT_SIRI_START,
	MSG_BT_SIRI_STOP,
    MSG_BT_CALL_LAST_NO,
    MSG_BT_HID_START,
	MSG_KEY_POWER_OFF,
	MSG_BT_APP_MESSAGE_START,
	MSG_BT_TWS_SWITCH_MODE,
	MSG_RECORDER_START_STOP,
	MSG_GMA_RECORDER_START,
	MSG_ALARM_ENTRY_EXIT,

	MSG_APP_MESSAGE_MAX_INDEX,
};

enum APP_UI_VIEW_ID {
    MAIN_VIEW,
	BTMUSIC_VIEW,
	BTCALL_VIEW,
	TWS_VIEW,
	LCMUSIC_VIEW,
	LOOP_PLAYER_VIEW,
	LINEIN_VIEW,
	AUDIO_INPUT_VIEW,
	RECORDER_VIEW,
	USOUND_VIEW,
	CARD_READER_VIEW,
	CHARGER_VIEW,
	ALARM_VIEW,
	FM_VIEW,
	OTA_VIEW,
};

enum APP_UI_EVENT_ID {
    UI_EVENT_NONE = 0,
    UI_EVENT_POWER_ON,
    UI_EVENT_POWER_OFF,
    UI_EVENT_STANDBY,
    UI_EVENT_WAKE_UP,

    UI_EVENT_LOW_POWER,
    UI_EVENT_NO_POWER,
    UI_EVENT_CHARGE,
    UI_EVENT_CHARGE_OUT,
    UI_EVENT_POWER_FULL,

    UI_EVENT_ENTER_PAIRING,
    UI_EVENT_CLEAR_PAIRED_LIST,
    UI_EVENT_WAIT_CONNECTION,
    UI_EVENT_CONNECT_SUCCESS,
    UI_EVENT_SECOND_DEVICE_CONNECT_SUCCESS,
    UI_EVENT_BT_DISCONNECT,
    UI_EVENT_TWS_START_TEAM,
    UI_EVENT_TWS_TEAM_SUCCESS,
    UI_EVENT_TWS_DISCONNECTED,
    UI_EVENT_TWS_PAIR_FAILED,

	UI_EVENT_VOLUME_CHANGED,
    UI_EVENT_MIN_VOLUME,
    UI_EVENT_MAX_VOLUME,

    UI_EVENT_PLAY_START,
    UI_EVENT_PLAY_PAUSE,
    UI_EVENT_PLAY_PREVIOUS,
    UI_EVENT_PLAY_NEXT,

    UI_EVENT_BT_INCOMING,
    UI_EVENT_BT_OUTGOING,
    UI_EVENT_BT_START_CALL,
    UI_EVENT_BT_ONGOING,
    UI_EVENT_BT_HANG_UP,
    UI_EVENT_START_SIRI,
    UI_EVENT_STOP_SIRI,
    UI_EVENT_BT_CALLRING,

    UI_EVENT_MIC_MUTE,
    UI_EVENT_MIC_MUTE_OFF,
    UI_EVENT_TAKE_PICTURES,
    UI_EVENT_FACTORY_DEFAULT,

	UI_EVENT_ENTER_BTMUSIC,
    UI_EVENT_ENTER_LINEIN,
    UI_EVENT_ENTER_USOUND,
    UI_EVENT_ENTER_SPDIF_IN,
	UI_EVENT_ENTER_I2SRX_IN,
    UI_EVENT_ENTER_SDMPLAYER,
    UI_EVENT_ENTER_UMPLAYER,
    UI_EVENT_ENTER_NORMPLAYER,
    UI_EVENT_ENTER_SDPLAYBACK,
    UI_EVENT_ENTER_UPLAYBACK,
    UI_EVENT_ENTER_RECORD,
    UI_EVENT_ENTER_MIC_IN,
	UI_EVENT_ENTER_FM,
};

struct event_strmap_s {
	int8_t event;
	const char *str;
};

struct event_ui_map {
    uint8_t  sys_event;
    uint8_t  ui_event;
};

#endif