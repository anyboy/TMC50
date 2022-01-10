/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for SPI Flash (NOR & NAND & PSRAM)
 */

#include <errno.h>
#include <kernel.h>
#include <init.h>
#include <string.h>
#include <linker/sections.h>
#include <flash.h>
#include <soc.h>
#include <misc/byteorder.h>
#include "xspi_mem_acts.h"

/* fix spimem ops, replace spimem ops with bugs which are in ic rom */

#ifdef CONFIG_XSPI_MEM_FIX

/* For exsample */
__ramfunc void spimem_read_page_fix(struct xspi_info *si, unsigned int addr, int addr_len, void *buf, int len)
{
	unsigned char cmd;
	unsigned int flag;
	unsigned char dummy_len;

    if (si->flag & XSPI_FLAG_USE_MIO_ADDR_DATA) {
		if (si->bus_width == 4) {
			cmd = XSPI_MEM_CMD_FAST_READ_X4IO;
			flag = XSPI_MEM_TFLAG_MIO_ADDR_DATA;
			dummy_len = 3;
		} else if (si->bus_width == 2) {
			cmd = XSPI_MEM_CMD_FAST_READ_X2IO;
			flag = XSPI_MEM_TFLAG_MIO_ADDR_DATA;
			dummy_len = 1;
		} else {
			cmd = XSPI_MEM_CMD_FAST_READ;
			flag = 0;
			dummy_len = 1;
		}
	} else {
		if (si->bus_width == 4) {
			cmd = XSPI_MEM_CMD_FAST_READ_X4;
			flag = XSPI_MEM_TFLAG_MIO_DATA;
			dummy_len = 1;
		} else if (si->bus_width == 2) {
			cmd = XSPI_MEM_CMD_FAST_READ_X2;
			flag = XSPI_MEM_TFLAG_MIO_DATA;
			dummy_len = 1;
		} else {
			cmd = XSPI_MEM_CMD_FAST_READ;
			flag = 0;
			dummy_len = 1;
		}
	}

	si->spimem_op_api->transfer(si, cmd, addr, addr_len, buf, len, dummy_len, flag);
}
/* For exsample END */

#endif
