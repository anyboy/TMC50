/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file PMU interface
 */

#include <kernel.h>
#include <soc.h>
#include <soc_pmu.h>

unsigned int soc_pmu_get_vdd_voltage(void)
{
	unsigned int sel, volt_mv;

	sel = sys_read32(VOUT_CTL0) & 0xf;

	volt_mv = 800 + sel * 50;

	return volt_mv;
}

void soc_pmu_set_vdd_voltage(unsigned int volt_mv)
{
	unsigned int sel;

	if (volt_mv < 800 || volt_mv > 1500)
		return;

	sel = (volt_mv - 800) / 50;
	sys_write32((sys_read32(VOUT_CTL0) & ~(0xf)) | (sel), VOUT_CTL0);

	k_busy_wait(1000);
}

void soc_pmu_set_seg_res_sel(unsigned int set_val)
{
	u32_t key;

	key = irq_lock();

	sys_write32((sys_read32(VOUT_CTL0) & ~((1)<<25)) | (set_val<<25), VOUT_CTL0);

	irq_unlock(key);
}

void soc_pmu_set_seg_dip_vcc_en(unsigned int set_val)
{
	u32_t key;

	key = irq_lock();

	sys_write32((sys_read32(VOUT_CTL0) & ~((1)<<24)) | (set_val<<24), VOUT_CTL0);

	irq_unlock(key);
}

void soc_pmu_set_seg_led_en(unsigned int set_val)
{
	u32_t key;

	key = irq_lock();

	sys_write32((sys_read32(VOUT_CTL0) & ~((1)<<23)) | (set_val<<23), VOUT_CTL0);

	irq_unlock(key);
}

void soc_pmu_avdd_enable(void)
{
	/* enable SPLL AVDD */
	sys_write32(sys_read32(VOUT_CTL1) | (1 << 15), VOUT_CTL1);

	/* disable SPLL_AVDD pull-down resistor */
	sys_write32((sys_read32(VOUT_CTL1) & ~(1 << 14)), VOUT_CTL1);

	/* enable AVDD LDO */
	sys_write32(sys_read32(VOUT_CTL0) | (1 << 19), VOUT_CTL0);

	/* disable AVDD pull-down resistor */
	sys_write32((sys_read32(VOUT_CTL0) & ~(1 << 18)), VOUT_CTL0);

	k_busy_wait(150);
}

void pmu_spll_init(void)
{
	/* enable SPLL AVDD/AVDD LDO */
	soc_pmu_avdd_enable();

	/* enable SPLL 32MHz/SPLL 48MHz/SPLL 64MHz clock */
	sys_write32(sys_read32(SPLL_CTL) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 0) , SPLL_CTL);
}

void pmu_init(void)
{
	sys_write32(0x12de224e, DCDC_CTL1);
	sys_write32(0x5085a992, DCDC_CTL2);
	k_busy_wait(200);

	/*disable VDD LDO pull down */
	sys_write32(sys_read32(VOUT_CTL0) & ~(0x3 << 14), VOUT_CTL0);

	/*disable OSCVDD pull down */
	sys_write32(sys_read32(SYSTEM_SET) & ~(0x3 << 12), SYSTEM_SET);
	k_busy_wait(200);

	/* disable VD15 pull down */
	sys_write32(sys_read32(SPD_CTL) & ~(1 << 2), SPD_CTL);

	/* VD15 switch to DCDC mode */
	sys_write32(sys_read32(VOUT_CTL1) | (1 << 8), VOUT_CTL1);

	/* disable the pull-down resistor of BANDGAP && enable filter resistor */
	sys_write32((sys_read32(BDG_CTL) & ~(1 << 5)) | (1 << 6), BDG_CTL);
	k_busy_wait(200);

	pmu_spll_init();
}
