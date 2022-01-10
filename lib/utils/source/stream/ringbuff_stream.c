/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file psram stream interface
 */
#define SYS_LOG_DOMAIN "ringstream"
#include <mem_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ringbuff_stream.h"
#include "stream_internal.h"

/**ringbff info */
typedef struct
{
	struct acts_ringbuf *buf;
} ringbuff_info_t;

static int ringbuff_stream_open(io_stream_t handle,stream_mode mode)
{
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if(!info)
		return -EACCES;

	handle->mode = mode;

	return 0;
}

static int ringbuff_stream_read(io_stream_t handle, unsigned char *buf, int len)
{
	int ret = 0;
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	ret = acts_ringbuf_get(info->buf, buf, len);

	if (ret != len) {
		//SYS_LOG_WRN("want read %d bytes ,but only read  %d bytes \n",len,ret);
	}

	handle->rofs = info->buf->head;
	handle->wofs = info->buf->tail;

	return ret;
}

static int ringbuff_stream_write(io_stream_t handle, unsigned char *buf, int len)
{
	int ret = 0;
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	/**fill none when buf is NULL, allow user modify write offset only*/
	if (!buf) {
		ret = acts_ringbuf_fill_none(info->buf, len);
	} else {
		ret = acts_ringbuf_put(info->buf, buf, len);
	}

	if (ret != len) {
		//SYS_LOG_WRN("want write %d bytes ,but only write %d bytes \n",len,ret);
	}
	handle->rofs = info->buf->head;
	handle->wofs = info->buf->tail;

	return ret;
}

static int ringbuff_stream_get_length(io_stream_t handle)
{
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	return acts_ringbuf_length(info->buf);
}

static int ringbuff_stream_get_space(io_stream_t handle)
{
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	return acts_ringbuf_space(info->buf);
}

static int ringbuff_stream_tell(io_stream_t handle)
{
	return ringbuff_stream_get_length(handle);
}

static int ringbuff_stream_flush(io_stream_t handle)
{
	int len = 0;
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	len = acts_ringbuf_length(info->buf);

	return acts_ringbuf_drop(info->buf, len);
}

static int ringbuff_stream_close(io_stream_t handle)
{
	int ret = 0;
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;


	if (!info)
		return -EACCES;

	return ret;
}

static int ringbuff_stream_destroy(io_stream_t handle)
{
	int ret = 0;
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	mem_free(info);
	handle->data = NULL;

	return ret;
}

static int ringbuff_stream_init(io_stream_t handle, void *param)
{
	ringbuff_info_t *info = NULL;

	info = mem_malloc(sizeof(ringbuff_info_t));
	if (!info) {
		SYS_LOG_ERR(" malloc failed \n");
		return -ENOMEM;
	}

	info->buf = (struct acts_ringbuf *)param;

	handle->data = info;

	handle->total_size = acts_ringbuf_size(info->buf);

	return 0;
}

void *ringbuff_stream_get_ringbuf(io_stream_t handle)
{
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (!info)
		return NULL;

	return (void *)(info->buf);
}

const stream_ops_t ringbuff_stream_ops = {
	.init = ringbuff_stream_init,
	.open = ringbuff_stream_open,
	.read = ringbuff_stream_read,
	.seek = NULL,
	.tell = ringbuff_stream_tell,
	.flush = ringbuff_stream_flush,
	.get_length = ringbuff_stream_get_length,
	.get_space = ringbuff_stream_get_space,
	.write = ringbuff_stream_write,
	.close = ringbuff_stream_close,
	.destroy = ringbuff_stream_destroy,
	.get_ringbuffer = ringbuff_stream_get_ringbuf,
};

io_stream_t ringbuff_stream_create(struct acts_ringbuf *param)
{
	return stream_create(&ringbuff_stream_ops, param);
}

/**ringbff stream init param */
struct ringbuff_stream_init_param_t
{
	void *ring_buff;
	u32_t ring_buff_size;
};

static int ringbuff_stream_init_ext(io_stream_t handle, void *param)
{
	struct ringbuff_stream_init_param_t *init_param = (struct ringbuff_stream_init_param_t *)param;
	struct acts_ringbuf *ringbuff = (struct acts_ringbuf *)mem_malloc(sizeof(struct acts_ringbuf));

	if (!ringbuff) {
		return -ENOMEM;
	}

	acts_ringbuf_init(ringbuff, init_param->ring_buff, init_param->ring_buff_size);

	ringbuff_stream_init(handle, ringbuff);

	return 0;
}

static int ringbuff_stream_destroy_ext(io_stream_t handle)
{
	int ret = 0;
	ringbuff_info_t *info = (ringbuff_info_t *)handle->data;

	if (info->buf) {
		mem_free(info->buf);
	}

	ringbuff_stream_destroy(handle);
	return ret;
}

const stream_ops_t ringbuff_stream_ops_ext = {
	.init = ringbuff_stream_init_ext,
	.open = ringbuff_stream_open,
	.read = ringbuff_stream_read,
	.seek = NULL,
	.tell = ringbuff_stream_tell,
	.flush = ringbuff_stream_flush,
	.get_length = ringbuff_stream_get_length,
	.get_space = ringbuff_stream_get_space,
	.write = ringbuff_stream_write,
	.close = ringbuff_stream_close,
	.destroy = ringbuff_stream_destroy_ext,
	.get_ringbuffer = ringbuff_stream_get_ringbuf,
};

io_stream_t ringbuff_stream_create_ext(void *ring_buff, u32_t ring_buff_size)
{
	struct ringbuff_stream_init_param_t param = {
		.ring_buff = ring_buff,
		.ring_buff_size = ring_buff_size,
	};

	return stream_create(&ringbuff_stream_ops_ext, &param);
}
