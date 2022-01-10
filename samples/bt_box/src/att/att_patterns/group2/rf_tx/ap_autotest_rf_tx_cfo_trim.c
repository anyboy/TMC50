/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_tx_cfo_trim.c
*/

#include "ap_autotest_rf_tx.h"

u32_t att_read_trim_cap(u32_t mode)
{
    u32_t trim_cap;
    int ret_val;

    ret_val = freq_compensation_read(&trim_cap, mode);

    if (ret_val == TRIM_CAP_READ_NO_ERROR) {
        return trim_cap;
    } else {
        extern int soc_get_hosc_cap(void);
        return soc_get_hosc_cap();
    }
}

