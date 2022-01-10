/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_bt_attr_read.c
*/

#include "ap_autotest_bt_attr_read.h"

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

read_bt_name_arg_t g_read_bt_name_arg;

static int get_nvram_bt_name(char* name)
{
    int ret_val;

    ret_val = property_get(CFG_BT_NAME, name, TEST_BTNAME_MAX_LEN);

    return ret_val;
}

static int get_nvram_ble_name(char* name)
{
    int ret_val;

    ret_val = property_get(CFG_BLE_NAME, name, TEST_BTBLENAME_MAX_LEN);

    return ret_val;
}

static int get_nvram_bt_addr(u8_t* addr)
{
    char cmd_data[16] = {0};
    u8_t addr_rev[6];
    int ret_val, i;

    ret_val = property_get(CFG_BT_MAC, cmd_data, 16);

    if (ret_val < 12)
       return -1;

    str_to_hex((char *)addr_rev, cmd_data, 6);

    for (i = 0; i < 6; i++)
    	addr[i] = addr_rev[5 - i];

    return 0;
}

static int act_test_read_bt_attr_arg(void)
{
    int ret_val = 0;
	u16_t *line_buffer;
	u16_t *start;
	u16_t *end;
	u8_t arg_num;

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
		
		if (self->test_id == TESTID_READ_BTNAME) {
			read_bt_name_arg_t *read_bt_name_arg = &g_read_bt_name_arg;

		    arg_num = 1;

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    read_bt_name_arg->cmp_btname_flag = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    unicode_to_utf8_str(start, end, read_bt_name_arg->cmp_btname, TEST_BTNAME_MAX_LEN);
		    SYS_LOG_INF("read bt name arg : %s\n", read_bt_name_arg->cmp_btname);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    read_bt_name_arg->cmp_blename_flag = (u8_t) unicode_to_int(start, end, 10);

		    act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    unicode_to_utf8_str(start, end, read_bt_name_arg->cmp_blename, TEST_BTBLENAME_MAX_LEN);
			SYS_LOG_INF("read ble name arg : %s\n", read_bt_name_arg->cmp_blename);
		}
	}

    return ret_val;
}

static bool act_test_report_bt_attr_result(bool ret_val)
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

static bool _check_bt_addr_valid(u8_t *bt_addr)
{
    int i;

    for (i = 0; i < 6; i++) {
        if ((bt_addr[i] != 0) && (bt_addr[i] != 0xff))
            return true;
    }

    return false;
}

//This test item is used to read Bluetooth address and provide it to PC tool for verification to confirm whether the address is correct
static bool act_test_read_bt_addr(void *arg_buffer)
{
    u8_t cmd_data[16];
    bool ret_val = false;
    int ret;

	memset(cmd_data, 0, sizeof(cmd_data));

    ret = get_nvram_bt_addr(cmd_data);
    if (ret < 0) {
    	SYS_LOG_INF("bt addr read fail\n");
    } else {
	    bytes_to_unicode(cmd_data, 5, 6, return_data->return_arg, &trans_bytes);

        if (_check_bt_addr_valid(cmd_data) == true)
            ret_val = true;
	}

	return ret_val;
}

static bool act_test_read_bt_name(read_bt_name_arg_t *read_bt_name_arg)
{
    bool ret_val = false;
    int ret;
    u8_t cmp_btname[TEST_BTNAME_MAX_LEN];
    u8_t cmp_blename[TEST_BTBLENAME_MAX_LEN];
    u8_t report_name[TEST_BTNAME_MAX_LEN+TEST_BTBLENAME_MAX_LEN];
    
    bool bt_name_valid = true;
    bool ble_name_valid = true;
    int cmp_btname_len;
    int cmp_blename_len;

    memset(cmp_btname, 0, sizeof(cmp_btname));
    memset(cmp_blename, 0, sizeof(cmp_blename));
    memset(report_name, 0, sizeof(report_name));
    
    ret = get_nvram_bt_name((char *)cmp_btname);
    if (ret <= 0) {
        SYS_LOG_WRN("get_nvram_bt_name fail!\n");
        memcpy(cmp_btname, "null", 5);
    }

    SYS_LOG_INF("read bt name: %s\n", cmp_btname);

    ret = get_nvram_ble_name((char *)cmp_blename); 
    if (ret <= 0) {
        SYS_LOG_WRN("get_nvram_ble_name fail!\n");
        memcpy(cmp_blename, "null", 5);
    }

    SYS_LOG_INF("read ble name: %s\n", cmp_blename);

    cmp_btname_len = strlen((const char *)cmp_btname);
    cmp_blename_len = strlen((const char *)cmp_blename);
    memcpy(report_name, cmp_btname, cmp_btname_len);
    report_name[cmp_btname_len] = ',';
    memcpy(&report_name[cmp_btname_len+1], cmp_blename, cmp_blename_len);

    utf8str_to_unicode(report_name, sizeof(report_name), return_data->return_arg, &trans_bytes); 

    if (bt_name_valid == true) {
        if (cmp_btname[0] != 0) {
            if (ble_name_valid == true) {    
                if (cmp_blename[0] != 0) {
                    ret_val = true;
                }
            }    
        }
    }
    
    if (ret_val == true) {
    	if (read_bt_name_arg->cmp_btname_flag == true) {
    		if (memcmp(cmp_btname, read_bt_name_arg->cmp_btname, 
    			strlen((const char *)(read_bt_name_arg->cmp_btname)) + 1) != 0)
    			ret_val = false;
    	}
    }

    if (ret_val == true) {
        if (read_bt_name_arg->cmp_blename_flag == true) {
            if (memcmp(cmp_blename, read_bt_name_arg->cmp_blename, 
            	strlen((const char *)(read_bt_name_arg->cmp_blename)) + 1) != 0)
            	ret_val = false;
        }
    }
    
    return ret_val;
}

bool pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	self = para;
	return_data = (return_result_t *) (self->return_data_buffer);
	trans_bytes = 0;

	if (self->arg_len != 0) {
		if (act_test_read_bt_attr_arg() < 0) {
			SYS_LOG_INF("read bt attr arg fail\n");
			goto main_exit;
		}
	}

	if (self->test_id == TESTID_READ_BTNAME) {
		ret_val = act_test_read_bt_name(&g_read_bt_name_arg);
	} else /*TESTID_READ_BTADDR*/ {
		ret_val = act_test_read_bt_addr(NULL);
	}

main_exit:

	SYS_LOG_INF("att report result : %d\n", ret_val);

	ret_val = act_test_report_bt_attr_result(ret_val);

    SYS_LOG_INF("att test end : %d\n", ret_val);
    
    return ret_val;
}
