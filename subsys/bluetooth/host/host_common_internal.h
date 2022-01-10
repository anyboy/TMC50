/** @file bt_internal_variable.h
 * @brief Bluetooth internel variables.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HOST_COMMON_INTERNAL_H__
#define __HOST_COMMON_INTERNAL_H__

#include <mem_manager.h>

#define		BT_HOST_MEM_DEBUG		0

#if BT_HOST_MEM_DEBUG
void *host_malloc(u32_t num_bytes);
void host_free(void *ptr);
void host_mem_debug_print(void);
#else
#define host_malloc(x)		mem_malloc(x)
#define host_free(x)		mem_free(x)
#define host_mem_debug_print()
#endif
#endif /* __HOST_COMMON_INTERNAL_H__ */
