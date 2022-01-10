/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS M4K power management
 *
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#ifdef CONFIG_TICKLESS_IDLE
#include <kernel_structs.h>
#endif
#include "mips32_regs.h"

GTEXT(k_cpu_idle)
GTEXT(k_cpu_atomic_idle)

/**
 *
 * @brief Power save idle routine for MIPS M4K
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in thekernel when the
 * '_sys_power_save_flag' variable is non-zero.  The MIPS 'wait' instruction
 * will be issued, causing a low-power consumption sleep mode.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void nano_cpu_idle (void);
 */

SECTION_FUNC(TEXT, k_cpu_idle)
	ei
	nop
	nop
	wait
	nop
	nop
	jr	ra
	nop

/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * INTERNAL
 * The requirements for k_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that occurs
 *    if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void k_cpu_atomic_idle (unsigned int imask);
 */
SECTION_FUNC(TEXT, k_cpu_atomic_idle)
	ei
	ehb
	wait
	nop
	
	/* restore interrupt lock state */
	beqz	a0, 1f
	di
	ei
1:
	ehb
	jr	ra
