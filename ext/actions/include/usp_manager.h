/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui manager interface
 */

#ifndef __USP_MANGER_H__
#define __USP_MANGER_H__


typedef struct
{
    u8_t type;
	u8_t predefine_command;
	u16_t protocol_type : 4;
	u16_t payload_len : 12;
    u8_t payload[0];
}usp_data_t;

struct usp_recv_frame_data
{
	/** FIFO uses first 4 bytes itself, reserve space */
	int _unused;
	usp_data_t frame;
};

typedef int (*usp_protocol_handler_t)(usp_data_t *data);

void usp_manager_init(void);

int usp_manager_open(void);

int usp_manager_open_protocol(u8_t protocol);

int usp_manager_register_handler(u8_t protocol, usp_protocol_handler_t handler);

int usp_manager_send_data(u8_t *data, u32_t len);

int usp_manager_get_lost_frame(void);

#endif

