/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system reboot interface for Actions SoC
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <watchdog.h>
#include <misc/printk.h>
#include <soc.h>
#include <rtc.h>

#define UPDATE_MAGIC  0xA596
#define UPDATE_OK	  0x5A69

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
static u32_t idle_start_time;
static u32_t idle_total_time;
#endif

/*
** update RTCVDD registers
*/
void sys_pm_update_rtc_regs(void)
{
    sys_write32(UPDATE_MAGIC, RTC_REGUPDATE);

    while (sys_read32(RTC_REGUPDATE) != UPDATE_OK)
    {
        ;    // wait rtc register update
    }
}

bool sys_pm_get_power_5v_status(void)
{
#ifdef CONFIG_HOTPLUG_DC5V_DETECT
	int dc5v_pin_level = !!(sys_read32(WIO0_CTL + CONFIG_HOTPLUG_DC5V_WIO*0x04) & (1 << WIO0_CTL_WIODAT));
	return (dc5v_pin_level == CONFIG_HOTPLUG_DC5V_LEVEL);
#else
	return false;
#endif
}

/*
**  configure the press time of on-off key
*/
void sys_pm_config_onoff_time(uint32_t value_ms)
{
	uint32_t val,val_tmp;

    /*select the nearest val set to reg*/
    if (value_ms < 187){
        val_tmp = 0;
    }else if (value_ms < 375){
        val_tmp = 1;
    }else if (value_ms < 750){
        val_tmp = 2;
    }else if (value_ms < 1250){
        val_tmp = 3;
    }else if (value_ms < 1750){
        val_tmp = 4;
    }else if (value_ms < 2500){
        val_tmp = 5;
    }else if (value_ms < 3500){
        val_tmp = 6;
    }else {
        val_tmp = 7;
    }

	/* config on/off key long press time */
	val = sys_read32(PMU_ONOFF_KEY);
	val &= ~(0x7 << 8);
	val |= ((val_tmp & 0x07) << 8);
	sys_write32(val, PMU_ONOFF_KEY);
	k_busy_wait(200);
}

void sys_pm_clear_wakeup_pending(void)
{
	uint32_t val;

	uint32_t start_cycle;

	val = sys_read32(PMU_WAKE_PD);
	/* WAKE_PD bit[10] and bit[13:15] are reseved and can't write 1 */
	val |= (0x3fffff & ~((1 << 10) | (0x7 << 13)));
	sys_write32(val, PMU_WAKE_PD);
	k_busy_wait(200);

	start_cycle = k_cycle_get_32();

	while(1){
        if((sys_read32(PMU_WAKE_PD) & 0x3fffff) == 0){
            break;
        }

        // wait almost 500ms
        if((k_cycle_get_32() - start_cycle) > 500 * 24){
            break;
        }
	}
}

int __ramfunc sys_pm_get_wakeup_pending(void)
{
	int val;

	val = sys_read32(PMU_WAKE_PD);

    //if(val != 0){
    //    printk("%d, get wakeup pend: 0x%x\n\n", __LINE__, val);
    //}

	return val;
}

int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src)
{
	u32_t wk_pd;

	if (!src)
		return -EINVAL;

	src->data = 0;

	wk_pd = sys_read32(PMU_WAKE_PD);

	printk("wk_pdp:0x%x\n",wk_pd);

	if (wk_pd & PMU_WAKE_PD_ONOFF_WK_PD)
		src->t.onoff = 1;

	if (wk_pd & PMU_WAKE_PD_RESET_PD)
		src->t.reset = 1;

	if (wk_pd & PMU_WAKE_PD_BATWK_PD)
		src->t.bat = 1;

	if (wk_pd & PMU_WAKE_PD_WIO0_PD)
		src->t.wio0 = 1;

	if (wk_pd & PMU_WAKE_PD_WIO1_PD)
		src->t.wio1 = 1;

	if (wk_pd & PMU_WAKE_PD_WIO2_PD)
		src->t.wio2 = 1;

	if (wk_pd & PMU_WAKE_PD_BT_WK_PD)
		src->t.bt_wk = 1;

	if (wk_pd & PMU_WAKE_PD_REMOTE_PD)
		src->t.remote = 1;

	if (wk_pd & PMU_WAKE_PD_SIRQ0_WK_PD)
		src->t.sirq0 = 1;

	if (wk_pd & PMU_WAKE_PD_SIRQ1_WK_PD)
		src->t.sirq1 = 1;

	if (soc_boot_get_watchdog_is_reboot() == 1)
		src->t.watchdog = 1;

#ifdef CONFIG_RTC_ACTS
	if (rtc_is_alarm_wakeup(device_get_binding(CONFIG_RTC_0_NAME)))
		src->t.alarm = 1;
#endif

	return 0;
}

/*
** set wakeup souce
*/
void sys_pm_set_wakeup_src(uint8_t mode)
{
	uint32_t key, val;

	key = irq_lock();

	sys_write32(sys_read32(PMU_WAKE_PD), PMU_WAKE_PD);
	k_busy_wait(200);

	val = sys_read32(PMU_WKEN_CTL);
	// disable all wakeup source
	val &= ~(0x1FFF);

    /* FIXME need add wio0 */

	val = PMU_WKEN_CTL_RESET_WKEN
#ifdef CONFIG_SOC_WKEN_BAT
	      | PMU_WKEN_CTL_BAT_WKEN
#endif
#ifdef CONFIG_SOC_WKEN_ONOFF_SHORT
	      | PMU_WKEN_CTL_SHORT_WKEN
#endif
	      | PMU_WKEN_CTL_LONG_WKEN
		  | PMU_WKEN_CTL_ALARM_WKEN;

#ifdef CONFIG_HOTPLUG_DC5V_DETECT
    if (mode == 2) {
        // ignore power detection
    } else {
#if (CONFIG_HOTPLUG_DC5V_WIO == 0)
	val |= PMU_WKEN_CTL_WIO0_WKEN;
#elif (CONFIG_HOTPLUG_DC5V_WIO == 1)
	val |= PMU_WKEN_CTL_WIO1_WKEN;
#else
	val |= PMU_WKEN_CTL_WIO2_WKEN;
#endif
    }
#endif

	if(mode == 0  || mode == 2) {
		// set wakeup source from S4
	} else {
		// set wakeup source from S3BT or S2
		val |= PMU_WKEN_CTL_REMOTE0_WK_EN | PMU_WKEN_CTL_SHORT_WKEN | PMU_WKEN_CTL_BT_WK_EN;
	}

	sys_write32(val, PMU_WKEN_CTL);
	k_busy_wait(200);

	/* WIO1 high voltage to wakeup */
#ifdef CONFIG_HOTPLUG_DC5V_DETECT
	val = sys_read32(WIO0_CTL + CONFIG_HOTPLUG_DC5V_WIO*0x04);
	val &= ~(0x07 << 21);
#if (CONFIG_HOTPLUG_DC5V_LEVEL == 1)
	val |= (0x03 << 21);
#else
	val |= (0x04 << 21);
#endif
	sys_write32(val, WIO0_CTL + CONFIG_HOTPLUG_DC5V_WIO*0x04);
	k_busy_wait(200);
#endif

	sys_pm_config_onoff_time(CONFIG_SOC_WKEN_ONOFF_LONG_PRESS_TIMER);

	irq_unlock(key);
}

/*
**  system power off
*/
static void _sys_pm_poweroff(u8_t mode)
{
	uint32_t key;
	uint32_t wake_pd;
    uint32_t key_pressed;

#ifdef CONFIG_WDT_ACTS_DEVICE_NAME
	struct device *wdg;
#endif
    do{
        /* bits define */
        #define PMU_ONOFF_KEY_ONOFF_PRESSED	(1 << 0)
	    key_pressed = !!(sys_read32(PMU_ONOFF_KEY) & PMU_ONOFF_KEY_ONOFF_PRESSED);
    }while(key_pressed);

	/* wait 10ms, avoid trigger onoff wakeup pending */
	k_busy_wait(10000);

	printk("system power down!\n");

	sys_pm_set_wakeup_src(mode);

	key = irq_lock();

#ifdef CONFIG_WDT_ACTS_DEVICE_NAME
	wdg = device_get_binding(CONFIG_WDT_ACTS_DEVICE_NAME);
	if (wdg) {
		wdt_disable(wdg);
	}
#endif

	/* enable VD15 & VCC pull down for the repid discharge */
	sys_write32(sys_read32(SPD_CTL) | (1 << 2) | (1 << 0), SPD_CTL);

	/* disable s3bt */
	sys_write32(sys_read32(PMU_POWER_CTL) & ~(1 << 1), PMU_POWER_CTL);
	k_busy_wait(500);

	/* enable s3 */
	sys_write32(sys_read32(PMU_POWER_CTL) | (1 << 2), PMU_POWER_CTL);
	k_busy_wait(500);

	while(1) {
		sys_write32(sys_read32(PMU_POWER_CTL) & ~(1 << 0), PMU_POWER_CTL);

		/* wait 10ms */
		k_busy_wait(10000);

		/* cannot enter s3/4, poll wakeup source */
		wake_pd = sys_read32(PMU_WAKE_PD) & (PMU_WAKE_PD_ONOFF_WK_PD|PMU_WAKE_PD_SHORT_ONOFF_PD);
		if (wake_pd) {
			printk("poweroff: wake_pd 0x%x, need reboot!\n", wake_pd);
			sys_pm_reboot(0);
		}
	}

	/* never return... */
}

void sys_pm_poweroff(void)
{
    _sys_pm_poweroff(0);
}

void sys_pm_poweroff_dc5v(void)
{
    _sys_pm_poweroff(2);
}


u32_t sys_pm_get_rc_timestamp(void)
{
	/* 3.2K RC timer */
	return (sys_read32(0xc017601c) * 10 / 32);
}

/* check if the timer pending is set or not  */
//static u32_t sys_pm_get_rc_pending(void)
//{
//    /* 3.2K RC timer pending */
//    return (sys_read32(0xc017602c) & (1 << 5));
//}


#ifdef CONFIG_PM_CPU_IDLE_LOW_POWER
static uint32_t sysclk_bak;

static void sys_pm_enter_low_clock(void)
{
    sysclk_bak = sys_read32(CMU_SYSCLK);
	/* switch cpu to 1.5M */
	sys_write32((sys_read32(CMU_SYSCLK) & ~(0xf << 4)) | (0x7 << 4), CMU_SYSCLK);
	sys_write32((sys_read32(CMU_SYSCLK) & ~0x3) | 0x01, CMU_SYSCLK);
}

static void sys_pm_exit_low_clock(void)
{
	/* switch cpu to 56M */
	sys_write32(sysclk_bak, CMU_SYSCLK);
}

static unsigned int origin_cpu_div;

void soc_pm_cpu_enter_lowpower(void)
{
	unsigned int core_freq, cpu_idle_div;

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
	idle_start_time = sys_pm_get_rc_timestamp();
#endif

	core_freq = soc_freq_get_core_freq();

	// if corepll is disabled that is mean under the standby state and can lower the cpu clock as 3M when idle.
	if(core_freq){
        cpu_idle_div = (16 * (CONFIG_PM_CPU_IDLE_FREQ_KHZ / 1000) - 1) / core_freq;
        if (cpu_idle_div > 15)
            cpu_idle_div = 15;

        origin_cpu_div = (sys_read32(CMU_SYSCLK) >> 4) & 0xf;
        sys_write32((sys_read32(CMU_SYSCLK) & ~(0xf<<4)) | (cpu_idle_div << 4), CMU_SYSCLK);
	}else{
        sys_pm_enter_low_clock();
	}
}

void soc_pm_cpu_exit_lowpower(void)
{
    unsigned int core_freq;

	core_freq = soc_freq_get_core_freq();

    if(core_freq){
        sys_write32((sys_read32(CMU_SYSCLK) & ~(0xf<<4)) | (origin_cpu_div << 4), CMU_SYSCLK);
    }else{
        sys_pm_exit_low_clock();
    }

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
    idle_total_time += sys_pm_get_rc_timestamp() - idle_start_time;
#endif

}

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
int sys_pm_get_idle_sleep_time(void)
{
    return idle_total_time;
}
#endif

#endif


#define REBOOT_REASON_MAGIC 		0x4252	/* 'RB' */
#define PRODUCT_FLAG_MAGIC 			0x5250	/* 'PR' */

/*
**  system reboot
*/
void sys_pm_reboot(int type)
{
	unsigned int key;

	printk("system reboot, type 0x%x!\n", type);

	key = irq_lock();

	/* store reboot reason in RTC_BAK3 for bootloader */
	sys_write32((REBOOT_REASON_MAGIC << 16) | (type & 0xffff), RTC_BAK3);
	sys_pm_update_rtc_regs();

	k_busy_wait(500);

	sys_write32(0x10, WD_CTL);

	while (1) {
		;
	}

	/* never return... */
}


/*
**  set product flag, use it for att card product
*/
void sys_pm_set_product_flag(int flag)
{
	printk("set product flag 0x%x!\n", flag);

	/* store product flag in RTC_BAK2 */
	sys_write32((PRODUCT_FLAG_MAGIC << 16) | (flag & 0xffff), RTC_BAK2);
	sys_pm_update_rtc_regs();
}

/*
**  get product flag, use it for att card product
*/
int sys_pm_get_product_flag(void)
{
	u32_t reg_val;
	int flag = 0;

	reg_val = sys_read32(RTC_BAK2);
	if ((reg_val >> 16) == PRODUCT_FLAG_MAGIC) {
		flag = (reg_val & 0xffff);

		/* clear product flag in RTC_BAK2 */
		sys_write32(0, RTC_BAK2);
		sys_pm_update_rtc_regs();
	}

	printk("get product flag 0x%x!\n", flag);

	return flag;
}


/*!
 * \brief temperature ADC collection control
 */
void sys_pm_temp_adc_ctrl_enable(uint32_t enable)
{
    uint32_t adc_ctl = sys_read32(PMUADC_CTL);

    if (enable)
    {
        if (!(adc_ctl & (1 << PMUADC_CTL_SENSORADC_EN)))
        {
            //PMU.PMUADC_CTL &= ~(1 << PMUADC_CTL_DC5VADC_EN);
            adc_ctl |=  (1 << PMUADC_CTL_SENSORADC_EN);
        }
    }
    else
    {
        if (adc_ctl & (1 << PMUADC_CTL_SENSORADC_EN))
        {
            adc_ctl &= ~(1 << PMUADC_CTL_SENSORADC_EN);
            //PMU.PMUADC_CTL |=  (1 << PMUADC_CTL_DC5VADC_EN);

            //p->dc5v_adc_enable_time = get_tick_msec();
        }
    }

}


/*!
 * \brief get temperature
 */
int sys_pm_temp_adc_get_temperature(void)
{
    int  temp  = 0;

    int  adc_val = ((sys_read32(SENSADC_DATA) & SENSADC_DATA_SENSADC_MASK) \
        >> SENSADC_DATA_SENSADC_SHIFT);

    /* sample temerature formula: temp = adc_val * 1.299 - 41.5
     */
    temp = (adc_val * 12990 / 1000 - 415) / 10;

    return temp;
}


static int soc_pm_init(struct device *arg)
{
	ARG_UNUSED(arg);

#ifndef CONFIG_SOC_WKEN_BAT
	u32_t wk_pd;

	wk_pd = sys_pm_get_wakeup_pending();
	if (wk_pd == PMU_WAKE_PD_BATWK_PD) {
		sys_pm_poweroff();
	}
#endif

	return 0;
}

int sys_pm_get_reboot_reason(u16_t *reboot_type, u8_t *reason)
{
	int ret = -1;
	u32_t reg_val;

	reg_val = soc_boot_get_reboot_reason();
	if ((reg_val >> 16) == REBOOT_REASON_MAGIC) {
		*reboot_type = (reg_val & 0xff00);
		*reason      = (reg_val & 0x00ff);
		ret = 0;
	}

	return ret;
}

SYS_INIT(soc_pm_init, POST_KERNEL, 20);
