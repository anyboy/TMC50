/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACING_ATRACE_OS_H
#define _TRACING_ATRACE_OS_H

#include <kernel.h>
#include <tracing/atrace/atrace_event_os.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void sys_trace_thread_switched_out(void)
{
	struct k_thread *thread = k_current_get();

	atrace_os_thread_switch_out((u32_t)thread);
}

static inline void sys_trace_thread_switched_in(void)
{
	struct k_thread *thread = k_current_get();

	atrace_os_thread_switch_in((u32_t)thread);
}

static inline void sys_trace_thread_create(struct k_thread *thread)
{
	atrace_os_thread_create(
		(u32_t)(uintptr_t)thread,
		thread->base.prio,
		thread->stack_info.start,
		thread->stack_info.size
		);
}

static inline void sys_trace_thread_name_set(struct k_thread *thread, const char *name)
{
	atrace_os_thread_name_set((u32_t)(uintptr_t)thread, name);
}

static inline void sys_trace_thread_info(struct k_thread *thread)
{
	atrace_os_thread_info(
		(u32_t)(uintptr_t)thread,
		thread->base.prio,
		thread->stack_info.start,
		thread->stack_info.size
		);
}

static inline void sys_trace_thread_abort(struct k_thread *thread)
{
	atrace_os_thread_abort((u32_t)thread);
}

static inline void sys_trace_thread_suspend(struct k_thread *thread)
{
	atrace_os_thread_suspend((u32_t)thread);
}

static inline void sys_trace_thread_resume(struct k_thread *thread)
{
	atrace_os_thread_resume((u32_t)thread);
}

static inline void sys_trace_thread_ready(struct k_thread *thread)
{
	atrace_os_thread_ready((u32_t)thread);
}

static inline void sys_trace_thread_pend(struct k_thread *thread)
{
	atrace_os_thread_pend((u32_t)thread);
}

static inline void sys_trace_thread_priority_set(struct k_thread *thread)
{
	atrace_os_thread_priority_set((u32_t)thread, thread->base.prio);
	sys_trace_thread_info(thread);
}

static inline void sys_trace_isr_enter()
{
	atrace_os_isr_enter();
}

static inline void sys_trace_isr_exit()
{
	atrace_os_isr_exit();
}

static inline void sys_trace_isr_exit_to_sched()
{
	atrace_os_isr_exit_to_sched();
}

#ifdef __cplusplus
}
#endif

#endif /* _TRACING_ATRACE_OS_H */
