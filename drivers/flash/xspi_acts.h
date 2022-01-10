/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI XIP controller helper for Actions SoC
 */

#ifndef __XSPI_ACTS_H__
#define __XSPI_ACTS_H__

#include <errno.h>
#include <kernel.h>
#include <soc_rom_xspi.h>
#include <soc_dma.h>

#define SPI_DELAY_LOOPS			5

/* spinor controller */
#define XSPI_CTL			0x00
#define XSPI_STATUS			0x04
#define XSPI_TXDAT			0x08
#define XSPI_RXDAT			0x0c
#define XSPI_BC				0x10
#define XSPI_SEED			0x14
#define XSPI_CACHE_ERR_ADDR		0x18
#define XSPI_CACHE_MODE			0x1C

#define XSPI_CTL_CLK_SEL_MASK		(1 << 31)
#define XSPI_CTL_CLK_SEL_CPU		(0 << 31)
#define XSPI_CTL_CLK_SEL_DMA		(1 << 31)

#define XSPI_CTL_MODE_MASK		(1 << 28)
#define XSPI_CTL_MODE_MODE3		(0 << 28)
#define XSPI_CTL_MODE_MODE0		(1 << 28)

#define XSPI_CTL_ADDR_MODE_MASK  (1 << 27)
#define XSPI_CTL_ADDR_MODE_2X4X		(0 << 27)
#define XSPI_CTL_ADDR_MODE_DUAL_QUAD	(1 << 27)

#define XSPI_CTL_CRC_EN			(1 << 20)

#define XSPI_CTL_DELAYCHAIN_MASK	(0xf << 16)
#define XSPI_CTL_DELAYCHAIN_SHIFT	(16)

#define XSPI_CTL_RAND_MASK		(0xf << 12)
#define XSPI_CTL_RAND_PAUSE		(1 << 15)
#define XSPI_CTL_RAND_SEL		(1 << 14)
#define XSPI_CTL_RAND_TXEN		(1 << 13)
#define XSPI_CTL_RAND_RXEN		(1 << 12)

#define XSPI_CTL_IO_MODE_MASK		(0x3 << 10)
#define XSPI_CTL_IO_MODE_SHIFT		(10)
#define XSPI_CTL_IO_MODE_1X		(0x0 << 10)
#define XSPI_CTL_IO_MODE_2X		(0x2 << 10)
#define XSPI_CTL_IO_MODE_4X		(0x3 << 10)
#define XSPI_CTL_SPI_3WIRE		(1 << 9)
#define XSPI_CTL_AHB_REQ		(1 << 8)
#define XSPI_CTL_TX_DRQ_EN		(1 << 7)
#define XSPI_CTL_RX_DRQ_EN		(1 << 6)
#define XSPI_CTL_TX_FIFO_EN		(1 << 5)
#define XSPI_CTL_RX_FIFO_EN		(1 << 4)
#define XSPI_CTL_SS			(1 << 3)
#define XSPI_CTL_WR_MODE_MASK		(0x3 << 0)
#define XSPI_CTL_WR_MODE_DISABLE	(0x0 << 0)
#define XSPI_CTL_WR_MODE_READ		(0x1 << 0)
#define XSPI_CTL_WR_MODE_WRITE		(0x2 << 0)
#define XSPI_CTL_WR_MODE_READ_WRITE	(0x3 << 0)

#define XSPI_STATUS_READY		(1 << 8)
#define XSPI_STATUS_BUSY		(1 << 6)
#define XSPI_STATUS_TX_FULL		(1 << 5)
#define XSPI_STATUS_TX_EMPTY		(1 << 4)
#define XSPI_STATUS_RX_FULL		(1 << 3)
#define XSPI_STATUS_RX_EMPTY		(1 << 2)

#define XSPI_FLAG_NEED_EXIT_CONTINUOUS_READ	(1 << 0)
#define XSPI_FLAG_SPI_MODE0			(1 << 1)

/* spi1 registers bits */
#define XSPI1_CTL_MODE_MASK		(3 << 28)
#define XSPI1_CTL_MODE_MODE3		(3 << 28)
#define XSPI1_CTL_MODE_MODE0		(0 << 28)
#define XSPI1_CTL_AHB_REQ		(1 << 15)

#define XSPI1_STATUS_BUSY		(1 << 0)
#define XSPI1_STATUS_TX_FULL		(1 << 5)
#define XSPI1_STATUS_TX_EMPTY		(1 << 4)
#define XSPI1_STATUS_RX_FULL		(1 << 7)
#define XSPI1_STATUS_RX_EMPTY		(1 << 6)

#define XSPI_CACHE_MODE_CHECK_RB_EN	(1 << 0)
#define XSPI_CACHE_MODE_RMS		(1 << 1)
#define XSPI_CACHE_MODE_RB_STATUS	(1 << 8)

#define XSPI1_STATUS_BUSY		(1 << 0)
#define XSPI1_STATUS_TX_FULL		(1 << 5)
#define XSPI1_STATUS_TX_EMPTY		(1 << 4)
#define XSPI1_STATUS_RX_FULL		(1 << 7)
#define XSPI1_STATUS_RX_EMPTY		(1 << 6)

static inline int xspi_controller_id(struct xspi_info *si)
{
	if (si->base == (SPI0_REG_BASE))
		return 0;
	else if (si->base == (SPI1_REG_BASE))
		return 1;
	else
		return 2;
}

static inline int xspi_need_lock_irq(struct xspi_info *si)
{
	return (si->flag & XSPI_FLAG_NO_IRQ_LOCK) ? 0 : 1;
}

static ALWAYS_INLINE void xspi_delay(void)
{
	volatile int i = SPI_DELAY_LOOPS;

	while (i--)
		;
}

static ALWAYS_INLINE u32_t xspi_read(struct xspi_info *si, u32_t reg)
{
	return sys_read32(si->base + reg);
}

static ALWAYS_INLINE void xspi_write(struct xspi_info *si, u32_t reg,
				    u32_t value)
{
	sys_write32(value, si->base + reg);
}

static ALWAYS_INLINE void xspi_set_bits(struct xspi_info *si, u32_t reg,
				       u32_t mask, u32_t value)
{
	xspi_write(si, reg, (xspi_read(si, reg) & ~mask) | value);
}

static ALWAYS_INLINE void xspi_set_clk(struct xspi_info *si, u32_t freq_khz)
{
#if 0
	u32_t pclk_freq_khz = 32000;
	u32_t div;

	if (!freq_khz)
		freq_khz = 1000; 
	
	div = find_lsb_set(pclk_freq_khz / freq_khz) - 1;

	if (div > 5)
		div = 5;

	sys_write32(div, si->spiclk_reg);
#endif
}

static ALWAYS_INLINE void xspi_reset(struct xspi_info *si)
{
	/* cannot call function located in XSPI NOR */
#if 0
	acts_reset_peripheral(si->reset_id);
#endif
}

static void ALWAYS_INLINE xspi_set_cs(struct xspi_info *si, int value)
{
	if (si->set_cs)
		si->set_cs(si, value);
	else
		xspi_set_bits(si, XSPI_CTL, XSPI_CTL_SS, value ? XSPI_CTL_SS : 0);
}

static ALWAYS_INLINE void xspi_setup_bus_width(struct xspi_info *si, u8_t bus_width)
{
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_IO_MODE_MASK,
		     ((bus_width & 0x7) / 2 + 1) << XSPI_CTL_IO_MODE_SHIFT);
	xspi_delay();
}

static ALWAYS_INLINE void xspi_setup_delaychain(struct xspi_info *si, u8_t ns)
{
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_DELAYCHAIN_MASK,
		      ns << XSPI_CTL_DELAYCHAIN_SHIFT);
	xspi_delay();
}

static inline void xspi_setup_randmomize(struct xspi_info *si, int is_tx, int enable)
{
	if (enable)
		xspi_set_bits(si, XSPI_CTL, XSPI_CTL_RAND_TXEN | XSPI_CTL_RAND_RXEN,
			     is_tx ? XSPI_CTL_RAND_TXEN : XSPI_CTL_RAND_RXEN);
	else
		xspi_set_bits(si, XSPI_CTL, XSPI_CTL_RAND_TXEN | XSPI_CTL_RAND_RXEN, 0);
}

static inline void xspi_pause_resume_randmomize(struct xspi_info *si, int pause)
{
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_RAND_PAUSE,
		     pause ? XSPI_CTL_RAND_PAUSE : 0);
}

static inline int xspi_is_randmomize_pause(struct xspi_info *si)
{
	return !!(xspi_read(si, XSPI_CTL) & XSPI_CTL_RAND_PAUSE);
}

static ALWAYS_INLINE void xspi_wait_tx_complete(struct xspi_info *si)
{
	if (xspi_controller_id(si) == 0) {
		/* SPI0 */
		while (!(xspi_read(si, XSPI_STATUS) & XSPI_STATUS_TX_EMPTY))
			;

		/* wait until tx fifo is empty */
		while ((xspi_read(si, XSPI_STATUS) & XSPI_STATUS_BUSY))
			;
	} else {
		/* SPI1 & SPI2 */
		while (!(xspi_read(si, XSPI_STATUS) & XSPI1_STATUS_TX_EMPTY))
			;

		/* wait until tx fifo is empty */
		while ((xspi_read(si, XSPI_STATUS) & XSPI1_STATUS_BUSY))
			;
	}
}

static ALWAYS_INLINE void xspi_read_data(struct xspi_info *si, u8_t *buf,
					s32_t len)
{
	xspi_write(si, XSPI_BC, len);

	/* switch to read mode */
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_WR_MODE_MASK, XSPI_CTL_WR_MODE_READ);

	/* read data */
	while (len--) {
		if (xspi_controller_id(si) == 0) {
			/* SPI0 */
			while (xspi_read(si, XSPI_STATUS) & XSPI_STATUS_RX_EMPTY)
				;
		} else {
			/* SPI1 & SPI2 */
			while (xspi_read(si, XSPI_STATUS) & XSPI1_STATUS_RX_EMPTY)
				;
		}

		*buf++ = xspi_read(si, XSPI_RXDAT);
	}

	/* disable read mode */
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_WR_MODE_MASK, XSPI_CTL_WR_MODE_DISABLE);
}

static ALWAYS_INLINE void xspi_write_data(struct xspi_info *si,
					 const u8_t *buf, s32_t len)
{
	/* switch to write mode */
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_WR_MODE_MASK, XSPI_CTL_WR_MODE_WRITE);

	/* write data */
	while (len--) {
		if (xspi_controller_id(si) == 0) {
			/* SPI0 */
		while (xspi_read(si, XSPI_STATUS) & XSPI_STATUS_TX_FULL)
			;
		} else {
			/* SPI1 & SPI2 */
			while (xspi_read(si, XSPI_STATUS) & XSPI1_STATUS_TX_FULL)
				;
		}

		xspi_write(si, XSPI_TXDAT, *buf++);
	}

	xspi_delay();
	xspi_wait_tx_complete(si);

	/* disable write mode */
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_WR_MODE_MASK, XSPI_CTL_WR_MODE_DISABLE);
}

static ALWAYS_INLINE void xspi_read_data_by_dma(struct xspi_info *si,
				 const unsigned char *buf, int len)
{
	if (si->dma_chan_base == 0)
		return;

	xspi_write(si, XSPI_BC, len);

	/* switch to dma read mode */
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_CLK_SEL_MASK |
		XSPI_CTL_RX_DRQ_EN | XSPI_CTL_WR_MODE_MASK,
		XSPI_CTL_CLK_SEL_DMA | XSPI_CTL_RX_DRQ_EN |
		XSPI_CTL_WR_MODE_READ);

	if (xspi_controller_id(si) == 0) {
		/* SPI0 */
		sys_write32(DMA_CTL_TWS_8BIT \
        	| (0<<DMA_CTL_DAM_SHIFT) | DMA_CTL_DST_TYPE(DMA_ID_MEMORY) \
        	| (1<<DMA_CTL_SAM_SHIFT) | DMA_CTL_SRC_TYPE(DMA_ID_SPI0), \
			si->dma_chan_base + DMA_CTL_OFFSET);
	} else if (xspi_controller_id(si) == 1) {
		/* SPI1 */
		sys_write32(DMA_CTL_TWS_8BIT \
        	| (0<<DMA_CTL_DAM_SHIFT) | DMA_CTL_DST_TYPE(DMA_ID_MEMORY) \
        	| (1<<DMA_CTL_SAM_SHIFT) | DMA_CTL_SRC_TYPE(DMA_ID_SPI1), \
			si->dma_chan_base + DMA_CTL_OFFSET);
	} else {
		/* SPI2 */
		sys_write32(0x0, si->dma_chan_base + DMA_CTL_OFFSET);
	}

	sys_write32(si->base + XSPI_RXDAT, si->dma_chan_base + DMA_SADDR_OFFSET);
	sys_write32((unsigned int)buf, si->dma_chan_base + DMA_DADDR_OFFSET);
	sys_write32(len, si->dma_chan_base + DMA_BC_OFFSET);

	/* start dma */
	sys_write32(1, si->dma_chan_base + DMA_START_OFFSET);

	while (sys_read32(si->dma_chan_base + DMA_START_OFFSET) & (1<<DMA0START_DMASTART)) {
		/* wait */
	}


	xspi_delay();
	xspi_wait_tx_complete(si);

	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_CLK_SEL_MASK |
		XSPI_CTL_RX_DRQ_EN | XSPI_CTL_WR_MODE_MASK,
		XSPI_CTL_CLK_SEL_CPU | XSPI_CTL_WR_MODE_DISABLE);
}

static ALWAYS_INLINE void xspi_write_data_by_dma(struct xspi_info *si,
				  const unsigned char *buf, int len)
{
	if (si->dma_chan_base == 0)
		return;

	/* switch to dma write mode */
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_CLK_SEL_MASK |
		XSPI_CTL_TX_DRQ_EN | XSPI_CTL_WR_MODE_MASK,
		XSPI_CTL_CLK_SEL_DMA | XSPI_CTL_TX_DRQ_EN |
		XSPI_CTL_WR_MODE_WRITE);

	if (xspi_controller_id(si) == 0) {
		/* SPI0 */
		sys_write32(DMA_CTL_TWS_8BIT \
			| (1<<DMA_CTL_DAM_SHIFT) | DMA_CTL_DST_TYPE(DMA_ID_SPI0) \
        	| (0<<DMA_CTL_SAM_SHIFT) | DMA_CTL_SRC_TYPE(DMA_ID_MEMORY), \
			si->dma_chan_base + DMA_CTL_OFFSET);
	} else if (xspi_controller_id(si) == 1) {
		/* SPI1 */
		sys_write32(DMA_CTL_TWS_8BIT \
			| (1<<DMA_CTL_DAM_SHIFT) | DMA_CTL_DST_TYPE(DMA_ID_SPI1) \
        	| (0<<DMA_CTL_SAM_SHIFT) | DMA_CTL_SRC_TYPE(DMA_ID_MEMORY), \
			si->dma_chan_base + DMA_CTL_OFFSET);
	} else {
		/* SPI2 */
		sys_write32(0x0, si->dma_chan_base + DMA_CTL_OFFSET);
	}

	sys_write32((unsigned int)buf, si->dma_chan_base + DMA_SADDR_OFFSET);
	sys_write32(si->base + XSPI_TXDAT, si->dma_chan_base + DMA_DADDR_OFFSET);
	sys_write32(len, si->dma_chan_base + DMA_BC_OFFSET);

	/* start dma */
	sys_write32(1, si->dma_chan_base + DMA_START_OFFSET);

	while (sys_read32(si->dma_chan_base + DMA_START_OFFSET) & (1<<DMA0START_DMASTART)) {
		/* wait */
	}
	
	xspi_delay();
	xspi_wait_tx_complete(si);

	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_CLK_SEL_MASK |
		XSPI_CTL_TX_DRQ_EN | XSPI_CTL_WR_MODE_MASK,
		XSPI_CTL_CLK_SEL_CPU | XSPI_CTL_WR_MODE_DISABLE);
}
#endif	/* __SPI_ACTS_H__ */
