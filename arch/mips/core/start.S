/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mips32_regs.h"
#include "mips32_cp0.h"

	.section .reset, "ax", @progbits
	.set	noreorder
	.global __start
	.ent	__start

__start:
	/* Initialize CP0 registers */
	li	t0, SR_CU0
	mtc0	t0, C0_STATUS
	mtc0	zero, C0_INTCTL
	mtc0	zero, C0_SRSCTL
	mtc0	zero, C0_SRSMAP
	mtc0	zero, C0_COUNT
	li	t8, 0xffffffff
	mtc0	t8, C0_COMPARE

	/* Initialize EBase register */
	la	t8, __exc_vector_base
	mtc0	t8, C0_EBASE

	/* disable IV */
	mtc0	zero, C0_CAUSE

#ifdef CONFIG_INIT_STACKS
	la	t0, _interrupt_stack
	li	t1, 0xaaaaaaaa
	li	t2, CONFIG_ISR_STACK_SIZE
1:
	sw	t1, 0(t0)
	addi	t0, 4
	addi	t2, -4
	bne	t2, zero, 1b
	nop
#endif

	/* Set up the initial stack pointer to the interrupt stack, safe
	 * to use this as the CPU boots up with interrupts disabled and we
	 * don't turn them on until much later, when the kernel is on
	 * the main stack */
#if 0
	la	sp, _interrupt_stack
	addiu	sp, CONFIG_ISR_STACK_SIZE
#else
	la	sp, sys_work_q_stack
	addiu	sp, CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE
#endif
	/* reserve 16 bytes for argument space */
	addiu	sp, -16

	la    t9, _PrepC
	jal   t9
	nop
	.end	__start
