/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <tracing/atrace/atrace_event_id.h>

#define ATRACE_KERNEL "atrace"

static int shell_cmd_ate_mid_mask(int argc, char *argv[])
{
	unsigned int mid_mask;

	if (argc < 2) {
		printk("atrace: event module mask 0x%x", ate_get_mid_mask());
		return 0;
	}

	mid_mask = strtoul(argv[1], NULL, 16);
	ate_set_mid_mask(mid_mask);

	printk("atrace: set event module mask to 0x%x", mid_mask);

	return 0;
}

static int shell_cmd_ate_sid_mask(int argc, char *argv[])
{
	unsigned int mid, sid_mask;

	if (argc < 2)
		return 0;

	mid = strtoul(argv[1], NULL, 10);
	if (mid > ATRACE_MAX_MODULE_ID)
		return 0;

	if (argc == 2) {
		printk("atrace: event module %d sub mask 0x%x", mid, ate_get_sid_mask(mid));
		return 0;
	}

	sid_mask = strtoul(argv[2], NULL, 16);
	ate_set_sid_mask(mid, sid_mask);

	printk("atrace: set event module %d sub mask to 0x%x", mid, sid_mask);

	return 0;
}

const struct shell_cmd atrace_commands[] = {
	{ "mid_mask", shell_cmd_ate_mid_mask, "set atrace event module mask" },
	{ "sid_mask", shell_cmd_ate_sid_mask, "set atrace event module sub mask" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(ATRACE_KERNEL, atrace_commands);
