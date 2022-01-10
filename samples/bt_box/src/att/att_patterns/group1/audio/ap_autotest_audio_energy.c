/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_audio_energy.c
*/

#include "ap_autotest_audio.h"


void energy_calc(short *frame_buf, u16_t sample_cnt, short *p_cur_power_max, short *p_cur_power_rms)
{
    s32_t sample_total = 0;
    s32_t sample_value;
    s32_t sample_max = 0;
    u16_t i;

    for (i = 0; i < sample_cnt; i++)
    {
        if (frame_buf[i] < 0)
        {
            sample_value = 0 - frame_buf[i];
        }
        else
        {
            sample_value = frame_buf[i];
        }

        sample_total += sample_value;
        if (sample_value > sample_max)
        {
            sample_max = sample_value;
        }
    }

    *p_cur_power_max = (sample_max > 0x7fff) ? (0x7fff) : (sample_max);
    *p_cur_power_rms = sample_total / sample_cnt;
}

