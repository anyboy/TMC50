/*
 * from linux/arch/mips/process.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 1999, 2000 by Ralf Baechle and others.
 * Copyright (C) 2005, 2006 by Ralf Baechle (ralf@linux-mips.org)
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2004 Thiemo Seufer
 * Copyright (C) 2013  Imagination Technologies Ltd.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <string.h>
#include <misc/printk.h>
#include <kallsyms.h>
#include <stack_backtrace.h>
#include <inst.h>

struct mips_frame_info {
	void		*func;
	unsigned long	func_size;
	int		frame_size;
	int		pc_offset;
};

typedef unsigned int mips_instruction;

static inline int is_ra_save_ins(union mips_instruction *ip)
{
	/* sw / sd $ra, offset($sp) */
	return (ip->i_format.opcode == sw_op || ip->i_format.opcode == sd_op) &&
		ip->i_format.rs == 29 &&
		ip->i_format.rt == 31;
}

static inline int is_jump_ins(union mips_instruction *ip)
{
	if (ip->j_format.opcode == j_op)
		return 1;
	if (ip->j_format.opcode == jal_op)
		return 1;
	if (ip->r_format.opcode != spec_op)
		return 0;
	return ip->r_format.func == jalr_op || ip->r_format.func == jr_op;
}

static inline int is_sp_move_ins(union mips_instruction *ip)
{
	/* addiu/daddiu sp,sp,-imm */
	if (ip->i_format.rs != 29 || ip->i_format.rt != 29)
		return 0;
	if (ip->i_format.opcode == addiu_op || ip->i_format.opcode == daddiu_op)
		return 1;

	return 0;
}

static inline int is_mips16e_jmp_ins(union mips16e_instruction *ip)
{
	/* jal(x) xxx, 32bit instruction */
	if (ip->jal.opcode == MIPS16e_jal_op)
		return 1;

	/* j(al)r xxx */
	if (ip->rr.opcode == MIPS16e_rr_op &&
	    ip->rr.func == MIPS16e_jr_func)
		return 1;

	return 0;
}

static inline int is_mips16e_extend_ins(union mips16e_instruction *ip)
{
	return (ip->rr.opcode == MIPS16e_extend_op);
}

static inline int is_mips16e_save_ins(union mips16e_instruction *ip)
{
	/* save xx, ra, xx */
	return (ip->i8.opcode == MIPS16e_i8_op &&
		ip->i8.func == MIPS16e_svrs_func &&
		(ip->i8.imm & 0xc0) == 0xc0);
}

static int get_frame_info_mips16e(struct mips_frame_info *info)
{
	union mips16e_instruction *ip, *ext_ip;
	unsigned max_insns = info->func_size / sizeof(union mips16e_instruction);
	unsigned i;

	info->pc_offset = -1;
	info->frame_size = 0;

	ip = (void *)((unsigned long)info->func & ~0x1);
	if (!ip)
		return -1;

	if (max_insns == 0)
		max_insns = 128U;	/* unknown function size */
	max_insns = min(128U, max_insns);

	for (i = 0; i < max_insns; i++, ip++) {
		if (is_mips16e_jmp_ins(ip))
			break;

		if (is_mips16e_extend_ins(ip)) {
			ext_ip = ip;

			/* move to next real ins */
			ip++;
			i++;

			if (is_mips16e_save_ins(ip)) {
				info->frame_size = ((ext_ip->i8.imm & 0xf0) +
						    (ip->i8.imm & 0xf)) << 3;
				info->pc_offset = (info->frame_size / 4) - 1;
				break;
			}
		} else {
			if (is_mips16e_save_ins(ip)) {
				info->frame_size = (ip->i8.imm & 0xf) << 3;
				info->pc_offset = (info->frame_size / 4) - 1;
				break;
			}
		}
	}

	if (info->frame_size && info->pc_offset >= 0) /* nested */
		return 0;
	if (info->pc_offset < 0) /* leaf */
		return 1;

	/* prologue seems boggus... */
	return -1;
}

static int get_frame_info_mips32(struct mips_frame_info *info)
{
	union mips_instruction *ip = info->func;
	unsigned max_insns = info->func_size / sizeof(union mips_instruction);
	unsigned i;

	info->pc_offset = -1;
	info->frame_size = 0;

	if (!ip)
		goto err;

	if (max_insns == 0)
		max_insns = 128U;	/* unknown function size */
	max_insns = min(128U, max_insns);

	for (i = 0; i < max_insns; i++, ip++) {

		if (is_jump_ins(ip))
			break;
		if (!info->frame_size) {
			if (is_sp_move_ins(ip))
			{
				info->frame_size = - ip->i_format.simmediate;
			}
			continue;
		}
		if (info->pc_offset == -1 && is_ra_save_ins(ip)) {
			info->pc_offset =
				ip->i_format.simmediate / sizeof(long);
			break;
		}
	}
	if (info->frame_size && info->pc_offset >= 0) /* nested */
		return 0;
	if (info->pc_offset < 0) /* leaf */
		return 1;
	/* prologue seems boggus... */
err:
	return -1;
}

static int get_frame_info(struct mips_frame_info *info)
{
	if ((unsigned long)info->func & 0x1)
		return get_frame_info_mips16e(info);
	else
		return get_frame_info_mips32(info);
}

static unsigned long unwind_stack_by_address(unsigned long stack_base,
					     unsigned long stack_size,
					     unsigned long *sp,
					     unsigned long pc,
					     unsigned long *ra)
{
	struct mips_frame_info info;
	unsigned long size, ofs;
	int leaf;

	if (!kallsyms_lookup_size_offset(pc & ~0x1u, &size, &ofs))
		return 0;

	/*
	 * Return ra if an exception occurred at the first instruction
	 */
	if (unlikely(ofs == 0)) {
		pc = *ra;
		*ra = 0;
		return pc;
	}


	info.func = (void *)(pc - ofs);
	info.func_size = ofs;	/* analyze from start to ofs */
	leaf = get_frame_info(&info);

	if (leaf < 0)
		return 0;

	if (*sp < stack_base ||
	    *sp + info.frame_size > (stack_base + stack_size))
		return 0;

	if (leaf)
		/*
		 * For some extreme cases, get_frame_info() can
		 * consider wrongly a nested function as a leaf
		 * one. In that cases avoid to return always the
		 * same value.
		 */
		pc = pc != *ra ? *ra : 0;
	else
		pc = ((unsigned long *)(*sp))[info.pc_offset];


	*sp += info.frame_size;
	*ra = 0;

	return is_ksym_addr(pc) ? pc : 0;
}

/* used by show_backtrace() */
static unsigned long unwind_stack(struct k_thread *thread, unsigned long *sp,
			   unsigned long pc, unsigned long *ra)
{
	unsigned long stack_base, stack_size;

	if (*sp >= (unsigned long)K_THREAD_STACK_BUFFER(_interrupt_stack) &&
	    *sp < ((unsigned long)K_THREAD_STACK_BUFFER(_interrupt_stack) + K_THREAD_STACK_SIZEOF(_interrupt_stack))) {
		/* within interrupt stack */
		stack_base = (unsigned long)K_THREAD_STACK_BUFFER(_interrupt_stack);
		stack_size = K_THREAD_STACK_SIZEOF(_interrupt_stack);
	} else {
		/* thread struct is just located at stack base */
		stack_base = thread->stack_info.start;
		stack_size = thread->stack_info.size;
	}

	return unwind_stack_by_address(stack_base, stack_size, sp, pc, ra);
}

#define get_current_pc()						\
({									\
	 unsigned long	__res;						\
									\
	__asm__ __volatile__(						\
	"	.set	push					\n"	\
	"	.set	mips32r2				\n"	\
	"	.set	noat					\n"	\
	"	1: la	%0, 1b					\n"	\
	"	.set	pop					\n"	\
	: "=r" (__res));						\
									\
	__res;								\
})


#define get_current_reg(reg)						\
({									\
	 unsigned long	__res;						\
									\
	__asm__ __volatile__(						\
	"	.set	push					\n"	\
	"	.set	mips32r2				\n"	\
	"	.set	noat					\n"	\
	"	move	%0, $" #reg "				\n"	\
	"	.set	pop					\n"	\
	: "=r" (__res));						\
									\
	__res;								\
})

void show_backtrace(struct k_thread *thread, const struct __esf *esf)
{
	unsigned long sp, ra, pc;
	unsigned long pre_sp;

	if (!thread)
		thread = k_current_get();

	if (!esf) {
		if (thread == k_current_get()) {
			pc = get_current_pc();
			sp = get_current_reg(29);
			ra = get_current_reg(31);
		} else {
			/* load esf from saved sp */
			esf = (const struct __esf *)thread->callee_saved.sp;

			sp = esf->sp;
			ra = esf->ra;
			pc = esf->epc;
		}
	} else {
		sp = esf->sp;
		ra = esf->ra;
		pc = esf->epc;
	}
	pre_sp = sp;
	printk("Call Trace:\n");
	do {
		printk(" %d ",(int)(sp - pre_sp));
		pre_sp = sp;
		print_ip_sym(pc);
		pc = unwind_stack(thread, &sp, pc, &ra);
	} while (pc);
	printk("\n");
}

void show_thread_stack(struct k_thread *thread)
{
	printk("Thread: 0x%p %s\n", thread,
		thread == k_current_get() ? "*" : " ");

	show_backtrace(thread, NULL);
}

void show_all_threads_stack(void)
{
#if defined(CONFIG_THREAD_MONITOR)
	struct k_thread *thread = NULL;

	thread = (struct k_thread *)(_kernel.threads);
	while (thread != NULL) {
		show_thread_stack(thread);
		thread = (struct k_thread *)thread->next_thread;
	}
#endif
}

void dump_stack(void)
{
	show_thread_stack(k_current_get());
}
