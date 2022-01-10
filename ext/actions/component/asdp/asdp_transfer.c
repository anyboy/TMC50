/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA bluetooth backend interface
 */

#include <kernel.h>
#include <string.h>
#include <soc.h>
#include <mem_manager.h>
#include <usp_protocol.h>
#include <asdp_adaptor.h>
#include <lib_asdp.h>
#include "asdp_inner.h"

#define BUFFER_SIZE_USP 128
static u8_t usp_buffer[BUFFER_SIZE_USP];

static int asdp_port_send_data(u8_t * buf, int size)
{
	int err;
	struct asdp_ctx *ctx = asdp_get_context();

	err = WriteUSPData(&(ctx->usp), buf, size);
	if (err < 0) {
		ASDP_ERR("failed to send data, size %d, err %d", size, err);
		return -EIO;
	}

	return 0;
}

int asdp_send_cmd(u8_t srv, u8_t cmd, u16_t len)
{
	struct asdp_port_head *head;
	int err;
	struct asdp_ctx *ctx = asdp_get_context();

	head = (struct asdp_port_head *)ctx->send_buf;
	head->svc_id = srv;
	head->cmd = cmd;
	head->param_type = TLV_TYPE_MAIN;
	head->param_len = len;

	err = asdp_port_send_data(ctx->send_buf, sizeof(struct asdp_port_head) + len);
	if (err) {
		ASDP_ERR("failed to send cmd %d, err %d", cmd, err);
		return err;
	}

	return 0;
}

char *adsp_get_tlv_buf(void)
{
	struct asdp_ctx *ctx = asdp_get_context();
	return ctx->send_buf + sizeof(struct asdp_port_head);
}

int adsp_get_tlv_buf_len(void)
{
	struct asdp_ctx *ctx = asdp_get_context();
	return ctx->send_buf_size - (sizeof(struct asdp_port_head));
}

int asdp_transfer_init(usp_handle_t* usp)
{
	int ret;
	int count;

	int connect_timeout = 500;

	memset((void*)usp, 0, sizeof(usp_handle_t));

	usp->usp_protocol_global_data.protocol_type = USP_PROTOCOL_TYPE_ASDP;
	//usp->usp_protocol_global_data.protocol_type = 0;
	usp->usp_protocol_global_data.master= 0;
	usp->usp_protocol_global_data.protocol_init_flag = 0;
	usp->usp_protocol_global_data.connected = 0;
	usp->usp_protocol_global_data.opened = 0;
	usp_phy_init(&(usp->api));

	InitUSPProtocol(usp);

	count=0;
	while(connect_timeout >= 0)
	{
		ASDP_INF("To connect USP at %d trying.", count);
		if(WaitUSPConnect(usp, 100))
		{
			ASDP_INF("Connected at %d trying.", count);
			break;
		}
		else {
			ASDP_INF("Not connected at %d trying.", count);
		}
		count++;
		connect_timeout -=100;
		os_sleep(20);
	}

	if(connect_timeout < 0){
		return -2;
	}

	count=0;
	connect_timeout = 500;
	while(connect_timeout >= 0) {
		ASDP_INF("To connect asdp at %d trying.", count);
		ret = USP_Protocol_Inquiry(usp, usp_buffer, BUFFER_SIZE_USP, NULL);
		if(0 > ret) {
			ASDP_INF("ASDP is not connected at %d trying, ret=%d.", count, ret);
		} else {
			if(usp->usp_protocol_global_data.opened){
				ASDP_INF("ASDP protocol is setup at %d trying.", count);
				break;
			}else{
				ASDP_INF("Some wrong protocol happens at %d tring, ret=%d.", count, ret);
			}
		}

		connect_timeout -=100;
		count++;
		os_sleep(20);
	}

	if(connect_timeout < 0){
		return -3;
	}

	return 0;
}
