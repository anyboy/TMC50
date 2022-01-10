/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief charger function.
*/
#ifndef _CHARGER_APP_H
#define _CHARGER_APP_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "charger"


#include <logging/sys_log.h>
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <global_mem.h>
#include <stream.h>
#include <file_stream.h>

#include <dvfs.h>
#include <thread_timer.h>
#include "btservice_api.h"

#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#include "app_switch.h"

#ifdef CONFIG_FILE_SYSTEM 
#include <fs.h>
#include <fs_manager.h>
#endif

enum
{
	MSG_BOOT_TO_SYSTEM = MSG_APP_MESSAGE_START,
};

void charger_view_init(void);
void charger_view_deinit(void);

#endif  // _RECORDER_APP_H

