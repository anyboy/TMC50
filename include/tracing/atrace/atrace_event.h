/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Prevent recursion */
#ifndef TRACING_ATRACE_COMMON
#define TRACING_ATRACE_COMMON

#include <kernel.h>
#include <tracing/tracing_format.h>
#include <tracing/atrace/atrace_event_id.h>

#define __field(type, item)				type item;
#define __field_ext(type, item, filter_type)		type item;
#define __field_struct(type, item)			type item;
#define __field_struct_ext(type, item, filter_type)	type item;
#define __array(type, item, len)			type item[len];
#define __string(item, src)				__array(char, item, strlen(src) + 1)

#define PARAMS(args...)					args
#define TP_ID(id)					id
#define TP_PROTO(args...)				args
#define TP_ARGS(args...)				args
#define TP_fast_assign(args...)				args
#define TP_STRUCT__entry(args...)			args
#define TP_printk(fmt, args...)				fmt, args

struct ate_hdr {
	union {
		struct ate_id eid;
		u16_t ate_id_w;
	};

	uint8_t length;
	uint8_t thread_id;
};
#endif	/* TRACING_ATRACE_COMMON */

#ifdef CONFIG_TRACING_ATRACE

#undef DECLARE_ATRACE_EVENT
#define DECLARE_ATRACE_EVENT(name, tstruct)                                   \
    struct ate_raw_##name {                                                   \
        struct ate_hdr hdr;                                                   \
        u32_t timestamp;                                                      \
        tstruct;                                                              \
        u32_t __data[0]; /* aligned to 32bit */                               \
    }

#undef ATRACE_EVENT
#define ATRACE_EVENT(name, ate_id, proto, args, tstruct, assign, print) \
    DECLARE_ATRACE_EVENT(name, tstruct);                                      \
    static void ALWAYS_INLINE atrace_##name(proto)                            \
    {                                                                         \
        struct ate_raw_##name ate_entry;                                      \
        struct ate_raw_##name *__entry = &ate_entry;                          \
        if (ate_is_enabled(ate_id)) {                                         \
            __entry->hdr.ate_id_w = ate_id;                                   \
            __entry->hdr.length = sizeof(*__entry);                            \
            __entry->hdr.thread_id = k_current_get()->base.prio;              \
            __entry->timestamp = k_cycle_get_32();                            \
            { assign; }                                                       \
            tracing_format_raw_data((u8_t *)__entry, ROUND_UP(sizeof(*__entry), 4));       \
        }                                                                     \
    }

#else /* CONFIG_TRACING_ATRACE */

#undef DECLARE_ATRACE_EVENT
#define DECLARE_ATRACE_EVENT(name, tstruct)

#undef ATRACE_EVENT
#define ATRACE_EVENT(name, ate_id, proto, args, tstruct, assign, print) \
    static void inline atrace_##name(proto)      { }

#endif /* CONFIG_TRACING_ATRACE */
