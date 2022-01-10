/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS M4K public interrupt handling
 *
 * MIPS-specific nanokernel interrupt handling interface. Included by mips/arch.h.
 */

#ifndef _ARCH_MIPS_IRQ_H_
#define _ARCH_MIPS_IRQ_H_

#include <irq.h>
#include <sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_arch_irq_enable)
GTEXT(_arch_irq_disable)
GTEXT(_arch_irq_is_enabled)
#else
extern void _arch_irq_enable(unsigned int irq);
extern void _arch_irq_disable(unsigned int irq);
extern int _arch_irq_is_enabled(unsigned int irq);

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void _irq_priority_set(unsigned int irq, unsigned int prio,
			      uint32_t flags);

/* Spurious interrupt handler. Throws an error if called */
extern void _irq_spurious(void *unused);

/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * _ISR_DECLARE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */
#define _ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_MIPS_IRQ_H_ */
