/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_read_atf_file.c
*/

#include "ap_autotest.h"

/**
 *  @brief  read data from atf file
 *
 *  @param  dst_addr  : buffer address
 *  @param  dst_buffer_len : buffer size
 *  @param  offset    : offset of the atf file head
 *  @param  total_len : read bytes
 *
 *  @return  negative errno code on fail, or return read bytes
 */
int read_atf_file_by_stub(u8_t *dst_addr, u32_t dst_buffer_len, u32_t offset, s32_t total_len)
{
#define MAX_READ_LEN 200
    u8_t *stub_rw_buffer = STUB_ATT_RW_TEMP_BUFFER;
    int ret_val = 0;

    atf_file_read_t * sendData = (atf_file_read_t*) stub_rw_buffer;
    atf_file_read_ack_t * recvData = (atf_file_read_ack_t*) stub_rw_buffer;

    u32_t cur_file_offset = offset;
    s32_t left_len = total_len;

    SYS_LOG_INF("data_info -> offset:%d total_len:%d\n", offset, total_len);
    while (left_len > 0) {
        u32_t read_len = left_len;
        if (read_len >= MAX_READ_LEN)
            read_len = MAX_READ_LEN;

        //The size of the actual read load data must be aligned with four bytes, otherwise the read fails
        u32_t aligned_len = (read_len + 3) / 4 * 4;
        sendData->offset = cur_file_offset;
        sendData->readLen = aligned_len;

        //Send read command
        ret_val = att_write_data(STUB_CMD_ATT_FREAD, (sizeof(atf_file_read_t) - sizeof(stub_ext_cmd_t)),
                (u8_t*) sendData);
        if (ret_val != 0) {
            SYS_LOG_INF("att_write_data ret:%d\n", ret_val);
            ret_val = -1;
            break;
        }

        //read data and save to buffer
        ret_val = att_read_data(STUB_CMD_ATT_FREAD, aligned_len, (u8_t*) recvData);
        if (ret_val != 0) {
            SYS_LOG_INF("att_read_data ret:%d\n",ret_val);
            ret_val = -1;
            break;
        }

        memcpy(dst_addr, recvData->payload, read_len);

        dst_addr += read_len;
        left_len -= read_len;
        cur_file_offset += read_len;
    }

    if (ret_val == 0) {
		SYS_LOG_INF("read:%d bytes ok!\n", total_len);
	}

    return (ret_val < 0) ? ret_val : total_len;
}

/**
 *  @brief  read data from atf file
 *
 *  @param  dst_addr  : buffer address
 *  @param  dst_buffer_len : buffer size
 *  @param  offset    : offset of the atf file head
 *  @param  read_len  : read bytes
 *
 *  @return  negative errno code on fail, or return read bytes
 */
int read_atf_file(u8_t *dst_addr, u32_t dst_buffer_len, u32_t offset, s32_t read_len)
{
    s32_t ret = -1;

    ret = read_atf_file_by_stub(dst_addr, dst_buffer_len, offset, read_len);

    return ret;
}

/**
 *  @brief  read the attr of one sub file in atf file by sub file name
 *
 *  @param  sub_file_name : sub file name
 *  @param  sub_atf_dir   : return parameter, the attr of the sub file
 *
 *  @return  0 on success, negative errno code on fail.
 */
int atf_read_sub_file_attr(const u8_t *sub_file_name, atf_dir_t *sub_atf_dir)
{
	int ret_val;
	int i, j, cur_files;
	int read_len;
	int atf_offset = 32;
	atf_dir_t *cur_atf_dir;
	atf_dir_t *atf_dir_buffer;

    if (NULL == sub_file_name || NULL == sub_atf_dir) {
    	return -1;
    }

	atf_dir_buffer = (atf_dir_t *) app_mem_malloc(512);
	if (NULL == atf_dir_buffer) {
		return -1;
	}

	i = g_atf_head.file_total;
	while (i > 0) {
		if (i >= 16) {
			cur_files = 16;
			read_len = 512;
		} else {
			cur_files = i;
			read_len = i*32;
		}
		
		ret_val = read_atf_file((u8_t *) atf_dir_buffer, 512, atf_offset, read_len);
	    if (ret_val < read_len) {
	    	SYS_LOG_INF("read_atf_file fail\n");
	    	break;
	    }
		
		cur_atf_dir = atf_dir_buffer;
		for (j = 0; j < cur_files; j++)
	    {
	        if (strncmp(sub_file_name, cur_atf_dir->filename, ATF_MAX_SUB_FILENAME_LENGTH) == 0)
	        {
	            memcpy(sub_atf_dir, cur_atf_dir, sizeof(atf_dir_t));
	            ret_val = 0;
	            goto read_exit;
	        }
	        
	        cur_atf_dir++;
	    }
		
		atf_offset += read_len;
		i -= cur_files;
	}

	ret_val = -1;
    SYS_LOG_INF("can't find %s\n", sub_file_name);

read_exit:
	app_mem_free(atf_dir_buffer);
    return ret_val;
}

/**
 *  @brief  read data from one sub file in atf file by name
 *
 *  @param  dst_addr  : buffer address
 *  @param  dst_buffer_len : buffer size
 *  @param  sub_file_name : sub file name
 *  @param  offset    : offset of the atf file head
 *  @param  read_len  : read bytes
 *
 *  @return  negative errno code on fail, or return read bytes
 */
int read_atf_sub_file(u8_t *dst_addr, u32_t dst_buffer_len, const u8_t *sub_file_name, s32_t offset, s32_t read_len)
{
	atf_dir_t sub_atf_dir;

    int ret = atf_read_sub_file_attr(sub_file_name, &sub_atf_dir);
    if (ret != 0)
        return -1;

	SYS_LOG_INF("sub_atf_dir offset = 0x%x, len = %x, load = %x, run = %x\n",
			sub_atf_dir.offset, sub_atf_dir.length, sub_atf_dir.load_addr, sub_atf_dir.run_addr);

    if (read_len < 0)
    {
        read_len = sub_atf_dir.length - offset;
    }

    if (read_len < 0)
    {
        SYS_LOG_INF("read_len <= 0\n");
        return -1;
    }

    if (read_len > dst_buffer_len)
    {
        SYS_LOG_INF("read_len:%d > dst_buffer_len:%d\n", read_len, dst_buffer_len);
        return -1;
    }

    if (offset + read_len > sub_atf_dir.length)
    {
        SYS_LOG_INF("sub_file_name%s, offset_from_sub_file:%d + read_len:%d > sub_file_length:%d\n",
                sub_file_name, offset, read_len, sub_atf_dir.length);
        return -1;
    }

    dst_addr = (u8_t *) sub_atf_dir.load_addr;
    g_test_info.run_addr = sub_atf_dir.run_addr;

    return read_atf_file(dst_addr, dst_buffer_len, sub_atf_dir.offset + offset, read_len);
}

