/* usb_descriptor.c - USB common device descriptor definition */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 * Copyright (c) 2017, 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <usb/usbstruct.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include "usb_descriptor.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_LEVEL
#include <logging/sys_log.h>

/*
 * The last index of the initializer_string without null character is:
 *   ascii_idx_max = bLength / 2 - 2
 * Use this macro to determine the last index of ASCII7 string.
 */
#define USB_BSTRING_ASCII_IDX_MAX(n)	(n / 2 - 2)

/*
 * The last index of the bString is:
 *   utf16le_idx_max = sizeof(initializer_string) * 2 - 2 - 1
 *   utf16le_idx_max = bLength - 2 - 1
 * Use this macro to determine the last index of UTF16LE string.
 */
#define USB_BSTRING_UTF16LE_IDX_MAX(n)	(n - 3)

/* Linker-defined symbols bound the USB descriptor structs */
extern struct usb_desc_header __usb_descriptor_start[];
extern struct usb_desc_header __usb_descriptor_end[];
extern struct usb_cfg_data __usb_data_start[];
extern struct usb_cfg_data __usb_data_end[];

/* Linker-defined symbols for re-initialization */
extern long __usb_descriptor_rom_start[];
extern long __usb_data_rom_start[];

/* Structure representing the global USB description */
struct common_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor cfg_descr;
} __packed;

#ifndef CONFIG_DEFINE_USB_DEVICE_DESC_STATIC
/*
 * Device and configuration descriptor placed in the device section,
 * no additional descriptor may be placed there.
 */
USBD_DEVICE_DESCR_DEFINE(primary) struct common_descriptor common_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_2_0),
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.bDeviceClass = MISC_CLASS,
		.bDeviceSubClass = 0x02,
		.bDeviceProtocol = 0x01,
#else
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
#endif
		.bMaxPacketSize0 = MAX_PACKET_SIZE0,
		.idVendor = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(BCDDEVICE_RELNUM),
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
	},
	/* Configuration descriptor */
	.cfg_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_CONFIGURATION_DESC,
		/*wTotalLength will be fixed in usb_fix_descriptor() */
		.wTotalLength = 0,
		.bNumInterfaces = 0,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = MAX_LOW_POWER,
	},
};

struct usb_string_desription {
	struct usb_string_descriptor lang_descr;
	struct usb_mfr_descriptor {
		u8_t bLength;
		u8_t bDescriptorType;
		u8_t bString[USB_BSTRING_LENGTH(
				CONFIG_USB_DEVICE_MANUFACTURER)];
	} __packed utf16le_mfr;

	struct usb_product_descriptor {
		u8_t bLength;
		u8_t bDescriptorType;
		u8_t bString[USB_BSTRING_LENGTH(CONFIG_USB_DEVICE_PRODUCT)];
	} __packed utf16le_product;

	struct usb_sn_descriptor {
		u8_t bLength;
		u8_t bDescriptorType;
		u8_t bString[USB_BSTRING_LENGTH(CONFIG_USB_DEVICE_SN)];
	} __packed utf16le_sn;
} __packed;

/*
 * Language, Manufacturer, Product and Serial string descriptors,
 * placed in the string section.
 * FIXME: These should be sorted additionally.
 */
USBD_STRING_DESCR_DEFINE(primary) struct usb_string_desription string_descr = {
	.lang_descr = {
		.bLength = sizeof(struct usb_string_descriptor),
		.bDescriptorType = USB_STRING_DESC,
		.bString = sys_cpu_to_le16(0x0409),
	},
	/* Manufacturer String Descriptor */
	.utf16le_mfr = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_MANUFACTURER),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_MANUFACTURER,
	},
	/* Product String Descriptor */
	.utf16le_product = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_PRODUCT),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_PRODUCT,
	},
	/* Serial Number String Descriptor */
	.utf16le_sn = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(CONFIG_USB_DEVICE_SN),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_SN,
	},
};
#endif

/* This element marks the end of the entire descriptor. */
USBD_TERM_DESCR_DEFINE(primary) struct usb_desc_header term_descr = {
	.bLength = 0,
	.bDescriptorType = 0,
};

/*
 * This function fixes bString by transforming the ASCII-7 string
 * into a UTF16-LE during runtime.
 */
static void ascii7_to_utf16le(void *descriptor)
{
	struct usb_string_descriptor *str_descr = descriptor;
	int idx_max = USB_BSTRING_UTF16LE_IDX_MAX(str_descr->bLength);
	int ascii_idx_max = USB_BSTRING_ASCII_IDX_MAX(str_descr->bLength);
	u8_t *buf = (u8_t *)&str_descr->bString;

	SYS_LOG_DBG("idx_max %d, ascii_idx_max %d, buf %x",
		    idx_max, ascii_idx_max, (u32_t)buf);

	for (int i = idx_max; i >= 0; i -= 2) {
		SYS_LOG_DBG("char %c : %x, idx %d -> %d",
			    buf[ascii_idx_max],
			    buf[ascii_idx_max],
			    ascii_idx_max, i);
		__ASSERT(buf[ascii_idx_max] > 0x1F && buf[ascii_idx_max] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB "
			 "string descriptors");
		buf[i] = 0;
		buf[i - 1] = buf[ascii_idx_max--];
	}
}

/*
 * Look for the bString that has the address equal to the ptr and
 * return its index. Use it to determine the index of the bString and
 * assign it to the interfaces iInterface variable.
 */
int usb_get_str_descriptor_idx(void *ptr)
{
	struct usb_desc_header *head = __usb_descriptor_start;
	struct usb_string_descriptor *str = ptr;
	int str_descr_idx = 0;

	while (head->bLength != 0) {
		switch (head->bDescriptorType) {
		case USB_STRING_DESC:
			if (head == (struct usb_desc_header *)str) {
				return str_descr_idx;
			}

			str_descr_idx += 1;
			break;
		default:
			break;
		}

		/* move to next descriptor */
		head = (struct usb_desc_header *)((u8_t *)head + head->bLength);
	}

	return 0;
}

#ifndef CONFIG_USB_DEVICE_FIX_ENDP_DESC_DISABLED
/*
 * Validate endpoint address and Update the endpoint descriptors at runtime,
 * the result depends on the capabilities of the driver and the number and
 * type of endpoints.
 * The default endpoint address is stored in endpoint descriptor and
 * usb_ep_cfg_data, so both variables bEndpointAddress and ep_addr need
 * to be updated.
 */
static int usb_validate_ep_cfg_data(struct usb_ep_descriptor * const ep_descr,
				    struct usb_cfg_data * const cfg_data,
				    u32_t *requested_ep)
{
	for (int i = 0; i < cfg_data->num_endpoints; i++) {
		struct usb_ep_cfg_data *ep_data = cfg_data->endpoint;

		/*
		 * Trying to find the right entry in the usb_ep_cfg_data.
		 */
		if (ep_descr->bEndpointAddress != ep_data[i].ep_addr) {
			continue;
		}

		for (u8_t idx = 1; idx < 16; idx++) {
			struct usb_dc_ep_cfg_data ep_cfg;

			ep_cfg.ep_type = ep_descr->bmAttributes;
			ep_cfg.ep_mps = ep_descr->wMaxPacketSize;
			ep_cfg.ep_addr = ep_descr->bEndpointAddress;
			if (ep_cfg.ep_addr & USB_EP_DIR_IN) {
				if ((*requested_ep & (1 << (idx + 16)))) {
					continue;
				}

				ep_cfg.ep_addr = (USB_EP_DIR_IN | idx);
			} else {
				if ((*requested_ep & (1 << (idx)))) {
					continue;
				}

				ep_cfg.ep_addr = idx;
			}
			if (!usb_dc_ep_check_cap(&ep_cfg)) {
				ep_descr->bEndpointAddress = ep_cfg.ep_addr;
				ep_data[i].ep_addr = ep_cfg.ep_addr;
				if (ep_cfg.ep_addr & USB_EP_DIR_IN) {
					*requested_ep |= (1 << (idx + 16));
				} else {
					*requested_ep |= (1 << idx);
				}
				SYS_LOG_DBG("endpoint 0x%x",
					    ep_data[i].ep_addr);
				return 0;
			}
		}
	}
	return -1;
}
#endif

/*
 * The interface descriptor of a USB function must be assigned to the
 * usb_cfg_data so that usb_ep_cfg_data and matching endpoint descriptor
 * can be found.
 */
static struct usb_cfg_data *usb_get_cfg_data(struct usb_if_descriptor *iface)
{
	size_t length;
	long *start_addr;

	/* re-initialize usb data */
	length = (u8_t *)__usb_data_end - (u8_t *)__usb_data_start;
	start_addr = (long *)(*__usb_data_rom_start);
	start_addr++;	/* skip itself */
	memcpy(__usb_data_start, start_addr, length);

	for (size_t i = 0; i < length; i++) {
		if (__usb_data_start[i].interface_descriptor == iface) {
			return &__usb_data_start[i];
		}
	}

	return NULL;
}

/*
 * The entire descriptor, placed in the .usb.descriptor section,
 * needs to be fixed before use. Currently, only the length of the
 * entire device configuration (with all interfaces and endpoints)
 * and the string descriptors will be corrected.
 *
 * Restrictions:
 * - just one device configuration (there is only one)
 * - string descriptor must be present
 */
static int usb_fix_descriptor(struct usb_desc_header *head)
{
	struct usb_cfg_descriptor *cfg_descr = NULL;
	struct usb_if_descriptor *if_descr = NULL;
	struct usb_cfg_data *cfg_data = NULL;
	u8_t numof_ifaces = 0;
	u8_t str_descr_idx = 0;
#ifndef CONFIG_USB_DEVICE_FIX_ENDP_DESC_DISABLED
	struct usb_ep_descriptor *ep_descr = NULL;
	u32_t requested_ep = BIT(16) | BIT(0);
#endif

	while (head->bLength != 0) {
		switch (head->bDescriptorType) {
		case USB_CONFIGURATION_DESC:
			cfg_descr = (struct usb_cfg_descriptor *)head;
			SYS_LOG_DBG("Configuration descriptor %p", head);
			break;
		case USB_ASSOCIATION_DESC:
			SYS_LOG_DBG("Association descriptor %p", head);
			break;
		case USB_INTERFACE_DESC:
			if_descr = (struct usb_if_descriptor *)head;
			SYS_LOG_DBG("Interface descriptor %p", head);
			if (if_descr->bAlternateSetting) {
				SYS_LOG_DBG("Skip alternate interface");
				break;
			}

			if (if_descr->bInterfaceNumber == 0) {
				cfg_data = usb_get_cfg_data(if_descr);
				if (!cfg_data) {
					SYS_LOG_ERR("There is no usb_cfg_data "
						    "for %p", head);
					return -1;
				}

				if (cfg_data->interface_config) {
					cfg_data->interface_config(
							numof_ifaces);
				}
			}

			numof_ifaces++;
			break;
		case USB_ENDPOINT_DESC:
			SYS_LOG_DBG("Endpoint descriptor %p", head);
#ifndef CONFIG_USB_DEVICE_FIX_ENDP_DESC_DISABLED
			ep_descr = (struct usb_ep_descriptor *)head;
			if (usb_validate_ep_cfg_data(ep_descr,
						     cfg_data,
						     &requested_ep)) {
				SYS_LOG_ERR("Failed to validate endpoints");
				return -1;
			}
#endif
			break;
		case 0:
		case USB_STRING_DESC:
			/*
			 * Skip language descriptor but correct
			 * wTotalLength and bNumInterfaces once.
			 */
			if (str_descr_idx) {
				ascii7_to_utf16le(head);
			} else {
				cfg_descr->wTotalLength =
					sys_cpu_to_le16((u8_t *)head -
							(u8_t *)cfg_descr);
				cfg_descr->bNumInterfaces = numof_ifaces;
			}

			str_descr_idx += 1;

			break;
		default:
			break;
		}

		/* Move to next descriptor */
		head = (struct usb_desc_header *)((u8_t *)head + head->bLength);
	}

	if ((head + 1) != __usb_descriptor_end) {
		SYS_LOG_DBG("try to fix next descriptor at %p", head + 1);
		return usb_fix_descriptor(head + 1);
	}

	return 0;
}


u8_t *usb_get_device_descriptor(void)
{
	long *start_addr;
	size_t length;

	SYS_LOG_DBG("__usb_descriptor_start %p", __usb_descriptor_start);
	SYS_LOG_DBG("__usb_descriptor_end %p", __usb_descriptor_end);

	/* re-initialize usb descriptor */
	length = (u8_t *)__usb_descriptor_end - (u8_t *)__usb_descriptor_start;
	start_addr = (long *)(*__usb_descriptor_rom_start);
	start_addr++;	/* skip itself */
	memcpy(__usb_descriptor_start, start_addr, length);

	if (usb_fix_descriptor(__usb_descriptor_start)) {
		SYS_LOG_ERR("Failed to fixup USB descriptor");
		return NULL;
	}

	return (u8_t *) __usb_descriptor_start;
}
