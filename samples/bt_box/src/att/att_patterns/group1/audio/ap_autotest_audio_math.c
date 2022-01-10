/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_audio_math.c
*/

#include "ap_autotest_audio.h"


#define EE          364841611           //Q27
#define INV_EE      49375942            //Q27

#define MUL32_32(a,b)      (long long)((long long)(a)*(long long)(b))
#define MUL32_32_RS15(a,b) (int)(((long long)(a)*(long long)(b))>>15)
#define MUL32_32_RS(a,b,c) (int)(((long long)(a)*(long long)(b))>>(c))

const int exp_taylor_coeff[6] =
{ 134217728, 67108864, 22369621, 5592405, 1118481, 186413 };

/*db unit is 0.1db*/
s32_t math_exp_fixed(s32_t db)
{
    int i;
    int temp = 32768;
    long long sum = 4398046511104L;
    int xin;
    int result;
    int count = 0;

    xin = (db * 75451) / 200;

    while (xin < -32768)
    {
        xin += 32768;
        count++;
    }

    while (xin > 32768)
    {
        xin -= 32768;
        count++;
    }

    for (i = 0; i < 6; i++)
    {
        temp = MUL32_32_RS15(temp, xin);
        sum += MUL32_32(temp, exp_taylor_coeff[i]);
    }

    result = (int) (sum >> 27);

    if (count > 0)
    {
        if (db < 0)
        {
            for (i = 0; i < count; i++)
            {
                result = MUL32_32_RS(result, INV_EE, 27);
            }
        }
        else
        {
            for (i = 0; i < count; i++)
            {
                result = MUL32_32_RS(result, EE, 27);
            }
        }
    }

    return result;
}


s32_t math_calc_db(s32_t value)
{
    s32_t i;

    for (i = -600; i <= 0; i++)
    {
        if (math_exp_fixed(i) >= value)
        {
            return i;
        }
    }

    return -600;
}

