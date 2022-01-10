/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains defintions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
 *  struct _caller_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef _kernel_arch_thread__h_
#define _kernel_arch_thread__h_

/*
 * Reason a thread has relinquished control: fibers can only be in the NONE
 * or COOP state, tasks can be one in the four.
 */
#define _CAUSE_NONE 0
#define _CAUSE_COOP 1
#define _CAUSE_RIRQ 2
#define _CAUSE_FIRQ 3

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _caller_saved {
	/*
	 * Nothing here, the exception code puts all the caller-saved
	 * registers onto the stack.
	 */
};

typedef struct _caller_saved _caller_saved_t;

struct _callee_saved {
	 /*
	  * all register (_caller_saved & _callee_saved ) are saved in thread stack,
	  * and this is the saved stack pointer.
	  */
	uint32_t sp;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* interrupt key when relinquishing control */
	uint32_t intlock_key;

	/* return value from _Swap */
	unsigned int return_value;
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_thread__h_ */
