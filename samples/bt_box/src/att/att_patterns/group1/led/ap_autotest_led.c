/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_led.c
*/

#include "ap_autotest_led.h"

#define LED_ON_LEVEL (1)
#define LED_ON       (LED_ON_LEVEL)
#define LED_OFF      (!LED_ON)

//GPIOs connected to LEDs
const u32_t LED_gpio[] = {24, 25, 26};

struct att_env_var *self;
struct device *gpio_dev;
return_result_t *return_data;
u16_t trans_bytes;

static void set_led_onoff(u32_t pin, u8_t state)
{
    gpio_pin_configure(gpio_dev, LED_gpio[pin], GPIO_DIR_OUT);
    gpio_pin_write(gpio_dev, LED_gpio[pin], state);
    k_busy_wait(1000);
}

static void set_led_all_onoff(u8_t state)
{
    u8_t i;
    for(i = 0; i < ARRAY_SIZE(LED_gpio); i++) {
        set_led_onoff(i, state);
    }
}

static void set_led_single_blink(u32_t pin)
{
    set_led_onoff(pin, LED_ON);
    k_busy_wait(500*1000);
    set_led_onoff(pin, LED_OFF);
}

static void set_led_blink(void)
{
    u8_t i, j;

    set_led_all_onoff(LED_OFF);
    
    for(i = 0; i < 3; i++) {
        for(j = 0; j< ARRAY_SIZE(LED_gpio); j++) {
            set_led_single_blink(j);
        }
    }
}

static bool act_test_led_test(void)
{
    bool ret_val = false;

    gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
    if(!gpio_dev) {
        SYS_LOG_WRN("gpio device not find: %s\n", CONFIG_GPIO_ACTS_DEV_NAME);
        return ret_val;
    }
    
    set_led_all_onoff(LED_OFF);
    set_led_blink();
    set_led_all_onoff(LED_ON);
    
    ret_val = true;
    return ret_val;
}

static bool act_test_report_led_result(bool ret_val)
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

bool pattern_main(struct att_env_var * para)
{
    bool ret_val = false;

    self = para;
    return_data = (return_result_t *) (self->return_data_buffer);
    trans_bytes = 0;
    
    ret_val = act_test_led_test();
    SYS_LOG_INF("att report result : %d\n", ret_val);
    ret_val = act_test_report_led_result(ret_val);

    SYS_LOG_INF("att test end : %d\n", ret_val);
    return ret_val;
}