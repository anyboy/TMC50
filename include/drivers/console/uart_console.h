/* uart_console.h - uart console driver */

/*
 * Copyright (c) 2011, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UART_CONSOLE__H_
#define _UART_CONSOLE__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>

/** @brief Register uart input processing
 *
 *  Input processing is started when string is typed in the console.
 *  Carriage return is translated to NULL making string always NULL
 *  terminated. Application before calling register function need to
 *  initialize two fifo queues mentioned below.
 *
 *  @param avail k_fifo queue keeping available input slots
 *  @param lines k_fifo queue of entered lines which to be processed
 *         in the application code.
 *  @param completion callback for tab completion of entered commands
 *
 *  @return N/A
 */
void uart_register_input(struct k_fifo *avail, struct k_fifo *lines,
			 u8_t (*completion)(char *str, u8_t len));

/*
 * Allows having debug hooks in the console driver for handling incoming
 * control characters, and letting other ones through.
 */
#ifdef CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS
#define UART_CONSOLE_DEBUG_HOOK_HANDLED 1
#define UART_CONSOLE_OUT_DEBUG_HOOK_SIG(x) int(x)(char c)
typedef UART_CONSOLE_OUT_DEBUG_HOOK_SIG(uart_console_out_debug_hook_t);

void uart_console_out_debug_hook_install(
				uart_console_out_debug_hook_t *hook);

typedef int (*uart_console_in_debug_hook_t) (u8_t);

void uart_console_in_debug_hook_install(uart_console_in_debug_hook_t hook);

void uart_console_hook_install(void);

void uart_console_dummy_hook_install(void);

#endif

int uart_console_is_busy(void);

void uart_console_output(char *s, int len);

#ifdef CONFIG_UART_CONSOLE_DMA_SUPPORT
void uart_console_dma_switch(unsigned int use_dma, dma_stream_handle_t dma_handler, void *pdata);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _UART_CONSOLE__H_ */
