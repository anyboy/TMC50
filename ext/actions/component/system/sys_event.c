/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system event notify
 */
#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_event"
#include <logging/sys_log.h>
#endif

#include <os_common_api.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <sys_event.h>
#include <ui_manager.h>
#include <btservice_api.h>
#include <bt_manager.h>

static const struct sys_event_ui_map *sys_event_map = NULL;
static int sys_event_map_size = 0;

void sys_event_send_message(u32_t message)
{
	struct app_msg  msg = {0};

#ifdef CONFIG_TWS
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
		bt_manager_tws_send_event(TWS_SYSTEM_EVENT, message);
	}
#endif

	msg.type = message;
	send_async_msg("main", &msg);
}

void sys_event_report_input(u32_t key_event)
{
	struct app_msg  msg = {0};

	msg.type = MSG_KEY_INPUT;
	msg.value = key_event;
	send_async_msg("main", &msg);
}

void sys_event_report_srinput(void *sr_event)
{
	struct app_msg  msg = {0};

	msg.type = MSG_SR_INPUT;
	msg.ptr = sr_event;
	send_async_msg("main", &msg);
}

void sys_event_notify(u32_t event)
{
#ifdef CONFIG_TWS
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER &&
		event != SYS_EVENT_TWS_CONNECTED &&
		event != SYS_EVENT_TWS_DISCONNECTED) {
		/* Tws master/slave all will receive TWS_CONNECTED/TWS_DISCONNECTED event */
		bt_manager_tws_send_event_sync(TWS_UI_EVENT, event);
		return;
	}
#endif
	struct app_msg  msg = {0};

	msg.type = MSG_SYS_EVENT;
	msg.cmd = event;

	send_async_msg("main", &msg);
}

void sys_event_map_register(const struct sys_event_ui_map *event_map, int size, int sys_view_id)
{
	if (!sys_event_map) {
		sys_event_map = event_map;
		sys_event_map_size = size;
	} else {
		SYS_LOG_ERR("failed\n");
	}
}

void sys_event_process(u32_t event)
{
	int ui_event = 0;

	if (!sys_event_map)
		return;

	for (int i = 0; i < sys_event_map_size; i++) {
		if (sys_event_map[i].sys_event == event) {
			ui_event = sys_event_map[i].ui_event;
			break;
		}
	}

	if (ui_event != 0) {
	#ifdef CONFIG_UI_MANAGER
		SYS_LOG_INF("ui_event %d\n", ui_event);
		ui_message_send_async(0, MSG_VIEW_PAINT, ui_event);
	#endif
	}
}

