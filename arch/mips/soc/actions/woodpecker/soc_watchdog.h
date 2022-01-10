/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SOC utils interface
 */

#ifndef __SOC_WATHDOG_H__
#define __SOC_WATHDOG_H__

#include "soc_regs.h"

/* clear soc watchdog */
static inline void soc_watchdog_clear(void)
{
	sys_write32(sys_read32(WD_CTL) | 0x1, WD_CTL);
}

static inline void soc_watchdog_stop(void)
{
	sys_write32(sys_read32(WD_CTL) & ~(1 << 4), WD_CTL);
}

#endif /* __SOC_WATCHDOG_H__ */
