/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file debug configuration interface for Actions SoC
 */

#include <kernel.h>
#include <soc.h>
#include <soc_debug.h>

void soc_debug_enable_jtag(int cpu_id, int group)
{
	ARG_UNUSED(cpu_id);
	if (group == 3) {
		/* Disable SEG-LED analog bias for ejtag */
		sys_write32(0, 0xc0190020);
	}
	sys_write32((sys_read32(JTAG_CTL) & ~0x3) |	((group & 0x3) << 0) | (1 << 4), JTAG_CTL);
}

void soc_debug_disable_jtag(int cpu_id)
{
	ARG_UNUSED(cpu_id);

	sys_write32(sys_read32(JTAG_CTL) & ~(1 << 4), JTAG_CTL);
}
