/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx.c
*/

#include "ap_autotest_rf_tx.h"
#include <cpuload_stat.h>

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

pwr_test_arg_t g_pwr_test_arg;

u32_t test_channel_num;

static int act_test_read_rf_tx_arg(void)
{
    int ret_val = 0;
	u16_t *line_buffer;
	u16_t *start;
	u16_t *end;
	int arg_num;

    ret_val = att_write_data(STUB_CMD_ATT_GET_TEST_ARG, 0, self->rw_temp_buffer);

    if (ret_val == 0) {
        ret_val = att_read_data(STUB_CMD_ATT_GET_TEST_ARG, self->arg_len, self->rw_temp_buffer);

        if (ret_val == 0) {
            u8_t temp_data[8];
            //Avoid using STUB_ATT_RW_TEMP_BUFFER£¬because we need to use this buffer to parse parameters
            ret_val = att_write_data(STUB_CMD_ATT_ACK, 0, temp_data);
        }
    }

	if (ret_val == 0) {
		line_buffer = (u16_t *)(self->rw_temp_buffer + sizeof(stub_ext_cmd_t));
		
		if (self->test_id == TESTID_PWR_TEST) {
		    pwr_test_arg_t *pwr_test_arg = &g_pwr_test_arg;

		    arg_num = 1;
		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    pwr_test_arg->pwr_channel[0] = (u8_t)unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    pwr_test_arg->pwr_channel[1] = (u8_t)unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    pwr_test_arg->pwr_channel[2] = (u8_t)unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    pwr_test_arg->pwr_thr_low = (s8_t)unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    pwr_test_arg->pwr_thr_high = (s8_t)unicode_to_int(start, end, 10);
		}
	}

    return ret_val;
}

static bool act_test_report_rf_tx_result(bool ret_val)
{
	pwr_test_arg_t *pwr_test_arg = &g_pwr_test_arg;

    return_data->test_id = self->test_id;

    return_data->test_result = ret_val;


    /** 0x002c is char , */
    int32_to_unicode(pwr_test_arg->pwr_channel[0], &(return_data->return_arg[trans_bytes]), &trans_bytes, 10); 
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(pwr_test_arg->pwr_channel[1], &(return_data->return_arg[trans_bytes]), &trans_bytes, 10); 
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(pwr_test_arg->pwr_channel[2], &(return_data->return_arg[trans_bytes]), &trans_bytes, 10); 
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(pwr_test_arg->pwr_thr_low, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);  
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(pwr_test_arg->pwr_thr_high, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10); 
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(pwr_test_arg->rssi_val, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10); 


    //add endl
    return_data->return_arg[trans_bytes++] = 0x0000;

    //If the parameter is not four byte aligned, four byte alignment is required
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;

    return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

bool att_bttool_pwr_tx_begin(u32_t channel)
{
    u32_t ret_val;

    bttools_mp_param_t tx_param;

    memset(&tx_param, 0, sizeof(tx_param));
    
    tx_param.mp_type = MP_ICTYPE;
    tx_param.sdk_type = 0;
    tx_param.packet_type = PKTTYPE_SET;
    tx_param.payload_len = PAYLOAD_LEN;
    tx_param.payload_type = PAYLOADTYPE_SET;
    tx_param.rf_channel = channel;
    tx_param.rf_power = RF_POWER;
    tx_param.timeout = 0;

    memcpy((u8_t *) (STUB_ATT_RW_TEMP_BUFFER + sizeof(stub_ext_cmd_t)), &tx_param, sizeof(tx_param));

    mp_task_tx_start(channel, 18, 1000); /*FIXME*/

    ret_val = att_write_data(STUB_CMD_ATT_PWR_TX_BEGIN, sizeof(bttools_mp_param_t), STUB_ATT_RW_TEMP_BUFFER);
    
    if (ret_val == 0) {
        ret_val = att_read_data(STUB_CMD_ATT_ACK, 0, STUB_ATT_RW_TEMP_BUFFER);
        if (ret_val != 0) {
            printk("pwr tx begain ack fail\n");
            return false;
        }
    } else {
        printk("pwr tx begain fail\n");
        return false;
    }
    
    return true;
}

bool att_bttool_pwr_tx_stop(void)
{
    bool ret_val;
    
    ret_val = att_cmd_send(STUB_CMD_ATT_PWR_TX_STOP, 3);
    
    return ret_val;
}

bool att_bttool_pwr_read(s16_t *rssi_value)
{
    u32_t ret_val;
    u32_t read_len;
    s16_t rssi_temp_buffer[16]; //Up to 30 groups of records with 8 bytes in each group
    u8_t* pdata;
    
    ret_val = att_write_data(STUB_CMD_ATT_MP_REPORT, 0, STUB_ATT_RW_TEMP_BUFFER);

    if (ret_val == 0) {
        read_len = 16 * 2;
        
        ret_val = att_read_data(STUB_CMD_ATT_MP_REPORT, read_len, STUB_ATT_RW_TEMP_BUFFER);
        if (ret_val == 0) {
            pdata = (u8_t *)STUB_ATT_RW_TEMP_BUFFER;
            memcpy(rssi_temp_buffer, &pdata[6], read_len);
            print_buffer(&pdata[6], 1, read_len, 16, -1);
            ret_val = rf_tx_rssi_calc(rssi_temp_buffer, 
                        sizeof(rssi_temp_buffer) / sizeof(rssi_temp_buffer[0]),  rssi_value);
        } else {
            printk("att pwr report read fail\n");
            ret_val = false;
        }
    } else {
        printk("att pwr report write fail\n");
        ret_val = false;
    }

    return ret_val;
}


bool rf_rx_pwr_test_channel(pwr_test_arg_t *pwr_arg, u8_t channel, s16_t *rssi_val)
{
    int ret_val;
    s32_t retry_cnt = 0;   
    u32_t timestamp;

retry:
    /** Start up tx packet
    */
    ret_val = att_bttool_pwr_tx_begin(channel);
    if (ret_val == false) {
        printk("pwr tx start fail !");
        goto start_fail;
    }

    timestamp = k_uptime_get_32();

    k_sleep(700);
    
    while (k_uptime_get_32() - timestamp < 10000) {
        ret_val = att_bttool_pwr_read(rssi_val);
        if (ret_val == true)
            break;
        k_sleep(400);
    }

    if (ret_val == true) {
        //Judge whether the signal power meets the requirements
        if (*rssi_val < pwr_arg->pwr_thr_low || *rssi_val > pwr_arg->pwr_thr_high) {
            printk("rssi value out of range,current rssi value:%d\n", *rssi_val);
            ret_val = false;
        }
    }

    att_bttool_pwr_tx_stop();
    
start_fail:    

    retry_cnt++;

    if (retry_cnt < 3 && ret_val == false)
        goto retry;
    
    return ret_val;
}

//Whether the tx power meets the requirements is judged by the signal power at the end of the dut received by the transmitter
bool act_test_pwr_test(pwr_test_arg_t *pwr_arg)
{
    bool ret_val;   
    int i;
    u32_t cap_value;
    s16_t rssi_val[3] = {0};

    printk("enter pwr_test !\n");

    cap_value = att_read_trim_cap(RW_TRIM_CAP_SNOR);

    extern void soc_set_hosc_cap(int cap);
	soc_set_hosc_cap(cap_value);

    printk("cap : %d\n",cap_value);

    for (i = 0; i < 3; i++) {
        if (pwr_arg->pwr_channel[i] == 0xff)
            continue;

        ret_val = rf_rx_pwr_test_channel(pwr_arg, pwr_arg->pwr_channel[i], &rssi_val[i]);
        if (ret_val != true) {
            printk("channel:%d, pwr return fail\n",pwr_arg->pwr_channel[i]);
            break;
        }     
    }

    if (ret_val == true)
        pwr_arg->rssi_val = (rssi_val[0] + rssi_val[1] + rssi_val[2]) / test_channel_num;

    return ret_val;
}

bool pattern_main(struct att_env_var *para)
{
    bool ret_val = false;
    int i;

#ifdef CONFIG_CPU_LOAD_STAT
    cpuload_debug_log_mask_and(~CPULOAD_DEBUG_LOG_THREAD_RUNTIME);
#endif

	self = para;
	return_data = (return_result_t *) (self->return_data_buffer);
	trans_bytes = 0;

	if (self->arg_len != 0) {
		if (act_test_read_rf_tx_arg() != 0) {
			SYS_LOG_INF("read rf tx arg fail\n");
		}
	}

    test_channel_num = 0;
    for (i = 0; i < 3; i++) {
        if (g_pwr_test_arg.pwr_channel[i] != 0xff) {
            test_channel_num++;
        }
    }

    if (test_channel_num == 0) {
        printk("mp channel num is zero\n");
        goto test_end_0;
    }

	mp_core_load();

	if (self->test_id == TESTID_PWR_TEST) {
		ret_val = act_test_pwr_test(&g_pwr_test_arg);
	}
	ret_val = act_test_report_rf_tx_result(ret_val);

test_end_0:
    SYS_LOG_INF("att test end\n");

#ifdef CONFIG_CPU_LOAD_STAT
    cpuload_debug_log_mask_or(CPULOAD_DEBUG_LOG_THREAD_RUNTIME);
#endif

    return ret_val;
}
