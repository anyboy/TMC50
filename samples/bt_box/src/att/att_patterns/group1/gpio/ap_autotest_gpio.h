/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_gpio.h
*/

#ifndef _AP_AUTOTEST_GPIO_H_
#define _AP_AUTOTEST_GPIO_H_

#include "att_pattern_header.h"
#include "soc_gpio.h"
#include "gpio.h"

typedef struct {
    u32_t gpioA_value;      //GPIO0 -GPIO31 which GPIO need test,one bit is one GPIO PIN
    u32_t gpioB_value;      //GPIO32-GPIO52 which GPIO need test,one bit is one GPIO PIN
    u32_t gpioWIO_value;    //WIO0 - WIO2 which GPIO need test,one bit is one GPIO PIN
} gpio_test_arg_t;

typedef struct {
    u32_t err_pin;          //test error pin index
    u32_t err_type;         //test error type
    u32_t reserved;         //reserved
} gpio_test_result_t;

extern struct device *gpio_dev;
extern gpio_test_arg_t g_gpio_test_arg;
extern gpio_test_result_t g_gpio_test_result;

#define TEST_GPIO_SHORT      1
#define TEST_GPIO_SHORT_VCC  2
#define TEST_GPIO_SHORT_GND  3
#define TEST_GPIO_OPEN       4

extern int test_gpio_short(u32_t reg_val_A, u32_t reg_val_B, u32_t io_ctrl, u8_t *index);
extern int test_gpio_short_gnd(u32_t reg_val_A, u32_t reg_val_B, u32_t io_ctrl, u8_t *index);
extern int test_gpio_open(u32_t reg_val_A, u32_t reg_val_B, u32_t io_ctrl, u8_t *index);

#endif
