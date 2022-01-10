/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for MIPS M4K
 *
 * This module implements the routines necessary for thread context switching
 * on MIPS M4K CPUs.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include "mips32_regs.h"
#include "mips32_cp0.h"

/* exports */
GTEXT(__swap)

/* imports */
GTEXT(_get_next_ready_thread)
GTEXT(_k_neg_eagain)
GDATA(_kernel)

/**
 *
 * @brief Initiate a cooperative context switch
 *
 * The _Swap() routine is invoked by various nanokernel services to effect
 * a cooperative context context switch.  Prior to invoking _Swap(), the caller
 * disables interrupts via irq_lock() and the return 'key' is passed as a
 * parameter to _Swap().
 *
 * @return may contain a return value setup by a call to _set_thread_return_value()
 *
 * C function prototype:
 *
 * unsigned int _Swap (unsigned int key);
 *
 */

SECTION_FUNC(TEXT, __swap)
	la	a1, _kernel

	/* get k_thread ptr */
	lw	a2, _kernel_offset_to_current(a1)
	sw	a0, _thread_offset_to_intlock_key(a2)

	/*
	* Set _Swap()'s default return code to -EAGAIN. This eliminates the need
	* for the timeout code to set it itself.
	*/
	la	a1, _k_neg_eagain
	lw	a1, 0(a1)
	sw	a1, _thread_offset_to_return_value(a2)

	/* enter syscall exception to swap context */
	syscall
	nop

	/* Load return value into v0 (return value register). -EAGAIN
	 * unless someone previously called _set_thread_return_value().
	 * Do this before we potentially unlock interrupts.
	 */
	la	a1, _kernel
	lw	a2, _kernel_offset_to_current(a1)
	lw	v0, _thread_offset_to_return_value(a2)

	/* restore interrupt lock state */
	lw	a0, _thread_offset_to_intlock_key(a2)
	beqz	a0, 1f
	di
	ei
1:
	ehb
	jr	ra
	nop
