/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various MIPS kernel structures.
 *
 * All of the absolute symbols defined by this module will be present in the
 * final kernel ELF image (due to the linker's reference to the _OffsetAbsSyms
 * symbol).
 *
 * INTERNAL
 * It is NOT necessary to define the offset for every member of a structure.
 * Typically, only those members that are accessed by assembly language routines
 * are defined; however, it doesn't hurt to define all fields for the sake of
 * completeness.
 */

#include <gen_offset.h>
#include <kernel_structs.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_thread_arch_t, intlock_key);
GEN_OFFSET_SYM(_thread_arch_t, return_value);

GEN_OFFSET_SYM(_esf_t, at);
GEN_OFFSET_SYM(_esf_t, v0);
GEN_OFFSET_SYM(_esf_t, v1);
GEN_OFFSET_SYM(_esf_t, a0);
GEN_OFFSET_SYM(_esf_t, a1);
GEN_OFFSET_SYM(_esf_t, a2);
GEN_OFFSET_SYM(_esf_t, a3);
GEN_OFFSET_SYM(_esf_t, t0);
GEN_OFFSET_SYM(_esf_t, t1);
GEN_OFFSET_SYM(_esf_t, t2);
GEN_OFFSET_SYM(_esf_t, t3);
GEN_OFFSET_SYM(_esf_t, t4);
GEN_OFFSET_SYM(_esf_t, t5);
GEN_OFFSET_SYM(_esf_t, t6);
GEN_OFFSET_SYM(_esf_t, t7);
GEN_OFFSET_SYM(_esf_t, s0);
GEN_OFFSET_SYM(_esf_t, s1);
GEN_OFFSET_SYM(_esf_t, s2);
GEN_OFFSET_SYM(_esf_t, s3);
GEN_OFFSET_SYM(_esf_t, s4);
GEN_OFFSET_SYM(_esf_t, s5);
GEN_OFFSET_SYM(_esf_t, s6);
GEN_OFFSET_SYM(_esf_t, s7);
GEN_OFFSET_SYM(_esf_t, t8);
GEN_OFFSET_SYM(_esf_t, t9);
GEN_OFFSET_SYM(_esf_t, k0);
GEN_OFFSET_SYM(_esf_t, k1);
GEN_OFFSET_SYM(_esf_t, gp);
GEN_OFFSET_SYM(_esf_t, sp);
GEN_OFFSET_SYM(_esf_t, fp);
GEN_OFFSET_SYM(_esf_t, ra);
GEN_OFFSET_SYM(_esf_t, hi);
GEN_OFFSET_SYM(_esf_t, lo);
GEN_OFFSET_SYM(_esf_t, epc);
GEN_OFFSET_SYM(_esf_t, sr);
GEN_OFFSET_SYM(_esf_t, cause);

GEN_ABSOLUTE_SYM(_esf_t_SIZEOF, sizeof(_esf_t));

GEN_OFFSET_SYM(_callee_saved_t, sp);

/* size of the entire preempt registers structure */

GEN_ABSOLUTE_SYM(___callee_saved_t_SIZEOF, sizeof(struct _callee_saved));

/* size of the struct tcs structure sans save area for floating point regs */
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF, sizeof(struct k_thread));

GEN_ABS_SYM_END
