/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tool_app_inner.h"

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

int tool_att_connect_try(void)
{
#ifdef CONFIG_STUB_DEV_UART

	int ret = 0;
	struct device *stub_dev;
	struct stub_device *stub;

	stub_dev = device_get_binding(CONFIG_STUB_DEV_UART_NAME);
	if (stub_dev != NULL) {
		ret = stub_open(stub_dev, NULL);
		if (ret != 0) {
			SYS_LOG_INF("uart stub open fail %d\n", ret);
		} else {
			SYS_LOG_INF("uart stub ATT");

			stub = (struct stub_device *)mem_malloc(sizeof(struct stub_device));
			if (!stub)
				return -1;

			memset((void *)stub, 0, sizeof(struct stub_device));
			g_tool_data.dev_stub = stub;
			g_tool_data.dev_stub->stub_dev = stub_dev;
			g_tool_data.dev_stub->open_flag = 1;

			g_tool_data.type = STUB_PC_TOOL_ATT_MODE;
			g_tool_data.dev_type = TOOL_DEV_TYPE_UART0;

			tool_init();

			goto stub_connected;
		}
	}

	return -1;

stub_connected:
	return 0;

#else

	return -1;

#endif
}

void tool_att_loop(void)
{
	void *cur_app_name;

	SYS_LOG_INF("Enter");

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

	/* create att front ap */
	app_switch_add_app(APP_ID_ATT);
	app_switch(APP_ID_ATT, APP_SWITCH_CURR, false);
	app_switch_force_lock(1);

	/* create sem & wait att front ap quit */
	while (1) {
		cur_app_name = app_manager_get_current_app();
		if (cur_app_name && (memcmp(APP_ID_ATT, cur_app_name, strlen(cur_app_name)) == 0)) {
			k_sleep(10);
		} else {
			break;
		}
	}

	app_switch_force_unlock(1);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

	SYS_LOG_INF("Exit");
}
