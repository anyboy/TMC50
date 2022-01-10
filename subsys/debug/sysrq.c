/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Magic System Request Key Hacks
 *
 */

#include <kernel.h>
#include <uart.h>
#include <misc/printk.h>
#include <drivers/console/uart_console.h>
#include <stack_backtrace.h>
#include <soc_debug.h>
#include <soc_watchdog.h>

struct sysrq_key_op {
	char key;
	char reserved[3];

	void (*handler)(int);
	char *help_msg;
	char *action_msg;
};

#ifdef CONFIG_STACK_BACKTRACE
static void sysrq_handle_show_stack(int key)
{
	show_all_threads_stack();
}
#endif

#ifdef CONFIG_IRQ_STAT
static void sysrq_handle_show_irq_stats(int key)
{
	extern void dump_irqstat(void);

	dump_irqstat();
}
#endif

static void sysrq_handle_switch_jtag(int key)
{
	soc_debug_enable_jtag(SOC_JTAG_CPU0, CONFIG_CPU0_EJTAG_GROUP);
}

static const struct sysrq_key_op sysrq_key_table[] = {
#ifdef CONFIG_STACK_BACKTRACE
	{
		.key		= 't',
		.handler	= sysrq_handle_show_stack,
		.help_msg	= "show-thread-states(t)",
		.action_msg	= "Show State:",
	},
#endif

#ifdef CONFIG_IRQ_STAT
	{
		.key		= 'i',
		.handler	= sysrq_handle_show_irq_stats,
		.help_msg	= "irq_stats(i)",
		.action_msg	= "IRQ Stats",
	},
#endif

	{
		.key		= 'j',
		.handler	= sysrq_handle_switch_jtag,
		.help_msg	= "jtag(j)",
		.action_msg	= "Switch JTAG",
	},
};

/* magic sysrq key: CTLR + 'b' 'r' 'e' 'a' 'k' */
static const char sysrq_breakbuf[] = {0x02, 0x12, 0x05, 0x01, 0x0b};
static int sysrq_break_idx;

static struct sysrq_key_op* sysrq_get_key_op(char key)
{
	int i, cnt;

	cnt = ARRAY_SIZE(sysrq_key_table);

	for (i = 0; i < cnt; i++) {
		if (key == sysrq_key_table[i].key) {
			return (struct sysrq_key_op*)&sysrq_key_table[i];
		}
	}

	return NULL;
}

static void sysrq_print_help(void)
{
	int i;

	printk("HELP : quit(q) ");
	for (i = 0; i < ARRAY_SIZE(sysrq_key_table); i++) {
		printk("%s ", sysrq_key_table[i].help_msg);
	}
	printk("\n");
}

static void handle_sysrq_main(struct device * port)
{
	struct sysrq_key_op* op_p;
	char key;

#ifdef CONFIG_UART_CONSOLE_DEFAULT_DISABLE
	/* enable the uart console */
	uart_console_hook_install();
#endif

	/* disable watchdog */
	soc_watchdog_stop();

	for(;;) {
		printk("Wait SYSRQ Key:\n");

		sysrq_print_help();

		uart_poll_in(port, &key);
		if (key == 'q') {
			printk("sysrq: exit\n");
			break;
		}

		op_p = sysrq_get_key_op(key);
		if (op_p) {
			printk("%s\n", op_p->action_msg);
			op_p->handler(key);
		} else {
			sysrq_print_help();
		}
	}
}

void uart_handle_sysrq_char(struct device * port, char c)
{
	if (c == sysrq_breakbuf[sysrq_break_idx]) {
		sysrq_break_idx++;
		if (sysrq_break_idx == sizeof(sysrq_breakbuf)) {
			sysrq_break_idx = 0;
			handle_sysrq_main(port);
		}
	} else {
		sysrq_break_idx = 0;
	}
}
