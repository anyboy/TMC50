/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file record app stream
 */

#include <mem_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#include <acts_ringbuf.h>

/**ringbff info */
struct recordbuff_info_t {
	struct acts_ringbuf *buf;
};

static int record_stream_open(io_stream_t handle, stream_mode mode)
{
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	handle->mode = mode;

	return 0;
}

static int record_stream_read(io_stream_t handle, unsigned char *buf, int len)
{
	int ret = 0;
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	ret = acts_ringbuf_get(info->buf, buf, len);

	if (ret != len) {
		SYS_LOG_WRN("want read %d bytes ,but only read  %d bytes\n", len, ret);
	}

	handle->rofs = info->buf->head;
	handle->wofs = info->buf->tail;

	return ret;
}

static int record_stream_write(io_stream_t handle, unsigned char *buf, int len)
{
	int ret = 0;
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	if (handle->write_finished) {
		SYS_LOG_INF("finish\n");
		return ret;
	}

	ret = acts_ringbuf_put(info->buf, buf, len);

	if (ret != len) {
		SYS_LOG_WRN("want write %d bytes ,but only write %d bytes\n", len, ret);
	}
	handle->rofs = info->buf->head;
	handle->wofs = info->buf->tail;

	return ret;
}

static int record_stream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	switch (origin) {
	case SEEK_DIR_BEG:
		if (offset == 0)
		handle->write_finished = 1;
		break;
	case SEEK_DIR_CUR:
		break;
	case SEEK_DIR_END:
		break;
	default:
		SYS_LOG_ERR("mode not support 0x%x\n", origin);
		return -1;
	}

	return 0;
}

static int record_stream_get_length(io_stream_t handle)
{
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	return acts_ringbuf_length(info->buf);
}

static int record_stream_get_space(io_stream_t handle)
{
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	return acts_ringbuf_space(info->buf);
}

static int record_stream_tell(io_stream_t handle)
{
	return record_stream_get_length(handle);
}

static int record_stream_flush(io_stream_t handle)
{
	int len = 0;
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	len = acts_ringbuf_length(info->buf);

	return acts_ringbuf_drop(info->buf, len);
}

static int record_stream_close(io_stream_t handle)
{
	int ret = 0;
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;


	if (!info)
		return -EACCES;

	return ret;
}

static int record_stream_destroy(io_stream_t handle)
{
	int ret = 0;
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	mem_free(info);
	handle->data = NULL;

	return ret;
}

static int record_stream_init(io_stream_t handle, void *param)
{
	struct recordbuff_info_t *info = NULL;

	info = mem_malloc(sizeof(struct recordbuff_info_t));
	if (!info) {
		SYS_LOG_ERR(" malloc failed\n");
		return -ENOMEM;
	}

	info->buf = (struct acts_ringbuf *)param;

	handle->data = info;

	handle->total_size = acts_ringbuf_size(info->buf);
	handle->write_finished = 0;

	return 0;
}

const stream_ops_t record_stream_ops = {
	.init = record_stream_init,
	.open = record_stream_open,
	.read = record_stream_read,
	.seek = record_stream_seek,
	.tell = record_stream_tell,
	.flush = record_stream_flush,
	.get_length = record_stream_get_length,
	.get_space = record_stream_get_space,
	.write = record_stream_write,
	.close = record_stream_close,
	.destroy = record_stream_destroy,
};

io_stream_t record_stream_create(struct acts_ringbuf *param)
{
	return stream_create(&record_stream_ops, param);
}

/**ringbff stream init param */
struct record_stream_init_param_t {
	void *ring_buff;
	u32_t ring_buff_size;
};

static int record_stream_init_ext(io_stream_t handle, void *param)
{
	struct record_stream_init_param_t *init_param = (struct record_stream_init_param_t *)param;
	struct acts_ringbuf *ringbuff = (struct acts_ringbuf *)mem_malloc(sizeof(struct acts_ringbuf));

	if (!ringbuff) {
		return -ENOMEM;
	}

	acts_ringbuf_init(ringbuff, init_param->ring_buff, init_param->ring_buff_size);

	record_stream_init(handle, ringbuff);

	return 0;
}

static int record_stream_destroy_ext(io_stream_t handle)
{
	int ret = 0;
	struct recordbuff_info_t *info = (struct recordbuff_info_t *)handle->data;

	if (info->buf) {
		mem_free(info->buf);
	}

	record_stream_destroy(handle);
	return ret;
}

const stream_ops_t record_stream_ops_ext = {
	.init = record_stream_init_ext,
	.open = record_stream_open,
	.read = record_stream_read,
	.seek = record_stream_seek,
	.tell = record_stream_tell,
	.flush = record_stream_flush,
	.get_length = record_stream_get_length,
	.get_space = record_stream_get_space,
	.write = record_stream_write,
	.close = record_stream_close,
	.destroy = record_stream_destroy_ext,
};

io_stream_t record_stream_create_ext(void *ring_buff, u32_t ring_buff_size)
{
	struct record_stream_init_param_t param = {
		.ring_buff = ring_buff,
		.ring_buff_size = ring_buff_size,
	};

	return stream_create(&record_stream_ops_ext, &param);
}

