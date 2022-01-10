/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_stub_comm.c
*/

#include "ap_autotest.h"


s32_t att_write_data(u16_t cmd, u32_t payload_len, u8_t *data_addr)
{
    s32_t ret_val;
    stub_ext_param_t ext_param;

    if((payload_len + 8) == 64)
    {
        payload_len += 4;
    }

    ext_param.opcode = cmd;
    ext_param.payload_len = payload_len;
    ext_param.rw_buffer = (u8_t *) data_addr;

    ret_val = stub_ext_rw(stub_dev, &ext_param, 1);

    return ret_val;
}

s32_t att_read_data(u16_t cmd, u32_t payload_len, u8_t *data_addr)
{
    s32_t ret_val;
    stub_ext_param_t ext_param;

    ext_param.opcode = cmd;
    ext_param.payload_len = payload_len;
    ext_param.rw_buffer = (u8_t *) data_addr;

    ret_val = stub_ext_rw(stub_dev, &ext_param, 0);

    return ret_val;
}

s32_t att_log_to_pc(u16_t log_cmd, const u8_t *msg, u8_t *cmd_buf)
{
    print_log_t *p_print_log;
    u8_t *error_msg;
    int ret_val = 0;
    int len, write_len;

    len = strlen(msg);
    write_len = (len + 1 + 3)&(~0x03); /*1 for null char, 3 for 4 bytes align*/

    memset(cmd_buf, 0, write_len);
    p_print_log = (print_log_t *) STUB_ATT_RW_TEMP_BUFFER;
    error_msg = (u8_t *) (p_print_log->log_data);
    strcpy(error_msg, msg);

    /* + 1 is \0; + 3 is word align */
    ret_val = att_write_data(log_cmd, write_len, (u8_t *)p_print_log);

    return ret_val;
}

bool act_test_report_result(return_result_t *write_data, u16_t payload_len)
{
    bool ret_val = false;
    int ret;
    u8_t cmd_data[8];

    write_data->timeout = 5;
    
    while (1) {
        ret = att_write_data(STUB_CMD_ATT_REPORT_TRESULT, payload_len, (u8_t *)write_data);
        if (ret == 0) {  
            ret = att_read_data(STUB_CMD_ATT_REPORT_TRESULT, 0, (u8_t *)cmd_data);
            if (ret == 0) {
                //comfirm if ACK packet
                if (cmd_data[1] == 0x04 && cmd_data[2] == 0xfe) {
                    ret_val = true;
                    break;
                }
            }
        }
    }

    return ret_val;
}
