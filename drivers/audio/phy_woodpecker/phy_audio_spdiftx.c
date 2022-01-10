/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio SPDIFTX physical implementation
 */

/*
 * Features
 *    - 32 level * 24bits FIFO
 *    - Support multiple devices (DAC + I2STX0 + SPDIFTX)
 *    - Sample rate support 32k/44.1k/48k/88.2k/96k/176.4k/192k
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board.h>
#include <audio_common.h>
#include "../phy_audio_common.h"
#include "../audio_acts_utils.h"
#include "audio_out.h"

#define SYS_LOG_DOMAIN "P_SPIDFTX"
#include <logging/sys_log.h>

/***************************************************************************************************
 * SPDIFTX_CTL
 */
#define SPDIFT0_CTL_FIFO_SEL                             BIT(3) /* FIFO select 0: I2STX FIFO; 1: DACFIFO(PCMBUF) */
#define SPDIFT0_CTL_VALIDITY                             BIT(2) /* validity flag */
#define SPDIFT0_CTL_SPD_DIS_CTL                          BIT(1) /* 0: close spdif immediately; 1: close spidf after right channel has stopped */
#define SPDIFT0_CTL_SPDEN                                BIT(0) /* SPDIFTX Enable */

/***************************************************************************************************
 * SPDIFTX_CSL
 */
#define SPDIFT0_CSL_SPDCSL_E                             (31) /* SPDIFTX Channel State Low */
#define SPDIFT0_CSL_SPDCSL_SHIFT                         (0)
#define SPDIFT0_CSL_SPDCSL_MASK                          (0xFFFFFFFF << SPDIFT0_CSL_SPDCSL_SHIFT)

/***************************************************************************************************
 * SPDIFTX_CSH
 */
#define SPDIFT0_CSH_SPDCSH_E                             (15) /* SPDIFTX Channel State High */
#define SPDIFT0_CSH_SPDCSH_SHIFT                         (0)
#define SPDIFT0_CSH_SPDCSH_MASK                          (0xFFFF << SPDIFT0_CSH_SPDCSH_SHIFT)

#define SPDTX_CTL_SPDEN						             BIT(0)
#define SPDTX_CTL_SPD_DIS_CTL                            BIT(1)
#define SPDTX_CTL_VALIDITY                               BIT(2)

/* test spdif channel state */
#define TEST_SPDTX_CSL                                   (0x12345678)
#define TEST_SPDTX_CSH                                   (0x9abc)

/* The SPDIF pin will consume about 200uA, and need to control independently */
static const struct acts_pin_config board_pin_spdiftx_config[] = {
#ifdef BOARD_SPDIFTX_MAP
	BOARD_SPDIFTX_MAP
#endif
};

/*
 * @struct acts_audio_spdiftx
 * @brief SPDIFTX controller hardware register
 */
struct acts_audio_spdiftx {
	volatile u32_t ctl; /* SPDIFTX Control */
	volatile u32_t csl; /* SPDIFTX Channel State Low */
	volatile u32_t csh; /* SPDIFTX Channel State High */
};

/*
 * struct spdiftx_private_data
 * @brief Physical SPDIFTX device private data
 */
struct spdiftx_private_data {
	struct device *parent_dev; /* Indicates the parent audio device */
	a_spdiftx_clksrc_e clksrc; /* spdiftx clock source selection */
	u8_t fifo_use;             /* Record the used FIFO */
	u8_t link_with_i2stx : 1;      /* The flag of linkage with i2stx */
};

/*
 * @brief  Get the SPDIFTX controller base address
 * @param  void
 * @return  The base address of the SPDIFTX controller
 */
static inline struct acts_audio_spdiftx *get_spdiftx_base(void)
{
	return (struct acts_audio_spdiftx *)AUDIO_SPDIFTX_REG_BASE;
}

/*
 * @brief Dump the SPDIFTX relative registers
 * @param void
 * @return void
 */
static void spdiftx_dump_regs(void)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	struct acts_audio_spdiftx *spdiftx_base = (struct acts_audio_spdiftx *)get_spdiftx_base();

	SYS_LOG_INF("** spdiftx contoller regster **");
	SYS_LOG_INF("          BASE: %08x", (u32_t)spdiftx_base);
	SYS_LOG_INF("   SPDIFTX_CTL: %08x", spdiftx_base->ctl);
	SYS_LOG_INF("   SPDIFTX_CSL: %08x", spdiftx_base->csl);
	SYS_LOG_INF("   SPDIFTX_CSH: %08x", spdiftx_base->csh);
	SYS_LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
	SYS_LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
	SYS_LOG_INF("    CMU_DACCLK: %08x", sys_read32(CMU_DACCLK));
	SYS_LOG_INF("  CMU_I2STXCLK: %08x", sys_read32(CMU_I2STXCLK));
	SYS_LOG_INF("   CMU_SPDIFTX: %08x", sys_read32(CMU_SPDIFTXCLK));
#endif
}

/*
 * @brief  Config the SPDIFTX FIFO source
 * @param  outfifo_type: the output FIFO type selection
 * @return 0 on success, negative errno code on failure
 */
static int phy_spdiftx_fifo_config(audio_outfifo_sel_e outfifo_type)
{
	struct acts_audio_spdiftx *spdiftx_base = (struct acts_audio_spdiftx *)get_spdiftx_base();
	u32_t reg;

	reg = spdiftx_base->ctl & ~SPDIFT0_CTL_FIFO_SEL;

	if (AOUT_FIFO_DAC0 == outfifo_type) {
		reg |= SPDIFT0_CTL_FIFO_SEL;
		spdiftx_base->ctl = reg;
	} else if (AOUT_FIFO_I2STX0 == outfifo_type) {
		spdiftx_base->ctl = reg;
	} else {
		SYS_LOG_ERR("Unsupport FIFO type %d", outfifo_type);
		return -ENOTSUP;
	}

	spdiftx_base->ctl = reg;

	return 0;
}

/*
 * @brief Set the SPDIFTX channel status
 * @param sts: the setting of channel status
 * @return void
 */
static void phy_spdiftx_channel_status_set(audio_spdif_ch_status_t *status)
{
	struct acts_audio_spdiftx *spdiftx_base = (struct acts_audio_spdiftx *)get_spdiftx_base();

	if (status) {
		spdiftx_base->csl = status->csl;
		spdiftx_base->csh = (u32_t)status->csh;
	} else {
		spdiftx_base->csl = TEST_SPDTX_CSL;
		spdiftx_base->csh = TEST_SPDTX_CSH;
	}
}

/*
 * @brief Get the SPDIFTX channel status
 * @param sts: the setting of channel status
 * @return void
 */
static void phy_spdiftx_channel_status_get(audio_spdif_ch_status_t *status)
{
	struct acts_audio_spdiftx *spdiftx_base = (struct acts_audio_spdiftx *)get_spdiftx_base();

	if (status) {
		status->csl = spdiftx_base->csl;
		status->csh = (u16_t)spdiftx_base->csh;
	}
}

static int phy_spdiftx_enable(struct phy_audio_device *dev, void *param)
{
	struct acts_audio_spdiftx *spdiftx_base = (struct acts_audio_spdiftx *)get_spdiftx_base();
	struct spdiftx_private_data *priv = (struct spdiftx_private_data *)dev->private;
	aout_param_t *out_param = (aout_param_t *)param;
	bool is_linkmode;
	a_spdiftx_clksrc_e clksrc;
	int ret;
	u8_t sr;

	if (!out_param) {
		SYS_LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if ((out_param->sample_rate < SAMPLE_32KHZ)
		|| (out_param->sample_rate == SAMPLE_64KHZ)) {
		SYS_LOG_ERR("Invalid samplerate %d", out_param->sample_rate);
		return -EINVAL;
	}

	sr = out_param->sample_rate;

	if (out_param->channel_type & AUDIO_CHANNEL_DAC) {
		SYS_LOG_INF("Enable linkage with DAC and I2STX");
		clksrc = CLK_SRCTX_DAC_256FS_DIV2;
		is_linkmode = true;
	} else if (out_param->channel_type & AUDIO_CHANNEL_I2STX) {
		SYS_LOG_INF("Enable linkage with I2STX");
		/* SPDIFTX uses 128FS but I2STX uses 256FS */
		clksrc = CLK_SRCTX_I2STX_MCLK_DIV2,
		is_linkmode = true;
	} else {
		if (out_param->outfifo_type == AOUT_FIFO_DAC0) {
			if (out_param->reload_setting) {
				SYS_LOG_ERR("DAC FIFO does not support reload mode");
				return -EINVAL;
			}
			clksrc = CLK_SRCTX_DAC_256FS_DIV2;
			/* SPDIF clock is div of DAC clock and has to compensate the sample rate */
			sr *= 2;
			priv->fifo_use = AOUT_FIFO_DAC0;
		} else {
#ifdef CONFIG_SPDIFTX_USE_I2STX_MCLK_DIV2
			clksrc = CLK_SRCTX_I2STX_MCLK_DIV2;
#else
			clksrc = CLK_SRCTX_I2STX_MCLK;
#endif
			priv->fifo_use = AOUT_FIFO_I2STX0;
		}

#ifdef CONFIG_SPDIFTX_USE_ADC_DIV2_CLK
		clksrc = CLK_SRCTX_ADC_256FS_DIV2;
#endif
		is_linkmode = false;
	}

	priv->clksrc = clksrc;
	priv->link_with_i2stx = is_linkmode;

	/* Set the SPDIFTX pins state */
	acts_pinmux_setup_pins(board_pin_spdiftx_config,
		ARRAY_SIZE(board_pin_spdiftx_config));

	acts_clock_peripheral_enable(CLOCK_ID_SPDIF_TX);

	if (priv->fifo_use == AOUT_FIFO_I2STX0)
		acts_clock_peripheral_enable(CLOCK_ID_I2S0_TX);

	if (priv->fifo_use == AOUT_FIFO_DAC0)
		acts_clock_peripheral_enable(CLOCK_ID_AUDIO_DAC);

	ret = audio_samplerate_spdiftx_set(sr, clksrc,
			is_linkmode ? true : false);
	if (ret) {
		SYS_LOG_ERR("Failed to spdiftx samplerate err=%d", ret);
		return ret;
	}

	ret = phy_spdiftx_fifo_config(out_param->outfifo_type);
	if (ret) {
		SYS_LOG_ERR("Failed to config spidftx fifo %d", out_param->outfifo_type);
		return ret;
	}

	phy_spdiftx_channel_status_set(out_param->spdiftx_setting->status);

	if (!is_linkmode)
		spdiftx_base->ctl |= SPDTX_CTL_SPDEN;

	return 0;
}

static int phy_spdiftx_disable(struct phy_audio_device *dev)
{
	int i;
	struct acts_audio_spdiftx *spdiftx_base = (struct acts_audio_spdiftx *)get_spdiftx_base();
	struct spdiftx_private_data *priv = (struct spdiftx_private_data *)dev->private;

	/* Set the spdiftx pins to be GPIO state for the power consume */
	for (i = 0; i < ARRAY_SIZE(board_pin_spdiftx_config); i++)
		acts_pinmux_set(board_pin_spdiftx_config[i].pin_num, 0);

	spdiftx_base->ctl &= (~SPDTX_CTL_SPDEN);

	if (priv->fifo_use == AOUT_FIFO_I2STX0)
		acts_clock_peripheral_disable(CLOCK_ID_I2S0_TX);

	if (priv->fifo_use == AOUT_FIFO_DAC0)
		acts_clock_peripheral_disable(CLOCK_ID_AUDIO_DAC);

	/* spdif tx clock gating disable */
	acts_clock_peripheral_disable(CLOCK_ID_SPDIF_TX);

	priv->fifo_use = AUDIO_FIFO_INVALID_TYPE;

	return 0;
}

static int phy_spdiftx_ioctl(struct phy_audio_device *dev, u32_t cmd, void *param)
{
	int ret = 0;
	struct spdiftx_private_data *priv = (struct spdiftx_private_data *)dev->private;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		spdiftx_dump_regs();
		break;
	}
	case AOUT_CMD_GET_SAMPLERATE:
	{
		ret = audio_samplerate_spdiftx_get();
		if (ret) {
			SYS_LOG_ERR("Failed to get SPDIFTX sample rate err=%d", ret);
			return ret;
		}
		*(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
		break;
	}
	case AOUT_CMD_SET_SAMPLERATE:
	{
		audio_sr_sel_e val = *(audio_sr_sel_e *)param;
		ret = audio_samplerate_spdiftx_set(val, priv->clksrc, priv->link_with_i2stx ? true: false);
		if (ret) {
			SYS_LOG_ERR("Failed to set I2STX sample rate err=%d", ret);
			return ret;
		}
		break;
	}
	case AOUT_CMD_GET_APS:
	{
		ret = audio_pll_get_spdiftx_aps();
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
		ret = audio_pll_set_spdiftx_aps(level);
		if (ret) {
			SYS_LOG_ERR("Failed to set audio pll APS err=%d", ret);
			return ret;
		}
		break;
	}
	case AOUT_CMD_SPDIF_SET_CHANNEL_STATUS:
	{
		audio_spdif_ch_status_t *status = (audio_spdif_ch_status_t *)param;
		if (!status) {
			SYS_LOG_ERR("Invalid parameters");
			return -EINVAL;
		}
		phy_spdiftx_channel_status_set(status);
		break;
	}
	case AOUT_CMD_SPDIF_GET_CHANNEL_STATUS:
	{
		phy_spdiftx_channel_status_get((audio_spdif_ch_status_t *)param);
		break;
	}
	default:
		SYS_LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;
	}

	return ret;
}

static int phy_spdiftx_init(struct phy_audio_device *dev, struct device *parent_dev)
{
	struct spdiftx_private_data *priv = (struct spdiftx_private_data *)dev->private;
	memset(priv, 0, sizeof(struct spdiftx_private_data));

	priv->parent_dev = parent_dev;
	priv->fifo_use = AUDIO_FIFO_INVALID_TYPE;
	SYS_LOG_INF("PHY SPDIFTX probe");

	return 0;
}

static struct spdiftx_private_data spdiftx_priv;

const struct phy_audio_operations spdiftx_aops = {
	.audio_init = phy_spdiftx_init,
	.audio_enable = phy_spdiftx_enable,
	.audio_disable = phy_spdiftx_disable,
	.audio_ioctl = phy_spdiftx_ioctl,
};

PHY_AUDIO_DEVICE(spdiftx, PHY_AUDIO_SPDIFTX_NAME, &spdiftx_aops, &spdiftx_priv);
PHY_AUDIO_DEVICE_FN(spdiftx);
