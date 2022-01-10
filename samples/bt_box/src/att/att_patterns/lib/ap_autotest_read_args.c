/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_stub_comm.c
*/

#include "att_pattern_header.h"


u8_t unicode_to_uint8(u16_t widechar)
{
    u8_t temp_value;

    if ((widechar >= '0') && (widechar <= '9'))
    {
        temp_value = (widechar - '0');
    }
    else if ((widechar >= 'A') && (widechar <= 'F'))
    {
        temp_value = widechar + 10 - 'A';
    }
    else if ((widechar >= 'a') && (widechar <= 'f'))
    {
        temp_value = widechar + 10 - 'a';
    }
    else
    {
        return 0;
    }

    return temp_value;
}

s32_t unicode_to_int(u16_t *start, u16_t *end, u32_t base)
{
    u32_t minus_flag;
    s32_t temp_value = 0;

    minus_flag = 0;

    while (start != end)
    {
        if (*start == '-')
        {
            minus_flag = 1;
        }
        else
        {
            temp_value *= base;

            temp_value += unicode_to_uint8(*start);
        }
        start++;
    }

    if (minus_flag == 1)
    {
        temp_value = 0 - temp_value;
    }

    return temp_value;
}

s32_t unicode_encode_utf8(u8_t *s, u16_t widechar)
{
    s32_t encode_len;

    if (widechar & 0xF800)
    {
        encode_len = 3;
    }
    else if (widechar & 0xFF80)
    {
        encode_len = 2;
    }
    else
    {
        encode_len = 1;
    }

    switch (encode_len)
    {
        case 1:
        *s = (char) widechar;
        break;

        case 2:
        *s++ = 0xC0 | (widechar >> 6);
        *s = 0x80 | (widechar & 0x3F);
        break;

        case 3:
        *s++ = 0xE0 | (widechar >> 12);
        *s++ = 0x80 | ((widechar >> 6) & 0x3F);
        *s = 0x80 | (widechar & 0x3F);
        break;
    }

    return encode_len;
}

s32_t unicode_to_utf8_str(u16_t *start, u16_t *end, u8_t *utf8_buffer, u32_t utf8_buffer_len)
{
    s32_t encode_len;
    s32_t encode_total_len;

    encode_len = 0;
    encode_total_len = 0;

    while (start != end)
    {
        encode_len = unicode_encode_utf8(utf8_buffer, *start);

        start++;

        if (encode_len + encode_total_len > utf8_buffer_len)
        {
            return false;
        }

        encode_total_len += encode_len;

        utf8_buffer += encode_len;
    }

    //add endl
    *utf8_buffer = 0;

    return true;
}

s32_t unicode_to_uint8_bytes(u16_t *start, u16_t *end, u8_t *byte_buffer, u8_t byte_index, u8_t byte_len)
{
    while (start != end)
    {
        byte_buffer[byte_index] = (unicode_to_uint8(*start) << 4);

        byte_buffer[byte_index] |= unicode_to_uint8(*(start + 1));

        byte_index--;

        byte_len--;

        if (byte_len == 0)
        {
            break;
        }

        start += 2;
    }

    return true;
}


u8_t *act_test_parse_test_arg(u16_t *line_buffer, u8_t arg_number, u16_t **start, u16_t **end)
{
    u16_t *cur;
    u8_t cur_arg_num;

    cur = line_buffer;

    cur_arg_num = 0;

    while (cur_arg_num < arg_number) {
        *start = cur;

        //meet the ',', it means it's parameter; meet the 0x0d0a it means line end
        while ((*cur != 0x002c) && (*cur != 0x000d) && (*cur != 0x0000))
            cur++;

        *end = cur;
        cur_arg_num++;
        cur++;
    }

    return 0;
}

