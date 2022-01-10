/* h4.c - H:4 UART based Bluetooth driver */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <ctype.h>

#include <zephyr.h>
#include <arch/cpu.h>

#include <init.h>
#include <uart.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <string.h>
#include <logging/sys_log.h>
#include <bluetooth/hci_driver.h>
#include <btdrv_api.h>

#define ACTIONS_RAW_HCI_DRIVER_TEST				0

#define HCIT_COMMAND             1
#define HCIT_ACLDATA             2
#define HCIT_SCODATA             3
#define HCIT_EVENT               4

static int acts_raw_hci_send(u8_t *buf, u16_t len, u8_t type)
{
	int ret = -EIO;;

	if (buf == NULL) {
		return ret;
	}

	switch (type) {
	case HCIT_COMMAND:
	case HCIT_ACLDATA:
	case HCIT_SCODATA:
		ret = btdrv_send(buf, (int)len, type);
		break;
	default:
		SYS_LOG_ERR("Error type %d\n", type);
		break;
	}

	return ret;
}

static void btdrv_raw_rx_cb(void *buf, int len, u8_t type)
{
	switch (type) {
	case HCIT_ACLDATA:
	case HCIT_SCODATA:
	case HCIT_EVENT:
#if ACTIONS_RAW_HCI_DRIVER_TEST
		bt_raw_hci_recv(buf, (u16_t)len, type);
#endif
		break;
	default:
		SYS_LOG_ERR("Error type %d\n", type);
		break;
	}
}

static int acts_raw_hci_open(void)
{
	SYS_LOG_INF("btdrv_init");
	return btdrv_init(btdrv_raw_rx_cb);
}

static int acts_raw_hci_close(void)
{
	SYS_LOG_INF("btdrv_deinit");
	btdrv_exit();
	return 0;
}

static struct bt_raw_hci_driver raw_drv = {
	.name		= "H:ACTS RAW",
	.bus		= BT_HCI_DRIVER_BUS_VIRTUAL,
	.open		= acts_raw_hci_open,
	.close		= acts_raw_hci_close,
	.send		= acts_raw_hci_send,
};

static int bt_acts_raw_init(struct device *unused)
{
	ARG_UNUSED(unused);
#if ACTIONS_RAW_HCI_DRIVER_TEST
	bt_raw_hci_driver_register(&raw_drv);
#else
	ARG_UNUSED(&raw_drv);
#endif
	return 0;
}

SYS_INIT(bt_acts_raw_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/********************** Actions raw hci test sample ******************************/
#if ACTIONS_RAW_HCI_DRIVER_TEST
#include <shell/shell.h>

static struct bt_raw_hci_driver *test_raw_hci_dev = NULL;
static u8_t hci_opened;

int bt_raw_hci_driver_register(const struct bt_raw_hci_driver *drv)
{
	test_raw_hci_dev = (struct bt_raw_hci_driver *)drv;
	return 0;
}

/* Call back in controler thread, can't process protocol in this thread */
int bt_raw_hci_recv(u8_t *buf, u16_t len, u8_t type)
{
	u16_t i;

	printk("HCI RX: %02x ", type);
	for (i = 0; i < len; i++) {
		printk("%02x ", buf[i]);
	}
	printk("\n");

	return 0;
}

static int shell_cmd_bthci_open(int argc, char *argv[])
{
	if (!test_raw_hci_dev || (hci_opened != 0)) {
		SYS_LOG_INF("Not register or already open\n");
		return -EIO;
	}

	test_raw_hci_dev->open();
	hci_opened = 1;
	return 0;
}

static int shell_cmd_bthci_close(int argc, char *argv[])
{
	if (!test_raw_hci_dev || (hci_opened != 1)) {
		SYS_LOG_INF("Not register or already Close\n");
		return -EIO;
	}

	test_raw_hci_dev->close();
	hci_opened = 2;		/* Current only support repeat open/close */
	return 0;
}

static int shell_cmd_bthci_send(int argc, char *argv[])
{
	const static u8_t hci_reset_cmd[] = {0x03, 0x0c, 0x00};

	if (!test_raw_hci_dev || (hci_opened != 1)) {
		SYS_LOG_INF("Not register or not open\n");
		return -EIO;
	}

	test_raw_hci_dev->send((u8_t *)hci_reset_cmd, sizeof(hci_reset_cmd), HCIT_COMMAND);
	return 0;
}

static const struct shell_cmd bthci_commands[] = {
	{ "open", shell_cmd_bthci_open, "Open bt hci" },
	{ "close", shell_cmd_bthci_close, "Close bt hci" },
	{ "send", shell_cmd_bthci_send, "Send hci data" },
	{ NULL, NULL, NULL}
};
SHELL_REGISTER("bthci", bthci_commands);
#endif
