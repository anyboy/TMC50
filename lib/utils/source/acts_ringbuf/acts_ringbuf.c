/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <kernel.h>
#include <soc_dsp.h>
#include <mem_manager.h>
#include <misc/util.h>
#include <acts_ringbuf.h>

int acts_ringbuf_init(struct acts_ringbuf *buf, void *data, uint32_t size)
{
	buf->head = 0;
	buf->tail = 0;
	buf->size = size;

	if (is_power_of_two(size)) {
		buf->mask = size - 1;
	} else {
		buf->mask = 0;
	}

	buf->cpu_ptr = (uint32_t)data;
	buf->dsp_ptr = mcu_to_dsp_data_address(buf->cpu_ptr);
	return 0;
}

struct acts_ringbuf *acts_ringbuf_init_ext(void *data, uint32_t size)
{
	struct acts_ringbuf *buf;

	buf = mem_malloc(sizeof(*buf));
	if (buf)
		acts_ringbuf_init(buf, data, size);

	return buf;
}

void acts_ringbuf_destroy_ext(struct acts_ringbuf *buf)
{
	if (buf)
		mem_free(buf);
}

struct acts_ringbuf *acts_ringbuf_alloc(uint32_t size)
{
	struct acts_ringbuf *buf = NULL;
	void *data = NULL;

	buf = mem_malloc(sizeof(*buf));
	if (buf == NULL)
		return NULL;

	data = mem_malloc(ACTS_RINGBUF_SIZE8(size));
	if (data == NULL) {
		mem_free(buf);
		return NULL;
	}

	acts_ringbuf_init(buf, data, size);
	return buf;
}

void acts_ringbuf_free(struct acts_ringbuf *buf)
{
	if (buf) {
		mem_free((void *)(buf->cpu_ptr));
		mem_free(buf);
	}
}

uint32_t acts_ringbuf_peek(struct acts_ringbuf *buf, void *data, uint32_t size)
{
	uint32_t offset, len;

	len = acts_ringbuf_length(buf);
	if (size > len)
		return 0;

	offset = buf->mask ? (buf->head & buf->mask) : (buf->head % buf->size);

	len = buf->size - offset;
	if (len >= size) {
		memcpy(data, (void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)), ACTS_RINGBUF_SIZE8(size));
	} else {
		memcpy(data, (void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)), ACTS_RINGBUF_SIZE8(len));
		memcpy(data + ACTS_RINGBUF_SIZE8(len), (void *)(buf->cpu_ptr), ACTS_RINGBUF_SIZE8(size - len));
	}

	return size;
}

uint32_t acts_ringbuf_get(struct acts_ringbuf *buf, void *data, uint32_t size)
{
	size = acts_ringbuf_peek(buf, data, size);
	buf->head += size;
	return size;
}

uint32_t acts_ringbuf_get_claim(struct acts_ringbuf *buf, void **data, uint32_t size)
{
	uint32_t offset = buf->mask ? (buf->head & buf->mask) : (buf->head % buf->size);
	uint32_t max_size = min_t(uint32_t, buf->size - offset, acts_ringbuf_length(buf));

	*data = (void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset));
	return (size <= max_size) ? size : max_size;
}

int acts_ringbuf_get_finish(struct acts_ringbuf *buf, uint32_t size)
{
	uint32_t offset = buf->mask ? (buf->head & buf->mask) : (buf->head % buf->size);
	uint32_t max_size = min_t(uint32_t, buf->size - offset, acts_ringbuf_length(buf));

	if (size > max_size)
		return -EINVAL;

	buf->head += size;
	return 0;
}

uint32_t acts_ringbuf_put(struct acts_ringbuf *buf, const void *data, uint32_t size)
{
	uint32_t offset, len;

	len = acts_ringbuf_space(buf);
	if (size > len)
		return 0;

	offset = buf->mask ? (buf->tail & buf->mask) : (buf->tail % buf->size);

	len = buf->size - offset;
	if (len >= size) {
		memcpy((void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)), data, ACTS_RINGBUF_SIZE8(size));
	} else {
		memcpy((void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)), data, ACTS_RINGBUF_SIZE8(len));
		memcpy((void *)(buf->cpu_ptr), data + ACTS_RINGBUF_SIZE8(len), ACTS_RINGBUF_SIZE8(size - len));
	}

	buf->tail += size;
	return size;
}

uint32_t acts_ringbuf_put_claim(struct acts_ringbuf *buf, void **data, uint32_t size)
{
	uint32_t offset = buf->mask ? (buf->tail & buf->mask) : (buf->tail % buf->size);
	uint32_t max_size = min_t(uint32_t, buf->size - offset, acts_ringbuf_space(buf));

	*data = (void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset));
	return (size <= max_size) ? size : max_size;
}

int acts_ringbuf_put_finish(struct acts_ringbuf *buf, uint32_t size)
{
	uint32_t offset = buf->mask ? (buf->tail & buf->mask) : (buf->tail % buf->size);
	uint32_t max_size = min_t(uint32_t, buf->size - offset, acts_ringbuf_space(buf));

	if (size > max_size)
		return -EINVAL;

	buf->tail += size;
	return 0;
}

uint32_t acts_ringbuf_copy(struct acts_ringbuf *dst_buf, struct acts_ringbuf *src_buf, uint32_t size)
{
	uint32_t src_length = acts_ringbuf_length(src_buf);
	uint32_t dst_space = acts_ringbuf_space(dst_buf);
	uint32_t src_offset, dst_offset;
	uint32_t len, copy_len;

	if (size > src_length)
		size = src_length;
	if (size > dst_space)
		size = dst_space;
	if (size == 0)
		return 0;

	src_offset = src_buf->mask ? (src_buf->head & src_buf->mask) : (src_buf->head % src_buf->size);
	dst_offset = dst_buf->mask ? (dst_buf->tail & dst_buf->mask) : (dst_buf->tail % dst_buf->size);

	copy_len = size;

	do {
		len = min_t(uint32_t, src_buf->size - src_offset, dst_buf->size - dst_offset);
		if (len > size)
			len = size;

		memcpy((void *)(dst_buf->cpu_ptr + ACTS_RINGBUF_SIZE8(dst_offset)),
				(void *)(src_buf->cpu_ptr + ACTS_RINGBUF_SIZE8(src_offset)),
				ACTS_RINGBUF_SIZE8(len));

		size -= len;
		if (size == 0)
			break;

		src_offset += len;
		if (src_offset >= src_buf->size)
			src_offset = 0;

		dst_offset += len;
		if (dst_offset >= dst_buf->size)
			dst_offset = 0;
	} while (1);

	src_buf->head += copy_len;
	dst_buf->tail += copy_len;

	return copy_len;
}

uint32_t acts_ringbuf_read(struct acts_ringbuf *buf,
		void *stream, uint32_t size, acts_ringbuf_write_fn stream_write)
{
	uint32_t offset, len;
	int stream_len = 0;

	len = acts_ringbuf_length(buf);
	if (size > len)
		return 0;

	offset = buf->mask ? (buf->head & buf->mask) : (buf->head % buf->size);

	len = buf->size - offset;
	if (len >= size) {
		stream_len = stream_write(stream,
				(void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)),
				ACTS_RINGBUF_SIZE8(size));
	} else {
		stream_len = stream_write(stream,
				(void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)),
				ACTS_RINGBUF_SIZE8(len));
		if (stream_len == ACTS_RINGBUF_SIZE8(len)) {
			ssize_t tmp = stream_write(stream, (void *)(buf->cpu_ptr),
									ACTS_RINGBUF_SIZE8(size - len));
			if (tmp > 0)
				stream_len += tmp;
		}
	}

	if (stream_len > 0) {
		stream_len = ACTS_RINGBUF_NELEM(stream_len);
		buf->head += stream_len;
		return stream_len;
	}

	return 0;
}

uint32_t acts_ringbuf_write(struct acts_ringbuf *buf,
		void *stream, uint32_t size, acts_ringbuf_read_fn stream_read)
{
	uint32_t offset, len;
	int stream_len = 0;

	len = acts_ringbuf_space(buf);
	if (size > len)
		return 0;

	offset = buf->mask ? (buf->tail & buf->mask) : (buf->tail % buf->size);

	len = buf->size - offset;
	if (len >= size) {
		stream_len = stream_read(stream,
				(void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)),
				ACTS_RINGBUF_SIZE8(size));
	} else {
		stream_len = stream_read(stream,
				(void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)),
				ACTS_RINGBUF_SIZE8(len));
		if (stream_len == ACTS_RINGBUF_SIZE8(len)) {
			ssize_t tmp = stream_read(stream, (void *)(buf->cpu_ptr),
									ACTS_RINGBUF_SIZE8(size - len));
			if (tmp > 0)
				stream_len += tmp;
		}
	}

	if (stream_len > 0) {
		stream_len = ACTS_RINGBUF_NELEM(stream_len);
		buf->tail += stream_len;
		return stream_len;
	}

	return 0;
}

uint32_t acts_ringbuf_drop(struct acts_ringbuf *buf, uint32_t size)
{
	uint32_t length = acts_ringbuf_length(buf);

	if (size > length)
		return 0;

	buf->head += size;
	return size;
}

uint32_t acts_ringbuf_drop_all(struct acts_ringbuf *buf)
{
	uint32_t length = acts_ringbuf_length(buf);

	buf->head += length;
	return length;
}

uint32_t acts_ringbuf_fill(struct acts_ringbuf *buf, uint8_t c, uint32_t size)
{
	uint32_t offset, len;

	len = acts_ringbuf_space(buf);
	if (size > len)
		return 0;

	offset = buf->mask ? (buf->tail & buf->mask) : (buf->tail % buf->size);

	len = buf->size - offset;
	if (len >= size) {
		memset((void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)), c, ACTS_RINGBUF_SIZE8(size));
	} else {
		memset((void *)(buf->cpu_ptr + ACTS_RINGBUF_SIZE8(offset)), c, ACTS_RINGBUF_SIZE8(len));
		memset((void *)(buf->cpu_ptr), c, ACTS_RINGBUF_SIZE8(size - len));
	}

	buf->tail += size;
	return size;
}

uint32_t acts_ringbuf_fill_none(struct acts_ringbuf *buf, uint32_t size)
{
	uint32_t space = acts_ringbuf_space(buf);

	if (size > space)
		return 0;

	buf->tail += size;
	return size;
}

void *acts_ringbuf_head_ptr(struct acts_ringbuf *buf, uint32_t *len)
{
	void *data = NULL;
	uint32_t size;

	size = acts_ringbuf_get_claim(buf, &data, acts_ringbuf_length(buf));
	if (len)
		*len = size;

	return data;
}

void *acts_ringbuf_tail_ptr(struct acts_ringbuf *buf, uint32_t *len)
{
	void *data = NULL;
	uint32_t size;

	size = acts_ringbuf_put_claim(buf, &data, acts_ringbuf_space(buf));
	if (len)
		*len = size;

	return data;
}

void acts_ringbuf_defrag(struct acts_ringbuf *buf)
{
	uint32_t roff = buf->mask ? (buf->head & buf->mask) : (buf->head % buf->size);
	uint32_t woff = buf->mask ? (buf->tail & buf->mask) : (buf->tail % buf->size);

	if (roff == 0 || acts_ringbuf_is_empty(buf))
		goto out_exit;

	if (roff >= woff) {
		buf->head = roff;
		buf->tail = woff + buf->size;
		return;
	}

	roff = ACTS_RINGBUF_SIZE8(roff);
	woff = ACTS_RINGBUF_SIZE8(woff);
	memcpy((void *)buf->cpu_ptr, (void *)(buf->cpu_ptr + roff), woff - roff);
out_exit:
	buf->tail -= buf->head;
	buf->head = 0;
}

void acts_ringbuf_dump(struct acts_ringbuf *buf, const char *name, const char *line_prefix)
{
	printk("%s%s: %p\n", line_prefix, name, buf);
	printk("%s\thead:   0x%x\n", line_prefix, buf->head);
	printk("%s\ttail:   0x%x\n", line_prefix, buf->tail);
	printk("%s\tlength: 0x%x\n", line_prefix, acts_ringbuf_length(buf));
	printk("%s\tsize:   0x%x\n", line_prefix, buf->size);
	printk("%s\tmask:   0x%x\n", line_prefix, buf->mask);
	printk("%s\tcpuptr: 0x%x\n", line_prefix, buf->cpu_ptr);
	printk("%s\tdspptr: 0x%x\n", line_prefix, buf->dsp_ptr);
}
