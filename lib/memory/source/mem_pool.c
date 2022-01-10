/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem pool manager.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <atomic.h>
#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <stack_backtrace.h>
#include "mem_inner.h"

#define SYSTEM_MEM_POOL_MAXSIZE_BLOCK_NUMER 8
#define SYSTEM_MEM_POOL_MAX_BLOCK_SIZE      2048
#define SYSTEM_MEM_POOL_MIN_BLOCK_SIZE      16
#define SYSTEM_MEM_POOL_ALIGN_SIZE         4

K_MEM_POOL_DEFINE(system_mem_pool, SYSTEM_MEM_POOL_MIN_BLOCK_SIZE,\
  SYSTEM_MEM_POOL_MAX_BLOCK_SIZE, SYSTEM_MEM_POOL_MAXSIZE_BLOCK_NUMER,SYSTEM_MEM_POOL_ALIGN_SIZE);


void *mem_pool_malloc(unsigned int num_bytes)
{
	struct k_mem_block block;
	/*
	 * get a block large enough to hold an initial (hidden) block
	 * descriptor, as well as the space the caller requested
	 */
	num_bytes += sizeof(struct k_mem_block);
	if (k_mem_pool_alloc(&system_mem_pool, &block,
							num_bytes, K_MSEC(1000)) != 0) {
		return NULL;
	}

	/* save the block descriptor info at the start of the actual block */
	memset(block.data + sizeof(struct k_mem_block), 0,
				num_bytes - sizeof(struct k_mem_block));
	memcpy(block.data, &block, sizeof(struct k_mem_block));

	/* return address of the user area part of the block to the caller */
	return (char *)block.data + sizeof(struct k_mem_block);
}

void mem_pool_free(void *ptr)
{
	if (ptr != NULL) {
		/* point to hidden block descriptor at start of block */
		ptr = (char *)ptr - sizeof(struct k_mem_block);

		/* return block to the heap memory pool */
		k_mem_pool_free(ptr);
		k_mem_pool_defrag(&system_mem_pool);

	}
}


