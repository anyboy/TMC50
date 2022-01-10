/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mem manager interface 
 */

#ifndef __MEM_MANAGER_H__
#define __MEM_MANAGER_H__

#include <kernel.h>
/**
 * @defgroup mem_manager_apis Mem Manager APIs
 * @ingroup system_apis
 * @{
 */
/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief Allocate memory from system mem heap .
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param num_bytes Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */

void * mem_malloc(unsigned int num_bytes);

/**
 * @brief Free memory allocated  system mem heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */

void mem_free(void *ptr);
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @brief Allocate memory from app mem heap .
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param num_bytes Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */

void * app_mem_malloc(unsigned int num_bytes);

/**
 * @brief Free memory allocated  app mem heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */

void app_mem_free(void *ptr);


/**
 * @brief dump mem info.
 *
 * This routine provides dump mem info .
 *
 * @return N/A
 */


void mem_manager_dump(void);

/**
 * @brief Allocate memory from system mem heap .
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param num_bytes Amount of memory requested (in bytes).
 * @param caller caller function address, use __builtin_return_address(0).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void *mem_malloc_debug(unsigned int num_bytes, void *caller);

/**
 * @} end defgroup mem_manager_apis
 */
#endif
