/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager config.
*/
#define SYS_LOG_DOMAIN "bt manager"
#define SYS_LOG_LEVEL 4		/* 4, debug */
#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include <sys_event.h>
#include <btservice_api.h>

static u8_t default_pre_mac[3] = {0xF4, 0x4E, 0xFD};

u8_t bt_manager_config_get_tws_limit_inquiry(void)
{
	return 0;
}

u8_t bt_manager_config_get_tws_compare_high_mac(void)
{
	return 1;
}

u8_t bt_manager_config_get_tws_compare_device_id(void)
{
	return 0;
}

u8_t bt_manager_config_get_idle_extend_windown(void)
{
	return 1;
}

void bt_manager_config_set_pre_bt_mac(u8_t *mac)
{
	memcpy(mac, default_pre_mac, sizeof(default_pre_mac));
}

void bt_manager_updata_pre_bt_mac(u8_t *mac)
{
	memcpy(default_pre_mac, mac, sizeof(default_pre_mac));
}

bool bt_manager_config_enable_tws_sync_event(void)
{
	return false;
}

u8_t bt_manager_config_connect_phone_num(void)
{
#ifdef CONFIG_BT_DOUBLE_PHONE
#ifdef CONFIG_BT_DOUBLE_PHONE_EXT_MODE
	return 3;
#else
	return 2;
#endif
#else
	return 1;
#endif
}

bool bt_manager_config_support_a2dp_aac(void)
{
#ifdef CONFIG_BT_A2DP_AAC
	return true;
#else
	return false;
#endif
}

bool bt_manager_config_pts_test(void)
{
#ifdef  CONFIG_BT_PTS_TEST
	return true;
#else
	return false;
#endif
}

u16_t bt_manager_config_volume_sync_delay_ms(void)
{
	return 3000;
}

u32_t bt_manager_config_bt_class(void)
{
	return 0x240404;		/* Rendering,Audio, Audio/Video, Wearable Headset Device */
}

u16_t *bt_manager_config_get_device_id(void)
{
	static const u16_t device_id[4] = {0x03E0, 0xFFFF, 0x0001, 0x0001};

	return (u16_t *)device_id;
}

int bt_manager_config_expect_tws_connect_role(void)
{
	/* If use expect tws role, Recommend user todo:
	 * use diff device ID to dicide tws master/slave role,
	 * and in bt_manager_tws_discover_check_device function check
	 * disvoer tws device is expect tws role.
	 */

	//return BTSRV_TWS_MASTER;		/* Expect as tws master */
	//return BTSRV_TWS_SLAVE;		/* Expect as tws slave */
	return BTSRV_TWS_NONE;		/* Decide tws role by bt service */
}
