/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <kernel_structs.h>
#include <offsets_short.h>
#include "mips32_regs.h"
#include "mips32_cp0.h"

GTEXT(_enter_irq)

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
GTEXT(_sys_k_event_logger_context_switch)
#endif

#ifdef CONFIG_STACK_MONITOR
GTEXT(check_stack_overlow)
#endif

GTEXT(_sys_cpu_load_context_switch)
GTEXT(__cli)

.macro SAVE_CONTEXT
	addiu	sp, sp, -_esf_t_SIZEOF

	sw	at_reg, ___esf_t_at_OFFSET(sp)
	sw	v0, ___esf_t_v0_OFFSET(sp)
	sw	v1, ___esf_t_v1_OFFSET(sp)
	sw	a0, ___esf_t_a0_OFFSET(sp)
	sw	a1, ___esf_t_a1_OFFSET(sp)
	sw	a2, ___esf_t_a2_OFFSET(sp)
	sw	a3, ___esf_t_a3_OFFSET(sp)
	sw	t0, ___esf_t_t0_OFFSET(sp)
	sw	t1, ___esf_t_t1_OFFSET(sp)
	sw	t2, ___esf_t_t2_OFFSET(sp)
	sw	t3, ___esf_t_t3_OFFSET(sp)
	sw	t4, ___esf_t_t4_OFFSET(sp)
	sw	t5, ___esf_t_t5_OFFSET(sp)
	sw	t6, ___esf_t_t6_OFFSET(sp)
	sw	t7, ___esf_t_t7_OFFSET(sp)
	sw	s0, ___esf_t_s0_OFFSET(sp)
	sw	s1, ___esf_t_s1_OFFSET(sp)
	sw	s2, ___esf_t_s2_OFFSET(sp)
	sw	s3, ___esf_t_s3_OFFSET(sp)
	sw	s4, ___esf_t_s4_OFFSET(sp)
	sw	s5, ___esf_t_s5_OFFSET(sp)
	sw	s6, ___esf_t_s6_OFFSET(sp)
	sw	s7, ___esf_t_s7_OFFSET(sp)
	sw	t8, ___esf_t_t8_OFFSET(sp)
	sw	t9, ___esf_t_t9_OFFSET(sp)
	sw	k0, ___esf_t_k0_OFFSET(sp)
	sw	k1, ___esf_t_k1_OFFSET(sp)
	sw	gp, ___esf_t_gp_OFFSET(sp)
	sw	fp, ___esf_t_fp_OFFSET(sp)
	sw	ra, ___esf_t_ra_OFFSET(sp)

	/* Save the contents of the LO and HI registers */
	mflo	t0
	mfhi	t1
	sw	t0, ___esf_t_lo_OFFSET(sp)
	sw	t1, ___esf_t_hi_OFFSET(sp)

	mfc0	t0, C0_EPC
	sw	t0, ___esf_t_epc_OFFSET(sp)

	mfc0	t0, C0_STATUS
	sw	t0, ___esf_t_sr_OFFSET(sp)

	mfc0	t0, C0_CAUSE
	sw	t0, ___esf_t_cause_OFFSET(sp)

	/* save old sp for stack backtrace */
	move	t0, sp
	addiu	t0, t0, _esf_t_SIZEOF
	sw	t0, ___esf_t_sp_OFFSET(sp)

	.endm


.macro RESTORE_CONTEXT
	/* Restore the Status register */
	lw	t0, ___esf_t_sr_OFFSET(sp)
	mtc0	t0, C0_STATUS
	ehb

	/* Restore the EPC */
	lw	t0, ___esf_t_epc_OFFSET(sp)
	mtc0	t0, C0_EPC
	ehb

	/* Restore the contents of the LO and HI registers */
	lw	t0, ___esf_t_lo_OFFSET(sp)
	lw	t1, ___esf_t_hi_OFFSET(sp)
	mtlo	t0
	mthi	t1

	lw	ra, ___esf_t_ra_OFFSET(sp)
	lw	fp, ___esf_t_fp_OFFSET(sp)
	lw	gp, ___esf_t_gp_OFFSET(sp)
	lw	k1, ___esf_t_k1_OFFSET(sp)
	lw	k0, ___esf_t_k0_OFFSET(sp)
	lw	t9, ___esf_t_t9_OFFSET(sp)
	lw	t8, ___esf_t_t8_OFFSET(sp)
	lw	s7, ___esf_t_s7_OFFSET(sp)
	lw	s6, ___esf_t_s6_OFFSET(sp)
	lw	s5, ___esf_t_s5_OFFSET(sp)
	lw	s4, ___esf_t_s4_OFFSET(sp)
	lw	s3, ___esf_t_s3_OFFSET(sp)
	lw	s2, ___esf_t_s2_OFFSET(sp)
	lw	s1, ___esf_t_s1_OFFSET(sp)
	lw	s0, ___esf_t_s0_OFFSET(sp)
	lw	t7, ___esf_t_t7_OFFSET(sp)
	lw	t6, ___esf_t_t6_OFFSET(sp)
	lw	t5, ___esf_t_t5_OFFSET(sp)
	lw	t4, ___esf_t_t4_OFFSET(sp)
	lw	t3, ___esf_t_t3_OFFSET(sp)
	lw	t2, ___esf_t_t2_OFFSET(sp)
	lw	t1, ___esf_t_t1_OFFSET(sp)
	lw	t0, ___esf_t_t0_OFFSET(sp)
	lw	a3, ___esf_t_a3_OFFSET(sp)
	lw	a2, ___esf_t_a2_OFFSET(sp)
	lw	a1, ___esf_t_a1_OFFSET(sp)
	lw	a0, ___esf_t_a0_OFFSET(sp)
	lw	v1, ___esf_t_v1_OFFSET(sp)
	lw	v0, ___esf_t_v0_OFFSET(sp)
	lw	at_reg, ___esf_t_at_OFFSET(sp)

	addiu	sp, sp, _esf_t_SIZEOF
	.endm

.macro CLI
	mfc0	t0, C0_STATUS
	li	t1, 0xfffffffc
	and t0, t0, t1
	mtc0	t0, C0_STATUS
	ehb
.endm

	.section .exc_vector, "ax", @progbits
	.set	noreorder
	.set noat

	.global __exc_vector_base
__exc_vector_base:

	.ent	__general_exception
__general_exception:
	SAVE_CONTEXT

	la	s0, _kernel
	move	s1, sp

	/* get interrupt stack top */
	lw	s2, _kernel_offset_to_irq_stack(s0)
	sltu	s2, s2, sp
	bne s2, zero, except_in_task
	nop

	la	s2, _interrupt_stack
	sltu	s2, sp, s2
	beq	s2, zero, except_in_irq0	/*to here means: sp > __irqstk_bottom && sp < irqstk_top*/
	nop

except_in_task:
	lw	t0, _kernel_offset_to_current(s0)

	/* save current sp */
	sw	sp, _thread_offset_to_sp(t0)

	/* Switch to interrupt stack */
	lw	sp, _kernel_offset_to_irq_stack(s0)

except_in_irq:
	jal 	_enter_exception
	move	a0, s1

	beq	s2, zero, eret_to_irq0
	nop

eret_to_task:
	/* change sp to current thread */
	lw	t0, _kernel_offset_to_current(s0)
	lw	sp, _thread_offset_to_sp(t0)

eret_to_irq:
	RESTORE_CONTEXT
	eret

except_in_irq0:
	//b	.
	//nop
	b	except_in_irq
	nop

eret_to_irq0:
	b	eret_to_irq
	nop

	.end	__general_exception

	.ent	__cli
__cli:
	CLI
	jr	ra
	nop
	.end	__cli
