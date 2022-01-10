/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_report_result.c
*/

#include "att_pattern_header.h"

extern struct att_env_var *self;

//If the command is sent to PC software without parameters, and the return response has no parameters.
//It is directly called through this interface
bool att_cmd_send(u16_t stub_cmd, u8_t try_count)
{
    bool ret_val = false;
    int ret;
    uint8_t count = 0;

test_start:
    ret = att_write_data(stub_cmd, 0, self->rw_temp_buffer);
    if (ret == 0) {       
        ret = att_read_data(STUB_CMD_ATT_ACK, 0, self->rw_temp_buffer);
        if (ret == 0)
            ret_val = true;
    }
    
    if (++count < try_count && ret_val == false) {
        k_sleep(150);
        goto test_start;
    }
    
    if (ret_val == false)
        SYS_LOG_INF("att cmd :0x%x fail, try count:%d\n", stub_cmd, try_count);
    
    return ret_val;
}

