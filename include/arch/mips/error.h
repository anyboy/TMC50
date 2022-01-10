/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS public error handling
 *
 * MIPS-specific kernel error handling interface. Included by mips/arch.h.
 */

#ifndef _ARCH_MIPS_ERROR_H_
#define _ARCH_MIPS_ERROR_H_

#include <arch/mips/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void _NanoFatalErrorHandler(unsigned int,
						 const NANO_ESF*);
extern void _SysFatalErrorHandler(unsigned int, const NANO_ESF*);
#endif

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_CPU_EXCEPTION (4)    /* Unhandled exception */
#define _NANO_ERR_KERNEL_OOPS (5)      /* Kernel oops (fatal to thread) */
#ifdef __cplusplus
}
#endif

#endif /* _ARCH_MIPS_ERROR_H_ */
