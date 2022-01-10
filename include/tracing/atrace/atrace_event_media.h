/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file defines for media events
 */

#ifndef __ATRACE_EVENT_MEDIA_H
#define __ATRACE_EVENT_MEDIA_H

#include <tracing/atrace/atrace_event.h>

#define ATE_MID_MEDIA_GENERAL		(0)
#define ATE_MID_MEDIA_AUDIO  		(1)

#define ATE_ID_RECORE_START         ATE_ID(ATE_MID_MEDIA, ATE_MID_MEDIA_GENERAL, 0x0)
#define ATE_ID_TRACK_START          ATE_ID(ATE_MID_MEDIA, ATE_MID_MEDIA_GENERAL, 0x1)


ATRACE_EVENT(media_record_start,
	TP_ID(ATE_ID_RECORE_START),

	TP_PROTO(u32_t handle, u32_t timestampe),

	TP_ARGS(handle, timestampe),

	TP_STRUCT__entry(
		__field(u32_t, handle)
		__field(u32_t, timestampe)
	),

	TP_fast_assign(
		__entry->handle = handle;
		__entry->timestampe = timestampe;
	),

	TP_printk("media: record start(0x%x, %d)", __entry->handle, __entry->timestampe)
);

ATRACE_EVENT(media_track_start,
	TP_ID(ATE_ID_TRACK_START),

	TP_PROTO(u32_t handle, u32_t timestampe),

	TP_ARGS(handle, timestampe),

	TP_STRUCT__entry(
		__field(u32_t, handle)
		__field(u32_t, timestampe)
	),

	TP_fast_assign(
		__entry->handle = handle;
		__entry->timestampe = timestampe;
	),

	TP_printk("media: track start(0x%x, %d)", __entry->handle, __entry->timestampe)
);


#endif /* __ATRACE_EVENT_MEDIA_H */
