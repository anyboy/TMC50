/*
 * Copyright (c) 2020 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB HID device descriptors
 */

#ifndef __USB_HID_DESC_H__
#define __USB_HID_DESC_H__

#include <usb/usb_common.h>

#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

static const u8_t usb_hid_fs_desc[] = {


};

static const u8_t usb_hid_hs_desc[] = {


};

#endif/*__USB_HID_DESC_H__*/
