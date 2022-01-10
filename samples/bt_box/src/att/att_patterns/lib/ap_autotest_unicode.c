/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_unicode.c
*/

#include "att_pattern_header.h"

s32_t byte_to_unicode(u8_t byte_value, u16_t *unicode_buffer)
{
    if((byte_value >= 0) && (byte_value <= 9))
    {
        byte_value = (byte_value + '0');    
    }
    else if((byte_value >= 10) && (byte_value <= 15))
    {
        byte_value = byte_value - 10 + 'A';
    }
    else
    {
        byte_value = 0;
    }  

    *unicode_buffer = byte_value; 

    return 1;
}

s32_t two_bytes_to_unicode(u8_t byte_value, u16_t *unicode_buffer, u32_t base)
{
    u8_t temp_value;
    u32_t i;
    s32_t trans_len = 0;

    for(i = 0; i < 2; i++)
    {
        if(i == 0)
        {
            temp_value = byte_value / base;
        }
        else
        {
            temp_value = byte_value % base; 
        }

        trans_len++;
    
        byte_to_unicode(temp_value, &(unicode_buffer[i]));
    }

    return trans_len;
}

void bytes_to_unicode(u8_t *byte_buffer, u8_t byte_index, u8_t byte_len, u16_t *unicode_buffer, u16_t *unicode_len)
{
    u32_t i;

    s32_t trans_len;
    
    for(i = 0; i < byte_len; i++)
    {
        trans_len = two_bytes_to_unicode(byte_buffer[byte_index - i], unicode_buffer, 16);

        unicode_buffer += trans_len;

        *unicode_len += trans_len;
    }

    return;
}


void uint32_to_unicode(u32_t value, u16_t *unicode_buffer, u16_t *unicode_len, u32_t base)
{
    u32_t i;
    u32_t trans_byte;
    u8_t temp_data[12];
    u32_t div_val;

    if(value == 0)
    {
        *unicode_buffer = '0';
        *unicode_len += 1;
        return;
    }

    i = 0;

    trans_byte = 0;

    div_val = value;
    
    while(div_val != 0)
    {
        temp_data[i] = div_val % base;
        div_val = div_val / base;
        i++;
    }
    
    while(i != 0)
    {
        trans_byte = byte_to_unicode(temp_data[i-1], unicode_buffer);
        unicode_buffer += trans_byte;
        *unicode_len += trans_byte;
        i--;
    }

    return; 
}

void int32_to_unicode(s32_t value, u16_t *unicode_buffer, u16_t *unicode_len, u32_t base)
{
    u32_t i;
    u32_t trans_byte;
    u8_t temp_data[12];
    u32_t div_val;

    if(value == 0)
    {
        *unicode_buffer = '0';
        *unicode_len += 1;
        return;
    }

    i = 0;
    trans_byte = 0;

    if(value < 0)
    {
        *unicode_buffer = '-';
        unicode_buffer++;
        *unicode_len += 1;
        value = 0 - value;
    }

    div_val = value;
    
    while(div_val != 0)
    {
        temp_data[i] = div_val % base;
        div_val = div_val / base;
        i++;
    }
    
    while(i != 0)
    {
        trans_byte = byte_to_unicode(temp_data[i-1], unicode_buffer);
        unicode_buffer += trans_byte;
        *unicode_len += trans_byte; 
        i--;
    }

    return; 
}

s32_t utf8str_to_unicode(u8_t *utf8, u32_t utf8Len, u16_t *unicode, u16_t *unicode_len)
{
    u32_t count = 0;
    u8_t c0, c1;
    u32_t scalar;

    while (--utf8Len >= 0)
    {
        c0 = *utf8;
        utf8++;

        if (c0 < 0x80)
        {
            *unicode = c0;

            if (*unicode == 0x00)
            {
                count += 2;
                break;
            }
            unicode++;
            count += 2;
            continue;
        }

        /*not a ascii char, its encoding first byte should't bt 10xxxxxx*/
        if ((c0 & 0xc0) == 0x80)
        {
            return -1;
        }

        scalar = c0;
        if (--utf8Len < 0)
        {
            return -1;
        }

        c1 = *utf8;
        utf8++;
        /*its encoding second byte should be 10xxxxxx*/
        if ((c1 & 0xc0) != 0x80)
        {
            return -1;
        }
        scalar <<= 6;
        scalar |= (c1 & 0x3f);

        /*if r0 & 0x20 equal 0, it means  the utf encoding length is 2 bytes, otherwise it will more than 2*/
        if (!(c0 & 0x20))
        {
            if ((scalar != 0) && (scalar < 0x80))
            {
                /*unicode code less than 0x80, but its utf8 encoding length is 2 bytes, it is over encoding*/
                return -1;
            }
            *unicode = (unsigned short) scalar & 0x7ff;
            if (*unicode == 0x00)
            {
                count += 2;
                break;
            }
            unicode++;
            count += 2;
            continue;
        }

        if (--utf8Len < 0)
        {
            return -1;
        }

        c1 = *utf8;
        utf8++;
        /*the third bytes of encoding should be 10xxxxxx*/
        if ((c1 & 0xc0) != 0x80)
        {
            return -1;
        }
        scalar <<= 6;
        scalar |= (c1 & 0x3f);

        /*if r0 & 0x10 equal 0, it means the utf encoding length is 3 bytes, otherwise it will more than*/
        if (!(c0 & 0x10))
        {
            if (scalar < 0x800)
            {
                return -1;
            }
            if ((scalar >= 0xd800) && (scalar < 0xe000))
            {
                return -1;
            }
            *unicode = (unsigned short) scalar & 0xffff;
            if (*unicode == 0x00)
            {
                count += 2;
                break;
            }
            unicode++;
            count += 2;
            continue;
        }

        return -1;
    }

    //Ensure string four byte alignment
    if ((count % 4) != 0)
    {
        unicode++;
        count += 2;
        *unicode = 0x00;
    }

    *unicode_len += (count/2);
    return 0;
}

