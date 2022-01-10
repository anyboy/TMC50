/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common utils for in/out audio drivers
 */
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <audio_common.h>
#include "audio_acts_utils.h"

#define SYS_LOG_DOMAIN "AUTILS"
#include <logging/sys_log.h>

/*
 * @brief AUDIO PLL clock selection
 */

#define MAX_AUDIO_PLL              (14)

/* 48ksr serials */
#define PLL_49152               (49152000)
#define PLL_24576               (24576000)
#define PLL_16384               (16384000)
#define PLL_12288               (12288000)
#define PLL_8192                 (8192000)
#define PLL_6144                 (6144000)
#define PLL_4096                 (4096000)
#define PLL_3072                 (3072000)
#define PLL_2048                 (2048000)

/* 44.1ksr serials */
#define PLL_451584             (45158400)
#define PLL_225792             (22579200)
#define PLL_112896             (11289600)
#define PLL_56448               (5644800)
#define PLL_28224               (2822400)

/*
 * enum a_pll_type_e
 * @brief The audio pll type selection
 */
typedef enum {
	AUDIOPLL_TYPE_0 = 0, /* AUDIO_PLL0 */
	AUDIOPLL_TYPE_1, /* AUDIO_PLL1 */
} a_pll_type_e;

/*
 * struct audio_pll_t
 * @brief The structure includes the pll clock and its setting parameters
 */
typedef struct {
    u32_t pll_clk; /* audio pll clock */
    u8_t pre_div; /* clock pre-divisor */
    u8_t clk_div; /* clock divisor */
} audio_pll_t;

static const audio_pll_t audio_pll_selections[MAX_AUDIO_PLL] = {
    {PLL_49152, 0, 0}, /* 48ksr series */
    {PLL_24576, 0, 1}, /* 48ksr series */
    {PLL_16384, 0, 2}, /* 48ksr series */
    {PLL_12288, 0, 3}, /* 48ksr series */
    {PLL_8192, 0, 4}, /* 48ksr series */
    {PLL_6144, 0, 5}, /* 48ksr series */
    {PLL_4096, 0, 6}, /* 48ksr series */
    {PLL_3072, 1, 5}, /* 48ksr series */
    {PLL_2048, 1, 6}, /* 48ksr series */
    {PLL_451584, 0, 0}, /* 44.1ksr series */
    {PLL_225792, 0, 1}, /* 44.1ksr series */
    {PLL_112896, 0, 3}, /* 44.1ksr series */
    {PLL_56448, 0, 5}, /* 44.1ksr series */
    {PLL_28224, 1, 5} /* 44.1ksr series */
};

/* @brief Translate the sample rate from KHz to Hz */
static u32_t audio_sr_khz_to_hz(audio_sr_sel_e sr_khz)
{
	u32_t ret_sample_hz;

    if (!(sr_khz % SAMPLE_RATE_11KHZ)) {
        /* 44.1KHz serials */
		ret_sample_hz = (sr_khz / SAMPLE_RATE_11KHZ) * 11025;
	} else {
	    /* 48KHz serials */
		ret_sample_hz = sr_khz * 1000;
	}

	return ret_sample_hz;
}

/* @brief Translate the sample rate from Hz to KHz */
static audio_sr_sel_e audio_sr_hz_to_Khz(u32_t sr_hz)
{
	u32_t sr_khz;
	if (!(sr_hz % 11025)) {
		sr_khz = (sr_hz / 11025) * SAMPLE_RATE_11KHZ;
	} else {
		sr_khz = sr_hz / 1000;
	}
	return (audio_sr_sel_e)sr_khz;
}

/* @brief Get the audio pll setting by specified sample rate and mclk*/
static int audio_get_pll_setting(audio_sr_sel_e sr_khz, a_mclk_type_e mclk,
	u8_t *pre_div, u8_t *clk_div, u8_t *series)
{
    u32_t sr_hz;
    int i;
    sr_hz = audio_sr_khz_to_hz(sr_khz);
    /* calculate the constant clock */
    sr_hz *= mclk;

    for (i = 0; i < MAX_AUDIO_PLL; i++) {
        if (sr_hz == audio_pll_selections[i].pll_clk) {
            *pre_div = audio_pll_selections[i].pre_div;
            *clk_div = audio_pll_selections[i].clk_div;
            if (i > 8)
            	*series = AUDIOPLL_44KSR;
            else
            	*series = AUDIOPLL_48KSR;
            break;
        }
    }

    /* Can not find the corresponding PLL setting */
    if (i == MAX_AUDIO_PLL) {
        SYS_LOG_ERR("Failed to find audio pll setting sr:%d mclk:%d", sr_khz, mclk);
        *pre_div = 0xFF;
        *pre_div = 0xFF;
        *series = 0xFF;
        return -ENOEXEC;
    }

    return 0;
}

/* @brief Get the audio pll usage info by the pll index */
static int audio_pll_get_usage(a_pll_type_e index, u8_t *series)
{
	/* AUDIO_PLL0 */
	if (AUDIOPLL_TYPE_0 == index) {
		/* check AUDIO_PLL0 enable or not */
		if ((sys_read32(AUDIO_PLL0_CTL) & (1 << CMU_AUDIOPLL0_CTL_EN)) == 0) {
			/* AUDIO_PLL0 disable */
			*series = 0xFF;
		} else if((sys_read32(AUDIO_PLL0_CTL) & CMU_AUDIOPLL0_CTL_APS0_MASK) >= 8) {
			/* AUDIO_PLL0 is 48k series */
			*series = AUDIOPLL_48KSR;
		} else {
			/* AUDIO_PLL0 is 44.1k series */
			*series = AUDIOPLL_44KSR;
		}
	} else if (AUDIOPLL_TYPE_1 == index) {
		/* AUDIO_PLL1 */
		if((sys_read32(AUDIO_PLL1_CTL) & (1 << CMU_AUDIOPLL1_CTL_EN)) == 0) {
			/* AUDIO_PLL1 disable */
			*series = 0xFF;
		} else if ((sys_read32(AUDIO_PLL1_CTL) & CMU_AUDIOPLL1_CTL_APS1_MASK) >= 8) {
			/* AUDIO_PLL1 is 48k series */
			*series = AUDIOPLL_48KSR;
		} else {
			/* AUDIO_PLL1 is 44.1k series */
			*series = AUDIOPLL_44KSR;
		}
	} else {
		SYS_LOG_ERR("Invalid AUDIO_PLL type %d", index);
		return -EINVAL;
	}

	SYS_LOG_INF("Get audio pll%d series 0x%x", index, *series);

	return 0;
}

/* @brief Get the audio pll aps by specified pll index */
static int audio_pll_get_aps(a_pll_type_e index)
{
	u32_t reg = -1;

	if (AUDIOPLL_TYPE_0 == index) {
		if ((sys_read32(AUDIO_PLL0_CTL) & (1 << CMU_AUDIOPLL0_CTL_EN)) == 0) {
			SYS_LOG_ERR("AUDIO_PLL0 is not enable yet");
			return -EPERM;
		}
		reg = sys_read32(AUDIO_PLL0_CTL) & CMU_AUDIOPLL0_CTL_APS0_MASK;
		if (reg >= 8) {
			/* 48KHz sample rate seires */
			reg -= 8;
		}
	} else if (AUDIOPLL_TYPE_1 == index) {
		if ((sys_read32(AUDIO_PLL1_CTL) & (1 << CMU_AUDIOPLL1_CTL_EN)) == 0) {
			SYS_LOG_ERR("AUDIO_PLL1 is not enable yet");
			return -EPERM;
		}
		reg = sys_read32(AUDIO_PLL1_CTL) & CMU_AUDIOPLL1_CTL_APS1_MASK;
		if (reg >= 8) {
			/* 48KHz sample rate seires */
			reg -= 8;
		}
	} else {
		SYS_LOG_ERR("Invalid AUDIO_PLL type %d", index);
		return -EINVAL;
	}

	return reg;
}

/* @brief Set the audio pll aps by specified pll index */
static int audio_pll_set_aps(a_pll_type_e index, audio_aps_level_e level)
{
	u32_t reg;

	if (level > APS_LEVEL_8) {
		SYS_LOG_ERR("Invalid APS level setting %d", level);
		return -EINVAL;
	}

	if (AUDIOPLL_TYPE_0 == index) {
		if ((sys_read32(AUDIO_PLL0_CTL) & (1 << CMU_AUDIOPLL0_CTL_EN)) == 0) {
			SYS_LOG_INF("AUDIO_PLL0 is not enable");
			return -EPERM;
		}
		reg = sys_read32(AUDIO_PLL0_CTL);
		if ((reg  & CMU_AUDIOPLL0_CTL_APS0_MASK) >= 8) /* 48KHz sample rate seires */
			level += 8;
		reg &= ~CMU_AUDIOPLL0_CTL_APS0_MASK;
		sys_write32(reg | CMU_AUDIOPLL0_CTL_APS0(level), AUDIO_PLL0_CTL);

	} else if (AUDIOPLL_TYPE_1 == index) {
		if ((sys_read32(AUDIO_PLL1_CTL) & (1 << CMU_AUDIOPLL1_CTL_EN)) == 0) {
			SYS_LOG_ERR("AUDIO_PLL1 is not enable yet");
			return -EPERM;
		}
		reg = sys_read32(AUDIO_PLL1_CTL);
		if ((reg & CMU_AUDIOPLL1_CTL_APS1_MASK) >= 8) /* 48KHz sample rate seires */
			level += 8;
		reg &= ~CMU_AUDIOPLL1_CTL_APS1_MASK;
		sys_write32(reg | CMU_AUDIOPLL1_CTL_APS1(level), AUDIO_PLL1_CTL);
	} else {
		SYS_LOG_ERR("Invalid AUDIO_PLL type %d", index);
		return -EINVAL;
	}

	return 0;
}

/* @brief Get the pll clock in HZ for the sample rate translation */
static int audio_get_pll_sample_rate(a_mclk_type_e mclk, u8_t pre_div, u8_t clk_div, a_pll_type_e index)
{
	u8_t series, start, end;
	int ret, i;

	ret = audio_pll_get_usage(index, &series);
	if (ret)
		return ret;

	if (AUDIOPLL_48KSR == series) {
		start = 0;
		end = 9;
	} else if (AUDIOPLL_44KSR == series) {
		start = 9;
		end = MAX_AUDIO_PLL;
	} else {
		SYS_LOG_ERR("Error series %d", series);
		return -EPERM;
	}

	for (i = start; i < end; i++) {
		if ((audio_pll_selections[i].pre_div == pre_div)
			&& (audio_pll_selections[i].clk_div) == clk_div) {
			ret = audio_pll_selections[i].pll_clk / mclk;
			ret = (int)audio_sr_hz_to_Khz(ret);
			break;
		}
	}

	if (i == end) {
		SYS_LOG_ERR("Failed to translate sr pre_div:%d clk_div%d index:%d",
					pre_div, clk_div, index);
		ret = -EFAULT;
	}

	return ret;
}

/* @brief audio pll set */
static void audio_pll_set(a_pll_type_e index, a_pll_series_e series)
{
	u32_t reg;
	if (AUDIOPLL_TYPE_0 == index) {
		/* Enable AUDIO_PLL0 */
		reg = sys_read32(AUDIO_PLL0_CTL) & (~CMU_AUDIOPLL0_CTL_APS0_MASK);
		reg |= (1 << CMU_AUDIOPLL0_CTL_EN);

		if (AUDIOPLL_44KSR == series)
			reg |= (0x04 << CMU_AUDIOPLL0_CTL_APS0_SHIFT);
		else
			reg |= (0x0c << CMU_AUDIOPLL0_CTL_APS0_SHIFT);

		sys_write32(reg, AUDIO_PLL0_CTL);

		SYS_LOG_INF("AUDIO_PLL0_CTL - 0x%x", sys_read32(AUDIO_PLL0_CTL));
	} else if (AUDIOPLL_TYPE_1 == index) {
		/* Enable AUDIO_PLL1 */
		reg = sys_read32(AUDIO_PLL1_CTL) & (~CMU_AUDIOPLL1_CTL_APS1_MASK);
		reg |= (1 << CMU_AUDIOPLL1_CTL_EN);

		if (AUDIOPLL_44KSR == series)
			reg |= (0x04 << CMU_AUDIOPLL1_CTL_APS1_SHIFT);
		else
			reg |= (0x0c << CMU_AUDIOPLL1_CTL_APS1_SHIFT);

		sys_write32(reg, AUDIO_PLL1_CTL);

		SYS_LOG_INF("AUDIO_PLL1_CTL - 0x%x", sys_read32(AUDIO_PLL1_CTL));
	}
}

/* @brief Check and config the audio pll */
int audio_pll_check_config(u8_t series, u8_t *index)
{
	int ret;
	u8_t get_series, pll_index = 0xFF;
	ret = audio_pll_get_usage(AUDIOPLL_TYPE_0, &get_series);
	if (ret) {
		SYS_LOG_ERR("Get AUDIO_PLL0 error %d", ret);
		return ret;
	}

	/* Keep the same series within the pll setting */
	if ((0xFF == get_series) || (series == (a_pll_series_e)get_series)) {
		pll_index = AUDIOPLL_TYPE_0;
	} else {
		ret = audio_pll_get_usage(AUDIOPLL_TYPE_1, &get_series);
		if (ret) {
			SYS_LOG_ERR("Get AUDIO_PLL1 error %d", ret);
			return ret;
		}
		if ((0xFF == get_series) || (series == (a_pll_series_e)get_series))
			pll_index = AUDIOPLL_TYPE_1;
	}

	if (0xFF == pll_index) {
		SYS_LOG_ERR("Failed to find the available pll %d", series);
		SYS_LOG_INF("AUDIO_PLL0: 0x%x", sys_read32(AUDIO_PLL0_CTL));
		SYS_LOG_INF("AUDIO_PLL1: 0x%x", sys_read32(AUDIO_PLL1_CTL));
		*index = 0xFF;
		return -ENOENT;
	}

	*index = pll_index;
	audio_pll_set(pll_index, series);

	return 0;
}

/* @brief DAC sample rate config */
int audio_samplerate_dac_set(audio_sr_sel_e sr_khz)
{
    int ret;
    u8_t pre_div, clk_div, series, pll_index;
	u32_t reg;

    /* Get audio PLL setting  */
    ret = audio_get_pll_setting(sr_khz, MCLK_256FS, /* DAC clock source is fixed 256FS */
    		&pre_div, &clk_div, &series);
    if (ret) {
		SYS_LOG_DBG("get pll setting error:%d", ret);
        return ret;
	}

	/* Check the pll usage and then config */
	ret = audio_pll_check_config(series, &pll_index);
	if (ret) {
		SYS_LOG_DBG("check pll config error:%d", ret);
		return ret;
	}

	reg = sys_read32(CMU_DACCLK) & ~0x1F;

	/* Select pll0 or pll1 */
	reg |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC;

	reg |=  (pre_div << CMU_DACCLK_DACCLKPREDIV) | (clk_div << CMU_DACCLK_DACCLKDIV_SHIFT);

	sys_write32(reg, CMU_DACCLK);

	return 0;
}

/* @brief Get the sample rate from the DAC config */
int audio_samplerate_dac_get(void)
{
	u8_t pre_div, clk_div, pll_index;
	u32_t reg;

	reg = sys_read32(CMU_DACCLK);

	pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC)) >> CMU_DACCLK_DACCLKSRC;
	pre_div = (reg & (1 << CMU_DACCLK_DACCLKPREDIV)) >> CMU_DACCLK_DACCLKPREDIV;
	clk_div = reg & CMU_DACCLK_DACCLKDIV_MASK;

	return audio_get_pll_sample_rate(MCLK_256FS, pre_div, clk_div, pll_index);
}

/* @brief Get the AUDIO_PLL APS used by DAC */
int audio_pll_get_dac_aps(void)
{
	u32_t reg;
	u8_t pll_index;
	reg = sys_read32(CMU_DACCLK);
	pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC)) >> CMU_DACCLK_DACCLKSRC;

	return audio_pll_get_aps((a_pll_type_e)pll_index);
}

/* @brief Set the AUDIO_PLL APS used by DAC */
int audio_pll_set_dac_aps(audio_aps_level_e level)
{
	u32_t reg;
	u8_t pll_index;

	reg = sys_read32(CMU_DACCLK);
	pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC)) >> CMU_DACCLK_DACCLKSRC;

	return audio_pll_set_aps((a_pll_type_e)pll_index, level);
}

/* @brief ADC sample rate config */
int audio_samplerate_adc_set(audio_sr_sel_e sr_khz)
{
    int ret;
    u8_t pre_div, clk_div, series, pll_index;
	u32_t reg;

    /* Get audio PLL setting  */
    ret = audio_get_pll_setting(sr_khz, MCLK_256FS, /* ADC clock source is fixed 256FS */
    		&pre_div, &clk_div, &series);
    if (ret) {
		SYS_LOG_DBG("get pll setting error:%d", ret);
        return ret;
	}

	/* Check the pll usage and then config */
	ret = audio_pll_check_config(series, &pll_index);
	if (ret) {
		SYS_LOG_DBG("check pll config error:%d", ret);
		return ret;
	}

	reg = sys_read32(CMU_ADCCLK) & ~0x3F;

	/* Select pll0 or pll1 */
	reg |= (pll_index & 0x1) << CMU_ADCCLK_ADCCLKSRC_SHIFT;

	reg |=  (pre_div << CMU_ADCCLK_ADCCLKPREDIV) | (clk_div << CMU_ADCCLK_ADCCLKDIV_SHIFT);

	sys_write32(reg, CMU_ADCCLK);

	return 0;

}

/* @brief Get the sample rate from the ADC config */
int audio_samplerate_adc_get(void)
{
	u8_t pre_div, clk_div, pll_index;
	u32_t reg;

	reg = sys_read32(CMU_ADCCLK);

	pll_index = (reg & (3 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;
	pre_div = (reg & (1 << CMU_ADCCLK_ADCCLKPREDIV)) >> CMU_ADCCLK_ADCCLKPREDIV;
	clk_div = reg & CMU_ADCCLK_ADCCLKDIV_MASK;

	return audio_get_pll_sample_rate(MCLK_256FS, pre_div, clk_div, pll_index);
}

/* @brief Get the AUDIO_PLL APS used by ADC */
int audio_pll_get_adc_aps(void)
{
	u32_t reg;
	u8_t pll_index;
	reg = sys_read32(CMU_ADCCLK);
	pll_index = (reg & (3 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;

	if (pll_index == 2) {
		SYS_LOG_ERR("ADC clock source use HOSC");
		return -EPERM;
	}

	return audio_pll_get_aps((a_pll_type_e)pll_index);
}

/* @brief Set the AUDIO_PLL APS used by ADC */
int audio_pll_set_adc_aps(audio_aps_level_e level)
{
	u32_t reg;
	u8_t pll_index;

	reg = sys_read32(CMU_ADCCLK);
	pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;

	if (pll_index == 2) {
		SYS_LOG_ERR("ADC clock source use HOSC");
		return -EPERM;
	}

	return audio_pll_set_aps((a_pll_type_e)pll_index, level);
}

/* @brief I2STX sample rate config */
int audio_samplerate_i2stx_set(audio_sr_sel_e sr_khz,
		a_mclk_type_e mclk, a_i2stx_mclk_clksrc_e mclk_src, bool direct_set)
{
    int ret;
    u8_t pre_div, clk_div, series, pll_index;
	u32_t reg, reg1;

	reg = sys_read32(CMU_I2STXCLK) & ~0xFFF;

	if (direct_set)
		goto out;

    /* Get audio PLL setting  */
    ret = audio_get_pll_setting(sr_khz, mclk, &pre_div, &clk_div, &series);
    if (ret) {
		SYS_LOG_DBG("get pll setting error:%d", ret);
        return ret;
	}

	/* Check the pll usage and then config */
	if (CLK_SRCTX_I2STX_EXT != mclk_src) {
		ret = audio_pll_check_config(series, &pll_index);
		if (ret) {
			SYS_LOG_DBG("check pll config error:%d", ret);
			return ret;
		}
	}

	if (CLK_SRCTX_DAC_256FS == mclk_src) {
		reg1 = sys_read32(CMU_DACCLK) & ~0x1F;
		reg1 |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC;
		reg1 |=  (pre_div << CMU_DACCLK_DACCLKPREDIV) | (clk_div << CMU_DACCLK_DACCLKDIV_SHIFT);
		sys_write32(reg1, CMU_DACCLK);
	} else if (CLK_SRCRX_I2STX_AUDIOPLL_DIV1_5 == mclk_src) {
		reg |= (pll_index & 0x1) << CMU_I2STXCLK_I2STXCLKSRC;
		reg |= 1 << CMU_I2STXCLK_I2STXMCLKSRCH;
	} else if (CLK_SRCTX_ADC_256FS == mclk_src) {
		reg1 = sys_read32(CMU_ADCCLK) & ~0x3F;
		reg1 |= (pll_index & 0x1) << CMU_ADCCLK_ADCCLKSRC_SHIFT;
		reg1 |=  (pre_div << CMU_ADCCLK_ADCCLKPREDIV) | (clk_div << CMU_ADCCLK_ADCCLKDIV_SHIFT);
		sys_write32(reg1, CMU_ADCCLK);
	} else if (CLK_SRCTX_I2STX == mclk_src) {
		reg |= (pll_index & 0x1) << CMU_I2STXCLK_I2STXCLKSRC;
		reg |= (pre_div << CMU_I2STXCLK_I2STXCLKPREDIV) | (clk_div << CMU_I2STXCLK_I2STXCLKDIV_SHIFT);
	} else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
		SYS_LOG_INF("I2STX clock source from extern oscillator");
	} else {
		SYS_LOG_ERR("Invalid i2stx clk source %d", mclk_src);
		return -EINVAL;
	}

out:
#ifdef CONFIG_I2STX_MCLKSRC_EXT_REVERSE
	reg |= (1 << CMU_I2STXCLK_I2STXMCLKEXTREV);
#endif

	/* Select the i2stx mclk source */
	reg |= (mclk_src & 0x3) << CMU_I2STXCLK_I2STXMCLKSRC_SHIFT;
	sys_write32(reg, CMU_I2STXCLK);

	return 0;
}

/* @brief Get the sample rate from the I2STX config */
int audio_samplerate_i2stx_get(a_mclk_type_e mclk)
{
	u8_t pre_div, clk_div, pll_index, mclk_src;
	u32_t reg;
	int ret = -1;

	reg = sys_read32(CMU_I2STXCLK);

	mclk_src = (reg & (3 << CMU_I2STXCLK_I2STXMCLKSRC_SHIFT)) >> CMU_I2STXCLK_I2STXMCLKSRC_SHIFT;

	if (CLK_SRCTX_DAC_256FS == mclk_src) {
		ret = audio_samplerate_dac_get();
	} else if (CLK_SRCTX_ADC_256FS == mclk_src) {
		ret = audio_samplerate_adc_get();
	} else if (CLK_SRCTX_I2STX == mclk_src) {
		pll_index = (reg & (1 << CMU_I2STXCLK_I2STXCLKSRC)) >> CMU_I2STXCLK_I2STXCLKSRC;
		pre_div = (reg & (1 << CMU_I2STXCLK_I2STXCLKPREDIV)) >> CMU_I2STXCLK_I2STXCLKPREDIV;
		clk_div = reg & CMU_I2STXCLK_I2STXCLKDIV_MASK;
		ret = audio_get_pll_sample_rate(mclk, pre_div, clk_div, pll_index);
	} else if (CLK_SRCRX_I2STX_AUDIOPLL_DIV1_5 == mclk_src) {
		ret = 98304 * 2 / 3; /* AUDIO_PLL/1.5 */
	} else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
		SYS_LOG_INF("I2STX is using the extern clock source");
		ret = -ENOENT;
	}

	return ret;
}

/* @brief Get the AUDIO_PLL APS used by I2STX */
int audio_pll_get_i2stx_aps(void)
{
	u32_t reg;
	u8_t pll_index, mclk_src;
	int ret;

	reg = sys_read32(CMU_I2STXCLK);

	mclk_src = (reg & (3 << CMU_I2STXCLK_I2STXMCLKSRC_SHIFT)) >> CMU_I2STXCLK_I2STXMCLKSRC_SHIFT;

	if (CLK_SRCTX_DAC_256FS == mclk_src) {
		ret = audio_pll_get_dac_aps();
	} else if (CLK_SRCTX_ADC_256FS == mclk_src) {
		ret = audio_pll_get_adc_aps();
	} else if (CLK_SRCTX_I2STX == mclk_src) {
		pll_index = (reg & (1 << CMU_I2STXCLK_I2STXCLKSRC)) >> CMU_I2STXCLK_I2STXCLKSRC;
		ret = audio_pll_get_aps((a_pll_type_e)pll_index);
	} else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
		SYS_LOG_INF("I2STX is using the extern clock source");
		ret = -ENOENT;
	}

	return ret;
}

/* @brief Set the AUDIO_PLL APS used by I2STX */
int audio_pll_set_i2stx_aps(audio_aps_level_e level)
{
	u32_t reg;
	u8_t pll_index, mclk_src;
	int ret;

	reg = sys_read32(CMU_I2STXCLK);

	mclk_src = (reg & (3 << CMU_I2STXCLK_I2STXMCLKSRC_SHIFT)) >> CMU_I2STXCLK_I2STXMCLKSRC_SHIFT;

	if (CLK_SRCTX_DAC_256FS == mclk_src) {
		ret = audio_pll_set_dac_aps(level);
	} else if (CLK_SRCTX_ADC_256FS == mclk_src) {
		ret = audio_pll_set_adc_aps(level);
	} else if (CLK_SRCTX_I2STX == mclk_src) {
		pll_index = (reg & (1 << CMU_I2STXCLK_I2STXCLKSRC)) >> CMU_I2STXCLK_I2STXCLKSRC;
		ret = audio_pll_set_aps((a_pll_type_e)pll_index, level);
	} else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
		SYS_LOG_INF("I2STX is using the extern clock source");
		ret = -ENOENT;
	}

	return ret;
}

/* @brief I2SRX sample rate config */
int audio_samplerate_i2srx_set(audio_sr_sel_e sr_khz,
		a_mclk_type_e mclk, a_i2srx_mclk_clksrc_e mclk_src)
{
    int ret;
    u8_t pre_div, clk_div, series, pll_index;
	u32_t reg, reg1;

    /* Get audio PLL setting  */
    ret = audio_get_pll_setting(sr_khz, mclk, &pre_div, &clk_div, &series);
    if (ret) {
		SYS_LOG_DBG("get pll setting error:%d", ret);
        return ret;
	}

	/* Check the pll usage and then config */
	if (CLK_SRCRX_I2SRX_EXT != mclk_src) {
		ret = audio_pll_check_config(series, &pll_index);
		if (ret) {
			SYS_LOG_DBG("check pll config error:%d", ret);
			return ret;
		}
	}

	reg = sys_read32(CMU_I2SRXCLK) & ~0xFFF;

	if (CLK_SRCRX_I2SRX == mclk_src) {
		/* Select pll0 or pll1 */
		reg |= (pll_index & 0x1) << CMU_I2SRXCLK_I2SRXCLPREKSRC;
		reg |= (pre_div << CMU_I2SRXCLK_I2SRXCLKPREDIV) | (clk_div << CMU_I2SRXCLK_I2SRXCLKDIV_SHIFT);
	} else if (CLK_SRCRX_I2SRX_AUDIOPLL_DIV1_5 == mclk_src) {
		reg |= (pll_index & 0x1) << CMU_I2SRXCLK_I2SRXCLPREKSRC;
		reg |= 1 << CMU_I2SRXCLK_I2SRXMCLKSRCH;
	} else if (CLK_SRCRX_I2STX == mclk_src) {
		reg1 = sys_read32(CMU_I2STXCLK) & ~0xFFF;
		/* Select the i2stx mclk source */
		reg1 |= CLK_SRCTX_I2STX << CMU_I2STXCLK_I2STXMCLKSRC_SHIFT;
		/* Select pll0 or pll1 */
		reg1 |= (pll_index & 0x1) << CMU_I2STXCLK_I2STXCLKSRC;
		reg1 |= (pre_div << CMU_I2STXCLK_I2STXCLKPREDIV) | (clk_div << CMU_I2STXCLK_I2STXCLKDIV_SHIFT);
		sys_write32(reg1, CMU_I2STXCLK);
	} else if (CLK_SRCRX_ADC_256FS == mclk_src) {
		reg1 = sys_read32(CMU_DACCLK) & ~0x1F;
		/* Select pll0 or pll1 */
		reg1 |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC;
		reg1 |=  (pre_div << CMU_DACCLK_DACCLKPREDIV) | (clk_div << CMU_DACCLK_DACCLKDIV_SHIFT);
		sys_write32(reg1, CMU_DACCLK);
	} else if (CLK_SRCRX_I2SRX_EXT == mclk_src) {
		SYS_LOG_INF("I2SRX clock source from extern oscillator");
	} else {
		SYS_LOG_ERR("Invalid i2srx clk source %d", mclk_src);
		return -EINVAL;
	}

#ifdef CONFIG_I2SRX_MCLKSRC_EXT_REVERSE
	reg |= (1 << CMU_I2SRXCLK_I2SRXMCLKEXTREV);
#endif

	/* Select the i2stx mclk source */
	reg |= (mclk_src & 0x3) << CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT;

	sys_write32(reg, CMU_I2SRXCLK);

	return 0;
}

/* @brief Get the sample rate from the I2SRX config */
int audio_samplerate_i2srx_get(a_mclk_type_e mclk)
{
	u8_t pre_div, clk_div, pll_index, mclk_src;
	u32_t reg;
	int ret = -1;

	reg = sys_read32(CMU_I2SRXCLK);

	mclk_src = (reg & (3 << CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT)) >> CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT;

	if (CLK_SRCRX_I2SRX == mclk_src) {
		pll_index = (reg & (1 << CMU_I2SRXCLK_I2SRXCLPREKSRC)) >> CMU_I2SRXCLK_I2SRXCLPREKSRC;
		pre_div = (reg & (1 << CMU_I2SRXCLK_I2SRXCLKPREDIV)) >> CMU_I2SRXCLK_I2SRXCLKPREDIV;
		clk_div = reg & CMU_I2SRXCLK_I2SRXCLKDIV_MASK;
		ret = audio_get_pll_sample_rate(mclk, pre_div, clk_div, pll_index);
	} else if (CLK_SRCRX_I2STX == mclk_src) {
		ret = audio_samplerate_i2stx_get(MCLK_256FS);
	} else if (CLK_SRCRX_ADC_256FS == mclk_src) {
		ret = audio_samplerate_adc_get();
	} else if (CLK_SRCRX_I2SRX_AUDIOPLL_DIV1_5 == mclk_src) {
		ret = 98304 * 2 / 3; /* AUDIO_PLL/1.5 */
	} else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
		SYS_LOG_INF("I2SRX is using the extern clock source");
		ret = -ENOENT;
	}

	return ret;
}

/* @brief Get the AUDIO_PLL APS used by I2SRX */
int audio_pll_get_i2srx_aps(void)
{
	u32_t reg;
	u8_t pll_index, mclk_src;
	int ret;

	reg = sys_read32(CMU_I2SRXCLK);

	mclk_src = (reg & (3 << CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT)) >> CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT;

	if (CLK_SRCRX_I2SRX == mclk_src) {
		pll_index = (reg & (1 << CMU_I2SRXCLK_I2SRXCLPREKSRC)) >> CMU_I2SRXCLK_I2SRXCLPREKSRC;
		ret = audio_pll_get_aps((a_pll_type_e)pll_index);
	} else if (CLK_SRCRX_I2STX == mclk_src) {
		ret = audio_pll_get_i2stx_aps();
	} else if (CLK_SRCRX_ADC_256FS == mclk_src) {
		ret = audio_pll_get_adc_aps();
	} else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
		SYS_LOG_INF("I2SRX is using the extern clock source");
		ret = -ENOENT;
	}

	return ret;
}

/* @brief Set the AUDIO_PLL APS used by I2SRX */
int audio_pll_set_i2srx_aps(audio_aps_level_e level)
{
	u32_t reg;
	u8_t pll_index, mclk_src;
	int ret;

	reg = sys_read32(CMU_I2SRXCLK);

	mclk_src = (reg & (3 << CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT)) >> CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT;

	if (CLK_SRCRX_I2SRX == mclk_src) {
		pll_index = (reg & (1 << CMU_I2SRXCLK_I2SRXCLPREKSRC)) >> CMU_I2SRXCLK_I2SRXCLPREKSRC;
		ret = audio_pll_set_aps((a_pll_type_e)pll_index, level);
	} else if (CLK_SRCRX_I2STX == mclk_src) {
		ret = audio_pll_set_i2stx_aps(level);
	} else if (CLK_SRCRX_ADC_256FS == mclk_src) {
		ret = audio_pll_set_adc_aps(level);
	} else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
		SYS_LOG_INF("I2SRX is using the extern clock source");
		ret = -ENOENT;
	}

	return ret;
}

/* @brief SPDIFTX sample rate config */
int audio_samplerate_spdiftx_set(audio_sr_sel_e sr_khz, a_spdiftx_clksrc_e clksrc, bool direct_set)
{
    int ret;
    u8_t pre_div, clk_div, series, pll_index;
	u32_t reg, reg1;

	reg = sys_read32(CMU_SPDIFTXCLK) & ~CMU_SPDIFTXCLK_SPDIFTXCLKSRC_MASK;

	if (direct_set)
		goto out;

    /* Get audio PLL setting */
    ret = audio_get_pll_setting(sr_khz, MCLK_128FS, &pre_div, &clk_div, &series);
    if (ret) {
		SYS_LOG_DBG("get pll setting error:%d", ret);
        return ret;
	}

	/* Check the pll usage and then config */
	ret = audio_pll_check_config(series, &pll_index);
	if (ret) {
		SYS_LOG_DBG("check pll config error:%d", ret);
		return ret;
	}

	if (CLK_SRCTX_DAC_256FS_DIV2 == clksrc) {
		reg1 = sys_read32(CMU_DACCLK) & ~0x1F;
		/* Select pll0 or pll1 */
		reg1 |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC;
		reg1 |=  (pre_div << CMU_DACCLK_DACCLKPREDIV) | (clk_div << CMU_DACCLK_DACCLKDIV_SHIFT);
		sys_write32(reg1, CMU_DACCLK);
	} else if ((CLK_SRCTX_I2STX_MCLK == clksrc)
		|| (CLK_SRCTX_I2STX_MCLK_DIV2 == clksrc)) {
		reg1 = sys_read32(CMU_I2STXCLK) & ~0xFFF;
		/* Select the i2stx mclk source */
		reg1 |= CLK_SRCTX_I2STX << CMU_I2STXCLK_I2STXMCLKSRC_SHIFT;
		/* Select pll0 or pll1 */
		reg1 |= (pll_index & 0x1) << CMU_I2STXCLK_I2STXCLKSRC;
		reg1 |= (pre_div << CMU_I2STXCLK_I2STXCLKPREDIV) | (clk_div << CMU_I2STXCLK_I2STXCLKDIV_SHIFT);
		sys_write32(reg1, CMU_I2STXCLK);
	} else if (CLK_SRCTX_ADC_256FS_DIV2 == clksrc) {
		reg1 = sys_read32(CMU_ADCCLK) & ~0x3F;
		reg1 |= (pll_index & 0x1) << CMU_ADCCLK_ADCCLKSRC_SHIFT;
		reg1 |=  (pre_div << CMU_ADCCLK_ADCCLKPREDIV) | (clk_div << CMU_ADCCLK_ADCCLKDIV_SHIFT);
		sys_write32(reg1, CMU_ADCCLK);
	}else {
		SYS_LOG_ERR("Invalid spdiftx clk source %d", clksrc);
		return -EINVAL;
	}

out:
	sys_write32(reg | (clksrc << CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT), CMU_SPDIFTXCLK);

	return 0;
}

/* @brief Get the sample rate from the SPDIFTX config */
int audio_samplerate_spdiftx_get(void)
{
	u8_t mclk_src;
	u32_t reg;
	int ret = -1;

	reg = sys_read32(CMU_SPDIFTXCLK);

	mclk_src = (reg & (3 << CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT)) >> CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT;

	if (CLK_SRCTX_DAC_256FS_DIV2 == mclk_src) {
		ret = audio_samplerate_dac_get();
	} else if (CLK_SRCTX_I2STX_MCLK == mclk_src) {
		ret = audio_samplerate_i2stx_get(MCLK_128FS);
	}

	return ret;
}

/* @brief Get the AUDIO_PLL APS used by SPDIFTX */
int audio_pll_get_spdiftx_aps(void)
{
	u32_t reg;
	u8_t mclk_src;
	int ret = -1;

	reg = sys_read32(CMU_SPDIFTXCLK);

	mclk_src = (reg & (3 << CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT)) >> CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT;

	if (CLK_SRCTX_DAC_256FS_DIV2 == mclk_src) {
		ret = audio_pll_get_dac_aps();
	} else if (CLK_SRCTX_I2STX_MCLK == mclk_src) {
		ret = audio_pll_get_i2stx_aps();
	}

	return ret;
}

/* @brief Set the AUDIO_PLL APS used by SPDIFTX */
int audio_pll_set_spdiftx_aps(audio_aps_level_e level)
{
	u32_t reg;
	u8_t mclk_src;
	int ret = -1;

	reg = sys_read32(CMU_SPDIFTXCLK);

	mclk_src = (reg & (3 << CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT)) >> CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT;

	if (CLK_SRCTX_DAC_256FS_DIV2 == mclk_src) {
		ret = audio_pll_set_dac_aps(level);
	} else if (CLK_SRCTX_I2STX_MCLK == mclk_src) {
		ret = audio_pll_set_i2stx_aps(level);
	}

	return ret;
}

