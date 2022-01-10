/*
 * Copyright (c) 2011-2012, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UART-driven console
 *
 *
 * Serial console driver.
 * Hooks into the printk and fputc (for printf) modules. Poll driven.
 */

#include <kernel.h>
#include <arch/cpu.h>

#include <stdio.h>
#include <zephyr/types.h>
#include <errno.h>
#include <ctype.h>

#include <device.h>
#include <init.h>
#include <soc.h>
#include <uart.h>
#include <console/console.h>
#include <console/uart_console.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <atomic.h>
#include <misc/printk.h>
#include <misc/sysrq.h>
#include <string.h>
#include <nvram_config.h>
#if defined(CONFIG_USB_UART_CONSOLE)
#include <console/uart_usb.h>
#endif

static struct device *uart_console_dev;

#ifdef CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS

static uart_console_in_debug_hook_t debug_hook_in;
void uart_console_in_debug_hook_install(uart_console_in_debug_hook_t hook)
{
	debug_hook_in = hook;
}

static UART_CONSOLE_OUT_DEBUG_HOOK_SIG(debug_hook_out_nop)
{
	ARG_UNUSED(c);
	return !UART_CONSOLE_DEBUG_HOOK_HANDLED;
}

static uart_console_out_debug_hook_t *debug_hook_out = debug_hook_out_nop;
void uart_console_out_debug_hook_install(uart_console_out_debug_hook_t *hook)
{
	debug_hook_out = hook;
}
#define HANDLE_DEBUG_HOOK_OUT(c) \
	(debug_hook_out(c) == UART_CONSOLE_DEBUG_HOOK_HANDLED)

#endif /* CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS */

#if 0 /* NOTUSED */
/**
 *
 * @brief Get a character from UART
 *
 * @return the character or EOF if nothing present
 */

static int console_in(void)
{
	unsigned char c;

	if (uart_poll_in(uart_console_dev, &c) < 0) {
		return EOF;
	} else {
		return (int)c;
	}
}
#endif

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
/**
 *
 * @brief Output one character to UART
 *
 * Outputs both line feed and carriage return in the case of a '\n'.
 *
 * @param c Character to output
 *
 * @return The character passed as input.
 */

static int console_out(int c)
{
#ifdef CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS

	int handled_by_debug_server = HANDLE_DEBUG_HOOK_OUT(c);

	if (handled_by_debug_server) {
		return c;
	}

#endif /* CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS */

	if ('\n' == c) {
		uart_poll_out(uart_console_dev, '\r');
	}
	uart_poll_out(uart_console_dev, c);

	return c;
}

#endif

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#else
#define __stdout_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#else
#define __printk_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

#if defined(CONFIG_CONSOLE_HANDLER)
static struct k_fifo *avail_queue;
static struct k_fifo *lines_queue;
static u8_t (*completion_cb)(char *line, u8_t len);

/* Control characters */
#define ESC                0x1b
#define DEL                0x7f
#define BACKTRACE          0x8

/* ANSI escape sequences */
#define ANSI_ESC           '['
#define ANSI_UP            'A'
#define ANSI_DOWN          'B'
#define ANSI_FORWARD       'C'
#define ANSI_BACKWARD      'D'
#define ANSI_END           'F'
#define ANSI_HOME          'H'
#define ANSI_DEL           '~'

#if !defined(CONFIG_USB_UART_CONSOLE)
static int read_uart(struct device *uart, u8_t *buf, unsigned int size)
{
	int rx;

	rx = uart_fifo_read(uart, buf, size);
	if (rx < 0) {
		/* Overrun issue. Stop the UART */
		uart_irq_rx_disable(uart);

		return -EIO;
	}

	return rx;
}
#endif

static inline void cursor_forward(unsigned int count)
{
	printk("\x1b[%uC", count);
}

static inline void cursor_backward(unsigned int count)
{
	printk("\x1b[%uD", count);
}

static inline void cursor_save(void)
{
	printk("\x1b[s");
}

static inline void cursor_restore(void)
{
	printk("\x1b[u");
}

static void insert_char(char *pos, char c, u8_t end)
{
	char tmp;

	/* Echo back to console */
#if defined(CONFIG_USB_UART_CONSOLE)
	uart_usb_send((const u8_t *)&c, 1);
#else
	uart_poll_out(uart_console_dev, c);
#endif

	if (end == 0) {
		*pos = c;
		return;
	}

	tmp = *pos;
	*(pos++) = c;

	cursor_save();

	while (end-- > 0) {
#if defined(CONFIG_USB_UART_CONSOLE)
		uart_usb_send((const u8_t *)&tmp, 1);
#else
		uart_poll_out(uart_console_dev, tmp);
#endif
		c = *pos;
		*(pos++) = tmp;
		tmp = c;
	}

	/* Move cursor back to right place */
	cursor_restore();
}

static void del_char(char *pos, u8_t end)
{
#if defined(CONFIG_USB_UART_CONSOLE)
	char c = '\b';
	uart_usb_send((const u8_t *)&c, 1);
#else
	uart_poll_out(uart_console_dev, '\b');
#endif

	if (end == 0) {
#if defined(CONFIG_USB_UART_CONSOLE)
		const u8_t *str = " \b";
		uart_usb_send(str, strlen(str));
#else
		uart_poll_out(uart_console_dev, ' ');
		uart_poll_out(uart_console_dev, '\b');
#endif
		return;
	}

	cursor_save();

	while (end-- > 0) {
		*pos = *(pos + 1);
#if defined(CONFIG_USB_UART_CONSOLE)
		uart_usb_send(pos++, 1);
#else
		uart_poll_out(uart_console_dev, *(pos++));
#endif
	}

#if defined(CONFIG_USB_UART_CONSOLE)
	c = ' ';
	uart_usb_send((const u8_t *)&c, 1);
#else
	uart_poll_out(uart_console_dev, ' ');
#endif

	/* Move cursor back to right place */
	cursor_restore();
}

enum {
	ESC_ESC,
	ESC_ANSI,
	ESC_ANSI_FIRST,
	ESC_ANSI_VAL,
	ESC_ANSI_VAL_2
};

static atomic_t esc_state;
static unsigned int ansi_val, ansi_val_2;
static u8_t cur, end;

static void handle_ansi(u8_t byte, char *line)
{
	if (atomic_test_and_clear_bit(&esc_state, ESC_ANSI_FIRST)) {
		if (!isdigit(byte)) {
			ansi_val = 1;
			goto ansi_cmd;
		}

		atomic_set_bit(&esc_state, ESC_ANSI_VAL);
		ansi_val = byte - '0';
		ansi_val_2 = 0;
		return;
	}

	if (atomic_test_bit(&esc_state, ESC_ANSI_VAL)) {
		if (isdigit(byte)) {
			if (atomic_test_bit(&esc_state, ESC_ANSI_VAL_2)) {
				ansi_val_2 *= 10;
				ansi_val_2 += byte - '0';
			} else {
				ansi_val *= 10;
				ansi_val += byte - '0';
			}
			return;
		}

		/* Multi value sequence, e.g. Esc[Line;ColumnH */
		if (byte == ';' &&
		    !atomic_test_and_set_bit(&esc_state, ESC_ANSI_VAL_2)) {
			return;
		}

		atomic_clear_bit(&esc_state, ESC_ANSI_VAL);
		atomic_clear_bit(&esc_state, ESC_ANSI_VAL_2);
	}

ansi_cmd:
	switch (byte) {
	case ANSI_BACKWARD:
		if (ansi_val > cur) {
			break;
		}

		end += ansi_val;
		cur -= ansi_val;
		cursor_backward(ansi_val);
		break;
	case ANSI_FORWARD:
		if (ansi_val > end) {
			break;
		}

		end -= ansi_val;
		cur += ansi_val;
		cursor_forward(ansi_val);
		break;
	case ANSI_HOME:
		if (!cur) {
			break;
		}

		cursor_backward(cur);
		end += cur;
		cur = 0;
		break;
	case ANSI_END:
		if (!end) {
			break;
		}

		cursor_forward(end);
		cur += end;
		end = 0;
		break;
	case ANSI_DEL:
		if (!end) {
			break;
		}

		cursor_forward(1);
		del_char(&line[cur], --end);
		break;
	default:
		break;
	}

	atomic_clear_bit(&esc_state, ESC_ANSI);
}

void uart_console_isr(struct device *unused)
{
	ARG_UNUSED(unused);

#if defined(CONFIG_USB_UART_CONSOLE)
	static u8_t _cmdline[CONSOLE_MAX_LINE_LEN];
	static u16_t _cmdline_index = 0;
#endif

	while (uart_irq_update(uart_console_dev) &&
	       uart_irq_is_pending(uart_console_dev)) {
		static struct console_input *cmd;
		u8_t byte;
		int rx;

#if defined(CONFIG_USB_UART_CONSOLE)
		if (uart_irq_tx_ready(uart_console_dev)) {
			uart_usb_update_tx_done();
		}
#endif

		if (!uart_irq_rx_ready(uart_console_dev)) {
			continue;
		}

		if (!cmd) {
			cmd = k_fifo_get(avail_queue, K_NO_WAIT);
			if (!cmd) {
				return;
			}
		}

		/* Character(s) have been received */
#if defined(CONFIG_USB_UART_CONSOLE)
		if (_cmdline_index >= sizeof(_cmdline)) {
			/* drain all data */
			while (uart_usb_receive(_cmdline, sizeof(_cmdline)) > 0)
			_cmdline_index = 0;
			return;
		}
		rx = uart_usb_receive(_cmdline + _cmdline_index,
				sizeof(_cmdline) - _cmdline_index);
		if (rx <= 0) {
			return;
		}

		if (rx == 1) {
			byte = _cmdline[0];
		} else {
			if (_cmdline[rx + _cmdline_index - 1] == '\r') {
				byte = '\r';
			} else if ((_cmdline[rx + _cmdline_index - 1] == '\n')
						&& (_cmdline[rx + _cmdline_index - 2] == '\r')) {
				--rx;
				_cmdline[rx + _cmdline_index - 1] = '\r';
				byte = '\r';
			} else {
				_cmdline_index += rx;
				return;
			}
			memcpy(cmd->line, _cmdline, strlen(_cmdline));
			cur = rx + _cmdline_index - 1;
		}
		_cmdline_index = 0;
#else
		rx = read_uart(uart_console_dev, &byte, 1);
		if (rx < 0) {
			return;
		}
#endif

#ifdef CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS
		if (debug_hook_in != NULL && debug_hook_in(byte) != 0) {
			/*
			 * The input hook indicates that no further processing
			 * should be done by this handler.
			 */
			return;
		}
#endif

		/* Check SYSRQ char */
		uart_handle_sysrq_char(uart_console_dev, byte);

		/* Handle ANSI escape mode */
		if (atomic_test_bit(&esc_state, ESC_ANSI)) {
			handle_ansi(byte, cmd->line);
			continue;
		}

		/* Handle escape mode */
		if (atomic_test_and_clear_bit(&esc_state, ESC_ESC)) {
			if (byte == ANSI_ESC) {
				atomic_set_bit(&esc_state, ESC_ANSI);
				atomic_set_bit(&esc_state, ESC_ANSI_FIRST);
			}

			continue;
		}

		/* Handle special control characters */
		if (!isprint(byte)) {
			switch (byte) {
			case BACKTRACE:
			case DEL:
				if (cur > 0) {
					del_char(&cmd->line[--cur], end);
				}
				break;
			case ESC:
				atomic_set_bit(&esc_state, ESC_ESC);
				break;
			case '\r':
				cmd->line[cur + end] = '\0';
#if defined(CONFIG_USB_UART_CONSOLE)
				const u8_t *str = "\r\n";
				uart_usb_send(str, strlen(str));
#else
				uart_poll_out(uart_console_dev, '\r');
				uart_poll_out(uart_console_dev, '\n');
#endif
				cur = 0;
				end = 0;
				k_fifo_put(lines_queue, cmd);
				cmd = NULL;
				break;
			case '\t':
				if (completion_cb && !end) {
					cur += completion_cb(cmd->line, cur);
				}
				break;
			default:
				break;
			}

			continue;
		}

		/* Ignore characters if there's no more buffer space */
		if (cur + end < sizeof(cmd->line) - 1) {
			insert_char(&cmd->line[cur++], byte, end);
		}
	}
}

void console_input_init(void)
{
	u8_t c;

	if (!uart_console_dev)
		uart_console_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

	uart_irq_rx_disable(uart_console_dev);
	uart_irq_tx_disable(uart_console_dev);

	uart_irq_callback_set(uart_console_dev, uart_console_isr);
	uart_irq_rx_exception_detection_set(uart_console_dev, true);

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart_console_dev)) {
		uart_fifo_read(uart_console_dev, &c, 1);
	}

	uart_irq_rx_enable(uart_console_dev);

}

void uart_register_input(struct k_fifo *avail, struct k_fifo *lines,
			 u8_t (*completion)(char *str, u8_t len))
{
	avail_queue = avail;
	lines_queue = lines;
	completion_cb = completion;

	console_input_init();
}

#else
#define console_input_init(x)			\
	do {/* nothing */			\
	} while ((0))
#define uart_register_input(x)			\
	do {/* nothing */			\
	} while ((0))
#endif

/**
 *
 * @brief Install printk/stdout hook for UART console output
 *
 * @return N/A
 */

void uart_console_hook_install(void)
{
	__stdout_hook_install(console_out);
	__printk_hook_install(console_out);
}

static int __dummy_console_out(int c)
{
    return c;
}

void uart_console_dummy_hook_install(void)
{
	__stdout_hook_install(__dummy_console_out);
	__printk_hook_install(__dummy_console_out);
}

#ifdef CONFIG_UART_CONSOLE_DEFAULT_DISABLE
int uart_console_force_init(struct device *arg)
{
	char temp[5];
	bool force_enable_uart = false;

	memset(temp, 0, sizeof(temp));

	if(nvram_config_get("FORCE_ENABLE_UART_CONSOLE", temp, sizeof(temp)) > 0)
	{
		if(!strcmp(temp,"true"))
		{
			force_enable_uart = true;
		}
	}

	if(force_enable_uart)
	{
		uart_console_hook_install();
	}

	return 0;
}
#endif

/**
 *
 * @brief Initialize one UART as the console/debug port
 *
 * @return 0 if successful, otherwise failed.
 */
static int uart_console_init(struct device *arg)
{
	ARG_UNUSED(arg);

	if (!uart_console_dev)
		uart_console_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

#if defined(CONFIG_USB_UART_CONSOLE) && defined(CONFIG_USB_UART_DTR_WAIT)
	while (1) {
		u32_t dtr = 0;
		uart_line_ctrl_get(uart_console_dev, LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}
	}
	k_busy_wait(1000000);
#endif

#ifdef CONFIG_UART_CONSOLE_DEFAULT_DISABLE
	uart_console_dummy_hook_install();
#else
	uart_console_hook_install();
#endif

	return 0;
}

#ifdef CONFIG_UART_CONSOLE_DMA_SUPPORT

static bool console_use_dma;

void uart_console_dma_switch(unsigned int use_dma, dma_stream_handle_t dma_handler, void *pdata)
{
#if defined(CONFIG_USB_UART_CONSOLE)
	console_use_dma = false;
#else
    if(console_use_dma == use_dma){
        return;
    }

    if(use_dma){
        uart_fifo_switch(uart_console_dev, 1, UART_FIFO_DMA);

        uart_dma_send_init(uart_console_dev, CONFIG_UART_CONSOLE_DMA_CHANNEL, \
            dma_handler, pdata);
    }else{
        uart_dma_send_stop(uart_console_dev);

    	uart_fifo_switch(uart_console_dev, 1, UART_FIFO_CPU);

    	__printk_hook_install(console_out);
    }

    console_use_dma = use_dma;
#endif
}

void uart_puts(char *s, int len)
{
    while (len != 0){
        console_out(*s);
        s++;
        len--;
    }
}

void uart_console_output(char *s, int len)
{
    if (console_use_dma) {
        uart_dma_send(uart_console_dev, s, len);
    } else {
        uart_puts(s, len);
    }
}

int uart_console_is_busy(void)
{
    if(console_use_dma){
        return (uart_dma_send_complete(uart_console_dev) == 0);
    }else{
        return 0;
    }
}

#ifdef CONFIG_TRACE_CONTEXT_SHELLTASK
int uart_console_output_trigger(uint32_t msg_id)
{
    static struct console_input *cmd;

    cmd = k_fifo_get(avail_queue, K_NO_WAIT);

    if(cmd){
        cmd->line[0] = msg_id;

        k_fifo_put(lines_queue, cmd);

        return 0;
    }else{
        return -ENOMEM;
    }
}
#endif

#endif

/* UART console initializes after the UART device itself */
SYS_INIT(uart_console_init,
#if defined(CONFIG_USB_UART_CONSOLE)
			APPLICATION,
#elif defined(CONFIG_EARLY_CONSOLE)
			PRE_KERNEL_1,
#else
			POST_KERNEL,
#endif
			CONFIG_UART_CONSOLE_INIT_PRIORITY);
#ifdef CONFIG_UART_CONSOLE_DEFAULT_DISABLE
SYS_INIT(uart_console_force_init,
#if defined(CONFIG_USB_UART_CONSOLE)
		 APPLICATION,
#elif defined(CONFIG_EARLY_CONSOLE)
		 PRE_KERNEL_2,
#else
		 POST_KERNEL,
#endif
		 CONFIG_UART_CONSOLE_INIT_PRIORITY);
#endif
