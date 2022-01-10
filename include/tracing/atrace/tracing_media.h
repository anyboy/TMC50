/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACING_ATRACE_MEDIA_H
#define _TRACING_ATRACE_MEDIA_H

#include <kernel.h>
#include <tracing/atrace/atrace_event_media.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void sys_trace_media_record_start(void *handle)
{
	atrace_media_record_start((u32_t)handle, k_cycle_get_32());
}

static inline void sys_trace_media_track_start(void *handle)
{

	atrace_media_track_start((u32_t)handle, k_cycle_get_32());
}

#ifdef __cplusplus
}
#endif

#endif /* _TRACING_ATRACE_MEDIA_H */
