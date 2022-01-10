/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2019-4-28-10:46:56AM             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_sleep.c
 * \brief
 * \author
 * \version  1.0
 * \date  2019-4-28-10:46:56AM
 *******************************************************************************/
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <watchdog.h>
#include <misc/printk.h>
#include <soc.h>
#include <stdlib.h>

#ifdef CONFIG_MEMORY
#include <mem_manager.h>
#endif

typedef void (*nor_send_command)(unsigned char cmd);

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
static u32_t deep_sleep_start_time;
static u32_t deep_sleep_total_time;
static u32_t light_sleep_start_time;
static u32_t light_sleep_total_time;
#endif

struct pmu_regs_backup {
	u32_t vout_ctl0;
	u32_t vout_ctl1;
	u32_t system_set;
	u32_t dcdc_ctl0;
	u32_t dcdc_ctl1;
};

/* COREPLL RW fields range from 0 t0 7 */
static u8_t corepll_backup;

/* Save and restore the registers */
static const u32_t backup_regs_addr[] = {
	CMU_SPI0CLK,
	CMU_SYSCLK,
	PMUADC_CTL,
	WIO0_CTL,
	SPD_CTL,
	AUDIO_PLL1_CTL,
	AUDIO_PLL0_CTL,
	CMU_DEVCLKEN1,
	CMU_DEVCLKEN0,
	RMU_MRCR1,
	RMU_MRCR0,
	CMU_MEMCLKEN,
};

static u32_t s2_reg_backups[ARRAY_SIZE(backup_regs_addr)];

/* The 'white list' of clock source that enabled during s3 stage */
static const u8_t deep_sleep_enable_clock_id[] =
{
    CLOCK_ID_UART0,
    CLOCK_ID_DMA,
    CLOCK_ID_TIMER0,
    CLOCK_ID_SPI0_CACHE,
    CLOCK_ID_SPI0,
    CLOCK_ID_BT_BB_3K2,
    CLOCK_ID_BT_BB_DIG,
    CLOCK_ID_BT_BB_AHB,
    CLOCK_ID_BT_MODEM_AHB,
    CLOCK_ID_BT_MODEM_DIG,
    CLOCK_ID_BT_MODEM_INTF,
    CLOCK_ID_BT_RF,
#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ
    CLOCK_ID_IR,
#endif
};

static void sys_pm_backup_registers(u32_t *backup_buffer)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(backup_regs_addr); i++)
		backup_buffer[i] = sys_read32(backup_regs_addr[i]);
}

static void sys_pm_restore_registers(u32_t *backup_buffer)
{
	int i;
	for (i = ARRAY_SIZE(backup_regs_addr) - 1; i >= 0; i--)
		sys_write32(backup_buffer[i], backup_regs_addr[i]);
}

void __ramfunc sys_norflash_power_ctrl(u32_t is_powerdown)
{
	nor_send_command p_send_command = (nor_send_command)0xbfc020d9;
	volatile int i;
	u32_t spi_ctl_ori = sys_read32(SPI0_REG_BASE);

	/* If spi mode is not the disable or write only mode, we need to disable firstly */
	if (((spi_ctl_ori & 0x3) != 0) && ((spi_ctl_ori & 0x3) != 2)) {
		sys_write32(sys_read32(SPI0_REG_BASE) & ~(3 << 0), SPI0_REG_BASE);
		for (i = 0; i < 5; i++) {
			;
		}
	}

	/* enable AHB interface for cpu write cmd */
	sys_write32(0x8013A, SPI0_REG_BASE);
	for(i = 0; i < 20; i++) {
		;
	}

	if (is_powerdown){
		/* 4x io need send 0xFF to exit the continuous mode */
		if (spi_ctl_ori & (0x3 << 10))
			p_send_command(0xFF);
		p_send_command(0xB9);
	} else {
		p_send_command(0xAB);
	}

	for(i = 0; i < 1000; i++) {
		;
	}

	/* set spi in disable mode */
	sys_write32(sys_read32(SPI0_REG_BASE) & ~(3 << 0), SPI0_REG_BASE);
	for (i = 0; i < 5; i++) {
		;
	}

	sys_write32(spi_ctl_ori, SPI0_REG_BASE);
}

int __ramfunc sys_pm_enter_s3_and_check_wake_up_from_s3bt(void)
{
    int wakeup_pending;

    sys_norflash_power_ctrl(true);

    /* S1-->S3 */
    sys_write32((1<<1), PMU_POWER_CTL);

    while(1){
        wakeup_pending = sys_pm_get_wakeup_pending();

        if (wakeup_pending) {
            break;
        }
    }

    sys_norflash_power_ctrl(false);

    return 0;
}

static u32_t soc_cmu_deep_sleep(void)
{
    u32_t spll_backup ;

    spll_backup = sys_read32(SPLL_CTL);

    /*  close dev clk except bt, spi controller */
    acts_deep_sleep_clock_peripheral(deep_sleep_enable_clock_id, ARRAY_SIZE(deep_sleep_enable_clock_id));

    /* switch spi0 clock to HOSC(24MHz) */
    sys_write32((sys_read32(CMU_SPI0CLK) & ~0x3FF) | (0x1 << 8), CMU_SPI0CLK);

    /* switch cpu to HOSC(24MHz) */
    sys_write32((sys_read32(CMU_SYSCLK) & ~(0xf << 4)) | (0xf << 4), CMU_SYSCLK);
    sys_write32((sys_read32(CMU_SYSCLK) & ~0x3) | 0x1, CMU_SYSCLK);

    /* disable hardware memory clock excludes RAM0 ~ RAM6 clock */
    sys_write32(sys_read32(CMU_MEMCLKEN) & (~(0xfffff800)), CMU_MEMCLKEN);

    /* disable SPLL clock */
    sys_write32(0, SPLL_CTL);

    return spll_backup;
}

static void soc_cmu_deep_wakeup(u32_t spll_backup)
{
    /* restore spll */
    sys_write32(spll_backup, SPLL_CTL);
    k_busy_wait(200);
}

static void soc_pmu_deep_sleep(struct pmu_regs_backup *pmu_backup)
{
	pmu_backup->vout_ctl0 = sys_read32(VOUT_CTL0);
	pmu_backup->vout_ctl1 = sys_read32(VOUT_CTL1);
	pmu_backup->system_set = sys_read32(SYSTEM_SET);
	pmu_backup->dcdc_ctl0 = sys_read32(DCDC_CTL1);
	pmu_backup->dcdc_ctl1 = sys_read32(DCDC_CTL2);

	/* disable SEG_LED_EN */
	sys_write32(sys_read32(VOUT_CTL0) & ~(1 << 23), VOUT_CTL0);

	/* disable AVDD */
	sys_write32(sys_read32(VOUT_CTL0) & ~(1 << 19), VOUT_CTL0);

	/* disable VDD pull down */
	sys_write32(sys_read32(VOUT_CTL0) & ~(0x3 << 14), VOUT_CTL0);
	/* VDD S3BT voltage: 0.9V */
	sys_write32((sys_read32(VOUT_CTL0) & ~(0x7 << 4)) | (0x2 << 4), VOUT_CTL0);

	/* disable SPLL_AVDD */
	sys_write32(sys_read32(VOUT_CTL1) & ~(1 << 15), VOUT_CTL1);

	/* VD15 select DCDC mode */
	sys_write32(sys_read32(VOUT_CTL1) | (1 << 8), VOUT_CTL1);

	/* VD15 S3BT voltage: 1.2V */
	sys_write32((sys_read32(VOUT_CTL1) & ~(0xF << 4)) | (0x4 << 4), VOUT_CTL1);

	sys_write32(sys_read32(BDG_CTL) & ~(1 << 5), BDG_CTL);

	sys_write32(0, SPD_CTL);

	/* disable OSCVDD pull down */
	sys_write32(0x4dc, SYSTEM_SET);

	/* USB DM/DP pins cause about 200uA */
	//sys_write32(0, 0xc0090028);
	//sys_write32(0, 0xc009002c);

	/* SD CMD/D0 pins cause about 40uA */
	//sys_write32(0, 0xc0090000);
	//sys_write32(0, 0xc0090008);

	sys_write32(0x13de224a, DCDC_CTL1);
	sys_write32(0x100da992, DCDC_CTL2);

	sys_write32(0, PMUADC_CTL);
}

static void soc_pmu_deep_wakeup(struct pmu_regs_backup *pmu_backup)
{
	sys_write32(pmu_backup->vout_ctl0, VOUT_CTL0);
	sys_write32(pmu_backup->vout_ctl1, VOUT_CTL1);
	sys_write32(pmu_backup->system_set, SYSTEM_SET);
	sys_write32(pmu_backup->dcdc_ctl0, DCDC_CTL1);
	sys_write32(pmu_backup->dcdc_ctl1, DCDC_CTL2);
}

#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ

__ramfunc void delay_us_24m(int us)
{
	volatile int count = us * 3;
	while (count-- > 0);
}

__ramfunc void delay_ms_32k(int ms)
{
	volatile int count = ms * 4;
	while (count-- > 0);
}

u32_t reg_bak_CMU_SYSCLK;

__ramfunc int sys_pm_enter_s2_lowfreq(void)
{
	reg_bak_CMU_SYSCLK = sys_read32(CMU_SYSCLK);

	/* 32KHz */
	sys_write32(0, CMU_SYSCLK);
	delay_ms_32k(1);

	return 0;
}

__ramfunc int sys_pm_exit_s2_lowfreq(void)
{
	/* HOSC or COREPLL */
	sys_write32(reg_bak_CMU_SYSCLK, CMU_SYSCLK);
	delay_us_24m(1);

	return 0;
}

__ramfunc int sys_pm_deep_sleep_routinue_s2_lowfreq(void)
{
	int wakeup_src = 0;
	u32_t intc_pending;

	sys_pm_enter_s2_lowfreq();

	while (1) {
		intc_pending = (sys_read32(INTC_PD0) & sys_read32(INTC_MSK0));
		if (intc_pending & (STANDBY_VALID_INTC_PD0 | (1<<IRQ_ID_BT_BASEBAND))) {
			break;
		}

		if (sys_read32(PMU_ONOFF_KEY) & 0x1) {
			wakeup_src = 1;
			break;
		}

		/* TODO: if it will stay here for a long long time, we must add dv5c, low power wakeup here!! */
	}

	sys_pm_exit_s2_lowfreq();

	return wakeup_src;
}
#endif


int sys_pm_deep_sleep_routinue(void)
{
	int wakeup_src = 0;
	u32_t backup_reg[ARRAY_SIZE(backup_regs_addr)];

	u32_t spll_backup;

	struct pmu_regs_backup pmu_regs_backup = {0};

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
	u32_t deep_sleep_time;
#endif

	sys_pm_backup_registers(backup_reg);

	spll_backup = soc_cmu_deep_sleep();

	soc_pmu_deep_sleep(&pmu_regs_backup);

	k_busy_wait(300);

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
	deep_sleep_start_time = sys_pm_get_rc_timestamp();

	if(light_sleep_start_time){
		light_sleep_total_time += deep_sleep_start_time - light_sleep_start_time;
	}
#endif

#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ
	wakeup_src = sys_pm_deep_sleep_routinue_s2_lowfreq();
#else
	sys_pm_enter_s3_and_check_wake_up_from_s3bt();
#endif

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
	deep_sleep_time = sys_pm_get_rc_timestamp() - deep_sleep_start_time;

	deep_sleep_total_time += deep_sleep_time;
	/* compensate ticks with sleep time */
	_sys_clock_tick_count += _ms_to_ticks(deep_sleep_time);

	light_sleep_start_time = sys_pm_get_rc_timestamp();
#endif

	soc_pmu_deep_wakeup(&pmu_regs_backup);

	soc_cmu_deep_wakeup(spll_backup);

	sys_pm_restore_registers(backup_reg);

	return wakeup_src;
}

int sys_pm_enter_deep_sleep(void)
{
	u32_t key;
	int wakeup_src;

	key = irq_lock();

#ifndef CONFIG_AUTO_STANDBY_S2_LOWFREQ
	sys_pm_set_wakeup_src(1);
	sys_pm_clear_wakeup_pending();
#endif

	wakeup_src = sys_pm_deep_sleep_routinue();

#ifndef CONFIG_AUTO_STANDBY_S2_LOWFREQ
	wakeup_src = sys_pm_get_wakeup_pending();
#endif

	irq_unlock(key);

	return wakeup_src;
}

static void soc_pmu_light_sleep(void)
{
	/* disable adc */
	sys_write32(0, PMUADC_CTL);
	/* disable WIO0 */
	sys_write32(0, WIO0_CTL);
}

static u32_t soc_cmu_light_sleep(void)
{
    u32_t corepll_backup;

    sys_write32(0, AUDIO_PLL0_CTL);
    sys_write32(0, AUDIO_PLL1_CTL);

    /* switch spi0 clock to ck48M */
    sys_write32((sys_read32(CMU_SPI0CLK) & ~0x3FF) | 0x300, CMU_SPI0CLK);

    /* switch cpu to 48M */
    sys_write32((sys_read32(CMU_SYSCLK) & ~0x3) | 0x3, CMU_SYSCLK);
    sys_write32((sys_read32(CMU_SYSCLK) & ~(0xf << 4)) | (0xb << 4), CMU_SYSCLK);

    /* disable coreplls */
    corepll_backup = sys_read32(CMU_COREPLL_CTL);
    sys_write32(sys_read32(CMU_COREPLL_CTL) & ~(1 << 7), CMU_COREPLL_CTL);
    sys_write32(0, CORE_PLL_CTL);

    return corepll_backup;
}

static void soc_cmu_ligth_wakeup(u32_t corepll_backup)
{
    /* restore core pll */
    sys_write32(corepll_backup, CMU_COREPLL_CTL);
    k_busy_wait(100);
}

void sys_pm_enter_light_sleep(void)
{
	u32_t key;

	key = irq_lock();

    sys_pm_backup_registers(s2_reg_backups);

    soc_pmu_light_sleep();

    corepll_backup = soc_cmu_light_sleep();

    irq_unlock(key);
}

void sys_pm_exit_light_sleep(void)
{
    soc_cmu_ligth_wakeup(corepll_backup);
    sys_pm_restore_registers(s2_reg_backups);
}

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
int sys_pm_get_deep_sleep_time(void)
{
	return deep_sleep_total_time;
}

int sys_pm_get_light_sleep_time(void)
{
	return light_sleep_total_time;
}
#endif

