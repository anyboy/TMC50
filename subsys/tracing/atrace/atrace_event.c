/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing/atrace/atrace_event.h>

u32_t ate_mid_mask = BIT(ATE_MID_OS);
u32_t ate_sid_masks[ATRACE_MAX_MODULE_ID + 1] = {
	[ATE_MID_OS] = 0x3,
	[ATE_MID_BTC] = 0x0,
};

int ate_is_enabled(u16_t ate_id)
{
	struct ate_id *id = (struct ate_id *)&ate_id;

	if ((id->mod_id <= ATRACE_MAX_MODULE_ID) &&
	    (ate_mid_mask & (1u << id->mod_id)) &&
	    (ate_sid_masks[id->mod_id] & (1u << id->sub_id))) {
		return 1;
	}

	return 0;
}

void ate_set_mid_enable(u8_t mid, int enable)
{
	if (mid > ATRACE_MAX_MODULE_ID)
		return;

	if (enable)
	    ate_mid_mask |= 1u << mid;
	else
	    ate_mid_mask &= ~(1u << mid);
}

u32_t ate_get_mid_mask(void)
{
	return ate_mid_mask;
}

void ate_set_mid_mask(u32_t mid_mask)
{
	ate_mid_mask = mid_mask;
}

u32_t ate_get_sid_mask(u8_t mid)
{
	return ate_sid_masks[mid];
}

void ate_set_sid_mask(u8_t mid, u32_t sid_mask)
{
	if (mid > ATRACE_MAX_MODULE_ID)
		return;

	ate_sid_masks[mid] = sid_mask;
}

