/*
 * USB STUB core header
 *
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB STUB Class public header
 */

#ifndef __USB_STUB_H__
#define __USB_STUB_H__

struct device;

bool usb_stub_enabled(void);
bool usb_stub_configured(void);
int usb_stub_init(struct device *dev, void *param);
int usb_stub_exit(void);
int usb_stub_ep_read(u8_t *data_buffer, u32_t data_len, u32_t timeout);
int usb_stub_ep_write(u8_t *data_buffer, u32_t data_len, u32_t timeout);

#endif /* __USB_STUB_H__ */

