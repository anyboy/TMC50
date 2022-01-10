#include <kernel.h>
#include <adc.h>
#include <init.h>
#include <uart.h>
#include <string.h>
#include <misc/byteorder.h>
#include <usb_device.h>
#include <usb_common.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_STUB_LEVEL
#define SYS_LOG_DOMAIN "usb/stub"
#include <logging/sys_log.h>

#include "usb_stub_descriptor.h"

#include <string.h>
#include <nvram_config.h>
#include <property_manager.h>

#define USB_STUB_ENABLED	BIT(0)
#define USB_STUB_CONFIGURED	BIT(1)

static u8_t usb_stub_state;
static char usb_stub_device_sn[20] = CONFIG_USB_STUB_DEVICE_SN;

static struct k_sem stub_write_sem;
static struct k_sem stub_read_sem;

static void stub_ep_write_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	SYS_LOG_DBG("stub_ep_write_cb");
	k_sem_give(&stub_write_sem);
}

static void stub_ep_read_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	k_sem_give(&stub_read_sem);
}

/* Describe EndPoints configuration */
static const struct usb_ep_cfg_data stub_ep_cfg[] = {
	{
		.ep_cb	= stub_ep_write_cb,
		.ep_addr = CONFIG_STUB_IN_EP_ADDR
	},
	{
		.ep_cb	= stub_ep_read_cb,
		.ep_addr = CONFIG_STUB_OUT_EP_ADDR
	},
};

bool usb_stub_enabled(void)
{
	return usb_stub_state & USB_STUB_ENABLED;
}

bool usb_stub_configured(void)
{
	return usb_stub_state & USB_STUB_CONFIGURED;
}

static void stub_status_callback(enum usb_dc_status_code status, u8_t *param)
{
	switch (status) {
	case USB_DC_ERROR:
		SYS_LOG_INF("USB device error");
		break;
	case USB_DC_RESET:
		SYS_LOG_INF("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		SYS_LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		usb_stub_state |= USB_STUB_CONFIGURED;
		SYS_LOG_INF("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		SYS_LOG_INF("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		SYS_LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		SYS_LOG_DBG("USB device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_INF("USB unknown state");
		break;
	}
}

static int stub_class_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	SYS_LOG_INF("bRequest 0x%x bmRequestType 0x%x len %d",
			setup->bRequest, setup->bmRequestType, *len);
	return 0;
}

static int stub_custom_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	SYS_LOG_INF("bRequest 0x%x bmRequestType 0x%x len %d",
			setup->bRequest, setup->bmRequestType, *len);
	return -1;
}

static int stub_vendor_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	SYS_LOG_INF("bRequest 0x%x bmRequestType 0x%x len %d",
			setup->bRequest, setup->bmRequestType, *len);
	return 0;
}


static const struct usb_cfg_data stub_config = {
	.usb_device_description = NULL,
	.cb_usb_status = stub_status_callback,
	.interface = {
		.class_handler  = stub_class_handle_req,
		.custom_handler = stub_custom_handle_req,
		.vendor_handler = stub_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(stub_ep_cfg),
	.endpoint = stub_ep_cfg,
};

int usb_stub_ep_read(u8_t *data_buffer, u32_t data_len, u32_t timeout)
{
	int read_bytes;
	int ret;

	while (data_len) {
		if (k_sem_take(&stub_read_sem, timeout) != 0) {
			usb_dc_ep_flush(CONFIG_STUB_OUT_EP_ADDR);
			SYS_LOG_ERR("timeout");
			return -ETIME;
		}

		ret = usb_read(CONFIG_STUB_OUT_EP_ADDR, data_buffer,
				data_len, &read_bytes);
		if (ret < 0) {
			usb_dc_ep_flush(CONFIG_STUB_OUT_EP_ADDR);
			SYS_LOG_ERR("%d", ret);
			return ret;
		}

		data_buffer += read_bytes;
		data_len -= read_bytes;
	}

	return 0;
}

int usb_stub_ep_write(u8_t *data_buffer, u32_t data_len, u32_t timeout)
{
	int write_bytes;
	int ret;
	u8_t need_zero = 0;
	u8_t zero;

	switch (usb_device_speed()) {
	case USB_SPEED_FULL:
		if (data_len % CONFIG_STUB_EP_MPS == 0) {
			need_zero = 1;
		}
		break;
	case USB_SPEED_HIGH:
		if (data_len % USB_MAX_HS_BULK_MPS == 0) {
			need_zero = 1;
		}
		break;
	default:
		break;
	}

	while (data_len) {
		ret = usb_write(CONFIG_STUB_IN_EP_ADDR, data_buffer, data_len,
				&write_bytes);
		if (ret < 0) {
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return ret;
		}

		if (k_sem_take(&stub_write_sem, timeout) != 0) {
			SYS_LOG_ERR("timeout");
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return -ETIME;
		}
		data_buffer += write_bytes;
		data_len -= write_bytes;
	}

	if (need_zero) {
		ret = usb_write(CONFIG_STUB_IN_EP_ADDR, &zero, 0, NULL);
		if (ret < 0) {
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return ret;
		}

		if (k_sem_take(&stub_write_sem, timeout) != 0) {
			SYS_LOG_ERR("zero timeout");
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return -ETIME;
		}
	}

	return 0;
}

static int usb_stub_fix_dev_sn(u8_t serial_num)
{
	int ret, sn_len;

	/* support up to 10 devices */
	if (serial_num > 9)
		return -EINVAL;

	sn_len = strlen(usb_stub_device_sn);
	usb_stub_device_sn[sn_len - 1] = (serial_num % 10) + '0';

	SYS_LOG_INF("device_sn: %s\n", usb_stub_device_sn);
	
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, usb_stub_device_sn, strlen(usb_stub_device_sn));
	if (ret) {
		return ret;
	}

	return 0;
}

/*
 * API: initialize USB STUB
 */
int usb_stub_init(struct device *dev, void *param)
{
	int ret;

	k_sem_init(&stub_write_sem, 0, 1);
	k_sem_init(&stub_read_sem, 0, 1);

	/* Register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_STUB_DEVICE_MANUFACTURER, strlen(CONFIG_USB_STUB_DEVICE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_STUB_DEVICE_PRODUCT, strlen(CONFIG_USB_STUB_DEVICE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_stub_fix_dev_sn(*(u8_t *)param);
	if (ret) {
		return ret;
	}

	/* Register device descriptor */
	usb_device_register_descriptors(usb_stub_fs_descriptor, usb_stub_hs_descriptor);

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&stub_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&stub_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}

	usb_stub_state = USB_STUB_ENABLED;

	return 0;
}

/*
 * API: deinitialize USB STUB
 */
int usb_stub_exit(void)
{
	int ret;

	usb_stub_state = 0;

	ret = usb_disable();
	if (ret) {
		SYS_LOG_ERR("Failed to disable USB: %d", ret);
		return ret;
	}
	usb_deconfig();
	return 0;
}
