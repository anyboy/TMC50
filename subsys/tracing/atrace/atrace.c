/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <init.h>
#include <ksched.h>
#include <logging/sys_log.h>

static void send_task_list_cb(void)
{
	struct k_thread *thread;
	int prio;

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		prio = k_thread_priority_get(thread);
		SYS_LOG_DBG("send thread info: thread %p prio %d\n", thread, prio);

		sys_trace_thread_info(thread);
	}
}

int atrace_init(void)
{
	send_task_list_cb();

	return 0;
}
