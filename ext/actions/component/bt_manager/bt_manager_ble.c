/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble manager.
 */
#define SYS_LOG_DOMAIN "btmgr_ble"

#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hex_str.h>

#include <msg_manager.h>
#include <mem_manager.h>
#include <bluetooth/host_interface.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include <sys_event.h>

/* TODO: New only support one ble connection */

#define BLE_CONNECT_INTERVAL_CHECK		(5000)		/* 5s */
#define BLE_CONNECT_DELAY_UPDATE_PARAM	(4000)
#define BLE_UPDATE_PARAM_FINISH_TIME	(4000)
#define BLE_DELAY_UPDATE_PARAM_TIME		(50)
#define BLE_NOINIT_INTERVAL				(0xFFFF)
#define BLE_DEFAULT_IDLE_INTERVAL		(80)		/* 80*1.25 = 100ms */
#define BLE_TRANSFER_INTERVAL			(12)		/* 12*1.25 = 15ms */

enum {
	PARAM_UPDATE_IDLE_STATE,
	PARAM_UPDATE_WAITO_UPDATE_STATE,
	PARAM_UPDATE_RUNING,
};

static OS_MUTEX_DEFINE(ble_mgr_lock);
static os_sem ble_ind_sem __in_section_unique(bthost_bss);
static struct bt_gatt_indicate_params ble_ind_params __in_section_unique(bthost_bss);
static sys_slist_t ble_list __in_section_unique(bthost_bss);
static u16_t ble_idle_interval __in_section_unique(bthost_bss);

struct ble_mgr_info {
	struct bt_conn *ble_conn;
	u8_t device_mac[6];
	u16_t ble_current_interval;
	u8_t br_a2dp_runing:1;
	u8_t br_hfp_runing:1;
	u8_t ble_transfer_state:1;
	u8_t update_work_state:4;
	u16_t ble_send_cnt;
	u16_t ble_pre_send_cnt;
	os_delayed_work param_update_work;
};

static struct ble_mgr_info ble_info __in_section_unique(bthost_bss);

static const struct bt_data ble_ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void ble_advertise(void)
{
	struct bt_le_adv_param param;
	const struct bt_data *ad, *scan_rsp;
	size_t ad_len, scan_rsp_len;
	int err;
	struct bt_data sd[1];
	u8_t ble_name[32];
	u8_t len;

	param.own_addr = NULL;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
	param.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME);

	len = hostif_bt_read_ble_name(ble_name, 32);
	sd[0].type = BT_DATA_NAME_COMPLETE;
	sd[0].data_len = len;
	sd[0].data = ble_name;

	scan_rsp = sd;
	scan_rsp_len = ARRAY_SIZE(sd);

	ad = ble_ad_discov;
	ad_len = ARRAY_SIZE(ble_ad_discov);

	err = hostif_bt_le_adv_start(&param, ad, ad_len, scan_rsp, scan_rsp_len);
	if (err < 0) {
		SYS_LOG_ERR("Failed to start advertising (err %d)", err);
	} else {
		SYS_LOG_INF("Advertising started");
	}
}

static void param_update_work_callback(struct k_work *work)
{
	u16_t req_interval;

	if (ble_info.ble_conn) {
		struct bt_le_conn_param param;

		if (ble_info.update_work_state == PARAM_UPDATE_RUNING) {
			ble_info.update_work_state = PARAM_UPDATE_IDLE_STATE;
			os_delayed_work_submit(&ble_info.param_update_work, BLE_CONNECT_INTERVAL_CHECK);
			return;
		}

		if (ble_info.update_work_state == PARAM_UPDATE_IDLE_STATE) {
			if (ble_info.ble_transfer_state &&
				(ble_info.ble_current_interval == BLE_TRANSFER_INTERVAL)) {
				if (ble_info.ble_pre_send_cnt == ble_info.ble_send_cnt) {
					ble_info.ble_transfer_state = 0;
				} else {
					ble_info.ble_pre_send_cnt = ble_info.ble_send_cnt;
				}
			}
		}

		if (ble_info.br_a2dp_runing || ble_info.br_hfp_runing) {
			req_interval = ble_idle_interval;
		} else if (ble_info.ble_transfer_state) {
			req_interval = BLE_TRANSFER_INTERVAL;
		} else {
			req_interval = ble_idle_interval;
		}

		if (req_interval == ble_info.ble_current_interval) {
			ble_info.update_work_state = PARAM_UPDATE_IDLE_STATE;
			os_delayed_work_submit(&ble_info.param_update_work, BLE_CONNECT_INTERVAL_CHECK);
			return;
		}

		ble_info.ble_current_interval = req_interval;

		/* interval time: x*1.25 */
		param.interval_min = req_interval;
		param.interval_max = req_interval;
		param.latency = 0;
		param.timeout = 500;
		hostif_bt_conn_le_param_update(ble_info.ble_conn, &param);
		SYS_LOG_INF("%d\n", req_interval);

		ble_info.update_work_state = PARAM_UPDATE_RUNING;
		os_delayed_work_submit(&ble_info.param_update_work, BLE_UPDATE_PARAM_FINISH_TIME);
	} else {
		ble_info.update_work_state = PARAM_UPDATE_IDLE_STATE;
	}
}

static void ble_check_update_param(void)
{
	if (ble_info.update_work_state == PARAM_UPDATE_IDLE_STATE) {
		ble_info.update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		os_delayed_work_submit(&ble_info.param_update_work, BLE_DELAY_UPDATE_PARAM_TIME);
	} else if (ble_info.update_work_state == PARAM_UPDATE_RUNING) {
		ble_info.update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		os_delayed_work_submit(&ble_info.param_update_work, BLE_UPDATE_PARAM_FINISH_TIME);
	} else {
		/* Already in PARAM_UPDATE_WAITO_UPDATE_STATE */
	}
}

static void ble_send_data_check_interval(void)
{
	ble_info.ble_send_cnt++;
	if ((ble_info.ble_current_interval != BLE_TRANSFER_INTERVAL) &&
		!ble_info.br_a2dp_runing && !ble_info.br_hfp_runing) {
		ble_info.ble_transfer_state = 1;
		ble_check_update_param();
	}
}

static void exchange_func(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *params)
{
	SYS_LOG_INF("Exchange %s\n", err == 0 ? "successful" : "failed");
}

static struct bt_gatt_exchange_params exchange_params = {
	.func = exchange_func,
};

static void notify_ble_connected(u8_t *mac, u8_t connected)
{
	struct ble_reg_manager *le_mgr;

	os_mutex_lock(&ble_mgr_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_list, le_mgr, node) {
		if (le_mgr->link_cb) {
			le_mgr->link_cb(ble_info.device_mac, connected);
		}
	}

	os_mutex_unlock(&ble_mgr_lock);
}

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[13];
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)) {
		return;
	}

	if (!ble_info.ble_conn) {
		memcpy(ble_info.device_mac, info.le.dst->a.val, 6);
		memset(addr, 0, 13);
		hex_to_str(addr, ble_info.device_mac, 6);
		SYS_LOG_INF("MAC: %s", addr);

		ble_info.ble_conn = hostif_bt_conn_ref(conn);
		notify_ble_connected(ble_info.device_mac, true);

		if (exchange_params.func) {
			hostif_bt_gatt_exchange_mtu(ble_info.ble_conn, &exchange_params);
		}

		ble_info.ble_current_interval = BLE_NOINIT_INTERVAL;
		ble_info.ble_transfer_state = 0;
		ble_info.ble_send_cnt = 0;
		ble_info.ble_pre_send_cnt = 0;
		ble_info.update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		os_delayed_work_submit(&ble_info.param_update_work, BLE_CONNECT_DELAY_UPDATE_PARAM);
	} else {
		SYS_LOG_ERR("Already connected");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[13];
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)) {
		return;
	}

	if (ble_info.ble_conn == conn) {
		os_delayed_work_cancel(&ble_info.param_update_work);

		hex_to_str(addr, ble_info.device_mac, 6);
		SYS_LOG_INF("MAC: %s, reason: %d", addr, reason);

		hostif_bt_conn_unref(ble_info.ble_conn);
		ble_info.ble_conn = NULL;

		notify_ble_connected(ble_info.device_mac, false);
		os_sem_give(&ble_ind_sem);

		/* param.options set BT_LE_ADV_OPT_ONE_TIME,
		 * need advertise again after disconnect.
		 */
#ifndef GONFIG_GMA_APP
		ble_advertise();
#endif

	} else {
		SYS_LOG_ERR("Error conn %p(%p)", ble_info.ble_conn, conn);
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	SYS_LOG_INF("int (0x%04x, 0x%04x) lat %d to %d", param->interval_min,
		param->interval_max, param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	SYS_LOG_INF("int 0x%x lat %d to %d", interval, latency, timeout);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

static int ble_notify_data(struct bt_gatt_attr *attr, u8_t *data, u16_t len)
{
	int ret;

	ret = hostif_bt_gatt_notify(ble_info.ble_conn, attr, data, len);
	if (ret < 0) {
		return ret;
	} else {
		return (int)len;
	}
}

static void ble_indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  u8_t err)
{
	os_sem_give(&ble_ind_sem);
}

static int ble_indicate_data(struct bt_gatt_attr *attr, u8_t *data, u16_t len)
{
	int ret;

	if (os_sem_take(&ble_ind_sem, K_NO_WAIT) < 0) {
		return -EBUSY;
	}

	ble_ind_params.attr = attr;
	ble_ind_params.func = ble_indicate_cb;
	ble_ind_params.len = len;
	ble_ind_params.data = data;

	ret = hostif_bt_gatt_indicate(ble_info.ble_conn, &ble_ind_params);
	if (ret < 0) {
		return ret;
	} else {
		return (int)len;
	}
}

u16_t bt_manager_get_ble_mtu(void)
{
	return (ble_info.ble_conn) ? hostif_bt_gatt_get_mtu(ble_info.ble_conn) : 0;
}

int bt_manager_ble_send_data(struct bt_gatt_attr *chrc_attr,
					struct bt_gatt_attr *des_attr, u8_t *data, u16_t len)
{
	struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)(chrc_attr->user_data);

	if (!ble_info.ble_conn) {
		return -EIO;
	}

	if (len > (bt_manager_get_ble_mtu() - 3)) {
		return -EFBIG;
	}

	ble_send_data_check_interval();

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		return ble_notify_data(des_attr, data, len);
	} else if (chrc->properties & BT_GATT_CHRC_INDICATE) {
		return ble_indicate_data(des_attr, data, len);
	}

	/* Wait TODO */
	/* return ble_write_data(attr, data, len) */
	SYS_LOG_WRN("Wait todo");
	return -EIO;
}

void bt_manager_ble_disconnect(void)
{
	int err;

	if (!ble_info.ble_conn) {
		return;
	}

	hostif_bt_conn_ref(ble_info.ble_conn);
	err = hostif_bt_conn_disconnect(ble_info.ble_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		SYS_LOG_INF("Disconnection failed (err %d)", err);
	}
	hostif_bt_conn_unref(ble_info.ble_conn);
}

void bt_manager_ble_service_reg(struct ble_reg_manager *le_mgr)
{
	os_mutex_lock(&ble_mgr_lock, K_FOREVER);

	sys_slist_append(&ble_list, &le_mgr->node);
	hostif_bt_gatt_service_register(&le_mgr->gatt_svc);

	os_mutex_unlock(&ble_mgr_lock);
}

int bt_manager_ble_set_idle_interval(u16_t interval)
{
	/* Range: 0x0006 to 0x0C80 */
	if (interval < 0x0006 || interval > 0x0C80) {
		return -EIO;
	}

	ble_idle_interval = interval;
	if (ble_info.update_work_state == PARAM_UPDATE_IDLE_STATE) {
		os_delayed_work_submit(&ble_info.param_update_work, BLE_DELAY_UPDATE_PARAM_TIME);
	}
	return 0;
}

void bt_manager_ble_init(void)
{
	memset(&ble_info, 0, sizeof(ble_info));
	ble_info.ble_current_interval = BLE_NOINIT_INTERVAL;
	ble_idle_interval = BLE_DEFAULT_IDLE_INTERVAL;

	sys_slist_init(&ble_list);
	os_delayed_work_init(&ble_info.param_update_work, param_update_work_callback);
	os_sem_init(&ble_ind_sem, 1, 1);

	hostif_bt_conn_cb_register(&conn_callbacks);
#ifndef GONFIG_GMA_APP
	ble_advertise();
#endif
}

void bt_manager_ble_a2dp_play_notify(bool play)
{
	ble_info.br_a2dp_runing = (play) ? 1 : 0;
	ble_check_update_param();
}

void bt_manager_ble_hfp_play_notify(bool play)
{
	ble_info.br_hfp_runing = (play) ? 1 : 0;
	ble_check_update_param();
}

void bt_manager_halt_ble(void)
{
	/* If ble connected, what todo */

	if (!ble_info.ble_conn) {
		hostif_bt_le_adv_stop();
	}
}

void bt_manager_resume_ble(void)
{
	/* If ble connected, what todo */

	if (!ble_info.ble_conn) {
		ble_advertise();
	}
}
