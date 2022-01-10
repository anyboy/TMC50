/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app
 */

#ifndef _SYSTEM_APP_H_
#define _SYSTEM_APP_H_
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <power_supply.h>
#include <srv_manager.h>
#include <app_manager.h>
#include <hotplug_manager.h>
#include <power_manager.h>
#include <input_manager.h>
#include <property_manager.h>
#include "app_defines.h"
#include <sys_manager.h>
#include <ui_manager.h>
#include "app_ui.h"
#include "app_switch.h"
#include <bt_manager.h>
#include "app_defines.h"
#include "system_util.h"

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "system_app"
#ifdef SYS_LOG_LEVEL
#undef SYS_LOG_LEVEL
#endif

#include "fcc_drv_extern.h"

enum{
	SYS_INIT_NORMAL_MODE,
	SYS_INIT_ATT_TEST_MODE,
	SYS_INIT_ALARM_MODE,
};

/** system app init*/
void system_app_init(void);

/** system app input handle*/
void system_key_event_handle(struct app_msg *msg);
void system_input_handle_init(void);
void system_input_event_handle(uint32_t key_event);
void system_sr_input_event_handle(void *value);
int sys_launch_app_init(uint8_t mode);
int system_app_launch(u8_t mode);
int system_app_launch_init(void);

extern void system_app_view_init(void);
extern void system_app_view_exit(void);
extern int system_app_ui_event(int event);
void system_hotplug_event_handle(struct app_msg *msg);
void system_audio_policy_init(void);
void system_tts_policy_init(void);
int system_event_map_init(void);
void system_led_policy_init(void);
void system_app_volume_show(struct app_msg *msg);

void system_app_uart_init(void);
void system_app_uart_tx(u8_t *buf, size_t len);

#endif
