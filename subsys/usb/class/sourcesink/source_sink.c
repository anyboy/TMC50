/*
 * USB Source/Sink configuration driver
 *
 * USB Source/Sink is a common driver that only handle data transfer,
 * the upper layer(s) should take care of the exact data and handle the
 * self-defined protocol. It support one interface with two endpoints
 * which the address, type, maxPacketSize and interval are predefined.
 * (support Bulk, Intr and Isoc for endpoint type!)
 *
 * Copyright (C) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARNING
#define SYS_LOG_DOMAIN "usb/ss"
#include <logging/sys_log.h>

#include <misc/byteorder.h>
#include <usb_common.h>
#include <usb_descriptor.h>

#include <class/usb_ss.h>

static usb_dc_ep_callback ss_in_ready;
static usb_dc_ep_callback ss_out_ready;

struct usb_ss_config {
	struct usb_if_descriptor intf;
	struct usb_ep_descriptor in_ep;
	struct usb_ep_descriptor out_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary) struct usb_ss_config ss_cfg = {
	.intf = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = CUSTOM_CLASS,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	.in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CONFIG_SS_IN_EP_ADDR,
		.bmAttributes = CONFIG_SS_IN_EP_TYPE,
		.wMaxPacketSize = sys_cpu_to_le16(CONFIG_SS_IN_EP_MPS),
		.bInterval = CONFIG_SS_IN_EP_INTERVAL,
	},
	.out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CONFIG_SS_OUT_EP_ADDR,
		.bmAttributes = CONFIG_SS_OUT_EP_TYPE,
		.wMaxPacketSize = sys_cpu_to_le16(CONFIG_SS_OUT_EP_MPS),
		.bInterval = CONFIG_SS_OUT_EP_INTERVAL,
	}
};

static void ss_interface_config(u8_t bInterfaceNumber)
{
	ss_cfg.intf.bInterfaceNumber = bInterfaceNumber;
}

static void ss_in_ep_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_IN || ss_in_ready == NULL)
		return;

	ss_in_ready(ep, ep_status);
}

static void ss_out_ep_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_OUT || ss_out_ready == NULL)
		return;

	ss_out_ready(ep, ep_status);
}

/* Describe Endpoints configuration */
static struct usb_ep_cfg_data ss_ep_data[] = {
	{
		.ep_cb = ss_in_ep_cb,
		.ep_addr = CONFIG_SS_IN_EP_ADDR
	},
	{
		.ep_cb = ss_out_ep_cb,
		.ep_addr = CONFIG_SS_OUT_EP_ADDR
	}
};

USBD_CFG_DATA_DEFINE(primary) struct usb_cfg_data ss_config = {
	.usb_device_description = NULL,
	.interface_config = ss_interface_config,
	.interface_descriptor = &ss_cfg.intf,
	.num_endpoints = ARRAY_SIZE(ss_ep_data),
	.endpoint = ss_ep_data,
};

void ss_register_status_cb(usb_status_callback cb_usb_status)
{
	ss_config.cb_usb_status = cb_usb_status;
}

void ss_register_class_handler(usb_request_handler class_handler,
				u8_t *payload_data)
{
	ss_config.interface.class_handler = class_handler;
	ss_config.interface.payload_data = payload_data;
}

void ss_register_vendor_handler(usb_request_handler vendor_handler,
				u8_t *vendor_data)
{
	ss_config.interface.vendor_handler = vendor_handler;
	ss_config.interface.vendor_data = vendor_data;
}

void ss_register_custom_handler(usb_request_handler custom_handler)
{
	ss_config.interface.custom_handler = custom_handler;
}

void ss_register_ops(usb_dc_ep_callback in_ready,
				usb_dc_ep_callback out_ready)
{
	ss_in_ready = in_ready;
	ss_out_ready = out_ready;
}

int usb_ss_init(void)
{
#ifndef CONFIG_USB_COMPOSITE_DEVICE
	int ret;

	SYS_LOG_DBG("Initializing");

	ss_config.usb_device_description = usb_get_device_descriptor();

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&ss_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&ss_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}
#endif

	return 0;
}

int ss_ep_write(const u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_write(CONFIG_SS_IN_EP_ADDR, data, data_len, bytes_ret);
}

int ss_ep_read(u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_read(CONFIG_SS_OUT_EP_ADDR, data, data_len, bytes_ret);
}

int ss_in_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_SS_IN_EP_ADDR);
}

int ss_out_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_SS_OUT_EP_ADDR);
}
