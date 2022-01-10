/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app launcher
 */

#include <os_common_api.h>
#include <string.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_manager.h>
#include <hotplug_manager.h>
#include "app_switch.h"
#include "app_defines.h"
#include "system_app.h"
#include "audio_system.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_STUB_DEV_USB
#include <usb/class/usb_stub.h>
#endif

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "system app launcher"
#include <logging/sys_log.h>
#endif

int system_app_launch(u8_t mode)
{
	char *default_app = APP_ID_BTMUSIC;

	/*waku up by alarm.switch to alarm app*/
#ifdef CONFIG_ALARM_APP
	if (mode == SYS_INIT_ALARM_MODE) {
		app_switch_add_app(APP_ID_ALARM);
		app_switch(APP_ID_ALARM, APP_SWITCH_NEXT, false);
		app_switch_lock(1);
		return 0;
	}
#endif
	/** add app to app switch manager*/
	app_switch_add_app(APP_ID_BTMUSIC);

#ifdef CONFIG_LINE_IN_APP
#ifdef CONFIG_HOTPLUG
	if (hotplug_manager_get_state(HOTPLUG_LINEIN) == HOTPLUG_IN) {
		app_switch_add_app(APP_ID_LINEIN);
	}
#endif
#endif

#ifdef CONFIG_SOUNDBAR_LCMUSIC
	app_switch_add_app(APP_ID_UHOST_MPLAYER);
#else

#ifdef CONFIG_LCMUSIC_APP
#ifdef CONFIG_HOTPLUG
	if (hotplug_manager_get_state(HOTPLUG_SDCARD) == HOTPLUG_IN) {
		app_switch_add_app(APP_ID_SD_MPLAYER);
	}

	if (hotplug_manager_get_state(HOTPLUG_USB_HOST) == HOTPLUG_IN) {
		app_switch_add_app(APP_ID_UHOST_MPLAYER);
	}
#endif
#ifdef CONFIG_DISK_MUSIC_APP
	app_switch_add_app(APP_ID_NOR_MPLAYER);
#endif
#endif

#endif	//CONFIG_SOUNDBAR_LCMUSIC

#ifdef CONFIG_DISK_MUSIC_APP
#ifdef CONFIG_LOOP_PLAYER_APP
	/* loop_player would start though no uhost and sdcard */
	app_switch_add_app(APP_ID_LOOP_PLAYER);
#endif
#endif

#ifdef CONFIG_RECORD_APP
#ifdef CONFIG_HOTPLUG
	if (hotplug_manager_get_state(HOTPLUG_USB_HOST) == HOTPLUG_IN
		 || hotplug_manager_get_state(HOTPLUG_SDCARD) == HOTPLUG_IN)
		app_switch_add_app(APP_ID_RECORDER);
#endif
#endif

#ifdef CONFIG_LOOP_PLAYER_APP
#ifdef CONFIG_HOTPLUG
	if (hotplug_manager_get_state(HOTPLUG_USB_HOST) == HOTPLUG_IN
		 || hotplug_manager_get_state(HOTPLUG_SDCARD) == HOTPLUG_IN)
		app_switch_add_app(APP_ID_LOOP_PLAYER);
#endif
#endif

#ifdef CONFIG_USOUND_APP
#ifdef CONFIG_HOTPLUG
	if (hotplug_manager_get_state(HOTPLUG_USB_DEVICE) == HOTPLUG_IN
#ifdef CONFIG_STUB_DEV_USB
		&& !usb_stub_enabled()
#endif
		) {
		app_switch_add_app(APP_ID_USOUND);
	}
#endif
#endif

#ifdef CONFIG_SPDIF_IN_APP
	app_switch_add_app(APP_ID_SPDIF_IN);
#endif

#ifdef CONFIG_I2SRX_IN_APP
	app_switch_add_app(APP_ID_I2SRX_IN);
#endif

#ifdef CONFIG_MIC_IN_APP
	app_switch_add_app(APP_ID_MIC_IN);
#endif

#ifdef CONFIG_FM_APP
	app_switch_add_app(APP_ID_FM);
#endif

#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		u8_t app_id = 0;
		u16_t volume_info = 0;

		esd_manager_restore_scene(TAG_APP_ID, &app_id, 1);
		default_app = app_switch_get_app_name(app_id);

	#ifdef CONFIG_AUDIO
		esd_manager_restore_scene(TAG_VOLUME, (u8_t *)&volume_info, 2);
		audio_system_set_stream_volume((volume_info >> 8) & 0xff, volume_info & 0xff);
	#endif

		/**
		 * if esd from btcall ,we switch btmusic first avoid call hangup when reset
		 * if hfp is always connected  device will auto switch to btcall later
		 */
		if (!strcmp(default_app, APP_ID_BTCALL)) {
			default_app = APP_ID_BTMUSIC;
		}

	#ifdef CONFIG_LCMUSIC_APP
		if (!strcmp(default_app, APP_ID_UHOST_MPLAYER)) {
			app_switch_add_app(APP_ID_UHOST_MPLAYER);
		}
	#endif
	#ifdef CONFIG_RECORD_APP
		if (!strcmp(default_app, APP_ID_RECORDER)) {
			app_switch_add_app(APP_ID_RECORDER);
		}
	#endif
	}
#endif

	SYS_LOG_INF("default_app: %s\n", default_app);
	if (app_switch(default_app, APP_SWITCH_CURR, false) == 0xff) {
		app_switch(APP_ID_BTMUSIC, APP_SWITCH_CURR, false);
	}
	return 0;
}

int system_app_launch_init(void)
{
	const char *app_id_list_array[] = app_id_list;

	if (app_switch_init(app_id_list_array, ARRAY_SIZE(app_id_list_array)))
		return -EINVAL;

	return 0;
}

