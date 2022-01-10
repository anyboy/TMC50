/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <misc/util.h>
#include <logging/sys_log.h>
#include <mem_manager.h>
#include <soc_dsp.h>
#include <dsp.h>
#include "dsp_inner.h"

extern const struct dsp_imageinfo *dsp_create_image(const char *name);
extern void dsp_free_image(const struct dsp_imageinfo *image);

static struct dsp_session *global_session = NULL;
static unsigned int global_uuid = 0;

int dsp_session_get_state(struct dsp_session *session)
{
	int state = 0;
	dsp_request_userinfo(session->dev, DSP_REQUEST_TASK_STATE, &state);
	return state;
}

int dsp_session_get_error(struct dsp_session *session)
{
	int error = 0;
	dsp_request_userinfo(session->dev, DSP_REQUEST_ERROR_CODE, &error);
	return error;
}

void dsp_session_dump(struct dsp_session *session)
{
	struct dsp_request_session request;

	if (!session) {
		/* implicitly point to global session */
		session = global_session;
		if (!session)
			return;
	}

	dsp_request_userinfo(session->dev, DSP_REQUEST_SESSION_INFO, &request);

	printk("\n---------------------------dsp dump---------------------------\n");
	printk("session (id=%u, uuid=%u):\n", session->id, session->uuid);
	printk("\tstate=%d\n", dsp_session_get_state(session));
	printk("\terror=0x%x\n", dsp_session_get_error(session));
	printk("\tfunc_allowed=0x%x\n", session->func_allowed);
	printk("\tfunc_enabled=0x%x\n", request.func_enabled);
	printk("\tfunc_counter=0x%x\n", request.func_counter);

	if (request.info)
		dsp_session_dump_info(session, request.info);
	if (request.func_enabled) {
		for (int i = 0; i < DSP_NUM_FUNCTIONS; i++) {
			if (request.func_enabled & DSP_FUNC_BIT(i))
				dsp_session_dump_function(session, i);
		}
	}
	printk("--------------------------------------------------------------\n\n");
}

static int dsp_session_set_id(struct dsp_session *session)
{
	struct dsp_session_id_set set = {
		.id = session->id,
		.uuid = session->uuid,
		.func_allowed = session->func_allowed,
	};

	struct dsp_command *command =
		dsp_command_alloc(DSP_CMD_SET_SESSION, NULL, sizeof(set), &set);
	if (command == NULL)
		return -ENOMEM;

	dsp_session_submit_command(session, command);
	dsp_command_free(command);
	return 0;
}

static inline int dsp_session_get(struct dsp_session *session)
{
	return atomic_inc(&session->ref_count);
}

static inline int dsp_session_put(struct dsp_session *session)
{
	return atomic_dec(&session->ref_count);
}

static void dsp_session_destroy(struct dsp_session *session);
static struct dsp_session *dsp_session_create(struct dsp_session_info *info)
{
	struct dsp_session *session = NULL;
	struct acts_ringbuf *ringbuf = NULL;

	if (info == NULL || info->main_dsp == NULL)
		return NULL;

	session = mem_malloc(sizeof(*session));
	if (session == NULL)
		return NULL;

	memset(session, 0, sizeof(*session));

	/* find dsp device */
	session->dev = device_get_binding(CONFIG_DSP_ACTS_DEV_NAME);
	if (session->dev == NULL) {
		SYS_LOG_ERR("cannot find device \"%s\"", CONFIG_DSP_ACTS_DEV_NAME);
		goto fail_exit;
	}

	/* find images */
	session->images[DSP_IMAGE_MAIN] = dsp_create_image(info->main_dsp);
	if (!session->images[DSP_IMAGE_MAIN]) {
		goto fail_exit;
	}

	if (info->sub_dsp) {
		session->images[DSP_IMAGE_SUB] = dsp_create_image(info->sub_dsp);
		if (!session->images[DSP_IMAGE_SUB]) {
			goto fail_exit;
		}
	}

	/* allocate command buffer */
	ringbuf = acts_ringbuf_alloc(DSP_SESSION_COMMAND_BUFFER_SIZE);
	if (ringbuf == NULL) {
		SYS_LOG_ERR("command buffer alloc failed");
		goto fail_exit;
	}

	session->cmdbuf.cur_seq = 0;
	session->cmdbuf.alloc_seq = 1;
	session->cmdbuf.cpu_bufptr = (uint32_t)ringbuf;
	session->cmdbuf.dsp_bufptr = mcu_to_dsp_data_address(session->cmdbuf.cpu_bufptr);

	k_sem_init(&session->sem, 0, 1);
	k_mutex_init(&session->mutex);
	atomic_set(&session->ref_count, 0);
	session->id = info->session;
	session->uuid = ++global_uuid;
	SYS_LOG_INF("session %u created (uuid=%u)", session->id, session->uuid);
	return session;
fail_exit:
	dsp_session_destroy(session);
	return NULL;
}

static void dsp_session_destroy(struct dsp_session *session)
{
	if (atomic_get(&session->ref_count) != 0)
		SYS_LOG_ERR("session not closed yet (ref=%d)", atomic_get(&session->ref_count));

	for (int i = 0; i < ARRAY_SIZE(session->images); i++) {
		if (session->images[i])
			dsp_free_image(session->images[i]);
	}

	if (session->cmdbuf.cpu_bufptr)
		acts_ringbuf_free((struct acts_ringbuf *)(session->cmdbuf.cpu_bufptr));

	SYS_LOG_INF("session %u destroyed (uuid=%u)", session->id, session->uuid);
	mem_free(session);
}

static int dsp_session_message_handler(struct dsp_message *message)
{
	if (!global_session || message->id != DSP_MSG_KICK)
		return -ENOTTY;

	switch (message->param1) {
	case DSP_EVENT_FENCE_SYNC:
		if (message->param2)
			k_sem_give((struct k_sem *)message->param2);
		break;
	case DSP_EVENT_NEW_CMD:
	case DSP_EVENT_NEW_DATA:
		k_sem_give(&global_session->sem);
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static int dsp_session_open(struct dsp_session *session, struct dsp_session_info *info)
{
	int res = 0;

	if (dsp_session_get(session) != 0)
		return -EALREADY;

	if (info == NULL) {
		res = -EINVAL;
		goto fail_out;
	}

	/* load main dsp image */
	res = dsp_request_image(session->dev, session->images[DSP_IMAGE_MAIN], DSP_IMAGE_MAIN);
	if (res) {
		SYS_LOG_ERR("failed to load main image \"%s\"", session->images[DSP_IMAGE_MAIN]->name);
		goto fail_out;
	}

	/* load sub dsp image */
	if (session->images[DSP_IMAGE_SUB]) {
		res = dsp_request_image(session->dev, session->images[DSP_IMAGE_SUB], DSP_IMAGE_SUB);
		if (res) {
			SYS_LOG_ERR("failed to load sub image \"%s\"", session->images[DSP_IMAGE_SUB]->name);
			goto fail_release_main;
		}
	}

	/* reset and register command buffer */
	session->cmdbuf.cur_seq = 0;
	session->cmdbuf.alloc_seq = 1;
	acts_ringbuf_reset((struct acts_ringbuf *)(session->cmdbuf.cpu_bufptr));

	/* power on */
	res = dsp_poweron(session->dev, &session->cmdbuf);
	if (res) {
		SYS_LOG_ERR("dsp_poweron failed");
		goto fail_release_sub;
	}

	/* configure session id to dsp */
	session->func_allowed = info->func_allowed;
	res = dsp_session_set_id(session);
	if (res) {
		SYS_LOG_ERR("dsp_session_set_id failed");
		dsp_poweroff(session->dev);
		goto fail_release_sub;
	}

	/* register message handler only after everything OK */
	k_sem_reset(&session->sem);
	dsp_register_message_handler(session->dev, dsp_session_message_handler);
	SYS_LOG_INF("session %u opened (uuid=%u, allowed=0x%x)",
			session->id, session->uuid, info->func_allowed);
	return 0;
fail_release_sub:
	if (session->images[DSP_IMAGE_SUB])
		dsp_release_image(session->dev, DSP_IMAGE_SUB);
fail_release_main:
	dsp_release_image(session->dev, DSP_IMAGE_MAIN);
fail_out:
	dsp_session_put(session);
	return res;
}

static int dsp_session_close(struct dsp_session *session)
{
	if (dsp_session_put(session) != 1)
		return -EBUSY;

	if (session->cmdbuf.cur_seq + 1 != session->cmdbuf.alloc_seq)
		SYS_LOG_WRN("session %u command not finished", session->id);

	dsp_unregister_message_handler(session->dev);
	dsp_poweroff(session->dev);

	for (int i = 0; i < ARRAY_SIZE(session->images); i++) {
		if (session->images[i])
			dsp_release_image(session->dev, i);
	}

	SYS_LOG_INF("session %u closed (uuid=%u)", session->id, session->uuid);
	return 0;
}

struct dsp_session *dsp_open_global_session(struct dsp_session_info *info)
{
	int res;

	if (global_session == NULL) {
		global_session = dsp_session_create(info);
		if (global_session == NULL) {
			SYS_LOG_ERR("failed to create global session");
			return NULL;
		}
	}

	res = dsp_session_open(global_session, info);
	if (res < 0 && res != -EALREADY) {
		SYS_LOG_ERR("failed to open global session");
		dsp_session_destroy(global_session);
		global_session = NULL;
	}

	return global_session;
}

void dsp_close_global_session(struct dsp_session *session)
{
	assert(session == global_session);

	if (global_session) {
		if (!dsp_session_close(global_session)) {
			dsp_session_destroy(global_session);
			global_session = NULL;
		}
	}
}

int dsp_session_wait(struct dsp_session *session, int timeout)
{
	return k_sem_take(&session->sem, timeout);
}

int dsp_session_kick(struct dsp_session *session)
{
	return dsp_kick(session->dev, session->uuid, DSP_EVENT_NEW_DATA, 0);
}

int dsp_session_submit_command(struct dsp_session *session, struct dsp_command *command)
{
	struct acts_ringbuf *buf = (struct acts_ringbuf *)(session->cmdbuf.cpu_bufptr);
	unsigned int size = sizeof_dsp_command(command);
	unsigned int space;
	int res = -ENOMEM;

	k_mutex_lock(&session->mutex, K_FOREVER);

	space = acts_ringbuf_space(buf);
	if (space < ACTS_RINGBUF_NELEM(size)) {
		SYS_LOG_ERR("No enough space (%u) for command (id=0x%04x, size=%u)",
				space, command->id, size);
		goto out_unlock;
	}

	/* alloc sequence number */
	command->seq = session->cmdbuf.alloc_seq++;
	/* insert command */
	acts_ringbuf_put(buf, command, ACTS_RINGBUF_NELEM(size));
	/* kick dsp to process command */
	dsp_kick(session->dev, session->uuid, DSP_EVENT_NEW_CMD, 0);
	res = 0;
out_unlock:
	k_mutex_unlock(&session->mutex);
	return res;
}

bool dsp_session_command_finished(struct dsp_session *session, struct dsp_command *command)
{
	return command->seq <= session->cmdbuf.cur_seq;
}
