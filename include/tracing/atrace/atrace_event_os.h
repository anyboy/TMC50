/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file defines for os events
 */

#ifndef __ATRACE_EVENT_OS_H
#define __ATRACE_EVENT_OS_H

#include <tracing/atrace/atrace_event.h>

#define ATE_MID_OS_GENERAL		(0)
#define ATE_MID_OS_TRHEAD		(1)
#define ATE_MID_OS_ISR			(2)

#define ATE_ID_OS_THREAD_SWITCHED_OUT	ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x0)
#define ATE_ID_OS_THREAD_SWITCHED_IN	ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x1)
#define ATE_ID_OS_THREAD_PRIORITY_SET	ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x2)
#define ATE_ID_OS_THREAD_CREATE		ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x3)
#define ATE_ID_OS_THREAD_ABORT		ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x4)
#define ATE_ID_OS_THREAD_SUSPEND	ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x5)
#define ATE_ID_OS_THREAD_RESUME		ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x6)
#define ATE_ID_OS_THREAD_READY		ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x7)
#define ATE_ID_OS_THREAD_PEND		ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x8)
#define ATE_ID_OS_THREAD_INFO		ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0x9)
#define ATE_ID_OS_THREAD_NAME_SET	ATE_ID(ATE_MID_OS, ATE_MID_OS_TRHEAD, 0xa)

#define ATE_ID_OS_ISR_ENTER		ATE_ID(ATE_MID_OS, ATE_MID_OS_ISR, 0x0)
#define ATE_ID_OS_ISR_EXIT		ATE_ID(ATE_MID_OS, ATE_MID_OS_ISR, 0x1)
#define ATE_ID_OS_ISR_EXIT_TO_SCHED	ATE_ID(ATE_MID_OS, ATE_MID_OS_ISR, 0x2)

ATRACE_EVENT(os_thread_switch_out,
	TP_ID(ATE_ID_OS_THREAD_SWITCHED_OUT),

	TP_PROTO(u32_t thread),

	TP_ARGS(thread),

	TP_STRUCT__entry(
		__field(u32_t, thread)
	),

	TP_fast_assign(
		__entry->thread = thread;
	),

	TP_printk("os: thread(0x%x) switch out", __entry->thread)
);

ATRACE_EVENT(os_thread_switch_in,
	TP_ID(ATE_ID_OS_THREAD_SWITCHED_IN),

	TP_PROTO(u32_t thread),

	TP_ARGS(thread),

	TP_STRUCT__entry(
		__field(u32_t, thread)
	),

	TP_fast_assign(
		__entry->thread = thread;
	),

	TP_printk("os: thread(0x%x) switch in", __entry->thread)
);

ATRACE_EVENT(os_thread_priority_set,
	TP_ID(ATE_ID_OS_THREAD_PRIORITY_SET),

	TP_PROTO(u32_t thread, u8_t prio),

	TP_ARGS(thread, prio),

	TP_STRUCT__entry(
		__field(u32_t, thread)
		__field(u8_t, prio)
	),

	TP_fast_assign(
		__entry->thread = thread;
		__entry->prio = prio;
	),

	TP_printk("os: thread (0x%x) set prio %d", __entry->thread, __entry->prio)
);

ATRACE_EVENT(os_thread_create,
	TP_ID(ATE_ID_OS_THREAD_CREATE),

	TP_PROTO(u32_t thread, u8_t prio, u32_t stack_start, u16_t stack_size),

	TP_ARGS(thread, prio, stack_start, stack_size),

	TP_STRUCT__entry(
		__field(u32_t, thread)
		__field(u32_t, stack_start)
		__field(u16_t, stack_size)
		__field(u8_t, prio)
	),

	TP_fast_assign(
		__entry->thread = thread;
		__entry->stack_start = stack_start;
		__entry->stack_size = stack_size;
		__entry->prio = prio;
	),

	TP_printk("os: thread(0x%x, %d) created, stack 0x%x size 0x%x", \
		__entry->thread, __entry->prio, __entry->stack_start, __entry->stack_size)
);

ATRACE_EVENT(os_thread_abort,
	TP_ID(ATE_ID_OS_THREAD_ABORT),

	TP_PROTO(u32_t thread),

	TP_ARGS(thread),

	TP_STRUCT__entry(
		__field(u32_t, thread)
	),

	TP_fast_assign(
		__entry->thread = thread;
	),

	TP_printk("os: thread(0x%x) abort", __entry->thread)
);

ATRACE_EVENT(os_thread_suspend,
	TP_ID(ATE_ID_OS_THREAD_SUSPEND),

	TP_PROTO(u32_t thread),

	TP_ARGS(thread),

	TP_STRUCT__entry(
		__field(u32_t, thread)
	),

	TP_fast_assign(
		__entry->thread = thread;
	),

	TP_printk("os: thread(0x%x) suspend", __entry->thread, __entry->prio)
);

ATRACE_EVENT(os_thread_resume,
	TP_ID(ATE_ID_OS_THREAD_RESUME),

	TP_PROTO(u32_t thread),

	TP_ARGS(thread),

	TP_STRUCT__entry(
		__field(u32_t, thread)
	),

	TP_fast_assign(
		__entry->thread = thread;
	),

	TP_printk("os: thread(0x%x) resume", __entry->thread)
);

ATRACE_EVENT(os_thread_ready,
	TP_ID(ATE_ID_OS_THREAD_READY),

	TP_PROTO(u32_t thread),

	TP_ARGS(thread),

	TP_STRUCT__entry(
		__field(u32_t, thread)
	),

	TP_fast_assign(
		__entry->thread = thread;
	),

	TP_printk("os: thread(0x%x, %d) ready", __entry->thread)
);

ATRACE_EVENT(os_thread_pend,
	TP_ID(ATE_ID_OS_THREAD_PEND),

	TP_PROTO(u32_t thread),

	TP_ARGS(thread),

	TP_STRUCT__entry(
		__field(u32_t, thread)
	),

	TP_fast_assign(
		__entry->thread = thread;
	),

	TP_printk("os: thread(0x%x) pend", __entry->thread)
);


ATRACE_EVENT(os_thread_info,
	TP_ID(ATE_ID_OS_THREAD_INFO),

	TP_PROTO(u32_t thread, u8_t prio, u32_t stack_start, u16_t stack_size),

	TP_ARGS(thread, prio, stack_start, stack_size),

	TP_STRUCT__entry(
		__field(u32_t, thread)
		__field(u32_t, stack_start)
		__field(u16_t, stack_size)
		__field(u8_t, prio)
	),

	TP_fast_assign(
		__entry->thread = thread;
		__entry->stack_start = stack_start;
		__entry->stack_size = stack_size;
		__entry->prio = prio;
	),

	TP_printk("os: thread(0x%x, %d) info, stack 0x%x size 0x%x", \
		__entry->thread, __entry->prio, __entry->stack_start, __entry->stack_size)
);

ATRACE_EVENT(os_thread_name_set,
	TP_ID(ATE_ID_OS_THREAD_NAME_SET),

	TP_PROTO(u32_t thread, const char *name),

	TP_ARGS(thread, name),

	TP_STRUCT__entry(
		__field(u32_t, thread)
		__array(char, name, 16)
	),

	TP_fast_assign(
		__entry->thread = thread;
		strncpy(__entry->name, name, 15);
	),

	TP_printk("os: thread(0x%x) name \'%s\'", \
		__entry->thread, __entry->name)
);

ATRACE_EVENT(os_isr_enter,
	TP_ID(ATE_ID_OS_ISR_ENTER),

	TP_PROTO(),

	TP_ARGS(),

	TP_STRUCT__entry(
	),

	TP_fast_assign(
	),

	TP_printk("os: isr enter")
);

ATRACE_EVENT(os_isr_exit,
	TP_ID(ATE_ID_OS_ISR_EXIT),

	TP_PROTO(),

	TP_ARGS(),

	TP_STRUCT__entry(
	),

	TP_fast_assign(
	),

	TP_printk("os: isr exit")
);

ATRACE_EVENT(os_isr_exit_to_sched,
	TP_ID(ATE_ID_OS_ISR_EXIT_TO_SCHED),

	TP_PROTO(),

	TP_ARGS(),

	TP_STRUCT__entry(
	),

	TP_fast_assign(
	),

	TP_printk("os: isr exit to sched")
);

#endif /* __ATRACE_EVENT_OS_THREAD_H */
