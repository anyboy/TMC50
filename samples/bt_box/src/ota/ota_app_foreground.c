/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <thread_timer.h>
#include <misc/util.h>
#include <logging/sys_log.h>
#include <mem_manager.h>
#include <global_mem.h>
#include <msg_manager.h>
#include <app_defines.h>
#include <bt_manager.h>
#include <app_manager.h>
#include <app_switch.h>
#include <srv_manager.h>
#include <sys_manager.h>
#include <sys_event.h>
#include <sys_wakelock.h>
#include <ota/ota_upgrade.h>
#include <ota/ota_backend.h>
#include <ota/ota_backend_sdcard.h>
#include <ota/ota_backend_bt.h>
#include <ota_app.h>
#include <ui_manager.h>
#include "flash.h"
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#ifdef CONFIG_OTA_RECOVERY
#define OTA_STORAGE_DEVICE_NAME		CONFIG_OTA_TEMP_PART_DEV_NAME
#else
#define OTA_STORAGE_DEVICE_NAME		CONFIG_XSPI_NOR_ACTS_DEV_NAME
#endif

static struct ota_upgrade_info *g_ota;
#ifdef CONFIG_OTA_BACKEND_SDCARD
static struct ota_backend *backend_sdcard;
#endif
#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
static struct ota_backend *backend_bt;
#endif
static bool is_sd_ota;

static void ota_app_start(void)
{
	struct app_msg msg = {0};

	SYS_LOG_INF("ota app start");

	struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);

	msg.type = MSG_START_APP;
	msg.ptr = APP_ID_OTA;
	msg.reserve = APP_SWITCH_CURR;
	send_async_msg(APP_ID_MAIN, &msg);
	sys_wake_lock(WAKELOCK_OTA);
}

static void ota_app_stop(void)
{
	struct app_msg msg = {0};

	SYS_LOG_INF("ok");

	struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, true);

	msg.type = MSG_START_APP;
	msg.ptr = NULL;
	msg.reserve = APP_SWITCH_LAST;
	send_async_msg(APP_ID_MAIN, &msg);
	sys_wake_unlock(WAKELOCK_OTA);
}
static bool check_sd_ota_restart(void)
{
#ifdef CONFIG_PROPERTY
	char sd_ota_flag[4] = { 0 };

	property_get("SD_OTA_FLAG", sd_ota_flag, 4);
	if (!memcmp(sd_ota_flag, "yes", strlen("yes"))) {
		property_set("SD_OTA_FLAG", "no", 4);
		return true;
	}
#endif
	return false;
}
extern void ota_app_backend_callback(struct ota_backend *backend, int cmd, int state)
{
	int err;

	SYS_LOG_INF("backend %p cmd %d state %d", backend, cmd, state);

	if (!strcmp(APP_ID_BTCALL, app_manager_get_current_app())) {
		SYS_LOG_WRN("btcall unsupport ota, skip...");
		return;
	}
	if (cmd == OTA_BACKEND_UPGRADE_STATE) {
		if (state == 1) {
			SYS_LOG_INF("bt_manager_allow_sco_connect false\n");
			bt_manager_allow_sco_connect(false);
			err = ota_upgrade_attach_backend(g_ota, backend);
			if (err) {
				SYS_LOG_INF("unable attach backend");
				return;
			}
			if (backend != backend_bt) {
				if (check_sd_ota_restart()) {
					SYS_LOG_INF("sd ota restart\n");
					return;
				}
				is_sd_ota = true;
			} else {
				is_sd_ota = false;
			}
			ota_app_start();
		} else {
			ota_upgrade_detach_backend(g_ota, backend);
			SYS_LOG_INF("bt_manager_allow_sco_connect true\n");
			bt_manager_allow_sco_connect(true);
		}
	} else if (cmd == OTA_BACKEND_UPGRADE_PROGRESS) {
		ota_view_show_upgrade_progress(state);
	}
}

#ifdef CONFIG_OTA_BACKEND_SDCARD
struct ota_backend_sdcard_init_param sdcard_init_param = {
	.fpath = "SD:ota.bin",
};

int ota_app_init_sdcard(void)
{
	backend_sdcard = ota_backend_sdcard_init(ota_app_backend_callback, &sdcard_init_param);
	if (!backend_sdcard) {
		SYS_LOG_INF("failed to init sdcard ota");
		return -ENODEV;
	}

	return 0;
}
#endif

#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
/* UUID order using BLE format */
/*static const u8_t ota_spp_uuid[16] = {0x00,0x00,0x66,0x66, \
	0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB};*/

/* "00006666-0000-1000-8000-00805F9B34FB"  ota uuid spp */
static const u8_t ota_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00};

/* BLE */
/*	"e49a25f8-f69a-11e8-8eb2-f2801f1b9fd1" reverse order  */
#define OTA_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xf8, 0x25, 0x9a, 0xe4)

/* "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_RX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4)

/* "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_TX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4)

static struct bt_gatt_ccc_cfg g_ota_ccc_cfg[1];

static struct bt_gatt_attr ota_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(OTA_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(OTA_CHA_RX_UUID, BT_GATT_CHRC_WRITE_WITHOUT_RESP|BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(OTA_CHA_RX_UUID, BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(OTA_CHA_TX_UUID, BT_GATT_CHRC_NOTIFY|BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(OTA_CHA_TX_UUID, BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(g_ota_ccc_cfg, NULL)
};


static const struct ota_backend_bt_init_param bt_init_param = {
	.spp_uuid = ota_spp_uuid,
	.gatt_attr = &ota_gatt_attr[0],
	.attr_size = ARRAY_SIZE(ota_gatt_attr),
	.tx_chrc_attr = &ota_gatt_attr[3],
	.tx_attr = &ota_gatt_attr[4],
	.tx_ccc_attr = &ota_gatt_attr[5],
	.rx_attr = &ota_gatt_attr[2],
	.read_timeout = K_FOREVER,	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	.write_timeout = K_FOREVER,
};

int ota_app_init_bluetooth(void)
{
	SYS_LOG_INF("spp uuid\n");
	print_buffer(bt_init_param.spp_uuid, 1, 16, 16, 0);

	backend_bt = ota_backend_bt_init(ota_app_backend_callback,
		(struct ota_backend_bt_init_param *)&bt_init_param);
	if (!backend_bt) {
		SYS_LOG_INF("failed");
		return -ENODEV;
	}

	return 0;
}
#endif

static void sys_reboot_by_ota(void)
{
	struct app_msg  msg = {0};
	msg.type = MSG_REBOOT;
	msg.cmd = REBOOT_REASON_OTA_FINISHED;
	send_async_msg("main", &msg);
}

void ota_app_notify(int state, int old_state)
{
	SYS_LOG_INF("ota state: %d->%d", old_state, state);

	if (old_state != OTA_RUNNING && state == OTA_RUNNING) {
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
	#endif
	} else if (old_state == OTA_RUNNING && state != OTA_RUNNING) {
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
	#endif
	}
	if (state == OTA_DONE) {
		ota_view_show_upgrade_result("Succ", false);
		if (is_sd_ota) {
		#ifdef CONFIG_PROPERTY
			property_set("SD_OTA_FLAG", "yes", 4);
		#endif
		}
		os_sleep(1000);
		sys_reboot_by_ota();
	}
}

int ota_app_init(void)
{
	struct ota_upgrade_param param;

	SYS_LOG_INF("device name %s ", OTA_STORAGE_DEVICE_NAME);

	memset(&param, 0x0, sizeof(struct ota_upgrade_param));

	param.storage_name = OTA_STORAGE_DEVICE_NAME;
	param.notify = ota_app_notify;
#ifdef CONFIG_OTA_RECOVERY
	param.flag_use_recovery = 1;
	param.flag_use_recovery_app = 0;
#endif
	param.no_version_control = 1;

	struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);

	g_ota = ota_upgrade_init(&param);
	if (!g_ota) {
		SYS_LOG_INF("init failed");
		if (flash_device)
			flash_write_protection_set(flash_device, true);
		return -1;
	}

	if (flash_device)
		flash_write_protection_set(flash_device, true);
	return 0;
}

static void ota_app_start_ota_upgrade(void)
{
	struct app_msg msg = {0};

	msg.type = MSG_START_APP;
	send_async_msg(APP_ID_OTA, &msg);
}

static void ota_app_main(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;

	SYS_LOG_INF("enter");
	ota_view_init();

	app_switch_lock(1);
	ota_app_start_ota_upgrade();

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			int result = 0;

			switch (msg.type) {
			case MSG_START_APP:
				if (ota_upgrade_check(g_ota)) {
					ota_view_show_upgrade_result("Fail", true);
				} else {
					ota_view_show_upgrade_result("Succ", false);
				}
				app_switch_unlock(1);
				ota_app_stop();
				break;

			case MSG_EXIT_APP:
				terminaltion = true;
				ota_view_deinit();
				app_manager_thread_exit(APP_ID_OTA);
				break;

			default:
				SYS_LOG_ERR("unknown: 0x%x!", msg.type);
				break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}

		if (!terminaltion) {
		    thread_timer_handle_expired();
		}
	}
}

APP_DEFINE(ota, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   ota_app_main, NULL);

