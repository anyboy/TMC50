/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager connect.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <sys_event.h>
#include <bluetooth/host_interface.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include <property_manager.h>
#include "btservice_api.h"

#define BT_MAX_AUTOCONN_DEV				3
#define BT_TRY_AUTOCONN_DEV				2
#define BT_BASE_DEFAULT_RECONNECT_TRY	3
#define BT_BASE_STARTUP_RECONNECT_TRY	3
#define BT_BASE_TIMEOUT_RECONNECT_TRY	15
#define BT_TWS_SLAVE_DISCONNECT_RETRY	10
#define BT_BASE_RECONNECT_INTERVAL		6000
#define BT_PROFILE_RECONNECT_TRY		3
#define BT_PROFILE_RECONNECT_INTERVAL	3000

void bt_manager_startup_reconnect(void)
{
	u8_t phone_num, master_max_index;
	int cnt, i, phone_cnt;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;

	info = mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	if (cnt == 0) {
		goto reconnnect_ext;
	}

	phone_cnt = 0;
	phone_num = bt_manager_config_connect_phone_num();

	cnt = (cnt > BT_TRY_AUTOCONN_DEV) ? BT_TRY_AUTOCONN_DEV : cnt;
	master_max_index = (phone_num > 1) ? phone_num : BT_MAX_AUTOCONN_DEV;

	for (i = 0; i < master_max_index; i++) {
		/* As tws master, connect first */
		if (info[i].addr_valid && (info[i].tws_role == BTSRV_TWS_MASTER) &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp || info[i].hid)) {
			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			btif_br_auto_reconnect(&reconnect_param);

			info[i].addr_valid = 0;
			cnt--;
			break;
		}
	}

	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp|| info[i].hid)) {
			if ((phone_cnt == phone_num) &&
				(info[i].tws_role == BTSRV_TWS_NONE)) {
				/* Config connect phone number */
				continue;
			}

			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			btif_br_auto_reconnect(&reconnect_param);

			if (info[i].tws_role == BTSRV_TWS_NONE) {
				phone_cnt++;
			}

			cnt--;
			if (cnt == 0 || info[i].tws_role == BTSRV_TWS_SLAVE) {
				/* Reconnect tow device,
				 *  or as tws slave, just reconnect master, not reconnect phone
				 */
				break;
			}
		}
	}

reconnnect_ext:
	mem_free(info);
}

void bt_manager_startup_reconnect_phone(void)
{
	u8_t phone_num;
	int cnt, i, phone_cnt;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;

	info = mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	if (cnt == 0) {
		goto reconnnect_ext;
	}

	phone_cnt = 0;
	phone_num = bt_manager_config_connect_phone_num();

	cnt = (cnt > BT_TRY_AUTOCONN_DEV) ? BT_TRY_AUTOCONN_DEV : cnt;


	for (i = 0; i < BT_TRY_AUTOCONN_DEV; i++) {
		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp|| info[i].hid)) {
			if ((phone_cnt == phone_num) ||
				(info[i].tws_role != BTSRV_TWS_NONE)) {
				/* Config connect phone number */
				continue;
			}

			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			btif_br_auto_reconnect(&reconnect_param);
			phone_cnt++;
			cnt--;
			if (cnt == 0) {
				/* Reconnect tow device,
				 *  or as tws slave, just reconnect master, not reconnect phone
				 */
				break;
			}
		}
	}

reconnnect_ext:
	mem_free(info);
}

static bool bt_manager_check_reconnect_enable(void)
{
	bool auto_reconnect = true;
	char temp[16];

	memset(temp, 0, sizeof(temp));

#ifdef CONFIG_PROPERTY
	if (property_get(CFG_AUTO_RECONNECT, temp, 16) > 0) {
		if (strcmp(temp, "false") == 0) {
			auto_reconnect = false;
		}
	}
#endif

	return auto_reconnect;
}

void bt_manager_disconnected_reason(void *param)
{
	struct bt_disconnect_reason *p_param = (struct bt_disconnect_reason *)param;
	struct bt_set_autoconn reconnect_param;

	SYS_LOG_INF("tws_role %d", p_param->tws_role);

	if (!bt_manager_check_reconnect_enable()) {
		SYS_LOG_WRN("Disable do reconnect\n");
		return;
	}

	if (p_param->tws_role == BTSRV_TWS_MASTER) {
		/* Just let slave device do reconnect */
		return;
	}

	if (p_param->reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN &&
		p_param->reason != BT_HCI_ERR_REMOTE_TERM_CONN_DUO_TO_POWEROFF &&
		p_param->reason != BT_HCI_ERR_LOCAL_USER_TERM_CONN) {
		memcpy(reconnect_param.addr.val, p_param->addr.val, sizeof(bd_address_t));
		reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;

		if (p_param->tws_role == BTSRV_TWS_SLAVE) {
			reconnect_param.base_try = BT_TWS_SLAVE_DISCONNECT_RETRY;
		} else {
			reconnect_param.base_try = BT_BASE_DEFAULT_RECONNECT_TRY;
		}

		if (p_param->tws_role == BTSRV_TWS_NONE &&
			p_param->reason == BT_HCI_ERR_CONNECTION_TIMEOUT) {
			reconnect_param.base_try = BT_BASE_TIMEOUT_RECONNECT_TRY;
		}

		reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
		reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
		reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
		btif_br_auto_reconnect(&reconnect_param);
	}
}
static bool halted_phone = false;
void bt_manager_halt_phone(void)
{
	/**only master need halt phone*/
	if (bt_manager_tws_get_dev_role() != BTSRV_TWS_MASTER) {
		return;
	}
	btif_br_set_connnectable(false);
	btif_br_set_discoverable(false);
	btif_br_auto_reconnect_stop();
	btif_br_disconnect_device(BTSRV_DISCONNECT_PHONE_MODE);
	halted_phone = true;
	SYS_LOG_INF("");
}

void bt_manager_resume_phone(void)
{
	if(halted_phone) {
		bt_manager_startup_reconnect_phone();
		btif_br_set_connnectable(true);
		btif_br_set_discoverable(true);
		halted_phone = false;
		SYS_LOG_INF("");
	}
}

void bt_manager_set_connectable(bool connectable)
{
	btif_br_set_connnectable(connectable);
	btif_br_set_discoverable(connectable);
}

bool bt_manager_is_auto_reconnect_runing(void)
{
	return btif_br_is_auto_reconnect_runing();
}

int bt_manager_get_linkkey(struct bt_linkkey_info *info, u8_t cnt)
{
	return btif_br_get_linkkey(info, cnt);
}

int bt_manager_update_linkkey(struct bt_linkkey_info *info, u8_t cnt)
{
	return btif_br_update_linkkey(info, cnt);
}

int bt_manager_write_ori_linkkey(bd_address_t *addr, u8_t *link_key)
{
	return btif_br_write_ori_linkkey(addr, link_key);
}
