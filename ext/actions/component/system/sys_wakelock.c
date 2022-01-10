/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system wakelock
 */

#include <os_common_api.h>
#include <app_manager.h>
#include <msg_manager.h>
#include <power_manager.h>
#include <property_manager.h>
#include <esd_manager.h>
#include <tts_manager.h>
#include <misc/util.h>
#include <string.h>
#include <soc.h>
#include <sys_event.h>
#include <bt_manager.h>
#include <sys_manager.h>
#include <sys_monitor.h>
#include <sys_wakelock.h>
#include <board.h>

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_wakelock"
#include <logging/sys_log.h>
#endif

static u32_t wakelocks_bitmaps;
static u32_t wakelocks_free_timestamp;

static int _sys_wakelock_lookup(int wakelock_holder)
{
	if (wakelocks_bitmaps & (1 << wakelock_holder)) {
		return wakelock_holder;
	} else {
		return 0;
	}
}

int sys_wake_lock(int wakelock_holder)
{
	int res = 0;
	int wakelock = 0;
	SYS_IRQ_FLAGS flags;

	sys_irq_lock(&flags);

	wakelock = _sys_wakelock_lookup(wakelock_holder);
	if (wakelock) {
		res = -EEXIST;
		goto exit;
	}

	wakelocks_bitmaps |= (1 << wakelock_holder);
	wakelocks_free_timestamp = 0;

exit:
	sys_irq_unlock(&flags);
	return res;
}

int sys_wake_unlock(int wakelock_holder)
{
	int res = 0;
	int wakelock = 0;
	SYS_IRQ_FLAGS flags;

	sys_irq_lock(&flags);

	wakelock = _sys_wakelock_lookup(wakelock_holder);
	if (!wakelock) {
		res = -ESRCH;
		goto exit;
	}

	wakelocks_bitmaps &= (~(1 << wakelock_holder));

	if (!wakelocks_bitmaps)
		wakelocks_free_timestamp = os_uptime_get_32();

exit:
	sys_irq_unlock(&flags);
	return res;
}

int sys_wakelocks_init(void)
{
	wakelocks_free_timestamp = os_uptime_get_32();
	return 0;
}


int sys_wakelocks_check(void)
{
	return wakelocks_bitmaps;
}

u32_t sys_wakelocks_get_free_time(void)
{
	if (wakelocks_bitmaps || !wakelocks_free_timestamp)
		return 0;

	return os_uptime_get_32() - wakelocks_free_timestamp;
}

