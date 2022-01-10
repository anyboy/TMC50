/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_exit_mode.c
*/

#include "att_pattern_header.h"


struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

static bool act_test_exit_mode_arg(void)
{
	int ret_val = 0;
	u16_t *line_buffer, *start, *end;
	u8_t arg_num;

	ret_val = att_write_data(STUB_CMD_ATT_GET_TEST_ARG, 0, self->rw_temp_buffer);

	if (ret_val == 0) {
		ret_val = att_read_data(STUB_CMD_ATT_GET_TEST_ARG, self->arg_len, self->rw_temp_buffer);

		if (ret_val == 0) {
			u8_t temp_data[8];
			//do not use STUB_ATT_RW_TEMP_BUFFER, which will be used to parse parameters
			ret_val = att_write_data(STUB_CMD_ATT_ACK, 0, temp_data);
		}
	}

	/* set exit_mode */
	if (ret_val == 0) {
		line_buffer = (u16_t *)(self->rw_temp_buffer + sizeof(stub_ext_cmd_t));
		arg_num = 1;
		act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		self->exit_mode = unicode_to_int(start, end, 10);
		SYS_LOG_INF("arg exit_mode: %d\n", self->exit_mode);
		return true;
	}

	return false;
}

static bool act_test_report_exit_mode_result(bool ret_val)
{
	return_data->test_id = self->test_id;
	return_data->test_result = ret_val;

	//add terminator
	return_data->return_arg[trans_bytes++] = 0x0000;

	//four-byte alignment
	if ((trans_bytes % 2) != 0)
		return_data->return_arg[trans_bytes++] = 0x0000;

	return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

bool pattern_main(struct att_env_var *para)
{
	bool ret_val = false;
	self = para;
	return_data = (return_result_t *) (self->return_data_buffer);
	trans_bytes = 0;

	if (self->arg_len != 0)
		ret_val = act_test_exit_mode_arg();

	if (ret_val == false)
		SYS_LOG_INF("read exit_mode arg fail\n");

	ret_val = act_test_report_exit_mode_result(ret_val);

	SYS_LOG_INF("att pattern end: %d\n", ret_val);
	return ret_val;
}
