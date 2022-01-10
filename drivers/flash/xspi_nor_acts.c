/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPINOR Flash driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <init.h>
#include <string.h>
#include <linker/sections.h>
#include <flash.h>
#include <soc.h>
#include <spi.h>

#include <soc_rom_xspi.h>
#include "xspi_acts.h"
#include "xspi_mem_acts.h"

#define SYS_LOG_DOMAIN "xspi_nor"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

/* spinor parameters */
#define XSPI_NOR_WRITE_PAGE_SIZE_BITS	8
#define XSPI_NOR_ERASE_SECTOR_SIZE_BITS	12
#define XSPI_NOR_ERASE_BLOCK_SIZE_BITS	16

#define XSPI_NOR_WRITE_PAGE_SIZE	(1u << XSPI_NOR_WRITE_PAGE_SIZE_BITS)
#define XSPI_NOR_ERASE_SECTOR_SIZE	(1u << XSPI_NOR_ERASE_SECTOR_SIZE_BITS)
#define XSPI_NOR_ERASE_BLOCK_SIZE	(1u << XSPI_NOR_ERASE_BLOCK_SIZE_BITS)

#define XSPI_NOR_WRITE_PAGE_MASK	(XSPI_NOR_WRITE_PAGE_SIZE - 1)
#define XSPI_NOR_ERASE_SECTOR_MASK	(XSPI_NOR_ERASE_SECTOR_SIZE - 1)
#define XSPI_NOR_ERASE_BLOCK_MASK	(XSPI_NOR_ERASE_BLOCK_SIZE - 1)

/* spinor commands */
#define	XSPI_NOR_CMD_WRITE_PAGE		0x02	/* write one page */
#define	XSPI_NOR_CMD_DISABLE_WRITE	0x04	/* disable write */
#define	XSPI_NOR_CMD_READ_STATUS	0x05	/* read status1 */
#define	XSPI_NOR_CMD_READ_STATUS2	0x35	/* read status2 */
#define	XSPI_NOR_CMD_READ_STATUS3	0x15	/* read status3 */
#define	XSPI_NOR_CMD_WRITE_STATUS	0x01	/* write status1 */
#define	XSPI_NOR_CMD_WRITE_STATUS2	0x31	/* write status2 */
#define	XSPI_NOR_CMD_WRITE_STATUS3	0x11	/* write status3 */
#define	XSPI_NOR_CMD_ENABLE_WRITE	0x06	/* enable write */
#define	XSPI_NOR_CMD_FAST_READ		0x0b	/* fast read */
#define XSPI_NOR_CMD_ERASE_SECTOR	0x20	/* 4KB erase */
#define XSPI_NOR_CMD_ERASE_BLOCK_32K	0x52	/* 32KB erase */
#define XSPI_NOR_CMD_ERASE_BLOCK	0xd8	/* 64KB erase */
#define	XSPI_NOR_CMD_READ_CHIPID	0x9f	/* JEDEC ID */
#define	XSPI_NOR_CMD_DISABLE_QSPI	0xff	/* disable QSPI */

/* NOR Flash vendors ID */
#define XSPI_NOR_MANU_ID_ALLIANCE	0x52	/* Alliance Semiconductor */
#define XSPI_NOR_MANU_ID_AMD		0x01	/* AMD */
#define XSPI_NOR_MANU_ID_AMIC		0x37	/* AMIC */
#define XSPI_NOR_MANU_ID_ATMEL		0x1f	/* ATMEL */
#define XSPI_NOR_MANU_ID_CATALYST	0x31	/* Catalyst */
#define XSPI_NOR_MANU_ID_ESMT		0x1c	/* ESMT */  /* change from 0x8c to 0x1c*/
//#define XSPI_NOR_MANU_ID_EON		0x1c	/* EON */   /* error */
#define XSPI_NOR_MANU_ID_FD_MICRO	0xa1	/* shanghai fudan microelectronics */
#define XSPI_NOR_MANU_ID_FIDELIX	0xf8	/* FIDELIX */
#define XSPI_NOR_MANU_ID_FMD		0x0e	/* Fremont Micro Device(FMD) */
#define XSPI_NOR_MANU_ID_FUJITSU	0x04	/* Fujitsu */
#define XSPI_NOR_MANU_ID_GIGADEVICE	0xc8	/* GigaDevice */
#define XSPI_NOR_MANU_ID_GIGADEVICE2	0x51	/* GigaDevice2 */
#define XSPI_NOR_MANU_ID_HYUNDAI	0xad	/* Hyundai */
#define XSPI_NOR_MANU_ID_INTEL		0x89	/* Intel */
#define XSPI_NOR_MANU_ID_MACRONIX	0xc2	/* Macronix (MX) */
#define XSPI_NOR_MANU_ID_NANTRONIC	0xd5	/* Nantronics */
#define XSPI_NOR_MANU_ID_NUMONYX	0x20	/* Numonyx, Micron, ST */
#define XSPI_NOR_MANU_ID_PMC		0x9d	/* PMC */
#define XSPI_NOR_MANU_ID_SANYO		0x62	/* SANYO */
#define XSPI_NOR_MANU_ID_SHARP		0xb0	/* SHARP */
#define XSPI_NOR_MANU_ID_SPANSION	0x01	/* SPANSION */
#define XSPI_NOR_MANU_ID_SST		0xbf	/* SST */
#define XSPI_NOR_MANU_ID_SYNCMOS_MVC	0x40	/* SyncMOS (SM) and Mosel Vitelic Corporation (MVC) */
#define XSPI_NOR_MANU_ID_TI		0x97	/* Texas Instruments */
#define XSPI_NOR_MANU_ID_WINBOND	0xda	/* Winbond */
#define XSPI_NOR_MANU_ID_WINBOND_NEX	0xef	/* Winbond (ex Nexcom) */
#define XSPI_NOR_MANU_ID_ZH_BERG	0xe0	/* ZhuHai Berg microelectronics (Bo Guan) */
#define XSPI_NOR_MANU_ID_XTX        0x0b    /* XTX */
#define XSPI_NOR_MANU_ID_BOYA       0x68    /* BOYA */
#define XSPI_NOR_MANU_ID_XMC        0x20    /* XMC */
#define XSPI_NOR_MANU_ID_PUYA       0x85    /* PUYA */
#define XSPI_NOR_MANU_ID_DS         0xff    /* Zbit Semiconductor */  /* FIX ME */

#define SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY		(1 << 0)
#define SPINOR_FLAG_DATA_PROTECTION_SUPPORT     (1 << 16)
#define SPINOR_FLAG_DATA_PROTECTION_OPENED      (1 << 17)

//#define CONFIG_SPINOR_TEST_DELAYCHAIN
//#define CONFIG_XSPI_NOR_ACTS_IO_BUS_WIDTH_TRY_TEST

//#define CONFIG_XSPI_NOR_FIX

struct xspi_nor_info {
	/* struct must be compatible with rom code */
	struct xspi_info spi;
	u32_t chipid;
	u32_t flag;

	struct spinor_rom_operation_api *spinor_api;
	u8_t clock_id;
	u8_t reset_id;
	u8_t dma_id;
	s8_t dma_chan;
	u32_t spiclk_reg;

#ifdef CONFIG_OUTSIDE_FLASH
	struct spi_config oflash_config;
#endif
};

static struct xspi_nor_info system_xspi_nor;

#ifdef CONFIG_OUTSIDE_FLASH

#include "outsideflash.h"
#include "board.h"
#define DRIVER_LAYER 1
#define CONTROLLER_LAYER 0
#define MISO_FUNC 0x3025
#define DC_SW_FUNC 0x2000060

int outside_nor_dc_pad_func_switch(int cmd, int pin){
	if(sys_read32(0xc0090000+pin*4)==MISO_FUNC)
		return 0;
	sys_write32(MISO_FUNC,0xc0090000+pin*4);
	return 0;
}
struct device *oflash_spi;
struct spi_cd_miso outside_nor_dc;
struct spi_cs_control outsideflash_cs;

u32_t outsides_nor_read_chipid(struct spi_config *config)
{
	u8_t chipid;
	config->dev=oflash_spi;

	oflash_read_chipid(config,&chipid, 3);

	return chipid;
}

int outsides_nor_read_status(struct spi_config *config, u8_t cmd)
{
	config->dev=oflash_spi;

	return oflash_read_status(config, cmd);
}
static int outsides_nor_wait_ready(struct spi_config *config)
{
	u8_t status;
	config->dev=oflash_spi;

	while (1) {
		status = oflash_read_status(config, XSPI_NOR_CMD_READ_STATUS);
		if (!(status & 0x1))
			break;
	}
	return 0;
}

void outsides_nor_write_data(struct spi_config *config, u8_t cmd,
			u32_t addr, s32_t addr_len, const u8_t *buf, s32_t len)
{
	config->dev=oflash_spi;
	oflash_set_write_protect(config, false);
	oflash_transfer(config, cmd, addr, addr_len, (u8_t *)buf, len,
			0, 1);
	outsides_nor_wait_ready(config);
}

int  outsides_nor_read_internal(struct spi_config *config,
			u32_t addr, u8_t *buf, int len)
{
	config->dev=oflash_spi;
	oflash_read_page(config, addr, 3, buf, len);
	return 0;
}

int outsides_nor_write_internal(struct spi_config *config, off_t addr,
				   const void *data, size_t len)
{

	size_t unaligned_len, remaining, write_size;

	if (addr & XSPI_NOR_WRITE_PAGE_MASK)
		unaligned_len = XSPI_NOR_WRITE_PAGE_SIZE - (addr & XSPI_NOR_WRITE_PAGE_MASK);
	else
		unaligned_len = 0;

	remaining = len;
	while (remaining > 0) {
		if (unaligned_len) {
			if (unaligned_len > len)
				write_size = len;
			else
				write_size = unaligned_len;
			unaligned_len = 0;
		} else if (remaining < XSPI_NOR_WRITE_PAGE_SIZE)
			write_size = remaining;
		else
			write_size = XSPI_NOR_WRITE_PAGE_SIZE;

		outsides_nor_write_data(config, XSPI_NOR_CMD_WRITE_PAGE, addr, 3, data, write_size);

		addr += write_size;
		data += write_size;
		remaining -= write_size;
	}

	return 0;
}

int outsides_nor_erase_internal(struct spi_config *config, off_t addr, size_t size)
{
	size_t remaining, erase_size;
	u8_t cmd;

	/* write aligned page data */
	remaining = size;
	while (remaining > 0) {
		if (addr & XSPI_NOR_ERASE_BLOCK_MASK || remaining < XSPI_NOR_ERASE_BLOCK_SIZE) {
			cmd = XSPI_NOR_CMD_ERASE_SECTOR;
			erase_size = XSPI_NOR_ERASE_SECTOR_SIZE;
		} else {
			cmd = XSPI_NOR_CMD_ERASE_BLOCK;
			erase_size = XSPI_NOR_ERASE_BLOCK_SIZE;
		}

		outsides_nor_write_data(config, cmd, addr, 3, NULL, 0);

		addr += erase_size;
		remaining -= erase_size;
	}

	return 0;
}

#endif//CONFIG_OUTSIDE_FLASH

void xspi_nor_dump_info(struct xspi_nor_info *sni)
{
#ifdef CONFIG_XSPI_NOR_ACTS_DUMP_INFO

	u32_t chipid, status, status2, status3;

#ifdef CONFIG_OUTSIDE_FLASH

	if(sni->oflash_config.cs) {

		chipid = outsides_nor_read_chipid(&sni->oflash_config);

		SYS_LOG_INF("SPINOR: chipid: 0x%02x 0x%02x 0x%02x", chipid & 0xff, (chipid >> 8) & 0xff, (chipid >> 16) & 0xff);

		status = outsides_nor_read_status(&sni->oflash_config, XSPI_NOR_CMD_READ_STATUS);
		status2 = outsides_nor_read_status(&sni->oflash_config, XSPI_NOR_CMD_READ_STATUS2);
		status3 = outsides_nor_read_status(&sni->oflash_config, XSPI_NOR_CMD_READ_STATUS3);

		SYS_LOG_INF("SPINOR: status: 0x%02x 0x%02x 0x%02x", status, status2, status3);

	}else {

#endif
		chipid = sni->spinor_api->read_chipid(sni);
		SYS_LOG_INF("SPINOR: chipid: 0x%02x 0x%02x 0x%02x",
			chipid & 0xff,
			(chipid >> 8) & 0xff,
			(chipid >> 16) & 0xff);

		status = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		status2 = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
		status3 = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

		SYS_LOG_INF("SPINOR: status: 0x%02x 0x%02x 0x%02x",
			status, status2, status3);

#ifdef CONFIG_OUTSIDE_FLASH
	}
#endif

#endif//CONFIG_XSPI_NOR_ACTS_DUMP_INFO
}


static void xspi_nor_enable_status_qe(struct xspi_nor_info *sni)
{
	uint16_t status;

	/* MACRONIX's spinor has different QE bit */
	if (XSPI_NOR_MANU_ID_MACRONIX == (sni->chipid & 0xff)) {
		status = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		if (!(status & 0x40)) {
			/* set QE bit to disable HOLD/WP pin function */
			status |= 0x40;
			sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 1);
		}

		return;
	}

	/* check QE bit */
	status = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	if (!(status & 0x2)) {
		/* set QE bit to disable HOLD/WP pin function, for WinBond */
		status |= 0x2;
		sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status, 1);

		/* check QE bit again */
		status = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
		if (!(status & 0x2)) {
			/* oh, let's try old write status cmd, for GigaDevice/Berg */
			status = ((status | 0x2) << 8) |
				 sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
			sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 2);

		}
	}
}


#ifdef CONFIG_XSPI_NOR_ACTS_DATA_PROTECTION_ENABLE

/* 	
	Note: General write operations may cause temporary errors in the status register, such as clearing QE,
	Therefore, it is necessary to turn off interrupts to ensure that no other code will be run in the middle, 
	so as to avoid cache miss accessing the spinor error.
*/

static __ramfunc void xspi_nor_write_status_general(struct xspi_nor_info *sni, u16_t status_high, u16_t status_low)
{
	u32_t flags;
	u16_t status = (status_high << 8) | status_low;

	flags = irq_lock();
	sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2, (u8_t *)&status_high, 1);
	sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,  (u8_t *)&status_low,  1);
	sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,  (u8_t *)&status, 2);
	irq_unlock(flags);
}

static bool data_protect_except_32kb_general(struct xspi_nor_info *sni)
{
	u16_t status_low, status_high, status = 0;

	status_high = 0x40 | sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status_low = 0x50;
	status = (status_high << 8) | status_low;

	/* P25Q16H: cmd 31H will overwrite config register */

	if (sni->chipid == 0x156085) {
		sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, (u8_t *)&status, 2);
		return true;
	}

	xspi_nor_write_status_general(sni, status_high, status_low);

	return true;
}

static bool data_protect_except_32kb_for_puya(struct xspi_nor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* P25Q16H(P25Q16SH) */
	if (chipid == 0x156085)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_ds(struct xspi_nor_info *sni)
{
	//u32_t chipid = sni->chipid;

	/* ZB25VQ16 supports theoretically according to its datasheet, but we haven't tested yet */
	//if (chipid == 0x15605e)
	//	return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_winbond(struct xspi_nor_info *sni)
{
	u32_t chipid = sni->chipid;
 
	/* W25Q16JV *//* note: there are 2 TIDs for this nor, and we're using 0x40 */
	if (chipid == 0x1540ef)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_giga(struct xspi_nor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* GD25Q16CSIG */
	if (chipid == 0x1540c8)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_xmc(struct xspi_nor_info *sni)
{
	//u32_t chipid = sni->chipid;

	/* XM25QH16B supports theoretically according to its datasheet, but we haven't tested yet */
	//if (chipid == 0x154020)
	//	return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_esmt(struct xspi_nor_info *sni)
{
	return false;
}

static bool data_protect_except_32kb_for_berg(struct xspi_nor_info *sni)
{
	return false;
}

static bool data_protect_except_32kb_for_boya(struct xspi_nor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* BY25Q16BSSIG */
	if (chipid == 0x154068)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_xtx(struct xspi_nor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* XT25W16B supports theoretically according to its datasheet, but we haven't tested yet */
	/* XT25F16BS, XT25W16B */
	//if ((chipid == 0x15400b) || (chipid == 0x15600b))
	/* XT25F16F & XT25F16B share the same chipid, and supports data_protection both, 
	   XT25F16F max clock support above 90MHz, But XT25F16B we've found it unstable above 80MHz */
	/* XT25F16F */
	if (chipid == 0x15400b)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_mxic(struct xspi_nor_info *sni)
{
	return false;
}

static bool xspi_nor_data_protect_except_32kb(struct xspi_nor_info *sni)
{
	u8_t manu_chipid = sni->chipid & 0xff;
	bool ret_val = false;

	switch (manu_chipid) {
	case XSPI_NOR_MANU_ID_MACRONIX:
		ret_val = data_protect_except_32kb_for_mxic(sni);
		break;

	case XSPI_NOR_MANU_ID_XTX:
		ret_val = data_protect_except_32kb_for_xtx(sni);
		break;

	case XSPI_NOR_MANU_ID_BOYA:
		ret_val = data_protect_except_32kb_for_boya(sni);
		break;

	case XSPI_NOR_MANU_ID_ZH_BERG:
		ret_val = data_protect_except_32kb_for_berg(sni);
		break;

	case XSPI_NOR_MANU_ID_ESMT:
		ret_val = data_protect_except_32kb_for_esmt(sni);
		break;

	case XSPI_NOR_MANU_ID_XMC:
		ret_val = data_protect_except_32kb_for_xmc(sni);
		break;

	case XSPI_NOR_MANU_ID_GIGADEVICE:
		ret_val = data_protect_except_32kb_for_giga(sni);
		break;

	case XSPI_NOR_MANU_ID_WINBOND_NEX:
		ret_val = data_protect_except_32kb_for_winbond(sni);
		break;

	case XSPI_NOR_MANU_ID_PUYA:
		ret_val = data_protect_except_32kb_for_puya(sni);
		break;

	case XSPI_NOR_MANU_ID_DS:
		ret_val = data_protect_except_32kb_for_ds(sni);
		break;

	default :
		ret_val = false;
		break;
	}

	return ret_val;
}

static void xspi_nor_data_protect_disable(struct xspi_nor_info *sni)
{
	u16_t status_low = 0, status_high = 0, status = 0;
	u32_t chipid = sni->chipid;

	status_low = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status_high = sni->spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status = (status_high << 8) | status_low;

	/* PUYA: P25Q80H (1M 0x146085), P25Q16H (2M 0x156085) */
	//if ((chipid == 0x146085) || (chipid == 0x156085)) { 
	if (chipid == 0x156085) {
		status &= 0xbfc3;
		sni->spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, (u8_t *)&status, 2);
    } else {
		status_high &= 0xbf;
		status_low &= 0xc3;
		xspi_nor_write_status_general(sni, status_high, status_low);
	}
}

#endif  //CONFIG_XSPI_NOR_ACTS_DATA_PROTECTION_ENABLE


static int xspi_nor_read(struct device *dev, off_t addr, void *data, size_t len)
{
	struct xspi_nor_info *sni =
		(struct xspi_nor_info *)dev->driver_data;

#ifdef CONFIG_OUTSIDE_FLASH
	if(sni->oflash_config.cs) {

		outsides_nor_read_internal(&sni->oflash_config, addr, data, len);
	} else
#endif
		sni->spinor_api->read(sni, addr, data, len);
	return 0;

}

static int xspi_nor_write(struct device *dev, off_t addr, const void *data, size_t len)
{
	struct xspi_nor_info *sni =
		(struct xspi_nor_info *)dev->driver_data;

#ifdef CONFIG_OUTSIDE_FLASH
	if(sni->oflash_config.cs) {

		outsides_nor_write_internal(&sni->oflash_config, addr, data, len);
		xspi_delay();

		return 0;
	}
#endif

	if ((u32_t)addr & (1 << 31))
		return sni->spinor_api->write_rdm(sni, (addr & ~(1 << 31)), data, len);
	else
		return sni->spinor_api->write(sni, addr, data, len);

}

static int xspi_nor_erase(struct device *dev, off_t addr, size_t size)
{
	struct xspi_nor_info *sni =
		(struct xspi_nor_info *)dev->driver_data;

#ifdef CONFIG_OUTSIDE_FLASH
	if(sni->oflash_config.cs) {
		if (addr & XSPI_NOR_ERASE_SECTOR_MASK || size & XSPI_NOR_ERASE_SECTOR_MASK)
			return -EINVAL;

		outsides_nor_erase_internal(&sni->oflash_config, addr, size);
		xspi_delay();

		return 0;
	}
#endif
	return sni->spinor_api->erase(sni, addr, size);

}

static int xspi_nor_write_protection(struct device *dev, bool enable)
{
#ifdef CONFIG_XSPI_NOR_ACTS_DATA_PROTECTION_ENABLE
	struct xspi_nor_info *sni = (struct xspi_nor_info *)dev->driver_data;
#endif

	ARG_UNUSED(dev);
	ARG_UNUSED(enable);

#ifdef CONFIG_XSPI_NOR_ACTS_DATA_PROTECTION_ENABLE
	if (sni->flag & SPINOR_FLAG_DATA_PROTECTION_SUPPORT) {
		xspi_nor_dump_info(sni);

		if (enable) {
			if ((sni->flag & SPINOR_FLAG_DATA_PROTECTION_OPENED) == 0) {
				if (xspi_nor_data_protect_except_32kb(sni) == true) {
					sni->flag |= SPINOR_FLAG_DATA_PROTECTION_OPENED;
					printk("data protect enable is true\n");
				} else {
					sni->flag &= ~SPINOR_FLAG_DATA_PROTECTION_OPENED;
					printk("data protect enable is false\n");
				}
			}
		} else {
			if (sni->flag & SPINOR_FLAG_DATA_PROTECTION_OPENED) {
				xspi_nor_data_protect_disable(sni);
				sni->flag &= ~SPINOR_FLAG_DATA_PROTECTION_OPENED;
				printk("data protect disable\n");
			}
		}

		xspi_nor_dump_info(sni);
	}
#endif

	return 0;
}

static const struct flash_driver_api xspi_nor_api = {
	.read = xspi_nor_read,
	.write = xspi_nor_write,
	.erase = xspi_nor_erase,
	.write_protection = xspi_nor_write_protection,
};

__ramfunc static void _nor_set_spi_read_mode(struct xspi_nor_info *sni)
{
	struct xspi_info *si = (struct xspi_info *)&sni->spi;

	sys_write32(sys_read32(SPI0_REG_BASE + XSPI_CTL) | XSPI_CTL_AHB_REQ, SPI0_REG_BASE + XSPI_CTL);
	while (1) {
		if (sys_read32(SPI0_REG_BASE + XSPI_STATUS) & XSPI_STATUS_READY)
			break;
	}

	if (sni->spi.flag & XSPI_FLAG_USE_MIO_ADDR_DATA) {
		sys_write32((sys_read32(SPI0_REG_BASE + XSPI_CTL) & (~XSPI_CTL_ADDR_MODE_MASK)) | XSPI_CTL_ADDR_MODE_2X4X, SPI0_REG_BASE + XSPI_CTL);
	} else {
		sys_write32((sys_read32(SPI0_REG_BASE + XSPI_CTL) & (~XSPI_CTL_ADDR_MODE_MASK)) | XSPI_CTL_ADDR_MODE_DUAL_QUAD, SPI0_REG_BASE + XSPI_CTL);
	}

	/* switch bus_width at best in sarm code */
	if (si->bus_width == 4) {
		/* enable 4x mode */
		xspi_setup_bus_width(si, 4);
	} else if (si->bus_width == 2) {
		/* enable 2x mode */
		xspi_setup_bus_width(si, 2);
	} else {
		/* enable 1x mode */
		xspi_setup_bus_width(si, 1);
	}

	sys_write32(sys_read32(SPI0_REG_BASE + XSPI_CTL) & (~XSPI_CTL_AHB_REQ), SPI0_REG_BASE + XSPI_CTL);
	while (1) {
		if ((sys_read32(SPI0_REG_BASE + XSPI_STATUS) & XSPI_STATUS_READY) == 0)
			break;
	}
}

#define COMPARE_BYTES   (32)
void _nor_read_mode_try(struct xspi_nor_info *sni, unsigned char bus_width)
{
	u8_t temp_data[COMPARE_BYTES], src_data[COMPARE_BYTES];
	const boot_info_t *p_boot_info = soc_boot_get_info();

	sni->spi.bus_width = 1;
	sni->spi.flag &= ~XSPI_FLAG_USE_MIO_ADDR_DATA;
	memset(src_data, 0xff, COMPARE_BYTES);
	sni->spinor_api->read(sni, p_boot_info->boot_phy_addr, src_data, COMPARE_BYTES);

	/* try io mode */
	sni->spi.bus_width = bus_width;
	sni->spi.flag |= XSPI_FLAG_USE_MIO_ADDR_DATA;

	if (sni->spi.bus_width == 4) {
		xspi_nor_enable_status_qe(sni);
	}

	memset(temp_data, 0x00, COMPARE_BYTES);
	sni->spinor_api->read(sni, p_boot_info->boot_phy_addr, temp_data, COMPARE_BYTES);
	if (memcmp(temp_data, src_data, COMPARE_BYTES) == 0) {
		return ;
	}

	/* try output mode */
	sni->spi.flag &= ~XSPI_FLAG_USE_MIO_ADDR_DATA;

	memset(temp_data, 0x00, COMPARE_BYTES);
	sni->spinor_api->read(sni, p_boot_info->boot_phy_addr, temp_data, COMPARE_BYTES); 
	if (memcmp(temp_data, src_data, COMPARE_BYTES) != 0) {
		sni->spi.bus_width = 1;
	}
}

void xspi_nor_dual_quad_read_mode_try(struct xspi_nor_info *sni)
{
	struct xspi_info *si = (struct xspi_info *)&sni->spi;

#if (CONFIG_XSPI_NOR_ACTS_IO_BUS_WIDTH == 4)

	_nor_read_mode_try(sni, 4);
	if (si->bus_width == 1) {
#ifdef CONFIG_XSPI_NOR_ACTS_IO_BUS_WIDTH_TRY_TEST
		_nor_read_mode_try(sni, 2);
		if (si->bus_width == 1) {
			panic("bus width & io mode try fail!\n");
		}
#else
		panic("bus width & io mode try fail!\n");
#endif
	}

#elif (CONFIG_XSPI_NOR_ACTS_IO_BUS_WIDTH == 2)

	_nor_read_mode_try(sni, 2);
	if (si->bus_width == 1) {
		panic("bus width & io mode try fail!\n");
	}

#endif

	_nor_set_spi_read_mode(sni);
}

void acts_xspi_nor_init_internal(struct xspi_nor_info *sni)
{
	struct xspi_info *si = (struct xspi_info *)&sni->spi;
	u32_t key;

	key = irq_lock();

	if (xspi_controller_id(si) == 0) {
		xspi_nor_dual_quad_read_mode_try(sni);

		printk("bus width : %d, and cache read use ", sni->spi.bus_width);
		if (sni->spi.flag & XSPI_FLAG_USE_MIO_ADDR_DATA)
			printk("input&output mode\n");
		else
			printk("output mode\n");

#ifdef CONFIG_XSPI_NOR_ACTS_DATA_PROTECTION_ENABLE
		if (xspi_nor_data_protect_except_32kb(sni) == true) {
			sni->flag |= SPINOR_FLAG_DATA_PROTECTION_SUPPORT;
			sni->flag |= SPINOR_FLAG_DATA_PROTECTION_OPENED;
			printk("data protect enable is true\n");
		} else {
			sni->flag &= ~SPINOR_FLAG_DATA_PROTECTION_SUPPORT;
			sni->flag &= ~SPINOR_FLAG_DATA_PROTECTION_OPENED;
			printk("data protect enable is false\n");
		}
#endif
	} else {
		if (si->bus_width == 4) {
			xspi_nor_enable_status_qe(sni);

			/* enable 4x mode */
			xspi_setup_bus_width(si, 4);
		} else if (si->bus_width == 2) {
			/* enable 2x mode */
			xspi_setup_bus_width(si, 2);
		} else {
			/* enable 2x mode */
			xspi_setup_bus_width(si, 1);
		}
	}

	soc_freq_set_spi_clk(xspi_controller_id(si), si->freq_mhz);

	xspi_setup_delaychain(si, si->delay_chain);

	irq_unlock(key);
}


#ifdef CONFIG_XSPI_NOR_FIX
struct spinor_rom_operation_api fix_spinor_api;

#ifdef CONFIG_XSPI_MEM_FIX
struct spimem_rom_operation_api fix_spimem_op_api;
extern void spimem_read_page_fix(struct xspi_info *si, unsigned int addr, int addr_len, void *buf, int len);
#endif

/* For exsample */
typedef unsigned char (*func_spimem_read_status)(struct xspi_info *si, unsigned char cmd);
__ramfunc static unsigned int spinor_read_status_fix(struct xspi_nor_info *sni, unsigned char cmd)
{
	/*
	 * Here use 0xbfc05b2d which is ROM address is only for showing the fix mechanism.
	 * It is not suggest to use this method as it will lead to influence the ROM later version.
	 * For compatibility, new BROM project is ought to keep this interface as consistet.
	 */

	func_spimem_read_status __rom_spimem_read_status = (func_spimem_read_status)0xbfc05b2d;
	return __rom_spimem_read_status(&sni->spi, cmd);
}
/* For exsample END */

#endif

int acts_xspi_nor_init(struct device *dev)
{
	struct xspi_nor_info *sni = (struct xspi_nor_info *)dev->driver_data;

#ifdef CONFIG_XSPI_NOR_FIX

	memcpy(&fix_spinor_api, BROM_DEVICE_GET_BINDING_SPINOR(), sizeof(fix_spinor_api));
	/* For exsample */
	fix_spinor_api.read_status = spinor_read_status_fix;
	/* For exsample END */
	sni->spinor_api = &fix_spinor_api;
#ifdef CONFIG_XSPI_MEM_FIX
	memcpy(&fix_spimem_op_api, BROM_DEVICE_GET_BINDING_SPIMEM(), sizeof(fix_spimem_op_api));
	/* For exsample */
	fix_spimem_op_api.read_page = spimem_read_page_fix;
	/* For exsample END */
	sni->spi.spimem_op_api = &fix_spimem_op_api;
#else
	sni->spi.spimem_op_api = BROM_DEVICE_GET_BINDING_SPIMEM();
#endif

#else

	sni->spinor_api = BROM_DEVICE_GET_BINDING_SPINOR();
    sni->spi.spimem_op_api = BROM_DEVICE_GET_BINDING_SPIMEM();

#endif

	sni->spi.prepare_hook = sni->spi.spimem_op_api->continuous_read_reset;

	sni->chipid = sni->spinor_api->read_chipid(sni) & 0xffffff;

	xspi_nor_dump_info(sni);

	acts_xspi_nor_init_internal(sni);

	xspi_nor_dump_info(sni);

	return 0;
}


#ifdef CONFIG_SPINOR_TEST_DELAYCHAIN

u32_t test_delaychain_read_id(struct device *dev, u8_t delay_chain)
{
	u32_t nor_id, mid;

	struct xspi_nor_info *sni = (struct xspi_nor_info *)dev->driver_data;
	sni->spi.delay_chain = delay_chain;
	nor_id = sni->spinor_api->read_chipid(sni) & 0xffffff;

	mid = nor_id & 0xff;
	if ((mid == 0xff) || (mid == 0x00))
		return 0;

	return nor_id;
}

s32_t test_delaychain_try(struct device *dev, u8_t *ret_delaychain, u32_t chipid_ref)
{
	u32_t i, try_delaychain;
	bool match_flag = 0;
	u32_t nor_id_value_check;
	u32_t local_irq_save;

	local_irq_save = irq_lock();

	ret_delaychain[0] = 0;
	for (try_delaychain = 1; try_delaychain <= 15; try_delaychain++) {
		match_flag = 1;
		for (i = 0; i < 64; i++) {
			nor_id_value_check = test_delaychain_read_id(dev, try_delaychain);
			if (nor_id_value_check != chipid_ref) {
				printk("read:0x%x @ %d\n", nor_id_value_check, try_delaychain);
				k_busy_wait(5000);
				match_flag = 0;
				break;
			}
		}

		ret_delaychain[try_delaychain] = match_flag;
	}

	test_delaychain_read_id(dev, CONFIG_XSPI_NOR_ACTS_DELAY_CHAIN);

	irq_unlock(local_irq_save);

	return 0;
}


u8_t spinor_test_delaychain(void)
{
	u8_t ret_delaychain = 0;
	u8_t delaychain_flag[16];
	u8_t delaychain_total[16];
	u8_t start = 0, end, middle, i;
	u8_t expect_max_count_delay_chain = 0, max_count_delay_chain;
	u32_t freq;
	u32_t chipid_ref;
	struct device *test_nor_dev = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);

	printk("spinor test delaychain start\n");

	chipid_ref = test_delaychain_read_id(test_nor_dev, CONFIG_XSPI_NOR_ACTS_DELAY_CHAIN);

	printk("chipid_ref : 0x%x\n", chipid_ref);

	memset(delaychain_total, 0x0, 16);

	for (freq = 6; freq <= 222; freq += 6) {
		expect_max_count_delay_chain++;

		soc_freq_set_cpu_clk(0, freq);

		printk("set cpu freq : %d\n", freq);

		if (test_delaychain_try(test_nor_dev, delaychain_flag, chipid_ref) == 0) {
			for (i = 0; i < 16; i++)
				delaychain_total[i] += delaychain_flag[i];
		} else {
			printk("test_delaychain_try error!!\n");
			goto delay_chain_exit;
		}
	}

	printk("delaychain_total : ");
	for (i = 0; i < 15; i++)
		printk("%d,", delaychain_total[i]);
	printk("%d\n", delaychain_total[15]);

	max_count_delay_chain = 0;
	for (i = 0; i < 16; i++) {
		if (delaychain_total[i] > max_count_delay_chain)
			max_count_delay_chain = delaychain_total[i];
	}

	for (i = 0; i < 16; i++) {
		if (delaychain_total[i] == max_count_delay_chain) {
			start = i;
			break;
		}
	}
	end = start;
	for (i = start + 1; i < 16; i++) {
		if (delaychain_total[i] != max_count_delay_chain)
			break;
		end = i;
	}

	if (max_count_delay_chain < expect_max_count_delay_chain) {
		printk("test delaychain max count is %d, less then expect %d!!\n",
			max_count_delay_chain, expect_max_count_delay_chain);
		goto delay_chain_exit;
	}

	if ((end - start + 1) < 3) {
		printk("test delaychain only %d ok!! too less!!\n", end - start + 1);
		goto delay_chain_exit;
	}

	middle = (start + end) / 2;
	printk("test delaychain pass, best delaychain is : %d\n\n", middle);

	ret_delaychain = middle;

	delay_chain_exit:

	return ret_delaychain;
}


int acts_xspi_nor_test_delaychain(struct device *dev)
{
	printk("acts_xspi_nor_test_delaychain\n");

	printk("delaychain : %d\n", spinor_test_delaychain());

	return 0;
}

SYS_INIT(acts_xspi_nor_test_delaychain, PRE_KERNEL_2, CONFIG_XSPI_NOR_ACTS_DEV_TEST_DELAYCHAIN_PRIORITY);

#endif


/* system XIP spinor */
static struct xspi_nor_info system_xspi_nor = {
	.spi = {
		.base = SPI0_REG_BASE,
		.bus_width = CONFIG_XSPI_NOR_ACTS_IO_BUS_WIDTH,
		.delay_chain = CONFIG_XSPI_NOR_ACTS_DELAY_CHAIN,
		.freq_mhz = CONFIG_XSPI_ACTS_FREQ_MHZ,
		.flag = 0,
		.dma_chan_base = 0,
		.sys_irq_lock_cb = sys_irq_lock,
		.sys_irq_unlock_cb = sys_irq_unlock,
	},
#ifdef CONFIG_XSPI_NOR_AUTO_CHECK_RB
	.flag = SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY,
#else
	.flag = 0,
#endif

	.clock_id = CLOCK_ID_SPI0,
	.reset_id = RESET_ID_SPI0,
	.dma_id = DMA_ID_SPI0,
	.dma_chan = -1,
};


DEVICE_AND_API_INIT(xspi_nor_acts, CONFIG_XSPI_NOR_ACTS_DEV_NAME, acts_xspi_nor_init,
	     &system_xspi_nor, NULL, PRE_KERNEL_1,
	     CONFIG_XSPI_NOR_ACTS_DEV_INIT_PRIORITY, &xspi_nor_api);

#ifdef CONFIG_XSPI1_NOR_ACTS
static struct xspi_nor_info xspi1_nor = {
	 .spi = {
		 .base = SPI1_REG_BASE,
		 .bus_width = 1,
		 .delay_chain = 8,
		 .freq_mhz = CONFIG_XSPI1_ACTS_FREQ_MHZ,


#ifdef CONFIG_XSPI1_ACTS_USE_DMA
		 .dma_chan_base = 0xC0010300, //0xC0010300 DMA2 
#endif
		 .flag = XSPI_FLAG_NO_IRQ_LOCK,
	 },

	 .clock_id = CLOCK_ID_SPI1,
	 .reset_id = RESET_ID_SPI1,
	 .dma_id = DMA_ID_SPI1,
	 .dma_chan = -1,
#ifdef CONFIG_OUTSIDE_FLASH
	 .oflash_config={
		 .frequency = 20000000,
		 .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER |
				 SPI_MODE_CPOL | SPI_MODE_CPHA,
		 .slave = 0,
		 .vendor = 0,
		 .cs=NULL,
		 .cs_func=NULL,
	 },
#endif
};


int acts_xspi1_nor_init(struct device *dev)
{
	struct xspi_nor_info *sni = (struct xspi_nor_info *)dev->driver_data;

#ifdef CONFIG_OUTSIDE_FLASH

	outsideflash_cs.gpio_dev=device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);

	if (outsideflash_cs.gpio_dev==NULL){
		printk("\ncan not find device\n\n");
		return -EINVAL;
	}

	outsideflash_cs.delay=0;
	outsideflash_cs.gpio_pin=BOARD_OUTSIDE_FALSH_CS_PIN_CONTROLLER;
	sni->oflash_config.cs=&outsideflash_cs;

	outside_nor_dc.dc_or_miso=1;
	outside_nor_dc.pin=BOARD_OUTSIDE_FALSH_DC_PIN_DRIVER;
	outside_nor_dc.dc_func=outside_nor_dc_pad_func_switch;
	sni->oflash_config.dc_func_switch=&outside_nor_dc;

	oflash_spi=device_get_binding(CONFIG_SPI_1_NAME);
	if (oflash_spi==NULL){
		printk("\ncan not find device\n\n");
		return -EINVAL;
	}

#else

	acts_clock_peripheral_enable(CLOCK_ID_SPI1);
	acts_reset_peripheral(RESET_ID_SPI1);

	sni->spinor_api = BROM_DEVICE_GET_BINDING_SPINOR();
	sni->spi.spimem_op_api = BROM_DEVICE_GET_BINDING_SPIMEM();

	acts_xspi_nor_init_internal(sni);

	sni->chipid = sni->spinor_api->read_chipid(sni) & 0xffffff;


#endif

	xspi_nor_dump_info(sni);

	return 0;
}


DEVICE_AND_API_INIT(spi1_xspi_nor_acts, CONFIG_XSPI1_NOR_ACTS_DEV_NAME, acts_xspi1_nor_init,
	     &xspi1_nor, NULL, POST_KERNEL,
	     10, &xspi_nor_api);
#endif /* CONFIG_XSPI1_NOR_ACTS*/

