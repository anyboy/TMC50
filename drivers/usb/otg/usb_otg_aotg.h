/*
 * Copyright (c) 2020 Actions Corporation.
 * Author: Jinang Lv <lvjinang@actions-semi.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Actions OTG controller driver private definitions
 *
 * This file contains the Actions OTG USB controller driver private
 * definitions.
 */

#ifndef __USB_OTG_AOTG_H__
#define __USB_OTG_AOTG_H__

#include <sys_io.h>
#include <board.h>

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))
#endif

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

/* Max device address */
#define USB_AOTG_MAX_ADDR	0x7F

#define USB_AOTG_EP0_IDX	0

/* AOTG EP reset type */
enum usb_aotg_ep_reset_type {
	USB_AOTG_EP_FIFO_RESET = 0,
	USB_AOTG_EP_TOGGLE_RESET,
	/* Toggle & FIFO reset */
	USB_AOTG_EP_RESET
};

/* AOTG FIFO type */
enum usb_aotg_fifo_type {
	USB_AOTG_FIFO_SINGLE = 0,	/* default */
	USB_AOTG_FIFO_DOUBLE,
	USB_AOTG_FIFO_TRIPLE,
	USB_AOTG_FIFO_QUAD
};

/**
 * There are 2 or 3 stages: Setup, Data (optional), Status
 * for control transfer.
 * There are 3 cases that may exist during control transfer
 * which we have to take care to make AOTG work fine.
 * Case 1: Setup, In Data, Out Status;
 * Case 2: Setup, In Status;
 * Case 3: Setup, Out Data, In Status;
 */
enum usb_aotg_ep0_phase {
	USB_AOTG_SETUP = 1,
	USB_AOTG_IN_DATA,
	USB_AOTG_OUT_DATA,
	USB_AOTG_IN_STATUS,
	USB_AOTG_OUT_STATUS,
	/* Special phase which is necessary */
	USB_AOTG_SET_ADDRESS
};

/*
 * version list: Append if need
 */
#define USB_AOTG_VERSION_ANDESM	0x551A20FF
#define USB_AOTG_VERSION_WOODPECKER	0x551A18CF

/*
 * specific header file
 */
#if (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_ANDESM)
#include "usb_aotg_andesm.h"
#elif (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_WOODPECKER)
#include "usb_aotg_woodpecker.h"
#else
#error Wrong CONFIG_USB_AOTG_OTG_VERSION
#endif

#endif /* __USB_OTG_AOTG_H__ */
