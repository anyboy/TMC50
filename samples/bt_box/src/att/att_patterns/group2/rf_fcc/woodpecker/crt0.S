/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mips32.h"
#include "cpu.h"
#include "regdef.h"
#include "m32c0.h"
#include "asm.h"

.global __bss_start
.global __bss_end
.global pattern_main

LEAF(_crt0_start)

	addi  sp,sp,-4
	sw    ra,0(sp)

	j	  _start
	nop

_start:

	la	t0, __bss_start
	la	t1, __bss_end
1:
	sw	zero, 0(t0)
	addi	t0, 4
	blt	t0, t1, 1b
	nop

	jalx	pattern_main
	nop

	lw	 ra,0(sp)
	addi sp,sp,4

	jr	 ra
	nop
END (_crt0_start)
