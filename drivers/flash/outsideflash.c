/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "errno.h"
#include <misc/byteorder.h>
#include <misc/printk.h>
#include <gpio.h>

#define SYS_LOG_DOMAIN "outsideflash"
#define SYS_LOG_LEVEL 4
#include <logging/sys_log.h>
#include <spi.h>
#include <outsideflash.h>

#define	CMD_READ_CHIPID		    0x9f	/* JEDEC ID */

#define	CMD_FAST_READ			0x0b	/* fast read */
#define	CMD_FAST_READ_X2		0x3b	/* data x2 */
#define	CMD_FAST_READ_X4		0x6b	/* data x4 */
#define	CMD_FAST_READ_X2IO		0xbb	/* addr & data x2 */
#define	CMD_FAST_READ_X4IO		0xeb	/* addr & data x4 */

#define	CMD_ENABLE_WRITE		0x06	/* enable write */
#define	CMD_DISABLE_WRITE		0x04	/* disable write */

#define	CMD_WRITE_PAGE			0x02	/* write one page */
#define CMD_ERASE_BLOCK		    0xd8	/* 64KB erase */
#define CMD_CONTINUOUS_READ_RESET	0xff	/* exit quad continuous_read mode */

#define XSPI_NOR_CMD_WRITE_STATUS2 0x31
#define XSPI_NOR_CMD_WRITE_STATUS 0x01

void oflash_continuous_read_reset(struct spi_config *config)
{
	struct spi_buf tx_buf[2] = {{NULL, 0}, {NULL, 0}};
	u8_t buf[1] = {CMD_CONTINUOUS_READ_RESET};

	tx_buf[0].buf = buf;
	tx_buf[0].len = 1;
	tx_buf[1].buf = buf;
	tx_buf[1].len = 1;

	spi_transceive(config, tx_buf, 2, NULL, 0);
}

int oflash_transfer(struct spi_config *config, u8_t cmd, u32_t addr,
					s32_t addr_len, void *buf, s32_t length,
					u8_t dummy_len, u32_t flag)
{
	u32_t addr_be;
	u8_t merge_buf[10] = {0};
	struct spi_buf tx_buf[2] = {{NULL, 0}, {NULL, 0}};
	struct spi_buf rx_buf[2] = {{NULL, 0}, {NULL, 0}};

	if (addr_len > 0)
		sys_memcpy_swap(&addr_be, &addr, addr_len);

	merge_buf[0] = cmd;

	if (addr_len == 3) {
		merge_buf[1] = addr_be/256/256%256;
		merge_buf[2] = addr_be/256%256;
		merge_buf[3] = addr_be%256;
	}
	if (addr_len == 4) {
		merge_buf[1] = addr_be/256/256/256%256;
		merge_buf[2] = addr_be/256/256%256;
		merge_buf[3] = addr_be/256%256;
		merge_buf[4] = addr_be%256;
	}

	tx_buf[0].buf = merge_buf;
	tx_buf[0].len = 1+addr_len+dummy_len;
	if (length > 0) {
		if (flag) {
			tx_buf[1].buf = buf;
			tx_buf[1].len = length;
			spi_transceive(config, tx_buf, 2, NULL, 0);

		} else {
			rx_buf[0].buf = NULL;
			rx_buf[0].len = 1+addr_len+dummy_len;
			rx_buf[1].buf = buf;
			rx_buf[1].len = length;
			spi_transceive( config, tx_buf, 1, rx_buf, 2);

		}
	} else {
		spi_transceive(config, tx_buf, 1, NULL, 0);
	}

	return 0;
}

int oflash_write_cmd_addr(struct spi_config *config, u8_t cmd,
					u32_t addr, s32_t addr_len)
{
	return oflash_transfer(config, cmd, addr, addr_len, NULL, 0, 0, 0);
}

int oflash_write_cmd(struct spi_config *config, u8_t cmd)
{
	return oflash_write_cmd_addr(config, cmd, 0, 0);
}

void oflash_read_chipid(struct spi_config *config, void *chipid, s32_t len)
{
	oflash_transfer(config, CMD_READ_CHIPID, 0, 0, chipid, len, 0, 0);
}

u8_t oflash_read_status(struct spi_config *config, u8_t cmd)
{
	u8_t status;

	oflash_transfer(config, cmd, 0, 0, &status, 1, 0, 0);

	return status;
}
void oflash_write_status(struct spi_config *config, u8_t *sta_l,u8_t *sta_h)
{
	u8_t cmd;
	u16_t status = (*sta_h << 8) | *sta_l;
	if(sta_h!=NULL) {
		cmd = XSPI_NOR_CMD_WRITE_STATUS2;
		oflash_transfer(config, cmd, 0, 0, sta_h, 1, 0, 1);
	}

	if(sta_l!=NULL) {
		cmd = XSPI_NOR_CMD_WRITE_STATUS;
		oflash_transfer(config, cmd, 0, 0, sta_l, 1, 0, 1);
	}

	if(sta_h!=NULL&&sta_l!=NULL) {
		cmd = XSPI_NOR_CMD_WRITE_STATUS;
		oflash_transfer(config, cmd, 0, 0, &status, 2, 0, 1);
	}

}

void oflash_set_write_protect(struct spi_config *config, bool protect)
{
	if (protect)
		oflash_write_cmd(config, CMD_DISABLE_WRITE);
	else
		oflash_write_cmd(config, CMD_ENABLE_WRITE);
}

void oflash_read_page(struct spi_config *config,
		u32_t addr, s32_t addr_len,
		void *buf, s32_t len)
{
	u8_t cmd;
	u32_t flag;

	cmd = CMD_FAST_READ;
	flag = 0;
	oflash_transfer(config, cmd, addr, addr_len, buf, len, 1, flag);
}

int oflash_erase_block(struct spi_config *config, u32_t addr)
{
	oflash_set_write_protect(config, false);
	oflash_write_cmd_addr(config, CMD_ERASE_BLOCK, addr, 3);

	return 0;
}

