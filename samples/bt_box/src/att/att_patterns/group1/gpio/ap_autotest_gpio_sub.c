/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_gpio_sub.c
*/

#include "ap_autotest_gpio.h"

int test_gpio_short(u32_t reg_val_A, u32_t reg_val_B, u32_t io_ctrl, u8_t *index)
{
    u8_t i, j;
    u32_t reg_value;
    u32_t gpio_dat1, gpio_dat2;
    u32_t reg_val;

    //test control io disable pu/pd resistance, output high, GPIO under test enable pd resistance
    gpio_pin_configure(gpio_dev, io_ctrl, GPIO_DIR_OUT);
    gpio_pin_write(gpio_dev, io_ctrl, 1);
    k_busy_wait(1000);

    //gpio[0~63]
    for (i = 0; i < GPIO_MAX_PIN_NUM; i++) {
        if (i < 32) {
            reg_val = reg_val_A;
            reg_value = (1 << i);
        } else {
            reg_val = reg_val_B;
            reg_value = (1 << (i - 32));
        }

        if ((reg_val & reg_value) != 0) {
            //GPIO under test ing -> input
            gpio_pin_configure(gpio_dev, i, GPIO_DIR_IN);
            k_busy_wait(1000);

            //other GPIO under test -> output high
            for (j = 0; j < GPIO_MAX_PIN_NUM; j++) {
                if (j < 32) {
                    reg_val = reg_val_A;
                    reg_value = (1 << j);
                } else {
                    reg_val = reg_val_B;
                    reg_value = (1 << (j - 32));
                }
                if (((reg_val & reg_value) != 0) && (j != i)) {
                    gpio_pin_configure(gpio_dev, j, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
    				gpio_pin_write(gpio_dev, j, 1);
                }
            }

            k_busy_wait(1000);

            //read input level of gpio under testing
            gpio_pin_read(gpio_dev, i, &gpio_dat1);

            //other GPIO under test -> output low
            for (j = 0; j < GPIO_MAX_PIN_NUM; j++) {
                if (j < 32) {
                    reg_val = reg_val_A;
                    reg_value = (1 << j);
                } else {
                    reg_val = reg_val_B;
                    reg_value = (1 << (j - 32));
                }

                if (((reg_val & reg_value) != 0) && (j != i)) {
                    gpio_pin_configure(gpio_dev, j, GPIO_DIR_OUT | GPIO_PUD_PULL_DOWN);
    				gpio_pin_write(gpio_dev, j, 0);
                }
            }

            k_busy_wait(1000);

            //read input level of gpio under testing, if level changed, means there is a short circuit
            gpio_pin_read(gpio_dev, i, &gpio_dat2);
            if (gpio_dat2 != gpio_dat1) {
                SYS_LOG_WRN("TEST_GPIO_SHORT\n");
                *index = i;
                return TEST_GPIO_SHORT;
            } else {
                //if always detect high level, means VCC is short circuit
                if (gpio_dat1 != 0) {
                    SYS_LOG_WRN("TEST_GPIO_SHORT_VCC\n");
                    *index = i;
                    return TEST_GPIO_SHORT_VCC;
                }
            }
        }
    }

    return 0;
}

int test_gpio_short_gnd(u32_t reg_val_A, u32_t reg_val_B, u32_t io_ctrl, u8_t *index)
{
    u8_t i;
    u32_t reg_value;
    u32_t gpio_dat;
    u32_t reg_val;

    //control io output low, GPIO under test enable pu resistance
    gpio_pin_configure(gpio_dev, io_ctrl, GPIO_DIR_OUT);
	gpio_pin_write(gpio_dev, io_ctrl, 0);
    k_busy_wait(1000);

    //all GPIO under test, disable pu/pd resistance, input mode
    for (i = 0; i < GPIO_MAX_PIN_NUM; i++) {
        if (i < 32) {
            reg_val = reg_val_A;
            reg_value = (1 << i);
        } else {
            reg_val = reg_val_B;
            reg_value = (1 << (i - 32));
        }

        if ((reg_val & reg_value) != 0) {
            gpio_pin_configure(gpio_dev, i, GPIO_DIR_IN);
        }
    }

    //read GPIO under test level
    for (i = 0; i < GPIO_MAX_PIN_NUM; i++) {
        if (i < 32) {
            reg_val = reg_val_A;
            reg_value = (1 << i);
        } else {
            reg_val = reg_val_B;
            reg_value = (1 << (i - 32));
        }

        if ((reg_val & reg_value) != 0) {
            //if GPIO under test is low level, it means short circuit to gnd
            gpio_pin_read(gpio_dev, i, &gpio_dat);
            if (gpio_dat == 0) {
                *index = i;
                return TEST_GPIO_SHORT_GND;
            }
        }
    }

    return 0;
}

int test_gpio_open(u32_t reg_val_A, u32_t reg_val_B, u32_t io_ctrl, u8_t *index)
{
    u8_t i;
    u32_t reg_value;
    u32_t gpio_dat;
    u32_t reg_val;

    //control IO output high level, GPIO under test enable pd resistance
    gpio_pin_configure(gpio_dev, io_ctrl, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(gpio_dev, io_ctrl, 1);

    //GPIO under test disable pu/pd resistance, input mode
    for (i = 0; i < GPIO_MAX_PIN_NUM; i++) {
        if (i < 32) {
            reg_val = reg_val_A;
            reg_value = (1 << i);
        } else {
            reg_val = reg_val_B;
            reg_value = (1 << (i - 32));
        }

        if ((reg_val & reg_value) != 0) {
            gpio_pin_configure(gpio_dev, i, GPIO_DIR_IN);
        }
    }

    //GPIO under test enable pu resistance one by one
    for (i = 0; i < GPIO_MAX_PIN_NUM; i++) {
        if (i < 32) {
            reg_val = reg_val_A;
            reg_value = (1 << i);
        } else {
            reg_val = reg_val_B;
            reg_value = (1 << (i - 32));
        }

        if ((reg_val & reg_value) != 0) {
            gpio_pin_configure(gpio_dev, i, GPIO_DIR_IN | GPIO_PUD_PULL_UP);

            //wait pu resistance stable
            k_busy_wait(2000);

            gpio_pin_read(gpio_dev, i, &gpio_dat);
            if (gpio_dat != 0) {
                SYS_LOG_WRN("TEST_GPIO_OPEN\n");
                *index = i;
                return TEST_GPIO_OPEN;
            }
        }
    }

    return 0;
}

