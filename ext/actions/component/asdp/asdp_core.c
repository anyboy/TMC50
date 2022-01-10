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
#include <init.h>
#include <mem_manager.h>
#include <lib_asdp.h>
#include <usp_protocol.h>
#include "asdp_inner.h"
#include "asdp_transfer.h"

static struct asdp_ctx g_asdp_ctx;
static struct _k_thread_stack_element __aligned(STACK_ALIGN) asdp_stack[0x800];

struct asdp_ctx *asdp_get_context(void)
{
	return &g_asdp_ctx;
}

int asdp_register_service_handle(const struct asdp_port_srv *srv_proc)
{
	int i;

	struct asdp_ctx *ctx = asdp_get_context();

	os_mutex_lock(&ctx->mutex, OS_FOREVER);

	for (i = 0; i < MAX_ASDP_SERVICE_NUM; i++) {
		if (ctx->cmd_proc[i].srv_id == srv_proc->srv_id) {
			return 0;
		}
	}

	for (i = 0; i < MAX_ASDP_SERVICE_NUM; i++) {
		if (ctx->cmd_proc[i].srv_id == 0) {
			memcpy(&ctx->cmd_proc[i], srv_proc,
			       sizeof(struct asdp_port_srv));
			break;
		}
	}
	SYS_LOG_INF("i:%d, srv_id:%d", i, ctx->cmd_proc[i].srv_id);
	os_mutex_unlock(&ctx->mutex);

	return 0;
}

int asdp_unregister_service_handle(u8_t service_id)
{
	int i;

	struct asdp_ctx *ctx = asdp_get_context();

	os_mutex_lock(&ctx->mutex, OS_FOREVER);

	for (i = 0; i < MAX_ASDP_SERVICE_NUM; i++) {
		if (ctx->cmd_proc[i].srv_id == service_id) {
			memset(&ctx->cmd_proc[i], 0,
			       sizeof(struct asdp_port_srv));
			break;
		}
	}

	os_mutex_unlock(&ctx->mutex);

	return 0;
}

int asdp_process_command(struct asdp_ctx *ctx)
{
	struct asdp_port_head* head;
	struct asdp_port_srv *cmd_proc;
	int i, err;
	int ret;

	//ret = ReceiveUSPDataPacket(&ctx->usp, NULL, ctx->receive_buf, ASDP_RECEIVE_PACKAGE_MAX_SIZE);
	ret = USP_Protocol_Inquiry(&ctx->usp, ctx->receive_buf, ASDP_RECEIVE_PACKAGE_MAX_SIZE, NULL);
	if(ret <= 0){
		return 0;
	}
	if (ret < sizeof(struct asdp_port_head)) {
		ASDP_ERR("cannot get command, ret=%d.", ret);
		return 0;
	}

	head = (struct asdp_port_head *)(ctx->receive_buf);

	ASDP_DBG("ASDP head: service=%d, command=0x%x, type=0x%x, len=%d",
			head->svc_id,
			head->cmd,
			head->param_type,
			head->param_len
			);


	if (head->param_type != TLV_TYPE_MAIN) {
		ASDP_ERR("invalid param type: %d", head->param_type);
		return -EIO;
	}

	if ((head->param_len + sizeof(struct asdp_port_head)) != ret) {
		ASDP_ERR("invalid param len. len=%d, read=%d", head->param_len, ret);
		return -EIO;
	}

	for (i = 0; i < MAX_ASDP_SERVICE_NUM; i++) {
		if (head->svc_id == ctx->cmd_proc[i].srv_id) {
			break;
		}
	}

	if (i == MAX_ASDP_SERVICE_NUM) {
		ASDP_ERR("invalid service id: %d", head->svc_id);
		return -EIO;
	}


	cmd_proc = &ctx->cmd_proc[i];
	err = cmd_proc->handler(ctx->receive_buf, sizeof(struct asdp_port_head) + head->param_len);
	if (err) {
		ASDP_ERR("cmd_handler %p, err: %d", cmd_proc, err);
		return err;
	}

	return err;
}


int asdp_service_init(struct device *dev)
{
	ARG_UNUSED(dev);

	struct asdp_ctx *ctx = asdp_get_context();

	memset(ctx, 0, sizeof(struct asdp_ctx));

	ctx->send_buf_size = ASDP_SEND_BUFFER_SIZE;

	ctx->send_buf = mem_malloc(ctx->send_buf_size);

	if (!ctx->send_buf) {
		ASDP_ERR("malloc failed");
		return -ENOMEM;
	}

	os_mutex_init(&ctx->mutex);

	//Not initialized as asdp is not connected.
	ctx->init = false;

	asdp_service_open();
	return 0;
}

static int asdp_port_init(struct asdp_ctx *ctx)
{
	return asdp_transfer_init(&(ctx->usp));
}

static void asdp_port_loop(void *parama1, void *parama2, void *parama3)
{
	struct asdp_ctx *ctx = asdp_get_context();
	int count = 0;

	while (1) {
		if (ctx->thread_terminal) {
			break;
		}

		if (ctx->init) {
			os_mutex_lock(&ctx->mutex, OS_FOREVER);
			asdp_process_command(ctx);
			os_mutex_unlock(&ctx->mutex);
			os_sleep(10);
		} else {
			ASDP_DBG("To init asdp, times=%d.", count);
			if (asdp_port_init(ctx) == 0) {
				ctx->init = true;
				ASDP_INF("asdp init at %d.", count);
			} else {
				ASDP_INF("asdp init unsuccessfully at %d.", count);
				os_sleep(10);
			}
		}

		count++;
	}

	ctx->thread_terminaled = 1;
}

int asdp_service_open(void)
{
	struct asdp_ctx *ctx = asdp_get_context();

	if (!ctx->port_thread) {
		ctx->port_thread = (os_tid_t) os_thread_create((char*)asdp_stack,
							       sizeof
							       (asdp_stack),
							       asdp_port_loop,
							       NULL, NULL, NULL,
							       CONFIG_APP_PRIORITY,
							       0,
							       OS_NO_WAIT);
	}

	return 0;
}

int adsp_service_close(void)
{
	struct asdp_ctx *ctx = asdp_get_context();

	if (ctx->port_thread) {
		ctx->thread_terminaled = 0;
		ctx->thread_terminal = 1;

		while (!ctx->thread_terminaled) {
			os_sleep(1);
		}

		ctx->port_thread = NULL;
	}

	return 0;
}

SYS_INIT(asdp_service_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

