/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS M4K interrupt management code for use with Internal
 * Interrupt Controller
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <irq.h>
#include <soc.h>
#include <misc/printk.h>
#include <misc/stack.h>
#include <sw_isr_table.h>
#include <logging/kernel_event_logger.h>
#include "mips32_cp0.h"

#define MAX_PRIO_NUM	2

void _arch_irq_enable(unsigned int irq)
{
	u32_t reg, bit;
	u32_t ienable;
	int key;

	reg = INTC_EN0 + ((irq / 32) * 4);
	bit = irq % 32;

	key = irq_lock();

	ienable = sys_read32(reg);
	ienable |= (1 << bit);
	sys_write32(ienable, reg);

	irq_unlock(key);
};

void _arch_irq_disable(unsigned int irq)
{
	u32_t reg, bit;
	u32_t ienable;
	int key;

	reg = INTC_EN0 + ((irq / 32) * 4);
	bit = irq % 32;

	key = irq_lock();

	ienable = sys_read32(reg);
	ienable &= ~(1 << bit);
	sys_write32(ienable, reg);

	irq_unlock(key);
};

/**
 * @brief Return IRQ enable state
 *
 * @param irq IRQ line
 * @return interrupt enable state, true or false
 */
int _arch_irq_is_enabled(unsigned int irq)
{
	u32_t reg, bit;

	reg = INTC_EN0 + ((irq / 32) * 4);
	bit = irq % 32;

	return !!(sys_read32(reg) & (1 << bit));
}

/*
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * Lower values take priority over higher values. Special case priorities are
 * expressed via mutually exclusive flags.

 * The priority is verified if ASSERT_ON is enabled; max priority level
 * depends on CONFIG_NUM_IRQ_PRIO_LEVELS.
 *
 * @return N/A
 */

void _irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	//printk("%s: irq %d prio %d\n", __func__, irq, prio);
	ARG_UNUSED(flags);
}

static u32_t _irq_priority_get(unsigned int irq)
{
	u32_t prio;

	if (irq == IRQ_ID_BT_BASEBAND)
		prio = 0;
	else
		prio = 1;

	return prio;	
}

static void _irq_clear_pending(unsigned int irq)
{
	u32_t reg, bit;

	reg = INTC_PD0 + ((irq / 32) * 4);
	bit = irq % 32;

	sys_write32(1 << bit, reg);
}

/*
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * @return N/A
 */
void _irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	printk("Spurious interrupt detected! ipending: %x, %x\n",
	       sys_read32(INTC_PD0), sys_read32(INTC_PD1));
	_NanoFatalErrorHandler(0, &_default_esf);
}

/**
 *
 * @brief Initialize interrupts
 *
 * Ensures all interrupts have their priority set to _EXC_IRQ_DEFAULT_PRIO and
 * not 0, which they have it set to when coming out of reset. This ensures that
 * interrupt locking via BASEPRI works as expected.
 *
 * @return N/A
 */

void _IntLibInit(void)
{
	int status;

	/* disable all interrupts by default */
	sys_write32(0x0, INTC_EN0);
	sys_write32(0x0, INTC_EN1);

	/* route all interrupts to IP2, bluetooth irq to IP3 */
	sys_write32(0x1, INTC_CFG0_0);
	sys_write32(0x0, INTC_CFG0_1);
	sys_write32(0x0, INTC_CFG0_2);
	sys_write32(0x0, INTC_CFG1_0);
	sys_write32(0x0, INTC_CFG1_1);
	sys_write32(0x0, INTC_CFG1_2);

	status = mips32_getstatus();
	/* disable all irq before schedule to the first thread */;
	status &= ~(SR_IM2 | SR_IM3 | SR_IM4 | SR_IM5 | SR_IM6 | SR_IM7 | SR_IE);
	mips32_setstatus(status);
}

static void enable_irq_nest(u32_t prio)
{
	u32_t status;

	status = mips32_getstatus();
	status &= ~(SR_IM2 | SR_IM3 | SR_IM4 | SR_IM5 | SR_IM6 | SR_IM7);
	if (prio != 0) {
		status |= SR_IM3 | SR_IE;
	}
	mips32_setstatus(status);
}

static void disable_irq_nest(void)
{
	u32_t status;

	status = mips32_getstatus();
	status &= ~(SR_IM2 | SR_IM3 | SR_IM4 | SR_IM5 | SR_IM6 | SR_IM7 | SR_IE);
	mips32_setstatus(status);
}

__ramfunc static void _do_irq_by_prio(u32_t prio)
{
	struct _isr_table_entry *ite;
	u32_t pd0, pd1, irq, irq_prio;
#ifdef CONFIG_IRQ_STAT
	u32_t ts_irq_enter, irq_cycles;
#endif

	pd0 = sys_read32(INTC_PD0) & sys_read32(INTC_EN0);
	pd1 = sys_read32(INTC_PD1) & sys_read32(INTC_EN1);

	if (pd0 == 0 && pd1 == 0)
		return;

	enable_irq_nest(prio);

	while (pd0 || pd1) {
		irq = find_lsb_set(pd0) - 1;
		if (pd0 == 0) {
			irq = 32  + (find_lsb_set(pd1) - 1);
			pd1 &= ~(1 << (irq - 32));
		} else {
			pd0 &= ~(1 << irq);
		}
		
		irq_prio = _irq_priority_get(irq);
		if (irq_prio != prio)
			continue;

		ite = &_sw_isr_table[irq];

#ifdef CONFIG_IRQ_STAT
		ts_irq_enter = mips32_getcount();
#endif
		ite->isr(ite->arg);

#ifdef CONFIG_IRQ_STAT
#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
		extern void mpu_disable_region(unsigned int index);
		mpu_disable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX);
#endif
		ite->irq_cnt++;
		irq_cycles = mips32_getcount() - ts_irq_enter;
		if (irq_cycles > ite->max_irq_cycles)
			ite->max_irq_cycles = irq_cycles;
#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
		extern void mpu_enable_region(unsigned int index);
		mpu_enable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX);
#endif
#endif

		_irq_clear_pending(irq);
	}

	disable_irq_nest();
}

__ramfunc void _enter_irq(void)
{
	u32_t prio, max_prio;

	_kernel.nested++;

	max_prio = MAX_PRIO_NUM - find_lsb_set((mips32_getstatus() >> SR_HWIM_SHIFT) & 0x1f);

	for (prio = 0; prio <= max_prio; prio++) {
		_do_irq_by_prio(prio);
	}

	_kernel.nested--;
}

#if defined(CONFIG_SYS_IRQ_LOCK)
__ramfunc void sys_irq_lock(SYS_IRQ_FLAGS *flags)
{
	u32_t key;

	k_sched_lock();

	key = irq_lock();

	flags->keys[0] = sys_read32(INTC_EN0);
	flags->keys[1] = sys_read32(INTC_EN1);

	/* only enable irq0(bluetooth irq) */
#ifdef CONFIG_SYS_IRQ_LOCK_EXCEPT_BT_CON
	sys_write32(flags->keys[0] & (1u << IRQ_ID_BT_BASEBAND), INTC_EN0);
#else
	sys_write32(0, INTC_EN0);
#endif
	sys_write32(0, INTC_EN1);

	irq_unlock(key);
}

__ramfunc void sys_irq_unlock(const SYS_IRQ_FLAGS *flags)
{
	u32_t key;

	key = irq_lock();

	sys_write32(flags->keys[0], INTC_EN0);
	sys_write32(flags->keys[1], INTC_EN1);

	irq_unlock(key);

	k_sched_unlock();
}
#else
void sys_irq_lock(SYS_IRQ_FLAGS *flags)
{
	flags->keys[0] = irq_lock();
}

void sys_irq_unlock(const SYS_IRQ_FLAGS *flags)
{
	irq_unlock(flags->keys[0]);
}
#endif
