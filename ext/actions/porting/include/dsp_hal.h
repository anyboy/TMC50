/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSP_HAL_H__
#define __DSP_HAL_H__

#include <zephyr.h>
#include <misc/util.h>
#include <mem_manager.h>
#include <string.h>
#include <errno.h>
#include <dsp.h>
#include <dsp_hal_defs.h>
#include <acts_ringbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* session information to initialize a session */
struct dsp_session_info {
	/* session id */
	unsigned int session;
	/* allowed functions (bitfield) */
	unsigned int func_allowed;
	/* dsp image name */
	const char *main_dsp;
	const char *sub_dsp;
};

struct dsp_session;

struct dsp_ringbuf {
	struct acts_ringbuf buf;
	struct dsp_session *session;
};

/* dsp session ops */
/**
 * @brief open the global session
 *
 * session can be open multiple times, thus, the parameter "info" will be
 * ignored if this is not the first open.
 *
 * @param info session information
 *
 * @return Address of global session
 */
struct dsp_session *dsp_open_global_session(struct dsp_session_info *info);

/**
 * @brief close the global session
 *
 * @return N/A
 */
void dsp_close_global_session(struct dsp_session *session);

/**
 * @brief get session (task) state
 *
 * @param session Address of session
 *
 * @return the state (DSP_TASK_x) of session
 */
int dsp_session_get_state(struct dsp_session *session);

/**
 * @brief get session run error
 *
 * @param session Address of session
 *
 * @return the error code of session
 */
int dsp_session_get_error(struct dsp_session *session);

/**
 * @brief dump session information
 *
 * @param session Address of session
 *
 * @return N/A
 */
void dsp_session_dump(struct dsp_session *session);

/**
 * @brief submit session command
 *
 * @param session Address of session
 * @param command Command to submit
 *
 * @return 0 if succeed, the others if failed
 */
int dsp_session_submit_command(struct dsp_session *session, struct dsp_command *command);

/**
 * @brief submit session command without data
 *
 * @param session Address of session
 * @param command Command to submit
 *
 * @return 0 if succeed, the others if failed
 */
static inline int dsp_session_submit_simple_command(struct dsp_session *session, unsigned int id, struct k_sem *sem)
{
	struct dsp_command command = {
		.id = id,
		.sem = (uint32_t)sem,
		.size = 0,
	};

	return dsp_session_submit_command(session, &command);;
}

/**
 * @brief query session command finished
 *
 * @param session Address of session
 * @param command Command to query
 *
 * @return the query result
 */
bool dsp_session_command_finished(struct dsp_session *session, struct dsp_command *command);

/**
 * @brief kick dsp, since data ready
 *
 * @param session Address of session
 *
 * @return 0 if succeed, the others if failed
 */
int dsp_session_kick(struct dsp_session *session);

/**
 * @brief wait dsp for data output
 *
 * @param session Address of session
 * @param timout Waiting period to take the semaphore (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Wait succeed.
 * @retval -ENOTSUP Waiting not supported, since wait_sem of sessoin_info
 *                  not configured when opening session.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int dsp_session_wait(struct dsp_session *session, int timout);

/**
 * @brief Define and initialize a dsp command without data.
 *
 * The command can be accessed outside the module where it is defined using:
 *
 * @code extern struct dsp_command <name>; @endcode
 *
 * @param name Name of the command.
 * @param cmd_id Command id.
 * @param sem_obj Semaphore object for synchronization.
 */
#define DSP_COMMAND_DEFINE(name, cmd_id, sem_obj) \
	struct dsp_command name = {                   \
		.id = cmd_id,                             \
		.sem = (uint32_t)sem_obj,                 \
		.size = 0,                                \
	}

/**
 * @brief Initialize allocate and initialize a dsp command.
 *
 * This routine allocate and initializes a dsp command.
 *
 * @param id Command id.
 * @param sem Address of semaphore object for synchronization.
 * @param size Size of command data.
 * @param data Address of command data.
 *
 * @return Address of command.
 */
static inline struct dsp_command *dsp_command_alloc(unsigned int id,
				struct k_sem *sem, size_t size, const void *data)
{
	struct dsp_command *command = mem_malloc(sizeof(*command) + size);
	if (command) {
		command->id = (uint16_t)id;
		command->sem = (uint32_t)sem;
		command->size = size;
		if (data)
			memcpy(command->data, data, size);
	}

	return command;
}

/**
 * @brief Free a dsp command.
 *
 * This routine free a dsp command.
 *
 * @param command Address of command.
 *
 * @return N/A
 */
static inline void dsp_command_free(struct dsp_command *command)
{
	mem_free((void *)command);
}

/**
 * @brief Initialize a session with command DSP_CMD_SESSION_INIT
 *
 * This routine initialize a session using command DSP_CMD_SESSION_INIT.
 *
 * @param session Address of the session.
 * @param size Size of prarameters.
 * @param params Address of prarameters.
 *
 * @return Command result.
 */
static inline int dsp_session_init(struct dsp_session *session, size_t size, const void *params)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_SESSION_INIT, NULL, size, params);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Begin a session with command DSP_CMD_SESSION_BEGIN
 *
 * This routine begin a session using command DSP_CMD_SESSION_BEGIN without params.
 *
 * @param session Address of the session.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_begin(struct dsp_session *session, struct k_sem *sem)
{
	DSP_COMMAND_DEFINE(command, DSP_CMD_SESSION_BEGIN, sem);
	return dsp_session_submit_command(session, &command);
}

/**
 * @brief End a session with command DSP_CMD_SESSION_END.
 *
 * This routine end a session using command DSP_CMD_SESSION_END without params.
 *
 * @param session Address of the session.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_end(struct dsp_session *session, struct k_sem *sem)
{
	DSP_COMMAND_DEFINE(command, DSP_CMD_SESSION_END, sem);
	return dsp_session_submit_command(session, &command);
}

/**
 * @brief Configure a session function with command DSP_CMD_FUNCTION_CONFIG.
 *
 * This routine configure a function using command DSP_CMD_FUNCTION_CONFIG.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param conf Function specific configure ID.
 * @param size Size of prarameters.
 * @param params Address of prarameters.
 *
 * @return Command result.
 */
static inline int dsp_session_config_func(struct dsp_session *session,
		unsigned int func, unsigned int conf, size_t size, const void *params)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_CONFIG, NULL, size + 8, NULL);
	int res = -ENOMEM;
	if (command) {
		command->data[0] = func;
		command->data[1] = conf;
		memcpy(&command->data[2], params, size);
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Enable a session function with command DSP_CMD_FUNCTION_ENABLE.
 *
 * This routine enable a function using command DSP_CMD_FUNCTION_ENABLE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 *
 * @return Command result.
 */
static inline int dsp_session_enable_func(struct dsp_session *session, unsigned int func)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_ENABLE, NULL, 4, &func);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Disable a session function with command DSP_CMD_FUNCTION_DISABLE.
 *
 * This routine disable a function using command DSP_CMD_FUNCTION_DISABLE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 *
 * @return Command result.
 */
static inline int dsp_session_disable_func(struct dsp_session *session, unsigned int func)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_DISABLE, NULL, 4, &func);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Debug a session function with command DSP_CMD_FUNCTION_DEBUG.
 *
 * This routine debug a function using command DSP_CMD_FUNCTION_DEBUG.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param size Size of debug options.
 * @param options Address of debug options.
 *
 * @return Command result.
 */
static inline int dsp_session_debug_func(struct dsp_session *session,
		unsigned int func, size_t size, const void *options)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_DEBUG, NULL, size + 4, NULL);
	int res = -ENOMEM;
	if (command) {
		command->data[0] = func;
		memcpy(&command->data[1], options, size);
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief get samples count of specific function
 *
 * This function must export samples_count information to cpu side.
 *
 * @param session Address of session
 * @param func Session function ID
 *
 * @return samples count
 */
unsigned int dsp_session_get_samples_count(struct dsp_session *session, unsigned int func);

/**
 * @brief get lost count of specific function
 *
 * This function must export datalost_count information to cpu side.
 *
 * @param session Address of session
 * @param func Session function ID
 *
 * @return lost count
 */
unsigned int dsp_session_get_datalost_count(struct dsp_session *session, unsigned int func);

/* dsp ring buffer ops */
/**
 * @brief Initialiize a dsp ring buffer.
 *
 * This routine initialize a dsp ring buffer with external buffer backstore.
 *
 * @param session Address of session.
 * @param data Address of external buffer backstore.
 * @param size Size of ring buffer, must be mutiple of 2.
 *
 * @return Address of ring buffer
 */
struct dsp_ringbuf *dsp_ringbuf_init(struct dsp_session *session, void *data, size_t size);

/**
 * @brief Destroy a dsp ring buffer
 *
 * This routine dstroy a dsp ring buffer, which has external buffer backstore.
 *
 * @param session Address of session.
 *
 * @return Address of ring buffer
 */
void dsp_ringbuf_destroy(struct dsp_ringbuf *buf);

/* dsp ring buffer ops */
/**
 * @brief Allocate a dsp ring buffer.
 *
 * This routine allocate a dsp ring buffer.
 *
 * @param session Address of session.
 * @param size Size of ring buffer, must be mutiple of 2.
 *
 * @return Address of ring buffer
 */
struct dsp_ringbuf *dsp_ringbuf_alloc(struct dsp_session *session, size_t size);

/**
 * @brief Free a dsp ring buffer.
 *
 * This routine free a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return N/A
 */
void dsp_ringbuf_free(struct dsp_ringbuf *buf);

/**
 * @brief Peek a dsp ring buffer.
 *
 * This routine peek a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param data Address of data.
 * @param size Size of data in elements.
 *
 * @return number of bytes successfully peek.
 */
static inline size_t dsp_ringbuf_peek(struct dsp_ringbuf *buf, void *data, size_t size)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_peek(&buf->buf, data, ACTS_RINGBUF_NELEM(size)));
}

/**
 * @brief Read a dsp ring buffer.
 *
 * This routine read a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param data Address of data buffer.
 * @param size Size of data.
 *
 * @return number of bytes successfully read.
 */
size_t dsp_ringbuf_get(struct dsp_ringbuf *buf, void *data, size_t size);

/**
 * @brief Write a dsp ring buffer.
 *
 * This routine write a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param data Address of data buffer.
 * @param size Size of data..
 *
 * @return number of bytes successfully written.
 */
size_t dsp_ringbuf_put(struct dsp_ringbuf *buf, const void *data, size_t size);

/**
 * @brief Read a dsp ring buffer to stream.
 *
 * This routine read a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param stream Handle of stream.
 * @param size Size of data.
 * @param stream_write Stream write ops.
 *
 * @return number of bytes successfully read.
 */
size_t dsp_ringbuf_read(struct dsp_ringbuf *buf, void *stream, size_t size,
		ssize_t (*stream_write)(void *, const void *, size_t));

/**
 * @brief Write a dsp ring buffer from stream.
 *
 * This routine read a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param stream Handle of stream.
 * @param size Size of data.
 * @param stream_write Stream read ops.
 *
 * @return number of bytes successfully written.
 */
size_t dsp_ringbuf_write(struct dsp_ringbuf *buf, void *stream, size_t size,
		ssize_t (*stream_read)(void *, void *, size_t));

/**
 * @brief Drop data of a dsp ring buffer
 *
 * @param buf Address of ring buffer.
 * @param size Size of data.
 *
 * @return number of bytes dropped.
 */
static inline size_t dsp_ringbuf_drop(struct dsp_ringbuf *buf, size_t size)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_drop(&buf->buf, ACTS_RINGBUF_NELEM(size)));
}

/**
 * @brief Drop all data of a dsp ring buffer
 *
 * @param buf Address of ring buffer.
 * @param size Size of data.
 *
 * @return number of bytes dropped.
 */
static inline size_t dsp_ringbuf_drop_all(struct dsp_ringbuf *buf)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_drop_all(&buf->buf));
}

/**
 * @brief Fill constant data of a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param size Size of data.
 *
 * @return number of bytes filled.
 */
static inline size_t dsp_ringbuf_fill(struct dsp_ringbuf *buf, char c, size_t size)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_fill(&buf->buf, c, ACTS_RINGBUF_NELEM(size)));
}

/**
 * @brief Reset a dsp ring buffer.
 *
 * This routine reset a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return N/A.
 */
static inline void dsp_ringbuf_reset(struct dsp_ringbuf *buf)
{
	acts_ringbuf_reset(&buf->buf);
}

/**
 * @brief Determine size of a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Session buffer size.
 */
static inline size_t dsp_ringbuf_size(struct dsp_ringbuf *buf)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_size(&buf->buf));
}

/**
 * @brief Determine free space a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Session buffer free space.
 */
static inline size_t dsp_ringbuf_space(struct dsp_ringbuf *buf)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_space(&buf->buf));
}

/**
 * @brief Determine length of a dsp ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return ring buffer data length.
 */
static inline size_t dsp_ringbuf_length(struct dsp_ringbuf *buf)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_length(&buf->buf));
}

/**
 * @brief Determine if a dsp ring buffer is empty.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is empty, or 0 if not.
 */
static inline int dsp_ringbuf_is_empty(struct dsp_ringbuf *buf)
{
	return acts_ringbuf_is_empty(&buf->buf);
}

/**
 * @brief Determine if a dsp ring buffer is full.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is full, or 0 if not.
 */
static inline int dsp_ringbuf_is_full(struct dsp_ringbuf *buf)
{
	return acts_ringbuf_is_full(&buf->buf);
}

#ifdef __cplusplus
}
#endif

#endif /* __DSP_HAL_H__ */
