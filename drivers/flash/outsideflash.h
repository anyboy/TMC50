/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for SPI memory (NOR & NAND & PSRAM)
 */


#include <kernel.h>

#define TFLAG_MIO_DATA				0x01
#define TFLAG_MIO_ADDR_DATA			0x02
#define TFLAG_MIO_CMD_ADDR_DATA		0x04
#define TFLAG_MIO_MASK				0x07
#define TFLAG_WRITE_DATA			0x08
#define TFLAG_ENABLE_RANDOMIZE		0x10
#define TFLAG_PAUSE_RANDOMIZE		0x20
#define TFLAG_RESUME_RANDOMIZE		0x40

void oflash_read_chipid(struct spi_config *config, void *chipid, s32_t len);
u8_t oflash_read_status(struct spi_config *config, u8_t cmd);

void oflash_read_page(struct spi_config *config,
		u32_t addr, s32_t addr_len,
		void *buf, s32_t len);
void oflash_continuous_read_reset(struct spi_config *config);

void oflash_set_write_protect(struct spi_config *config, bool protect);
int oflash_erase_block(struct spi_config *config, u32_t page);

int oflash_transfer(struct spi_config *config, u8_t cmd, u32_t addr,
		    s32_t addr_len, void *buf, s32_t length,
		    u8_t dummy_len, u32_t flag);

int oflash_write_cmd_addr(struct spi_config *config, u8_t cmd,
			  u32_t addr, s32_t addr_len);

int oflash_write_cmd(struct spi_config *config, u8_t cmd);

void oflash_write_status(struct spi_config *config, u8_t *sta_l,u8_t *sta_h);


