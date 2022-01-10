/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system app shell.
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdio.h>
#include <init.h>
#include <string.h>
#include <kernel.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <property_manager.h>

#include "system_app.h"

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#define BTBOX_SHELL "btboxapp"

#ifdef CONFIG_CONSOLE_SHELL
static int shell_input_key_event(int argc, char *argv[])
{
	if (argv[1] != NULL) {
		u32_t key_event;
		key_event = strtoul (argv[1], (char **) NULL, 0);
		sys_event_report_input(key_event);
	}
	return 0;
}

#ifdef CONFIG_PLAYTTS
void keytone_proc(os_timer *timer)
{
#ifdef CONFIG_BT_ENGINE_ACTS_ANDES
	app_message_post_async("main", MSG_VIEW_PAINT, MSG_UI_EVENT, &evt, 4);
#endif
}

static int shell_keytone_test(int argc, char *argv[])
{
	static os_timer timer;

	if (argv[1] != NULL) {
		if (memcmp(argv[1], "start", sizeof("start")) == 0) {
			os_timer_init(&timer, keytone_proc, NULL);
			os_timer_start(&timer, K_MSEC(10),  K_MSEC(strtoul(argv[2], NULL, 0)));
		} else if ((memcmp(argv[1], "stop", sizeof("stop")) == 0)) {
			os_timer_stop(&timer);
		}
	}
	return 0;
}

static int shell_tts_switch(int argc, char *argv[])
{
	if (argv[1] != NULL) {
		if (memcmp(argv[1], "on", sizeof("on")) == 0) {
			tts_manager_lock();
		} else if ((memcmp(argv[1], "off", sizeof("off")) == 0)) {
			tts_manager_unlock();
		}
	}

	return 0;
}
#endif

static int shell_set_config(int argc, char *argv[])
{
	int ret = 0;
#ifdef CONFIG_PROPERTY
	if (argc < 2) {
		ret = property_set(argv[1], argv[1], 0);
	} else {
		ret = property_set(argv[1], argv[2], strlen(argv[2]));
	}

	if (ret < 0) {
		ret = -1;
	} else {
		property_flush(NULL);
	}
#endif

	SYS_LOG_INF("set config %s : %s ok\n", argv[1], argv[2]);
	return 0;
}

static int shell_dump_bt_info(int argc, char *argv[])
{
#ifdef CONFIG_BT_MANAGER
	bt_manager_dump_info();
#endif
	return 0;
}
#ifdef CONFIG_RECORD_APP
extern void recorder_sample_rate_set(u8_t sample_rate_kh);
static int shell_set_record_sample_rate(int argc, char *argv[])
{
	if (argv[1] != NULL) {
		recorder_sample_rate_set(atoi(argv[1]));
	}
	return 0;
}
#endif


static const struct shell_cmd btboxapp_commands[] = {
	{ "set_config", shell_set_config, "set system config " },
	{ "input", shell_input_key_event, "input key event" },
	{ "btinfo", shell_dump_bt_info, "dump bt info" },

#ifdef CONFIG_PLAYTTS
	{ "tts", shell_tts_switch, "tts on/off" },
	{ "keytone", shell_keytone_test, "keytone start or stop" },
#endif
#ifdef CONFIG_RECORD_APP
	{ "set_record_sample_rate_kh", shell_set_record_sample_rate, "set record sample rate" },
#endif
	{ NULL, NULL, NULL }
};
#endif

SHELL_REGISTER(BTBOX_SHELL, btboxapp_commands);


