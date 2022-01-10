/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager avrcp profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include "btservice_api.h"
#include <audio_policy.h>
#include <audio_system.h>
#include <volume_manager.h>
#include <msg_manager.h>

static void _bt_manager_avrcp_msg_cb(struct app_msg *msg, int result, void *not_used)
{
	if (msg && msg->ptr){
		struct bt_id3_info *info = msg->ptr;
		for(int i =0 ; i< BT_TOTAL_ATTRIBUTE_ITEM_NUM; i++){
			if(info->item[i].data){
				mem_free(info->item[i].data);
			}
		}
		mem_free(msg->ptr);
	}
}

void _bt_manager_avrcp_callback(btsrv_avrcp_event_e event,void * param)
{
	int status;

	switch (event) {
	/** notifying that avrcp connected */
	case BTSRV_AVRCP_CONNECTED:
	{
		SYS_LOG_INF("avrcp connected\n");
		if (bt_manager_get_connected_dev_num() == 1) {
			bt_manager_event_notify(BT_AVRCP_CONNECTION_EVENT, NULL, 0);
		}
	}
	break;
	/** notifying that avrcp disconnected*/
	case BTSRV_AVRCP_DISCONNECTED:
	{
		SYS_LOG_INF("avrcp disconnected\n");
		if (bt_manager_get_connected_dev_num() == 1) {
			bt_manager_event_notify(BT_AVRCP_DISCONNECTION_EVENT, NULL, 0);
		}
	}
	break;
	/** notifying that avrcp stoped */
	case BTSRV_AVRCP_STOPED:
	{
		SYS_LOG_INF("avrcp stoped\n");
		status = BT_STATUS_PAUSED;
		bt_manager_event_notify(BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT, (void *)&status, 4);
	}
	break;
	/** notifying that avrcp paused */
	case BTSRV_AVRCP_PAUSED:
	{
		SYS_LOG_INF("avrcp paused\n");
		bt_manager_event_notify(BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT, (void *)&status, 4);
	}
	break;
	/** notifying that avrcp playing */
	case BTSRV_AVRCP_PLAYING:
	{
		SYS_LOG_INF("avrcp playing\n");
		bt_manager_event_notify(BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT, (void *)&status, 4);
	}
	break;
	/** notifying that avrcp cur track change */
	case BTSRV_AVRCP_TRACK_CHANGE:
	{
		SYS_LOG_INF("avrcp track change\n");
		bt_manager_event_notify(BT_AVRCP_TRACK_CHANGED_EVENT, NULL, 0);
	}
	break;
	/** notifying that id3 info */
	case BTSRV_AVRCP_UPDATE_ID3_INFO:
	{
		SYS_LOG_INF("avrcp id3 info\n");
		struct bt_id3_info info;
		struct bt_id3_info *pinfo = (struct bt_id3_info *)param;
		memcpy(&info,pinfo,sizeof(info));
		for(int i =0 ; i< BT_TOTAL_ATTRIBUTE_ITEM_NUM; i++){
			if(info.item[i].len && pinfo->item[i].data){
				info.item[i].data = mem_malloc(info.item[i].len + 1);
				memset(info.item[i].data, 0, info.item[i].len + 1);
				memcpy(info.item[i].data, pinfo->item[i].data, info.item[i].len);
			}
			SYS_LOG_INF("id3 %d %s\n",info.item[i].id,info.item[i].data);
		}
		int ret = bt_manager_event_notify_ext(BT_AVRCP_UPDATE_ID3_INFO_EVENT, (void *)&info,sizeof(info),_bt_manager_avrcp_msg_cb);
		if(ret <= 0){
			for(int i =0 ; i< BT_TOTAL_ATTRIBUTE_ITEM_NUM; i++){
				if(info.item[i].data){
					mem_free(info.item[i].data);
				}
			}
		}
	}
	break;
	}
}

#if (defined CONFIG_VOLUEM_MANAGER) || (defined CONFIG_BT_AVRCP_VOL_SYNC)
static u32_t _bt_manager_music_to_avrcp_volume(u32_t music_vol)
{
	u32_t  avrcp_vol = 0;
#ifdef CONFIG_AUDIO
	u32_t  max_volume = audio_policy_get_volume_level();

	if (music_vol == 0) {
		avrcp_vol = 0;
	} else if (music_vol >= max_volume) {
		avrcp_vol = 0x7F;
	} else {
		avrcp_vol = (music_vol * 0x80 / max_volume);
	}
#endif
	return avrcp_vol;
}
#endif

#ifdef CONFIG_VOLUEM_MANAGER
static u32_t _bt_manager_avrcp_to_music_volume(u32_t avrcp_vol)
{
	u32_t  music_vol = 0;
	u32_t  max_volume = audio_policy_get_volume_level();

	if (avrcp_vol == 0) {
		music_vol = 0;
	} else if (avrcp_vol >= 0x7F) {
		music_vol = max_volume;
	} else {
		music_vol = (avrcp_vol + 1) * max_volume / 0x80;
		if (music_vol == 0) {
			music_vol = 1;
		}
	}

	return music_vol;
}
#endif
void _bt_manager_avrcp_get_volume_callback(void *param, u8_t *volume)
{

#ifdef CONFIG_VOLUEM_MANAGER
	u32_t  music_vol = audio_system_get_stream_volume(AUDIO_STREAM_MUSIC);

	*volume = (u8_t)_bt_manager_music_to_avrcp_volume(music_vol);
#endif

}

void _bt_manager_avrcp_set_volume_callback(void *param, u8_t volume)
{
#ifdef CONFIG_VOLUEM_MANAGER
	u32_t music_vol = (u8_t)_bt_manager_avrcp_to_music_volume(volume);

	SYS_LOG_INF("volume %d\n", volume);

	system_volume_sync_remote_to_device(AUDIO_STREAM_MUSIC, music_vol);
#endif

}

static const btsrv_avrcp_callback_t btm_avrcp_ctrl_cb = {
	.event_cb = _bt_manager_avrcp_callback,
	.get_volume_cb = _bt_manager_avrcp_get_volume_callback,
	.set_volume_cb = _bt_manager_avrcp_set_volume_callback,
};

int bt_manager_avrcp_profile_start(void)
{
	return btif_avrcp_start((btsrv_avrcp_callback_t *)&btm_avrcp_ctrl_cb);
}

int bt_manager_avrcp_profile_stop(void)
{
	return btif_avrcp_stop();
}

int bt_manager_avrcp_play(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_PLAY);
}

int bt_manager_avrcp_stop(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_STOP);
}

int bt_manager_avrcp_pause(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_PAUSE);
}

int bt_manager_avrcp_play_next(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_FORWARD);
}

int bt_manager_avrcp_play_previous(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_BACKWARD);
}

int bt_manager_avrcp_fast_forward(bool start)
{
	if (start) {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_FORWARD_START);
	} else {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_FORWARD_STOP);
	}
	return 0;
}

int bt_manager_avrcp_fast_backward(bool start)
{
	if (start) {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_BACKWARD_START);
	} else {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_BACKWARD_STOP);
	}
	return 0;
}

int bt_manager_avrcp_sync_vol_to_remote(u32_t music_vol)
{
#ifdef CONFIG_BT_AVRCP_VOL_SYNC
	u32_t  avrcp_vol = _bt_manager_music_to_avrcp_volume(music_vol);

	return btif_avrcp_sync_vol(avrcp_vol);
#else
	return 0;
#endif
}

int bt_manager_avrcp_get_id3_info()
{
	return btif_avrcp_get_id3_info();
}

