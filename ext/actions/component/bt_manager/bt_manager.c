/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

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
#include <dvfs.h>
#include <bluetooth/host_interface.h>
#include <property_manager.h>

#ifdef CONFIG_GMA_APP
extern void gma_ble_advertise(void);
#endif

#define MAX_MGR_DEV 	CONFIG_BT_MAX_CONN

struct bt_mgr_dev_info {
	bd_address_t addr;
	u16_t used:1;
	u16_t notify_connected:1;
	u16_t is_tws:1;
	u16_t hf_connected:1;
	u16_t a2dp_connected:1;
	u16_t avrcp_connected:1;
	u16_t spp_connected:3;
	u16_t hid_connected:1;
	u8_t *name;
};

struct bt_manager_info_t {
	u16_t connected_phone_num:2;
	u16_t tws_mode:1;
	u16_t playing:1;
	u16_t inited:1;
	u16_t bt_state;
	u8_t dis_reason;
	struct bt_mgr_dev_info dev[MAX_MGR_DEV];
	bd_address_t halt_addr[MAX_MGR_DEV];
};

static struct bt_manager_info_t bt_mgr_info;

static const struct bt_scan_parameter default_scan_param = {
	.tws_limit_inquiry = 1,		/* 0: Normal inquiry,  other: limit inquiry */
	.idle_extend_windown = 0,
	.inquiry_interval = 0x1000,		/* Spec default value 0x1000 */
	.inquiry_windown = 0x0024,		/* Spec default value 0x12 */
	.page_interval = 0x0800,		/* Spec default value 0x0800 */
	.page_windown = 0x0024,			/* Spec default value 0x12 */
};

static const struct bt_scan_parameter enhance_scan_param = {
	.tws_limit_inquiry = 1,		/* 0: Normal inquiry,  other: limit inquiry */
	.idle_extend_windown = 0,
	.inquiry_interval = 0x0200,
	.inquiry_windown = 0x0060,
	.page_interval = 0x0480,
	.page_windown = 0x0080,
};

static void bt_mgr_set_config_info(void)
{
	struct btsrv_config_info cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.max_conn_num = CONFIG_BT_MAX_BR_CONN;
	cfg.max_phone_num = bt_manager_config_connect_phone_num();
	cfg.pts_test_mode = bt_manager_config_pts_test() ? 1 : 0;
	cfg.volume_sync_delay_ms = bt_manager_config_volume_sync_delay_ms();
	cfg.tws_version = get_tws_current_versoin();
	cfg.tws_feature = get_tws_current_feature();
#ifdef CONFIG_BT_DOUBLE_PHONE_PREEMPTION_MODE
	cfg.double_preemption_mode = 1;
#else
	cfg.double_preemption_mode = 0;
#endif
	btif_base_set_config_info(&cfg);
}

/* User can change to different scan param at any time after bt ready.
 * btif_br_set_connnectable(false);
 * btif_br_set_discoverable(false);
 * btif_br_set_scan_param(&scan_param, true);		// new parameter
 * btif_br_set_discoverable(true);
 * btif_br_set_connnectable(true);
 */
static void bt_mgr_set_scan_param(void)
{
	struct bt_scan_parameter param;

	/* If not set default_scan_param, bt service will used default param from spec */
	memcpy(&param, &default_scan_param, sizeof(param));
	param.tws_limit_inquiry = bt_manager_config_get_tws_limit_inquiry();
	param.idle_extend_windown = bt_manager_config_get_idle_extend_windown();
	btif_br_set_scan_param(&param, false);

	memcpy(&param, &enhance_scan_param, sizeof(param));
	param.tws_limit_inquiry = bt_manager_config_get_tws_limit_inquiry();
	param.idle_extend_windown = bt_manager_config_get_idle_extend_windown();
	btif_br_set_scan_param(&param, true);
}

static void bt_mgr_btsrv_ready(void)
{
	SYS_LOG_INF("btsrv ready\n");
	bt_mgr_set_config_info();
	bt_mgr_set_scan_param();
	btif_br_set_connnectable(true);
	btif_br_set_discoverable(true);
#ifdef CONFIG_BT_A2DP
	bt_manager_a2dp_profile_start();
#endif
#ifdef CONFIG_BT_AVRCP
	bt_manager_avrcp_profile_start();
#endif
#ifdef CONFIG_BT_HFP_HF
	bt_manager_hfp_sco_start();
	bt_manager_hfp_profile_start();
#endif
#ifdef CONFIG_BT_HFP_AG
	bt_manager_hfp_ag_profile_start();
#endif

#ifdef CONFIG_BT_SPP
	bt_manager_spp_profile_start();
#endif

#ifdef CONFIG_BT_HID
	bt_manager_hid_register_sdp();
	bt_manager_hid_profile_start();
	bt_manager_did_register_sdp();
#endif

#ifdef CONFIG_BT_BLE
	bt_manager_ble_init();
#endif
	/* TODO: fixed me, temp to send message to main */
	struct app_msg  msg = {0};

	msg.type = MSG_BT_ENGINE_READY;
	send_async_msg("main", &msg);

#ifndef CONFIG_CASCADE
	bt_manager_startup_reconnect();
#endif
}

static void bt_mgr_add_dev_info(bd_address_t *addr)
{
	int i;

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && !memcmp(&bt_mgr_info.dev[i].addr, addr, sizeof(bd_address_t))) {
			SYS_LOG_WRN("Already exist!\n");
			return;
		}
	}

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used == 0) {
			bt_mgr_info.dev[i].used = 1;
			memcpy(&bt_mgr_info.dev[i].addr, addr, sizeof(bd_address_t));
			return;
		}
	}

	SYS_LOG_WRN("Without new dev info!\n");
}

static void bt_mgr_free_dev_info(struct bt_mgr_dev_info *info)
{
	memset(info, 0, sizeof(struct bt_mgr_dev_info));
}

static struct bt_mgr_dev_info *bt_mgr_find_dev_info(bd_address_t *addr)
{
	int i;

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && !memcmp(&bt_mgr_info.dev[i].addr, addr, sizeof(bd_address_t))) {
			return &bt_mgr_info.dev[i];
		}
	}

	return NULL;
}

static void bt_mgr_notify_connected(struct bt_mgr_dev_info *info)
{
	if (info->is_tws || info->notify_connected) {
		/* Tws not notify in here, or already notify */
		return;
	}

	SYS_LOG_INF("btsrv connected:%s\n", (char *)info->name);
	info->notify_connected = 1;
	bt_manager_set_status(BT_STATUS_CONNECTED);

	/* Advise not to set, just let phone make dicision. */
	//btif_br_set_phone_controler_role(&info->addr, CONTROLER_ROLE_MASTER);	/* Set phone controler as master */
	btif_br_set_phone_controler_role(&info->addr, CONTROLER_ROLE_SLAVE);		/* Set phone controler as slave */
}

static void bt_mgr_check_disconnect_notify(struct bt_mgr_dev_info *info, u8_t reason)
{
	if (info->is_tws) {
		/* Tws not notify in here */
		return;
	}

	if (info->notify_connected) {
		SYS_LOG_INF("btsrv disconnected reason %d\n", reason);
		info->notify_connected = 0;
		bt_mgr_info.dis_reason = reason;		/* Transfer to BT_STATUS_DISCONNECTED */
		bt_manager_set_status(BT_STATUS_DISCONNECTED);
	}
}

/* return 0: accept connect request, other: rejuect connect request
 * Direct callback from bt stack, can't do too much thing in this function.
 */
static int bt_mgr_check_connect_req(struct bt_link_cb_param *param)
{
	if (param->new_dev) {
		SYS_LOG_INF("New connect request\n");
	} else {
		SYS_LOG_INF("%s connect request\n", param->is_tws ? "Tws" : "Phone");
	}

	return 0;
}

/* Sample code, just for reference */
#ifdef CONFIG_BT_DOUBLE_PHONE_EXT_MODE
#define SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV		1
#else
#define SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV		0
#endif

#if SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV
static void bt_mgr_check_disconnect_nonactive_dev(struct bt_mgr_dev_info *info)
{
	int i, phone_cnt = 0, tws_cnt = 0;
	bd_address_t a2dp_active_addr;
	struct bt_mgr_dev_info *exp_disconnect_info = NULL;

	btif_a2dp_get_active_mac(&a2dp_active_addr);

	for (i = 0; ((i < MAX_MGR_DEV) && bt_mgr_info.dev[i].used); i++) {
		if (bt_mgr_info.dev[i].is_tws) {
			tws_cnt++;
		} else {
			phone_cnt++;
			if (memcmp(&bt_mgr_info.dev[i].addr, &a2dp_active_addr, sizeof(bd_address_t)) &&
				memcmp(&bt_mgr_info.dev[i].addr, &info->addr, sizeof(bd_address_t))) {
				exp_disconnect_info = &bt_mgr_info.dev[i];
			}
		}
	}

	/* Tws paired */
	if (tws_cnt) {
		if (phone_cnt >= 2) {
			bt_manager_br_disconnect(&info->addr);
		}
		return;
	}

	if (phone_cnt >= 3) {
		if (exp_disconnect_info) {
			bt_manager_br_disconnect(&exp_disconnect_info->addr);
		}
	}
}
#endif

static int bt_mgr_link_event(void *param)
{
	int ret = 0;
	struct bt_mgr_dev_info *info;
	struct bt_link_cb_param *in_param = param;

	SYS_LOG_INF("Link event(%d) %02x:%02x:%02x:%02x:%02x:%02x\n", in_param->link_event,
			in_param->addr->val[5], in_param->addr->val[4], in_param->addr->val[3],
			in_param->addr->val[2], in_param->addr->val[1], in_param->addr->val[0]);

	info = bt_mgr_find_dev_info(in_param->addr);
	if ((info == NULL) && (in_param->link_event != BT_LINK_EV_ACL_CONNECTED) &&
		(in_param->link_event != BT_LINK_EV_ACL_CONNECT_REQ)) {
		SYS_LOG_WRN("Already free %d\n", in_param->link_event);
		return ret;
	}

	switch (in_param->link_event) {
	case BT_LINK_EV_ACL_CONNECT_REQ:
		ret = bt_mgr_check_connect_req(in_param);
		break;
	case BT_LINK_EV_ACL_CONNECTED:
		bt_mgr_add_dev_info(in_param->addr);
		break;
	case BT_LINK_EV_ACL_DISCONNECTED:
		bt_mgr_check_disconnect_notify(info, in_param->reason);
		bt_mgr_free_dev_info(info);
		break;
	case BT_LINK_EV_GET_NAME:
		info->name = in_param->name;
		info->is_tws = in_param->is_tws;
#if SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV
		bt_mgr_check_disconnect_nonactive_dev(info);
#endif
		break;
	case BT_LINK_EV_HF_CONNECTED:
		bt_mgr_notify_connected(info);
		info->hf_connected = 1;
		break;
	case BT_LINK_EV_HF_DISCONNECTED:
		info->hf_connected = 0;
		break;
	case BT_LINK_EV_A2DP_CONNECTED:
		info->a2dp_connected = 1;
		bt_mgr_notify_connected(info);
		break;
	case BT_LINK_EV_A2DP_DISCONNECTED:
		info->a2dp_connected = 0;
		break;
	case BT_LINK_EV_AVRCP_CONNECTED:
		info->avrcp_connected = 1;
		break;
	case BT_LINK_EV_AVRCP_DISCONNECTED:
		info->avrcp_connected = 0;
		break;
	case BT_LINK_EV_SPP_CONNECTED:
		info->spp_connected++;
		break;
	case BT_LINK_EV_SPP_DISCONNECTED:
		if (info->spp_connected) {
			info->spp_connected--;
		}
		break;
	case BT_LINK_EV_HID_CONNECTED:
		info->hid_connected = 1;
		break;
	case BT_LINK_EV_HID_DISCONNECTED:
		info->hid_connected = 0;
		break;
	default:
		break;
	}

	return ret;
}

/* Return 0: phone device; other : tws device
 * Direct callback from bt stack, can't do too much thing in this function.
 */
static int bt_mgr_check_new_device_role(void *param)
{
	struct btsrv_check_device_role_s *cb_param = param;
	u8_t pre_mac[3];
	u8_t name[33];

	if (bt_manager_config_get_tws_compare_high_mac()) {
		bt_manager_config_set_pre_bt_mac(pre_mac);
		if (cb_param->addr.val[5] != pre_mac[0] ||
			cb_param->addr.val[4] != pre_mac[1] ||
			cb_param->addr.val[3] != pre_mac[2]) {
			return 0;
		}
	}

#ifdef CONFIG_PROPERTY
	memset(name, 0, sizeof(name));
	property_get(CFG_BT_NAME, name, 32);
	if (strlen(name) != cb_param->len || memcmp(cb_param->name, name, cb_param->len)) {
		return 0;
	}
#endif

	return 1;
}

static int _bt_mgr_callback(btsrv_event_e event, void *param)
{
	int ret = 0;

	switch (event) {
	case BTSRV_READY:
		bt_mgr_btsrv_ready();
		break;
	case BTSRV_LINK_EVENT:
		ret = bt_mgr_link_event(param);
		break;
	case BTSRV_DISCONNECTED_REASON:
		bt_manager_disconnected_reason(param);
		break;
	case BTSRV_REQ_HIGH_PERFORMANCE:
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "btmanager");
	#endif
	break;
	case BTSRV_RELEASE_HIGH_PERFORMANCE:
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "btmanager");
	#endif
		break;
	case BTSRV_REQ_FLUSH_PROPERTY:
		SYS_LOG_INF("Req flush %s\n", (char *)param);
#ifdef CONFIG_PROPERTY
		property_flush_req(param);
#endif
		break;
	case BTSRV_CHECK_NEW_DEVICE_ROLE:
		ret = bt_mgr_check_new_device_role(param);
		break;
	}

	return ret;
}

void bt_manager_record_halt_phone(void)
{
	u8_t i, record = 0;

	memset(bt_mgr_info.halt_addr, 0, sizeof(bt_mgr_info.halt_addr));
	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && !bt_mgr_info.dev[i].is_tws &&
			(bt_mgr_info.dev[i].a2dp_connected || bt_mgr_info.dev[i].hf_connected)) {
			memcpy(&bt_mgr_info.halt_addr[record], &bt_mgr_info.dev[i].addr, sizeof(bd_address_t));
			record++;
		}
	}
}

void *bt_manager_get_halt_phone(u8_t *halt_cnt)
{
	*halt_cnt = MAX_MGR_DEV;
	return bt_mgr_info.halt_addr;
}

int bt_manager_get_status(void)
{
	return bt_mgr_info.bt_state;
}

int bt_manager_get_connected_dev_num(void)
{
	return bt_mgr_info.connected_phone_num;
}

#include "app_switch.h"
#include "app_manager.h"
#define APP_ID_BTMUSIC			"btmusic"
#define APP_ID_USOUND			"usound"

//output status to uart
extern void system_app_uart_tx(u8_t *buf, size_t len);

int bt_manager_set_status(int state)
{
	switch (state) {
	case BT_STATUS_CONNECTED:
	{
		bt_mgr_info.connected_phone_num++;
		if (bt_mgr_info.connected_phone_num == 1) {
			sys_event_notify(SYS_EVENT_BT_CONNECTED);
			bt_manager_event_notify(BT_CONNECTION_EVENT, NULL, 0);
			bt_mgr_info.bt_state = BT_STATUS_PAUSED;
#ifdef CONFIG_GMA_APP
			hostif_bt_le_adv_stop();
			gma_ble_advertise();
#endif
#ifdef CONFIG_CASCADE
		if (bt_manager_tws_get_dev_role() == BTSRV_TWS_NONE)
			bt_manager_tws_wait_pair();


		//printk(" cur app:%s, \n", (char *)app_manager_get_current_app());
		//if (!strcmp(APP_ID_BTMUSIC, app_manager_get_current_app())) {
		//	app_switch_lock(1);
		//}
		app_switch(APP_ID_BTMUSIC, APP_SWITCH_NEXT, false);
		app_switch_lock(1);
#endif
		} else {
			sys_event_notify(SYS_EVENT_2ND_CONNECTED);
		}

		{
			u8_t inno_bt_con_code = 0xc1;
			system_app_uart_tx(&inno_bt_con_code, 1);
		}

		return 0;
	}
	case BT_STATUS_DISCONNECTED:
	{
		if (bt_mgr_info.connected_phone_num > 0) {
			bt_mgr_info.connected_phone_num--;
			if (bt_mgr_info.dis_reason != 0x16) {
				sys_event_notify(SYS_EVENT_BT_DISCONNECTED);
			}

			if (!bt_mgr_info.connected_phone_num) {
				sys_event_notify(SYS_EVENT_BT_UNLINKED);
				bt_manager_event_notify(BT_DISCONNECTION_EVENT, NULL, 0);

				{
					u8_t inno_bt_con_code = 0xc0;
					system_app_uart_tx(&inno_bt_con_code, 1);
				}
			}
		}

#ifdef CONFIG_GMA_APP
		if(bt_mgr_info.connected_phone_num == 0) {
			hostif_bt_le_adv_stop();
		}
#endif

#ifdef CONFIG_CASCADE
		if(bt_mgr_info.connected_phone_num == 0) {
			bt_manager_tws_disconnect();
		}

		if (!strcmp(APP_ID_BTMUSIC, app_manager_get_current_app())) {
			app_switch_unlock(1);
			app_switch(APP_ID_USOUND, APP_SWITCH_NEXT, false);
		}
#endif

		return 0;
	}
	case BT_STATUS_TWS_PAIRED:
	{
		if (!bt_mgr_info.tws_mode) {
			bt_mgr_info.tws_mode = 1;
			if (btif_tws_get_dev_role() == BTSRV_TWS_MASTER) {
				sys_event_notify(SYS_EVENT_TWS_CONNECTED);
			}
			bt_manager_event_notify(BT_TWS_CONNECTION_EVENT, NULL, 0);
		}
		return 0;
	}
	case BT_STATUS_TWS_UNPAIRED:
	{
		if (bt_mgr_info.tws_mode) {
			bt_mgr_info.tws_mode = 0;
			bt_manager_event_notify(BT_TWS_DISCONNECTION_EVENT, NULL, 0);
		}
		return 0;
	}

	case BT_STATUS_MASTER_WAIT_PAIR:
	{
		/* Check can_do_pair before call
		 * bt_manager_set_status(BT_STATUS_MASTER_WAIT_PAIR)
		 */
		sys_event_notify(SYS_EVENT_TWS_START_PAIR);
		break;
	}

	case BT_STATUS_PAUSED:
	{
		if (bt_mgr_info.playing) {
			bt_mgr_info.playing = 0;
		}

		break;
	}
	case BT_STATUS_PLAYING:
	{
		if (!bt_mgr_info.playing) {
			bt_mgr_info.playing = 1;
		}
		break;
	}
	default:
		break;
	}

	bt_mgr_info.bt_state = state;
	return 0;
}

void bt_manager_clear_list(int mode)
{
	btif_br_clear_list(mode);
	sys_event_notify(SYS_EVENT_CLEAR_PAIRED_LIST);
}

void bt_manager_set_stream_type(u8_t stream_type)
{
#ifdef CONFIG_TWS
	bt_manager_tws_set_stream_type(stream_type);
#endif
}

void bt_manager_set_codec(u8_t codec)
{
#ifdef CONFIG_TWS
	bt_manager_tws_set_codec(codec);
#endif
}

int bt_manager_br_start_discover(struct btsrv_discover_param *param)
{
	return btif_br_start_discover(param);
}

int bt_manager_br_stop_discover(void)
{
	return btif_br_stop_discover();
}

int bt_manager_br_connect(bd_address_t *bd)
{
	return btif_br_connect(bd);
}

int bt_manager_br_disconnect(bd_address_t *bd)
{
	return btif_br_disconnect(bd);
}

int bt_manager_init(void)
{
	int ret = 0;

	memset(&bt_mgr_info, 0, sizeof(struct bt_manager_info_t));

	bt_manager_check_mac_name();

	bt_manager_set_status(BT_STATUS_NONE);

	btif_base_register_processer();
#ifdef CONFIG_BT_HFP_HF
	btif_hfp_register_processer();
#endif
#ifdef CONFIG_BT_HFP_AG
	btif_hfp_ag_register_processer();
#endif
#ifdef CONFIG_BT_A2DP
	btif_a2dp_register_processer();
#endif
#ifdef CONFIG_BT_AVRCP
	btif_avrcp_register_processer();
#endif
#ifdef CONFIG_BT_SPP
	btif_spp_register_processer();
#endif
#ifdef CONFIG_BT_PBAP_CLIENT
	btif_pbap_register_processer();
#endif
#ifdef CONFIG_BT_MAP_CLIENT
	btif_map_register_processer();
#endif
#if CONFIG_BT_HID
	btif_hid_register_processer();
#endif
#ifdef CONFIG_TWS
	btif_tws_register_processer();
#endif

	if (btif_start(_bt_mgr_callback, bt_manager_config_bt_class(), bt_manager_config_get_device_id()) < 0) {
		SYS_LOG_ERR("btsrv start error\n");
		ret = -EACCES;
		goto bt_start_err;
	}

#ifdef CONFIG_TWS
	bt_manager_tws_init();
#endif

	bt_manager_hfp_init();
	bt_manager_hfp_sco_init();
#ifdef CONFIG_BT_HFP_AG
	bt_manager_hfp_ag_init();
#endif

	bt_manager_set_status(BT_STATUS_WAIT_PAIR);

	bt_mgr_info.inited = 1;

	SYS_LOG_INF("done\n");
	return 0;

bt_start_err:
	return ret;
}

void bt_manager_deinit(void)
{
	int time_out = 0;

	btif_br_set_connnectable(false);
	btif_br_set_discoverable(false);

	btif_br_auto_reconnect_stop();
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);

	while (btif_br_get_connected_device_num() && time_out++ < 500) {
		os_sleep(10);
	}

#ifdef CONFIG_BT_SPP
	bt_manager_spp_profile_stop();
#endif

	bt_manager_a2dp_profile_stop();
	bt_manager_avrcp_profile_stop();
	bt_manager_hfp_profile_stop();
	bt_manager_hfp_sco_stop();
#ifdef CONFIG_BT_HFP_AG
	bt_manager_hfp_ag_profile_stop();
#endif

#ifdef CONFIG_BT_HID
	bt_manager_hid_profile_stop();
#endif

	/**
	 *  TODO: must clean btdrv /bt stack and bt service when bt manager deinit
	 *  enable this after all is work well.
	 */
	bt_mgr_info.inited = 0;
#if 0
	btif_stop();
#endif

	SYS_LOG_INF("done\n");
}

bool bt_manager_is_inited(void)
{
	return (bt_mgr_info.inited == 1);
}

void bt_manager_dump_info(void)
{
	int i;

	SYS_LOG_INF("Bt manager info\n");
	printk("num %d, tws_mode %d, bt_state %d, playing %d\n", bt_mgr_info.connected_phone_num,
		bt_mgr_info.tws_mode, bt_mgr_info.bt_state, bt_mgr_info.playing);
	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used) {
			printk("Dev name %s, tws %d (%d,%d,%d,%d,%d,%d)\n", bt_mgr_info.dev[i].name, bt_mgr_info.dev[i].is_tws,
				bt_mgr_info.dev[i].notify_connected, bt_mgr_info.dev[i].a2dp_connected, bt_mgr_info.dev[i].avrcp_connected,
				bt_mgr_info.dev[i].hf_connected, bt_mgr_info.dev[i].spp_connected, bt_mgr_info.dev[i].hid_connected);
		}
	}

	printk("\n");
	btif_dump_brsrv_info();
}
