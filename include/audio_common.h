/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief The common audio structures and definitions used within the audio
 */

#ifndef __AUDIO_COMMON_H__
#define __AUDIO_COMMON_H__

/* @brief Definition for FIFO counter */
#define AOUT_FIFO_CNT_MAX               (0xFFFF) /* The max value of the audio out FIFO counter */

/* @brief Definition the invalid type of audio FIFO */
#define AUDIO_FIFO_INVALID_TYPE         (0xFF)

/**
 * @brief Definition for the audio channel type
 * @note If there is a multi-linkage reguirement, it allows using "or" channels.
 *       And it is only allow the linkage usage of the output channels.
 * @par example:
 * (AUDIO_CHANNEL_DAC | AUDIO_CHANNEL_I2STX) stands for DAC linkage with I2S.
 */
#define AUDIO_CHANNEL_DAC               (1 << 0)
#define AUDIO_CHANNEL_I2STX             (1 << 1)
#define AUDIO_CHANNEL_SPDIFTX           (1 << 2)
#define AUDIO_CHANNEL_ADC               (1 << 3)
#define AUDIO_CHANNEL_I2SRX             (1 << 4)
#define AUDIO_CHANNEL_SPDIFRX           (1 << 5)

/**
 * @brief Definition for the audio channel status
 */
#define AUDIO_CHANNEL_STATUS_BUSY       (1 << 0) /* channel FIFO still busy status */
#define AUDIO_CHANNEL_STATUS_ERROR      (1 << 1) /* channel error status */

/**
 * enum audio_sr_sel_e
 * @brief The sample rate choices
 */
typedef enum {
	SAMPLE_RATE_8KHZ   = 8,
	SAMPLE_RATE_11KHZ  = 11,   /* 11.025KHz */
	SAMPLE_RATE_12KHZ  = 12,
	SAMPLE_RATE_16KHZ  = 16,
	SAMPLE_RATE_22KHZ  = 22,   /* 22.05KHz */
	SAMPLE_RATE_24KHZ  = 24,
	SAMPLE_RATE_32KHZ  = 32,
	SAMPLE_RATE_44KHZ  = 44,   /* 44.1KHz */
	SAMPLE_RATE_48KHZ  = 48,
	SAMPLE_RATE_64KHZ  = 64,
	SAMPLE_RATE_88KHZ  = 88,   /* 88.2KHz */
	SAMPLE_RATE_96KHZ  = 96,
	SAMPLE_RATE_176KHZ = 176,  /* 176.4KHz */
	SAMPLE_RATE_192KHZ = 192,
} audio_sr_sel_e;

/*
 * enum audio_aps_level_e
 * @brief The APS for the AUDIO_PLL tuning dynamically
 */
typedef enum {
	APS_LEVEL_1 = 0, /* 44.0006 / 47.8795 */
	APS_LEVEL_2, /* 44.0245 / 47.9167 */
	APS_LEVEL_3, /* 44.0625 / 47.9651 */
	APS_LEVEL_4, /* 44.0848 / 47.9805 */
	APS_LEVEL_5, /* 44.0995 / 48.0018 */
	APS_LEVEL_6, /* 44.1176 / 48.0239 */
	APS_LEVEL_7, /* 44.1323 / 48.0469 */
	APS_LEVEL_8, /* 44.1964 / 48.1086 */
} audio_aps_level_e;

/*
 * enum audio_ch_mode_e
 * @brief Define the channel mode
 */
typedef enum {
	MONO_MODE = 1,
	STEREO_MODE,
} audio_ch_mode_e;

/*
 * enum audio_outfifo_sel_e
 * @brief Audio out fifo selection
 */
typedef enum {
	AOUT_FIFO_DAC0 = 0, /* DAC FIFO */
	AOUT_FIFO_I2STX0, /* I2STX FIFO */
} audio_outfifo_sel_e;

/*
 * enum a_infifo_sel_e
 * @brief Audio in fifo selection
 */
typedef enum {
	AIN_FIFO_ADC0 = 0, /* ADC FIFO */
	AIN_FIFO_I2SRX0, /* I2SRX FIFO */
	AIN_FIFO_SPIDFRX0, /* SPIDFRX FIFO */
} audio_infifo_sel_e;

/*
 * enum audio_i2s_fmt_e
 * @brief I2S transfer format selection
 */
typedef enum {
	I2S_FORMAT = 0,
	LEFT_JUSTIFIED_FORMAT,
	RIGHT_JUSTIFIED_FORMAT
} audio_i2s_fmt_e;

/*
 * enum audio_i2s_bclk_e
 * @brief Rate of BCLK with LRCLK
 */
typedef enum {
	I2S_BCLK_64FS = 0,
	I2S_BCLK_32FS
} audio_i2s_bclk_e;

/*
 * enum audio_i2s_mode_e
 * @brief I2S mode selection
 */
typedef enum {
	I2S_MASTER_MODE = 0,
	I2S_SLAVE_MODE
} audio_i2s_mode_e;

/*
 * enum audio_i2s_srd_wl_e
 * @brief I2S sample rate width length detect
 */
typedef enum {
	SRDSTA_WL_32RATE = 0, /* BCLK = 32LRCLK */
	SRDSTA_WL_64RATE /* BCLK = 64LRCLK */
} audio_i2s_srd_wl_e;

/*
 * enum audio_ch_width_e
 * @brief The effective data width of audio channel
 * @note DAC and SPDIF only support #CHANNEL_WIDTH_16BITS and #CHANNEL_WIDTH_24BITS
 *		I2S support #CHANNEL_WIDTH_16BITS, #CHANNEL_WIDTH_20BITS and #CHANNEL_WIDTH_24BITS
 *		285x ADC support #CHANNEL_WIDTH_16BITS and #CHANNEL_WIDTH_18BITS
 *		283x ADC support #CHANNEL_WIDTH_16BITS and #CHANNEL_WIDTH_24BITS
 */
typedef enum {
	CHANNEL_WIDTH_16BITS = 0,
	CHANNEL_WIDTH_18BITS,
	CHANNEL_WIDTH_20BITS,
	CHANNEL_WIDTH_24BITS
} audio_ch_width_e;

/*!
 * enum audio_lch_input_e
 * @brief The ADC left channel analog input selection
 */
typedef enum {
	INPUT_LCH_DISABLE = 0,  /*!< ADC left channel disable */
	INPUT0L_SINGLE_END,     /*!< ADC left channel connects with the input0 left endpoint */
	INPUT1L_SINGLE_END,     /*!< ADC left channel connects with the input1 left endpoint */
	INPUT2L_SINGLE_END,     /*!< ADC left channel connects with the input2 left endpoint */
	INPUT0LR_MIX,           /*!< ADC left channel connects with both the input0 left and right endpoints worked in mix mode */
	INPUT0LR_DIFF,          /*!< ADC left channel connects with both the input0 left and right endpoints worked in differential mode */
	INPUT2LR_MIX,           /*!< ADC left channel connects with both the input2 left and right endpoints worked in mix mode */
	INPUT2LR_DIFF,          /*!< ADC left channel connects with both the input2 left and right endpoints worked in differential mode */
	INPUT_LCH_DMIC          /*!< ADC left channel connects with the Digital MIC */
} audio_lch_input_e;

/*!
 * enum audio_rch_input_e
 * @brief The ADC right channel analog input selection
 */
typedef enum {
	INPUT_RCH_DISABLE = 0,  /*!< ADC left channel disable */
	INPUT0R_SINGLE_END,     /*!< ADC right channel connects with the input0 right endpoint */
	INPUT1R_SINGLE_END,     /*!< ADC right channel connects with the input1 right endpoint */
	INPUT2R_SINGLE_END,     /*!< ADC right channel connects with the input2 right endpoint */
	INPUT1LR_MIX,           /*!< ADC right channel connects with both the input1 left and right endpoints worked in mix mode */
	INPUT1LR_DIFF,          /*!< ADC right channel connects with both the input1 left and right endpoints worked in differential mode */
	INPUT_RCH_DMIC          /*!< ADC right channel connects with the Digital MIC */
} audio_rch_input_e;

/*!
 * enum audio_ext_pa_class_e
 * @brief The external PA class mode selection
 * @note CLASS_AB: higher power consume and lower radiation.
 * ClASS_D: lower power consume and higher radiation.
 */
typedef enum {
	EXT_PA_CLASS_AB = 0,
	EXT_PA_CLASS_D
} audio_ext_pa_class_e;

/*!
 * struct audio_spdif_ch_status_t
 * @brief The SPDIF channel status setting
 */
typedef struct {
	u32_t csl; /* The low 32bits of channel status */
	u16_t csh; /* The high 16bits of channel status */
} audio_spdif_ch_status_t;

/*!
 * struct audio_reload_t
 * @brief Audio out reload mode configuration.
 * @note If enable reload function the audio driver will transfer the same buffer address and notify the user when the buffer is half full or full.
 */
typedef struct {
	u8_t *reload_addr; /*!< Reload buffer address to transfer */
	u32_t reload_len;  /*!< The length of the reload buffer */
} audio_reload_t;

/*!
 * struct audio_input_map_t
 * @brief The mapping relationship between audio device and ADC L/R input.
 */
typedef struct {
	u8_t audio_dev;
	u8_t left_input;
	u8_t right_input;
} audio_input_map_t;

#endif /* __AUDIO_COMMON_H__ */
