/*
 * Copyright (c) 2017 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions OTG USB device controller driver
 *
 * Actions OTG USB device controller driver. The driver implements
 * the low level control routines to deal directly with the hardware.
 */

/*
 * NOTICE: for now, usb_device does not support zero-length transfer yet.
 * We have no ways to support zero-length by ourselves, because the core
 * only tell us how many bytes to transfer without the total length, so the
 * upper layer should take care by itself!
 *
 * The right solution is that the usb core should take care of zero-length
 * request instead of the gadget controller driver layer.
 */

#include <kernel.h>
#include <sys_io.h>
#include <board.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <misc/byteorder.h>
#include <usb/usb_dc.h>

#include "usb_aotg_registers.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DRIVER_LEVEL

#include <logging/sys_log.h>

/* Endpoint type: bit[0:1] */
#define USB_DC_EP_TYPE_MASK	0x3

#define REQTYPE_DIR_IS_IN(bRequestType)	(bRequestType & 0x80)

/* NOTE: keep pace with usb_device.h */
#define MAX_PACKET_SIZE0	64

/* setup packet */
struct usb_ctrl_setup_packet {
	u8_t bRequestType;
	u8_t bRequest;
	u16_t wValue;
	u16_t wIndex;
	u16_t wLength;
};

/*
 * USB endpoint private structure.
 */
struct usb_ep_ctrl_prv {
	u8_t ep_ena;	/* Endpoint Enabled */
	u16_t mps;	/* Endpoint Max packet size */
	usb_dc_ep_callback cb;	/* Endpoint callback function */
	u32_t data_len;
	enum usb_dc_ep_type ep_type;
};

/*
 * USB controller private structure.
 */
struct usb_aotg_ctrl_prv {
	usb_dc_status_callback status_cb;
	struct usb_ep_ctrl_prv in_ep_ctrl[USB_AOTG_IN_EP_NUM];
	struct usb_ep_ctrl_prv out_ep_ctrl[USB_AOTG_OUT_EP_NUM];
	u8_t attached;
	enum usb_aotg_ep0_phase phase;	/* Control transfer stage */
	enum usb_device_speed speed;
	union {
		u8_t raw_setup[sizeof(struct usb_ctrl_setup_packet)];
		struct usb_ctrl_setup_packet setup;
	};
};

static struct usb_aotg_ctrl_prv usb_aotg_ctrl;

static inline u8_t usb_read8(u32_t addr)
{
	return *(volatile u8_t *)addr;
}

static inline void usb_write8(u32_t addr, u8_t val)
{
	*(volatile u8_t *)addr = val;
}

static inline u16_t usb_read16(u32_t addr)
{
	return *(volatile u16_t *)addr;
}

static inline void usb_write16(u32_t addr, u16_t val)
{
	*(volatile u16_t *)addr = val;
}

static inline u32_t usb_read32(u32_t addr)
{
	return sys_read32(addr);
}

static inline void usb_write32(u32_t addr, u32_t val)
{
	sys_write32(val, addr);
}

static inline void usb_set_bit8(u32_t addr, u8_t val)
{
	u8_t tmp = *(volatile u8_t *)addr;

	*(volatile u8_t *)addr = tmp | (1 << val);
}

static inline void usb_clear_bit8(u32_t addr, u8_t val)
{
	u8_t tmp = *(volatile u8_t *)addr;

	*(volatile u8_t *)addr = tmp & ~(1 << val);
}

static inline void usb_set_bit32(u32_t addr, u32_t val)
{
	sys_set_bit(addr, val);
}

static inline void usb_clear_bit32(u32_t addr, u32_t val)
{
	sys_clear_bit(addr, val);
}

static void usb_aotg_reg_dump(void)
{
	SYS_LOG_DBG("USB AOTG registers:");

	SYS_LOG_DBG("IDVBUSCTRL: 0x%x", usb_read8(IDVBUSCTRL));
	SYS_LOG_DBG("DPDMCTRL: 0x%x", usb_read8(DPDMCTRL));
	SYS_LOG_DBG("LINESTATUS: 0x%x", usb_read8(LINESTATUS));
	SYS_LOG_DBG("USBIEN: 0x%x", usb_read8(USBIEN));
	SYS_LOG_DBG("OTGIEN: 0x%x", usb_read8(OTGIEN));
	SYS_LOG_DBG("USBEIEN: 0x%x", usb_read8(USBEIEN));
	SYS_LOG_DBG("USBIRQ: 0x%x", usb_read8(USBIRQ));
	SYS_LOG_DBG("OTGIRQ: 0x%x", usb_read8(OTGIRQ));
	SYS_LOG_DBG("USBEIRQ: 0x%x", usb_read8(USBEIRQ));
	SYS_LOG_DBG("OTGSTATE: 0x%x", usb_read8(OTGSTATE));
}

static void usb_aotg_ep_reg_dump(u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	ARG_UNUSED(ep_idx);

	SYS_LOG_DBG("USB AOTG EP 0x%x registers:", ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		SYS_LOG_DBG("STADDR: 0x%x", usb_read32(EPxOUT_STADDR(ep_idx)));
		SYS_LOG_DBG("OUT_CTRL: 0x%x", usb_read8(OUTxCTRL(ep_idx)));
		SYS_LOG_DBG("OUT_MAXPKT: 0x%x", usb_read16(OUTxMAXPKT(ep_idx)));
		SYS_LOG_DBG("OUT_IEN: 0x%x", usb_read8(OUTIEN));
		SYS_LOG_DBG("OUT_CS: 0x%x", usb_read8(OUTxCS(ep_idx)));
		SYS_LOG_DBG("OUT_SHTPKT: 0x%x", usb_read8(OUT_SHTPKT));
		SYS_LOG_DBG("OUT_BC: 0x%x", usb_read16(OUTxBC(ep_idx)));
		SYS_LOG_DBG("OUT_IRQ: 0x%x", usb_read16(OUTIRQ));
	} else {
		SYS_LOG_DBG("STADDR: 0x%x", usb_read32(EPxIN_STADDR(ep_idx)));
		SYS_LOG_DBG("IN_CTRL: 0x%x", usb_read8(INxCTRL(ep_idx)));
		SYS_LOG_DBG("IN_MAXPKT: 0x%x", usb_read16(INxMAXPKT(ep_idx)));
		SYS_LOG_DBG("IN_IEN: 0x%x", usb_read8(INIEN));
		SYS_LOG_DBG("IN_CS: 0x%x", usb_read8(INxCS(ep_idx)));
		SYS_LOG_DBG("IN_IRQ: 0x%x", usb_read8(INIRQ));
	}
}

/*
 * Helpers of endpoint type
 */

static inline bool ep_type_ctrl(enum usb_dc_ep_type ep_type)
{
	return ((ep_type & USB_DC_EP_TYPE_MASK) == USB_DC_EP_CONTROL);
}

static inline bool ep_type_isoc(enum usb_dc_ep_type ep_type)
{
	return ((ep_type & USB_DC_EP_TYPE_MASK) == USB_DC_EP_ISOCHRONOUS);
}

static inline bool ep_type_bulk(enum usb_dc_ep_type ep_type)
{
	return ((ep_type & USB_DC_EP_TYPE_MASK) == USB_DC_EP_BULK);
}

static inline bool ep_type_intr(enum usb_dc_ep_type ep_type)
{
	return ((ep_type & USB_DC_EP_TYPE_MASK) == USB_DC_EP_INTERRUPT);
}

/*
 * Check if the MaxPacketSize of endpoint is valid
 */
static bool usb_aotg_ep_mps_valid(u16_t ep_mps, enum usb_dc_ep_type ep_type)
{
	if (ep_type_ctrl(ep_type)) {
		/* Arbitrary: speed may be unknown */
		return ep_mps <= USB_AOTG_CTRL_MAX_PKT;
	}

	/* for non-control endpoints, speed is known */
	if (usb_aotg_ctrl.speed == USB_SPEED_FULL) {
		if ((ep_type_isoc(ep_type) &&
				(ep_mps > USB_AOTG_FS_ISOC_MAX_PKT)) ||
			(ep_type_bulk(ep_type) &&
				(ep_mps > USB_AOTG_FS_BULK_MAX_PKT)) ||
			(ep_type_intr(ep_type) &&
				(ep_mps > USB_AOTG_FS_INTR_MAX_PKT))) {
			return false;
		}
	} else if (usb_aotg_ctrl.speed == USB_SPEED_HIGH) {
		if ((ep_type_isoc(ep_type) &&
				(ep_mps > USB_AOTG_HS_ISOC_MAX_PKT)) ||
			(ep_type_bulk(ep_type) &&
				(ep_mps > USB_AOTG_HS_BULK_MAX_PKT)) ||
			(ep_type_intr(ep_type) &&
				(ep_mps > USB_AOTG_HS_INTR_MAX_PKT))) {
			return false;
		}
	}

	return true;
}

/*
 * Check if the address of endpoint is valid
 *
 * NOTE: we don't check if the address of control endpoint is non-zero!
 */
static bool usb_aotg_ep_addr_valid(u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep) && (ep_idx < USB_AOTG_OUT_EP_NUM)) {
		return true;
	} else if (USB_EP_DIR_IS_IN(ep) && (ep_idx < USB_AOTG_IN_EP_NUM)) {
		return true;
	}

	return false;
}

/*
 * Check if the endpoint is enabled
 */
static bool usb_aotg_ep_is_enabled(u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep) && usb_aotg_ctrl.out_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}
	if (USB_EP_DIR_IS_IN(ep) && usb_aotg_ctrl.in_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}

	return false;
}

/*
 * Using single FIFO for every endpoint (except for ep0) by default
 */
static int usb_aotg_ep_alloc_fifo(u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!ep_idx) {
		return 0;
	}

	/*
	 * FIXME: It is kind of simple (stupid) which could not support
	 * complicated cases, need to optimize the allocation.
	 */
	if (USB_EP_DIR_IS_OUT(ep)) {
		switch (ep_idx) {
		case USB_AOTG_OUT_EP_1:
			usb_write32(EP1OUT_STADDR, EP1OUT_FIFO_START);
			break;

		case USB_AOTG_OUT_EP_2:
			usb_write32(EP2OUT_STADDR, URAM1_FIFO_START);
			break;

		case USB_AOTG_OUT_EP_3:
			usb_write32(EP3OUT_STADDR, EP3OUT_FIFO_START);
			break;

		default:
			return -EINVAL;
		}
	} else {
		switch (ep_idx) {
		case USB_AOTG_IN_EP_1:
			usb_write32(EP1IN_STADDR, URAM0_FIFO_START);
			break;

		case USB_AOTG_IN_EP_2:
			usb_write32(EP2IN_STADDR, EP2IN_FIFO_START);
			break;

		case USB_AOTG_IN_EP_3:
			usb_write32(EP3IN_STADDR, EP3IN_FIFO_START);
			break;

		default:
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * Max Packet Size Limit
 */
static int usb_aotg_set_max_packet(u8_t ep, u16_t ep_mps,
				enum usb_dc_ep_type ep_type)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	/*
	 * NOTE: If the MaxPacketSize is beyond the limit, we won't try to
	 * assign a valid value, because it will fail anyway when the upper
	 * layer try to check capacity of the endpoint!
	 */
	if (!usb_aotg_ep_mps_valid(ep_mps, ep_type)) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_aotg_ctrl.out_ep_ctrl[ep_idx].mps = ep_mps;
	} else {
		usb_aotg_ctrl.in_ep_ctrl[ep_idx].mps = ep_mps;
	}

	return 0;
}

/*
 * Endpoint type
 */
static inline void usb_aotg_set_ep_type(u8_t ep, enum usb_dc_ep_type ep_type)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_aotg_ctrl.out_ep_ctrl[ep_idx].ep_type = ep_type;
	} else {
		usb_aotg_ctrl.in_ep_ctrl[ep_idx].ep_type = ep_type;
	}
}

/*
 * Initialize AOTG hardware endpoint.
 *
 * Set FIFO address, endpoint type, FIFO type, max packet size
 */
static int usb_aotg_ep_set(u8_t ep, u16_t ep_mps,
				enum usb_dc_ep_type ep_type)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	SYS_LOG_DBG("Ep setup 0x%x, mps %d, type %d", ep, ep_mps, ep_type);

	/* Set FIFO address */
	usb_aotg_ep_alloc_fifo(ep);

	/* Set Max Packet */
	usb_aotg_set_max_packet(ep, ep_mps, ep_type);

	/* Set EP type */
	usb_aotg_set_ep_type(ep, ep_type);

	/* No need to set for endpoint 0 */
	if (!ep_idx) {
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		/* Set endpoint type and FIFO type */
		usb_write8(OUTxCTRL(ep_idx),
			(ep_type << 2) | USB_AOTG_FIFO_SINGLE);
		/* Set max packet size for EP */
		usb_write16(OUTxMAXPKT(ep_idx),
			usb_aotg_ctrl.out_ep_ctrl[ep_idx].mps);
	} else {
		usb_write8(INxCTRL(ep_idx),
			(ep_type << 2) | USB_AOTG_FIFO_SINGLE);
		usb_write16(INxMAXPKT(ep_idx),
			usb_aotg_ctrl.in_ep_ctrl[ep_idx].mps);
	}

	return 0;
}

/*
 * Prepare for the next OUT transaction
 *
 * There are two ways: write OUTxBC or set busy for EPxCS.
 * I don't know which one is better really, just keep it both.
 */
static inline void usb_aotg_prep_rx(u8_t ep_idx)
{
	/* Set busy */
	if (!ep_idx) {
#if 0
		usb_set_bit8(EP0CS, EP0CS_OUTBUSY);
#endif
		usb_write8(OUT0BC, 0x0);
	} else {
#if 0
		usb_write8(OUTxCS(ep_idx), 0);
#endif
		usb_set_bit8(OUTxCS(ep_idx), EPCS_BUSY);
	}
}

/*
 * Setup stage: read data from Setup FIFO
 */
static inline int ep0_read_setup(u8_t *data, u32_t data_len)
{
	u8_t len = data_len;

	if (data_len > sizeof(struct usb_ctrl_setup_packet)) {
		len = sizeof(struct usb_ctrl_setup_packet);
	}

	memcpy(data, usb_aotg_ctrl.raw_setup, len);

	return len;
}

/*
 * Data stage: write data to EP0 FIFO
 *
 * @return  Actual length
 */
static int ep0_write_fifo(const u8_t *const data, u32_t data_len)
{
	u32_t i;
	u8_t j;
	u32_t addr = EP0IN_FIFO;

	/* Check Busy */
	if (usb_read8(EP0CS) & BIT(EP0CS_INBUSY)) {
		SYS_LOG_DBG("EP0IN busy");
		return -EAGAIN;
	}

	/* 32-bit alignment */
	if (((long)data & 0x3) == 0) {
		for (i = 0; i < (data_len & ~0x3); i += 4) {
			usb_write32(addr + i, *(u32_t *)(data + i));
		}
		for (j = 0; j < (data_len & 0x3); j++) {
			usb_write8(addr + i + j, *(data + i + j));
		}
	/* 16-bit alignment */
	} else if (((long)data & 0x1) == 0) {
		for (i = 0; i < (data_len & ~0x1); i += 2) {
			usb_write16(addr + i, *(u16_t *)(data + i));
		}
		/* The last byte */
		if (data_len & 0x1) {
			usb_write8(addr + i, *(data + i));
		}
	} else {
		for (i = 0; i < data_len; i++) {
			usb_write8(addr + i, *(data + i));
		}
	}

	/* Set Byte Counter: set busy */
	usb_write8(IN0BC, data_len);

	return data_len;
}

/*
 * Data stage: read data from EP0 FIFO
 *
 * @return  Actual length
 */
static int ep0_read_fifo(u8_t *data, u32_t data_len)
{
	u32_t i;
	u8_t j;
	u8_t len;
	u32_t addr = EP0OUT_FIFO;

	/* Check Busy */
	if (usb_read8(EP0CS) & BIT(EP0CS_OUTBUSY)) {
		return -1;
	}

	/* Get Actual length */
	len = usb_read8(OUT0BC);

	/* 32-bit alignment */
	if (((long)data & 0x3) == 0) {
		for (i = 0; i < (data_len & ~0x3); i += 4) {
			*(u32_t *)(data + i) = usb_read32(addr + i);
		}
		for (j = 0; j < (data_len & 0x3); j++) {
			*(data + i + j) = usb_read8(addr + i + j);
		}
	/* 16-bit alignment */
	} else if (((long)data & 0x1) == 0) {
		for (i = 0; i < (data_len & ~0x1); i += 2) {
			*(u16_t *)(data + i) = usb_read16(addr + i);
		}
		/* The last byte */
		if (data_len & 0x1) {
			*(data + i) = usb_read8(addr + i);
		}
	} else {
		for (i = 0; i < data_len; i++) {
			*(data + i) = usb_read8(addr + i);
		}
	}

	return len;
}

/*
 * Status stage
 */
static inline void handle_status(void)
{
	usb_set_bit8(EP0CS, EP0CS_NAK);
}

/*
 * Write data to EPx FIFO
 *
 * @return  Actual length
 */
static int ep_write_fifo(u8_t ep, const u8_t *const data,
				u32_t data_len)
{
	u32_t i;
	u32_t len;
	u8_t left;
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);
	u32_t addr = FIFOxDAT(ep_idx);
	const u8_t *buf = (const u8_t *)data;

	/* Check Busy */
	if (usb_read8(INxCS(ep_idx)) & BIT(EPCS_BUSY)) {
		SYS_LOG_DBG("EP 0x%x still busy", ep);
		return -EAGAIN;
	}

	/* 32-bit alignment */
	if (((long)buf & 0x3) == 0) {
		len = data_len >> 2;
		left = data_len & 0x3;
		for (i = 0; i < len; i++, buf += 4) {
			usb_write32(addr, *(u32_t *)buf);
		}
	/* 16-bit alignment */
	} else if (((long)buf & 0x1) == 0) {
		len = data_len >> 1;
		left = data_len & 0x1;
		for (i = 0; i < len; i++, buf += 2) {
			usb_write16(addr, *(u16_t *)buf);
		}
	} else {
		len = data_len;
		left = 0;
		for (i = 0; i < len; i++, buf++) {
			usb_write8(addr, *buf);
		}
	}

	/* Handle left byte(s) */
	for (i = 0; i < left; i++, buf++) {
		usb_write8(addr, *buf);
	}

	/* Set Byte Counter */
	usb_write16(INxBC(ep_idx), data_len);

	/* Set busy */
	usb_set_bit8(INxCS(ep_idx), EPCS_BUSY);

	return data_len;
}

/*
 * Read data from EP FIFO
 *
 * @return  Actual length
 */
static int ep_read_fifo(u8_t ep, u8_t *data, u32_t data_len)
{
	u32_t i;
	u32_t len;
	u8_t left;
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);
	u32_t addr = FIFOxDAT(ep_idx);
	u8_t *buf = (u8_t *)data;

	/* Check Busy */
	if (usb_read8(OUTxCS(ep_idx)) & BIT(EPCS_BUSY)) {
		SYS_LOG_DBG("EP 0x%x still busy", ep);
		return -1;
	}

	/* 32-bit alignment */
	if (((long)buf & 0x3) == 0) {
		len = data_len >> 2;
		left = data_len & 0x3;
		for (i = 0; i < len; i++, buf += 4) {
			*(u32_t *)buf = usb_read32(addr);
		}
	/* 16-bit alignment */
	} else if (((long)buf & 0x1) == 0) {
		len = data_len >> 1;
		left = data_len & 0x1;
		for (i = 0; i < len; i++, buf += 2) {
			*(u16_t *)buf = usb_read16(addr);
		}
	} else {
		len = data_len;
		left = 0;
		for (i = 0; i < len; i++, buf++) {
			*buf = usb_read8(addr);
		}
	}

	/* Handle left byte(s) */
	for (i = 0; i < left; i++, buf++) {
		*buf = usb_read8(addr);
	}

	return data_len;
}

/*
 * Receive data from host: could be Setup, Data or Status
 *
 * @return  transfer length
 */
static int usb_aotg_rx(u8_t ep, u8_t *data, u32_t data_len)
{
	int act_len = 0;

	/* Endpoint 0 */
	if (!USB_EP_ADDR2IDX(ep)) {
		if (usb_aotg_ctrl.phase == USB_AOTG_SETUP) {
			act_len = ep0_read_setup(data, data_len);
		} else if (usb_aotg_ctrl.phase == USB_AOTG_OUT_DATA) {
			act_len = ep0_read_fifo(data, data_len);
#if 0
		} else if (usb_aotg_ctrl.phase == USB_AOTG_OUT_STATUS) {
			/*
			 * Case 1: Setup, In Data, Out Status
			 */
			handle_status();
#endif
		}
	} else {
		act_len = ep_read_fifo(ep, data, data_len);
	}

	return act_len;
}

/*
 * Send data to host
 *
 * @return  transfer length
 */
static int usb_aotg_tx(u8_t ep, const u8_t *const data,
				u32_t data_len)
{
	u32_t act_len = 0;

	/* Endpoint 0 */
	if (!USB_EP_ADDR2IDX(ep)) {
		if ((usb_aotg_ctrl.phase == USB_AOTG_IN_DATA) && !data_len) {
			/*
			 * Handling zero-length packet for In-data phase
			 */
			act_len = ep0_write_fifo(data, data_len);
		} else if (!data_len) {
			/*
			 * Handling in-status stage
			 * Case 2: Setup, In Status
			 * Case 3: Setup, Out Data, In Status
			 */
			if (usb_aotg_ctrl.phase != USB_AOTG_SET_ADDRESS) {
				handle_status();
			}
			usb_aotg_ctrl.phase = USB_AOTG_IN_STATUS;
		} else {
			act_len = ep0_write_fifo(data, data_len);
			usb_aotg_ctrl.phase = USB_AOTG_IN_DATA;
		}
	} else {
		act_len = ep_write_fifo(ep, data, data_len);
	}

	return act_len;
}

/*
 * Reset specific endpoint
 */
static int usb_aotg_ep_reset(u8_t ep, u8_t type)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (!ep_idx) {
		SYS_LOG_DBG("Ep reset all endpoints");
	}

	/* Select specific endpoint */
	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_write8(EPRST, ep_idx);
	} else {
		usb_write8(EPRST, ep_idx | EPRST_IO_IN);
	}

	if (type == USB_AOTG_EP_FIFO_RESET) {
		usb_set_bit8(EPRST, EPRST_FIFORST);
	} else if (type == USB_AOTG_EP_TOGGLE_RESET) {
		usb_set_bit8(EPRST, EPRST_TOGRST);
	} else if (type == USB_AOTG_EP_RESET) {
		usb_set_bit8(EPRST, EPRST_FIFORST);
		usb_set_bit8(EPRST, EPRST_TOGRST);
	} else {
		SYS_LOG_ERR("Ep reset type: %d", type);
		return -EINVAL;
	}

	return 0;
}

static void usb_aotg_handle_reset(void)
{
	/* Clear USB Reset IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_RESET));

	usb_aotg_ctrl.speed = USB_SPEED_FULL;

	SYS_LOG_DBG("USB RESET event");

	/* Inform upper layers */
	if (usb_aotg_ctrl.status_cb) {
		usb_aotg_ctrl.status_cb(USB_DC_RESET, NULL);
	}
}

static void usb_aotg_handle_hs(void)
{
	/* Clear USB high-speed IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_HIGH_SPEED));

	usb_aotg_ctrl.speed = USB_SPEED_HIGH;

	SYS_LOG_DBG("USB high-speed event");
	/* Inform upper layers */
	if (usb_aotg_ctrl.status_cb) {
		usb_aotg_ctrl.status_cb(USB_DC_HIGHSPEED, NULL);
	}
}

static void usb_aotg_handle_otg(void)
{
	u8_t tmp;
	u8_t state;

	/* Clear OTG IRQ(s) */
	tmp = usb_read8(OTGIEN) & usb_read8(OTGIRQ);
	usb_write8(OTGIRQ, tmp);

	state = usb_read8(OTGSTATE);

	/* If AOTG enters b_peripheral state, enable Core interrupts */
	if (state == OTG_B_PERIPHERAL) {
		usb_set_bit8(USBIEN, USBIEN_HIGH_SPEED);
		usb_set_bit8(USBIEN, USBIEN_RESET);
		usb_set_bit8(USBIEN, USBIEN_SUSPEND);
		usb_set_bit8(USBIEN, USBIEN_SETUP);
	}

	SYS_LOG_DBG("USB OTG State: 0x%x, irq: 0x%x", state, tmp);
}

/*
 * Handle SETUP packet
 */
static void usb_aotg_handle_setup(void)
{
	usb_dc_ep_callback ep_cb;
	u32_t addr = SETUP_FIFO;
	u8_t i;

	/* Clear SETUP IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_SETUP));

	/* Fetch setup packet */
	for (i = 0; i < sizeof(struct usb_ctrl_setup_packet); i++, addr++) {
		*(usb_aotg_ctrl.raw_setup + i) = usb_read8(addr);
	}

	/* Set setup data length */
	usb_aotg_ctrl.out_ep_ctrl[USB_AOTG_OUT_EP_0].data_len =
			sizeof(struct usb_ctrl_setup_packet);

	/* Set in data length */
	if (REQTYPE_DIR_IS_IN(usb_aotg_ctrl.setup.bRequestType)) {
		usb_aotg_ctrl.in_ep_ctrl[USB_AOTG_IN_EP_0].data_len =
				usb_aotg_ctrl.setup.wLength;
	}

	usb_aotg_ctrl.phase = USB_AOTG_SETUP;

	ep_cb = usb_aotg_ctrl.out_ep_ctrl[USB_AOTG_OUT_EP_0].cb;
	/* Call the registered callback if any */
	if (ep_cb) {
		ep_cb(USB_AOTG_OUT_EP_0, USB_DC_EP_SETUP);
	}
}

/*
 * Handle EPxIN data
 */
static void usb_aotg_handle_epin(u8_t ep_idx)
{
	usb_dc_ep_callback ep_cb;
	u8_t ep;

	/* Clear EPxIN IRQ */
	usb_write8(INIRQ, BIT(ep_idx));

	ep_cb = usb_aotg_ctrl.in_ep_ctrl[ep_idx].cb;
	ep = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_IN);
	/* Call the registered callback if any */
	if (ep_cb) {
		ep_cb(ep, USB_DC_EP_DATA_IN);
	}
}

/*
 * Handle EPxOUT data
 */
static void usb_aotg_handle_epout(u8_t ep_idx)
{
	usb_dc_ep_callback ep_cb;
	u32_t len;
	u8_t ep = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_OUT);

	/* Clear EPxOUT IRQ */
	usb_write8(OUTIRQ, BIT(ep_idx));

	/* Get Actual Length */
	if (!ep_idx) {
		/* Check phase before checking byte counter */
		if (usb_aotg_ctrl.phase == USB_AOTG_OUT_STATUS) {
			return;
		}
		/*
		 * EP0OUT data irq may come before EPxOUT token irq in
		 * out-status phase, filter it otherwise it will be disturbed.
		 * It may come even in in-data phase, can't explain!
		 *
		 * Keep it (only for record), we will check setup packet!
		 */
		if (usb_aotg_ctrl.phase == USB_AOTG_IN_DATA) {
			return;
		}
		/* Filter by direction and length of setup packet */
		if (REQTYPE_DIR_IS_IN(usb_aotg_ctrl.setup.bRequestType)) {
			return;
		} else if (usb_aotg_ctrl.setup.wLength == 0) {
			return;
		}

		/* NOTICE: OUT0BC will be set in setup phase and data phase */
		len = usb_read8(OUT0BC);
		if (!len) {
			return;
		}
		usb_aotg_ctrl.phase = USB_AOTG_OUT_DATA;
	} else {
		len = usb_read16(OUTxBC(ep_idx));
		if (len == 0) {
			SYS_LOG_DBG("ep 0x%x recv zero-length packet", ep);
		}
	}

	usb_aotg_ctrl.out_ep_ctrl[ep_idx].data_len = len;

	ep_cb = usb_aotg_ctrl.out_ep_ctrl[ep_idx].cb;
	/* Call the registered callback if any */
	if (ep_cb) {
		ep_cb(ep, USB_DC_EP_DATA_OUT);
	}
}

static inline void usb_aotg_handle_out_token_internal(u8_t ep_idx)
{
	if (ep_idx == USB_AOTG_OUT_EP_0) {
		/*
		 * We may get one or more OUT token(s) even though
		 * we have set ACK bit already, filter the case!
		 */
		if (usb_aotg_ctrl.phase == USB_AOTG_OUT_STATUS) {
			return;
		}

		/*
		 * Case 1: Setup, In Data, Out Status
		 *
		 * Status stage: no need to check busy
		 */
		if (usb_aotg_ctrl.phase == USB_AOTG_IN_DATA) {
			handle_status();
			usb_aotg_ctrl.phase = USB_AOTG_OUT_STATUS;
			return;
		}
	}

	/* Do nothing */
	return;

	/*
	 * NOTE: out token only take cares of out status phase, don't
	 * use it for out data phase, because ep0 may be busy all the
	 * time and can never complete!
	 */
#if 0
	usb_dc_ep_callback ep_cb;

	while (usb_read8(EP0CS) & BIT(EP0CS_OUTBUSY)) {
		;
	}

	usb_aotg_ctrl.out_ep_ctrl[ep_idx].data_len = usb_read8(OUT0BC);
	usb_aotg_ctrl.phase = USB_AOTG_OUT_DATA;

	ep_cb = usb_aotg_ctrl.out_ep_ctrl[ep_idx].cb;
	ep_idx = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_OUT);
	/* Call the registered callback if any */
	if (ep_cb) {
		ep_cb(ep_idx, USB_DC_EP_DATA_OUT);
	}
#endif
}

/*
 * Handle EPxOUT Token
 */
static void usb_aotg_handle_out_token(void)
{
	u8_t ep_msk, i;

	/* Get endpoint mask */
	ep_msk = (usb_read8(OUT_TOKIRQ) & usb_read8(OUT_TOKIEN));

	/* NOTE: There may be more than one out token irqs simutaneously */
	for (i = 0; i < EPOUT_TOKEN_NUM; i++) {
		if (ep_msk & BIT(i)) {
			/* Clear IRQ */
			usb_write8(OUT_TOKIRQ, BIT(i));

			usb_aotg_handle_out_token_internal(i);
		}
	}
}

/*
 * Handle EPxIN Token
 */
static void usb_aotg_handle_in_token(void)
{
	struct usb_ep_ctrl_prv *ep_ctrl;
	u8_t ep_msk, i;

	/* Get endpoint mask */
	ep_msk = usb_read8(IN_TOKIRQ) & usb_read8(IN_TOKIEN);

	/* NOTE: There may be more than one out token irqs simutaneously */
	for (i = 0; i < EPIN_TOKEN_NUM; i++) {
		if (ep_msk & BIT(i)) {
			/* Clear IRQ */
			usb_write8(IN_TOKIRQ, BIT(i));

			ep_ctrl = &usb_aotg_ctrl.in_ep_ctrl[i];
			/* Call the registered callback if any */
			if (ep_ctrl->cb) {
				ep_ctrl->cb(i, USB_DC_EP_DATA_IN);
			}
		}
	}
}

/*
 * Handle EPxOUT Short Packet
 */
static void usb_aotg_handle_short_packet(void)
{
	usb_write8(OUT_SHTPKT, usb_read8(OUT_SHTPKT));

	SYS_LOG_DBG("aotg short packet");
}

/*
 * USB ISR handler
 *
 * NOTICE: Should use usb_writeX() instead of usb_set_bitX()
 * to clear IRQs in most cases to avoid two or more IRQs may
 * be pending simultaneously.
 */
static void usb_aotg_isr_handler(void)
{
	/* IRQ Vector */
	u8_t vector = usb_read8(IVECT);

	SYS_LOG_DBG("vector: 0x%x", vector);

	/*
	 * Make sure external IRQ has been cleared right!
	 *
	 * TODO: If two or more IRQs are pending simultaneously,
	 * EIRQ will be pending immediately after cleared or not?
	 */
	while (usb_read8(USBEIRQ) & BIT(USBEIRQ_EXTERN)) {
		usb_set_bit8(USBEIRQ, USBEIRQ_EXTERN);
	}

	switch (vector) {
	/* OTG */
	case UIV_OTGIRQ:
		usb_aotg_handle_otg();
		break;

	/* USB Reset */
	case UIV_USBRST:
		usb_aotg_handle_reset();
		break;

	case UIV_HSPEED:
		usb_aotg_handle_hs();
		break;

	/* USB Suspend */
	case UIV_SUSPEND:
		usb_write8(USBIRQ, BIT(USBIRQ_SUSPEND));
		if (usb_aotg_ctrl.status_cb) {
			usb_aotg_ctrl.status_cb(USB_DC_SUSPEND, NULL);
		}
		break;

	/* USB Resume */
	case UIV_RESUME:
		usb_set_bit8(USBEIRQ, USBEIRQ_RESUME);
		if (usb_aotg_ctrl.status_cb) {
			usb_aotg_ctrl.status_cb(USB_DC_RESUME, NULL);
		}
		break;

	/* SETUP */
	case UIV_SUDAV:
		usb_aotg_handle_setup();
		break;

	/* EP0IN */
	case UIV_EP0IN:
		usb_aotg_handle_epin(USB_AOTG_IN_EP_0);
		break;

	/* EP0OUT */
	case UIV_EP0OUT:
		usb_aotg_handle_epout(USB_AOTG_OUT_EP_0);
		break;

	/* EP1IN */
	case UIV_EP1IN:
		usb_aotg_handle_epin(USB_AOTG_IN_EP_1);
		break;

	/* EP2IN */
	case UIV_EP2IN:
		usb_aotg_handle_epin(USB_AOTG_IN_EP_2);
		break;

	/* EP3IN */
	case UIV_EP3IN:
		usb_aotg_handle_epin(USB_AOTG_IN_EP_3);
		break;

	/* EP1OUT */
	case UIV_EP1OUT:
		usb_aotg_handle_epout(USB_AOTG_OUT_EP_1);
		break;

	/* EP2OUT */
	case UIV_EP2OUT:
		usb_aotg_handle_epout(USB_AOTG_OUT_EP_2);
		break;

	/* EP3OUT */
	case UIV_EP3OUT:
		usb_aotg_handle_epout(USB_AOTG_OUT_EP_3);
		break;

	/*
	 * Using OUT Token for EP0OUT data transfer for assistance.
	 *
	 * AOTG handles in/out status by software (handle_status()),
	 * which means it depends on USB core totally. But USB core
	 * thinks that controller should take care of out status by
	 * itself (excluding int status phase). That means EP0 Data
	 * IRQ will never be pending until handle_status() called!
	 */
	case UIV_OUTTOKEN:
		usb_aotg_handle_out_token();
		break;

	case UIV_INTOKEN:
		usb_aotg_handle_in_token();
		break;

	/* Short Packet */
	case UIV_EPOUTSHTPKT:
		usb_aotg_handle_short_packet();
		break;

	default:
		break;
	}
}

/*
 * Turn USB controller power on
 */
static inline void usb_aotg_power_on(void)
{
	u32_t val = usb_read32(USB_VDD);

	val &= ~USB_VDD_VOLTAGE_MASK;
	val |= (USB_VDD_VOLTAGE_DEFAULT | BIT(USB_VDD_EN));
	usb_write32(USB_VDD, val);
}

/*
 * Turn USB controller power off
 */
static inline void usb_aotg_power_off(void)
{
	usb_write32(USB_VDD, usb_read32(USB_VDD) & (~BIT(USB_VDD_EN)));
}

/*
 * Enable USB controller clock
 */
static void usb_aotg_clock_enable(void)
{
	acts_clock_peripheral_enable(CLOCK_ID_USB);
}

/*
 * Disable USB controller clock
 */
static void usb_aotg_clock_disable(void)
{
	acts_clock_peripheral_disable(CLOCK_ID_USB);
}

/*
 * Reset USB controller
 */
static int usb_aotg_reset(void)
{
	u8_t reg;

	acts_reset_peripheral(RESET_ID_USB2);
	acts_reset_peripheral(RESET_ID_USB);

	/* enable usb phy pll */
	usb_write8(USBPHYCTRL, (usb_read8(USBPHYCTRL) | 0xc0));
	k_busy_wait(1000);

	usb_write8(USBEFUSEREF, 0);
	reg = usb_read8(USBEFUSEREF);
	usb_write8(USBEFUSEREF, 0x80);
	usb_write8(USBEFUSEREF, (usb_read8(USBEFUSEREF) | reg));

	/* Wait for USB controller until reset done */
	while (usb_read32(LINESTATUS) & BIT(OTGRESET)) {
		;
	}

	return 0;
}

/*
 * USB PHY configuration
 */
static void usb_phy_setting(u8_t reg, u8_t value)
{
	u8_t low, high, tmp;

	low = reg & 0x0f;
	high = (reg >> 4) & 0x0f;

	tmp = usb_read8(VDCTRL) & 0x80;

	/* write vstatus */
	usb_write8(VDSTATE, value);

	/* write vcontrol */
	low |= 0x10;
	usb_write8(VDCTRL, (low | tmp));

	low &= 0x0f;
	usb_write8(VDCTRL, (low | tmp));

	low |= 0x10;
	usb_write8(VDCTRL, (low | tmp));

	high |= 0x10;
	usb_write8(VDCTRL, (high | tmp));

	high &= 0x0f;
	usb_write8(VDCTRL, (high | tmp));

	high |= 0x10;
	usb_write8(VDCTRL, (high | tmp));
}

/*
 * Initialize controller
 */
static void usb_aotg_init(void)
{
	usb_aotg_power_on();

	usb_aotg_clock_enable();

	usb_aotg_reset();

	/* Set soft Idpin */
	usb_set_bit8(IDVBUSCTRL, SOFT_IDPIN);
	usb_set_bit8(IDVBUSCTRL, SOFT_IDEN);

	usb_phy_setting(0x84, 0x1a);
	usb_phy_setting(0xe0, 0xa3);
	usb_phy_setting(0xe6, 0x8d);
	usb_phy_setting(0xe7, 0x2b);

#ifdef CONFIG_USB_AOTG_UDC_FS
	usb_set_bit8(BKDOOR, HS_DISABLE);
#endif
}


/* Disable soft disconnect */
static inline void usb_aotg_connect(void)
{
	usb_clear_bit8(USBCS, USBCS_DISCONN);
}

/* Enable soft disconnect */
static inline void usb_aotg_disconnect(void)
{
	usb_set_bit8(USBCS, USBCS_DISCONN);
}


/*
 * Switch FIFO clock to make sure it is available for AOTG
 */
static inline void usb_aotg_fifo_init(void)
{
	usb_write32(USB_MEM_CLK, usb_read32(USB_MEM_CLK) |
		BIT(USB_MEM_CLK_URAM2_OFFSET) |
		BIT(USB_MEM_CLK_URAM1_OFFSET) |
		BIT(USB_MEM_CLK_URAM0_OFFSET));
}

int usb_dc_attach(void)
{
	if (usb_aotg_ctrl.attached)
		return 0;

	usb_aotg_fifo_init();

	usb_aotg_init();

	/* Enable OTG(b_peripheral) interrupt */
	usb_set_bit8(OTGIEN, OTGIEN_PERIPHERAL);
	/* Enable external interrupt */
	usb_set_bit8(USBEIEN, USBEIEN_EXTERN);

	/* Connect and enable USB interrupt */
	IRQ_CONNECT(USB_AOTG_IRQ, CONFIG_IRQ_PRIO_NORMAL,
			usb_aotg_isr_handler, 0, 0);
	irq_enable(USB_AOTG_IRQ);

	usb_aotg_connect();

	usb_aotg_reg_dump();

	usb_aotg_ctrl.attached = 1;

	return 0;
}

int usb_dc_detach(void)
{
	if (!usb_aotg_ctrl.attached) {
		return 0;
	}

	irq_disable(USB_AOTG_IRQ);

	usb_aotg_disconnect();

	usb_aotg_reset();

	usb_phy_setting(0xe7, 0x0);

	/* disable PHY PLL */
	usb_write8(USBPHYCTRL, 0x0);

	usb_aotg_clock_disable();

	usb_aotg_power_off();

	usb_aotg_ctrl.attached = 0;
	usb_aotg_ctrl.speed = USB_SPEED_UNKNOWN;

	return 0;
}

int usb_dc_reset(void)
{
	int ret;

	ret = usb_aotg_reset();

	/* Clear private data */
	memset(&usb_aotg_ctrl, 0, sizeof(usb_aotg_ctrl));

	return ret;
}

int usb_dc_set_address(const u8_t addr)
{
	if (addr > USB_AOTG_MAX_ADDR) {
		return -EINVAL;
	}

	/* Respond "Set Address" automatically in device mode */
	SYS_LOG_DBG("Set Address: %d", usb_read8(FNADDR));

	/* Set Address phase */
	usb_aotg_ctrl.phase = USB_AOTG_SET_ADDRESS;

	return 0;
}

int usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	usb_aotg_ctrl.status_cb = cb;

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	SYS_LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
			cfg->ep_type);

	/*
	 * Check if the address of control endpoint is non-zero!
	 */
	if (ep_type_ctrl(cfg->ep_type) && USB_EP_ADDR2IDX(cfg->ep_addr)) {
		SYS_LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (!usb_aotg_ep_mps_valid(cfg->ep_mps, cfg->ep_type))  {
		SYS_LOG_WRN("unsupported packet size");
		return -1;
	}

	if (!usb_aotg_ep_addr_valid(cfg->ep_addr)) {
		SYS_LOG_WRN("endpoint 0x%x address out of range", cfg->ep_addr);
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep_cfg->ep_addr))
		return -EINVAL;

	return usb_aotg_ep_set(ep_cfg->ep_addr, ep_cfg->ep_mps,
				ep_cfg->ep_type);
}

int usb_dc_ep_set_stall(const u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		usb_set_bit8(EP0CS, EP0CS_STALL);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_set_bit8(OUTxCTRL(ep_idx), EPCTRL_STALL);
	} else {
		usb_set_bit8(INxCTRL(ep_idx), EPCTRL_STALL);
	}

	/* Reset the data toggle */
	usb_aotg_ep_reset(ep, USB_AOTG_EP_TOGGLE_RESET);

	return 0;
}

int usb_dc_ep_clear_stall(const u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (!ep_idx) {
		/*
		 * case 1: EP0 stall will be cleared when SETUP token comes
		 * case 2: we can clear EP0CS_STALL bit to clear stall
		 */
		usb_clear_bit8(EP0CS, EP0CS_STALL);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_clear_bit8(OUTxCTRL(ep_idx), EPCTRL_STALL);
	} else {
		usb_clear_bit8(INxCTRL(ep_idx), EPCTRL_STALL);
	}

	return 0;
}

int usb_dc_ep_is_stalled(const u8_t ep, u8_t *const stalled)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = 0;

	/* Endpoint 0 */
	if (!ep_idx && (usb_read8(EP0CS) & BIT(EP0CS_STALL))) {
		*stalled = 1;
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		if (usb_read8(OUTxCTRL(ep_idx)) & BIT(EPCTRL_STALL)) {
			*stalled = 1;
		}
	} else {
		if (usb_read8(INxCTRL(ep_idx)) & BIT(EPCTRL_STALL)) {
			*stalled = 1;
		}
	}

	return 0;
}

int usb_dc_ep_halt(const u8_t ep)
{
	/*
	 * Just set stall, by the way no clear halt operation?
	 */
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_enable(const u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);
	bool dir_out = USB_EP_DIR_IS_OUT(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (usb_aotg_ep_reset(ep, USB_AOTG_EP_RESET)) {
		return -EINVAL;
	}

	/* Enable EP interrupts */
	if (dir_out) {
		/* EP0 OUT uses OUT Token IRQ for assistance */
		if (!ep_idx) {
			usb_set_bit8(OUT_TOKIEN, EP0OUT_TOKEN);
		}
		usb_set_bit8(OUTIEN, ep_idx);
		usb_aotg_ctrl.out_ep_ctrl[ep_idx].ep_ena = 1;
	} else {
#if 0
		/* Using In Token IRQ for isochronous transfer */
		if (ep_type_isoc(usb_aotg_ctrl.in_ep_ctrl[ep_idx].ep_type)) {
			usb_set_bit8(IN_TOKIEN, ep_idx);
		} else {
			usb_set_bit8(INIEN, ep_idx);
		}
#else
		usb_set_bit8(INIEN, ep_idx);
#endif
		usb_aotg_ctrl.in_ep_ctrl[ep_idx].ep_ena = 1;
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		return 0;
	}

	if (dir_out) {
		/* Activate Ep */
		usb_set_bit8(OUTxCTRL(ep_idx), EPCTRL_VALID);
		/* Set FIFOCTRL */
		usb_write8(FIFOCTRL, ep_idx);
		/* Short Packet */
		/* usb_set_bit8(OUT_SHTPKT, ep_idx); */
	} else {
		usb_set_bit8(INxCTRL(ep_idx), EPCTRL_VALID);
		usb_write8(FIFOCTRL, ep_idx | FIFOCTRL_IO_IN);
	}

	/* Prepare EP for rx */
	if (dir_out) {
		usb_aotg_prep_rx(ep_idx);
	}

	usb_aotg_ep_reg_dump(ep);

	return 0;
}

int usb_dc_ep_disable(const u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);
	bool dir_out = USB_EP_DIR_IS_OUT(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	/* Disable EP interrupts */
	if (dir_out) {
		usb_clear_bit8(OUTIEN, ep_idx);
		usb_aotg_ctrl.out_ep_ctrl[ep_idx].ep_ena = 0;
	} else {
		usb_clear_bit8(INIEN, ep_idx);
		usb_aotg_ctrl.in_ep_ctrl[ep_idx].ep_ena = 0;
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		return 0;
	}

	/* De-activate Ep */
	if (dir_out) {
		usb_clear_bit8(OUTxCTRL(ep_idx), EPCTRL_VALID);
	} else {
		usb_clear_bit8(INxCTRL(ep_idx), EPCTRL_VALID);
	}

	return 0;
}

int usb_dc_ep_flush(const u8_t ep)
{
	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	return usb_aotg_ep_reset(ep, USB_AOTG_EP_FIFO_RESET);
}

int usb_dc_ep_write(const u8_t ep, const u8_t *const data,
				const u32_t data_len, u32_t * const ret_bytes)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);
	int ret;
	u32_t len;

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	/* Check if IN ep */
	if (!USB_EP_DIR_IS_IN(ep)) {
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usb_aotg_ep_is_enabled(ep)) {
		return -EINVAL;
	}

	len = usb_aotg_ctrl.in_ep_ctrl[USB_EP_ADDR2IDX(ep)].mps;

	if (data_len <= len) {
		len = data_len;
	} else {
		SYS_LOG_DBG("send %d bytes!", data_len);
	}

	ret = usb_aotg_tx(ep, data, len);
	if (ret < 0) {
		return ret;
	}

	if (ret_bytes) {
		*ret_bytes = ret;
	}

	if (ep_idx != 0)
		return 0;
	if (usb_aotg_ctrl.phase != USB_AOTG_IN_DATA) {
		return 0;
	}

	usb_aotg_ctrl.in_ep_ctrl[ep_idx].data_len -= ret;
	if ((usb_aotg_ctrl.in_ep_ctrl[ep_idx].data_len == 0) ||
	    (ret < MAX_PACKET_SIZE0)) {
		/* Case 1: Setup, In Data, Out Status */
		handle_status();
		usb_aotg_ctrl.phase = USB_AOTG_OUT_STATUS;
	}

	return 0;
}

int usb_dc_ep_read(const u8_t ep, u8_t *const data,
		const u32_t max_data_len, u32_t * const read_bytes)
{
	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_set_callback(const u8_t ep, const usb_dc_ep_callback cb)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		usb_aotg_ctrl.in_ep_ctrl[ep_idx].cb = cb;
	} else {
		usb_aotg_ctrl.out_ep_ctrl[ep_idx].cb = cb;
	}

	return 0;
}

int usb_dc_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
				u32_t *read_bytes)
{
	u32_t data_len, bytes_to_copy;
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		SYS_LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (!USB_EP_DIR_IS_OUT(ep)) {
		SYS_LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		SYS_LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usb_aotg_ep_is_enabled(ep)) {
		SYS_LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	data_len = usb_aotg_ctrl.out_ep_ctrl[ep_idx].data_len;

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero return
		 * the available data in buffer
		 */
		if (read_bytes) {
			*read_bytes = data_len;
		}

		return 0;
	}

	if (data_len > max_data_len) {
		SYS_LOG_ERR("Not enough room to copy all the rcvd data!");
		bytes_to_copy = max_data_len;
	} else {
		bytes_to_copy = data_len;
	}

	SYS_LOG_DBG("Read EP%d, req %d, read %d bytes",
		ep, max_data_len, bytes_to_copy);

	usb_aotg_rx(ep, data, bytes_to_copy);

	usb_aotg_ctrl.out_ep_ctrl[ep_idx].data_len -= bytes_to_copy;

	if (read_bytes) {
		*read_bytes = bytes_to_copy;
	}

	return 0;
}

int usb_dc_ep_read_continue(u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_ctrl.attached && !usb_aotg_ep_addr_valid(ep)) {
		SYS_LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (!USB_EP_DIR_IS_OUT(ep)) {
		SYS_LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (!usb_aotg_ctrl.out_ep_ctrl[ep_idx].data_len) {
		usb_aotg_prep_rx(ep_idx);
	}

	return 0;
}

int usb_dc_ep_mps(const u8_t ep)
{
	u8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		return usb_aotg_ctrl.out_ep_ctrl[ep_idx].mps;
	} else {
		return usb_aotg_ctrl.in_ep_ctrl[ep_idx].mps;
	}
}

bool usb_dc_attached(void)
{
	bool attached;

	if (usb_aotg_ctrl.attached) {
		return true;
	}

	usb_aotg_clock_enable();

	acts_reset_peripheral(RESET_ID_USB2);

	attached = usb_read8(DPDMCTRL) & BIT(PLUGIN);
	usb_aotg_clock_disable();

	return attached;
}

bool usb_dc_connected(void)
{
	return usb_aotg_ctrl.speed != USB_SPEED_UNKNOWN;
}

enum usb_device_speed usb_dc_maxspeed(void)
{
#ifdef CONFIG_USB_AOTG_UDC_FS
	return USB_SPEED_FULL;
#else
	return USB_SPEED_HIGH;
#endif
}

enum usb_device_speed usb_dc_speed(void)
{
	return usb_aotg_ctrl.speed;
}

static int usb_aotg_pre_init(struct device *dev)
{
	ARG_UNUSED(dev);

	usb_aotg_power_on();

	usb_aotg_clock_enable();

	usb_aotg_reset();

	usb_phy_setting(0xe7, 0x0);

	/* disable PHY PLL */
	usb_write8(USBPHYCTRL, 0x0);

	usb_aotg_clock_disable();

	usb_aotg_power_off();

	return 0;
}

SYS_INIT(usb_aotg_pre_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
