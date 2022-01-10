/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx.c
*/

#include "ap_autotest_rf_rx.h"
#include <cpuload_stat.h>

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

mp_test_arg_t g_mp_test_arg;

struct mp_test_stru g_mp_test;

cfo_return_arg_t g_cfo_return_arg;

u32_t test_channel_num;

static int act_test_read_rf_rx_arg(void)
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
            //Avoid using STUB_ATT_RW_TEMP_BUFFER, because we need to use this buffer to parse parameters
            ret_val = att_write_data(STUB_CMD_ATT_ACK, 0, temp_data);
        }
    }

	if (ret_val == 0) {
		line_buffer = (u16_t *)(self->rw_temp_buffer + sizeof(stub_ext_cmd_t));
		
		if (self->test_id == TESTID_MP_TEST) {
		    mp_test_arg_t *mp_arg = &g_mp_test_arg;

		    arg_num = 1;

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_channel[0] = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_channel[1] = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_channel[2] = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_test = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->ref_cap_index = (u16_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cap_index_low = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cap_index_high = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cap_index_changed = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_threshold_low = (s8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_threshold_high = (s8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_write_mode = (u8_t) unicode_to_int(start, end, 10);

		    //act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    //mp_arg->cfo_upt_offset = (s32_t) unicode_to_int(start, end, 10);



		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->ber_test = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->ber_threshold_low = (u32_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->ber_threshold_high = (u32_t) unicode_to_int(start, end, 10);



		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->rssi_test = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->rssi_threshold_low = (s16_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->rssi_threshold_high = (s16_t) unicode_to_int(start, end, 10);

		} else /*TESTID_MP_READ_TEST*/ {
			mp_test_arg_t *mp_arg = &g_mp_test_arg;

		    arg_num = 1;

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_channel[0] = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_channel[1] = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_channel[2] = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_test = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_threshold_low = (s8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->cfo_threshold_high = (s8_t) unicode_to_int(start, end, 10);



		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->ber_test = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->ber_threshold_low = (u32_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->ber_threshold_high = (u32_t) unicode_to_int(start, end, 10);



		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->rssi_test = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->rssi_threshold_low = (s16_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    mp_arg->rssi_threshold_high = (s16_t) unicode_to_int(start, end, 10);
		}
	}

    return ret_val;
}

static bool act_test_report_rf_rx_result(bool ret_val)
{
	mp_test_arg_t *mp_arg = &g_mp_test_arg;
	cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg;

    return_data->test_id = self->test_id;

    return_data->test_result = ret_val;


    cfo_return_arg->cap_index_low = mp_arg->cap_index_low;
    cfo_return_arg->cap_index_high = mp_arg->cap_index_high;

    int32_to_unicode(mp_arg->cfo_channel[0], &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->cfo_channel[1], &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->cfo_channel[2], &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->cfo_test, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    //the parameter need be modified
    return_data->return_arg[trans_bytes++] = 0x002c;

    if (self->test_id == TESTID_MP_TEST)
    {
        int32_to_unicode(mp_arg->ref_cap_index, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
        return_data->return_arg[trans_bytes++] = 0x002c;

        int32_to_unicode(cfo_return_arg->cap_index_low, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
        return_data->return_arg[trans_bytes++] = 0x002c;

        int32_to_unicode(cfo_return_arg->cap_index_high, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
        return_data->return_arg[trans_bytes++] = 0x002c;

        int32_to_unicode(mp_arg->cap_index_changed, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
        return_data->return_arg[trans_bytes++] = 0x002c;
    }

    int32_to_unicode(mp_arg->cfo_threshold_low, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->cfo_threshold_high, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(g_cfo_return_arg.cfo_val, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    if (self->test_id == TESTID_MP_TEST)
    {
        return_data->return_arg[trans_bytes++] = 0x002c;
		int32_to_unicode(mp_arg->cfo_write_mode, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
    }

    return_data->return_arg[trans_bytes++] = 0x003b; /* 0x3b is character ; */

    int32_to_unicode(mp_arg->ber_test, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->ber_threshold_low, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->ber_threshold_high, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(g_cfo_return_arg.ber_val, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x003b; /* 0x3b is character ; */

    int32_to_unicode(mp_arg->rssi_test, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->rssi_threshold_low, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(mp_arg->rssi_threshold_high, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(g_cfo_return_arg.rssi_val, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    //add endl
    return_data->return_arg[trans_bytes++] = 0x0000;

    //If the parameter is not four byte aligned, four byte alignment is required
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;

    return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

bool att_bttool_tx_begin(u32_t channel)
{
#if 0
    int ret_val;

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

    memcpy(self->rw_temp_buffer + sizeof(stub_ext_cmd_t), &tx_param, sizeof(tx_param));

    ret_val = att_write_data(STUB_CMD_ATT_CFO_TX_BEGIN, sizeof(bttools_mp_param_t), self->rw_temp_buffer);
    if (ret_val == 0) {
        ret_val = att_read_data(STUB_CMD_ATT_ACK, 0, self->rw_temp_buffer);
        if (ret_val < 0) {
            printk("att bttool tx begin fail\n");
            return false;
        }
    } else {
        printk("att bttool tx begin fail\n");
        return false;
    }
#endif
    return true;
}

bool att_bttool_tx_stop(void)
{
#if 0
    return att_cmd_send(STUB_CMD_ATT_CFO_TX_STOP, 2);
#endif
    return true;
}


static bool att_ber_rssi_test(mp_test_arg_t *mp_arg, s16_t cap_value)
{
    cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg;
    s16_t rssi_val[3] = {0};
    u32_t ber_val[3] = {0};
    bool ret = false;
    int ret_val, i;

    for(i = 0; i < 3; i++) {
        if (mp_arg->cfo_channel[i] == 0xff)
           	continue;

        printk("test channel : %d\n", mp_arg->cfo_channel[i]);
        ret = att_bttool_tx_begin(mp_arg->cfo_channel[i]);
        if (ret == false)
            goto test_end;

        ret_val = rf_rx_ber_test_channel(mp_arg, mp_arg->cfo_channel[i], cap_value, &rssi_val[i], &ber_val[i]);
        if (ret_val < 0) {
            ret = false;
            printk("ber or rssi test fail, channel[%d]:%d\n", i, mp_arg->cfo_channel[i]);
            goto test_end;
        }

        ret = att_bttool_tx_stop();
        if (ret == false) {
            printk("bttool stop fail\n");
            goto test_end;
        }

        //ber(bit error rate) the smaller , the sensitivity higher
        if (mp_arg->ber_test && (ber_val[i] > mp_arg->ber_threshold_high
                    /*|| ber_val[i] < mp_arg->ber_threshold_low*/)) {
            printk("ber test fail ber_val:%d\n", ber_val[i]);
            ret = false;
            goto test_end;
        }

        if (mp_arg->rssi_test && (rssi_val[i] > mp_arg->rssi_threshold_high
                    || rssi_val[i] < mp_arg->rssi_threshold_low)) {
            printk("rssi test fail rssi_val:%d\n", rssi_val[i]);
            ret = false;
            goto test_end;
        }
    }

    if (ret == true) {
        cfo_return_arg->rssi_val = (rssi_val[0] + rssi_val[1] + rssi_val[2]) / (test_channel_num);
        cfo_return_arg->ber_val = (ber_val[0] + ber_val[1] + ber_val[2]) / (test_channel_num);
        printk("mp read rssi value:%d, ber:%d\n", cfo_return_arg->rssi_val, cfo_return_arg->ber_val);
    }

	test_end:
	return ret;
}


static bool att_cfo_adjust_test(mp_test_arg_t *mp_arg)
{
    cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg;
    s16_t cap_index[3] = {0};
    s16_t cfo_value[3] = {0};
    bool ret = false;
    int ret_val, i;
    u8_t ret_cap_index;

    for(i = 0; i < 3; i++) {
        if (mp_arg->cfo_channel[i] == 0xff)
            continue;

        ret = att_bttool_tx_begin(mp_arg->cfo_channel[i]);
        if (ret == false)
            goto test_end;

        ret_val = rf_rx_cfo_test_channel(mp_arg, mp_arg->cfo_channel[i], mp_arg->ref_cap_index, &cap_index[i], &cfo_value[i]);
        if (ret_val < 0) {
            ret = false;
            goto test_end;
        }

        ret = att_bttool_tx_stop();
        if (ret == false)
            goto test_end;
    }

    if (ret == true) {
        cfo_return_arg->cap_index = (cap_index[0] + cap_index[1] + cap_index[2]) / (test_channel_num);
        cfo_return_arg->cfo_val = (cfo_value[0] + cfo_value[1] + cfo_value[2]) / (test_channel_num);

        mp_arg->ref_cap_index = cfo_return_arg->cap_index;

        printk("mp test success cap index:%d\n", cfo_return_arg->cap_index);

        //only write efuse, if write efuse fail, report error
        if (mp_arg->cfo_write_mode == 1) {
            ret_cap_index = att_write_trim_cap(cfo_return_arg->cap_index, RW_TRIM_CAP_EFUSE);
            if (ret_cap_index == 0) {
                printk("cap write efuse fail\n");
                ret = false;
                goto test_end;
            }
        }

        //write efuse first, if write efuse fail, write nvram
        if (mp_arg->cfo_write_mode == 2) {
            ret_cap_index = att_write_trim_cap(cfo_return_arg->cap_index, RW_TRIM_CAP_SNOR);
            if (ret_cap_index == 0) {
                printk("cap write efuse+nvram fail\n");
                ret = false;
                goto test_end;
            }
        }
    }

test_end:

    return ret;
}


static bool att_cfo_read_test(mp_test_arg_t *mp_arg)
{
    cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg;;
    s16_t cfo_val[3] = {0};
    u8_t cap_value;

    bool ret = true;
    int ret_val, i;

    printk("mp read test start\n");

    cap_value = att_read_trim_cap(RW_TRIM_CAP_SNOR);

    for(i = 0; i < 3; i++) {
        if (mp_arg->cfo_channel[i] == 0xff)
            continue;

        ret = att_bttool_tx_begin(mp_arg->cfo_channel[i]);
        if (ret == false)
            goto test_end;

        ret_val = rf_rx_cfo_test_channel_once(mp_arg, mp_arg->cfo_channel[i], cap_value, &cfo_val[i]);
        if (ret_val < 0) {
            printk("mp read test,channel:%d,return fail\n", mp_arg->cfo_channel[i]);
            ret = false;
            goto test_end;
        }

        ret = att_bttool_tx_stop();
        if (ret == false) {
            printk("bttool stop fail\n");
            goto test_end;
        }

        if (cfo_val[i] < mp_arg->cfo_threshold_low || cfo_val[i] > mp_arg->cfo_threshold_high) {
            printk("mp read test fail,channel:%d,cfo:%d\n", mp_arg->cfo_channel[i], cfo_val[i]);
            ret = false;
            goto test_end;
        }
    }

    if (ret == true) {
        cfo_return_arg->cfo_val = (cfo_val[0] + cfo_val[1] + cfo_val[2]) / (test_channel_num);
        printk("mp read cfo value:%d\n", cfo_return_arg->cfo_val);
    }

test_end:

    return ret;
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
		if (act_test_read_rf_rx_arg() < 0) {
			SYS_LOG_INF("read rf rx arg fail\n");
			goto test_end_0;
		}
	}

    test_channel_num = 0;
    for (i = 0; i < 3; i++) {
        if (g_mp_test_arg.cfo_channel[i] != 0xff) {
            test_channel_num++;
        }
    }

    if (test_channel_num == 0) {
        printk("mp channel num is zero\n");
        goto test_end_0;
    }

    g_cfo_return_arg.cap_index_low = g_mp_test_arg.cap_index_low;
    g_cfo_return_arg.cap_index_high = g_mp_test_arg.cap_index_high;

	mp_core_load();

	if (self->test_id == TESTID_MP_TEST) {
		if (g_mp_test_arg.cfo_test) {
			ret_val = att_cfo_adjust_test(&g_mp_test_arg);
		}
	} else /*TESTID_MP_READ_TEST*/ {
		if (g_mp_test_arg.cfo_test) {
			ret_val = att_cfo_read_test(&g_mp_test_arg);
		}
	}

	if (ret_val == true) {
		if (g_mp_test_arg.ber_test || g_mp_test_arg.rssi_test) {
			s16_t cap_temp;
	        cap_temp = att_read_trim_cap(RW_TRIM_CAP_SNOR);
			ret_val = att_ber_rssi_test(&g_mp_test_arg, cap_temp);
		}
	}

	ret_val = act_test_report_rf_rx_result(ret_val);

test_end_0:
    SYS_LOG_INF("att test end : %d\n", ret_val);

#ifdef CONFIG_CPU_LOAD_STAT
    cpuload_debug_log_mask_or(CPULOAD_DEBUG_LOG_THREAD_RUNTIME);
#endif

    return ret_val;
}
