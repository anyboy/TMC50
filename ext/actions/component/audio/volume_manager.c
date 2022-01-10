/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file volume manager interface
 */

#include <os_common_api.h>
#include <kernel.h>
#include <app_manager.h>
#include <bt_manager.h>
#include <msg_manager.h>
#include <sys_event.h>
#include "ui_manager.h"
#include "audio_system.h"
#include <media_player.h>
#ifdef CONFIG_ESD_MANAGER
#include <esd_manager.h>
#endif

int system_volume_get(int stream_type)
{
	return audio_system_get_stream_volume(stream_type);
}

static void _system_volume_set_notify(u32_t volume, bool is_limited)
{
	struct app_msg new_msg = { 0 };

	new_msg.type = MSG_VOLUME_CHANGED_EVENT;
	new_msg.cmd = is_limited;
	new_msg.value = volume;

	send_async_msg("main", &new_msg);
}

int system_volume_set(int stream_type, int volume, bool display)
{
	int ret = 0;
	bool is_limited = false;
	static u32_t volume_timestampe = 0;
	int old_volume = audio_system_get_stream_volume(stream_type);
	media_player_t *player = media_player_get_current_main_player();

	/* set DA volume */
	if (player)
		media_player_set_volume(player, volume, volume);

	ret = audio_system_set_stream_volume(stream_type, volume);
	if (display) {
		if (ret == MAX_VOLUME_VALUE) {
			if ((u32_t)(k_cycle_get_32() - volume_timestampe) / 24 > 500000) {
				sys_event_notify(SYS_EVENT_MAX_VOLUME);
				volume_timestampe = k_cycle_get_32();
			}
			is_limited = true;
		} else if (ret == MIN_VOLUME_VALUE) {
			if ((u32_t)(k_cycle_get_32() - volume_timestampe) / 24 > 500000) {
				sys_event_notify(SYS_EVENT_MIN_VOLUME);
				volume_timestampe = k_cycle_get_32();
			}
			is_limited = true;
		}
	}
	if (old_volume == audio_system_get_stream_volume(stream_type)) {
		is_limited = true;
		ret = old_volume;
		goto exit;
	}

	ret = volume;

#ifdef CONFIG_BT_MANAGER
	if (stream_type == AUDIO_STREAM_VOICE) {
		bt_manager_hfp_sync_vol_to_remote(volume);
	} else if (stream_type == AUDIO_STREAM_MUSIC) {
		bt_manager_avrcp_sync_vol_to_remote(volume);
	}
#endif

#ifdef CONFIG_ESD_MANAGER
	{
		u16_t volume_info = ((stream_type & 0xff) << 8) | (volume & 0xff);
		esd_manager_save_scene(TAG_VOLUME, (u8_t *)&volume_info, 2);
	}
#endif

#ifdef CONFIG_TWS
	bt_manager_tws_sync_volume_to_slave(stream_type, volume);
#endif
	SYS_LOG_INF("old_volume %d new_volume %d\n", old_volume, volume);
exit:

	if (display) {
		_system_volume_set_notify(ret, is_limited);
	}
	return ret;
}

int system_volume_down(int stream_type, int decrement)
{
	return system_volume_set(stream_type,
				system_volume_get(stream_type) - decrement, true);
}

int system_volume_up(int stream_type, int increment)
{
	return system_volume_set(stream_type,
				system_volume_get(stream_type) + increment, true);
}

static void _system_volume_change_notify(u32_t volume)
{
	struct app_msg new_msg = { 0 };
	char *current_app = app_manager_get_current_app();

	new_msg.type = MSG_BT_EVENT;
	new_msg.cmd = BT_RMT_VOL_SYNC_EVENT;
	new_msg.value = volume;

	if (current_app) {
		send_async_msg(current_app, &new_msg);
	}
}

void system_volume_sync_remote_to_device(u32_t stream_type, u32_t volume)
{
	int old_volume = audio_system_get_stream_volume(stream_type);

	if (old_volume == volume) {
		return;
	}

	SYS_LOG_INF("old_volume %d new_volume %d\n", old_volume, volume);

#ifdef CONFIG_TWS
	bt_manager_tws_sync_volume_to_slave(stream_type, volume);
#endif

#ifdef CONFIG_ESD_MANAGER
	{
		u16_t volume_info = ((stream_type & 0xff) << 8) | (volume & 0xff);
		esd_manager_save_scene(TAG_VOLUME, (u8_t *)&volume_info, 2);
	}
#endif

	_system_volume_change_notify(volume);

#ifdef CONFIG_TWS
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
		return;
	}
#endif
}
