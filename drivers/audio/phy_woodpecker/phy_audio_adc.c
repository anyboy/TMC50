/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio ADC physical implementation
 */

/*
 * Features
 *    - 32 level * 18bits FIFO
 *    - Support single ended and full different input
 *    - Support 2 DMIC input
 *	- Support 3 pairs input and each pair can be formed as mix or different input
 *	_ Programmable HPF
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k
 */

/*
 * Signal List
 * 	- AVCC: Analog power
 *	- AGND: Analog ground
 *	- INPUT0L: Audio input for ADCL / Audio input for ADCL LP
 *	- INPUT0R: Audio input for ADCR / Audio input for ADCL LN
 *	- INPUT1L: Audio input for ADCL / Audio input for ADCR RN
 *	- INPUT1P: Audio input for ADCR / Audio input for ADCR RP
 *	- INPUT2L: Audio input for ADCL / Audio input for ADCL LP
 *	- INPUT2R: Audio input for ADCR / Audio input for ADCL LN
 *	- DMIC_CLK: DMIC clk output
 *	- DMIC_DATA: DMIC data input
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

#define SYS_LOG_DOMAIN "P_ADC"
#include <logging/sys_log.h>

/***************************************************************************************************
 * ADC_DIGCTL
 */
#define ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT                                    (8) /* Pre gain control for DMIC */
#define ADC_DIGCTL_DMIC_PRE_GAIN_MASK                                     (0x7 << ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT)
#define ADC_DIGCTL_DMIC_PRE_GAIN(x)                                       ((x) << ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT)

#define ADC_DIGCTL_DMIC_CHS                                               BIT(7) /* DMIC channel select, 0: L/R; 1:R/L */
#define ADC_DIGCTL_ADC_OVFS                                               BIT(6) /* ADC CIC over samples rate select */

#define ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT                                   (4) /* FIR(Programmable frequency response) mode select*/
#define ADC_DIGCTL_ADC_FIR_MD_SEL_MASK                                    (0x3 << ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT)
#define ADC_DIGCTL_ADC_FIR_MD_SEL(x)                                      ((x) << ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT)

#define ADC_DIGCTL_ADDEN                                                  BIT(3) /* ADC digital debug enable */
#define ADC_DIGCTL_AADEN                                                  BIT(2) /* ADC analog debug enable */
#define ADC_DIGCTL_ADCR_DIG_EN                                            BIT(1) /* ADCR digital enable */
#define ADC_DIGCTL_ADCL_DIG_EN                                            BIT(0) /* ADCL digital enable */

/***************************************************************************************************
 * LCH_DIGCTL
 */
#define LCH_DIGCTL_MIC_SEL                                                BIT(17) /* LCH data source select  */
#define LCH_DIGCTL_AAL_CH_EN                                              BIT(16) /* L channel AAL enable */
#define LCH_DIGCTL_DAT_OUT_EN                                             BIT(15) /* L channel FIFO timming slot enable */
#define LCH_DIGCTL_HPF_AS_TS_SHIFT                                        (13) /* HPF auto set time select */
#define LCH_DIGCTL_HPF_AS_TS_MASK                                         (0x3 << LCH_DIGCTL_HPF_AS_TS_SHIFT)
#define LCH_DIGCTL_HPF_AS_TS(x)                                           ((x) << LCH_DIGCTL_HPF_AS_TS_SHIFT)

#define LCH_DIGCTL_HPF_AS_EN                                              BIT(12) /* HPF auto set enable */
#define LCH_DIGCTL_HPFEN                                                  BIT(11) /* High pass filter L enable */
#define LCH_DIGCTL_HPF_S                                                  BIT(10) /* LCH HPF frequency fc0 & s0 select */

#define LCH_DIGCTL_HPF_N_SHIFT                                            (4) /* LCH HPF frequency fc select (0000 ~ 111111) */
#define LCH_DIGCTL_HPF_N_MASK                                             (0x3F << LCH_DIGCTL_HPF_N_SHIFT)
#define LCH_DIGCTL_HPF_N(x)                                               ((x) << LCH_DIGCTL_HPF_N_SHIFT)

#define LCH_DIGCTL_ADCGC_SHIFT                                            (0) /* ADC digital L gain control */
#define LCH_DIGCTL_ADCGC_MASK                                             (0xF << LCH_DIGCTL_ADCGC_SHIFT)
#define LCH_DIGCTL_ADCGC(x)                                               ((x) << LCH_DIGCTL_ADCGC_SHIFT)

/***************************************************************************************************
 * RCH_DIGCTL
 */
#define RCH_DIGCTL_MIC_SEL                                                BIT(17) /* RCH data source select */
#define RCH_DIGCTL_AAL_CH_EN                                              BIT(16) /* R channel AAL enable */
#define RCH_DIGCTL_DAT_OUT_EN                                             BIT(15) /* R channel FIFO timmint slot enable */

#define RCH_DIGCTL_HPF_AS_TS_SHIFT                                        (13) /* HPF auto set time select */
#define RCH_DIGCTL_HPF_AS_TS_MASK                                         (0x3 << RCH_DIGCTL_HPF_AS_TS_SHIFT)
#define RCH_DIGCTL_HPF_AS_TS(x)                                           ((x) << RCH_DIGCTL_HPF_AS_TS_SHIFT)

#define RCH_DIGCTL_HPF_AUTO_SET                                           BIT(12) /* HPF auto set enable */
#define RCH_DIGCTL_HPFEN                                                  BIT(11) /* High pass filter R enable */
#define RCH_DIGCTL_HPF_S                                                  BIT(10) /* RCH HPF frequency fc0 & s0 select */

#define RCH_DIGCTL_HPF_N_SHIFT                                            (4) /* RCH HPF frequency fc select */
#define RCH_DIGCTL_HPF_N_MASK                                             (0x3F << RCH_DIGCTL_HPF_N_SHIFT)
#define RCH_DIGCTL_HPF_N(x)                                               ((x) << RCH_DIGCTL_HPF_N_SHIFT)

#define RCH_DIGCTL_ADCGC_SHIFT                                            (0) /* ADC digital R gain control */
#define RCH_DIGCTL_ADCGC_MASK                                             (0xF << RCH_DIGCTL_ADCGC_SHIFT)
#define RCH_DIGCTL_ADCGC(x)                                               ((x) << RCH_DIGCTL_ADCGC_SHIFT)

/***************************************************************************************************
 * ADC_FIFOCTL
 */
#define ADC_FIFOCTL_ADCFIFO_DMAWIDTH                                      BIT(7) /* ADC fifo DMA transfer width */
#define ADC_FIFOCTL_ADFOS                                                 BIT(3) /* ADC fifo output select */
#define ADC_FIFOCTL_ADFFIE                                                BIT(2) /* half full irq enable */
#define ADC_FIFOCTL_ADFFDE                                                BIT(1) /* half full drq enable */
#define ADC_FIFOCTL_ADFRT                                                 BIT(0) /* ADC fifo reset */

/***************************************************************************************************
 * ADC_STAT
 */
#define ADC_STAT_FIFO_ER                                                  BIT(8) /* fifo error */
#define ADC_STAT_ADFEF                                                    BIT(7) /* ADC fifo empty flag */
#define ADC_STAT_ADFIP                                                    BIT(6) /* ADC fifo half full irq pending */
#define ADC_STAT_ADFS_SHIFT                                               (0) /* ADC fufo status */
#define ADC_STAT_ADFS_MASK                                                (0x3F << ADC_STAT_ADFS_SHIFT)

/***************************************************************************************************
 * ADC_DAT
 */
#define ADC_DAT_ADDAT_SHIFT                                               (14) /* ADC data (fifo: 18 bit x 2 x 16 level) */
#define ADC_DAT_ADDAT_MASK                                                (0x3FFFF << ADC_DAT_ADDAT_SHIFT)
#define ADC_DAT_ADDAT(x)                                                  ((x) << ADC_DAT_ADDAT_SHIFT)

/***************************************************************************************************
 * AAL_CTL
 */
#define AAL_CTL_AAL_EN                                                    BIT(17) /* automatic amplitude limiter enable */
#define AAL_CTL_AAL_RECOVER_MODE                                          BIT(16) /* AAL recover mode */

#define AAL_CTL_AAL_FL_SHIFT                                              (14) /* AAL frame length select */
#define AAL_CTL_AAL_FL_MASK                                               (0x3 << AAL_CTL_AAL_FL_SHIFT)

#define AAL_CTL_AAL_VT1_SHIFT                                             (12) /* AAL VT1 level select */
#define AAL_CTL_AAL_VT1_MASK                                              (0x3 << AAL_CTL_AAL_VT1_SHIFT)
#define AAL_CTL_AAL_VT1(x)                                                ((x) << AAL_CTL_AAL_VT1_SHIFT)

#define AAL_CTL_AAL_VT0_SHIFT                                             (8)
#define AAL_CTL_AAL_VT0_MASK                                              (0x7 << AAL_CTL_AAL_VT0_SHIFT)
#define AAL_CTL_AAL_VT0(x)                                                ((x) << AAL_CTL_AAL_VT0_SHIFT)

#define AAL_CTL_AAL_MAX_SHIFT                                             (4) /* The max gain adjust */
#define AAL_CTL_AAL_MAX_MASK                                              (0xF << AAL_CTL_AAL_MAX_SHIFT)
#define AAL_CTL_AAL_MAX(x)                                                ((x) << AAL_CTL_AAL_MAX_SHIFT)

#define AAL_CTL_AAL_CNT_SHIFT                                             (0)
#define AAL_CTL_AAL_CNT_MASK                                              (0xF << AAL_CTL_AAL_CNT_SHIFT)
#define AAL_CTL_AAL_CNT(x)                                                ((x) << AAL_CTL_AAL_CNT_SHIFT)

/***************************************************************************************************
 * ADC_INPUT_CTL
 */
#define ADC_INPUT_CTL_MIXFDSE                                             BIT(30) /* Mixout to adc input mode select */
#define ADC_INPUT_CTL_PAR2ADL_EN                                          BIT(29) /* PA OUTR mix to ADC L channel */
#define ADC_INPUT_CTL_PAL2ADL_EN                                          BIT(28) /* PA OUTL mix to ADC L channel */
#define ADC_INPUT_CTL_PAR2ADR_EN                                          BIT(27) /* PA OUTR mix to ADC R channel */
#define ADC_INPUT_CTL_PAL2ADR_EN                                          BIT(26) /* PA OUTL mix to ADC R channel */
#define ADC_INPUT_CTL_PAR_PD_EN                                           BIT(25) /* PA OUTR mix to ADC chananel pull down*/
#define ADC_INPUT_CTL_PAL_PD_EN                                           BIT(24) /* PA OUTL mix to ADC channel pull down */

#define ADC_INPUT_CTL_INPUT2L_EN_SHIFT                                    (22) /* INPUT2L to ADC L channel enable */
#define ADC_INPUT_CTL_INPUT2L_EN_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT2L_EN_SHIFT)
#define ADC_INPUT_CTL_INPUT2L_EN(x)                                       ((x) << ADC_INPUT_CTL_INPUT2L_EN_SHIFT)

#define ADC_INPUT_CTL_INPUT2R_EN_SHIFT                                    (20) /* INPUT2R to ADC R channel input enable */
#define ADC_INPUT_CTL_INPUT2R_EN_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT2R_EN_SHIFT)
#define ADC_INPUT_CTL_INPUT2R_EN(x)                                       ((x) << ADC_INPUT_CTL_INPUT2R_EN_SHIFT)

#define ADC_INPUT_CTL_INPUT2_IN_MODE                                      BIT(19) /* INPUT2 mode select */
#define ADC_INPUT_CTL_INPUT2LRMIX                                         BIT(18) /* INPUT2L and INPUT2R mix to ADCL enable */
#define ADC_INPUT_CTL_INPUT2LRMIX_SHIFT                                   (18)

#define ADC_INPUT_CTL_INPUT2_IRS_SHIFT                                    (16) /* INPUT2 input registor select */
#define ADC_INPUT_CTL_INPUT2_IRS_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT2_IRS_SHIFT)
#define ADC_INPUT_CTL_INPUT2_IRS(x)                                       ((x) << ADC_INPUT_CTL_INPUT2_IRS_SHIFT)

#define ADC_INPUT_CTL_INPUT1L_EN_SHIFT                                    (14) /* INPUT1L to ADC L channel input enable */
#define ADC_INPUT_CTL_INPUT1L_EN_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT1L_EN_SHIFT)
#define ADC_INPUT_CTL_INPUT1L_EN(x)                                       ((x) << ADC_INPUT_CTL_INPUT1L_EN_SHIFT)

#define ADC_INPUT_CTL_INPUT1R_EN_SHIFT                                    (12) /* INPUT1R to ADC R channel input enable */
#define ADC_INPUT_CTL_INPUT1R_EN_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT1R_EN_SHIFT)
#define ADC_INPUT_CTL_INPUT1R_EN(x)                                       ((x) << ADC_INPUT_CTL_INPUT1R_EN_SHIFT)

#define ADC_INPUT_CTL_INPUT1_IN_MODE                                      BIT(11) /* INPUT1 input mode select */
#define ADC_INPUT_CTL_INPUT1LRMIX                                         BIT(10) /* INPUT1L and input1R mix to ADC R enable */
#define ADC_INPUT_CTL_INPUT1LRMIX_SHIFT                                   (10)

#define ADC_INPUT_CTL_INPUT1_IRS_SHIFT                                    (8) /* INPUT1 input resistor select */
#define ADC_INPUT_CTL_INPUT1_IRS_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT1_IRS_SHIFT)
#define ADC_INPUT_CTL_INPUT1_IRS(x)                                       ((x) << ADC_INPUT_CTL_INPUT1_IRS_SHIFT)

#define ADC_INPUT_CTL_INPUT0L_EN_SHIFT                                    (6) /* INPUT0L to ADC L channel input enable */
#define ADC_INPUT_CTL_INPUT0L_EN_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT0L_EN_SHIFT)
#define ADC_INPUT_CTL_INPUT0L_EN(x)                                       ((x) << ADC_INPUT_CTL_INPUT0L_EN_SHIFT)

#define ADC_INPUT_CTL_INPUT0R_EN_SHIFT                                    (4) /* INPUT0R to ADC R channel input enable */
#define ADC_INPUT_CTL_INPUT0R_EN_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT0R_EN_SHIFT)
#define ADC_INPUT_CTL_INPUT0R_EN(x)                                       ((x) << ADC_INPUT_CTL_INPUT0R_EN_SHIFT)

#define ADC_INPUT_CTL_INPUT0_IN_MODE                                      BIT(3) /* INPUT0 input mode select */
#define ADC_INPUT_CTL_INPUT0LRMIX                                         BIT(2) /* INPUT0L and INPUT0R mix to ADCL enable */
#define ADC_INPUT_CTL_INPUT0LRMIX_SHIFT                                   (2)

#define ADC_INPUT_CTL_INPUT0_IRS_SHIFT                                    (0) /* INPUT0 input registor select */
#define ADC_INPUT_CTL_INPUT0_IRS_MASK                                     (0x3 << ADC_INPUT_CTL_INPUT0_IRS_SHIFT)
#define ADC_INPUT_CTL_INPUT0_IRS(x)                                       ((x) << ADC_INPUT_CTL_INPUT0_IRS_SHIFT)

/***************************************************************************************************
 * ADC_CTL
 */
#define ADC_CTL_BIASEN                                                    BIT(24) /* BIAS enable */
#define ADC_CTL_ADR_CAPFC_EN                                              BIT(23) /* input cap to ADC R channel fast charge enable */
#define ADC_CTL_ADL_CAPFC_EN                                              BIT(22) /* input cap to ADC L channel fast charge enable */
#define ADC_CTL_ADCL_BINV                                                 BIT(21) /* ADC L channel output bits invert, to make the phase right */
#define ADC_CTL_ADCR_BINV                                                 BIT(20) /* ADC R channel output bits invert, to make the phase right */

#define ADC_CTL_FDBUFL_IRS_E                                              (18)
#define ADC_CTL_FDBUFL_IRS_SHIFT                                          (16) /* FDBUFL input registor select */
#define ADC_CTL_FDBUFL_IRS_MASK                                           (0x7 << ADC_CTL_FDBUFL_IRS_SHIFT)
#define ADC_CTL_FDBUFL_IRS(x)                                             ((x) << ADC_CTL_FDBUFL_IRS_SHIFT)

#define ADC_CTL_FDBUFR_IRS_E                                              (14)
#define ADC_CTL_FDBUFR_IRS_SHIFT                                          (12) /* FDBUFR input registor select */
#define ADC_CTL_FDBUFR_IRS_MASK                                           (0x7 << ADC_CTL_FDBUFR_IRS_SHIFT)
#define ADC_CTL_FDBUFR_IRS(x)                                             ((x) << ADC_CTL_FDBUFR_IRS_SHIFT)

#define ADC_CTL_PREAM_PG_SHIFT                                            (8) /* PREAMP OP feedback res select to form different gain with different input res */
#define ADC_CTL_PREAM_PG_MASK                                             (0xF << ADC_CTL_PREAM_PG_SHIFT)
#define ADC_CTL_PREAM_PG(x)                                               ((x) << ADC_CTL_PREAM_PG_SHIFT)

#define ADC_CTL_FDBUFL_EN                                                 BIT(7) /* FD BUF OP L enable for SE mode */
#define ADC_CTL_FDBUFR_EN                                                 BIT(6) /* FD BUF OP R enable for SE mode */
#define ADC_CTL_PREOPL_EN                                                 BIT(5) /* PREOP L enable */
#define ADC_CTL_PREOPR_EN                                                 BIT(4) /* PREOP R enable */
#define ADC_CTL_VRDAL_EN                                                  BIT(3) /* VRDAL OP enable */
#define ADC_CTL_VRDAR_EN                                                  BIT(2) /* VRDAR OP enable */
#define ADC_CTL_ADCL_EN                                                   BIT(1) /* ADC L channel enable */
#define ADC_CTL_ADCR_EN                                                   BIT(0) /* ADC R channel enable */

/***************************************************************************************************
 * ADC_BIAS
 */
#define ADC_BIAS_BIASSEL_SHIFT                                            (24) /* All BIAS current select */
#define ADC_BIAS_BIASSEL_MASK                                             (0x3 << ADC_BIAS_BIASSEL_SHIFT)
#define ADC_BIAS_BIASSEL(x)                                               ((x) << ADC_BIAS_BIASSEL_SHIFT)

#define ADC_BIAS_VRDA_IQS_SHIFT                                           (21) /* VRDA OP output stage current control */
#define ADC_BIAS_VRDA_IQS_MASK                                            (0x3 << ADC_BIAS_VRDA_IQS_SHIFT)
#define ADC_BIAS_VRDA_IQS(x)                                              ((x) << ADC_BIAS_VRDA_IQS_SHIFT)

#define ADC_BIAS_PREOP_ODSC                                               BIT(20) /* PRE OP output driver dtrength control */

#define ADC_BIAS_PREOP_IQS_SHIFT                                          (18) /* PRE OP output stage current control */
#define ADC_BIAS_PREOP_IQS_MASK                                           (0x3 << ADC_BIAS_PREOP_IQS_SHIFT)
#define ADC_BIAS_PREOP_IQS(x)                                             ((x) << ADC_BIAS_PREOP_IQS_SHIFT)

#define ADC_BIAS_OPBUF_ODSC                                               BIT(17) /*  FDBUF OP output driver strength control */
#define ADC_BIAS_OPBUF_IQS                                                BIT(16) /* FD OP BUF output stage current control */

#define ADC_BIAS_IAD3_SHIFT                                               (14) /* sdm op3 bias control */
#define ADC_BIAS_IAD3_MASK                                                (0x3 << ADC_BIAS_IAD3_SHIFT)
#define ADC_BIAS_IAD3(x)                                                  ((x) << ADC_BIAS_IAD3_SHIFT)

#define ADC_BIAS_IAD2_SHIFT                                               (12) /* sdm op2 bias control */
#define ADC_BIAS_IAD2_MASK                                                (0x3 << ADC_BIAS_IAD2_SHIFT)
#define ADC_BIAS_IAD2(x)                                                  ((x) << ADC_BIAS_IAD2_SHIFT)

#define ADC_BIAS_IAD1_SHIFT                                               (8) /* sdm op1 bias control */
#define ADC_BIAS_IAD1_MASK                                                (0x7 << ADC_BIAS_IAD1_SHIFT)
#define ADC_BIAS_IAD1(x)                                                  ((x) << ADC_BIAS_IAD1_SHIFT)

#define ADC_BIAS_VRDA_IB_SHIFT                                            (6) /* VRDA op bias control */
#define ADC_BIAS_VRDA_IB_MASK                                             (0x3 << ADC_BIAS_VRDA_IB_SHIFT)
#define ADC_BIAS_VRDA_IB(x)                                               ((x) << ADC_BIAS_VRDA_IB_SHIFT)

#define ADC_BIAS_OPBUF_IB_SHIFT                                           (4) /* FD BUF op bias control */
#define ADC_BIAS_OPBUF_IB_MASK                                            (0x3 << ADC_BIAS_OPBUF_IB_SHIFT)
#define ADC_BIAS_OPBUF_IB(x)                                              ((x) << ADC_BIAS_OPBUF_IB_SHIFT)

#define ADC_BIAS_PREOP_IB_SHIFT                                           (0) /* PRE op bias control */
#define ADC_BIAS_PREOP_IB_MASK                                            (0x7 << ADC_BIAS_PREOP_IB_SHIFT)
#define ADC_BIAS_PREOP_IB(x)                                              ((x) << ADC_BIAS_PREOP_IB_SHIFT)

/***************************************************************************************************
 * ADC FEATURES CONGIURATION
 */
#define ADC_OSR_DEFAULT                                                   (0) /* ADC_OSR_128FS */
#define ADC_FEEDBACK_RES_INVALID                                          (0xFF)

/* @brief Definition for ADC OP setting */
#define ADC_OP_GAIN_DIFF_MIN_DB                                           (-12)
#define ADC_OP_GAIN_DIFF_MAX_DB                                           (85)
#define ADC_OP_GAIN_SE_MIN_DB                                             (-18)
#define ADC_OP_GAIN_SE_MAX_DB                                             (79)
#define ADC_DMIC_GAIN_MIN_DB                                              (0)
#define ADC_DMIC_GAIN_MAX_DB                                              (88)

#define	ADC_INPUT0_CHANNEL    			                                  (0)
#define	ADC_INPUT1_CHANNEL				                                  (8)
#define	ADC_INPUT2_CHANNEL				                                  (16)

/*
 * @struct acts_audio_adc
 * @brief ADC controller hardware register
 */
struct acts_audio_adc {
	volatile u32_t adc_digctl; /* ADC digital control */
	volatile u32_t lch_digctl; /* left channel digital control */
	volatile u32_t rch_digctl; /* right channel digital control */
	volatile u32_t fifoctl;    /* ADC fifo control */
	volatile u32_t stat;       /* ADC stat */
	volatile u32_t dat;        /* ADC data */
	volatile u32_t aac_ctl;    /* Auto amplitude limit control */
	volatile u32_t input_ctl;  /* ADC input control */
	volatile u32_t adc_ctl;    /* ADC control */
	volatile u32_t bias;       /* ADC bias */
};

struct adc_private_data {
	struct device *parent_dev; /* Indicates the parent audio device */
	audio_lch_input_e left_input;   /* Left channel input selection */
	audio_rch_input_e right_input;  /* Right channel input selection */
};

struct adc_amic_aux_gain_setting {
	s16_t gain;
	u8_t input_res;
	u8_t feedback_res;
	u8_t digital_gain;
};

struct adc_dmic_gain_setting {
	s16_t gain;
	u8_t dmic_pre_gain;
	u8_t digital_gain;
};

/**
  * @struct adc_amic_aux_gain_setting
  * @brief The gain mapping table of the analog mic and aux.
  * @note By the SD suggestion, it is suitable to ajust the analog gain when below 20dB.
  * Whereas, it is the same effect to ajust the analog or digital gian when above 20dB.
  */
static const struct adc_amic_aux_gain_setting amic_aux_gain_mapping[] = {
	{-120, 0, 0, 0},
	{-90, 0, 1, 0},
	{-60, 0, 2, 0},
	{-50, 0, 3, 0},
	{-40, 0, 4, 0},
	{-30, 0, 5, 0},
	{-20, 0, 6, 0},
	{-10, 0, 7, 0},
	{0, 0, 8, 0},
	{10, 0, 9, 0},
	{20, 0, 10, 0},
	{30, 0, 11, 0},
	{40, 0, 12, 0},
	{50, 0, 13, 0},
	{60, 0, 14, 0},
	{70, 0, 15, 0},
	{80, 1, 10, 0},
	{90, 1, 11, 0},
	{100, 1, 12, 0},
	{110, 1, 13, 0},
	{120, 1, 14, 0},
	{130, 1, 15, 0},
	{140, 2, 10, 0},
	{150, 2, 11, 0},
	{160, 2, 12, 0},
	{170, 2, 13, 0},
	{180, 2, 15, 0},
	{190, 2, 16, 0},
	{200, 3, 2, 0},
	{210, 3, 3, 0},
	{220, 3, 4, 0},
	{230, 3, 5, 0},
	{235, 3, 2, 1},
	{240, 3, 6, 0},
	{245, 3, 3, 1},
	{250, 3, 7, 0},
	{255, 3, 4, 1},
	{260, 3, 8, 0},
	{265, 3, 5, 1},
	{270, 3, 9, 0},
	{275, 3, 6, 1},
	{280, 3, 10, 0},
	{285, 3, 7, 1},
	{290, 3, 11, 0},
	{295, 3, 8, 1},
	{300, 3, 12, 0},
	{305, 3, 9, 1},
	{310, 3, 13, 0},
	{315, 3, 10, 1},
	{320, 3, 14, 0},
	{325, 3, 11, 1},
	{330, 3, 15, 0},
	{335, 3, 12, 1},
	{340, 3, 9, 2},
	{345, 3, 13, 1},
	{350, 3, 10, 2},
	{355, 3, 14, 1},
	{360, 3, 11, 2},
	{365, 3, 15, 1},
	{370, 3, 12, 2},
	{375, 3, 9, 3},
	{380, 3, 13, 2},
	{385, 3, 10, 3},
	{390, 3, 14, 2},
	{395, 3, 11, 3},
	{400, 3, 15, 2},
	{405, 3, 12, 3},
	{410, 3, 9, 4},
	{415, 3, 13, 3},
	{420, 3, 10, 4},
	{425, 3, 14, 3},
	{430, 3, 11, 4},
	{435, 3, 15, 3},
	{440, 3, 12, 4},
	{445, 3, 9, 5},
	{450, 3, 13, 4},
	{455, 3, 10, 5},
	{460, 3, 14, 4},
	{465, 3, 11, 5},
	{470, 3, 15, 4},
	{475, 3, 12, 5},
	{480, 3, 9, 6},
	{485, 3, 13, 5},
	{490, 3, 10, 6},
	{495, 3, 14, 5},
	{500, 3, 11, 6},
	{505, 3, 15, 5},
	{510, 3, 12, 6},
	{515, 3, 9, 7},
	{520, 3, 13, 6},
	{525, 3, 10, 7},
	{530, 3, 14, 6},
	{535, 3, 11, 7},
	{540, 3, 15, 6},
	{545, 3, 12, 7},
	{550, 3, 9, 8},
	{555, 3, 13, 7},
	{560, 3, 10, 8},
	{565, 3, 14, 7},
	{570, 3, 11, 8},
	{575, 3, 15, 7},
	{580, 3, 12, 8},
	{585, 3, 9, 9},
	{590, 3, 13, 8},
	{595, 3, 10, 9},
	{600, 3, 14, 8},
	{605, 3, 11, 9},
	{610, 3, 15, 8},
	{615, 3, 12, 9},
	{620, 3, 9, 10},
	{625, 3, 13, 9},
	{630, 3, 10, 10},
	{635, 3, 14, 9},
	{640, 3, 11, 10},
	{645, 3, 15, 9},
	{650, 3, 12, 10},
	{655, 3, 9, 11},
	{660, 3, 13, 10},
	{665, 3, 10, 11},
	{670, 3, 14, 10},
	{675, 3, 11, 11},
	{680, 3, 15, 10},
	{685, 3, 12, 11},
	{690, 3, 9, 12},
	{695, 3, 13, 11},
	{700, 3, 10, 12},
	{705, 3, 14, 11},
	{710, 3, 11, 12},
	{715, 3, 15, 11},
	{720, 3, 12, 12},
	{725, 3, 9, 13},
	{730, 3, 13, 12},
	{735, 3, 10, 13},
	{740, 3, 14, 12},
	{745, 3, 11, 13},
	{750, 3, 15, 12},
	{755, 3, 12, 13},
	{760, 3, 9, 14},
	{765, 3, 13, 13},
	{770, 3, 10, 14},
	{775, 3, 14, 13},
	{780, 3, 11, 14},
	{785, 3, 15, 13},
	{790, 3, 12, 14},
	{795, 3, 9, 15},
	{800, 3, 13, 14},
	{805, 3, 10, 15},
	{810, 3, 14, 14},
	{815, 3, 11, 15},
	{820, 3, 15, 14},
	{825, 3, 12, 15},
	{835, 3, 13, 15},
	{845, 3, 14, 15},
	{855, 3, 15, 15},
};

#ifdef CONFIG_ADC_DMIC
/* dB = 20 x log(x) */
static const struct adc_dmic_gain_setting dmic_gain_mapping[] = {
	{0, 0, 0},
	{60, 1, 0},
	{120, 2, 0},
	{180, 3, 0},
	{240, 4, 0},
	{300, 5, 0},
	{360, 6, 0},
	{395, 6, 1},
	{430, 6, 2},
	{465, 6, 3},
	{500, 6, 4},
	{535, 6, 5},
	{570, 6, 6},
	{605, 6, 7},
	{640, 6, 8},
	{675, 6, 9},
	{710, 6, 10},
	{745, 6, 11},
	{780, 6, 12},
	{815, 6, 13},
	{850, 6, 14},
	{885, 6, 15},
};
#endif

/*
 * enum a_adc_fir_e
 * @brief ADC frequency response (FIR) mode selection
 */
typedef enum {
	ADC_FIR_MODE_A = 0,
	ADC_FIR_MODE_B,
	ADC_FIR_MODE_C,
} a_adc_fir_e;

/*
 * enum a_hpf_time_e
 * @brief HPF(High Pass Filter) auto setting time selection
 */
typedef enum {
	HPF_TIME_0 = 0, /* 1.3ms at 48kfs*/
	HPF_TIME_1, /* 5ms at 48kfs */
	HPF_TIME_2, /* 10ms  at 48kfs*/
	HPF_TIME_3 /* 20ms at 48kfs */
} a_hpf_time_e;

/*
 * enum a_input_lr_e
 * @brief ADC input left and right selection
 */
typedef enum {
	INPUT_LEFT_SEL = (1 << 0), /* INPUTL selection */
	INPUT_RIGHT_SEL = (1 << 1) /* INPUTR selection */
} a_input_lr_e;

typedef struct {
	s16_t gain;
	u8_t fb_res;
	u8_t input_res;
} adc_gain_input_t;

/*
 * @brief  Get the ADC controller base address
 * @param  void
 * @return  The base address of the ADC controller
 */
static inline struct acts_audio_adc *get_adc_base(void)
{
	return (struct acts_audio_adc *)AUDIO_ADC_REG_BASE;
}

/*
 * @brief Dump the ADC relative registers
 * @param void
 * @return void
 */
static void adc_dump_regs(void)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	struct acts_audio_adc *adc_reg = get_adc_base();

	SYS_LOG_INF("** adc contoller regster **");
	SYS_LOG_INF("          BASE: %08x", (u32_t)adc_reg);
	SYS_LOG_INF("    ADC_DIGCTL: %08x", adc_reg->adc_digctl);
	SYS_LOG_INF("    LCH_DIGCTL: %08x", adc_reg->lch_digctl);
	SYS_LOG_INF("    RCH_DIGCTL: %08x", adc_reg->rch_digctl);
	SYS_LOG_INF("   ADC_FIFOCTL: %08x", adc_reg->fifoctl);
	SYS_LOG_INF("      ADC_STAT: %08x", adc_reg->stat);
	SYS_LOG_INF("       ADC_DAT: %08x", adc_reg->dat);
	SYS_LOG_INF("       AAL_CTL: %08x", adc_reg->aac_ctl);
	SYS_LOG_INF(" ADC_INPUT_CTL: %08x", adc_reg->input_ctl);
	SYS_LOG_INF("       ADC_CTL: %08x", adc_reg->adc_ctl);
	SYS_LOG_INF("      ADC_BIAS: %08x", adc_reg->bias);
	SYS_LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
	SYS_LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
	SYS_LOG_INF("    CMU_ADCCLK: %08x", sys_read32(CMU_ADCCLK));
#endif
}

/*
 * @brief  Disable the ADC FIFO
 * @param  void
 * @return  void
 */
static void adc_fifo_disable(void)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	adc_reg->fifoctl &= ~ADC_FIFOCTL_ADFRT;
}

/*
 * @brief  Enable the ADC FIFO
 * @param  sel: select the fifo input source, e.g. CPU or DMA
 * @param  width: specify the transfer width when in dma mode
 * @return 0 on success, negative errno code on fail.
 */
static void adc_fifo_enable(audio_fifouse_sel_e sel, audio_dma_width_e width)
{
	struct acts_audio_adc *adc_reg = get_adc_base();

	if (FIFO_SEL_CPU == sel) {
		adc_reg->fifoctl = (ADC_FIFOCTL_ADFFIE | ADC_FIFOCTL_ADFRT);
	} else if (FIFO_SEL_DMA == sel) {
		adc_reg->fifoctl = (ADC_FIFOCTL_ADFOS
			| ADC_FIFOCTL_ADFFDE | ADC_FIFOCTL_ADFRT);
		if (DMA_WIDTH_16BITS == width)
			adc_reg->fifoctl |= ADC_FIFOCTL_ADCFIFO_DMAWIDTH;
	} else {
		SYS_LOG_ERR("invalid fifo sel %d", sel);
	}
}

/*
 * @brief  Enable the ADC digital function
 * @param  left_input: select the channel type of the left channel
 * @param  right_input: select the channel type of the right channel
 * @param  sample_rate: the channel sample rate
 * @return 0 on success, negative errno code on fail.
 */
static int adc_digital_enable(audio_lch_input_e left_input, audio_rch_input_e right_input, u8_t sample_rate)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg_chl = 0, reg_adc = 0;
	a_adc_fir_e fir;

	/* Configure the Programmable frequency response curve according with the sample rate */
	if (sample_rate <= 16) {
		fir = ADC_FIR_MODE_B;
	} else if (sample_rate < 96) {
		fir = ADC_FIR_MODE_A;
	} else {
		fir = ADC_FIR_MODE_C;
	}

	reg_adc = adc_reg->adc_digctl;
	reg_adc &= ~(ADC_DIGCTL_ADC_OVFS | (ADC_DIGCTL_ADC_FIR_MD_SEL_MASK)
				| ADC_DIGCTL_ADCR_DIG_EN | ADC_DIGCTL_ADCL_DIG_EN);

	if (ADC_OSR_DEFAULT)
		reg_adc |= ADC_DIGCTL_ADC_OVFS;
	else
		reg_adc &= ~ADC_DIGCTL_ADC_OVFS;

	/* ADC OSR 64fs when above 96fs */
	if (sample_rate >= 96)
		reg_adc |= ADC_DIGCTL_ADC_OVFS;

	reg_adc |= ADC_DIGCTL_ADC_FIR_MD_SEL(fir);

	/* Enable ADC FIR Filter CLK and ADC CIC Filter*/
	sys_write32(sys_read32(CMU_ADCCLK) | (1 << CMU_ADCCLK_ADCCIC_EN)
		| (1 << CMU_ADCCLK_ADCFIR_EN), CMU_ADCCLK);

	if (INPUT_LCH_DISABLE != left_input) {
		if (INPUT_LCH_DMIC == left_input) {
#ifdef CONFIG_ADC_DMIC
			/* Enable DMIC clk */
			sys_write32(sys_read32(CMU_ADCCLK)
				| (1 << CMU_ADCCLK_ADCDMIC_EN), CMU_ADCCLK);
			reg_chl = adc_reg->lch_digctl
				| LCH_DIGCTL_DAT_OUT_EN | LCH_DIGCTL_MIC_SEL;
#endif
		} else {
			/* Enable ADC analog clk */
			sys_write32(sys_read32(CMU_ADCCLK)
				| (1 << CMU_ADCCLK_ADCANA_EN), CMU_ADCCLK);
			/* Select ADC analog part */
			reg_chl = adc_reg->lch_digctl & ~LCH_DIGCTL_MIC_SEL;
			reg_chl |= LCH_DIGCTL_DAT_OUT_EN;
		}
		reg_adc |= ADC_DIGCTL_ADCL_DIG_EN;
		adc_reg->lch_digctl = reg_chl;
	} else {
		/* L channel FIFO timing slot disable */
		reg_chl = adc_reg->lch_digctl & (~LCH_DIGCTL_DAT_OUT_EN);
		adc_reg->lch_digctl = reg_chl;
	}

	if (INPUT_RCH_DISABLE != right_input) {
		if (INPUT_RCH_DMIC == right_input) {
#ifdef CONFIG_ADC_DMIC
			/* Enable DMIC clk */
			sys_write32(sys_read32(CMU_ADCCLK)
				| (1 << CMU_ADCCLK_ADCDMIC_EN), CMU_ADCCLK);
			reg_chl = adc_reg->rch_digctl
				| RCH_DIGCTL_DAT_OUT_EN | RCH_DIGCTL_MIC_SEL;
#endif
		} else {
			/* Enable ADC analog clk */
			sys_write32(sys_read32(CMU_ADCCLK)
				| (1 << CMU_ADCCLK_ADCANA_EN), CMU_ADCCLK);
			/* Select ADC analog part */
			reg_chl = adc_reg->rch_digctl & ~RCH_DIGCTL_MIC_SEL;
			reg_chl |= RCH_DIGCTL_DAT_OUT_EN;
		}
		reg_adc |= ADC_DIGCTL_ADCR_DIG_EN;
		adc_reg->rch_digctl = reg_chl;
	} else {
		/* R channel FIFO timing slot disable */
		reg_chl = adc_reg->rch_digctl & (~RCH_DIGCTL_DAT_OUT_EN);
		adc_reg->rch_digctl = reg_chl;
	}

	adc_reg->adc_digctl = reg_adc;

	return 0;
}

/*
 * @brief  Disable the ADC digital function
 * @param  void
 * @return void
 */
static void adc_digital_disable(void)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg = adc_reg->adc_digctl;;
	/* Disable  FIR / CIC / ANA / DMIC clk */
	sys_write32(sys_read32(CMU_ADCCLK)
		& ~(0xF << CMU_ADCCLK_ADCDMIC_EN), CMU_ADCCLK);
	reg &= ~(ADC_DIGCTL_ADC_OVFS | ADC_DIGCTL_ADC_FIR_MD_SEL_MASK
				| ADC_DIGCTL_ADCR_DIG_EN | ADC_DIGCTL_ADCL_DIG_EN);
	adc_reg->adc_digctl = reg;
}

/*
 * @brief  The ADC HPF(High Pass Filter) auto set enable
 * @param lch_t: left channel HPF time setting
 * @param rch_t: right channel HPF time setting
 * @return void
 */
static void adc_hpf_auto_set(a_hpf_time_e lch_t, a_hpf_time_e rch_t)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t regl, regr;
	regl = adc_reg->lch_digctl & (~LCH_DIGCTL_HPF_AS_TS_MASK);
	regr = adc_reg->rch_digctl & (~RCH_DIGCTL_HPF_AS_TS_MASK);

	/*
	 * HPF auto set steps:
	 * 1. configure auto set time
	 * 2. enable HPFEN
	 * 3. enable ADC digital
	 */
	regl |= LCH_DIGCTL_HPF_AS_TS(lch_t);
	regr |= RCH_DIGCTL_HPF_AS_TS(rch_t);

	adc_reg->lch_digctl = regl;
	adc_reg->rch_digctl = regr;

	adc_reg->lch_digctl |= LCH_DIGCTL_HPF_AS_EN;
	adc_reg->rch_digctl |= RCH_DIGCTL_HPF_AUTO_SET;

}

/*
 * @brief The ADC HPF(High Pass Filter) configuration
 * @param lch_hpf: left channel HPF frequency fc select
 * @param rch_hpf: left channel HPF frequency fc select
 * @return void
 */
static void adc_hpf_enable(u8_t lch_hpf, u8_t rch_hpf, bool lch_low_fc, bool rch_low_fc)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t regl, regr;
	regl = adc_reg->lch_digctl & (~LCH_DIGCTL_HPF_N_MASK);
	regr = adc_reg->rch_digctl & (~RCH_DIGCTL_HPF_N_MASK);

	/* Low or hight frequency range selection */
	if (lch_low_fc)
		regl &= ~LCH_DIGCTL_HPF_S;
	else
		regl |= LCH_DIGCTL_HPF_S;

	if (rch_low_fc)
		regr &= ~RCH_DIGCTL_HPF_S;
	else
		regr |= RCH_DIGCTL_HPF_S;

	regl |= LCH_DIGCTL_HPF_N(lch_hpf & 0x3F);
	regr |= RCH_DIGCTL_HPF_N(rch_hpf & 0x3F);

	/* Enable High Pass Filter */
	regl |= LCH_DIGCTL_HPFEN;
	regr |= RCH_DIGCTL_HPFEN;

	adc_reg->lch_digctl = regl;
	adc_reg->rch_digctl = regr;
}

static void adc_hpf_config(void)
{
	bool lch_low_fc = true, rch_low_fc = true;

	adc_hpf_auto_set(CONFIG_ADC_LCH_HPF_AUTOSET_TIME,
						CONFIG_ADC_RCH_HPF_AUTOSET_TIME);

#ifdef CONFIG_ADC_LCH_HPF_HIGH_FREQ
	lch_low_fc = false;
#endif

#ifdef CONFIG_ADC_RCH_HPF_HIGH_FREQ
	rch_low_fc = false;
#endif

	adc_hpf_enable(CONFIG_ADC_LCH_HPF_FC_SEL, CONFIG_ADC_RCH_HPF_FC_SEL,
					lch_low_fc, rch_low_fc);
}

/*
 * @brief  Set ADC L/R channel digital gain control
 * @param  l_vol: left channel digital gain value
 * @param  r_vol: right channel digital gain value
 * @return void
 */
static void adc_dig_vol_set(u16_t l_vol, u16_t r_vol)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t regl, regr;

	if (ADC_GAIN_INVALID != l_vol) {
		regl = adc_reg->lch_digctl & (~LCH_DIGCTL_ADCGC_MASK);
		regl |= LCH_DIGCTL_ADCGC(l_vol & 0xF);
		adc_reg->lch_digctl = regl;
	}

	if (ADC_GAIN_INVALID != r_vol) {
		regr = adc_reg->rch_digctl & (~RCH_DIGCTL_ADCGC_MASK);
		regr |= RCH_DIGCTL_ADCGC(r_vol & 0xF);
		adc_reg->rch_digctl = regr;
	}
}

/*
 * @brief  ADC DMIC pre gain configuration
 * @param  vol: pre gain control for DMIC
 * @return void
 */
#ifdef CONFIG_ADC_DMIC
static void adc_dmic_pre_gain_cfg(s16_t vol)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg = adc_reg->adc_digctl & ~ADC_DIGCTL_DMIC_PRE_GAIN_MASK;
	if (ADC_GAIN_INVALID != vol) {
		reg |= ADC_DIGCTL_DMIC_PRE_GAIN(vol);
		adc_reg->adc_digctl = reg;
	}
}
#endif

static int inputx_analog_gain_set(u8_t input_channel, u8_t input_res, u8_t feedback_res)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg;

	if ((input_channel != ADC_INPUT0_CHANNEL) && (input_channel != ADC_INPUT1_CHANNEL)
		&& (input_channel != ADC_INPUT2_CHANNEL)) {
		SYS_LOG_DBG("invalid input channel %d", input_channel);
		return -EINVAL;
	}

	reg = adc_reg->input_ctl;
	/* clear inputx res */
	reg &= ~(0x3 << input_channel);
	reg |= (input_res << input_channel);
	adc_reg->input_ctl = reg;

	/* ADC_INPUT_CTL[3]/[11]/[19] == 1 means single end mode */
	if ((reg >> (3 + input_channel)) & 1) {
		reg = adc_reg->adc_ctl;
		reg &= ~ADC_CTL_FDBUFL_IRS_MASK;
		reg &= ~ADC_CTL_FDBUFR_IRS_MASK;
		reg &= ~ADC_CTL_PREAM_PG_MASK;

		/* if adcr en,enable fdbuf r input res.if adcr disable,disable input res */
		if (reg & 1) {
		  reg |= (input_res << ADC_CTL_FDBUFR_IRS_SHIFT);
		  reg |= 1 << ADC_CTL_FDBUFR_IRS_E;
		}
		if( reg & 2 )	//if adcl en,enable fdbuf l input res.if adcl disable,disable input res
		{
		  reg |= (input_res << ADC_CTL_FDBUFL_IRS_SHIFT);
		  reg |= 1 << ADC_CTL_FDBUFL_IRS_E;
		}
		reg |= ((feedback_res & 0xf) << ADC_CTL_PREAM_PG_SHIFT);
		adc_reg->adc_ctl = reg;
	} else { /* diff mode */
		reg = adc_reg->adc_ctl;
		reg &= ~ADC_CTL_FDBUFL_IRS_MASK;
		reg &= ~ADC_CTL_FDBUFR_IRS_MASK;
		reg &= ~ADC_CTL_PREAM_PG_MASK;
		reg |= ((feedback_res & 0xf) << ADC_CTL_PREAM_PG_SHIFT);
		adc_reg->adc_ctl = reg;
	}

	return 0;
}

static int adc_aux_amic_gain_translate(s16_t gain, u8_t *input_res, u8_t *fd_res, u8_t *dig_gain)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(amic_aux_gain_mapping); i++) {
		if (gain <= amic_aux_gain_mapping[i].gain) {
			*input_res = amic_aux_gain_mapping[i].input_res;
			*fd_res = amic_aux_gain_mapping[i].feedback_res;
			*dig_gain = amic_aux_gain_mapping[i].digital_gain;
			SYS_LOG_INF("gain:%d map [%d %d %d]",
				gain, *input_res, *fd_res, *dig_gain);
			break;
		}
	}

	if (i == ARRAY_SIZE(amic_aux_gain_mapping)) {
		SYS_LOG_ERR("can not find out gain map %d", gain);
		return -ENOENT;
	}

	return 0;
}

#ifdef CONFIG_ADC_DMIC
static int adc_dmic_gain_translate(s16_t gain, u8_t *dmic_gain, u8_t *dig_gain)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(dmic_gain_mapping); i++) {
		if (gain <= dmic_gain_mapping[i].gain) {
			*dmic_gain = dmic_gain_mapping[i].dmic_pre_gain;
			*dig_gain = dmic_gain_mapping[i].digital_gain;
			SYS_LOG_DBG("gain:%d map [%d %d]",
				gain, *dmic_gain, *dig_gain);
			break;
		}
	}

	if (i == ARRAY_SIZE(dmic_gain_mapping)) {
		SYS_LOG_ERR("can not find out gain map %d", gain);
		return -ENOENT;
	}

	return 0;
}
#endif

static int adc_gain_config(audio_lch_input_e left_input, audio_rch_input_e right_input, s16_t left_gain, s16_t right_gain)
{
	u8_t left_input_ch, right_input_ch;
	u8_t dig_gain, input_res, fd_res;

	SYS_LOG_INF("left input:%d gain: %d, right input:%d gain %d",
		left_input, left_gain, right_input, right_gain);

#ifdef CONFIG_ADC_DMIC
	u8_t dmic_gain;
	if (INPUT_LCH_DMIC == left_input) {
		if (adc_dmic_gain_translate(left_gain, &dmic_gain, &dig_gain)) {
			SYS_LOG_DBG("failed to translate dmic left gain %d", left_gain);
			return -EFAULT;
		}
		adc_dig_vol_set(dig_gain, ADC_GAIN_INVALID);
		adc_dmic_pre_gain_cfg(dmic_gain);
		return 0;
	}

	if (INPUT_RCH_DMIC == right_input) {
		if (adc_dmic_gain_translate(right_gain, &dmic_gain, &dig_gain)) {
			SYS_LOG_DBG("failed to translate dmic right gain %d", right_gain);
			return -EFAULT;
		}
		adc_dig_vol_set(ADC_GAIN_INVALID, dig_gain);
		adc_dmic_pre_gain_cfg(dmic_gain);
		return 0;
	}
#endif

	if ((INPUT0LR_DIFF == left_input)
		|| (INPUT0L_SINGLE_END == left_input)
		|| (INPUT0LR_MIX == left_input)) {
		left_input_ch = ADC_INPUT0_CHANNEL;
	} else if (INPUT1L_SINGLE_END == left_input) {
		left_input_ch = ADC_INPUT1_CHANNEL;
	} else if ((INPUT2LR_DIFF == left_input)
		|| ((INPUT2LR_MIX == left_input))
		|| (INPUT2L_SINGLE_END == left_input)) {
		left_input_ch = ADC_INPUT2_CHANNEL;
	} else {
		left_input_ch = 0xFF;
	}

	if (INPUT0R_SINGLE_END == right_input) {
		right_input_ch = ADC_INPUT0_CHANNEL;
	} else if ((INPUT1R_SINGLE_END == right_input)
		|| (INPUT1LR_MIX == right_input)
		|| (INPUT1LR_DIFF == right_input)) {
		right_input_ch = ADC_INPUT1_CHANNEL;
	} else if (INPUT2R_SINGLE_END == right_input) {
		right_input_ch = ADC_INPUT2_CHANNEL;
	} else {
		right_input_ch = 0xFF;
	}

	if ((INPUT0L_SINGLE_END == left_input)
		|| (INPUT1L_SINGLE_END == left_input)
		|| (INPUT2L_SINGLE_END == left_input)) {
		left_gain += 60; /* single-end is 6db minus than diff mode */
	}

	if ((INPUT0R_SINGLE_END == right_input)
		|| (INPUT1R_SINGLE_END == right_input)
		|| (INPUT2R_SINGLE_END == right_input)) {
		right_gain += 60;
	}

	if (left_input_ch == right_input_ch) {
		if (adc_aux_amic_gain_translate(left_gain, &input_res, &fd_res, &dig_gain)) {
			SYS_LOG_DBG("failed to translate amic_aux left gain %d", left_gain);
			return -EFAULT;
		}
		adc_dig_vol_set(dig_gain, dig_gain);
		inputx_analog_gain_set(left_input_ch, input_res, fd_res);
	} else {
		if (adc_aux_amic_gain_translate(left_gain, &input_res, &fd_res, &dig_gain)) {
			SYS_LOG_DBG("failed to translate amic_aux left gain %d", left_gain);
			return -EFAULT;
		}
		adc_dig_vol_set(dig_gain, ADC_GAIN_INVALID);
		inputx_analog_gain_set(left_input_ch, input_res, fd_res);
		if (adc_aux_amic_gain_translate(right_gain, &input_res, &fd_res, &dig_gain)) {
			SYS_LOG_DBG("failed to translate amic_aux right gain %d", right_gain);
			return -EFAULT;
		}
		adc_dig_vol_set(ADC_GAIN_INVALID, dig_gain);
		inputx_analog_gain_set(right_input_ch, input_res, fd_res);
	}

	return 0;
}

/*
 * @brief  ADC INPUT0 to ADCL at differential input mode
 * @param  void
 * @return void
 */
static void adc_input0_diff_to_adcl(void)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg = adc_reg->input_ctl;
	reg &= ~(0x3F << ADC_INPUT_CTL_INPUT0LRMIX_SHIFT);
	/* INPUT0L to ADC L channel enable; INPUT0R to ADC R channel enable */
	reg |= (ADC_INPUT_CTL_INPUT0R_EN(3) | ADC_INPUT_CTL_INPUT0L_EN(3));
	adc_reg->input_ctl = reg;

	reg = adc_reg->adc_ctl;

	/* FD BUF OP L disable for differential mode */
	reg &= ~ADC_CTL_FDBUFL_EN;

	/* Enable  BIAS / PREOP L / VRDAL OP / ADC L channel*/
	reg |= (ADC_CTL_BIASEN | ADC_CTL_PREOPL_EN
		| ADC_CTL_VRDAL_EN | ADC_CTL_ADCL_EN);
	adc_reg->adc_ctl = reg;
}

/*
 * @brief  ADC INPUT1 to ADCR at differential input mode
 * @param  void
 * @return void
 */
static void adc_input1_diff_to_adcr(void)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg = adc_reg->input_ctl;
	reg &= ~(0x3F << ADC_INPUT_CTL_INPUT1LRMIX_SHIFT);
	reg |= (ADC_INPUT_CTL_INPUT1R_EN(3) | (ADC_INPUT_CTL_INPUT1L_EN(3)));
	adc_reg->input_ctl = reg;

	reg = adc_reg->adc_ctl;

	/* FD BUF OP R disable for differential mode */
	reg &= ~(ADC_CTL_FDBUFR_EN);

	/* Enable  BIAS / PREOP R / VRDAR OP / ADC R channel*/
	reg |= (ADC_CTL_BIASEN | ADC_CTL_PREOPR_EN
			| ADC_CTL_VRDAR_EN | ADC_CTL_ADCR_EN);
	adc_reg->adc_ctl = reg;
}

/*
 * @brief  ADC INPUT2 to ADCL at differential input mode
 * @param  void
 * @return void
 */
static void adc_input2_diff_to_adcl(void)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg = adc_reg->input_ctl;
	reg &= ~(0x3F << ADC_INPUT_CTL_INPUT2LRMIX_SHIFT);
	reg |= (ADC_INPUT_CTL_INPUT2R_EN(3) | ADC_INPUT_CTL_INPUT2L_EN(3));
	adc_reg->input_ctl = reg;

	reg = adc_reg->adc_ctl;

	/* FD BUF OP L disable for differential mode */
	reg &= ~ADC_CTL_FDBUFL_EN;

	/* Enable  BIAS / PREOP L / VRDAL OP / ADC L channel*/
	reg |= (ADC_CTL_BIASEN | ADC_CTL_PREOPL_EN
		| ADC_CTL_VRDAL_EN | ADC_CTL_ADCL_EN);
	adc_reg->adc_ctl = reg;
}

/*
 * @brief  ADC INPUT0 configuration at single end mode
 * @param  lr_sel: input left and right selection
 * @param  lr_mix: control input left and right mix to ADC L
 * @return void
 */
static void adc_input0_single_end(u8_t lr_sel, bool lrmix)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg_input = adc_reg->input_ctl;
	u32_t reg_adc = adc_reg->adc_ctl;
	reg_input &= ~(0x3 << ADC_INPUT_CTL_INPUT0LRMIX_SHIFT);

	/* Select the  single end input mode
	 * INPUTL to ADC L channel; INPUTR to ADC R channel
	 */
	reg_input |= ADC_INPUT_CTL_INPUT0_IN_MODE;

	/* If both enable INPUT LR channel, it is allow to enable mix function,
	 * And the mix function is forbidden when only one channel enabled,
	 * for the issue of the noise.
	 */
	if (lr_sel & (INPUT_LEFT_SEL | INPUT_RIGHT_SEL)) {
		/* INPUTL and INPUTR mix to ADCL enable */
		if (lrmix)
			reg_input |= ADC_INPUT_CTL_INPUT0LRMIX;
		else
			reg_input &= ~ADC_INPUT_CTL_INPUT0LRMIX;
	}

	/* BIAS enable */
	reg_adc |= ADC_CTL_BIASEN;

	if (lr_sel & INPUT_LEFT_SEL) {
		reg_input &= ~(ADC_INPUT_CTL_INPUT0L_EN_MASK);
		/* INPUT0L to ADC L channel input enable  */
		reg_input |= ADC_INPUT_CTL_INPUT0L_EN(3);

		/* FD BUF OP L enable for SE mode */
		reg_adc |= ADC_CTL_FDBUFL_EN;

		/* Enable  PREOP L / VRDAL OP / ADC L channel*/
		reg_adc |= (ADC_CTL_PREOPL_EN | ADC_CTL_VRDAL_EN
					| ADC_CTL_ADCL_EN);
	}

	if (lr_sel & INPUT_RIGHT_SEL) {
		reg_input &= ~(ADC_INPUT_CTL_INPUT0R_EN_MASK);
		/* INPUT0R to ADC R channel input enable  */
		reg_input |= ADC_INPUT_CTL_INPUT0R_EN(3);

		/* FD BUF OP R enable for SE mode */
		reg_adc |= ADC_CTL_FDBUFR_EN;

		if (!lrmix) {
			/* Enable PREOP R / VRDAR OP / ADC R channel*/
			reg_adc |= (ADC_CTL_PREOPR_EN | ADC_CTL_VRDAR_EN
						| ADC_CTL_ADCR_EN);
		}
	}

	adc_reg->input_ctl = reg_input;
	adc_reg->adc_ctl = reg_adc;
}

/*
 * @brief  ADC INPUT1 configuration at single end mode
 * @param  lr_sel: input left and right selection
 * @param  lr_mix: control input left and right mix to ADC R
 * @return void
 */
static void adc_input1_single_end(u8_t lr_sel, bool lrmix)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg_input = adc_reg->input_ctl;
	u32_t reg_adc = adc_reg->adc_ctl;
	reg_input &= ~(0x3 << ADC_INPUT_CTL_INPUT1LRMIX_SHIFT);

	/* Select the  single end input mode
	 * INPUTL to ADC L channel; INPUTR to ADC R channel
	 */
	reg_input |= ADC_INPUT_CTL_INPUT1_IN_MODE;

	/* If both enable INPUT LR channel, it is allow to enable mix function,
	 * And the mix function is forbidden when only one channel enabled,
	 * for the issue of the noise.
	 */
	if (lr_sel & (INPUT_LEFT_SEL | INPUT_RIGHT_SEL)) {
		/* INPUTL and INPUTR mix to ADC R enable */
		if (lrmix)
			reg_input |= ADC_INPUT_CTL_INPUT1LRMIX;
		else
			reg_input &= ~ADC_INPUT_CTL_INPUT1LRMIX;
	}

	/* BIAS enable */
	reg_adc |= ADC_CTL_BIASEN;

	if (lr_sel & INPUT_LEFT_SEL) {
		reg_input &= ~(ADC_INPUT_CTL_INPUT1L_EN_MASK);
		/* INPUT0L to ADC L channel input enable  */
		reg_input |= ADC_INPUT_CTL_INPUT1L_EN(3);

		/* FD BUF OP L enable for SE mode */
		reg_adc |= ADC_CTL_FDBUFL_EN;

		/* Enable  PREOP L / VRDAL OP / ADC L channel*/
		reg_adc |= (ADC_CTL_PREOPL_EN | ADC_CTL_VRDAL_EN
					| ADC_CTL_ADCL_EN);
	}

	if (lr_sel & INPUT_RIGHT_SEL) {
		reg_input &= ~(ADC_INPUT_CTL_INPUT1R_EN_MASK);
		/* INPUT0R to ADC R channel input enable  */
		reg_input |= ADC_INPUT_CTL_INPUT1R_EN(3);

		/* FD BUF OP R enable for SE mode */
		reg_adc |= ADC_CTL_FDBUFR_EN;

		if (!lrmix) {
			/* Enable PREOP R / VRDAR OP / ADC R channel*/
			reg_adc |= (ADC_CTL_PREOPR_EN | ADC_CTL_VRDAR_EN
						| ADC_CTL_ADCR_EN);
		}
	}

	adc_reg->input_ctl = reg_input;
	adc_reg->adc_ctl = reg_adc;
}

/*
 * @brief  ADC INPUT2 configuration at single end mode
 * @param  lr_sel: input left and right selection
 * @param  lr_mix: control input left and right mix to ADC L
 * @return void
 */
static void adc_input2_single_end(u8_t lr_sel, bool lrmix)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	u32_t reg_input = adc_reg->input_ctl;
	u32_t reg_adc = adc_reg->adc_ctl;
	reg_input &= ~(0x3 << ADC_INPUT_CTL_INPUT2LRMIX_SHIFT);

	/* Select the single end input mode
	 * INPUTL to ADC L channel; INPUTR to ADC R channel
	 */
	reg_input |= ADC_INPUT_CTL_INPUT2_IN_MODE;

	/* If both enable INPUT LR channel, it is allow to enable mix function,
	 * And the mix function is forbidden when only one channel enabled,
	 * for the issue of the noise.
	 */
	if (lr_sel & (INPUT_LEFT_SEL | INPUT_RIGHT_SEL)) {
		/* INPUTL and INPUTR mix to ADCL enable */
		if (lrmix)
			reg_input |= ADC_INPUT_CTL_INPUT2LRMIX;
		else
			reg_input &= ~ADC_INPUT_CTL_INPUT2LRMIX;
	}

	/* BIAS enable */
	reg_adc |= ADC_CTL_BIASEN;

	if (lr_sel & INPUT_LEFT_SEL) {
		reg_input &= ~(ADC_INPUT_CTL_INPUT2L_EN_MASK);
		/* INPUT2L to ADC L channel input enable  */
		reg_input |= ADC_INPUT_CTL_INPUT2L_EN(3);

		/* FD BUF OP L enable for SE mode */
		reg_adc |= ADC_CTL_FDBUFL_EN;

		/* Enable  PREOP L / VRDAL OP / ADC L channel*/
		reg_adc |= (ADC_CTL_PREOPL_EN | ADC_CTL_VRDAL_EN
					| ADC_CTL_ADCL_EN);
	}

	if (lr_sel & INPUT_RIGHT_SEL) {
		reg_input &= ~(ADC_INPUT_CTL_INPUT2R_EN_MASK);
		/* INPUT2R to ADC R channel input enable  */
		reg_input |= ADC_INPUT_CTL_INPUT2R_EN(3);

		/* FD BUF OP R enable for SE mode */
		reg_adc |= ADC_CTL_FDBUFR_EN;

		if (!lrmix) {
			/* Enable PREOP R / VRDAR OP / ADC R channel*/
			reg_adc |= (ADC_CTL_PREOPR_EN | ADC_CTL_VRDAR_EN
						| ADC_CTL_ADCR_EN);
		}
	}

	adc_reg->input_ctl = reg_input;
	adc_reg->adc_ctl = reg_adc;
}

/*
 * @brief  ADC input configuration
 * @param  left_input: The left channel input configuration
 * @param  right_input: The right channel input configuration
 * @return void
 */
static int adc_input_config(audio_lch_input_e left_input, audio_rch_input_e right_input)
{
	int ret = 0;

	if ((INPUT_LCH_DISABLE != left_input) && (INPUT_LCH_DMIC != left_input)) {
		if (INPUT0L_SINGLE_END == left_input)
			adc_input0_single_end(INPUT_LEFT_SEL, false);
		else if (INPUT1L_SINGLE_END == left_input)
			adc_input1_single_end(INPUT_LEFT_SEL, false);
		else if (INPUT2L_SINGLE_END == left_input)
			adc_input2_single_end(INPUT_LEFT_SEL, false);
		else if (INPUT0LR_MIX == left_input)
			adc_input0_single_end(INPUT_LEFT_SEL | INPUT_RIGHT_SEL, true);
		else if (INPUT0LR_DIFF == left_input)
			adc_input0_diff_to_adcl();
		else if (INPUT2LR_MIX == left_input)
			adc_input2_single_end(INPUT_LEFT_SEL | INPUT_RIGHT_SEL, true);
		else if (INPUT2LR_DIFF == left_input)
			adc_input2_diff_to_adcl();
		else
			ret = -ENOTSUP;
	}

	if ((INPUT_RCH_DISABLE != right_input) && (INPUT_RCH_DMIC != right_input)) {
		if (INPUT0R_SINGLE_END == right_input)
			adc_input0_single_end(INPUT_RIGHT_SEL, false);
		else if (INPUT1R_SINGLE_END == right_input)
			adc_input1_single_end(INPUT_RIGHT_SEL, false);
		else if (INPUT2R_SINGLE_END == right_input)
			adc_input2_single_end(INPUT_RIGHT_SEL, false);
		else if (INPUT1LR_MIX == right_input)
			adc_input1_single_end(INPUT_LEFT_SEL | INPUT_RIGHT_SEL, true);
		else if (INPUT1LR_DIFF == right_input)
			adc_input1_diff_to_adcr();
		else
			ret = -ENOTSUP;
	}

	SYS_LOG_INF("left_input type#%d, right input type#%d", left_input, right_input);

	return ret;
}

static int phy_adc_enable(struct phy_audio_device *dev, void *param)
{
	ain_param_t *in_param = (ain_param_t *)param;
	adc_setting_t *adc_setting = in_param->adc_setting;
	struct adc_private_data *priv = (struct adc_private_data *)dev->private;
	struct acts_audio_adc *adc_reg = get_adc_base();
	u8_t lch_input, rch_input;

	if ((!in_param) || (!adc_setting)
		|| (!in_param->sample_rate)) {
		SYS_LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (in_param->channel_type != AUDIO_CHANNEL_ADC) {
		SYS_LOG_ERR("Invalid channel type %d", in_param->channel_type);
		return -EINVAL;
	}

	/* enable adc clock */
	acts_clock_peripheral_enable(CLOCK_ID_AUDIO_ADC);

	if (audio_samplerate_adc_set(in_param->sample_rate)) {
		SYS_LOG_ERR("Failed to config sample rate %d", in_param->sample_rate);
		return -ESRCH;
	}

	if (board_audio_input_function_switch(adc_setting->device, &lch_input, &rch_input)) {
		SYS_LOG_ERR("Failed to get input function dev:%d", adc_setting->device);
		return -ENOENT;
	}

	priv->left_input = lch_input;
	priv->right_input = rch_input;

	adc_fifo_disable();

	/* The difference between the 32 bits data width and 16 bits data shown as the following:
	  * - The ADC sigma-delta is 18bits and in 32 bits data width mode can get the whole 18bits.
	  * - The SNR in 16 bits is about 92dB however 32 bits is up to 96dB.
	  */
#ifdef CONFIG_ADC_32BITS_WIDTH_SEL
	adc_fifo_enable(FIFO_SEL_DMA, DMA_WIDTH_32BITS);
#else
	adc_fifo_enable(FIFO_SEL_DMA, DMA_WIDTH_16BITS);
#endif

	adc_hpf_config();

	/* TODO config AAL register */

	/* In case of sample rate bigger than 16kfs, need to set the large PRE OP driver control */
	if (in_param->sample_rate > SAMPLE_RATE_16KHZ)
		adc_reg->bias |= ADC_BIAS_PREOP_ODSC;
	else
		adc_reg->bias &= ~ADC_BIAS_PREOP_ODSC;

	adc_digital_enable(priv->left_input, priv->right_input,
						in_param->sample_rate);

	if (adc_input_config(priv->left_input, priv->right_input)) {
		SYS_LOG_ERR("ADC input configure error");
		return -EFAULT;
	}

	if (adc_gain_config(priv->left_input, priv->right_input,
						adc_setting->gain.left_gain,
						adc_setting->gain.right_gain)) {
		SYS_LOG_ERR("ADC gain config error");
		return -EFAULT;
	}

	/* TODO check if need to eliminate noise ?? */

	return 0;
}

static int phy_adc_disable(struct phy_audio_device *dev)
{
	struct acts_audio_adc *adc_reg = get_adc_base();
	struct adc_private_data *priv = (struct adc_private_data *)dev->private;

	/* DAC FIFO reset */
	adc_fifo_disable();
	adc_digital_disable();

	/* TODO check analog register */
	adc_reg->input_ctl = 0;
	adc_reg->adc_ctl = 0;

	acts_clock_peripheral_disable(CLOCK_ID_AUDIO_ADC);

	priv->left_input = priv->right_input = 0;

	return 0;
}

static int phy_adc_ioctl(struct phy_audio_device *dev, u32_t cmd, void *param)
{
	struct adc_private_data *priv = (struct adc_private_data *)dev->private;
	int ret = 0;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		adc_dump_regs();
		break;
	}
	case AIN_CMD_SET_ADC_GAIN:
	{
		adc_gain *gain = (adc_gain *)param;
		ret = adc_gain_config(priv->left_input, priv->right_input,
				gain->left_gain, gain->right_gain);
		break;
	}
	case AIN_CMD_GET_ADC_LEFT_GAIN_RANGE:
	{
		adc_gain_range *range = (adc_gain_range *)param;
		if (!priv->left_input) {
			SYS_LOG_INF("Left input not set yet using SE range");
			range->min = ADC_OP_GAIN_SE_MIN_DB;
			range->max = ADC_OP_GAIN_SE_MAX_DB;
		} else {
			if ((priv->left_input == INPUT0LR_DIFF)
				|| (priv->left_input == INPUT2LR_DIFF)) {
				range->min = ADC_OP_GAIN_DIFF_MIN_DB;
				range->max = ADC_OP_GAIN_DIFF_MAX_DB;
			} else if (priv->left_input == INPUT_LCH_DMIC) {
				range->min = ADC_DMIC_GAIN_MIN_DB;
				range->max = ADC_DMIC_GAIN_MAX_DB;
			} else {
				range->min = ADC_OP_GAIN_SE_MIN_DB;
				range->max = ADC_OP_GAIN_SE_MAX_DB;
			}
		}

		break;
	}
	case AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE:
	{
		adc_gain_range *range = (adc_gain_range *)param;
		if (!priv->right_input) {
			SYS_LOG_INF("Right input not set yet using SE range");
			range->min = ADC_OP_GAIN_SE_MIN_DB;
			range->max = ADC_OP_GAIN_SE_MAX_DB;
		} else {
			if (priv->right_input == INPUT1LR_DIFF) {
				range->min = ADC_OP_GAIN_DIFF_MIN_DB;
				range->max = ADC_OP_GAIN_DIFF_MAX_DB;
			} else if (priv->right_input == INPUT_RCH_DMIC) {
				range->min = ADC_DMIC_GAIN_MIN_DB;
				range->max = ADC_DMIC_GAIN_MAX_DB;
			}else {
				range->min = ADC_OP_GAIN_SE_MIN_DB;
				range->max = ADC_OP_GAIN_SE_MAX_DB;
			}
		}
		break;
	}
	case AIN_CMD_GET_SAMPLERATE:
	{
		ret = audio_samplerate_adc_get();
		if (ret) {
			SYS_LOG_ERR("Failed to get ADC sample rate err=%d", ret);
			return ret;
		}
		*(audio_sample_rate_e *)param = (audio_sr_sel_e)ret;
		break;
	}
	case AIN_CMD_SET_SAMPLERATE:
	{
		audio_sample_rate_e val = *(audio_sr_sel_e *)param;
		ret = audio_samplerate_adc_set(val);
		if (ret) {
			SYS_LOG_ERR("Failed to set ADC sample rate err=%d", ret);
			return ret;
		}
		break;
	}
	case AIN_CMD_GET_APS:
	{
		ret = audio_pll_get_adc_aps();
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
		ret = audio_pll_set_adc_aps(level);
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

	return ret;
}

static int phy_adc_init(struct phy_audio_device *dev, struct device *parent_dev)
{
	struct adc_private_data *priv = (struct adc_private_data *)dev->private;
	memset(priv, 0, sizeof(struct adc_private_data));

	priv->parent_dev = parent_dev;

	SYS_LOG_INF("PHY ADC probe");

	return 0;
}

static struct adc_private_data adc_priv;

const struct phy_audio_operations adc_aops = {
	.audio_init = phy_adc_init,
	.audio_enable = phy_adc_enable,
	.audio_disable = phy_adc_disable,
	.audio_ioctl = phy_adc_ioctl,
};

PHY_AUDIO_DEVICE(adc, PHY_AUDIO_ADC_NAME, &adc_aops, &adc_priv);
PHY_AUDIO_DEVICE_FN(adc);
