/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_bqb.c
*/

#include "att_pattern_header.h"
#include "soc_watchdog.h"
#include "soc_pm.h"
#include "sys_manager.h"
#include <cpuload_stat.h>

#define CONFIG_DUTTEST_DIRECTLY

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

static bool act_test_report_bqb_result(bool ret_val)
{
    return_data->test_id = self->test_id;

    return_data->test_result = ret_val;

    //add endl
    return_data->return_arg[trans_bytes++] = 0x0000;

    //If the parameter is not four byte aligned, four byte alignment is required
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;

    return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

bool act_test_enter_BQB_mode(void)
{
    bool ret_val = false;
    int ret;
    u8_t *cmd_data;
    u16_t test_id;

#ifdef CONFIG_DUTTEST_DIRECTLY
	struct app_msg msg = {0};

    SYS_LOG_INF("prepare BQB test\n");

#ifdef CONFIG_BT_MANAGER
	if (bt_manager_is_inited() == false) {
		bt_manager_init();

		while (1) {
			if (receive_msg(&msg, 10)) {
				if (msg.type == MSG_ATT_BT_ENGINE_READY) {
					SYS_LOG_INF("MSG_ATT_BT_ENGINE_READY\n");
					break;
				}
			}
		}
	}
#endif
#endif

	if (act_test_report_bqb_result(1) == false) {
		return false;
	}
    
    cmd_data = self->rw_temp_buffer;
    while (1) {
        //The timeout is in seconds, depending on the time used by the longest test item
        cmd_data[6] = 20;  // Test item normal work timeout
        cmd_data[7] = 0;
        cmd_data[8] = 0;
        cmd_data[9] = 0;

        ret = att_write_data(STUB_CMD_ATT_GET_TEST_ID, 4, cmd_data);

        memset(cmd_data, 0x00, 20);
        
        if (ret == 0) {
            ret = att_read_data(STUB_CMD_ATT_GET_TEST_ID, 4, cmd_data);
            if (ret == 0) {
                test_id = (cmd_data[6] | (cmd_data[7] << 8));

                if (test_id == TESTID_ID_WAIT) {
                    att_write_data(STUB_CMD_ATT_ACK, 0, cmd_data);
                    k_busy_wait(50000);
                    continue;
                }

                if (test_id == TESTID_ID_QUIT) {
                    ret_val = 0;
                    break;
                }

                att_write_data(STUB_CMD_ATT_ACK, 0, cmd_data);
                SYS_LOG_INF("bqb test error : id 0x%x\n", test_id);
                break;
            }            
        }
    }  

#ifdef CONFIG_DUTTEST_DIRECTLY
	k_thread_priority_set(k_current_get(), 0);

	/* disable watchdog */
	soc_watchdog_stop();

#ifdef CONFIG_BT_CONTROLER_BQB
	extern void BT_DutTest(int bqb_mode);
	BT_DutTest(1); /*1-classic bqb, 2-ble bqb*/
#endif

#else
	os_sleep(10);
	sys_pm_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_GOTO_BQB_ATT);
#endif

    return ret_val;
}

bool pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

#ifdef CONFIG_CPU_LOAD_STAT
    cpuload_debug_log_mask_and(~CPULOAD_DEBUG_LOG_THREAD_RUNTIME);
#endif

	self = para;
	return_data = (return_result_t *) (self->return_data_buffer);
	trans_bytes = 0;

	ret_val = act_test_enter_BQB_mode();

    SYS_LOG_INF("att test end : %d\n", ret_val);

#ifdef CONFIG_CPU_LOAD_STAT
    cpuload_debug_log_mask_or(CPULOAD_DEBUG_LOG_THREAD_RUNTIME);
#endif

    return ret_val;
}
