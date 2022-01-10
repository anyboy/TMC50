/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file reboot configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_REBOOT_H_
#define	_ACTIONS_SOC_REBOOT_H_

//reboot type, mask is 0xff00, low byte for details
#define REBOOT_TYPE_NORMAL			0x0000
#define REBOOT_TYPE_GOTO_ADFU		0x1000
#define REBOOT_TYPE_GOTO_BTSYS		0x1100
#define REBOOT_TYPE_GOTO_WIFISYS	0x1200
#define REBOOT_TYPE_GOTO_SYSTEM		0x1200
#define REBOOT_TYPE_GOTO_RECOVERY	0x1300


// wakeup enable
#define PMU_WKEN_CTL_SIRQ1_WK_EN    (1 << 12)
#define PMU_WKEN_CTL_SIRQ0_WK_EN    (1 << 11)
#define PMU_WKEN_CTL_REMOTE1_WK_EN  (1 << 10)
#define PMU_WKEN_CTL_REMOTE0_WK_EN  (1 << 9)
#define PMU_WKEN_CTL_BT_WK_EN       (1 << 8)
#define PMU_WKEN_CTL_WIO2_WKEN      (1 << 7)
#define PMU_WKEN_CTL_WIO1_WKEN      (1 << 6)
#define PMU_WKEN_CTL_WIO0_WKEN      (1 << 5)
#define PMU_WKEN_CTL_ALARM_WKEN     (1 << 4)
#define PMU_WKEN_CTL_BAT_WKEN       (1 << 3)
#define PMU_WKEN_CTL_RESET_WKEN     (1 << 2)
#define PMU_WKEN_CTL_SHORT_WKEN     (1 << 1)
#define PMU_WKEN_CTL_LONG_WKEN      (1 << 0)


// wakeup pending
#define PMU_WAKE_PD_SVCC_LOW        (1 << 21)
#define PMU_WAKE_PD_PRESET_PD       (1 << 20)
#define PMU_WAKE_PD_POWEROK_PD      (1 << 19)
#define PMU_WAKE_PD_LB_PD           (1 << 18)
#define PMU_WAKE_PD_OC_PD           (1 << 17)
#define PMU_WAKE_PD_LVPRO_PD        (1 << 16)
#define PMU_WAKE_PD_SIRQ1_WK_PD     (1 << 12)
#define PMU_WAKE_PD_SIRQ0_WK_PD     (1 << 11)
#define PMU_WAKE_PD_REMOTE_PD       (1 << 9)
#define PMU_WAKE_PD_BT_WK_PD        (1 << 8)
#define PMU_WAKE_PD_WIO2_PD         (1 << 7)
#define PMU_WAKE_PD_WIO1_PD         (1 << 6)
#define PMU_WAKE_PD_WIO0_PD         (1 << 5)
#define PMU_WAKE_PD_ALARM_PD        (1 << 4)
#define PMU_WAKE_PD_BATWK_PD        (1 << 3)
#define PMU_WAKE_PD_RESET_PD        (1 << 2)
#define PMU_WAKE_PD_ONOFF_WK_PD     (1 << 1)
#define PMU_WAKE_PD_SHORT_ONOFF_PD  (1 << 0)


#define PMUADC_CTL_SENSORADC_EN     (4)
#define SENSADC_DATA_SENSADC_MASK   (0x3FF<<0)
#define SENSADC_DATA_SENSADC_SHIFT  (0)

#ifndef _ASMLANGUAGE

union sys_pm_wakeup_src {
	u32_t data;
	struct {
		u32_t onoff : 1; /* ONOFF key wakeup */
		u32_t reset : 1; /* RESET key wakeup */
		u32_t bat : 1; /* battery plug in wakeup */
		u32_t alarm : 1; /* RTC alarm wakeup */
		u32_t wio0 : 1; /* WIO0 wakeup */
		u32_t wio1 : 1; /* WIO1 wakeup */
		u32_t wio2 : 1; /* WIO2 wakeup */
		u32_t bt_wk : 1; /* Bluetooth wakeup pending */
		u32_t remote : 1; /* remote wakeup */
		u32_t sirq0 : 1; /* SIRQ0 wakeup */
		u32_t sirq1 : 1; /* SIRQ1 wakeup */
		u32_t watchdog : 1; /* watchdog reboot */
	} t;
};

void sys_pm_poweroff(void);
void sys_pm_poweroff_dc5v(void);
void sys_pm_reboot(int type);
void sys_pm_set_product_flag(int flag);
int sys_pm_get_product_flag(void);

void sys_pm_config_onoff_time(unsigned int value_ms);

bool sys_pm_get_power_5v_status(void);
void sys_pm_set_wakeup_src(uint8_t mode);
int sys_pm_get_wakeup_pending(void);
int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src);
int sys_pm_get_reboot_reason(u16_t *reboot_type, u8_t *reason);
void sys_pm_clear_wakeup_pending(void);
int sys_pm_enter_deep_sleep(void);

void sys_pm_enter_light_sleep(void);
void sys_pm_exit_light_sleep(void);

void sys_pm_temp_adc_ctrl_enable(uint32_t enable);

int sys_pm_temp_adc_get_temperature(void);

u32_t sys_pm_get_rc_timestamp(void);

void sys_pm_update_rtc_regs(void);

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
int sys_pm_get_deep_sleep_time(void);

int sys_pm_get_light_sleep_time(void);

int sys_pm_get_idle_sleep_time(void);

int sys_pm_get_bt_irq_time(void);

#endif

#ifdef CONFIG_PM_CPU_IDLE_LOW_POWER
void soc_pm_cpu_enter_lowpower(void);
void soc_pm_cpu_exit_lowpower(void);
#else
#define soc_pm_cpu_enter_lowpower()			do {} while (0)
#define soc_pm_cpu_exit_lowpower()			do {} while (0)
#endif	/* CONFIG_PM_CPU_IDLE_LOW_POWER */

#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ
#define STANDBY_VALID_INTC_PD0  ((1<<IRQ_ID_RTC) | (1<<IRQ_ID_IRC))
#endif

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_REBOOT_H_	*/
