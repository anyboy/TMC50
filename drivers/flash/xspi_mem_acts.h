/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for SPI memory (NOR & NAND & PSRAM)
 */

#ifndef __XSPI_MEM_ACTS_H__
#define __XSPI_MEM_ACTS_H__

#include <kernel.h>
#include "xspi_acts.h"

//#define CONFIG_XSPI_MEM_FIX

/* no need to use ramfunc if nor is conne./cted by spi1 */
#if defined(CONFIG_XSPI_NOR_ROM_ACTS) && defined(CONFIG_XSPI1_NOR_ACTS)
#define __XSPI_RAMFUNC
#else
#define __XSPI_RAMFUNC __ramfunc
#endif

#define XSPI_MEM_TFLAG_MIO_DATA			0x01
#define XSPI_MEM_TFLAG_MIO_ADDR_DATA		0x02
#define XSPI_MEM_TFLAG_MIO_CMD_ADDR_DATA	0x04
#define XSPI_MEM_TFLAG_MIO_MASK			0x07
#define XSPI_MEM_TFLAG_WRITE_DATA		0x08
#define XSPI_MEM_TFLAG_ENABLE_RANDOMIZE		0x10
#define XSPI_MEM_TFLAG_PAUSE_RANDOMIZE		0x20
#define XSPI_MEM_TFLAG_RESUME_RANDOMIZE		0x40

/* spimem commands */
#define	XSPI_MEM_CMD_READ_CHIPID				0x9f	/* JEDEC ID */

#define	XSPI_MEM_CMD_FAST_READ				0x0b	/* fast read */
#define	XSPI_MEM_CMD_FAST_READ_X2			0x3b	/* data x2 */
#define	XSPI_MEM_CMD_FAST_READ_X4			0x6b	/* data x4 */
#define	XSPI_MEM_CMD_FAST_READ_X2IO			0xbb	/* addr & data x2 */
#define	XSPI_MEM_CMD_FAST_READ_X4IO			0xeb	/* addr & data x4 */

#define	XSPI_MEM_CMD_ENABLE_WRITE			0x06	/* enable write */
#define	XSPI_MEM_CMD_DISABLE_WRITE			0x04	/* disable write */

#define	XSPI_MEM_CMD_WRITE_PAGE				0x02	/* write one page */
#define XSPI_MEM_CMD_ERASE_BLOCK				0xd8	/* 64KB erase */
#define XSPI_MEM_CMD_CONTINUOUS_READ_RESET	0xff	/* exit quad continuous_read mode */

#endif	/* __XSPI_MEM_ACTS_H__ */
