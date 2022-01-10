/** @file bt_internal_variable.c
 * @brief Bluetooth internel variables.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <atomic.h>

#include "host_common_internal.h"

#if BT_HOST_MEM_DEBUG
#define HOST_MEM_DEBUG_MAX	100
static u32_t host_max_malloc_cut;
static u32_t host_cur_malloc_cut;
static u32_t host_max_memory_used;
static u32_t host_cur_memory_used;

static u32_t host_mem_debug[HOST_MEM_DEBUG_MAX][2];

static void host_mem_debug_malloc(void *addr, u32_t size)
{
	u32_t i;

	host_cur_malloc_cut++;
	if (host_cur_malloc_cut > host_max_malloc_cut) {
		host_max_malloc_cut = host_cur_malloc_cut;
	}

	host_cur_memory_used += size;
	if (host_cur_memory_used > host_max_memory_used) {
		host_max_memory_used = host_cur_memory_used;
	}

	for (i = 0; i < HOST_MEM_DEBUG_MAX; i++) {
		if (host_mem_debug[i][0] == 0) {
			host_mem_debug[i][0] = (u32_t)addr;
			host_mem_debug[i][1] = size;
			return;
		}
	}

	printk("not have enough index for debug, addr:%p\n", addr);
}

static void host_mem_debug_free(void *addr)
{
	u32_t i;

	host_cur_malloc_cut--;

	for (i = 0; i < HOST_MEM_DEBUG_MAX; i++) {
		if (host_mem_debug[i][0] == (u32_t)addr) {
			host_mem_debug[i][0] = 0;
			host_cur_memory_used -= host_mem_debug[i][1];
			return;
		}
	}

	printk("unknow addr:%p\n", addr);
}

void host_mem_debug_print(void)
{
	u32_t i, flag = 0;

	printk("host_max_malloc_cut:%d, host_cur_malloc_cut:%d\n", host_max_malloc_cut, host_cur_malloc_cut);
	printk("host_max_memory_used:%d, host_cur_memory_used:%d\n", host_max_memory_used, host_cur_memory_used);

	for (i = 0; i < HOST_MEM_DEBUG_MAX; i++) {
		if (host_mem_debug[i][0] != 0) {
			printk("Unfree mem addr:0x%x, size:%d\n", host_mem_debug[i][0], host_mem_debug[i][1]);
			flag = 1;
		}
	}

	if (!flag) {
		printk("All mem free!\n");
	}
}

void *host_malloc(u32_t num_bytes)
{
	void *ptr = mem_malloc(num_bytes);

	if (ptr) {
		host_mem_debug_malloc(ptr, num_bytes);
	}

	return ptr;
}

void host_free(void *ptr)
{
	host_mem_debug_free(ptr);
	mem_free(ptr);
}
#endif
