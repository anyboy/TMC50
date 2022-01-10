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
struct recorder_srv_buff_info_t {
	struct acts_ringbuf *buf;
};

static int recorder_srv_stream_open(io_stream_t handle, stream_mode mode)
{
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	handle->mode = mode;

	return 0;
}

static int recorder_srv_stream_read(io_stream_t handle, unsigned char *buf, int len)
{
	int ret = 0;
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

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
static int acts_ringbuf_get_head_addr(struct acts_ringbuf *buf, void **data, uint32_t size)
{
	uint32_t offset, len;

	len = acts_ringbuf_length(buf);
	if (size > len)
		return -EACCES;

	offset = buf->mask ? (buf->head & buf->mask) : (buf->head % buf->size);
	*data = (void *)(buf->cpu_ptr + offset);
	buf->head += size;

	return 0;
}

int recorder_srv_get_cache_addr(io_stream_t handle, unsigned char **buf, int len)
{
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	if (acts_ringbuf_get_head_addr(info->buf, (void **)buf, len)) {
		SYS_LOG_WRN("get addr fail\n");
		return -EACCES;
	}

	handle->rofs = info->buf->head;
	handle->wofs = info->buf->tail;

	return 0;
}

static int recorder_srv_stream_write(io_stream_t handle, unsigned char *buf, int len)
{
	int ret = 0;
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

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

static int recorder_srv_stream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

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

static int recorder_srv_stream_get_length(io_stream_t handle)
{
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	return acts_ringbuf_length(info->buf);
}

static int recorder_srv_stream_get_space(io_stream_t handle)
{
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	return acts_ringbuf_space(info->buf);
}

static int recorder_srv_stream_tell(io_stream_t handle)
{
	return recorder_srv_stream_get_length(handle);
}

static int recorder_srv_stream_flush(io_stream_t handle)
{
	int len = 0;
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	len = acts_ringbuf_length(info->buf);

	return acts_ringbuf_drop(info->buf, len);
}

static int recorder_srv_stream_close(io_stream_t handle)
{
	int ret = 0;
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;


	if (!info)
		return -EACCES;

	return ret;
}

static int recorder_srv_stream_destroy(io_stream_t handle)
{
	int ret = 0;
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

	if (!info)
		return -EACCES;

	mem_free(info);
	handle->data = NULL;

	return ret;
}

static int recorder_srv_stream_init(io_stream_t handle, void *param)
{
	struct recorder_srv_buff_info_t *info = NULL;

	info = mem_malloc(sizeof(struct recorder_srv_buff_info_t));
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
	.init = recorder_srv_stream_init,
	.open = recorder_srv_stream_open,
	.read = recorder_srv_stream_read,
	.seek = recorder_srv_stream_seek,
	.tell = recorder_srv_stream_tell,
	.flush = recorder_srv_stream_flush,
	.get_length = recorder_srv_stream_get_length,
	.get_space = recorder_srv_stream_get_space,
	.write = recorder_srv_stream_write,
	.close = recorder_srv_stream_close,
	.destroy = recorder_srv_stream_destroy,
};

io_stream_t recorder_srv_stream_create(struct acts_ringbuf *param)
{
	return stream_create(&record_stream_ops, param);
}

/**ringbff stream init param */
struct recorder_srv_stream_init_param_t {
	void *ring_buff;
	u32_t ring_buff_size;
};

static int recorder_srv_stream_init_ext(io_stream_t handle, void *param)
{
	struct recorder_srv_stream_init_param_t *init_param = (struct recorder_srv_stream_init_param_t *)param;
	struct acts_ringbuf *ringbuff = (struct acts_ringbuf *)mem_malloc(sizeof(struct acts_ringbuf));

	if (!ringbuff) {
		return -ENOMEM;
	}

	acts_ringbuf_init(ringbuff, init_param->ring_buff, init_param->ring_buff_size);

	recorder_srv_stream_init(handle, ringbuff);

	return 0;
}

static int recorder_srv_stream_destroy_ext(io_stream_t handle)
{
	int ret = 0;
	struct recorder_srv_buff_info_t *info = (struct recorder_srv_buff_info_t *)handle->data;

	if (info->buf) {
		mem_free(info->buf);
	}

	recorder_srv_stream_destroy(handle);
	return ret;
}

const stream_ops_t recorder_srv_stream_ops_ext = {
	.init = recorder_srv_stream_init_ext,
	.open = recorder_srv_stream_open,
	.read = recorder_srv_stream_read,
	.seek = recorder_srv_stream_seek,
	.tell = recorder_srv_stream_tell,
	.flush = recorder_srv_stream_flush,
	.get_length = recorder_srv_stream_get_length,
	.get_space = recorder_srv_stream_get_space,
	.write = recorder_srv_stream_write,
	.close = recorder_srv_stream_close,
	.destroy = recorder_srv_stream_destroy_ext,
};

io_stream_t recorder_srv_stream_create_ext(void *ring_buff, u32_t ring_buff_size)
{
	struct recorder_srv_stream_init_param_t param = {
		.ring_buff = ring_buff,
		.ring_buff_size = ring_buff_size,
	};

	return stream_create(&recorder_srv_stream_ops_ext, &param);
}

