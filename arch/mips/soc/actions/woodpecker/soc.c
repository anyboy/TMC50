/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC init for Actions SoC.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <arch/cpu.h>
#include <string.h>
#include "soc_version.h"

static int soc_init(struct device *arg)
{
	ARG_UNUSED(arg);

#ifdef CONFIG_XIP

#define     SPI0_BASE  0xC0110000
#define     SPI_CTL    (SPI0_BASE+0x0000)
	sys_write32(sys_read32(SPI_CTL) | (3 << 22), SPI_CTL); //CRC ERROR TIMES = 16

#if (CONFIG_CACHE_SIZE == 16)
	/* Cache Size Use 16KB, another 16KB Use as Sram */
	soc_memctrl_config_cache_size(CACHE_SIZE_16KB);
#endif

#endif

	pmu_init();

	soc_cmu_init();

	/* enable cpu clock gating and all interrupt wakeup sources by default */
	sys_write32(0xffffffff, M4K_WAKEUP_EN0);
	sys_write32(0x7fffffff, M4K_WAKEUP_EN1);

	/* config timer0/1 clock source to HOSC */
	sys_write32((sys_read32(CMU_TIMER0CLK) & ~0x1) | 0x0, CMU_TIMER0CLK);
	sys_write32((sys_read32(CMU_TIMER1CLK) & ~0x1) | 0x0, CMU_TIMER1CLK);

	soc_freq_set_cpu_clk(0, CONFIG_SOC_CPU_CLK_FREQ / 1000);

	/*  disable all ADC channels by default to save power consume */
	sys_write32(sys_read32(PMUADC_CTL) & ~0xffff, PMUADC_CTL);

	return 0;
}

uint32_t libsoc_version_get(void)
{
    return LIBSOC_VERSION_NUMBER;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 20);
