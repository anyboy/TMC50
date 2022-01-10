#define SYS_LOG_DOMAIN "btbox_test_ble"

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
#include <sys_event.h>

/********************  ble manager test code  *******************************/

#define SPEED_BLE_SERVICE_UUID		BT_UUID_DECLARE_16(0xFFA0)
#define SPEED_BLE_WRITE_UUID		BT_UUID_DECLARE_16(0xFFA1)
#define SPEED_BLE_READ_UUID			BT_UUID_DECLARE_16(0xFFA2)

extern u8_t hci_set_acl_print_enable(u8_t enable);
static void test_start_stop_send_delaywork(void);

#define BLE_TEST_SEND_SIZE			200
#define BLE_TEST_WORK_INTERVAL		5		/* 5ms */

//static const char ble_tx_trigger[] = "1122334455";
static u8_t ble_tx_flag = 0;
static u8_t ble_notify_enable = 0;
static os_delayed_work ble_test_work;
u8_t send_buf[BLE_TEST_SEND_SIZE];

static struct bt_gatt_ccc_cfg g_speed_ccc_cfg[1];

static ssize_t speed_write_cb(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, u16_t len, u16_t offset,
			      u8_t flags)
{
	//speed_rx_data();
	return len;
}

static void speed_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	SYS_LOG_INF("value: %d", value);
	ble_notify_enable = (u8_t)value;
	test_start_stop_send_delaywork();
}

static struct bt_gatt_attr speed_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(SPEED_BLE_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(SPEED_BLE_WRITE_UUID, BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(SPEED_BLE_WRITE_UUID, BT_GATT_PERM_WRITE,
			   NULL, speed_write_cb, NULL),

	BT_GATT_CHARACTERISTIC(SPEED_BLE_READ_UUID, BT_GATT_CHRC_NOTIFY),		/* or BT_GATT_CHRC_INDICATE */
	BT_GATT_DESCRIPTOR(SPEED_BLE_READ_UUID, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   NULL, NULL, NULL),
	BT_GATT_CCC(g_speed_ccc_cfg, speed_ccc_cfg_changed)
};

static void ble_test_delaywork(os_work *work)
{
	static u32_t curr_time;
	static u32_t pre_time;
	static u32_t TxCount;
	u16_t mtu;
	static u8_t data = 0;
	u8_t i, j, repeat;
	int ret;

	if (ble_tx_flag && ble_notify_enable) {
		mtu = bt_manager_get_ble_mtu() - 3;
		mtu = (mtu > BLE_TEST_SEND_SIZE) ? BLE_TEST_SEND_SIZE : mtu;

		if (mtu <= 23) {
			repeat = 10;
		} else if (mtu <=  64) {
			repeat = 4;
		} else {
			repeat = 1;
		}

		for (j = 0; j < repeat; j++) {
			for (i = 0; i < mtu; i++) {
				send_buf[i] = data++;
			}

			ret = bt_manager_ble_send_data(&speed_attrs[3], &speed_attrs[4], send_buf, mtu);
			if (ret < 0) {
				break;
			}

			TxCount += mtu;
			curr_time = k_uptime_get_32();
			if ((curr_time - pre_time) >= 1000) {
				printk("Tx: %d byte\n", TxCount);
				TxCount = 0;
				pre_time = curr_time;
			}

			os_yield();
		}
	}
	os_delayed_work_submit(&ble_test_work, BLE_TEST_WORK_INTERVAL);
}

static void test_start_stop_send_delaywork(void)
{
	if (!ble_tx_flag) {
		ble_tx_flag = 1;
		os_delayed_work_init(&ble_test_work, ble_test_delaywork);
		os_delayed_work_submit(&ble_test_work, BLE_TEST_WORK_INTERVAL);
		SYS_LOG_INF("BLE tx start");
	} else {
		ble_tx_flag = 0;
		ble_notify_enable = 0;
		os_delayed_work_cancel(&ble_test_work);
		SYS_LOG_INF("BLE tx stop");
	}
}

static void test_ble_connect_cb(u8_t *mac, u8_t connected)
{
	SYS_LOG_INF("BLE %s", connected ? "connected" : "disconnected");
	SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	if (connected) {
		hci_set_acl_print_enable(0);
	} else {
		hci_set_acl_print_enable(1);
		ble_tx_flag = 0;
	}
}

static struct ble_reg_manager ble_speed_mgr = {
	.link_cb = test_ble_connect_cb,
};

extern void ble_test_init(void)
{
	ble_speed_mgr.gatt_svc.attrs = speed_attrs;
	ble_speed_mgr.gatt_svc.attr_count = ARRAY_SIZE(speed_attrs);

	bt_manager_ble_service_reg(&ble_speed_mgr);
}

