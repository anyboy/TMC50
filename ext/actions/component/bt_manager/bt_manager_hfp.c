/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <logging/sys_log.h>
#include <media/media_type.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <bt_manager.h>
#include <power_manager.h>
#include <app_manager.h>
#include <sys_event.h>
#include <mem_manager.h>
#include <bt_manager_inner.h>
#include <assert.h>
#include "app_switch.h"
#include "btservice_api.h"
#include <audio_policy.h>
#include <audio_system.h>
#include <volume_manager.h>
#include <alarm_manager.h>

enum {
	SIRI_INIT_STATE,
	SIRI_START_STATE,
	SIRI_RUNNING_STATE,
};

struct bt_manager_hfp_info_t {
	u8_t siri_mode:3;
	u8_t only_sco:1;
	u8_t allow_sco:1;
	u32_t hfp_status;
	u32_t sco_connect_time;
	u32_t send_atcmd_time;
};

static struct bt_manager_hfp_info_t hfp_manager;

static u32_t _bt_manager_call_to_hfp_volume(u32_t call_vol)
{
	u32_t  hfp_vol = 0;
#ifdef CONFIG_AUDIO
	if (call_vol == 0) {
		hfp_vol = 0;
	} else if (call_vol >= audio_policy_get_volume_level()) {
		hfp_vol = 0x0F;
	} else {
		hfp_vol = (call_vol * 0x10 / audio_policy_get_volume_level());
		if (hfp_vol == 0) {
			hfp_vol = 1;
		}
	}
#endif
	return hfp_vol;
}
#ifdef CONFIG_VOLUEM_MANAGER
static u32_t _bt_manager_hfp_to_call_volume(u32_t hfp_vol)
{
	u32_t  call_vol = 0;
#ifdef CONFIG_AUDIO
	if (hfp_vol == 0) {
		call_vol = 0;
	} else if (hfp_vol >= 0x0F) {
		call_vol = audio_policy_get_volume_level();
	} else {
		call_vol = (hfp_vol + 1) * audio_policy_get_volume_level() / 0x10;

		if (call_vol == 0) {
	        call_vol = 1;
		}
	}
#endif
	return call_vol;
}
#endif

int bt_manager_hfp_set_status(u32_t state)
{
	hfp_manager.hfp_status = state;
	return 0;
}

int bt_manager_hfp_get_status(void)
{
	return hfp_manager.hfp_status;
}

u8_t string_to_num(u8_t c){
	if(c >= '0' && c <= '9'){
		return c - '0';
	}else
		return 0;
}

//format like "20/11/30, 18:58:09"
//https://m2msupport.net/m2msupport/atcclk-clock/
int _bt_hfp_set_time(u8_t *time)
{
	SYS_LOG_INF("time %s\n", time);

	struct rtc_time tm;
	tm.tm_year = 2000 + string_to_num(time[0])*10 + string_to_num(time[1]);
	tm.tm_mon = string_to_num(time[3])*10 + string_to_num(time[4]);
	tm.tm_mday = string_to_num(time[6])*10 + string_to_num(time[7]);

	if(time[9] == ' '){
		tm.tm_hour = string_to_num(time[10])*10 + string_to_num(time[11]);
		tm.tm_min = string_to_num(time[13])*10 + string_to_num(time[14]);
		tm.tm_sec = string_to_num(time[16])*10 + string_to_num(time[17]);
	}else if(time[9] >= '0' && time[9] <= '9'){
		tm.tm_hour = string_to_num(time[9])*10 + string_to_num(time[10]);
		tm.tm_min = string_to_num(time[12])*10 + string_to_num(time[13]);
		tm.tm_sec = string_to_num(time[15])*10 + string_to_num(time[16]);
	}
#ifdef CONFIG_ALARM_MANAGER
	int ret = alarm_manager_set_time(&tm); 
	if (ret) { 
		SYS_LOG_ERR("set time error ret=%d\n", ret); 
		return -1; 
	}
#endif
	return 0;
}

static void _bt_manager_hfp_callback(btsrv_hfp_event_e event, u8_t *param, int param_size)
{
	switch (event) {
	case BTSRV_HFP_CONNECTED:
	{
		SYS_LOG_INF("hfp connected\n");
	#ifdef CONFIG_POWER
		bt_manager_hfp_battery_report(BT_BATTERY_REPORT_INIT, 0);
		bt_manager_hfp_battery_report(BT_BATTERY_REPORT_VAL, power_manager_get_battery_capacity());
	#endif
	#ifdef CONFIG_VOLUEM_MANAGER
		bt_manager_hfp_sync_vol_to_remote(audio_system_get_stream_volume(AUDIO_STREAM_VOICE));
	#endif
		bt_manager_hfp_get_time();
		break;
	}
	case BTSRV_HFP_PHONE_NUM:
	{
		bt_manager_event_notify(BT_HFP_CALL_RING_STATR_EVENT, param, param_size);
		SYS_LOG_INF("hfp phone num %s size %d\n", param, param_size);
		break;
	}
	case BTSRV_HFP_PHONE_NUM_STOP:
	{
		bt_manager_event_notify(BT_HFP_CALL_RING_STOP_EVENT, NULL, 0);
		SYS_LOG_INF("hfp phone stop\n");
		break;
	}
	case BTSRV_HFP_DISCONNECTED:
	{
		SYS_LOG_INF("hfp disconnected\n");
		bt_manager_hfp_set_status(BT_STATUS_NONE);
		break;
	}
	case BTSRV_HFP_CODEC_INFO:
	{
		u8_t hfp_codec_id = param[0];
		u8_t hfp_sample_rate = param[1];
		SYS_LOG_INF("codec_id %d sample_rate %d\n", hfp_codec_id, hfp_sample_rate);
		bt_manager_sco_set_codec_info(hfp_codec_id,hfp_sample_rate);
		break;
	}
	case BTSRV_HFP_CALL_INCOMING:
	{
		SYS_LOG_INF("hfp incoming\n");
		bt_manager_hfp_set_status(BT_STATUS_INCOMING);
		if (hfp_manager.allow_sco && strcmp("tws", app_manager_get_current_app())) {
			app_switch("btcall", APP_SWITCH_CURR, true);
			app_switch_lock(1);
		}
		hfp_manager.only_sco = 0;
		bt_manager_event_notify(BT_HFP_CALL_INCOMING, NULL, 0);
		break;
	}
	case BTSRV_HFP_CALL_OUTGOING:
	case BTSRV_HFP_CALL_ALERTED:
	{
		if (event == BTSRV_HFP_CALL_OUTGOING) {
			SYS_LOG_INF("hfp outgoing\n");
		} else {
			SYS_LOG_INF("hfp alerted\n");
		}
		bt_manager_hfp_set_status(BT_STATUS_OUTGOING);
		if (hfp_manager.allow_sco && strcmp("tws", app_manager_get_current_app())) {
			app_switch("btcall", APP_SWITCH_CURR, true);
			app_switch_lock(1);
		}
		hfp_manager.only_sco = 0;
		bt_manager_event_notify(BT_HFP_CALL_OUTGOING, NULL, 0);
		break;
	}

	case BTSRV_HFP_CALL_ONGOING:
	{
		SYS_LOG_INF("hfp ongoing\n");
		bt_manager_hfp_set_status(BT_STATUS_ONGOING);
		if (hfp_manager.allow_sco && strcmp("tws", app_manager_get_current_app())) {
			app_switch("btcall", APP_SWITCH_CURR, true);
			app_switch_lock(1);
		}
		hfp_manager.only_sco = 0;
		bt_manager_event_notify(BT_HFP_CALL_ONGOING, NULL, 0);
		break;
	}

	case BTSRV_HFP_CALL_3WAYIN:
	{
		SYS_LOG_INF("hfp 3wayin\n");
		bt_manager_hfp_set_status(BT_STATUS_3WAYIN);
		hfp_manager.only_sco = 0;
		break;
	}

	case BTSRV_HFP_CALL_MULTIPARTY:
	{
		SYS_LOG_INF("hfp multiparty\n");
		bt_manager_hfp_set_status(BT_STATUS_MULTIPARTY);
		hfp_manager.only_sco = 0;
		break;
	}

	case BTSRV_HFP_CALL_EXIT:
	{
		SYS_LOG_INF("hfp call exit\n");
		if (!strcmp("btcall", app_manager_get_current_app())) {
			bt_manager_event_notify(BT_HFP_CALL_HUNGUP, NULL, 0);
			app_switch_unlock(1);
			app_switch(NULL, APP_SWITCH_LAST, false);
		}
		bt_manager_hfp_set_status(BT_STATUS_NONE);
		break;
	}

	case BTSRV_HFP_SCO:
	{
		SYS_LOG_INF("hfp sco\n");
		if (hfp_manager.allow_sco && strcmp("tws", app_manager_get_current_app())) {
			hfp_manager.only_sco = 1;
			app_switch("btcall", APP_SWITCH_CURR, true);
			app_switch_lock(1);
		}
		if (hfp_manager.siri_mode == SIRI_START_STATE) {
			hfp_manager.siri_mode = SIRI_RUNNING_STATE;
		}
		if (hfp_manager.siri_mode == SIRI_RUNNING_STATE) {
			bt_manager_hfp_set_status(BT_STATUS_SIRI);
		}
		bt_manager_event_notify(BT_HFP_CALL_SIRI_MODE, NULL, 0);
		break;
	}

	case BTSRV_HFP_VOLUME_CHANGE:
	{
		u32_t volume = *(u8_t *)(param);

		SYS_LOG_INF("hfp volume change %d\n", volume);
		u32_t pass_time = os_uptime_get_32() - hfp_manager.sco_connect_time;
		/* some phone may send volume sync event after sco connect. */
		/* we must drop these event to avoid volume conflict */
		/* To Do: do we have better solution?? */
		if ((hfp_manager.sco_connect_time && (pass_time < 500))
			|| strcmp("btcall", app_manager_get_current_app())) {
			SYS_LOG_INF("drop volume sync event from phone\n");
			return;
		}
	#ifdef CONFIG_VOLUEM_MANAGER
		u32_t call_vol = _bt_manager_hfp_to_call_volume(volume);
		system_volume_sync_remote_to_device(AUDIO_STREAM_VOICE, call_vol);
	#endif
		break;
	}

	case BTSRV_HFP_SIRI_STATE_CHANGE:
	{
		u32_t state = *(u8_t *)(param);

		SYS_LOG_INF("hfp siri state %d\n", state);
		if (state == 1) {
			hfp_manager.siri_mode = SIRI_RUNNING_STATE;
			sys_event_notify(SYS_EVENT_SIRI_START);
		} else if (state == 2) {
			/* start siri cmd send OK,just play tts */
			sys_event_notify(SYS_EVENT_SIRI_START);
		} else{
			hfp_manager.siri_mode = SIRI_INIT_STATE;
			sys_event_notify(SYS_EVENT_SIRI_STOP);
		}
		if (hfp_manager.siri_mode == SIRI_RUNNING_STATE)
			bt_manager_hfp_set_status(BT_STATUS_SIRI);
		break;
	}

	case BTSRV_SCO_CONNECTED:
	{
		SYS_LOG_INF("hfp sco connected\n");
		hfp_manager.sco_connect_time = os_uptime_get_32();
		bt_manager_event_notify(BT_HFP_ESCO_ESTABLISHED_EVENT, NULL, 0);
		break;
	}
	case BTSRV_SCO_DISCONNECTED:
	{
		SYS_LOG_INF("hfp sco disconnected\n");
		hfp_manager.sco_connect_time = 0;
		bt_manager_event_notify(BT_HFP_ESCO_RELEASED_EVENT, NULL, 0);
		/**hfp_manager.only_sco == 0 means sco disconnect by switch sound source, not exit btcall app */
		if (hfp_manager.only_sco) {
			hfp_manager.only_sco = 0;
			if(!strcmp("btcall", app_manager_get_current_app())){
				app_switch_unlock(1);
				app_switch(NULL, APP_SWITCH_LAST, false);	
			}
			bt_manager_hfp_set_status(BT_STATUS_NONE);
		}
		hfp_manager.siri_mode = SIRI_INIT_STATE;

		break;
	}
	case BTSRV_HFP_ACTIVE_DEV_CHANGE:
	{
		SYS_LOG_INF("hfp dev changed\n");
		bt_manager_event_notify(BT_HFP_ACTIVEDEV_CHANGE_EVENT, NULL, 0);
		break;
	}
	case BTSRV_HFP_TIME_UPDATE:
	{	
		SYS_LOG_INF("hfp time update\n");
		_bt_hfp_set_time(param);
		break;
	}
	default:
		break;
	}
}

int bt_manager_hfp_profile_start(void)
{
	return btif_hfp_start(&_bt_manager_hfp_callback);
}

int bt_manager_hfp_profile_stop(void)
{
	return btif_hfp_stop();
}

int bt_manager_hfp_get_call_state(u8_t active_call)
{
	return btif_hfp_hf_get_call_state(active_call);
}

int bt_manager_hfp_check_atcmd_time(void)
{
	u32_t pass_time = os_uptime_get_32() - hfp_manager.send_atcmd_time;

	if (hfp_manager.send_atcmd_time && (pass_time < 600)) {
		return 0;
	}
	hfp_manager.send_atcmd_time = os_uptime_get_32();
	return 1;
}

int bt_manager_hfp_dial_number(u8_t *number)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_dial_number(number);
}

int bt_manager_hfp_dial_last_number(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_dial_last_number();
}

int bt_manager_hfp_dial_memory(int location)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_dial_memory(location);
}

int bt_manager_hfp_volume_control(u8_t type, u8_t volume)
{
#ifdef CONFIG_BT_HFP_HF_VOL_SYNC
	return btif_hfp_hf_volume_control(type, volume);
#else
	return 0;
#endif
}

int bt_manager_hfp_battery_report(u8_t mode, u8_t bat_val)
{
	if (mode == BT_BATTERY_REPORT_VAL) {
		bat_val = bat_val / 10;
		if (bat_val > 9) {
			bat_val = 9;
		}
	}
	SYS_LOG_INF("mode %d , bat_val %d\n", mode, bat_val);
	return btif_hfp_hf_battery_report(mode, bat_val);
}

int bt_manager_hfp_accept_call(void)
{
	return btif_hfp_hf_accept_call();
}

int bt_manager_hfp_reject_call(void)
{
	return btif_hfp_hf_reject_call();
}

int bt_manager_hfp_hangup_call(void)
{
	return btif_hfp_hf_hangup_call();
}

int bt_manager_hfp_hangup_another_call(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_hangup_another_call();
}

int bt_manager_hfp_holdcur_answer_call(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_holdcur_answer_call();
}

int bt_manager_hfp_hangupcur_answer_call(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_hangupcur_answer_call();
}

int bt_manager_hfp_start_siri(void)
{
	int ret = 0;

	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	if (hfp_manager.siri_mode != SIRI_RUNNING_STATE) {
		ret = btif_hfp_hf_voice_recognition_start();
		hfp_manager.siri_mode = SIRI_START_STATE;
	}

	return ret;
}

int bt_manager_hfp_stop_siri(void)
{
	int ret = 0;

	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	if (hfp_manager.siri_mode != SIRI_INIT_STATE) {
		sys_event_notify(SYS_EVENT_SIRI_STOP);
		ret = btif_hfp_hf_voice_recognition_stop();
		hfp_manager.siri_mode = SIRI_INIT_STATE;
	}
	return ret;
}

int bt_manager_hfp_switch_sound_source(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_switch_sound_source();
}

int bt_manager_hfp_get_time(void)
{
	return btif_hfp_hf_get_time();
}

int bt_manager_allow_sco_connect(bool allowed)
{
	hfp_manager.allow_sco = allowed ? 1 : 0;
	return btif_br_allow_sco_connect(allowed);
}


int bt_manager_hfp_send_at_command(u8_t *command,u8_t active_call)
{
	//if (!bt_manager_hfp_check_atcmd_time()) {
	//	return -1;
	//}
	return btif_hfp_hf_send_at_command(command,active_call);
}

int bt_manager_hfp_sync_vol_to_remote(u32_t call_vol)
{
	u32_t  hfp_vol = _bt_manager_call_to_hfp_volume(call_vol);

	return bt_manager_hfp_volume_control(1, (u8_t)hfp_vol);
}

int bt_manager_hfp_init(void)
{
	memset(&hfp_manager, 0, sizeof(struct bt_manager_hfp_info_t));
	hfp_manager.allow_sco = 1;
	hfp_manager.hfp_status = BT_STATUS_NONE;
	return 0;
}

