/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA bluetooth backend interface
 */

#include <kernel.h>
#include <string.h>
#include <stream.h>
#include <soc.h>
#include <logging/sys_log.h>
#include <mem_manager.h>
#include <ota/ota_backend_bt.h>

//#define ASDP_SPPBLE_TEST

//#define print_hex(str, buf, size) do {printk("%s\n", str); print_buffer(buf, 1, size, 16, 0);} while(0)
#define print_hex(str, buf, size) do {} while(0)


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


static io_stream_t sppble_stream = NULL;
static bool stream_opened=false;

int asdp_sppble_get_data_len(void)
{
	int read_size;

	if(!stream_opened) {
		return 0;
	}

	if (NULL != sppble_stream) {
		read_size = stream_tell(sppble_stream);
		SYS_LOG_INF("len=%d.", read_size);
		if ( read_size > 0){
			return read_size;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

int asdp_sppble_read_data(u8_t *buf, int size)
{
	int read_size;
	int len;

	len = asdp_sppble_get_data_len();
	if(len > size) {
		len = size;
	}

	if(len <= 0) {
		return 0;
	}

	read_size = stream_read(sppble_stream, buf, len);
	SYS_LOG_INF("Read sppble data, len=%d.", read_size);
	if (read_size > 0) {
		SYS_LOG_INF("Got sppble data.");
		print_hex("SPPBLE read: ", buf, read_size);
	}

	return read_size;
}

int asdp_sppble_write_data(u8_t *buf, int size)
{
	int err;

	print_hex("SPPBLE write: ", buf, size);
	err = stream_write(sppble_stream, buf, size);
	if (err < 0) {
		SYS_LOG_ERR("Failed to write data, size=%d, err=%d", size, err);
		return -EIO;
	}

	return 0;
}

static void asdp_sppble_connect_cb(int connected)
{
	int err;

	SYS_LOG_INF("connect: %d", connected);
	if(connected) {
		SYS_LOG_INF("To open sppble stream");
		if (NULL != sppble_stream) {
			err = stream_open(sppble_stream, MODE_IN_OUT);
			if (err) {
				SYS_LOG_ERR("Failed to open stream.");
			} else {
				SYS_LOG_INF("Open stream successfully.");
				stream_opened = true;
			}
		}

	}
}

int asdp_ota_spp_init(void)
{
	struct sppble_stream_init_param init_param;

	init_param.spp_uuid = ota_spp_uuid,
	init_param.attr_size = 4;
	init_param.le_gatt_attr = &ota_ble_gatt_attr[0],
	init_param.tx_attr = &ota_ble_gatt_attr[1],
	init_param.ccc_attr = &ota_ble_gatt_attr[2],
	init_param.rx_attr = &ota_ble_gatt_attr[3],

	init_param.connect_cb = asdp_sppble_connect_cb;
	init_param.read_timeout = K_FOREVER;
	init_param.write_timeout = K_FOREVER;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	SYS_LOG_INF("To create sppble stream");
	sppble_stream = stream_create(TYPE_SPPBLE_STREAM, &init_param);
	if (NULL == sppble_stream) {
		SYS_LOG_ERR("stream_create failed");
		return -EIO;
	}


	return 0;
}

int asdp_ota_spp_deinit(void)
{
	int err;

	if (NULL != sppble_stream) {
		err = stream_close(sppble_stream);
		if (err) {
			SYS_LOG_ERR("stream_close Failed");
		}else{
			stream_destroy(sppble_stream);
		}
	}

	return 0;
}

#ifdef ASDP_SPPBLE_TEST
static struct _k_thread_stack_element __aligned(STACK_ALIGN) asdp_stack[0x800];
static void asdp_ota_task(void *parama1, void *parama2, void *parama3)
{
	u8_t buf[128];

	while(1){
		if (stream_opened) {

			asdp_sppble_read_data(buf, 128);
		}
		k_sleep(1000);
	}
}

int asdp_ota_test(void)
{
	asdp_ota_spp_init();
	os_thread_create((char*)asdp_stack,
			sizeof
			(asdp_stack),
			asdp_ota_task,
			NULL, NULL, NULL,
			K_LOWEST_THREAD_PRIO
			- 1, 0,
			OS_NO_WAIT);

	return 0;
}
#endif
