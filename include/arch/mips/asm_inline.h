/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* MIPS M4K GCC specific public inline assembler functions and macros */

#ifndef _ASM_INLINE_GCC_PUBLIC_GCC_H
#define _ASM_INLINE_GCC_PUBLIC_GCC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifdef _ASMLANGUAGE


#else /* !_ASMLANGUAGE */
#include <stdint.h>
#include <irq.h>

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_msb_set(uint32_t op)
{
	if (!op) {
		return 0;
	}

	return 32 - __builtin_clz(op);
}


/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_lsb_set(uint32_t op)
{
	return __builtin_ffs(op);
}


/**
 *
 * @brief Disable all interrupts on the CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API.  It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 */

#ifndef CONFIG_USE_MIPS16E
static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned long key;

	__asm__ __volatile__(
	"	.set	push						\n"
	"	.set    mips32r2					\n"
	"	.set	reorder						\n"
	"	.set	noat						\n"
	"	di	%0						\n"
	"	andi	%0, 1						\n"
	"	ehb							\n"
	"	.set	pop						\n"
	: "=r" (key)
	: /* no inputs */
	: "memory");

	return key;
}
/**
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * This routine re-enables interrupts on the CPU.  The @a key parameter is an
 * architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @param key architecture-dependent lock-out key
 *
 * @return N/A
 *
 */

static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	__asm__ __volatile__(
	"	.set	push						\n"
	"	.set    mips32r2					\n"
	"	.set	noreorder					\n"
	"	.set	noat						\n"
	"	beqz	%0, 1f						\n"
	"	di							\n"
	"	ei							\n"
	"1:								\n"
	"	ehb							\n"
	"	.set	pop						\n"
	: "=r" (key)
	: "0" (key)
	: "memory");
}

static ALWAYS_INLINE int _arch_irq_is_locked(void)
{
	int key;

	__asm__ __volatile__(
	"	.set	push						\n"
	"	.set    mips32r2					\n"
	"	.set	noreorder					\n"
	"	.set	noat						\n"
	"	mfc0	%0, $12						\n"
	"	andi	%0, 0x1						\n"
	"	not	%0						\n"
	"	.set	pop						\n"
	: "=r" (key)
	:
	: "memory");

	return key;
}

#else	/* CONFIG_USE_MIPS16E */

unsigned int _arch_irq_lock(void);
void _arch_irq_unlock(unsigned int key);
int _arch_irq_is_locked(void);

#endif	/* CONFIG_USE_MIPS16E */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */
