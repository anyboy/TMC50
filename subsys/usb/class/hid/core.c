/*
 * Human Interface Device (HID) USB class core
 *
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_LEVEL
#define SYS_LOG_DOMAIN "usb/hid"
#include <logging/sys_log.h>

#include <misc/byteorder.h>
#include <usb_device.h>
#include <usb_common.h>

#include <class/usb_hid.h>

#ifdef CONFIG_NVRAM_CONFIG
#include <string.h>
#include <nvram_config.h>
#include <property_manager.h>
#endif

#include "usb_hid_desc.h"

#define HID_INT_IN_EP_IDX			0
#define HID_INT_OUT_EP_IDX			1

static struct hid_device_info {
	const u8_t *report_desc;
	size_t report_size;
	const struct hid_ops *ops;
} hid_device;

static const struct usb_hid_descriptor hid_descriptor = {
	.bLength = sizeof(struct usb_hid_descriptor),
	.bDescriptorType = USB_HID_DESC,
	.bcdHID = sys_cpu_to_le16(USB_1_1),
	.bCountryCode = 0,
	.bNumDescriptors = 1,
	.subdesc[0] = {
		.bDescriptorType = USB_HID_REPORT_DESC,
		.wDescriptorLength = sys_cpu_to_le16(CONFIG_HID_REPORT_DESC_SIZE),
	},
};

static void hid_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		SYS_LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		SYS_LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		SYS_LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		SYS_LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		SYS_LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		SYS_LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		SYS_LOG_DBG("USB device resumed");
		break;
	case USB_DC_HIGHSPEED:
		SYS_LOG_DBG("USB device stack work in high-speed mode");
		break;
	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_DBG("USB unknown state");
		break;
	}
}

static int hid_class_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("Class request: bRequest 0x%x bmRequestType 0x%x len %d",
		    setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) {
		switch (setup->bRequest) {
		case HID_GET_REPORT:
			SYS_LOG_DBG("Get Report");
			if (hid_device.ops->get_report) {
				return hid_device.ops->get_report(setup, len,
								  data);
			} else {
				SYS_LOG_ERR("Mandatory request not supported");
				return -EINVAL;
			}
			break;
		default:
			SYS_LOG_ERR("Unhandled request 0x%x", setup->bRequest);
			break;
		}
	} else {
		switch (setup->bRequest) {
		case HID_SET_IDLE:
			SYS_LOG_DBG("Set Idle");
			if (hid_device.ops->set_idle) {
				return hid_device.ops->set_idle(setup, len,
								data);
			}
			break;
		case HID_SET_REPORT:
			if (hid_device.ops->set_report == NULL) {
				SYS_LOG_ERR("set_report not implemented");
				return -EINVAL;
			}
			return hid_device.ops->set_report(setup, len, data);
		default:
			SYS_LOG_ERR("Unhandled request 0x%x", setup->bRequest);
			break;
		}
	}

	return -ENOTSUP;
}

static int hid_custom_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("Standard request: bRequest 0x%x bmRequestType 0x%x len %d",
		    setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST &&
	    REQTYPE_GET_RECIP(setup->bmRequestType) ==
					REQTYPE_RECIP_INTERFACE &&
					setup->bRequest == REQ_GET_DESCRIPTOR) {
		switch (sys_le16_to_cpu(setup->wValue)) {
		case 0x2200:
			SYS_LOG_DBG("Return Report Descriptor");

			/* Some buggy system may be pass a larger wLength when
			 * it try read HID report descriptor, although we had
			 * already tell it the right descriptor size.
			 * So truncated wLength if it doesn't match.
			 */
			if (*len != hid_device.report_size) {
				SYS_LOG_DBG("len %d doesn't match"
					    "Report Descriptor size", *len);
				*len = min(*len, hid_device.report_size);
			}
			*data = (u8_t *)hid_device.report_desc;
			break;
		case 0x2100:
			SYS_LOG_DBG("Return HID Descriptor");
			*len = min(*len, hid_descriptor.bLength);
			*data = (u8_t *)&hid_descriptor;
			break;
		default:
			return -ENOTSUP;
		}

		return 0;
	}

	return -ENOTSUP;
}

static void hid_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_IN ||
	    hid_device.ops->int_in_ready == NULL) {
		return;
	}
	hid_device.ops->int_in_ready();
}

#ifdef CONFIG_HID_INTERRUPT_OUT
static void hid_int_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_OUT ||
	    hid_device.ops->int_out_ready == NULL) {
		return;
	}
	hid_device.ops->int_out_ready();
}
#endif

/* Describe Endpoints configuration */
static const struct usb_ep_cfg_data hid_ep_data[] = {
	{
		.ep_cb = hid_int_in,
		.ep_addr = CONFIG_HID_INTERRUPT_IN_EP_ADDR
	},
#ifdef CONFIG_HID_INTERRUPT_OUT
	{
		.ep_cb = hid_int_out,
		.ep_addr = CONFIG_HID_INTERRUPT_OUT_EP_ADDR
	}
#endif
};

static const struct usb_if_descriptor usb_hid_if = {
	.bLength = sizeof(struct usb_if_descriptor),
	.bDescriptorType = USB_INTERFACE_DESC,
	.bInterfaceNumber = CONFIG_USB_HID_DEVICE_IF_NUM,
	.bAlternateSetting = 0,
#ifdef CONFIG_HID_INTERRUPT_OUT
	.bNumEndpoints = 2,
#else
	.bNumEndpoints = 1,
#endif
	.bInterfaceClass = HID_CLASS,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
};

static const struct usb_cfg_data hid_config = {
	.usb_device_description = NULL,
	.interface_descriptor = &usb_hid_if,
	.cb_usb_status = hid_status_cb,
	.interface = {
		.class_handler = hid_class_handle_req,
		.custom_handler = hid_custom_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(hid_ep_data),
	.endpoint = hid_ep_data,
};


static int usb_hid_fix_dev_sn(void)
{
	static u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];
	int ret;
	int read_len;
#ifdef CONFIG_NVRAM_CONFIG
	read_len = nvram_config_get(CFG_BT_MAC, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		SYS_LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_HID_DEVICE_SN, strlen(CONFIG_USB_HID_DEVICE_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_HID_DEVICE_SN, strlen(CONFIG_USB_HID_DEVICE_SN));
		if (ret)
			return ret;
#endif
	return 0;
}


/*
 * API: initialize USB HID device
 */
int usb_hid_init(void)
{

	SYS_LOG_DBG("Init HID Device");

	int ret;

	/* Register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_HID_DEVICE_MANUFACTURER, strlen(CONFIG_USB_HID_DEVICE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_HID_DEVICE_PRODUCT, strlen(CONFIG_USB_HID_DEVICE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_hid_fix_dev_sn();
	if (ret) {
		return ret;
	}

	/* Register device descriptors */
	usb_device_register_descriptors(usb_hid_fs_desc, usb_hid_hs_desc);

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&hid_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&hid_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}
	return 0;
}

/*
 * API: deinitialize USB HID device
 */
int usb_hid_exit(void)
{
	int ret;

	ret = usb_disable();
	if (ret) {
		SYS_LOG_ERR("Failed to disable USB: %d", ret);
		return ret;
	}
	usb_deconfig();
	return 0;
}

/*
 * API: Initialize USB HID composite devie
 */
int usb_hid_composite_init(void)
{
	return usb_decice_composite_set_config(&hid_config);
}

void usb_hid_register_device(const u8_t *desc, size_t size,
			     const struct hid_ops *ops)
{
	hid_device.report_desc = desc;
	hid_device.report_size = size;
	hid_device.ops = ops;
}

int hid_in_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_HID_INTERRUPT_IN_EP_ADDR);
}

#ifdef CONFIG_HID_INTERRUPT_OUT
int hid_out_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_HID_INTERRUPT_OUT_EP_ADDR);
}
#endif

int hid_int_ep_write(const u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_write(hid_ep_data[HID_INT_IN_EP_IDX].ep_addr, data,
			 data_len, bytes_ret);
}

int hid_int_ep_read(u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
#ifdef CONFIG_HID_INTERRUPT_OUT
	return usb_read(hid_ep_data[HID_INT_OUT_EP_IDX].ep_addr, data,
			 data_len, bytes_ret);
#else
	return -EINVAL;
#endif
}
