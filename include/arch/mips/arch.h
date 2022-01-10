/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS specific kernel interface header
 *
 * This header contains the MIPS specific kernel interface.  It is
 * included by the nanokernel interface architecture-abstraction header
 * (kernel/cpu.h)
 */

#ifndef _MIPS_ARCH__H_
#define _MIPS_ARCH__H_

#ifdef __cplusplus
extern "C" {
#endif

/* MIPS GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { uint32_t name1, name2; }

/* APIs need to support non-byte addressable architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#include <arch/mips/exc.h>
#include <arch/mips/error.h>
#include <arch/mips/irq.h>
#include <arch/mips/asm_inline.h>
#include <arch/mips/sys_io.h>
#include <sw_isr_table.h>

#define STACK_ALIGN  8

#ifndef _ASMLANGUAGE
extern void panic(const char *err_msg);
extern u32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()	_timer_cycle_get_32()

typedef struct sys_irq_flags {
	u32_t keys[2];
} SYS_IRQ_FLAGS;

void sys_irq_lock(SYS_IRQ_FLAGS *flags);
void sys_irq_unlock(const SYS_IRQ_FLAGS *flags);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _MIPS_ARCH__H_ */
