/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_gpio.c
*/

#include "ap_autotest_gpio.h"

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

struct device *gpio_dev;
gpio_test_arg_t g_gpio_test_arg;
gpio_test_result_t g_gpio_test_result;

static int act_test_read_gpio_arg(void)
{
    int ret_val = 0;
	u16_t *line_buffer;
	u16_t *start;
	u16_t *end;
	gpio_test_arg_t *gpio_arg = &g_gpio_test_arg;

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
		
		act_test_parse_test_arg(line_buffer, 1, &start, &end);

	    gpio_arg->gpioA_value = unicode_to_int(start, end, 16);

	    act_test_parse_test_arg(line_buffer, 2, &start, &end);

	    gpio_arg->gpioB_value = unicode_to_int(start, end, 16);

	    act_test_parse_test_arg(line_buffer, 3, &start, &end);

	    gpio_arg->gpioWIO_value = unicode_to_int(start, end, 16);
	    
	    SYS_LOG_INF("gpio arg : 0x%x,0x%x,0x%x\n", gpio_arg->gpioA_value, gpio_arg->gpioB_value, gpio_arg->gpioWIO_value);
	}

    return ret_val;
}

static bool act_test_report_gpio_result(bool ret_val)
{
    return_data->test_id = self->test_id;

    return_data->test_result = ret_val;

    uint32_to_unicode(g_gpio_test_result.err_pin, return_data->return_arg, &trans_bytes, 16);
    return_data->return_arg[trans_bytes++] = 0x002c; //add ','

    uint32_to_unicode(g_gpio_test_result.err_type, &(return_data->return_arg[trans_bytes]), &trans_bytes, 16);
    return_data->return_arg[trans_bytes++] = 0x002c; //add ','

    uint32_to_unicode(g_gpio_test_result.reserved, &(return_data->return_arg[trans_bytes]), &trans_bytes, 16);

    //add endl
    return_data->return_arg[trans_bytes++] = 0x0000;

    //If the parameter is not four byte aligned, four byte alignment is required
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;

    return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

bool test_gpio(gpio_test_arg_t *gpio_test_arg)
{
    bool ret_val = false;
    int ret;
    u8_t err_pin;

    ret = test_gpio_short(gpio_test_arg->gpioA_value, gpio_test_arg->gpioB_value, gpio_test_arg->gpioWIO_value, &err_pin);
    if (ret != 0) {
        SYS_LOG_WRN("GPIO TEST(SHORT) NG , err pin is %d\n", err_pin);
        goto end_test;
    }
    SYS_LOG_INF("GPIO TEST(SHORT) OK\n");

    //short circuit to GND test
    ret = test_gpio_short_gnd(gpio_test_arg->gpioA_value, gpio_test_arg->gpioB_value, gpio_test_arg->gpioWIO_value, &err_pin);
    if (ret != 0)
    {
        SYS_LOG_WRN("GPIO TEST(SHORT GND) NG , err pin is %d\n", err_pin);
        goto end_test;
    }
    SYS_LOG_INF("GPIO TEST(SHORT GND) OK\n");

    //open circuit test
    ret = test_gpio_open(gpio_test_arg->gpioA_value, gpio_test_arg->gpioB_value, gpio_test_arg->gpioWIO_value, &err_pin);
    if (ret != 0)
    {
        SYS_LOG_WRN("GPIO TEST(OPEN) NG , err pin is %d\n", err_pin);
        goto end_test;
    }
    SYS_LOG_INF("GPIO TEST(OPEN) OK\n");
    
    ret_val = true;
    err_pin = 0;

    end_test:

    g_gpio_test_result.err_pin = err_pin;
    g_gpio_test_result.err_type = ret;

    return ret_val;
}

bool pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	self = para;
	return_data = (return_result_t *) (self->return_data_buffer);
	trans_bytes = 0;

	if (self->arg_len != 0) {
		if (act_test_read_gpio_arg() < 0) {
			SYS_LOG_INF("read bt attr arg fail\n");
			goto main_exit;
		}
	}

	gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
	if (!gpio_dev) {
		SYS_LOG_WRN("gpio device not find: %s\n", CONFIG_GPIO_ACTS_DEV_NAME);
		goto main_exit;
	}

	ret_val = test_gpio(&g_gpio_test_arg);

main_exit:

	SYS_LOG_INF("att report result : %d\n", ret_val);

	ret_val = act_test_report_gpio_result(ret_val);

    SYS_LOG_INF("att test end : %d\n", ret_val);
    
    return ret_val;
}

