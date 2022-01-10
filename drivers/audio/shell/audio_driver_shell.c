/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio Driver Shell implementation
 */
#include <shell/shell.h>
#include <string.h>
#include "../phy_audio_common.h"

#define SYS_LOG_DOMAIN "A_SHELL"
#include <logging/sys_log.h>

#define SHELL_AUDIO_DRV "audio_drv"

static int shell_cmd_audio_dump_regs(int argc, char *argv[])
{
	struct phy_audio_device *phy_dev = NULL;

	if (argc != 2) {
		SYS_LOG_ERR("Invalid input");
		SYS_LOG_INF("usage: dump_regs [dac|i2stx|spdiftx|adc|i2srx|spdifrx]");
		return -1;
	}

	if (!strcmp(argv[1], "dac"))
		phy_dev = GET_AUDIO_DEVICE(dac);
	else if (!strcmp(argv[1], "i2stx"))
		phy_dev = GET_AUDIO_DEVICE(i2stx);
	else if (!strcmp(argv[1], "spdiftx"))
		phy_dev = GET_AUDIO_DEVICE(spdiftx);
	else if (!strcmp(argv[1], "adc"))
		phy_dev = GET_AUDIO_DEVICE(adc);
	else if (!strcmp(argv[1], "i2srx"))
		phy_dev = GET_AUDIO_DEVICE(i2srx);
	else if (!strcmp(argv[1], "spdifrx"))
		phy_dev = GET_AUDIO_DEVICE(spdifrx);

	if (phy_dev)
		phy_audio_control(phy_dev, PHY_CMD_DUMP_REGS, NULL);

	return 0;
}

const struct shell_cmd audiodrv_commands[] = {
    {"dump_regs", shell_cmd_audio_dump_regs, "Dump the audio channels registers"},
    { NULL, NULL, NULL }
};

SHELL_REGISTER(SHELL_AUDIO_DRV, audiodrv_commands);
