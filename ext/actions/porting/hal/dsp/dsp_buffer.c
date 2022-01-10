#include <logging/sys_log.h>
#include <mem_manager.h>
#include "dsp_inner.h"

struct dsp_ringbuf *dsp_ringbuf_init(struct dsp_session *session, void *data,
				     size_t size)
{
	struct dsp_ringbuf *buf = mem_malloc(sizeof(*buf));
	if (buf == NULL)
		return NULL;

	acts_ringbuf_init(&buf->buf, data, ACTS_RINGBUF_NELEM(size));
	buf->session = session;
	return buf;
}

void dsp_ringbuf_destroy(struct dsp_ringbuf *buf)
{
	if (buf)
		mem_free(buf);
}

struct dsp_ringbuf *dsp_ringbuf_alloc(struct dsp_session *session, size_t size)
{
	struct dsp_ringbuf *buf = mem_malloc(sizeof(*buf));
	if (buf == NULL)
		return NULL;

	void *data = mem_malloc(size);
	if (data == NULL) {
		mem_free(buf);
		return NULL;
	}

	acts_ringbuf_init(&buf->buf, data, ACTS_RINGBUF_NELEM(size));
	buf->session = session;
	return buf;
}

void dsp_ringbuf_free(struct dsp_ringbuf *buf)
{
	if (buf) {
		mem_free((void *)(buf->buf.cpu_ptr));
		mem_free(buf);
	}
}

size_t dsp_ringbuf_get(struct dsp_ringbuf *buf, void *data, size_t size)
{
	size_t len = ACTS_RINGBUF_SIZE8(acts_ringbuf_get(&buf->buf, data, ACTS_RINGBUF_NELEM(size)));

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	/* Supposed next read the same size */
	//if (!k_is_in_isr() && dsp_ringbuf_length(buf) < size) {
	//	if (!dsp_session_kick(buf->session))
	//		SYS_LOG_DBG("kick %u (%u ms)", dsp_ringbuf_length(buf), k_uptime_get_32());
	//}
#endif

	return len;
}

size_t dsp_ringbuf_put(struct dsp_ringbuf *buf, const void *data, size_t size)
{
	size_t len = ACTS_RINGBUF_SIZE8(acts_ringbuf_put(&buf->buf, data, ACTS_RINGBUF_NELEM(size)));

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	/* Supposed next write the same size */
	if (!k_is_in_isr() && (empty || dsp_ringbuf_space(buf) < size)) {
		if (!dsp_session_kick(buf->session))
			SYS_LOG_DBG("kick %u (%u ms)", dsp_ringbuf_space(buf), k_uptime_get_32());
	}
#endif

	return len;
}

size_t dsp_ringbuf_read(struct dsp_ringbuf *buf, void *stream, size_t size,
			ssize_t (*stream_write)(void *, const void *, size_t))
{
	size_t len = ACTS_RINGBUF_SIZE8(
		acts_ringbuf_read(&buf->buf, stream, ACTS_RINGBUF_NELEM(size),
				  (acts_ringbuf_write_fn)stream_write));

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	/* Supposed next read the same size */
	//if (!k_is_in_isr() && dsp_ringbuf_length(buf) < size) {
	//	if (!dsp_session_kick(buf->session))
	//		SYS_LOG_DBG("kick %u (%u ms)", dsp_ringbuf_length(buf), k_uptime_get_32());
	//}
#endif

	return len;
}

size_t dsp_ringbuf_write(struct dsp_ringbuf *buf, void *stream, size_t size,
			 ssize_t (*stream_read)(void *, void *, size_t))
{
	size_t len = ACTS_RINGBUF_SIZE8(
		acts_ringbuf_write(&buf->buf, stream, ACTS_RINGBUF_NELEM(size),
				   (acts_ringbuf_read_fn)stream_read));

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	/* Supposed next write the same size */
	if (!k_is_in_isr() && (empty || dsp_ringbuf_space(buf) < size)) {
		if (!dsp_session_kick(buf->session))
			SYS_LOG_DBG("kick %u (%u ms)", dsp_ringbuf_space(buf), k_uptime_get_32());
	}
#endif

	return len;
}
