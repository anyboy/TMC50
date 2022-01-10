/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system manager interface
 */

#ifndef _SYS_MANAGER_H
#define _SYS_MANAGER_H
#include <os_common_api.h>
#include <thread_timer.h>
/**
 * @defgroup sys_manager_apis App system Manager APIs
 * @ingroup system_apis
 * @{
 */

/**
 * @brief mark system ready
 *
 * @details This routine mark system ready 
 * when system base service init finished, system app call this routine 
 * mark system ready.
 *
 * @return N/A
 */

void system_ready(void);

/**
 * @brief check system is ready
 *
 * @details  This routine check system is ready .
 *
 * @return false system is not ready
 * @return true  system is ready
 */

bool system_is_ready(void);

/**
 * @brief system init
 *
 * @details this rontine make system init . 
 * init system core manager and core service ,such as app_manager 
 * srv manager and bt service .
 *
 * @param message message id want to send
 *
 * @return N/A
 */

void system_init(void);

/**
 * @brief system deinit
 *
 * @details this rontine make system deinit .
 * deinit system core manager and core service , system not ready 
 * after call this functions.
 *
 * @return N/A
 */

void system_deinit(void);

/**
 * @brief restore factory config
 *
 * @details this rontine clear user property config 
 * and used factory default config
 *
 * @return 0 success
 * @return others failed
 */

int system_restore_factory_config(void);

/**
 * @brief sys power off
 *
 * @details This routine make system power off . 
 *  first lock key and tts , exit all  foreground app and background service
 *  stop system monitor and make system power down
 *
 * @return N/A
 */

void system_power_off(void);
/**
 * @brief sys power reboot
 *
 * @details This routine make system power reboot . 
 *  first lock key and tts , exit all  foreground app and background service
 *  stop system monitor and make system power reboot
 *
 * @return N/A
 */

/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief return the duration after wakeup
 *
 * @details This routine get the duration atfer wake up
 *
 * @return time ms
 */
u32_t system_wakeup_time(void);
/**
 * @brief return the duration after boot
 *
 * @details This routine get the duration atfer boot
 *
 * @return  time ms
 */
u32_t system_boot_time(void);

/**
 * @brief get reboot type and reason
 *
 * @details This routine get system reboot reason
 * @param reboot_type reboot type
 * @param reason reboot reason 
 *
 * @return N/A
 */

/** reboot type */
enum{
	REBOOT_TYPE_WATCHDOG = 0x01,
	REBOOT_TYPE_HW_RESET = 0x02,
	REBOOT_TYPE_ONOFF_RESET = 0x03,
	REBOOT_TYPE_SF_RESET = 0x04,
	REBOOT_TYPE_ALARM = 0x05,
};

/** reboot reason */
enum{
	REBOOT_REASON_NORMAL = 0x01,
	REBOOT_REASON_OTA_FINISHED = 0x02,
	REBOOT_REASON_FACTORY_RESTORE = 0x03,
	REBOOT_REASON_SYSTEM_EXCEPTION = 0x04,
	REBOOT_REASON_PRODUCT_FINISHED = 0x05,
	REBOOT_REASON_GOTO_BQB = 0x06, /* FCC -> BQB */
	REBOOT_REASON_GOTO_BQB_ATT = 0x07, /* ATT -> BQB */
	REBOOT_REASON_GOTO_PROD_CARD_ATT = 0x08, /* ATT -> CARD PRODUCT */
};

void system_power_get_reboot_reason(u16_t *reboot_type, u8_t *reason);

/**
 * INTERNAL_HIDDEN @endcond
 */
void system_power_reboot(int reason);
/**
 * @} end defgroup sys_manager_apis
 */




#endif


