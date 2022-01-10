/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_main.c
*/


#include "ap_autotest.h"
#include "soc_pm.h"
#include "tool_app.h"

struct device * stub_dev;
u8_t *STUB_ATT_RW_TEMP_BUFFER;
u8_t *STUB_ATT_RETURN_DATA_BUFFER;

autotest_test_info_t g_test_info;

atf_head_t g_atf_head;

struct att_env_var g_att_env_var;


/*
system app notify bt engine ready in MSG_BT_ENGINE_READY
*/
int act_att_notify_bt_engine_ready(void)
{
	struct app_msg msg = {0};

	msg.type = MSG_ATT_BT_ENGINE_READY;
	if (false == send_async_msg("att", &msg)) {
		SYS_LOG_WRN("send_async_msg MSG_ATT_BT_ENGINE_READY fail\n");
		return -1;
	}

	return 0;
}

/*
get stub connect status, 0 on connect successful, -1 fail
*/
int get_autotest_connect_status(void)
{
	int ret = 0;

	stub_dev = tool_get_dev_device();
	if (stub_dev) {
		goto stub_connected;
	} else {
#ifdef CONFIG_USB_HOTPLUG
		if (hotplug_manager_get_state(HOTPLUG_USB_DEVICE) == HOTPLUG_IN)
		{
			stub_dev = device_get_binding(CONFIG_STUB_DEV_USB_NAME);
			if (stub_dev != NULL) {
				g_att_env_var.test_mode = TEST_MODE_USB;
				goto stub_connected;
			}
		}
#endif
	}

	ret = -1;

#ifdef CONFIG_STUB_DEV_USB
stub_connected:
#endif

	return ret;
}

static int test_init(void)
{
	int ret_val = 0;

	ret_val = att_write_data(STUB_CMD_ATT_ACK, 0, STUB_ATT_RW_TEMP_BUFFER);
	if (ret_val < 0) {
	}

//	ret_val = read_atf_file((u8_t *)&g_atf_head, sizeof(g_atf_head), 0, sizeof(g_atf_head));
//	if (ret_val < (int) sizeof(g_atf_head)) {
//	}
//
//	SYS_LOG_INF("atf sub files : %d\n", g_atf_head.file_total);

	g_test_info.test_id = 0;

	return ret_val;
}

/*
At the end of the auto test, it is decided to shut down, restart or start directly
*/
static void test_exit(u16_t exit_mode)
{
	volatile bool run_flag = true;

	switch (exit_mode) {
	case ATT_EXIT_REGULAR:
		while (run_flag) {
			os_sleep(100);
		}
		break;

	case ATT_EXIT_SHUTOFF_DIRECTLY:
		os_sleep(1000);		//eliminate the influence of key test
		sys_pm_poweroff_dc5v();
		break;

	case ATT_EXIT_SHUTOFF_PLUG_USB:
		while (run_flag) {
			os_sleep(100);
		#ifdef CONFIG_USB_HOTPLUG
			if (hotplug_manager_get_state(HOTPLUG_USB_DEVICE) == HOTPLUG_OUT)
		#endif
				break;
		}
		sys_pm_poweroff();
		break;

	case ATT_EXIT_SYS_REBOOT_PLUG_USB:
		while (run_flag) {
			os_sleep(100);
		#ifdef CONFIG_USB_HOTPLUG
			if (hotplug_manager_get_state(HOTPLUG_USB_DEVICE) == HOTPLUG_OUT)
		#endif
				break;
		}
		sys_pm_reboot(REBOOT_TYPE_NORMAL);
		break;

	default :
		/* shut down directly */
		os_sleep(1000);
		sys_pm_poweroff_dc5v();
	}
}

void autotest_main(void *p1, void *p2, void *p3)
{
	int tool_dev_type = tool_get_dev_type();

    SYS_LOG_INF("test ap running...");
    if (get_autotest_connect_status() < 0) {
        SYS_LOG_INF("stub is NOT available\n");
    }

    STUB_ATT_RW_TEMP_BUFFER = app_mem_malloc(STUB_ATT_RW_TEMP_BUFFER_LEN + STUB_ATT_RETURN_DATA_LEN);
    if (!STUB_ATT_RW_TEMP_BUFFER)
    {
        SYS_LOG_ERR("autotest malloc fail\n");
        return;
    }
    memset(STUB_ATT_RW_TEMP_BUFFER, 0x00, STUB_ATT_RW_TEMP_BUFFER_LEN + STUB_ATT_RETURN_DATA_LEN);
    STUB_ATT_RETURN_DATA_BUFFER = STUB_ATT_RW_TEMP_BUFFER + STUB_ATT_RW_TEMP_BUFFER_LEN;

	g_att_env_var.rw_temp_buffer = STUB_ATT_RW_TEMP_BUFFER;
	g_att_env_var.return_data_buffer = STUB_ATT_RETURN_DATA_BUFFER;

	g_att_env_var.test_mode = TEST_MODE_USB;
	if (tool_dev_type == TOOL_DEV_TYPE_UART0 || tool_dev_type == TOOL_DEV_TYPE_UART1) {
		g_att_env_var.test_mode = TEST_MODE_UART;
	} else if (tool_dev_type == TOOL_DEV_TYPE_USB) {
		g_att_env_var.test_mode = TEST_MODE_USB;
	}

	/* default exit mode is shut off after USB pluged out */
	g_att_env_var.exit_mode = ATT_EXIT_SHUTOFF_PLUG_USB;

	test_init();

	if (test_dispatch() < 0) {
		g_att_env_var.exit_mode = ATT_EXIT_SHUTOFF_DIRECTLY;
	}

	test_exit(g_att_env_var.exit_mode);

	app_mem_free(STUB_ATT_RW_TEMP_BUFFER);

	app_manager_thread_exit(app_manager_get_current_app());

	return;
}

APP_DEFINE(att, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	   autotest_main, NULL);

