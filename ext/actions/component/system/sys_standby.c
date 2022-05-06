/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system standby
 */

#include <os_common_api.h>
#include <app_manager.h>
#include <msg_manager.h>
#include <power_manager.h>
#include <ui_manager.h>
#include <property_manager.h>
#include <esd_manager.h>
#include <tts_manager.h>
#include <misc/util.h>
#include <string.h>
#include <soc.h>
#include <sys_event.h>
#include <bt_manager.h>
#include <sys_manager.h>
#include <mem_manager.h>
#include <sys_monitor.h>
#include <sys_wakelock.h>
#include <watchdog_hal.h>
#include <board.h>
#include <kernel.h>
#include <power.h>
#include <device.h>
#include <misc/printk.h>
#include <power.h>
#include <input_dev.h>
#include <audio_hal.h>
#include <dvfs.h>
#include <cpuload_stat.h>

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_standby"
#include <logging/sys_log.h>
#endif

#define STANDBY_MIN_TIME_SEC (10)
#define STANDBY_BT_MIN_SLEEP_MSEC (10)

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)

#define STANDBY_VALID_WAKEUP_PD (PMU_WAKE_PD_SHORT_ONOFF_PD | \
				 PMU_WAKE_PD_ONOFF_WK_PD |  \
				 PMU_WAKE_PD_BATWK_PD | \
				 PMU_WAKE_PD_REMOTE_PD | \
				 PMU_WAKE_PD_RESET_PD | PMU_WAKE_PD_WIO1_PD)

#else

#define STANDBY_VALID_WAKEUP_PD (PMU_WAKE_PD_SHORT_WK_PD | \
				 PMU_WAKE_PD_LONG_WK_PD |  \
				 PMU_WAKE_PD_BAT_PD | \
				 PMU_WAKE_PD_ONOFF0_S_PD | \
				 PMU_WAKE_PD_ONOFF0_L_PD | \
				 PMU_WAKE_PD_REMOTE_PD | \
				 PMU_WAKE_PD_RESET_WK_PD | \
				 PMU_WAKE_PD_DC5VON_PD)

#endif

enum STANDBY_STATE_E {
	STANDBY_NORMAL,
	STANDBY_S1,
	STANDBY_S2,
	STANDBY_S3,
};

struct standby_context_t {
	u32_t	auto_standby_time;
	u32_t	auto_powerdown_time;
	u8_t	standby_state;
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	u8_t	dvfs_level;
#endif
	u32_t	bt_ctrl_sleep_time;
	u32_t	bt_ctrl_sleep_timestamp; /* to calculate the elapsed time */
	u8_t	bt_ctrl_sleep_pending : 1;
	u8_t	bt_host_wake_up_pending : 1;
	u32_t   wakeup_timestamp;
};

struct standby_context_t *standby_context = NULL;

extern void thread_usleep(uint32_t usec);
extern int usb_hotplug_suspend(void);
extern int usb_hotplug_resume(void);

void sys_set_bt_ctrl_sleep_pending(u8_t pending, u32_t sleep_time)
{
	u32_t irq_flags = irq_lock();

	standby_context->bt_ctrl_sleep_pending = pending;
	standby_context->bt_ctrl_sleep_time = sleep_time;

	standby_context->bt_ctrl_sleep_timestamp = sys_pm_get_rc_timestamp();

	irq_unlock(irq_flags);
}

void sys_set_bt_host_wake_up_pending(uint8_t pending)
{
	u32_t irq_flags = irq_lock();

	standby_context->bt_host_wake_up_pending = pending;

	irq_unlock(irq_flags);
}

static int sys_get_bt_host_wake_up_pending(void)
{
	return standby_context->bt_host_wake_up_pending;
}

static int _sys_standby_check_auto_powerdown(void)
{
	int ret = 0;

	if (bt_manager_get_connected_dev_num() == 0
	#ifdef CONFIG_TWS
		&& bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE
	#endif
	) {
		if (sys_wakelocks_get_free_time() >= standby_context->auto_powerdown_time) {
			SYS_LOG_INF("POWERDOWN!!!");
			sys_event_notify(SYS_EVENT_POWER_OFF);
			sys_event_send_message(MSG_POWER_OFF);
			ret = 1;
		}
	}

	return ret;
}

static int _sys_standby_enter_s1(void)
{
	struct device *dev = NULL;

	standby_context->standby_state = STANDBY_S1;

	/**disable device for adc */
	/* power_manager_disable_bat_adc(); */

	/**disable key adc */
	dev = device_get_binding(CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME);
	if (dev)
		input_dev_disable(dev);

	/**disable battery adc*/
	dev = device_get_binding("battery");
	if (dev)
		power_supply_disable(dev);

	/**disable usb phy */

	/**disable pa */
#ifdef CONFIG_AUDIO_POWERON_OPEN_PA
	hal_aout_close_pa();
#endif

#ifdef CONFIG_USB
	usb_hotplug_suspend();
#endif

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_sleep();
#endif

#ifdef CONFIG_LED_MANAGER
	//led_manager_sleep();
#endif

	/**set ble state stanyby*/
	SYS_LOG_INF("Enter S1");
	return 0;
}

static int _sys_standby_exit_s1(void)
{
	struct device *dev;

	standby_context->standby_state = STANDBY_NORMAL;
	standby_context->wakeup_timestamp = os_uptime_get_32();

	/**enable seg led */
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_wake_up();
#endif

	/**enable key adc */
	dev = device_get_binding(CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME);
	if (dev)
		input_dev_enable(dev);

	/**enable usb phy */

	/**enable battery adc*/
	dev = device_get_binding("battery");
	if (dev)
		power_supply_enable(dev);

	/**enable pa */
#ifdef CONFIG_AUDIO_POWERON_OPEN_PA
	hal_aout_open_pa();
#endif

#ifdef CONFIG_USB
	usb_hotplug_resume();
#endif

#ifdef CONFIG_LED_MANAGER
	led_manager_wake_up();
#endif

	/**clear ble state stanyby*/
	SYS_LOG_INF("Exit S1");

	return 0;
}


static int _sys_standby_enter_s2(void)
{
	standby_context->standby_state = STANDBY_S2;

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	/** unset dvfs level */
	standby_context->dvfs_level = dvfs_get_current_level();
	dvfs_unset_level(standby_context->dvfs_level, "S2");
	/** set dvfs level to s2*/
	dvfs_set_level(DVFS_LEVEL_S2, "S2");
#endif

	/** setup the wakeup sources under standby */
	sys_pm_set_wakeup_src(1);

	/** clear wakeup flag */
	sys_pm_clear_wakeup_pending();

	/** enter light sleep */
	sys_pm_enter_light_sleep();


	SYS_LOG_INF("Enter S2");
	return 0;
}

static int _sys_standby_exit_s2(void)
{
	standby_context->standby_state = STANDBY_S1;

	sys_pm_exit_light_sleep();

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	/**recover old dvfs level */
	dvfs_unset_level(DVFS_LEVEL_S2, "S2");
	dvfs_set_level(standby_context->dvfs_level, "S2");
#endif
	SYS_LOG_INF("Exit S2");
	return 0;
}

//#define CONFIG_S3BT_LEAST_LOG

static int _sys_standby_enter_s3(void)
{
	int wakeup_src;
	standby_context->standby_state = STANDBY_S3;

#ifdef CONFIG_S3BT_LEAST_LOG
	printk(">>S3BT\n");
#else
	SYS_LOG_INF("Enter S3BT");
#endif

	wakeup_src = sys_pm_enter_deep_sleep();

#ifdef CONFIG_S3BT_LEAST_LOG
	printk("<<0x%x\n", wakeup_src);
#else
	SYS_LOG_INF("Wakeup from S3BT, pending: 0x%x", wakeup_src);
#endif

#ifdef CONFIG_PM_SLEEP_TIME_TRACE
	SYS_LOG_INF("deep sleep time %dms, light sleep time %dms, idle time %d ms", \
			sys_pm_get_deep_sleep_time(), sys_pm_get_light_sleep_time(), sys_pm_get_idle_sleep_time());
#endif

	return wakeup_src;
}

static int _sys_standby_process_normal(void)
{
	u32_t wakelocks = sys_wakelocks_check();

	/**have sys wake lock*/
	if (wakelocks) {
		SYS_LOG_DBG("wakelocks: 0x%08x\n", wakelocks);
		return 0;
	}

	/** check DC5V plug in */
	if (sys_pm_get_power_5v_status()) {
		SYS_LOG_DBG("DC5V plug in and not enter standby");
		return 0;
	}

	if (sys_get_bt_host_wake_up_pending()) {
		sys_set_bt_host_wake_up_pending(0);
		return 0;
	}

#ifdef CONFIG_TWS
	if (!bt_manager_tws_check_is_woodpecker()) {
		SYS_LOG_DBG("281B tws not enter s1");
		return 0;
	}
#endif

	SYS_LOG_DBG("idle time %d", sys_wakelocks_get_free_time());

	if (sys_wakelocks_get_free_time() > standby_context->auto_standby_time)
		_sys_standby_enter_s1();

	return 0;
}

static int _sys_standby_process_s1(void)
{
	u32_t wakelocks = sys_wakelocks_check();

	/**have sys wake lock*/
	if (wakelocks) {
		SYS_LOG_DBG("hold status: 0x%08x\n", wakelocks);
		_sys_standby_exit_s1();
		return 0;
	}

	if (sys_wakelocks_get_free_time() > standby_context->auto_standby_time)
		_sys_standby_enter_s2();
	else
		_sys_standby_exit_s1();

	return 0;
}

#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ
static bool _sys_standby_wakeup_by_intc(void)
{
	u32_t pending;
	pending = (sys_read32(INTC_PD0) & sys_read32(INTC_MSK0));
	if (pending & STANDBY_VALID_INTC_PD0) {
		SYS_LOG_DBG("wakeup from S2, irq: 0x%x", pending);
		return true;
	}

	return false;
}
#endif

static bool _sys_standby_wakeup_from_s2(void)
{
	u32_t wakelocks = sys_wakelocks_check();

	if (_sys_standby_check_auto_powerdown()) {
		return true;
	}

#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ
	if (_sys_standby_wakeup_by_intc()) {
		return true;
	}
#else
	u32_t pending = sys_pm_get_wakeup_pending();
	if (pending & STANDBY_VALID_WAKEUP_PD) {
		sys_pm_clear_wakeup_pending();
		SYS_LOG_DBG("wakeup from S2: 0x%x", pending);
		return true;
	}
#endif

	if (power_manager_get_dc5v_status()) {
		SYS_LOG_DBG("wakeup from S2 because dc5v \n");
		return true;
	}

	if (wakelocks || sys_wakelocks_get_free_time() < standby_context->auto_standby_time) {
		SYS_LOG_DBG("wakelock: 0x%x\n", wakelocks);
		return true;
	}

	if (power_manager_check_is_no_power()) {
		SYS_LOG_INF("NO POWER\n");
		sys_pm_poweroff();
		return true;
	}

	/* FIXME: Here catch a hardware related issue.
	  * There is no wakeup pending of on-off key during s3bt stage,
	  * whereas the pending is happened at s1/s2 stage.
	  */
	if (sys_read32(PMU_ONOFF_KEY) & 0x1) {
		/* wait until ON-OFF key bounce */
		while (sys_read32(PMU_ONOFF_KEY) & 0x1)
			;
		SYS_LOG_INF("wakeup from ON-OFF key");
		return true;
	}

	return false;
}

static int _sys_standby_process_s2(void)
{
	u32_t cur_time;
	unsigned int irq_flags = irq_lock();
	unsigned int wakeup_src;

#ifdef CONFIG_CPU_LOAD_STAT
	cpuload_debug_log_mask_and(~CPULOAD_DEBUG_LOG_THREAD_RUNTIME);
#endif

	while (true) {
		/**clear watchdog */
#ifdef CONFIG_WATCHDOG
		watchdog_clear();
#endif

		/**have sys wake lock*/
		if (_sys_standby_wakeup_from_s2()) {
			break;
		}

		if (standby_context->bt_ctrl_sleep_pending) {
#ifdef CONFIG_PM_SLEEP_TIME_TRACE
			SYS_LOG_INF("BT sleep time %d", standby_context->bt_ctrl_sleep_time);
#endif
			cur_time = sys_pm_get_rc_timestamp();
			/**handle the timer overflow*/
			if ((cur_time - standby_context->bt_ctrl_sleep_timestamp) < 0) {
				if ((standby_context->bt_ctrl_sleep_time + standby_context->bt_ctrl_sleep_timestamp)
					> (cur_time + STANDBY_BT_MIN_SLEEP_MSEC + 0x10000000)) /* rc clock is a 28bits(0~27bits) timer */
					wakeup_src = _sys_standby_enter_s3();
			} else {
				if ((standby_context->bt_ctrl_sleep_time + standby_context->bt_ctrl_sleep_timestamp)
					> (STANDBY_BT_MIN_SLEEP_MSEC + cur_time))
					wakeup_src = _sys_standby_enter_s3();
			}
		}

#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ
		if (_sys_standby_wakeup_by_intc())
			break;
#endif

		irq_unlock(irq_flags);
		/* Here we need to let the bt has a chance to run */
		thread_usleep(300);
		irq_flags = irq_lock();

#ifdef CONFIG_AUTO_STANDBY_S2_LOWFREQ
		if (wakeup_src) {
			SYS_LOG_DBG("wakeup from S2, src: 0x%x", wakeup_src);
			break;
		}
#endif
	}

	_sys_standby_exit_s2();

#ifdef CONFIG_CPU_LOAD_STAT
	cpuload_debug_log_mask_or(CPULOAD_DEBUG_LOG_THREAD_RUNTIME);
#endif

	irq_unlock(irq_flags);
	sys_wake_lock(WAKELOCK_WAKE_UP);
	_sys_standby_exit_s1();
	sys_wake_unlock(WAKELOCK_WAKE_UP);
	return 0;
}

static int _sys_standby_work_handle(void)
{
	int ret = _sys_standby_check_auto_powerdown();

	if (ret)
		return ret;

	switch (standby_context->standby_state) {
	case STANDBY_NORMAL:
		ret = _sys_standby_process_normal();
		break;
	case STANDBY_S1:
		ret = _sys_standby_process_s1();
		break;
	case STANDBY_S2:
		ret = _sys_standby_process_s2();
		break;
	}
	return ret;
}

#ifdef CONFIG_AUTO_POWEDOWN_TIME_SEC
static bool _sys_standby_is_auto_powerdown(void)
{
	bool auto_powerdown = true;
	char temp[16];
	int ret;

	memset(temp, 0, sizeof(temp));
	ret = property_get(CFG_AUTO_POWERDOWN, temp, 16);
	if (ret > 0) {
		if (strcmp(temp, "false") == 0) {
			auto_powerdown = false;
		}
	}

	return auto_powerdown;
}
#endif

static struct standby_context_t global_standby_context;
int sys_standby_init(void)
{
	standby_context = &global_standby_context;

	memset(standby_context, 0, sizeof(struct standby_context_t));

#ifdef CONFIG_AUTO_STANDBY_TIME_SEC
	if (0 == CONFIG_AUTO_STANDBY_TIME_SEC) {
		standby_context->auto_standby_time = OS_FOREVER;
	} else if (CONFIG_AUTO_STANDBY_TIME_SEC < STANDBY_MIN_TIME_SEC) {
		SYS_LOG_WRN("too small, used default");
		standby_context->auto_standby_time = STANDBY_MIN_TIME_SEC * 1000;
	} else {
		standby_context->auto_standby_time = CONFIG_AUTO_STANDBY_TIME_SEC * 1000;
	}
#else
	standby_context->auto_standby_time = OS_FOREVER;
#endif

#ifdef CONFIG_AUTO_POWEDOWN_TIME_SEC
	if (_sys_standby_is_auto_powerdown()) {
		standby_context->auto_powerdown_time = CONFIG_AUTO_POWEDOWN_TIME_SEC * 1000;
	} else {
		SYS_LOG_WRN("Disable auto powerdown\n");
		standby_context->auto_powerdown_time = OS_FOREVER;
	}
#else
	standby_context->auto_powerdown_time = OS_FOREVER;
#endif

	standby_context->standby_state = STANDBY_NORMAL;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wakelocks_init();
#endif

	if (sys_monitor_add_work(_sys_standby_work_handle)) {
		SYS_LOG_ERR("add work failed\n");
		return -EFAULT;
	}

	SYS_LOG_INF("standby time : %d", standby_context->auto_standby_time);

	return 0;
}

u32_t system_wakeup_time(void)
{
	u32_t wakeup_time = OS_FOREVER;

	/** no need deal u32_t overflow */
	if (standby_context->wakeup_timestamp) {
		wakeup_time = os_uptime_get_32() - standby_context->wakeup_timestamp;
	}

	SYS_LOG_INF("wakeup_time %d ms\n", wakeup_time);
	return wakeup_time;
}

u32_t system_boot_time(void)
{
	return k_uptime_get();
}
