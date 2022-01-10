/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common utils for in/out audio drivers
 */
#ifndef __AUDIO_ACTS_UTILS_H__
#define __AUDIO_ACTS_UTILS_H__

/*
 * enum a_mclk_type_e
 * @brief The rate of MCLK in the multiple of sample rate
 * @note DAC MCLK is always 256FS, and the I2S MCLK depends on BCLK (MCLK = 4BCLK)
 */
typedef enum {
    MCLK_128FS = 128,
    MCLK_256FS = 256,
    MCLK_512FS = 512
} a_mclk_type_e;

/*
 * enum a_spdiftx_clksrc_e
 * @brief The SPDIFTX clock source selection
 */
typedef enum {
	CLK_SRCTX_DAC_256FS_DIV2 = 0, /* DAC_256FS_CLK / 2 */
	CLK_SRCTX_I2STX_MCLK, /* I2STX_MCLK  */
	CLK_SRCTX_I2STX_MCLK_DIV2, /* I2STX_MCLK / 2 */
	CLK_SRCTX_ADC_256FS_DIV2 /* ADC_256FS_CLK / 4 */
} a_spdiftx_clksrc_e;

/*
 * enum a_i2s_mclk_clksrc_e
 * @brief The MCLK clock source of i2stx selection
 */
typedef enum {
	CLK_SRCTX_DAC_256FS = 0, /* I2STX clock source from DAC 256FS */
	CLK_SRCTX_ADC_256FS = 1, /* I2STX clock source from ADC 256FS */
	CLK_SRCTX_I2STX = 2, /* I2STX clock source from I2STX MCLK */
	CLK_SRCTX_I2STX_EXT = 3, /* I2STX clock source from I2STX extern MCLK */
	CLK_SRCRX_I2STX_AUDIOPLL_DIV1_5 = 4, /* I2STX clock source from AUDIOPLLx/1.5 */
} a_i2stx_mclk_clksrc_e;

/*
 * enum a_i2srx_mclk_clksrc_e
 * @brief The MCLK clock source of i2srx selection
 */
typedef enum {
	CLK_SRCRX_I2SRX = 0, /* I2SRX clock source from I2SRX clk */
	CLK_SRCRX_I2STX = 1, /* I2SRX clock source from I2STX MCLK */
	CLK_SRCRX_ADC_256FS = 2, /* I2SRX clock source from ADC 256FS */
	CLK_SRCRX_I2SRX_EXT = 3, /* I2SRX clock source from I2SRX extern MCLK */
	CLK_SRCRX_I2SRX_AUDIOPLL_DIV1_5 = 4, /* I2SRX clock source from AUDIOPLLx/1.5 */
} a_i2srx_mclk_clksrc_e;

/*
 * enum a_pll_series_e
 * @brief The series of audio pll
 */
typedef enum {
	AUDIOPLL_44KSR = 0, /* 44.1K sample rate seires */
	AUDIOPLL_48KSR /* 48K sample rate series */
} a_pll_series_e;

/* @brief DAC sample rate config */
int audio_samplerate_dac_set(audio_sr_sel_e sr_khz);
/* @brief Get the sample rate from the DAC config */
int audio_samplerate_dac_get(void);

/* @brief ADC sample rate config */
int audio_samplerate_adc_set(audio_sr_sel_e sr_khz);
/* @brief Get the sample rate from the ADC config */
int audio_samplerate_adc_get(void);

/* @brief I2STX sample rate config */
int audio_samplerate_i2stx_set(audio_sr_sel_e sr_khz,
		a_mclk_type_e mclk, a_i2stx_mclk_clksrc_e mclk_src, bool direct_set);
/* @brief Get the sample rate from the I2STX config */
int audio_samplerate_i2stx_get(a_mclk_type_e mclk);

/* @brief I2SRX sample rate config */
int audio_samplerate_i2srx_set(audio_sr_sel_e sr_khz,
		a_mclk_type_e mclk, a_i2srx_mclk_clksrc_e mclk_src);
/* @brief Get the sample rate from the I2SRX config */
int audio_samplerate_i2srx_get(a_mclk_type_e mclk);

/* @brief SPDIFTX sample rate config */
int audio_samplerate_spdiftx_set(audio_sr_sel_e sr_khz, a_spdiftx_clksrc_e clksrc, bool direct_set);
/* @brief Get the sample rate from the SPDIFTX config */
int audio_samplerate_spdiftx_get(void);

/* @brief Get the AUDIO_PLL APS used by DAC */
int audio_pll_get_dac_aps(void);
/* @brief Set the AUDIO_PLL APS used by DAC */
int audio_pll_set_dac_aps(audio_aps_level_e level);

/* @brief Get the AUDIO_PLL APS used by ADC */
int audio_pll_get_adc_aps(void);
/* @brief Set the AUDIO_PLL APS used by ADC */
int audio_pll_set_adc_aps(audio_aps_level_e level);

/* @brief Get the AUDIO_PLL APS used by I2STX */
int audio_pll_get_i2stx_aps(void);
/* @brief Set the AUDIO_PLL APS used by I2STX */
int audio_pll_set_i2stx_aps(audio_aps_level_e level);

/* @brief Get the AUDIO_PLL APS used by I2SRX */
int audio_pll_get_i2srx_aps(void);
/* @brief Set the AUDIO_PLL APS used by I2SRX */
int audio_pll_set_i2srx_aps(audio_aps_level_e level);

/* @brief Get the AUDIO_PLL APS used by SPDIFTX */
int audio_pll_get_spdiftx_aps(void);
/* @brief Set the AUDIO_PLL APS used by SPDIFTX */
int audio_pll_set_spdiftx_aps(audio_aps_level_e level);

/* @brief Check and config the audio pll */
int audio_pll_check_config(u8_t series, u8_t *index);

#endif /* __AUDIO_ACTS_UTILS_H__ */
