/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2SRX physical implementation
 */

/*
 * Features
 *    - Support master and slave mode
 *    - Support I2S 2ch format
 *	- I2SRXFIFO(32 x 24bits level)
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
#include "audio_in.h"

#define SYS_LOG_DOMAIN "P_I2SRX"
#include <logging/sys_log.h>

/***************************************************************************************************
 * I2SRX_CTL
 */
#define I2SR0_CTL_TDMRX_CHAN                                  BIT(16) /* 4 channel or 8 channel */
#define I2SR0_CTL_TDMRX_MODE                                  BIT(15) /* I2S format or left-justified format */

#define I2SR0_CTL_TDMRX_SYNC_SHIFT                            (13) /* at the beginning of the data frame, the type of LRCLK setting */
#define I2SR0_CTL_TDMRX_SYNC_MASK                             (0x3 << I2SR0_CTL_TDMRX_SYNC_SHIFT)
#define I2SR0_CTL_TDMRX_SYNC(x)                               ((x) << I2SR0_CTL_TDMRX_SYNC_SHIFT)

#define I2SR0_CTL_RXMODE                                      BIT(7) /* I2S mode selection */
#define I2SR0_CTL_RX_SMCLK                                    BIT(6) /* MCLK source when in slave mode */

#define I2SR0_CTL_RXWIDTH_SHIFT                               (4) /* effective width */
#define I2SR0_CTL_RXWIDTH_MASK                                (0x3 << I2SR0_CTL_RXWIDTH_SHIFT)
#define I2SR0_CTL_RXWIDTH(x)                                  ((x) << I2SR0_CTL_RXWIDTH_SHIFT)

#define I2SR0_CTL_RXBCLKSET                                   BIT(3) /* rate of BCLK with LRCLK */

#define I2SR0_CTL_RXMODELSEL_SHIFT                            (1) /* I2S transfer format select */
#define I2SR0_CTL_RXMODELSEL_MASK                             (0x3 << I2SR0_CTL_RXMODELSEL_SHIFT)
#define I2SR0_CTL_RXMODELSEL(x)                               ((x) << I2SR0_CTL_RXMODELSEL_SHIFT)

#define I2SR0_CTL_RXEN                                        BIT(0) /* I2S RX enable */

/***************************************************************************************************
 * I2SRX_FIFOCTL
 */
#define I2SR0_FIFOCTL_RXFIFO_DMAWIDTH                         BIT(7) /* DMA transfer width */

#define I2SR0_FIFOCTL_RXFOS_SHIFT                             (4) /* RX FIFO output select */
#define I2SR0_FIFOCTL_RXFOS_MASK                              (0x3 << I2SR0_FIFOCTL_RXFOS_SHIFT)
#define I2SR0_FIFOCTL_RXFOS(x)                                ((x) << I2SR0_FIFOCTL_RXFOS_SHIFT)

#define I2SR0_FIFOCTL_RXFFIE                                  BIT(2) /* RX FIFO half filled irq enable */
#define I2SR0_FIFOCTL_RXFFDE                                  BIT(1) /* RX FIFO half filed drq enable */
#define I2SR0_FIFOCTL_RXFRT                                   BIT(0) /* RX FIFO reset */

/***************************************************************************************************
 * I2SRX_STAT
 */
#define I2SR0_FIFOSTA_FIFO_ER                                 BIT(8) /* FIFO error */
#define I2SR0_FIFOSTA_RXFEF                                   BIT(7) /* RX FIFO empty flag */
#define I2SR0_FIFOSTA_RXFIP                                   BIT(6) /* RX FIFO half filled irq pending */
#define I2SR0_FIFOSTA_RXFS_SHIFT                              (0) /* RX FIFO status */
#define I2SR0_FIFOSTA_RXFS_MASK                               (0x3F << I2SR0_FIFOSTA_RXFS_SHIFT)

/***************************************************************************************************
 * I2SRX_DAT
 */
#define I2SR0_DAT_RXDAT_SHIFT                                 (8) /* I2S RX FIFO data */
#define I2SR0_DAT_RXDAT_MASK                                  (0xFFFFFF << I2SR0_DAT_RXDAT_SHIFT)

/***************************************************************************************************
 * I2SRX_CTL
 */
#define I2SR0_SRDCTL_MUTE_EN                                  BIT(12) /* If detect sample rate or channel width changing, mute output */
#define I2SR0_SRDCTL_SRD_IE                                   BIT(8) /* sample rate detect result change interrupt enable */

#define I2SR0_SRDCTL_CNT_TIM_SHIFT                            (4) /* slave mode rample rate detect counter period select */
#define I2SR0_SRDCTL_CNT_TIM_MASK                             (0x3 << I2SR0_SRDCTL_CNT_TIM_SHIFT)

#define I2SR0_SRDCTL_SRD_TH_SHIFT                             (1) /* the sampling sensitivity */
#define I2SR0_SRDCTL_SRD_TH_MASK                              (0x7 << I2SR0_SRDCTL_SRD_TH_SHIFT)
#define I2SR0_SRDCTL_SRD_TH(x)                                ((x) << I2SR0_SRDCTL_SRD_TH_SHIFT)

#define I2SR0_SRDCTL_SRD_EN                                   BIT(0) /* slave mode sample rate detect enable */

/***************************************************************************************************
 * I2SRX_SRDSTA
 */
#define I2SR0_SRDSTA_CNT_SHIFT                                (12) /* CNT of LRCLK which sampling by SRC_CLK */
#define I2SR0_SRDSTA_CNT_MASK                                 (0x1FFF << I2SR0_SRDSTA_CNT_SHIFT)

#define I2SR0_SRDSTA_TO_PD                                    BIT(11) /* sample rate changing detection timeout interrupt pending */
#define I2SR0_SRDSTA_SRC_PD                                   BIT(10) /* sample rate changing detection interrupt pending */
#define I2SR0_SRDSTA_CHW_PD                                   BIT(8) /* channel width change interrupt pending */
#define I2SR0_SRDSTA_WL_SHIFT                                 (0) /* channel word length */
#define I2SR0_SRDSTA_WL_MASK                                  (0x7 << I2SR0_SRDSTA_WL_SHIFT)

/***************************************************************************************************
 * i2SRX FEATURES CONGIURATION
 */
/* The sensitivity of the SRD */
#define I2SRX_SRD_TH_DEFAULT                                  (7)

#define I2SRX_BCLK_DEFAULT                                    (I2S_BCLK_64FS)
#define I2SRX_MCLK_DEFAULT                                    (MCLK_256FS) /* MCLK = 4BCLK */

#define I2SRX_SRD_CONFIG_TIMEOUT_MS                           (500)

/*
 * @struct acts_audio_i2srx
 * @brief I2SRX controller hardware register
 */
struct acts_audio_i2srx {
	volatile u32_t rx_ctl; /* I2SRX control */
	volatile u32_t fifoctl; /* I2SRX FIFO control */
	volatile u32_t fifostat; /* I2SRX FIFO state */
	volatile u32_t dat; /* I2SRX FIFO data */
	volatile u32_t srdctl; /* I2S slave mode RX sample rate detect control */
	volatile u32_t srdstat; /* I2S slave mode RX sample rate detect state */
};

/*
 * struct i2srx_private_data
 * @brief Physical I2SRX device private data
 */
struct i2srx_private_data {
	struct device *parent_dev; /* Indicates the parent audio device */
	a_i2srx_mclk_clksrc_e mclksrc; /* MCLK clock source selection used in slave mode */
#ifdef CONFIG_I2SRX_SRD
	int (*srd_callback)(void *cb_data, u32_t cmd, void *param);
	void *cb_data;
	u8_t srd_wl; /* The width length detected by SRD */
#endif
	u8_t sample_rate;
	u8_t channel_opened : 1; /* flag of channel opened */
};

#ifdef CONFIG_I2SRX_SRD
static void phy_i2srx_irq_enable(struct phy_audio_device *dev);
static void phy_i2srx_irq_disable(struct phy_audio_device *dev);
#endif

/*
 * @brief  Get the I2SRX controller base address
 * @param  void
 * @return  The base address of the I2SRX controller
 */
static inline struct acts_audio_i2srx *get_i2srx_base(void)
{
	return (struct acts_audio_i2srx *)AUDIO_I2SRX0_REG_BASE;
}

/*
 * @brief Dump the I2SRX relative registers
 * @param void
 * @return void
 */
static void i2srx_dump_regs(void)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();

	SYS_LOG_INF("** i2srx contoller regster **");
	SYS_LOG_INF("          BASE: %08x", (u32_t)i2srx_reg);
	SYS_LOG_INF("     I2SRX_CTL: %08x", i2srx_reg->rx_ctl);
	SYS_LOG_INF(" I2SRX_FIFOCTL: %08x", i2srx_reg->fifoctl);
	SYS_LOG_INF(" I2SRX_FIFOSTA: %08x", i2srx_reg->fifostat);
	SYS_LOG_INF("     I2SRX_DAT: %08x", i2srx_reg->dat);
	SYS_LOG_INF("  I2SRX_SRDCTL: %08x", i2srx_reg->srdctl);
	SYS_LOG_INF("  I2SRX_SRDSTA: %08x", i2srx_reg->srdstat);
	SYS_LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
	SYS_LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
	SYS_LOG_INF("  CMU_I2SRXCLK: %08x", sys_read32(CMU_I2SRXCLK));
#endif
}

/*
 * @brief  Disable the I2SRX FIFO
 * @param  void
 * @return  void
 */
static void i2srx_fifo_disable(void)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	i2srx_reg->fifoctl &= ~I2SR0_CTL_RXEN;
}
#ifdef CONFIG_I2SRX_SRD

/*
 * @brief  Reset the I2SRX FIFO
 * @param  void
 * @return  void
 */
static void i2srx_fifo_reset(void)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	i2srx_reg->fifoctl &= ~I2SR0_CTL_RXEN;
	i2srx_reg->fifoctl |= I2SR0_CTL_RXEN;
}
#endif
/*
 * @brief  Enable the I2SRX FIFO
 * @param  sel: select the fifo input source, e.g. CPU or DMA
 * @param  width: specify the transfer width when in dma mode
 * @return void
 */
static void i2srx_fifo_enable(audio_fifouse_sel_e sel, audio_dma_width_e width)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	u32_t reg = 0;

	if (DMA_WIDTH_16BITS == width)
		reg |= I2SR0_FIFOCTL_RXFIFO_DMAWIDTH;

	if (FIFO_SEL_CPU == sel) {
		reg |= (I2SR0_FIFOCTL_RXFFIE | I2SR0_FIFOCTL_RXFRT);
	} else if (FIFO_SEL_DMA == sel) {
		reg |= (I2SR0_FIFOCTL_RXFFDE | I2SR0_FIFOCTL_RXFRT
				| I2SR0_FIFOCTL_RXFOS(1));
	} else {
		SYS_LOG_ERR("invalid fifo sel %d", sel);
	}
	i2srx_reg->fifoctl = reg;
}

/*
 * @brief  I2SRX digital function control
 * @param fmt: I2S transfer format selection
 * @param bclk: Rate of BCLK with LRCLK
 * @param width: I2s effective data width
 * @param mclk: I2S MCLK source which used in slave mode
 * @param mode: I2S mode selection
 * @return void
 */
static void i2srx_digital_enable(audio_i2s_fmt_e fmt, audio_i2s_bclk_e bclk, audio_ch_width_e width,
	audio_i2s_mode_e mode)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	u32_t reg;
	u8_t rx_width;

	if (CHANNEL_WIDTH_16BITS == width)
		rx_width = 0;
	else if (CHANNEL_WIDTH_20BITS == width)
		rx_width = 1;
	else
		rx_width = 2;

	reg = (I2SR0_CTL_RXMODELSEL(fmt) | I2SR0_CTL_RXWIDTH(rx_width));

	if (I2S_SLAVE_MODE == mode)
		reg |= I2SR0_CTL_RXMODE;
	if (I2S_BCLK_32FS == bclk)
		reg |= I2SR0_CTL_RXBCLKSET;

#if !defined(CONFIG_I2SRX_SLAVE_MCLKSRC_INTERNAL) && !defined(CONFIG_I2S_PSEUDO_5WIRE)
	if (I2S_SLAVE_MODE == mode)
		reg |= I2SR0_CTL_RX_SMCLK;
#endif

	i2srx_reg->rx_ctl = reg;

#ifndef CONFIG_I2S_5WIRE
	/* I2S RX enable */
	i2srx_reg->rx_ctl |= I2SR0_CTL_RXEN;
#endif
}

/*
 * @brief  Disable I2SRX digital function
 * @param void
 * @return void
 */
static void i2srx_digital_disable(void)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	i2srx_reg->rx_ctl = 0;
}

#ifdef CONFIG_I2SRX_SRD
/*
 * @brief get the I2SRX sample rate detect counter
 * @param void
 * @return the counter of SRD
 */
static u32_t read_i2srx_srd_count(void)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	/* CNT of LRCLK which sampling by SRC_CLK */
	return ((i2srx_reg->srdstat & I2SR0_SRDSTA_CNT_MASK) >> I2SR0_SRDSTA_CNT_SHIFT);
}

static void i2srx_srd_fs_change(struct phy_audio_device *dev)
{
	struct i2srx_private_data *priv = (struct i2srx_private_data *)dev->private;
	u32_t cnt, fs;
	audio_sr_sel_e sr;
	cnt = read_i2srx_srd_count();
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
	i2srx_fifo_reset();

	if (priv->sample_rate != sr) {
		audio_samplerate_i2srx_set(sr,
				I2SRX_MCLK_DEFAULT, priv->mclksrc);
		priv->sample_rate = sr;
	}

	if (priv->srd_callback)
		priv->srd_callback(priv->cb_data, I2SRX_SRD_FS_CHANGE, (void *)&sr);
}

static void i2srx_srd_wl_change(struct phy_audio_device *dev)
{
	struct i2srx_private_data *priv = (struct i2srx_private_data *)dev->private;
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	u8_t width;

	width = i2srx_reg->srdstat & I2SR0_SRDSTA_WL_MASK;

	SYS_LOG_DBG("Detect new width length: %d", width);

	if (((priv->srd_wl == SRDSTA_WL_64RATE) && (width == SRDSTA_WL_32RATE))
		|| ((priv->srd_wl == SRDSTA_WL_32RATE) && (width == SRDSTA_WL_64RATE))) {
		priv->srd_wl = width;
		if (priv->srd_callback)
			priv->srd_callback(priv->cb_data, I2SRX_SRD_WL_CHANGE, (void *)&priv->srd_wl);
	}
}

/*
 * @brief  I2SRX sample rate detect function configuration
 * @param srd_th: the sensitivity value of sample rate detection
 * @param period: the period of the sample rate detection
 * @param irq_en: the sample rate result change interrupt enable flag
 * @param mute: if the sample rate result change, mute the TX output as 0
 * @return 0 on success, negative errno code on fail
 */
static int i2srx_srd_cfg(struct phy_audio_device *dev, u8_t srd_th, audio_i2s_srd_period_e period, bool irq_en, bool mute)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();
	u32_t reg;
	u8_t _wl, wl;
	u32_t start_time, curr_time;

	reg = (I2SR0_SRDCTL_SRD_TH(srd_th & 0x7) | I2SR0_SRDCTL_SRD_TH(period));
	if (irq_en)
		reg |= I2SR0_SRDCTL_SRD_IE;
	if (mute)
		reg |= I2SR0_SRDCTL_MUTE_EN;

	i2srx_reg->srdctl = reg;

	/* enable slave mode sample rate detect */
	i2srx_reg->srdctl |= I2SR0_SRDCTL_SRD_EN;

	if (I2SRX_BCLK_DEFAULT == I2S_BCLK_64FS)
		wl = SRDSTA_WL_64RATE;
	else
		wl = SRDSTA_WL_32RATE;

	start_time = k_uptime_get();
	_wl = i2srx_reg->srdstat & I2SR0_SRDSTA_WL_MASK;
	while (_wl != wl) {
		curr_time = k_uptime_get_32();
		if ((curr_time - start_time) > I2SRX_SRD_CONFIG_TIMEOUT_MS) {
			SYS_LOG_ERR("Wait SRD WL status timeout");
			return -ETIMEDOUT;
		}
		_wl = i2srx_reg->srdstat & I2SR0_SRDSTA_WL_MASK;
		k_sleep(2);
	}
	i2srx_reg->srdstat |= I2SR0_SRDSTA_CHW_PD;

	i2srx_srd_fs_change(dev);

	return 0;
}
#endif

static int phy_i2srx_enable(struct phy_audio_device *dev, void *param)
{
	ain_param_t *in_param = (ain_param_t *)param;
	i2srx_setting_t *i2srx_setting = in_param->i2srx_setting;
	struct i2srx_private_data *priv = (struct i2srx_private_data *)dev->private;

	if ((!in_param) || (!i2srx_setting)
		|| (!in_param->sample_rate)) {
		SYS_LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (in_param->channel_type != AUDIO_CHANNEL_I2SRX) {
		SYS_LOG_ERR("Invalid channel type %d", in_param->channel_type);
		return -EINVAL;
	}

#ifdef CONFIG_I2S_5WIRE
	if (i2srx_setting->mode != I2S_MASTER_MODE) {
		SYS_LOG_ERR("In 5 wires mode I2SRX only support master mode");
		return -ENOTSUP;
	}
#endif

#ifdef CONFIG_I2S_PSEUDO_5WIRE
	if (i2srx_setting->mode != I2S_SLAVE_MODE) {
		SYS_LOG_ERR("In pseudo 5 wires mode I2SRX only support slave mode");
		return -ENOTSUP;
	}
#endif

	/* enable adc clock */
	acts_clock_peripheral_enable(CLOCK_ID_I2S0_RX);

#ifdef CONFIG_I2SRX_SRD
	acts_clock_peripheral_enable(CLOCK_ID_I2S_SRD);
#endif

#if defined(CONFIG_I2SRX_USE_I2SRX_CLK)
	priv->mclksrc = CLK_SRCRX_I2SRX;
	if (I2S_SLAVE_MODE == i2srx_setting->mode) {
#ifndef CONFIG_I2SRX_SLAVE_MCLKSRC_INTERNAL
		priv->mclksrc = CLK_SRCRX_I2SRX_EXT;
#else
		priv->mclksrc = CLK_SRCRX_I2SRX_AUDIOPLL_DIV1_5;
#endif
	}
#elif defined(CONFIG_I2SRX_USE_I2STX_MCLK)
	priv->mclksrc = CLK_SRCRX_I2STX;
#elif defined(CONFIG_I2SRX_USE_ADC_256FS_CLK)
	priv->mclksrc = CLK_SRCRX_ADC_256FS;
#endif

#if defined(CONFIG_I2S_5WIRE) || defined(CONFIG_I2S_PSEUDO_5WIRE)
	priv->mclksrc = CLK_SRCRX_I2STX;
#endif

	/* I2SRX sample rate set */
	if (audio_samplerate_i2srx_set(in_param->sample_rate,
			I2SRX_MCLK_DEFAULT, priv->mclksrc)) {
		SYS_LOG_ERR("Failed to config sample rate %d",
			in_param->sample_rate);
		return -ESRCH;
	}

	i2srx_fifo_enable(FIFO_SEL_DMA, (in_param->channel_width == CHANNEL_WIDTH_16BITS)
						? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS);

	i2srx_digital_enable(CONFIG_I2SRX_FORMAT, I2SRX_BCLK_DEFAULT,
		in_param->channel_width, i2srx_setting->mode);

#ifdef CONFIG_I2SRX_SRD
	if (I2S_SLAVE_MODE == i2srx_setting->mode) {
		SYS_LOG_INF("I2SRX SRD enable");
		i2srx_srd_cfg(dev, I2SRX_SRD_TH_DEFAULT, I2S_SRD_2LRCLK, true, false);
		priv->srd_callback = i2srx_setting->srd_callback;
		priv->cb_data = i2srx_setting->cb_data;
		priv->srd_wl = SRDSTA_WL_64RATE;
		phy_i2srx_irq_enable(dev);
	}
#endif

#ifdef CONFIG_I2S_5WIRE
	u8_t is_i2stx_opened = 0;
	phy_audio_control(GET_AUDIO_DEVICE(i2stx), PHY_CMD_I2STX_IS_OPENED, &is_i2stx_opened);
	if (is_i2stx_opened)
		phy_audio_control(GET_AUDIO_DEVICE(i2stx), PHY_CMD_I2S_ENABLE_5WIRE, NULL);
#endif

	priv->channel_opened = 1;

	return 0;
}

static int phy_i2srx_disable(struct phy_audio_device *dev)
{
	struct i2srx_private_data *priv = (struct i2srx_private_data *)dev->private;

	i2srx_fifo_disable();

	i2srx_digital_disable();

#ifdef CONFIG_I2SRX_SRD
	phy_i2srx_irq_disable(dev);
	acts_clock_peripheral_disable(CLOCK_ID_I2S_SRD);
	priv->srd_callback = NULL;
	priv->cb_data = NULL;
	priv->srd_wl = SRDSTA_WL_64RATE;
#endif
	priv->sample_rate = 0;
	priv->channel_opened = 0;

	acts_clock_peripheral_disable(CLOCK_ID_I2S0_RX);

	return 0;
}

static int phy_i2srx_ioctl(struct phy_audio_device *dev, u32_t cmd, void *param)
{
	struct i2srx_private_data *priv = (struct i2srx_private_data *)dev->private;
	int ret;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		i2srx_dump_regs();
		break;
	}
	case AIN_CMD_I2SRX_QUERY_SAMPLE_RATE:
	{
		*(audio_sr_sel_e *)param = priv->sample_rate;
		break;
	}
	case PHY_CMD_I2SRX_IS_OPENED:
	{
		*(u8_t *)param = priv->channel_opened;
		break;
	}
	case AIN_CMD_GET_SAMPLERATE:
	{
		ret = audio_samplerate_i2srx_get(priv->mclksrc);
		if (ret) {
			SYS_LOG_ERR("Failed to get I2SRX sample rate err=%d", ret);
			return ret;
		}
		*(audio_sample_rate_e *)param = (audio_sr_sel_e)ret;
		break;
	}
	case AIN_CMD_SET_SAMPLERATE:
	{
		audio_sample_rate_e val = *(audio_sr_sel_e *)param;
		ret = audio_samplerate_i2srx_set(val, I2SRX_MCLK_DEFAULT, priv->mclksrc);
		if (ret) {
			SYS_LOG_ERR("Failed to set I2SRX sample rate err=%d", ret);
			return ret;
		}
		break;
	}
	case AIN_CMD_GET_APS:
	{
		ret = audio_pll_get_i2srx_aps();
		if (ret < 0) {
			SYS_LOG_ERR("Failed to get audio pll APS err=%d", ret);
			return ret;
		}
		*(audio_aps_level_e *)param = (audio_aps_level_e)ret;
		break;
	}
	case AIN_CMD_SET_APS:
	{
		audio_aps_level_e level = *(audio_aps_level_e *)param;
		ret = audio_pll_set_i2srx_aps(level);
		if (ret) {
			SYS_LOG_ERR("Failed to set audio pll APS err=%d", ret);
			return ret;
		}
		SYS_LOG_DBG("set new aps level %d", level);
		break;
	}
	default:
		SYS_LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;
	}

	return 0;
}

static int phy_i2srx_init(struct phy_audio_device *dev, struct device *parent_dev)
{
	struct i2srx_private_data *priv = (struct i2srx_private_data *)dev->private;
	memset(priv, 0, sizeof(struct i2srx_private_data));
	priv->parent_dev = parent_dev;

	SYS_LOG_INF("PHY I2SRX probe");
	return 0;
}

void phy_i2srx_isr(void *arg)
{
#ifdef CONFIG_I2SRX_SRD

	struct phy_audio_device *dev = (struct phy_audio_device *)arg;
	struct i2srx_private_data *priv = (struct i2srx_private_data *)dev->private;

	struct acts_audio_i2srx *i2srx_reg = get_i2srx_base();

	SYS_LOG_DBG("srdstat: 0x%x", i2srx_reg->srdstat);


	/* Sample rate detection timeout irq pending */
	if (i2srx_reg->srdstat & I2SR0_SRDSTA_TO_PD) {
		i2srx_reg->srdstat |= I2SR0_SRDSTA_TO_PD;
		if (priv->srd_callback)
			priv->srd_callback(priv->cb_data, I2SRX_SRD_TIMEOUT, NULL);
	}

	/* Sample rate changed detection irq pending */
	if (i2srx_reg->srdstat & I2SR0_SRDSTA_SRC_PD) {
		i2srx_reg->srdstat |= I2SR0_SRDSTA_SRC_PD;
		i2srx_srd_fs_change(dev);
	}

	/* Channel width change irq pending */
	if (i2srx_reg->srdstat & I2SR0_SRDSTA_CHW_PD) {
		i2srx_reg->srdstat |= I2SR0_SRDSTA_CHW_PD;
		i2srx_srd_wl_change(dev);
	}
#endif
}

static struct i2srx_private_data i2srx_priv;

const struct phy_audio_operations i2srx_aops = {
	.audio_init = phy_i2srx_init,
	.audio_enable = phy_i2srx_enable,
	.audio_disable = phy_i2srx_disable,
	.audio_ioctl = phy_i2srx_ioctl,
};

PHY_AUDIO_DEVICE(i2srx, PHY_AUDIO_I2SRX_NAME, &i2srx_aops, &i2srx_priv);
PHY_AUDIO_DEVICE_FN(i2srx);

/*
 * @brief Enable I2SRX IRQ
 * @note I2STX IRQ source as shown below:
 *	- I2STX FIFO Half Filled IRQ
 *	- I2STX SRDTO IRQ
 *	- I2STX SRDSR IRQ
 *	- I2STX SRDCHW IRQ
 */
#ifdef CONFIG_I2SRX_SRD
static void phy_i2srx_irq_enable(struct phy_audio_device *dev)
{
	if (irq_is_enabled(IRQ_ID_I2S0_RX) == 0) {
		IRQ_CONNECT(IRQ_ID_I2S0_RX, CONFIG_IRQ_PRIO_NORMAL,
			phy_i2srx_isr, PHY_AUDIO_DEVICE_GET(i2srx), 0);
		irq_enable(IRQ_ID_I2S0_RX);
	}
}

static void phy_i2srx_irq_disable(struct phy_audio_device *dev)
{
	ARG_UNUSED(dev);

	if(irq_is_enabled(IRQ_ID_I2S0_RX) != 0) {
		irq_disable(IRQ_ID_I2S0_RX);
	}
}
#endif
