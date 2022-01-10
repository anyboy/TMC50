/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_bt_attr_modify.c
*/

#include "ap_autotest_bt_attr_modify.h"

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

bt_name_arg_t g_bt_name_arg;
ble_name_arg_t g_ble_name_arg;
bt_addr_arg_t g_bt_addr_arg;

static int get_nvram_bt_name(char* name)
{
    int ret_val;

    ret_val = property_get(CFG_BT_NAME, name, TEST_BTNAME_MAX_LEN);

    return ret_val;
}

static int set_nvram_bt_name(const char* name)
{
    char cmd_data[TEST_BTNAME_MAX_LEN] = {0};
    int ret_val;
    int len;

    len = strlen(name) + 1 > TEST_BTNAME_MAX_LEN ? TEST_BTNAME_MAX_LEN : strlen(name) + 1;
    memcpy(cmd_data, name, len - 1);

    ret_val = property_set(CFG_BT_NAME, cmd_data, len);
    if (ret_val < 0)
        return ret_val;

    property_flush(CFG_BT_NAME);

    return 0;
}

static int get_nvram_ble_name(char* name)
{
    int ret_val;

    ret_val = property_get(CFG_BLE_NAME, name, TEST_BTBLENAME_MAX_LEN);

    return ret_val;
}

static int set_nvram_ble_name(const char* name)
{
    char cmd_data[TEST_BTBLENAME_MAX_LEN] = {0};
    int ret_val;
    int len;

    len = strlen(name) + 1 > TEST_BTBLENAME_MAX_LEN ? TEST_BTBLENAME_MAX_LEN : strlen(name) + 1;
    memcpy(cmd_data, name, len - 1);

    ret_val = property_set(CFG_BLE_NAME, cmd_data, len);
    if (ret_val < 0)
         return ret_val;

    property_flush(CFG_BLE_NAME);

    return 0;
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

static int set_nvram_bt_addr(const u8_t* addr)
{
    u8_t mac_str[16] = {0};
    u8_t addr_rev[6];
    int ret_val, i;

    for (i = 0; i < 6; i++)
    	addr_rev[i] = addr[5 - i];

    hex_to_str((char *)mac_str, (char *)addr_rev, 6);

    ret_val = property_set(CFG_BT_MAC, (char *)mac_str, 12);
    if (ret_val < 0)
        return ret_val;

    property_flush(CFG_BT_MAC);

    return 0;
}


static int act_test_read_bt_attr_arg(void)
{
    int ret_val = 0;
	u16_t *line_buffer;
	u16_t *start;
	u16_t *end;

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
		
		if (self->test_id == TESTID_MODIFY_BTNAME) {
			bt_name_arg_t *bt_name_arg = &g_bt_name_arg;

		    act_test_parse_test_arg(line_buffer, 1, &start, &end);

		    unicode_to_utf8_str(start, end, bt_name_arg->bt_name, TEST_BTNAME_MAX_LEN);
		    
		    SYS_LOG_INF("read bt name arg : %s\n", bt_name_arg->bt_name);
		} else if (self->test_id == TESTID_MODIFY_BLENAME) {
			ble_name_arg_t *ble_name_arg = &g_ble_name_arg;
		    
		    act_test_parse_test_arg(line_buffer, 1, &start, &end);

		    unicode_to_utf8_str(start, end, ble_name_arg->bt_ble_name, TEST_BTBLENAME_MAX_LEN);
		    
		    SYS_LOG_INF("read ble name arg : %s\n", ble_name_arg->bt_ble_name);
		} else if (self->test_id == TESTID_MODIFY_BTADDR) {
			bt_addr_arg_t *bt_addr_arg = &g_bt_addr_arg;

		    act_test_parse_test_arg(line_buffer, 1, &start, &end);

		    //read byte5, byte4, byte3 data
		    unicode_to_uint8_bytes(start, end, bt_addr_arg->bt_addr, 5, 3);
		    
		    act_test_parse_test_arg(line_buffer, 2, &start, &end);

    		//read byte2, byte1, byte0 data
    		unicode_to_uint8_bytes(start, end, bt_addr_arg->bt_addr, 2, 3);
    		
    		SYS_LOG_INF("read bt mac arg : %02x%02x%02x%02x%02x%02x\n", 
    				bt_addr_arg->bt_addr[5], bt_addr_arg->bt_addr[4], bt_addr_arg->bt_addr[3],
    				bt_addr_arg->bt_addr[2], bt_addr_arg->bt_addr[1], bt_addr_arg->bt_addr[0]);
		} else /*TESTID_MODIFY_BTADDR2*/ {
			bt_addr_arg_t *bt_addr_arg = &g_bt_addr_arg;

		    act_test_parse_test_arg(line_buffer, 1, &start, &end);

		    //The complete 6-byte address is entered in the Editer
		    unicode_to_uint8_bytes(start, end, bt_addr_arg->bt_addr, 5, 6);

    		SYS_LOG_INF("read bt mac arg : %02x%02x%02x%02x%02x%02x\n", 
    				bt_addr_arg->bt_addr[5], bt_addr_arg->bt_addr[4], bt_addr_arg->bt_addr[3],
    				bt_addr_arg->bt_addr[2], bt_addr_arg->bt_addr[1], bt_addr_arg->bt_addr[0]);
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

static bool act_test_modify_bt_name(bt_name_arg_t *bt_name_arg)
{
    bool ret_val = false;
    int ret = 0;
    u8_t name_len;
	u8_t bt_name_check[TEST_BTNAME_MAX_LEN];

    name_len = strlen((const char *)(bt_name_arg->bt_name)) + 1;

	utf8str_to_unicode(bt_name_arg->bt_name, name_len, return_data->return_arg, &trans_bytes);

    ret = set_nvram_bt_name((const char *)(bt_name_arg->bt_name));
    if (ret < 0) {
        SYS_LOG_INF("bt name write fail\n");
    } else {
    	memset(bt_name_check, 0, sizeof(bt_name_check));
    	
    	ret = get_nvram_bt_name((char *)bt_name_check);
    	if (ret <= 0) {
    		SYS_LOG_INF("bt name read fail\n");
    	} else {
	    	SYS_LOG_INF("bt name check : %s\n", bt_name_check);
	    	if (memcmp(bt_name_arg->bt_name, bt_name_check, name_len) == 0)
		        ret_val = true;
	    }
    }

    return ret_val;
}

static bool act_test_modify_bt_ble_name(ble_name_arg_t *ble_name_arg)
{
    bool ret_val = false;
    int ret;
    u8_t name_len;
    u8_t ble_name_check[TEST_BTBLENAME_MAX_LEN];

    name_len = strlen((const char *)(ble_name_arg->bt_ble_name)) + 1;
    
    utf8str_to_unicode(ble_name_arg->bt_ble_name, name_len, return_data->return_arg, &trans_bytes);

    ret = set_nvram_ble_name((const char *)(ble_name_arg->bt_ble_name));
    if (ret < 0) {
        SYS_LOG_INF("ble name write fail\n");
    } else {
    	memset(ble_name_check, 0, sizeof(ble_name_check));
    	
    	ret = get_nvram_ble_name((char *)ble_name_check);
    	if (ret < 0) {
    		SYS_LOG_INF("ble name read fail\n");
    	} else {
	    	SYS_LOG_INF("ble name check : %s\n", ble_name_check);
	    	
	    	if (memcmp(ble_name_arg->bt_ble_name, ble_name_check, name_len) == 0)
		        ret_val = true;
		    }
    }

    return ret_val;
}

static bool act_test_modify_bt_addr(bt_addr_arg_t *bt_addr_arg)
{
    bool ret_val = false;
    int ret;
    u8_t bt_addr_check[16];

    ret = set_nvram_bt_addr((const u8_t *)(bt_addr_arg->bt_addr));
    if (ret < 0) {
        SYS_LOG_INF("bt addr write fail\n");
    } else {
    	memset(bt_addr_check, 0, sizeof(bt_addr_check));
    	
    	ret = get_nvram_bt_addr(bt_addr_check);
    	if (ret < 0) {
    		SYS_LOG_INF("bt addr read fail\n");
    	} else {
			SYS_LOG_INF("bt mac check : %02x%02x%02x%02x%02x%02x\n", 
				bt_addr_check[5], bt_addr_check[4], bt_addr_check[3],
				bt_addr_check[2], bt_addr_check[1], bt_addr_check[0]);

	        bytes_to_unicode(bt_addr_check, 5, 6, return_data->return_arg, &trans_bytes);
	        return_data->return_arg[trans_bytes++] = 0x002c; //add ','
	        bytes_to_unicode(&(bt_addr_arg->bt_addr_add_mode), 0, 1, &(return_data->return_arg[trans_bytes]), &trans_bytes);
	        return_data->return_arg[trans_bytes++] = 0x002c; //add ','
	        bytes_to_unicode(&(bt_addr_arg->bt_burn_mode), 0, 1, &(return_data->return_arg[trans_bytes]), &trans_bytes);

	    	if (memcmp(bt_addr_arg->bt_addr, bt_addr_check, 6) == 0)
		        ret_val = true;
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

	if (self->test_id == TESTID_MODIFY_BTNAME) {
		ret_val = act_test_modify_bt_name(&g_bt_name_arg);
	} else if (self->test_id == TESTID_MODIFY_BLENAME) {
		ret_val = act_test_modify_bt_ble_name(&g_ble_name_arg);
	} else if (self->test_id == TESTID_MODIFY_BTADDR) {
		ret_val = act_test_modify_bt_addr(&g_bt_addr_arg);
	} else /*TESTID_MODIFY_BTADDR2*/ {
		ret_val = act_test_modify_bt_addr(&g_bt_addr_arg);
	}

main_exit:

	SYS_LOG_INF("att report result : %d\n", ret_val);

	ret_val = act_test_report_bt_attr_result(ret_val);

    SYS_LOG_INF("att test end : %d\n", ret_val);
    
    return ret_val;
}

