/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2STX physical implementation
 */

/*
 * Features
 *    - Support master and slave mode
 *    - Support I2S 2ch format
 *	- I2STX FIFO(32 x 24bits level) and DAC FIFO(8 x 24 bits level)
 *	- Support 3 format: left-justified format, right-justified format, I2S format
 *	- Support 16/20/24 effective data width; BCLK support 32/64fs, and MCLK=4BCLK
 *	- Support sample rate auto detect in slave mode
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k/192k
 */
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <audio_common.h>
#include "../phy_audio_common.h"
#include "../audio_acts_utils.h"
#include "audio_out.h"

#define SYS_LOG_DOMAIN "P_I2STX"
#include <logging/sys_log.h>
/***************************************************************************************************
 * I2STX_CTL
 */
#define I2ST0_CTL_TDMTX_CHAN                                  BIT(20) /* 4 channel or 8 channel */
#define I2ST0_CTL_TDMTX_MODE                                  BIT(19) /* I2S format or left-justified format */

#define I2ST0_CTL_TDMTX_SYNC_SHIFT                            (17) /* the type of LRCLK setting at the begining of the data frame */
#define I2ST0_CTL_TDMTX_SYNC_MASK                             (0x3 << I2ST0_CTL_TDMTX_SYNC_SHIFT)
#define I2ST0_CTL_TDMTX_SYNC(x)                               ((x) << I2ST0_CTL_TDMTX_SYNC_SHIFT)

#define I2ST0_CTL_MULT_DEVICE                                 BIT(16) /* multi device selection */
#define I2ST0_CTL_I2SRX_5W_EN                                 BIT(12) /* 5wire enable */
#define I2ST0_CTL_PSEUDO_5W_EN                                BIT(9) /* pseudo 5 wire enable */
#define I2ST0_CTL_LPEN0                                       BIT(8) /* I2STX and I2SRX data loopback enable */
#define I2ST0_CTL_TXMODE                                      BIT(7) /* I2STX mode selection */
#define I2ST0_CTL_TX_SMCLK                                    BIT(6) /* MCLK(256FS) source when in slave mode */

#define I2ST0_CTL_TXWIDTH_SHIFT                               (4) /* effective data width */
#define I2ST0_CTL_TXWIDTH_MASK                                (0x3 << I2ST0_CTL_TXWIDTH_SHIFT)
#define I2ST0_CTL_TXWIDTH(x)                                  ((x) << I2ST0_CTL_TXWIDTH_SHIFT)

#define I2ST0_CTL_TXBCLKSET                                   BIT(3) /* rate of BCLK with LRCLK */

#define I2ST0_CTL_TXMODELSEL_SHIFT                            (1) /* I2S transfer format select */
#define I2ST0_CTL_TXMODELSEL_MASK                             (0x3 << I2ST0_CTL_TXMODELSEL_SHIFT)
#define I2ST0_CTL_TXMODELSEL(x)                               ((x) << I2ST0_CTL_TXMODELSEL_SHIFT)

#define I2ST0_CTL_TXEN                                        BIT(0) /* I2S TX Enable */

/***************************************************************************************************
 * I2STX_FIFOCTL
 */
#define I2ST0_FIFOCTL_TXFIFO_DMAWIDTH                         BIT(7) /* dma transfer width */

#define I2ST0_FIFOCTL_FIFO_IN_SEL_SHIFT                       (4) /* I2STX FIFO input select */
#define I2ST0_FIFOCTL_FIFO_IN_SEL_MASK                        (0x3 << I2ST0_FIFOCTL_FIFO_IN_SEL_SHIFT)
#define I2ST0_FIFOCTL_FIFO_IN_SEL(x)                          ((x) << I2ST0_FIFOCTL_FIFO_IN_SEL_SHIFT)

#define I2ST0_FIFOCTL_FIFO_SEL                                BIT(3) /* I2S/SPDIF module FIFO selection */
#define I2ST0_FIFOCTL_FIFO_IEN                                BIT(2) /* I2STX FIFO half empty irq enable */
#define I2ST0_FIFOCTL_FIFO_DEN                                BIT(1) /* I2STX FIFO half empty drq enable */
#define I2ST0_FIFOCTL_FIFO_RST                                BIT(0) /* I2STX FIFO reset */

/***************************************************************************************************
 * I2STX_FIFOSTAT
 */
#define I2ST0_FIFOSTA_FIFO_ER                                 BIT(8) /* FIFO error */
#define I2ST0_FIFOSTA_IP                                      BIT(7) /* half empty irq pending bit */
#define I2ST0_FIFOSTA_TFFU                                    BIT(6) /* fifo full flag */

#define I2ST0_FIFOSTA_STA_SHIFT                               (0) /* fifo status */
#define I2ST0_FIFOSTA_STA_MASK                                (0x3F << I2ST0_FIFOSTA_STA_SHIFT)

/***************************************************************************************************
 * I2STX_DAT
 */
#define I2ST0_DAT_DAT_SHIFT                                   (8) /* I2STX FIFO is 24bits x 32 levels */
#define I2ST0_DAT_DAT_MASK                                    (0xFFFFFF << I2ST0_DAT_DAT_SHIFT)

/***************************************************************************************************
 * I2STX_SRDCTL - I2STX Slave mode sample rate detect register
 */
#define I2ST0_SRDCTL_MUTE_EN                                  BIT(12) /* If detect sample rate or channel width changing, mute the TX output as 0 */
#define I2ST0_SRDCTL_SRD_IE                                   BIT(8) /* sample rate detect result change interrupt enable */

#define I2ST0_SRDCTL_CNT_TIM_SHIFT                            (4) /* slave mode sample rate detect counter period select */
#define I2ST0_SRDCTL_CNT_TIM_MASK                             (0x3 << I2ST0_SRDCTL_CNT_TIM_SHIFT)
#define I2ST0_SRDCTL_CNT_TIM(x)                               ((x) << I2ST0_SRDCTL_CNT_TIM_SHIFT)

#define I2ST0_SRDCTL_SRD_TH_SHIFT                             (1) /* sample rate detecting sensitivity setting */
#define I2ST0_SRDCTL_SRD_TH_MASK                              (0x7 << I2ST0_SRDCTL_SRD_TH_SHIFT)
#define I2ST0_SRDCTL_SRD_TH(x)                                ((x) << I2ST0_SRDCTL_SRD_TH_SHIFT)

#define I2ST0_SRDCTL_SRD_EN                                   BIT(0) /* slave mode sample rate detect enable */

/***************************************************************************************************
 * I2STX_SRDSTA
 */
#define I2ST0_SRDSTA_CNT_SHIFT                                (12) /* CNT of LRCLK which sampling by SRC_CLK */
#define I2ST0_SRDSTA_CNT_MASK                                 (0x1FFF << I2ST0_SRDSTA_CNT_SHIFT)
#define I2ST0_SRDSTA_CNT(x)                                   ((x) << I2ST0_SRDSTA_CNT_SHIFT)

#define I2ST0_SRDSTA_TO_PD                                    BIT(11) /* SRD timput irq pending */
#define I2ST0_SRDSTA_SRC_PD                                   BIT(10) /* sample rate changing detection interrupt pending */
#define I2ST0_SRDSTA_CHW_PD                                   BIT(8) /* channel width change irq pending */

#define I2ST0_SRDSTA_WL_SHIFT                                 (0) /* channel word lenght */
#define I2ST0_SRDSTA_WL_MASK                                  (0x7 << I2ST0_SRDSTA_WL_SHIFT)
#define I2ST0_SRDSTA_WL(x)                                    ((x) << I2ST0_SRDSTA_WL_SHIFT)

/***************************************************************************************************
 * I2STX_FIFO_CNT
 */
#define I2ST0_FIFO_CNT_IP                                     BIT(18) /* I2STX FIFO counter overflow irq pending */
#define I2ST0_FIFO_CNT_IE                                     BIT(17) /* I2STX FIFO counter overflow irq enable */
#define I2ST0_FIFO_CNT_EN                                     BIT(16) /* I2STX FIFO counter enable */

#define I2ST0_FIFO_CNT_CNT_SHIFT                              (0) /* I2STX FIFO counter */
#define I2ST0_FIFO_CNT_CNT_MASK                               (0xFFFF << I2ST0_FIFO_CNT_CNT_SHIFT)

/***************************************************************************************************
 * i2STX FEATURES CONGIURATION
 */

/* The sensitivity of the SRD */
#define I2STX_SRD_TH_DEFAULT                                  (7)

#define I2STX_BCLK_DEFAULT                                    (I2S_BCLK_64FS)
#define I2STX_MCLK_DEFAULT                                    (MCLK_256FS) /* MCLK = 4BCLK */

#define I2STX_FIFO_LEVEL                                      (32)

#define I2STX_SRD_CONFIG_TIMEOUT_MS                           (500)

/*
 * @struct acts_audio_i2stx0
 * @brief I2STX controller hardware register
 */
struct acts_audio_i2stx {
	volatile u32_t tx_ctl; /* I2STX control */
	volatile u32_t fifoctl; /* I2STX FIFO control */
	volatile u32_t fifostat; /* I2STX FIFO state */
	volatile u32_t dat; /* I2STX FIFO data */
	volatile u32_t srdctl; /* I2S slave mode TX sample rate detect control */
	volatile u32_t srdstat; /* I2S slave mode TX sample rate detect state */
	volatile u32_t fifocnt; /* I2STX out FIFO counter */
};

/*
 * struct i2stx_private_data
 * @brief Physical I2STX device private data
 */
struct i2stx_private_data {
	struct device *parent_dev; /* Indicates the parent audio device */
	a_i2stx_mclk_clksrc_e mclksrc; /* MCLK clock source selection */
	u32_t fifo_cnt; /* I2STX FIFO hardware counter max value is 0xFFFF */
#ifdef CONFIG_I2STX_SRD
	int (*srd_callback)(void *cb_data, u32_t cmd, void *param);
	void *cb_data;
	u8_t srd_wl; /* The width length detected by SRD */
#endif
	u8_t fifo_use; /* Record the used FIFO */
	u8_t i2stx_fifo_busy : 1; /* I2STX FIFO is busy as gotten by other user */
	u8_t link_with_dac : 1;   /* The flag of linkage with dac */
	u8_t irq_en : 1; /* The indicator of IRQ enable or not */
	u8_t channel_opened : 1; /* flag of channel opened */
};

static void phy_i2stx_irq_enable(struct phy_audio_device *dev);
static void phy_i2stx_irq_disable(struct phy_audio_device *dev);

/*
 * @brief  Get the I2STX controller base address
 * @param  void
 * @return  The base address of the I2STX controller
 */
static inline struct acts_audio_i2stx *get_i2stx_base(void)
{
	return (struct acts_audio_i2stx *)AUDIO_I2STX0_REG_BASE;
}

/*
 * @brief Dump the I2STX relative registers
 * @param void
 * @return void
 */
static void i2stx_dump_regs(void)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();

	SYS_LOG_INF("** i2stx contoller regster **");
	SYS_LOG_INF("          BASE: %08x", (u32_t)i2stx_reg);
	SYS_LOG_INF("     I2STX_CTL: %08x", i2stx_reg->tx_ctl);
	SYS_LOG_INF(" I2STX_FIFOCTL: %08x", i2stx_reg->fifoctl);
	SYS_LOG_INF(" I2STX_FIFOSTA: %08x", i2stx_reg->fifostat);
	SYS_LOG_INF("     I2STX_DAT: %08x", i2stx_reg->dat);
	SYS_LOG_INF("  I2STX_SRDCTL: %08x", i2stx_reg->srdctl);
	SYS_LOG_INF("  I2STX_SRDSTA: %08x", i2stx_reg->srdstat);
	SYS_LOG_INF(" I2STX_FIFOCNT: %08x", i2stx_reg->fifocnt);
	SYS_LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
	SYS_LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
	SYS_LOG_INF("    CMU_DACCLK: %08x", sys_read32(CMU_DACCLK));
	SYS_LOG_INF("  CMU_I2STXCLK: %08x", sys_read32(CMU_I2STXCLK));
#endif
}

/*
 * @brief  Disable the I2STX FIFO
 * @param  void
 * @return  void
 */
static void i2stx_fifo_disable(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	i2stx_reg->fifoctl = 0;
}

/*
 * @brief  Reset the I2STX FIFO
 * @param  void
 * @return  void
 */
#ifdef CONFIG_I2STX_SRD
static void i2stx_fifo_reset(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	i2stx_reg->fifoctl &= ~I2ST0_FIFOCTL_FIFO_RST;
	i2stx_reg->fifoctl |= I2ST0_FIFOCTL_FIFO_RST;
}
#endif

/*
 * @brief Check the I2STX FIFO is working or not
 * @param void
 * @return If true the I2STX FIFO is working
 */
static bool is_i2stx_fifo_working(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	return !!(i2stx_reg->fifoctl & I2ST0_FIFOCTL_FIFO_RST);
}

/*
 * @brief Check the I2STX FIFO is error
 * @param void
 * @return If true the I2STX FIFO is error
 */
static bool check_i2stx_fifo_error(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();

	if (i2stx_reg->fifostat & I2ST0_FIFOSTA_FIFO_ER) {
		i2stx_reg->fifostat |= I2ST0_FIFOSTA_FIFO_ER;
		return true;
	}

	return false;
}

/*
 * @brief Get the I2STX FIFO status which indicates how many samples not filled
 * @param void
 * @return The status of I2STX FIFO
 */
static u32_t i2stx_fifo_status(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	return (i2stx_reg->fifostat & I2ST0_FIFOSTA_STA_MASK) >> I2ST0_FIFOSTA_STA_SHIFT;
}

/*
 * @brief  Enable the I2STX FIFO
 * @param  sel: select the fifo input source, e.g. CPU or DMA
 * @param  width: specify the transfer width when in dma mode
 * @param  this_use: indicates the flag of the i2stx modue to use its own FIFO
 * @return void
 */
static void i2stx_fifo_enable(audio_fifouse_sel_e sel, audio_dma_width_e width, bool this_use)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	u32_t reg = 0;

	/* If the I2STX FIFO is used by SPDIFTX, the FIFO_SEL is not allow to configure, otherwise will get no dma IRQ */
	if (this_use)
		reg = I2ST0_FIFOCTL_FIFO_SEL;

	if (DMA_WIDTH_16BITS == width)
		reg |= I2ST0_FIFOCTL_TXFIFO_DMAWIDTH;

	if (FIFO_SEL_CPU == sel) {
		reg |= (I2ST0_FIFOCTL_FIFO_IEN | I2ST0_FIFOCTL_FIFO_RST);
	} else if (FIFO_SEL_DMA == sel) {
		reg |= (I2ST0_FIFOCTL_FIFO_DEN | I2ST0_FIFOCTL_FIFO_RST
				| I2ST0_FIFOCTL_FIFO_IN_SEL(1));
	} else {
		SYS_LOG_ERR("invalid fifo sel %d", sel);
	}
	i2stx_reg->fifoctl = reg;
}

/*
 * @brief  Enable the I2STX FIFO counter function
 * @return void
 */
static void i2stx_fifocount_enable(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	u32_t reg = 0;

	reg = I2ST0_FIFO_CNT_EN;

	reg |= I2ST0_FIFO_CNT_IE;

	reg |= I2ST0_FIFO_CNT_IP;

	i2stx_reg->fifocnt = reg;
	SYS_LOG_INF("I2STX FIFO counter enable");
}

/*
 * @brief  Disable the I2STX FIFO counter function
 * @return void
 */
static void i2stx_fifocount_disable(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	i2stx_reg->fifocnt &= ~(I2ST0_FIFO_CNT_EN | I2ST0_FIFO_CNT_IE);
}

/*
 * @brief  Reset the I2STX FIFO counter function and by default to enable IRQ
 * @return void
 */
static void i2stx_fifocount_reset(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	i2stx_reg->fifocnt &= ~(I2ST0_FIFO_CNT_EN | I2ST0_FIFO_CNT_IE);

	i2stx_reg->fifocnt |= I2ST0_FIFO_CNT_EN;

	i2stx_reg->fifocnt |= I2ST0_FIFO_CNT_IE;
}

/*
 * @brief  Enable the I2STX FIFO counter function
 * @param  irq_flag: I2STX FIFO counter overflow irq enable flag
 * @return I2STX FIFO counter
 */
static u32_t i2stx_read_fifocount(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	return i2stx_reg->fifocnt & I2ST0_FIFO_CNT_CNT_MASK;
}

/*
 * @brief  I2STX digital function control enable
 * @param fmt: I2S transfer format selection
 * @param bclk: Rate of BCLK with LRCLK
 * @param width: I2s effective data width
 * @param mclk: I2S MCLK source which used in slave mode
 * @param mode: I2S mode selection
 * @return void
 */
static void i2stx_digital_enable(audio_i2s_fmt_e fmt, audio_i2s_bclk_e bclk, audio_ch_width_e width,
	audio_i2s_mode_e mode, bool en)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	u32_t reg = i2stx_reg->tx_ctl & ~(0xFE);
	u8_t tx_width;

	if (CHANNEL_WIDTH_16BITS == width)
		tx_width = 0;
	else if (CHANNEL_WIDTH_20BITS == width)
		tx_width = 1;
	else
		tx_width = 2;

	reg |= (I2ST0_CTL_TXMODELSEL(fmt) | I2ST0_CTL_TXWIDTH(tx_width));

	if (I2S_BCLK_32FS == bclk)
		reg |= I2ST0_CTL_TXBCLKSET;

	if (I2S_SLAVE_MODE == mode)
		reg |= I2ST0_CTL_TXMODE;

#ifndef CONFIG_I2STX_SLAVE_MCLKSRC_INTERNAL
	if (I2S_SLAVE_MODE == mode)
		reg |= I2ST0_CTL_TX_SMCLK;
#endif

	i2stx_reg->tx_ctl = reg;

	/* I2S TX enable */
	if (en)
		i2stx_reg->tx_ctl |= I2ST0_CTL_TXEN;
}


/*
 * @brief  enable I2STX/I2SRX 5 wires mode
 * @return void
 * @note Real 5 wires enable and the I2SRX shall work in master mode
 */
static void i2stx_digital_enable_5wire(void)
{
#ifdef CONFIG_I2S_5WIRE
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();

	if (i2stx_reg->tx_ctl & I2ST0_CTL_I2SRX_5W_EN)
		i2stx_reg->tx_ctl &= ~I2ST0_CTL_I2SRX_5W_EN;

	/* Disable I2STX for sync with I2SRX on the timing */
	if (i2stx_reg->tx_ctl & I2ST0_CTL_TXEN)
		i2stx_reg->tx_ctl &= ~I2ST0_CTL_TXEN;

	i2stx_reg->tx_ctl |= I2ST0_CTL_I2SRX_5W_EN;
	i2stx_reg->tx_ctl |= I2ST0_CTL_TXEN;

	SYS_LOG_INF("Enable I2S 5 wires mode");
#endif
}

/*
 * @brief  I2STX digital function control disable
 * @param void
 * @return void
 */
static void i2stx_digital_disable(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	i2stx_reg->tx_ctl = 0;
}

/*
 * @brief  I2STX sample rate detect function configuration
 * @param srd_th: the sensitivity value of sample rate detection
 * @param period: the period of the sample rate detection
 * @param irq_en: the sample rate result change interrupt enable flag
 * @param mute: if the sample rate result change, mute the TX output as 0
 * @return 0 on success, negative errno code on fail
 */
#ifdef CONFIG_I2STX_SRD
static int i2stx_srd_cfg(u8_t srd_th, audio_i2s_srd_period_e period, bool irq_en, bool mute)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	u32_t reg;
	u8_t _wl, wl;
	u32_t start_time, curr_time;

	/* Select the I2STX SRD clock source to be HOSC */
	sys_write32(sys_read32(CMU_I2STXCLK) & ~(CMU_I2STXCLK_I2SSRDCLKSRC_MASK), CMU_I2STXCLK);

	reg = (I2ST0_SRDCTL_SRD_TH(srd_th & 0x7) | I2ST0_SRDCTL_CNT_TIM(period));
	if (irq_en)
		reg |= I2ST0_SRDCTL_SRD_IE;
	if (mute)
		reg |= I2ST0_SRDCTL_MUTE_EN;

	i2stx_reg->srdctl = reg;

	/* enable slave mode sample rate detect */
	i2stx_reg->srdctl |= I2ST0_SRDCTL_SRD_EN;


	if (I2STX_BCLK_DEFAULT == I2S_BCLK_64FS)
		wl = SRDSTA_WL_64RATE;
	else
		wl = SRDSTA_WL_32RATE;

	start_time = k_uptime_get();
	_wl = i2stx_reg->srdstat & I2ST0_SRDSTA_WL_MASK;
	while (_wl != wl) {
		curr_time = k_uptime_get_32();
		if ((curr_time - start_time) > I2STX_SRD_CONFIG_TIMEOUT_MS) {
			SYS_LOG_ERR("Wait SRD WL status timeout");
			return -ETIMEDOUT;
		}
		_wl = i2stx_reg->srdstat & I2ST0_SRDSTA_WL_MASK;
	}

	return 0;
}

/*
 * @brief get the I2STX sample rate detect counter
 * @param void
 * @return the counter of SRD
 */
static u32_t read_i2stx_srd_count(void)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	/* CNT of LRCLK which sampling by SRC_CLK */
	return ((i2stx_reg->srdstat & I2ST0_SRDSTA_CNT_MASK) >> I2ST0_SRDSTA_CNT_SHIFT);
}

static void i2stx_srd_fs_change(struct phy_audio_device *dev)
{
	struct i2stx_private_data *priv = (struct i2stx_private_data *)dev->private;
	u32_t cnt, fs;
	audio_sr_sel_e sr;
	cnt = read_i2stx_srd_count();
	/* CNT = SRD_CLK / LRCLK and SRD_CLK uses HOSC which is a 24000000Hz clock source*/
	fs = 24000000 / cnt;

	/* Allow a 1% deviation */
	if ((fs > 7920) && (fs < 8080)) { /* 8kfs */
		sr = SAMPLE_RATE_8KHZ;
	} else if ((fs > 10915) && (fs < 11135)) { /* 11.025kfs */
		sr = SAMPLE_RATE_11KHZ;
	} else if ((fs > 11880) && (fs < 12120)) { /* 12kfs */
		sr = SAMPLE_RATE_12KHZ;
	} else if ((fs > 15840) && (fs < 16160)) { /* 16kfs */
		sr = SAMPLE_RATE_16KHZ;
	} else if ((fs > 21830) && (fs < 22270)) { /* 22.05kfs */
		sr = SAMPLE_RATE_22KHZ;
	} else if ((fs > 23760) && (fs < 24240)) { /* 24kfs */
		sr = SAMPLE_RATE_24KHZ;
	} else if ((fs > 31680) && (fs < 32320)) { /* 32kfs */
		sr = SAMPLE_RATE_32KHZ;
	} else if ((fs > 43659) && (fs < 44541)) { /* 44.1kfs */
		sr = SAMPLE_RATE_44KHZ;
	} else if ((fs > 47520) && (fs < 48480)) { /* 48kfs */
		sr = SAMPLE_RATE_48KHZ;
	} else if ((fs > 63360) && (fs < 64640)) { /* 64kfs */
		sr = SAMPLE_RATE_64KHZ;
	} else if ((fs > 87318) && (fs < 89082)) { /* 88.2kfs */
		sr = SAMPLE_RATE_88KHZ;
	} else if ((fs > 95040) && (fs < 96960)) { /* 96kfs */
		sr = SAMPLE_RATE_96KHZ;
	} else if ((fs > 174636) && (fs < 178164)) { /* 176.4kfs */
		sr = SAMPLE_RATE_176KHZ;
	} else if((fs > 190080) && (fs < 193920)) { /* 192kfs */
		sr = SAMPLE_RATE_192KHZ;
	} else {
		SYS_LOG_ERR("Invalid sample rate %d", fs);
		return ;
	}

	SYS_LOG_INF("Detect new sample rate %d -> %d", fs, sr);

	/* FIXME: If not do the fifo reset, the left and right channel will exchange probably. */
	i2stx_fifo_reset();

	audio_samplerate_i2stx_set(sr,
			I2STX_MCLK_DEFAULT, priv->mclksrc, priv->link_with_dac ? true : false);

	if (priv->srd_callback)
		priv->srd_callback(priv->cb_data, I2STX_SRD_FS_CHANGE, (void *)&sr);

}

static void i2stx_srd_wl_change(struct phy_audio_device *dev)
{
	struct i2stx_private_data *priv = (struct i2stx_private_data *)dev->private;
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	u8_t width;

	width = i2stx_reg->srdstat & I2ST0_SRDSTA_WL_MASK;

	SYS_LOG_INF("Detect new width length: %d", width);

	if (((priv->srd_wl == SRDSTA_WL_64RATE) && (width == SRDSTA_WL_32RATE))
		|| ((priv->srd_wl == SRDSTA_WL_32RATE) && (width == SRDSTA_WL_64RATE))) {
		priv->srd_wl = width;
		if (priv->srd_callback)
			priv->srd_callback(priv->cb_data, I2STX_SRD_WL_CHANGE, (void *)&priv->srd_wl);
	}
}
#endif

static int phy_i2stx_prepare_enable(struct phy_audio_device *dev, aout_param_t *out_param)
{
	if ((!out_param) || (!out_param->i2stx_setting)
		|| (!out_param->sample_rate)) {
		SYS_LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (!(out_param->channel_type & AUDIO_CHANNEL_I2STX)) {
		SYS_LOG_ERR("Invalid channel type %d", out_param->channel_type);
		return -EINVAL;
	}

#if defined(CONFIG_I2S_5WIRE) && defined(CONFIG_I2S_PSEUDO_5WIRE)
#error "Can not support CONFIG_I2S_5WIRE and CONFIG_I2S_PSEUDO_5WIRE simultaneously"
#endif

#if defined(CONFIG_I2S_5WIRE) || defined(CONFIG_I2S_PSEUDO_5WIRE)
	if (out_param->i2stx_setting->mode != I2S_MASTER_MODE) {
		SYS_LOG_ERR("In 5 wires mode I2STX only support master mode");
		return -ENOTSUP;
	}
#endif

	acts_clock_peripheral_enable(CLOCK_ID_I2S0_TX);

#ifdef CONFIG_I2STX_SRD
	acts_clock_peripheral_enable(CLOCK_ID_I2S_SRD);
#endif

	return 0;
}

static int phy_i2stx_enable(struct phy_audio_device *dev, void *param)
{
	aout_param_t *out_param = (aout_param_t *)param;
	i2stx_setting_t *i2stx_setting = out_param->i2stx_setting;
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	struct i2stx_private_data *priv = (struct i2stx_private_data *)dev->private;
	int ret;
	bool link_dac = false, link_spdif = false;

	ret = phy_i2stx_prepare_enable(dev, out_param);
	if (ret) {
		SYS_LOG_ERR("Failed to prepare enable i2stx err=%d", ret);
		return ret;
	}

	if ((out_param->channel_type & AUDIO_CHANNEL_DAC)
		&& (out_param->channel_type & AUDIO_CHANNEL_SPDIFTX)) {
		link_dac = true;
		priv->link_with_dac = 1;
		priv->mclksrc = CLK_SRCTX_DAC_256FS;
	} else if (out_param->channel_type & AUDIO_CHANNEL_SPDIFTX) {
		link_spdif = true;
		priv->mclksrc = CLK_SRCTX_I2STX;
	} else {
		if (AOUT_FIFO_DAC0 == out_param->outfifo_type) {
			if (out_param->reload_setting) {
				SYS_LOG_ERR("DAC FIFO does not support reload mode");
				return -EINVAL;
			}
			acts_clock_peripheral_enable(CLOCK_ID_AUDIO_DAC);
			priv->mclksrc = CLK_SRCTX_DAC_256FS;
			priv->fifo_use = AOUT_FIFO_DAC0;
		} else {
			priv->mclksrc = CLK_SRCTX_I2STX;
		}

		if (I2S_SLAVE_MODE == i2stx_setting->mode) {
#ifndef CONFIG_I2STX_SLAVE_MCLKSRC_INTERNAL
			priv->mclksrc = CLK_SRCTX_I2STX_EXT;
#else
			if (AOUT_FIFO_DAC0 != priv->fifo_use)
				priv->mclksrc = CLK_SRCRX_I2STX_AUDIOPLL_DIV1_5;
#endif
		}
#ifdef CONFIG_I2STX_USE_ADC_256FS_CLK
		priv->mclksrc = CLK_SRCTX_ADC_256FS;
#endif
	}

	/* I2STX sample rate set */
	if (audio_samplerate_i2stx_set(out_param->sample_rate,
			I2STX_MCLK_DEFAULT, priv->mclksrc, link_dac ? true : false)) {
		SYS_LOG_ERR("Failed to config sample rate %d",
			out_param->sample_rate);
		return -ESRCH;
	}

	/* Linkage with DAC will force the I2STX use the DAC FIFO */
	if ((!link_dac) && (AOUT_FIFO_I2STX0 == out_param->outfifo_type)) {
		if (is_i2stx_fifo_working() || priv->i2stx_fifo_busy) {
			SYS_LOG_ERR("I2STX FIFO now is using ...");
			return -EACCES;
		}

		i2stx_fifo_enable(FIFO_SEL_DMA, (out_param->channel_width == CHANNEL_WIDTH_16BITS)
							? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS, true);
		priv->fifo_use = AOUT_FIFO_I2STX0;
#ifdef I2STX_FIFO_CNT_EN
		i2stx_fifocount_enable();
#endif
	}

	/* I2STX linkage with SPDIFTX */
	if (link_spdif)
		i2stx_reg->tx_ctl |= I2ST0_CTL_MULT_DEVICE;

	/* Pseudo 5 wires enable and the I2SRX shall work in slave mode */
#ifdef CONFIG_I2S_PSEUDO_5WIRE
	i2stx_reg->tx_ctl |= I2ST0_CTL_PSEUDO_5W_EN;
#else
	i2stx_reg->tx_ctl &= ~I2ST0_CTL_PSEUDO_5W_EN;
#endif

#ifndef CONFIG_I2STX_CLK_OUT_FOLLOW_DMA
	if (!link_dac)
		i2stx_digital_enable(CONFIG_I2STX_FORMAT, I2STX_BCLK_DEFAULT,
			out_param->channel_width, i2stx_setting->mode, true);
	else
#endif
		i2stx_digital_enable(CONFIG_I2STX_FORMAT, I2STX_BCLK_DEFAULT,
			out_param->channel_width, i2stx_setting->mode, false);

#ifdef CONFIG_I2STX_SRD
	if (!link_dac && (I2S_SLAVE_MODE == i2stx_setting->mode)) {
		SYS_LOG_INF("I2STX SRD enable");
		i2stx_srd_cfg(I2STX_SRD_TH_DEFAULT, I2S_SRD_2LRCLK, true, false);
		priv->srd_callback = i2stx_setting->srd_callback;
		priv->cb_data = i2stx_setting->cb_data;
		priv->srd_wl = SRDSTA_WL_64RATE;
	}
#endif

#if defined(I2STX_FIFO_CNT_EN) || defined(CONFIG_I2STX_SRD)
	/* linkage with DAC don't need to enable IRQ */
	if (!link_dac) {
		phy_i2stx_irq_enable(dev);
		priv->irq_en = 1;
	}
#endif

	if (AOUT_FIFO_I2STX0 == priv->fifo_use) {
		/* Clear FIFO ERROR */
		if (i2stx_reg->fifostat & I2ST0_FIFOSTA_FIFO_ER)
			i2stx_reg->fifostat |= I2ST0_FIFOSTA_FIFO_ER;
	}


#ifdef CONFIG_I2S_5WIRE
		u8_t is_i2srx_opened = 0;
		phy_audio_control(GET_AUDIO_DEVICE(i2srx), PHY_CMD_I2SRX_IS_OPENED, &is_i2srx_opened);
		if (is_i2srx_opened)
			i2stx_digital_enable_5wire();
#endif

	/* set flag of channel opened */
	priv->channel_opened = 1;

	return 0;
}

static int phy_i2stx_disable(struct phy_audio_device *dev)
{
	struct i2stx_private_data *priv = (struct i2stx_private_data *)dev->private;

	if (priv->fifo_use == AOUT_FIFO_I2STX0)
		i2stx_fifo_disable();

	if (priv->irq_en)
		phy_i2stx_irq_disable(dev);

	/* Don't disable I2STX until the user call iotcl #PHY_CMD_I2STX_DISABLE_DEVICE */
#ifndef CONFIG_I2STX_MAINTAIN_ALWAYS_OUTPUT
	i2stx_digital_disable();
	acts_clock_peripheral_disable(CLOCK_ID_I2S0_TX);
#endif

#ifdef CONFIG_I2STX_SRD
	acts_clock_peripheral_disable(CLOCK_ID_I2S_SRD);
	priv->srd_callback = NULL;
	priv->cb_data = NULL;
	priv->srd_wl = SRDSTA_WL_64RATE;
#endif

	priv->fifo_use = AUDIO_FIFO_INVALID_TYPE;
	priv->fifo_cnt = 0;
	priv->i2stx_fifo_busy = 0;
	priv->link_with_dac = 0;
	priv->channel_opened = 0;

	return 0;
}

static int phy_i2stx_ioctl(struct phy_audio_device *dev, u32_t cmd, void *param)
{
	struct i2stx_private_data *priv = (struct i2stx_private_data *)dev->private;
	int ret = 0;
	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		i2stx_dump_regs();
		break;
	}
	case AOUT_CMD_GET_SAMPLERATE:
	{
		ret = audio_samplerate_i2stx_get(I2STX_MCLK_DEFAULT);
		if (ret) {
			SYS_LOG_ERR("Failed to get I2STX sample rate err=%d", ret);
			return ret;
		}
		*(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
		break;
	}
	case AOUT_CMD_SET_SAMPLERATE:
	{
		audio_sr_sel_e val = *(audio_sr_sel_e *)param;
		ret = audio_samplerate_i2stx_set(val, I2STX_MCLK_DEFAULT, priv->mclksrc,
				priv->link_with_dac ? true : false);
		if (ret) {
			SYS_LOG_ERR("Failed to set I2STX sample rate err=%d", ret);
			return ret;
		}
		break;
	}
	case AOUT_CMD_GET_SAMPLE_CNT:
	{
		u32_t val;
		val = i2stx_read_fifocount();
		*(u32_t *)param = val + priv->fifo_cnt;
		SYS_LOG_DBG("I2STX FIFO counter: %d", *(u32_t *)param);
		break;
	}
	case AOUT_CMD_RESET_SAMPLE_CNT:
	{
		priv->fifo_cnt = 0;
		i2stx_fifocount_reset();
		break;
	}
	case AOUT_CMD_ENABLE_SAMPLE_CNT:
	{
		i2stx_fifocount_enable();
		phy_i2stx_irq_enable(dev);
		break;
	}
	case AOUT_CMD_DISABLE_SAMPLE_CNT:
	{
		phy_i2stx_irq_disable(dev);
		i2stx_fifocount_disable();
		break;
	}
	case AOUT_CMD_GET_CHANNEL_STATUS:
	{
		u8_t status = 0;
		if (check_i2stx_fifo_error()) {
			status = AUDIO_CHANNEL_STATUS_ERROR;
			SYS_LOG_DBG("I2STX FIFO ERROR");
		}

		if (i2stx_fifo_status() < (I2STX_FIFO_LEVEL - 1))
			status |= AUDIO_CHANNEL_STATUS_BUSY;

		*(u8_t *)param = status;
		break;
	}
	case AOUT_CMD_GET_FIFO_LEN:
	{
		*(u32_t *)param = I2STX_FIFO_LEVEL;
		break;
	}
	case AOUT_CMD_GET_FIFO_AVAILABLE_LEN:
	{
		*(u32_t *)param = i2stx_fifo_status();
		break;
	}
	case AOUT_CMD_GET_APS:
	{
		ret = audio_pll_get_i2stx_aps();
		if (ret < 0) {
			SYS_LOG_ERR("Failed to get audio pll APS err=%d", ret);
			return ret;
		}
		*(audio_aps_level_e *)param = (audio_aps_level_e)ret;
		break;
	}
	case AOUT_CMD_SET_APS:
	{
		audio_aps_level_e level = *(audio_aps_level_e *)param;
		ret = audio_pll_set_i2stx_aps(level);
		if (ret) {
			SYS_LOG_ERR("Failed to set audio pll APS err=%d", ret);
			return ret;
		}
		break;
	}
	case PHY_CMD_FIFO_GET:
	{
		aout_param_t *out_param = (aout_param_t *)param;
		if (!out_param)
			return -EINVAL;

		if (is_i2stx_fifo_working()) {
			SYS_LOG_ERR("I2STX FIFO is using");
			return -EBUSY;
		}

		i2stx_fifo_enable(FIFO_SEL_DMA,
			(out_param->channel_width == CHANNEL_WIDTH_16BITS)
			? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS, false);

		priv->i2stx_fifo_busy = 1;
		break;
	}
	case PHY_CMD_FIFO_PUT:
	{
		if (is_i2stx_fifo_working())
			i2stx_fifo_disable();
		priv->fifo_cnt = 0;
		priv->i2stx_fifo_busy = 0;
		break;
	}
	case PHY_CMD_I2S_ENABLE_5WIRE:
	{
		i2stx_digital_enable_5wire();
		break;
	}
	case PHY_CMD_I2STX_IS_OPENED:
	{
		*(u8_t *)param = priv->channel_opened;
		break;
	}
	case PHY_CMD_I2STX_DISABLE_DEVICE:
	{
		i2stx_digital_disable();
		acts_clock_peripheral_disable(CLOCK_ID_I2S0_TX);
		break;
	}
	case PHY_CMD_CHANNEL_START:
	{
#ifdef CONFIG_I2STX_CLK_OUT_FOLLOW_DMA
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();
	i2stx_reg->tx_ctl |= I2ST0_CTL_TXEN;
#endif
		break;
	}
	default:
		SYS_LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;
	}

	return ret;
}

static int phy_i2stx_init(struct phy_audio_device *dev, struct device *parent_dev)
{
	struct i2stx_private_data *priv = (struct i2stx_private_data *)dev->private;
	memset(priv, 0, sizeof(struct i2stx_private_data));

	priv->parent_dev = parent_dev;
	priv->fifo_use = AUDIO_FIFO_INVALID_TYPE;

	SYS_LOG_INF("PHY I2STX probe");

	return 0;
}

void phy_i2stx_isr(void *arg)
{
	struct phy_audio_device *dev = (struct phy_audio_device *)arg;
	struct i2stx_private_data *priv = (struct i2stx_private_data *)dev->private;

	struct acts_audio_i2stx *i2stx_reg = get_i2stx_base();

	if (i2stx_reg->fifocnt & I2ST0_FIFO_CNT_IP) {
		priv->fifo_cnt += (AOUT_FIFO_CNT_MAX + 1);
		/* Here we need to wait 100us for the synchronization of audio clock fields */
		k_busy_wait(100);
		i2stx_reg->fifocnt |= I2ST0_FIFO_CNT_IP;
	}

#ifdef CONFIG_I2STX_SRD
	/* Sample rate detection timeout irq pending */
	if (i2stx_reg->srdstat & I2ST0_SRDSTA_TO_PD) {
		i2stx_reg->srdstat |= I2ST0_SRDSTA_TO_PD;
		if (priv->srd_callback)
			priv->srd_callback(priv->cb_data, I2STX_SRD_TIMEOUT, NULL);
	}

	/* Sample rate changed detection irq pending */
	if (i2stx_reg->srdstat & I2ST0_SRDSTA_SRC_PD) {
		i2stx_reg->srdstat |= I2ST0_SRDSTA_SRC_PD;
		i2stx_srd_fs_change(dev);
	}

	/* Channel width change irq pending */
	if (i2stx_reg->srdstat & I2ST0_SRDSTA_CHW_PD) {
		i2stx_reg->srdstat |= I2ST0_SRDSTA_CHW_PD;
		i2stx_srd_wl_change(dev);
	}
#endif
}

static struct i2stx_private_data i2stx_priv;

const struct phy_audio_operations i2stx_aops = {
	.audio_init = phy_i2stx_init,
	.audio_enable = phy_i2stx_enable,
	.audio_disable = phy_i2stx_disable,
	.audio_ioctl = phy_i2stx_ioctl,
};

PHY_AUDIO_DEVICE(i2stx, PHY_AUDIO_I2STX_NAME, &i2stx_aops, &i2stx_priv);
PHY_AUDIO_DEVICE_FN(i2stx);

/*
 * @brief Enable I2STX IRQ
 * @note I2STX IRQ source as shown below:
 *	- I2STX FIFO Half Empty IRQ
 *	- I2STX FIFO CNT IRQ
 *	- I2STX SRDTO IRQ
 *	- I2STX SRDSR IRQ
 *	- I2STX SRDCHW IRQ
 */
static void phy_i2stx_irq_enable(struct phy_audio_device *dev)
{
	ARG_UNUSED(dev);
	if (irq_is_enabled(IRQ_ID_I2S0_TX) == 0) {
		IRQ_CONNECT(IRQ_ID_I2S0_TX, CONFIG_IRQ_PRIO_NORMAL,
			phy_i2stx_isr, PHY_AUDIO_DEVICE_GET(i2stx), 0);
		irq_enable(IRQ_ID_I2S0_TX);
	}
}

static void phy_i2stx_irq_disable(struct phy_audio_device *dev)
{
	ARG_UNUSED(dev);
	if(irq_is_enabled(IRQ_ID_I2S0_TX) != 0)
		irq_disable(IRQ_ID_I2S0_TX);
}
