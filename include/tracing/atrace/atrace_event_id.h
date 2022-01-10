/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file defines for Actions trace event ID
 */

#ifndef __ATRACE_EVENT_ID_H
#define __ATRACE_EVENT_ID_H

#define ATRACE_MAX_MODULE_ID		8

#define ATE_MID_OS			(0x0)  /* OS */
#define ATE_MID_BTC			(0x1)  /* Bluetooth controller */
#define ATE_MID_BTH			(0x2)  /* Bluetooth host */
#define ATE_MID_MEDIA		(0x3)  /* media */

#define ATE_ID(mod_id, sub_id, eid)	((((mod_id) & 0x1f) << 11) | \
					(((sub_id) & 0x1f) << 6) | \
					((eid) & 0x3f))

struct ate_id {
	u16_t eid:6;
	u16_t sub_id:5;
	u16_t mod_id:5;
};

int ate_is_enabled(u16_t ate_id);
void ate_set_mid_enable(u8_t mid, int enable);
u32_t ate_get_mid_mask(void);
void ate_set_mid_mask(u32_t mid_mask);
u32_t ate_get_sid_mask(u8_t mid);
void ate_set_sid_mask(u8_t mid, u32_t sid_mask);

#endif /* __ATRACE_EVENT_ID_H */
