/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_bt_link.c
*/

#include <property_manager.h>
#include "ap_autotest_bt_link.h"

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

btplay_test_arg_t g_btplay_test_arg;

static int act_test_read_bt_link_arg(void)
{
    int ret_val = 0;
	u16_t *line_buffer;
	u16_t *start;
	u16_t *end;
	u8_t arg_num;
	btplay_test_arg_t *btplay_test_arg = &g_btplay_test_arg;

    ret_val = att_write_data(STUB_CMD_ATT_GET_TEST_ARG, 0, self->rw_temp_buffer);

    if (ret_val == 0) {
        ret_val = att_read_data(STUB_CMD_ATT_GET_TEST_ARG, self->arg_len, self->rw_temp_buffer);

        if (ret_val == 0) {
            u8_t temp_data[8];
            //Avoid using STUB_ATT_RW_TEMP_BUFFER, because we need to use this buffer to parse parameters
            ret_val = att_write_data(STUB_CMD_ATT_ACK, 0, temp_data);
        }
    }

	if (ret_val == 0) {
		line_buffer = (u16_t *)(self->rw_temp_buffer + sizeof(stub_ext_cmd_t));

	    arg_num = 1;

        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);

	    //read 6 bytes bt address
    	unicode_to_uint8_bytes(start, end, btplay_test_arg->bt_transmitter_addr, 5, 6);
    
        //read test mode
    	act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
    	btplay_test_arg->bt_test_mode = unicode_to_int(start, end, 16);
	}
    SYS_LOG_INF("transmitter_addr = %s, test_mode = %d\n\n",btplay_test_arg->bt_transmitter_addr,btplay_test_arg->bt_test_mode);
    return ret_val;
}

static bool act_test_report_bt_link_result(int ret_val)
{
    btplay_test_arg_t *btplay_test_arg = &g_btplay_test_arg;
    
    return_data->test_id = self->test_id;
    return_data->test_result = ret_val;

    bytes_to_unicode(btplay_test_arg->bt_transmitter_addr, 5, 6, return_data->return_arg, &trans_bytes);

    //add ','
    return_data->return_arg[trans_bytes++] = 0x002c;
    bytes_to_unicode(&(btplay_test_arg->bt_test_mode), 0, 1, &(return_data->return_arg[trans_bytes]), &trans_bytes);

    //add ','
    return_data->return_arg[trans_bytes++] = 0x002c;
    bytes_to_unicode(&(btplay_test_arg->bt_fast_mode), 0, 1, &(return_data->return_arg[trans_bytes]), &trans_bytes);

    //add endl
    return_data->return_arg[trans_bytes++] = 0x0000;

    //If the parameter is not four byte aligned, four byte alignment is required
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;

    act_test_report_result(return_data, trans_bytes * 2 + 4);

    return ret_val;
}

#define BTTEST_STATUS_NULL              0x0000
#define BTTEST_ACL_CONNECTED            0x0001
#define BTTEST_A2DP_CONNECTED           0x0002
#define BTTEST_A2DP_STARTED             0x0004
#define BTTEST_HFP_CONNECTED            0x0008
#define BTTEST_SCO_CONNECTED            0x0010
#define BT_TEST_ACL_LINKING_TIME        3000
#define BT_TEST_SCO_CALLING_TIME        7000
#define BT_TEST_TIMEOUT                 20000

bool att_bttest_start(void)
{
    return att_cmd_send(STUB_CMD_ATT_BT_START, 3);
}

bool att_bttest_stop(void)
{
    return att_cmd_send(STUB_CMD_ATT_BT_STOP, 3);
}

bool att_bttest_a2dp_start(void)
{
    return att_cmd_send(STUB_CMD_ATT_BT_A2DP, 3);
}

bool att_bttest_sco_start(void)
{
    return att_cmd_send(STUB_CMD_ATT_BT_SCO, 3);
}

u32_t check_bt_status(uint8_t *addr)
{
	struct bt_dev_rdm_state state;
    u32_t ret = BTTEST_STATUS_NULL;

    memcpy(state.addr.val, addr, 6);
    btif_br_get_dev_rdm_state(&state);

    if (state.acl_connected)
        ret |= BTTEST_ACL_CONNECTED;
    if (state.a2dp_connected)
        ret |= BTTEST_A2DP_CONNECTED;
    if (state.a2dp_stream_started)
        ret |= BTTEST_A2DP_STARTED;
    if (state.hfp_connected)
        ret |= BTTEST_HFP_CONNECTED;
    if (state.sco_connected)
        ret |= BTTEST_SCO_CONNECTED;

    return ret;
}

bool bttest_status_judge(uint8_t *addr, u32_t status)
{
    u32_t ret = BTTEST_STATUS_NULL;

    ret = check_bt_status(addr);
    if ((ret & status) == status)
        return true;
    else
        return false;
}

int bt_test_profile_only_link(u8_t *addr, bool *ret_val)
{
    u32_t start_time;
    
    if (bttest_status_judge(addr, BTTEST_ACL_CONNECTED)) {
       	*ret_val = true;

        //to make sure that mac address can be displayed on the transmitter
        start_time = k_uptime_get_32();
        while (k_uptime_get_32() - start_time < BT_TEST_ACL_LINKING_TIME) {
            k_sleep(100);
            if (!(bttest_status_judge(addr, BTTEST_ACL_CONNECTED)))
                break;
        }
       	return 0;
    }
    return -1;
}

int bt_test_profile_sco_play(u8_t *addr, bool *ret_val)
{
    u32_t start_time;
    bool ret;
    
    if (bttest_status_judge(addr, BTTEST_SCO_CONNECTED)) {
        ret = att_btcall_start_play();
        
        if(ret) {
            *ret_val = true;
            start_time = k_uptime_get_32();
            while (k_uptime_get_32() - start_time < BT_TEST_SCO_CALLING_TIME) {
                k_sleep(100);
                if (!(bttest_status_judge(addr, BTTEST_SCO_CONNECTED)))
                    break;
            }
            att_btcall_stop_play();
        }
        else {
            *ret_val = false;
        }
        return 0;
    }
    return -1;
}

int bt_test_profile_a2dp_play(u8_t *addr, bool *ret_val)
{
    //not realized yet
    return -1;
}

int bt_test_profile_both_play(u8_t *addr, bool *ret_val)
{
    // not realized yet
    return -1;
}

bool bt_test_profile_mode(u8_t test_mode, u8_t *addr)
{
    bool ret = false;
    u32_t start_time;
    start_time = k_uptime_get_32();

    while (k_uptime_get_32() - start_time < BT_TEST_TIMEOUT) {
        k_sleep(100);
        
        if (test_mode == 0) {
            if (bt_test_profile_only_link(addr, &ret) == 0)
            	return ret;
        } else if (test_mode == 1) {
            if (bt_test_profile_sco_play(addr, &ret) == 0)
            	return ret;
        } else if (test_mode == 2) {
            if (bt_test_profile_a2dp_play(addr, &ret) == 0)
            	return ret;
        } else if (test_mode == 3) {
            if (bt_test_profile_both_play(addr, &ret) == 0)
            	return ret;
        }
    }

    printk("bt test timeout\n");

    return false;
}

bool test_bt_manager_loop_deal(btplay_test_arg_t *btplay_test_arg)
{
    bool ret_val;
    uint8_t *addr = btplay_test_arg->bt_transmitter_addr;
    uint8_t  test_mode = btplay_test_arg->bt_test_mode;

    ret_val = bt_test_profile_mode(test_mode, addr);
    return ret_val;
}

void act_test_btstack_uninstall(void)
{
    bt_manager_deinit();
    bt_manager_clear_list(BTSRV_DEVICE_ALL);
}

void act_test_btstack_install(btplay_test_arg_t *btplay_test_arg)
{
	struct autoconn_info info[3];
	struct app_msg msg = {0};

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

	memset(info, 0, sizeof(info));
	memcpy(info[0].addr.val, btplay_test_arg->bt_transmitter_addr, sizeof(info[0].addr.val));
	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 1;
	info[0].hfp = 0;
	info[0].avrcp = 1;
	info[0].hfp_first = 0;
	btif_br_set_auto_reconnect_info(info, 3);
	property_flush(NULL);

	bt_manager_startup_reconnect();
}

test_result_e act_test_bt_link_test(btplay_test_arg_t *btplay_test_arg)
{
    s32_t ret_val;

    act_test_btstack_install(btplay_test_arg);

    ret_val = test_bt_manager_loop_deal(btplay_test_arg);

    act_test_btstack_uninstall();

    if(ret_val == false) {
        printk("bt test fail\n");
    }
    return ret_val;
}

bool pattern_main(struct att_env_var *para)
{
    int ret_val = 0;

	self = para;
	return_data = (return_result_t *) (self->return_data_buffer);
	trans_bytes = 0;

	if (self->arg_len != 0) {
		if (act_test_read_bt_link_arg() != 0) {
			SYS_LOG_INF("read bt link arg fail\n");
		}
	}

	ret_val = act_test_bt_link_test(&g_btplay_test_arg);
    
	ret_val = act_test_report_bt_link_result(ret_val);

    SYS_LOG_INF("att test end\n");
    
    return ret_val;
}
