/*
 * Copyright (c) 2020 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system app shell.
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdio.h>
#include <init.h>
#include <string.h>
#include <kernel.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <property_manager.h>

#include "fm.h"


#define FM_SHELL "fmapp"

static int shell_set_tx_freq(int argc, char *argv[])
{
	int ret ,fm_freq = 0;

	//Read argv 1 and set to FM transmitter point
	fm_freq = atoi(argv[1]);

	ret = fm_tx_set_freq(fm_freq);
	if(ret) {
		SYS_LOG_INF("set fm tx fm_freq: %d err \n", fm_freq);
		return -1;
	}

	SYS_LOG_INF("set fm tx fm_freq: %d\n", fm_freq);
	return ret;
}

static const struct shell_cmd fmapp_commands[] = {
	{ "set_tx_freq", shell_set_tx_freq, "set fm tx module freq" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(FM_SHELL, fmapp_commands);
