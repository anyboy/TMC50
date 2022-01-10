/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fatal fault handling
 *
 * This module implements the routines necessary for handling fatal faults on
 * MIPS M4K CPUs.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <soc.h>
#include <sw_isr_table.h>
#include "mips32_cp0.h"
#include <kallsyms.h>
#include <tracing/tracing.h>
#ifdef CONFIG_CPU0_PANIC_EJTAG_ENABLE
#include <soc_debug.h>
#endif

#ifdef CONFIG_STACK_BACKTRACE
extern void show_backtrace(struct k_thread *thread, const struct __esf *esf);
#endif

#ifdef CONFIG_ACTIONS_TRACE
extern void trace_set_panic(void);
#endif

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

const NANO_ESF _default_esf = {
	0xdeaddead, /* placeholder */
};

void _DeadLoop(void)
{
	volatile int flag = 1;

	while(flag);
}

/**
 *
 * @brief Nanokernel fatal error handler
 *
 * This routine is called when fatal error conditions are detected by software
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 *
 * The caller is expected to always provide a usable ESF. In the event that the
 * fatal error does not have a hardware generated ESF, the caller should either
 * create its own or use a pointer to the global default ESF <_default_esf>.
 *
 * @return This function does not return.
 */
void _NanoFatalErrorHandler(unsigned int reason,
							const NANO_ESF *esf)
{
	switch (reason) {
	case _NANO_ERR_INVALID_TASK_EXIT:
		PR_EXC("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES)
	case _NANO_ERR_STACK_CHK_FAIL:
		PR_EXC("***** Stack Check Fail! *****\n");
		break;
#endif

	case _NANO_ERR_ALLOCATION_FAIL:
		PR_EXC("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		PR_EXC("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}
	PR_EXC("Current thread ID = %p\n"
	       "Faulting instruction address = 0x%x\n",
	       k_current_get(),
	       esf->epc);

	/*
	 * Now that the error has been reported, call the user implemented
	 * policy
	 * to respond to the error.  The decisions as to what responses are
	 * appropriate to the various errors are something the customer must
	 * decide.
	 */

	_SysFatalErrorHandler(reason, esf);

	_DeadLoop();
}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO) && defined(CONFIG_PRINTK)
static char *cause_str(u32_t cause_code)
{
	switch (cause_code) {
	case 0:
		return "Interrupt";
	case 1:
		return "TLB modification execption";
	case 2:
		return "TLB exception (load or instruction fetch)";
	case 3:
		return "TLB exception (store)";
	case 4:
		return "Address error exception (load or instruction fetch)";
	case 5:
		return "Address error exception (store)";
	case 6:
		return "bus error exception (instruction fetch)";
	case 7:
		return "bus error exception (data load or store)";
	case 10:
		return "Reserved instruction exeption";
	default:
		return "unknown";
	}
}
#endif

static void _show_regs(const NANO_ESF *esf)
{
	PR_EXC("esf: %p\n", esf);
	PR_EXC("Current thread ID: %p, prio %d\n"
		"EPC: %08x  STATUS: %08x\n",
		k_current_get(), k_thread_priority_get(k_current_get()),
		esf->epc, esf->sr);
	//print_symbol_name("Thread name: %s\n", (unsigned long)(k_current_get()->entry.pEntry));

	PR_EXC("  at: %08x  v0: %08x  v1: %08x  a0: %08x\n"
	       "  a1: %08x  a2: %08x  a3: %08x  t0: %08x\n",
		esf->at, esf->v0, esf->v1, esf->a0,
		esf->a1, esf->a2, esf->a3, esf->t0);

	PR_EXC("  t1: %08x  t2: %08x  t3: %08x  t4: %08x\n"
	       "  t5: %08x  t6: %08x  t7: %08x  s0: %08x\n",
		esf->t1, esf->t2, esf->t3, esf->t4,
		esf->t5, esf->t6, esf->t7, esf->s0);

	PR_EXC("  s1: %08x  s2: %08x  s3: %08x  s4: %08x\n"
	       "  s5: %08x  s6: %08x  s7: %08x  t8: %08x\n",
		esf->s1, esf->s2, esf->s3, esf->s4,
		esf->s5, esf->s6, esf->s7, esf->t8);

	PR_EXC("  t9: %08x  k0: %08x  k1: %08x  gp: %08x\n"
	       "  sp: %08x  fp: %08x  ra: %08x  hi: %08x\n",
		esf->t9, esf->k0, esf->k1, esf->gp,
		esf->sp, esf->fp, esf->ra, esf->hi);

	PR_EXC("  lo: %08x\n",
		esf->lo);
}

static void dump_stack_data(struct k_thread *thread, const NANO_ESF *esf)
{
	u32_t sp;

	sp = esf->sp;

	if (sp >= (unsigned long)K_THREAD_STACK_BUFFER(_interrupt_stack) &&
	    sp < ((unsigned long)K_THREAD_STACK_BUFFER(_interrupt_stack) + K_THREAD_STACK_SIZEOF(_interrupt_stack))) {
		PR_EXC("Stack 0x%08x in isr stack[0x%08x - 0x%08x]\n", sp,
		(u32_t)K_THREAD_STACK_BUFFER(_interrupt_stack),
		(u32_t)K_THREAD_STACK_BUFFER(_interrupt_stack) + (u32_t)K_THREAD_STACK_SIZEOF(_interrupt_stack));
		//print_buffer(K_THREAD_STACK_BUFFER(_interrupt_stack), 4, K_THREAD_STACK_SIZEOF(_interrupt_stack) / 4, 8, -1);

	} else {
#if defined(CONFIG_THREAD_STACK_INFO)
		PR_EXC("Stack 0x%08x in thread stack[0x%08x, 0x%08x]\n", sp,
			thread->stack_info.start,
			thread->stack_info.start + thread->stack_info.size);
		//print_buffer((void *)thread->stack_info.start, 4, thread->stack_info.size / 4, 8, -1);
#endif
	}
}

void _Fault(const NANO_ESF *esf)
{
#ifdef CONFIG_CPU0_PANIC_EJTAG_ENABLE
	soc_debug_enable_jtag(SOC_JTAG_CPU0, CONFIG_CPU0_EJTAG_GROUP);
#endif

#ifdef CONFIG_ACTIONS_TRACE
	trace_set_panic();
#endif

#ifdef CONFIG_EXTRA_EXCEPTION_INFO
	u32_t cause;

	/* Bits 2-6 contain the cause code */
	cause = (esf->cause & 0x7c) >> 2 ;

	PR_EXC("*** Unhandled Exception *** \n");

	PR_EXC("Exception cause: 0x%02x\n", cause);
	PR_EXC("Reason: %s\n", cause_str(cause));

	_show_regs(esf);

#ifdef CONFIG_STACK_BACKTRACE
	show_backtrace(_current, esf);
#endif

	dump_stack_data(_current, esf);

#endif /* CONFIG_EXTRA_EXCEPTION_INFO */

	_DeadLoop();
}

void panic(const char *err_msg)
{
	u32_t key;

	key = irq_lock();

#ifdef CONFIG_CPU0_PANIC_EJTAG_ENABLE
	soc_debug_enable_jtag(SOC_JTAG_CPU0, CONFIG_CPU0_EJTAG_GROUP);
#endif

#ifdef CONFIG_ACTIONS_TRACE
	trace_set_panic();
#endif

	PR_EXC("Oops: system panic:\n");
	if (err_msg) {
		PR_EXC("%s\n", err_msg);
	}
#ifdef CONFIG_STACK_BACKTRACE
	show_backtrace(NULL, NULL);
#endif

	_DeadLoop();

	irq_unlock(key);
}


/**
 *
 * @brief Fatal error handler
 *
 * This routine implements the corrective action to be taken when the system
 * detects a fatal error.
 *
 * This sample implementation attempts to abort the current thread and allow
 * the system to continue executing, which may permit the system to continue
 * functioning with degraded capabilities.
 *
 * System designers may wish to enhance or substitute this sample
 * implementation to take other actions, such as logging error (or debug)
 * information to a persistent repository and/or rebooting the system.
 *
 * @param reason the fatal error reason
 * @param pEsf pointer to exception stack frame
 *
 * @return N/A
 */
void _SysFatalErrorHandler(unsigned int reason,
					 const NANO_ESF *pEsf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

#if !defined(CONFIG_SIMPLE_FATAL_ERROR_HANDLER)
	if (k_is_in_isr() || _is_thread_essential()) {
		PR_EXC("Fatal fault in %s! Spinning...\n",
		       k_is_in_isr() ? "ISR" : "essential thread");
	}
	PR_EXC("Fatal fault in thread %p! Aborting.\n", _current);
	//k_thread_abort(_current);
#endif
	_DeadLoop();
}

#ifdef CONFIG_STACK_MONITOR
void check_thread_stack_overlow(struct k_thread *thread, unsigned long sp, const NANO_ESF *esf)
{
	unsigned long stack_bottom;
	int free_size;

	stack_bottom = (unsigned long)thread->stack_info.start;

	/* check coarsely */
	free_size = (long)(sp - stack_bottom);
	if (free_size >= 128)
		return;
	else {
#ifdef CONFIG_STACK_MONITOR_PARANOID
		unsigned int *stack_ptr;
		int real_free_size;

		/* check more precisely */
		stack_ptr = (unsigned int *)stack_bottom;
		real_free_size = 0;
		while ((unsigned long)stack_ptr < sp) {
			if (*(stack_ptr++) != 0xaaaaaaaa)
				break;

			real_free_size += 4;
		}

		if (free_size > real_free_size)
			free_size = real_free_size;
#endif
	}

	if (free_size <= 0) {
		PR_EXC("*** Stack overflow detected ***\n");
#if defined(CONFIG_THREAD_MONITOR)
		//print_symbol_name("Thread %s: ", (unsigned long)thread->entry.pEntry);
		PR_EXC(", SP: 0x%lx, orverride %d bytes!\n", sp, -free_size);
#else
		PR_EXC("Thread: 0x%p, SP: 0x%lx, orverride %d bytes!\n", thread, sp, -free_size);
#endif

		panic("Stack overflow");
	} else if (free_size <= CONFIG_STACK_MONITOR_WARN_THRESHOLD) {
		PR_EXC("*** Stack usage warning *** \n");
#if defined(CONFIG_THREAD_MONITOR)
		//print_symbol_name("Thread %s: ", (unsigned long)thread->entry.pEntry);
		PR_EXC(", SP: 0x%lx: stack only has %d bytes free space!\n",
			sp, free_size);
#else
		PR_EXC("Thread: %p, SP: 0x%lx: stack only has %d bytes free space!\n",
			thread, sp, free_size);
#endif

#ifdef CONFIG_STACK_BACKTRACE
		show_backtrace(_current, esf);
#endif
	}
}

void check_stack_overlow(unsigned long sp, const NANO_ESF *esf)
{
	if (sp >= (unsigned long)K_THREAD_STACK_BUFFER(_interrupt_stack) &&
	    sp < ((unsigned long)K_THREAD_STACK_BUFFER(_interrupt_stack) + K_THREAD_STACK_SIZEOF(_interrupt_stack))) {
		/* skip check interrupt stack */
		return;
	}

	check_thread_stack_overlow(k_current_get(), sp, esf);
}
#endif	/* CONFIG_STACK_MONITOR */

void __cli(void);
void _enter_irq(void);
#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
void _sys_k_event_logger_context_switch(void);
#endif
#ifdef CONFIG_CPU_LOAD_STAT
void _sys_cpuload_context_switch(struct k_thread *from, struct k_thread *to);
#endif

void _enter_syscall_exception(NANO_ESF *esf)
{
#ifdef CONFIG_STACK_MONITOR
	uint32_t epc;
#endif
	/* EPC will reference the instruction following syscall, and eret will jump to it */
	esf->epc += 4;

	_kernel.current->callee_saved.sp = (uint32_t)esf;

	/* sys_trace_thread_switched_out(); */

#ifdef CONFIG_STACK_MONITOR
	check_stack_overlow((unsigned long)&epc, esf);
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
	_sys_k_event_logger_context_switch();
#endif

#ifdef CONFIG_CPU_LOAD_STAT
	_sys_cpuload_context_switch(_kernel.current, _kernel.ready_q.cache);
#endif

	_kernel.current = _kernel.ready_q.cache;

	sys_trace_thread_switched_in();
}

extern void __cli(void);

__ramfunc void _enter_interrupt_exception(NANO_ESF *esf)
{
#ifdef CONFIG_STACK_MONITOR
	check_stack_overlow((unsigned long)esf, esf);
#endif

	__cli();

	sys_trace_isr_enter();

	_enter_irq();

#ifdef CONFIG_PREEMPT_ENABLED
//	if(_kernel.nested == 0 && (mips32_getstatus() & 0x3c00) && _kernel.current->base.preempt < _NON_PREEMPT_THRESHOLD && _kernel.ready_q.cache != _kernel.current) {
	if(_kernel.nested == 0 && _kernel.current->base.preempt < _NON_PREEMPT_THRESHOLD && _kernel.ready_q.cache != _kernel.current) {
		sys_trace_isr_exit_to_sched();

#ifdef CONFIG_CPU_LOAD_STAT
		_sys_cpuload_context_switch(_kernel.current, _kernel.ready_q.cache);
#endif	/* CONFIG_KERNEL_CPU_LOAD_STAT */

		_kernel.current = _kernel.ready_q.cache;

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
		_sys_k_event_logger_context_switch();
#endif

		sys_trace_thread_switched_in();

	} else {
		sys_trace_isr_exit();
	}
#endif
}

__ramfunc void _enter_exception(NANO_ESF *esf)
{
	u32_t vector = (esf->cause & 0x7c) >> 2;

	if(vector == 0) {
		_enter_interrupt_exception(esf);
	}
	else if(vector == 8)
		_enter_syscall_exception(esf);
	else
		_Fault(esf);
}

