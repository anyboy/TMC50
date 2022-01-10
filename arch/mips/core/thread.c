/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <wait_q.h>
#include <string.h>
#include "mips32_cp0.h"
#include <tracing/tracing.h>

static __attribute__((nomips16)) uint32_t get_gp_reg(void)
{
	uint32_t gp_val;

	__asm__ volatile("addi   %0, $28, 0" : "=r"(gp_val));

	return gp_val;
}

void _new_thread(struct k_thread *thread, k_thread_stack_t stack,
		 size_t stack_size, _thread_entry_t thread_func,
		 void *arg1, void *arg2, void *arg3,
		 int priority, unsigned int options)
{
	char *stack_memory = K_THREAD_STACK_BUFFER(stack);
	struct __esf *initctx;

	_ASSERT_VALID_PRIO(priority, thread_func);

	_new_thread_init(thread, stack_memory, stack_size, priority, options);

	/* Initial stack frame data, stored at the base of the stack */
	initctx = (struct __esf *)
		STACK_ROUND_DOWN(stack_memory + stack_size - sizeof(*initctx));

#ifdef CONFIG_INIT_STACKS
	initctx->at = (u32_t)0x01010101;
	initctx->v0 = (u32_t)0x02020202;
	initctx->v1 = (u32_t)0x03030303;
	initctx->a0 = (u32_t)0x04040404;
	initctx->a1 = (u32_t)0x05050505;
	initctx->a2 = (u32_t)0x06060606;
	initctx->a3 = (u32_t)0x07070707;
	initctx->t0 = (u32_t)0x08080808;
	initctx->t1 = (u32_t)0x09090909;
	initctx->t2 = (u32_t)0x10101010;
	initctx->t3 = (u32_t)0x11111111;
	initctx->t4 = (u32_t)0x12121212;
	initctx->t5 = (u32_t)0x13131313;
	initctx->t6 = (u32_t)0x14141414;
	initctx->t7 = (u32_t)0x15151515;
	initctx->s0 = (u32_t)0x16161616;
	initctx->s1 = (u32_t)0x17171717;
	initctx->s2 = (u32_t)0x18181818;
	initctx->s3 = (u32_t)0x19191919;
	initctx->s4 = (u32_t)0x20202020;
	initctx->s5 = (u32_t)0x21212121;
	initctx->s6 = (u32_t)0x22222222;
	initctx->s7 = (u32_t)0x23232323;
	initctx->t8 = (u32_t)0x24242424;
	initctx->t9 = (u32_t)0x25252525;
	initctx->k0 = (u32_t)0x26262626;
	initctx->k1 = (u32_t)0x27272727;
	initctx->gp = (u32_t)0x28282828;
	initctx->sp = (u32_t)0x29292929;
	initctx->fp = (u32_t)0x30303030;
	initctx->ra = (u32_t)0x31313131;
	initctx->hi = (u32_t)0x32323232;
	initctx->lo = (u32_t)0x33333333;
#endif

	/* Setup the initial stack frame */
	initctx->epc = (u32_t)_thread_entry;
	initctx->a0 = (u32_t)thread_func;
	initctx->a1 = (u32_t)arg1;
	initctx->a2 = (u32_t)arg2;
	initctx->a3 = (u32_t)arg3;

	/* enable SR_EXL to pretend in expection handler */
	initctx->sr = mips32_getstatus() | 0x7c03;
	initctx->cause = 0;
	
	initctx->gp = get_gp_reg();

#ifdef CONFIG_THREAD_MONITOR
	/*
	 * In debug mode thread->entry give direct access to the thread entry
	 * and the corresponding parameters.
	 */
	thread->entry = (struct __thread_entry *)(initctx);
#endif

	thread->callee_saved.sp = (u32_t)initctx;
	thread->arch.intlock_key = 0;

#ifdef CONFIG_CPU_LOAD_STAT
	thread->running_cycles = 0;
	thread->start_time = 0;
#endif

#ifdef CONFIG_CPU_LOAD_DEBUG
	thread->keep_entry = thread_func;
#endif

	sys_trace_thread_create(thread);

	thread_monitor_init(thread);
}
