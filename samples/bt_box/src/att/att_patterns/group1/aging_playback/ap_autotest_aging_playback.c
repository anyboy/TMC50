/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_aging_playback.c
*/

#include "ap_autotest_aging_playback.h"

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

aging_playback_start_arg_t g_aging_pb_start_arg;
aging_playback_check_arg_t g_aging_pb_check_arg;
aging_playback_result_t g_aging_pb_result;

static struct aging_playback_info g_aging_pb_info;

static int get_nvram_aging_pb(struct aging_playback_info* aging_pb_info)
{
    int ret_val;

    ret_val = property_get(CFG_AGING_PLAYBACK_INFO, (char *)aging_pb_info, sizeof(struct aging_playback_info));

    return ret_val;
}

static int set_nvram_aging_pb(struct aging_playback_info* aging_pb_info)
{
    int ret_val;
    ret_val = property_set(CFG_AGING_PLAYBACK_INFO, (char *)aging_pb_info, sizeof(struct aging_playback_info));
    if (ret_val < 0)
        return ret_val;

    property_flush(CFG_AGING_PLAYBACK_INFO);

    return 0;
}

static int act_test_aging_playback_arg(void)
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
		
		if (self->test_id == TESTID_AGING_PB_START) {
			aging_playback_start_arg_t *aging_pb_start_arg = &g_aging_pb_start_arg;

		    arg_num = 1;
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    aging_pb_start_arg->aging_playback_source = unicode_to_int(start, end, 10);

			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    aging_pb_start_arg->aging_playback_volume = unicode_to_int(start, end, 10);

			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    aging_pb_start_arg->aging_playback_duration = unicode_to_int(start, end, 10);

		    SYS_LOG_INF("read aging playback arg : %d, %d, %d\n", 
					aging_pb_start_arg->aging_playback_source,
					aging_pb_start_arg->aging_playback_volume,
					aging_pb_start_arg->aging_playback_duration);
		} else /*TESTID_AGING_PB_CHECK*/ {
			aging_playback_check_arg_t *aging_playback_check_arg = &g_aging_pb_check_arg;

		    arg_num = 1;
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    aging_playback_check_arg->cmp_aging_playback_status = unicode_to_int(start, end, 10);

			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    aging_playback_check_arg->stop_aging_playback = unicode_to_int(start, end, 10);

    		SYS_LOG_INF("read aging playback check arg : %d,%d\n", 
    				aging_playback_check_arg->cmp_aging_playback_status,
    				aging_playback_check_arg->stop_aging_playback);
		}
	}

    return ret_val;
}

static bool act_test_report_aging_playback_result(bool ret_val)
{
    return_data->test_id = self->test_id;

    return_data->test_result = ret_val;

	uint32_to_unicode(g_aging_pb_result.aging_playback_status, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    //add endl
    return_data->return_arg[trans_bytes++] = 0x0000;

    //If the parameter is not four byte aligned, four byte alignment is required
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;

    return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

static bool act_test_aging_playback_start(aging_playback_start_arg_t *aging_pb_start_arg)
{
    bool ret_val = true;
    int ret = 0;

	ret = get_nvram_aging_pb(&g_aging_pb_info);
	if (ret < 0) {
		memset(&g_aging_pb_info, 0, sizeof(struct aging_playback_info));
		ret = set_nvram_aging_pb(&g_aging_pb_info);
		if (ret < 0) {
			SYS_LOG_INF("aging playback info set fail\n");
			ret_val = false;
			g_aging_pb_result.aging_playback_status = AGING_PLAYBACK_STA_ERROR_VNRAM;
		}
	}

	if (ret_val == true) {
		g_aging_pb_info.aging_playback_status = AGING_PLAYBACK_STA_DOING;
		g_aging_pb_info.aging_playback_source = aging_pb_start_arg->aging_playback_source;
		g_aging_pb_info.aging_playback_volume = aging_pb_start_arg->aging_playback_volume;
		g_aging_pb_info.aging_playback_duration = aging_pb_start_arg->aging_playback_duration;
		g_aging_pb_info.aging_playback_consume = 0;
		ret = set_nvram_aging_pb(&g_aging_pb_info);
		if (ret < 0) {
			SYS_LOG_INF("aging playback info set fail\n");
			ret_val = false;
			g_aging_pb_result.aging_playback_status = AGING_PLAYBACK_STA_ERROR_VNRAM;
		} else {
			g_aging_pb_result.aging_playback_status = AGING_PLAYBACK_STA_DOING;
		}
	}

	return ret_val;
}

static bool act_test_aging_playback_check(aging_playback_check_arg_t *aging_pb_check_arg)
{
    bool ret_val = true;
    int ret = 0;

	ret = get_nvram_aging_pb(&g_aging_pb_info);
	if (ret < 0) {
		SYS_LOG_INF("aging playback info read fail\n");
		ret_val = false;
		g_aging_pb_result.aging_playback_status = AGING_PLAYBACK_STA_ERROR_VNRAM;
	} else {
		g_aging_pb_result.aging_playback_status = g_aging_pb_info.aging_playback_status;
		if (aging_pb_check_arg->cmp_aging_playback_status == 1) {
			if (g_aging_pb_info.aging_playback_status != AGING_PLAYBACK_STA_PASSED) {
				ret_val = false;
			}
		}

		if (aging_pb_check_arg->stop_aging_playback == 1) {
			if (g_aging_pb_info.aging_playback_status == AGING_PLAYBACK_STA_DOING) {
				g_aging_pb_info.aging_playback_status = AGING_PLAYBACK_STA_DISABLE;
				ret = set_nvram_aging_pb(&g_aging_pb_info);
				if (ret < 0) {
					SYS_LOG_INF("aging playback info set fail\n");
					ret_val = false;
					g_aging_pb_result.aging_playback_status = AGING_PLAYBACK_STA_ERROR_VNRAM;
				}
			}
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
		if (act_test_aging_playback_arg() < 0) {
			SYS_LOG_INF("read aging playback arg fail\n");
			goto main_exit;
		}
	}

	if (self->test_id == TESTID_AGING_PB_START) {
		ret_val = act_test_aging_playback_start(&g_aging_pb_start_arg);
	} else /*TESTID_AGING_PB_CHECK*/ {
		ret_val = act_test_aging_playback_check(&g_aging_pb_check_arg);
	}

main_exit:

	SYS_LOG_INF("att report result : %d\n", ret_val);

	ret_val = act_test_report_aging_playback_result(ret_val);

    SYS_LOG_INF("att test end : %d\n", ret_val);
    
    return ret_val;
}

