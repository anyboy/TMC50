/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS16E helper functions
 */

#include "mips32_regs.h"
#include "mips32_cp0.h"

#define __read_cp0_register(reg, sel)					\
({	unsigned int __res;						\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mfc0\t%0, $" #reg "\n\t"			\
			: "=r" (__res));				\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mfc0\t%0, $" #reg ", " #sel "\n\t"		\
			".set\tmips0\n\t"				\
			: "=r" (__res));				\
	__res;								\
})

#define __write_cp0_register(reg, sel, value)				\
do {									\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mtc0\t%z0, $" #reg "\n\t"			\
			: : "Jr" ((unsigned int)(value)));		\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mtc0\t%z0, $" #reg ", " #sel "\n\t"		\
			".set\tmips0"					\
			: : "Jr" ((unsigned int)(value)));		\
} while (0)

__attribute__((nomips16)) unsigned int mips32_getcount(void)
{
	return __read_cp0_register(9, 0);
}

__attribute__((nomips16)) unsigned int mips32_getstatus(void)
{
	return __read_cp0_register(12, 0);
}

__attribute__((nomips16)) void mips32_setstatus(unsigned int val)
{
	__write_cp0_register(12, 0, val);
}

__attribute__((nomips16)) unsigned int mips32_getcause(void)
{
	return __read_cp0_register(13, 0);
}

__attribute__((nomips16)) void mips32_setcause(unsigned int val)
{
	__write_cp0_register(13, 0, val);
}

#include "linker/section_tags.h"

__ramfunc __attribute__((nomips16)) unsigned int _arch_irq_lock(void)
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

__ramfunc __attribute__((nomips16)) void _arch_irq_unlock(unsigned int key)
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

__ramfunc __attribute__((nomips16)) int _arch_irq_is_locked(void)
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
