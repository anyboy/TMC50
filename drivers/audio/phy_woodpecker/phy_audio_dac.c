/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio DAC physical implementation
 */

/*
 * Features
 *    - Build in a 20 bits input sigma-delta DAC
 *    - 4 * 2 * 24 bits FIFO
 *    - Support digital volume with zero cross detection
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k
 */

/*
 * Signal List
 * 	- AVCC: Analog power
 *	- AGND: Analog ground
 *	- PAGND: Ground for PA
 *	- AOUTL/AOUTLP: Left output of PA / Left Positive output of PA
 *	- AOUTR/AOUTLN: Right output of PA / Left Negative output of PA
 *	- AOUTRP/VRO: Right Positive output of PA / Virtual Ground for PA
 *	- AOUTRN/VROS: Right Negative output of PA / VRO Sense for PA
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

#define SYS_LOG_DOMAIN "P_DAC"
#include <logging/sys_log.h>

/***************************************************************************************************
 * DAC_DIGCTL
 */
#define DAC_DIGCTL_AMEN                                        BIT(27) /* auto mute enable */
#define DAC_DIGCTL_AMDS                                        BIT(26) /* auto mute detect status */

#define DAC_DIGCTL_AMCNT_SHIFT                                 (24) /* auto mute count */
#define DAC_DIGCTL_AMCNT_MASK                                  (0x3 << DAC_DIGCTL_AMCNT_SHIFT)
#define DAC_DIGCTL_AMCNT(x)                                    ((x) << DAC_DIGCTL_AMCNT_SHIFT)
#define DAC_DIGCTL_AMCNT_512                                   DAC_DIGCTL_AMCNT(0) /* 10ms at 48ksr */
#define DAC_DIGCTL_AMCNT_1024                                  DAC_DIGCTL_AMCNT(1) /* 20ms at 48ksr */
#define DAC_DIGCTL_AMCNT_2048                                  DAC_DIGCTL_AMCNT(2) /* 40ms at 48ksr */
#define DAC_DIGCTL_AMCNT_4096                                  DAC_DIGCTL_AMCNT(3) /* 80ms at 48ksr */

#define DAC_DIGCTL_MULT_DEVICE_SHIFT                           (21) /* mult-devices select */
#define DAC_DIGCTL_MULT_DEVICE_MASK                            (0x3 << DAC_DIGCTL_MULT_DEVICE_SHIFT)
#define DAC_DIGCTL_MULT_DEVICE(x)                              ((x) << DAC_DIGCTL_MULT_DEVICE_SHIFT)
#define DAC_DIGCTL_MULT_DEVICE_DIS                             DAC_DIGCTL_MULT_DEVICE(0)
#define DAC_DIGCTL_MULT_DEVICE_DAC_I2S                         DAC_DIGCTL_MULT_DEVICE(1)
#define DAC_DIGCTL_MULT_DEVICE_DAC_SPDIF                       DAC_DIGCTL_MULT_DEVICE(2)
#define DAC_DIGCTL_MULT_DEVICE_DAC_I2S_SPDIF                   DAC_DIGCTL_MULT_DEVICE(3)

#define DAC_DIGCTL_SDMCNT                                      BIT(20)

#define DAC_DIGCTL_SDMNDTH_SHIFT                               (18)
#define DAC_DIGCTL_SDMNDTH_MASK                                (0x3 << DAC_DIGCTL_SDMNDTH_SHIFT)
#define DAC_DIGCTL_SDMNDTH(x)                                  ((x) << DAC_DIGCTL_SDMNDTH_SHIFT)
#define DAC_DIGCTL_SDMNDTH_HIGH16                              DAC_DIGCTL_SDMNDTH(0) /* SDM noise detection threshold: high 16bits all 0 or 1 */
#define DAC_DIGCTL_SDMNDTH_HIGH15                              DAC_DIGCTL_SDMNDTH(1)
#define DAC_DIGCTL_SDMNDTH_HIGH14                              DAC_DIGCTL_SDMNDTH(2)
#define DAC_DIGCTL_SDMNDTH_HIGH13                              DAC_DIGCTL_SDMNDTH(3)

#define DAC_DIGCTL_SDMREEN                                     BIT(17) /* reset SDM after detect noise, enabled when sr:8k/11k/12k/16k */
#define DAC_DIGCTL_AUDIO_128FS_256FS16                         BIT(16) /* 1: 128fs and 256fs are both existed([SPDIFTX + I2STX(256fs)] or [SPDIFTX + DAC]),  otherwise 0. */
#define DAC_DIGCTL_DADCS                                       BIT(15) /* DAC debug channel select */
#define DAC_DIGCTL_DADEN                                       BIT(14) /* DAC analog debug enable */
#define DAC_DIGCTL_DDDEN                                       BIT(13) /* DAC digital debug enable */
#define DAC_DIGCTL_AD2DALPEN_L                                 BIT(12) /* ADCL digital to DAC loop back enable */
#define DAC_DIGCTL_AD2DALPEN_R                                 BIT(11) /* ADCR digital to DAC loop back enable */
#define DAC_DIGCTL_ADC_LRMIX                                   BIT(10) /* ADCL channel mix ADCR channel enable */
#define DAC_DIGCTL_DALRMIX                                     BIT(9) /* DAC left channel mix right channel */
#define DAC_DIGCTL_DACHNUM                                     BIT(8) /* 0: stereo; 1: mono; NOTE: when mono; LR mix shall be disabled */

#define DAC_DIGCTL_INTFRS_SHIFT                                (6) /* interpolation filter rate select */
#define DAC_DIGCTL_INTFRS_MASK                                 (0x3 << DAC_DIGCTL_INTFRS_SHIFT)
#define DAC_DIGCTL_INTFRS(x)                                   ((x) << DAC_DIGCTL_INTFRS_SHIFT)
#define DAC_DIGCTL_INTFRS_1X                                   DAC_DIGCTL_INTFRS(0) /* bypass */
#define DAC_DIGCTL_INTFRS_2X                                   DAC_DIGCTL_INTFRS(1)
#define DAC_DIGCTL_INTFRS_4X                                   DAC_DIGCTL_INTFRS(2)
#define DAC_DIGCTL_INTFRS_8X                                   DAC_DIGCTL_INTFRS(3)

#define DAC_DIGCTL_OSRDA_SHIFT                                 (2)
#define DAC_DIGCTL_OSRDA_MASK                                  (0x3 << DAC_DIGCTL_OSRDA_SHIFT)
#define DAC_DIGCTL_OSRDA(x)                                    ((x) << DAC_DIGCTL_OSRDA_SHIFT)
#define DAC_DIGCTL_OSRDA_32X                                   DAC_DIGCTL_OSRDA(0)
#define DAC_DIGCTL_OSRDA_64X                                   DAC_DIGCTL_OSRDA(1)
#define DAC_DIGCTL_OSRDA_128X                                  DAC_DIGCTL_OSRDA(2)
#define DAC_DIGCTL_OSRDA_256X                                  DAC_DIGCTL_OSRDA(3)

#define DAC_DIGCTL_ENDITH                                      BIT(1) /* dac dither enable */
#define DAC_DIGCTL_DDEN                                        BIT(0) /* dac digital enable */

/***************************************************************************************************
 * DAC_FIFOCTL
 */
#define DAC_FIFOCTL_DACFIFO_DMAWIDTH                           BIT(7) /* DAC FIFO DMA transfer width; 0: 32bits 1: 16bits */

#define DAC_FIFOCTL_DAFIS_SHIFT                                (4) /* DAC FIFO input select */
#define DAC_FIFOCTL_DAFIS_MASK                                 (0x3 << DAC_FIFOCTL_DAFIS_SHIFT)
#define DAC_FIFOCTL_DAFIS(x)                                   ((x) << DAC_FIFOCTL_DAFIS_SHIFT)
#define DAC_FIFOCTL_DAFIS_CPU                                  DAC_FIFOCTL_DAFIS(0)
#define DAC_FIFOCTL_DAFIS_DMA                                  DAC_FIFOCTL_DAFIS(1)

#define DAC_FIFOCTL_DAFEIE                                     BIT(2) /* DAC FIFO empty IRQ enable */
#define DAC_FIFOCTL_DAFEDE                                     BIT(1) /* DAC FIFO empty DRQ enable */
#define DAC_FIFOCTL_DAFRT                                      BIT(0) /* DAC FIFO Reset; 0: reset, 1: enable */

/***************************************************************************************************
 * DAC_STAT
 */
#define DAC_STAT_FIFO_ER                                       BIT(8) /* 1: error indicates dac fifo was empty */
#define DAC_STAT_DAFEIP                                        BIT(7) /* empty irq pending bit */
#define DAC_STAT_DAFF                                          BIT(6) /* dac fifo full flag, 1: full */
#define DAC_STAT_DAFS_SHIFT                                    (0) /* dac fifo status, indicates how many sample not filled */
#define DAC_STAT_DAFS_MASK                                     (0xF << DAC_STAT_DAFS_SHIFT)

/***************************************************************************************************
 * DAC_DAT_FIFO
 */
#define DAC_DAT_FIFO_DAFDAT_SHIFT                              (8)
#define DAC_DAT_FIFO_DAFDAT_MASK                               (0xFFFFFF << DAC_DAT_FIFO_DAFDAT_SHIFT)
#define DAC_DAT_FIFO_DAFDAT(x)                                 ((x) << DAC_DAT_FIFO_DAFDAT_SHIFT)

/***************************************************************************************************
 * PCM_BUF_CTL
 */
#define PCM_BUF_CTL_PCMBEPIE                                   BIT(7) /* PCM BUF empty irq enable */
#define PCM_BUF_CTL_PCMBFUIE                                   BIT(6) /* PCM BUF full irq enable */
#define PCM_BUF_CTL_PCMBHEIE                                   BIT(5) /* PCM BUF half empty irq enable */
#define PCM_BUF_CTL_PCMBHFIE                                   BIT(4) /* PCM BUF half full irq enable */
#define PCM_BUF_CTL_IRQ_SHIFT                                  (4) /* PCM BUF irq register shift */
#define PCM_BUF_CTL_IRQ_MASK                                   (0xF << PCM_BUF_CTL_IRQ_SHIFT)
#define PCM_BUF_CTL_DATWL_SHIFT                                (0) /* PCM BUF data width select. 0: 24bits; 1: 16bits */
#define PCM_BUF_CTL_DATWL_MASK                                 (1 << PCM_BUF_CTL_DATWL_SHIFT)
#define PCM_BUF_CTL_DATWL(x)                                   ((x) << PCM_BUF_CTL_DATWL_SHIFT)
#define PCM_BUF_CTL_DATWL_24                                   PCM_BUF_CTL_DATWL(0)
#define PCM_BUF_CTL_DATWL_16                                   PCM_BUF_CTL_DATWL(1)

/***************************************************************************************************
 * PCM_BUF_STAT
 */
#define PCM_BUF_STAT_PCMBEIP                                   BIT(19) /* empty irq pending */
#define PCM_BUF_STAT_PCMBFIP                                   BIT(18) /* full irq pending */
#define PCM_BUF_STAT_PCMBHEIP                                  BIT(17) /* half empty irq pending */
#define PCM_BUF_STAT_PCMBHFIP                                  BIT(16) /* half full irq pending */
#define PCM_BUF_STAT_IRQ_SHIFT                                 (16) /* irq pending start */
#define PCM_BUF_STAT_IRQ_MASK                                  (0xF << PCM_BUF_STAT_IRQ_SHIFT)

#define PCM_BUF_STAT_PCMBS_SHIFT                               (0) /* PCM BUF status, indicates how many sample not filled */
#define PCM_BUF_STAT_PCMBS_MASK                                (0xFFF << PCM_BUF_STAT_PCMBS_SHIFT)

/***************************************************************************************************
 * PCM_BUF_THRES_HE
 */
#define PCM_BUF_THRES_HE_THRESHOLD_SHIFT                       (0) /* PCM BUF threshold of half empty */
#define PCM_BUF_THRES_HE_THRESHOLD_MASK                        (0xFFF << PCM_BUF_THRES_HE_THRESHOLD_SHIFT)
#define PCM_BUF_THRES_HE_THRESHOLD(x)                          ((x) << PCM_BUF_THRES_HE_THRESHOLD_SHIFT)

/***************************************************************************************************
 * PCM_BUF_THRES_HF
 */
#define PCM_BUF_THRES_HF_THRESHOLD_SHIFT                       (0) /* PCM BUF threshold of half full */
#define PCM_BUF_THRES_HF_THRESHOLD_MASK                        (0xFFF << PCM_BUF_THRES_HF_THRESHOLD_SHIFT)
#define PCM_BUF_THRES_HF_THRESHOLD(x)                          ((x) << PCM_BUF_THRES_HF_THRESHOLD_SHIFT)

/***************************************************************************************************
 * VOL_LCH
 */
#define VOL_LCH_VOLL_IRQ_EN                                    BIT(21) /* if enable, when finishing soft step would trigger IRQ */
#define VOL_LCH_TO_CNT_SHIFT                                   (20) /* TO_CNT */
#define VOL_LCH_TO_CNT_MASK                                    (1 << VOL_LCH_TO_CNT_SHIFT)
#define VOL_LCH_TO_CNT(x)                                      ((x) << VOL_LCH_TO_CNT_SHIFT)
#define VOL_LCH_TO_CNT_8X                                      VOL_LCH_TO_CNT(0) /* 8x ADJ_CNT */
#define VOL_LCH_TO_CNT_128X                                    VOL_LCH_TO_CNT(1) /* 128x ADJ_CNT */

#define VOL_LCH_ADJ_CNT_SHIFT                                  (12) /* ADJ_CNT which the same as sample rate setting */
#define VOL_LCH_ADJ_CNT_MASK                                   (0xFF << VOL_LCH_ADJ_CNT_SHIFT)
#define VOL_LCH_ADJ_CNT(x)                                     ((x) << VOL_LCH_ADJ_CNT_SHIFT)

#define VOL_LCH_SOFT_STEP_DONE                                 BIT(11) /* if 1, soft step done irq pending */
#define VOL_LCH_SOFT_STEP_EN                                   BIT(10) /* dac volume left channel soft stepping gain control enable */
#define VOL_LCH_VOLLZCTOEN                                     BIT(9) /* volume left channel zero cross detection time out enable */
#define VOL_LCH_VOLLZCEN                                       BIT(8) /* volume left channel zero cross detection enable */

#define VOL_LCH_VOLL_SHIFT                                     (0) /* volume left channel control; step: 0.375dB */
#define VOL_LCH_VOLL_MASK                                      (0xFF << VOL_LCH_VOLL_SHIFT)
#define VOL_LCH_VOLL(x)                                        ((x) << VOL_LCH_VOLL_SHIFT)

#define VOL_LCH_SOFT_CFG_DEFAULT                               (VOL_LCH_VOLLZCEN | VOL_LCH_VOLLZCTOEN | VOL_LCH_SOFT_STEP_EN)

/***************************************************************************************************
 * VOL_RCH
 */
#define VOL_RCH_VOLR_IRQ_EN                                    BIT(21) /* if enable, when finishing soft step would trigger IRQ */
#define VOL_RCH_TO_CNT_SHIFT                                   (20) /* TO_CNT configuration */
#define VOL_RCH_TO_CNT(x)                                      ((x) << VOL_RCH_TO_CNT_SHIFT)
#define VOL_RCH_TO_CNT_8X                                      VOL_RCH_TO_CNT(0) /* 8x ADJ_CNT */
#define VOL_RCH_TO_CNT_128X                                    VOL_RCH_TO_CNT(1) /* 128x ADJ_CNT */

#define VOL_RCH_ADJ_CNT_SHIFT                                  (12) /* ADJ_CNT which the same as sample rate setting */
#define VOL_RCH_ADJ_CNT_MASK                                   (0xFF << VOL_RCH_ADJ_CNT_SHIFT)
#define VOL_RCH_ADJ_CNT(x)                                     ((x) << VOL_RCH_ADJ_CNT_SHIFT)

#define VOL_RCH_SOFT_STEP_DONE                                 BIT(11) /* if 1, soft step done irq pending */
#define VOL_RCH_SOFT_STEP_EN                                   BIT(10) /* dac volume left channel soft stepping gain control enable */
#define VOL_RCH_VOLRZCTOEN                                     BIT(9) /* volume left channel zero cross detection time out enable */
#define VOL_RCH_VOLRZCEN                                       BIT(8) /* volume left channel zero cross detection enable */

#define VOL_RCH_VOLR_SHIFT                                     (0) /* volume right channel control; step: 0.375dB */
#define VOL_RCH_VOLR_MASK                                      (0xFF << VOL_RCH_VOLR_SHIFT)
#define VOL_RCH_VOLR(x)                                        ((x) << VOL_RCH_VOLR_SHIFT)

#define VOL_RCH_SOFT_CFG_DEFAULT                               (VOL_RCH_VOLRZCEN | VOL_RCH_VOLRZCTOEN | VOL_RCH_SOFT_STEP_EN)

/***************************************************************************************************
 * DAC_ANACTL0
 */
#define DAC_ANACTL0_SW1RN                                      BIT(25) /* RN PA output volume enable */
#define DAC_ANACTL0_SW1RP                                      BIT(24) /* RP PA output volume enable */
#define DAC_ANACTL0_SW1LN                                      BIT(23) /* LN PA output volume enable */
#define DAC_ANACTL0_SW1LP                                      BIT(22) /* LP PA output volume enable */

#define DAC_ANACTL0_PA_GMSEL_SHIFT                             (19) /* PA DFC option gm select */
#define DAC_ANACTL0_PA_GMSEL_MASK                              (0x7 << DAC_ANACTL0_PA_GMSEL_SHIFT)
#define DAC_ANACTL0_PA_GMSEL(x)                                ((x) << DAC_ANACTL0_PA_GMSEL_SHIFT) /* range 0 ~ 7 */

#define DAC_ANACTL0_DFCEN                                      BIT(18) /* PA DFC option enable */

#define DAC_ANACTL0_MODE_SETTING_SHIFT                         (14) /* PA output mode setting */
#define DAC_ANACTL0_MODE_SETTING_MASK                          (0xF << DAC_ANACTL0_MODE_SETTING_SHIFT)
#define DAC_ANACTL0_MODE_SETTING(x)                            ((x) << DAC_ANACTL0_MODE_SETTING_SHIFT)
#define DAC_ANACTL0_MODE_SETTING_N26                           DAC_ANACTL0_MODE_SETTING(0) /* normal 2.6Vpp */
#define DAC_ANACTL0_MODE_SETTING_N16                           DAC_ANACTL0_MODE_SETTING(1) /* normal 1.6Vpp */
#define DAC_ANACTL0_MODE_SETTING_D26                           DAC_ANACTL0_MODE_SETTING(2) /* direct 2.6Vpp */
#define DAC_ANACTL0_MODE_SETTING_D16                           DAC_ANACTL0_MODE_SETTING(3) /* direct 1.6Vpp */
#define DAC_ANACTL0_MODE_SETTING_F26                           DAC_ANACTL0_MODE_SETTING(4) /* diff 2.6Vpp */
#define DAC_ANACTL0_MODE_SETTING_F16                           DAC_ANACTL0_MODE_SETTING(0xD) /* diff 1.6Vpp */

#define DAC_ANACTL0_PARNOSEN                                   BIT(13) /* LP RN output stage enable */
#define DAC_ANACTL0_PARNEN                                     BIT(12) /* LP RN op enable */
#define DAC_ANACTL0_PARPOSEN                                   BIT(11) /* PA RP output stage enable (VRO) */
#define DAC_ANACTL0_PARPEN                                     BIT(10) /* PA RP op enable(VRO) */
#define DAC_ANACTL0_PALNOSEN                                   BIT(9) /* PA R & LN output stage enable*/
#define DAC_ANACTL0_PALNEN                                     BIT(8) /* PA R & LN op enable */
#define DAC_ANACTL0_PALPOSEN                                   BIT(7) /* PA L & LP output stage enable */
#define DAC_ANACTL0_PALPEN                                     BIT(6) /* PA L & LP op enable */
#define DAC_ANACTL0_ZERODT                                     BIT(5) /* zero data test enable  */
#define DAC_ANACTL0_DAINVENR                                   BIT(4) /* DAC R channel INV enable */
#define DAC_ANACTL0_DAINVENL                                   BIT(3) /* DAC L channel INV enable */
#define DAC_ANACTL0_DAENR                                      BIT(2) /* DAC R channel enable */
#define DAC_ANACTL0_DAENL                                      BIT(1) /* DAC L channel enable */
#define DAC_ANACTL0_BIASEN                                     BIT(0) /* DAC + PA current bias enable */

/***************************************************************************************************
 * DAC_ANACTL1
 */
#define DAC_ANACTL1_OVLS                                       BIT(22) /* PA / VRO over load state */
#define DAC_ANACTL1_OVDTEN                                     BIT(21) /* PA / VRO output over load detect enable */
#define DAC_ANACTL1_SMCCKS_SHIFT                               (19) /* soft mute control clock select */
#define DAC_ANACTL1_SMCCKS_MASK                                (0x3 << DAC_ANACTL1_SMCCKS_SHIFT)
#define DAC_ANACTL1_SMCCKS(x)                                  ((x) << DAC_ANACTL1_SMCCKS_SHIFT)
#define DAC_ANACTL1_SMCCKS_250HZ                               DAC_ANACTL1_SMCCKS(0)
#define DAC_ANACTL1_SMCCKS_1KHZ                                DAC_ANACTL1_SMCCKS(1)
#define DAC_ANACTL1_SMCCKS_8KHZ                                DAC_ANACTL1_SMCCKS(2)
#define DAC_ANACTL1_SMCCKS_32KHZ                               DAC_ANACTL1_SMCCKS(3)

#define DAC_ANACTL1_DPBMRP                                     BIT(18) /* DAC RP playback mute enable */
#define DAC_ANACTL1_DPBMLN                                     BIT(17) /* DAC LN playback mute enable */
#define DAC_ANACTL1_DPBMLP                                     BIT(16) /* DAC LP playback mute enable */
#define DAC_ANACTL1_SMCEN                                      BIT(15) /* soft mute control enable */
#define DAC_ANACTL1_ATPSW2RP                                   BIT(14) /* switch 2 enable for RP channel PA */
#define DAC_ANACTL1_ATPSW2LN                                   BIT(13) /* switch 2 enable for LN channel PA */
#define DAC_ANACTL1_ATPSW2LP                                   BIT(12) /* switch 2 enable for LP channel PA */
#define DAC_ANACTL1_BCDISCH_RP                                 BIT(11) /* block cap discharge enable for RP PA */
#define DAC_ANACTL1_BCDISCH_LN                                 BIT(10) /* block cap discharge enable for LN PA */
#define DAC_ANACTL1_BCDISCH_LP                                 BIT(9)  /* block cap discharge enable for LP PA */
#define DAC_ANACTL1_ATPRC2EN_RP                                BIT(8) /* antipope ramp connect 2 enable for RP PA */
#define DAC_ANACTL1_ATPRC2EN_LN                                BIT(7) /* antipope ramp connect 2 enable for LN PA */
#define DAC_ANACTL1_ATPRC2EN_LP                                BIT(6) /* antipope ramp connect 2 enable for LP PA */
#define DAC_ANACTL1_ATPRCEN_RP                                 BIT(5) /* antipope ramp connect enable for RP PA */
#define DAC_ANACTL1_ATPRCEN_LN                                 BIT(4) /* antipope ramp connect enable for LN PA */
#define DAC_ANACTL1_ATPRCEN_LP                                 BIT(3) /* antipope ramp connect enable for LP PA */
#define DAC_ANACTL1_LP2RPEN                                    BIT(2) /* loop2 enable for RP PA */
#define DAC_ANACTL1_LP2LNEN                                    BIT(1) /* loop2 enable for LN PA */
#define DAC_ANACTL1_LP2LPEN                                    BIT(0) /* loop2 enable for LP PA */

/***************************************************************************************************
 * DAC_BIAS
 */
#define DAC_BIAS_BIASCTRLH_SHIFT                               (28) /* A11 BIAS curent select */
#define DAC_BIAS_BIASCTRLH_MASK                                (0x3 << DAC_BIAS_BIASCTRLH_SHIFT)
#define DAC_BIAS_BIASCTRLH(x)                                  ((x) << DAC_BIAS_BIASCTRLH_SHIFT)

#define DAC_BIAS_PARIQS_SHIFT                                  (25) /* PARP/RN output stage current control */
#define DAC_BIAS_PARIQS_MASK                                   (0x7 << DAC_BIAS_PARIQS_SHIFT)
#define DAC_BIAS_PARIQS(x)                                     ((x) << DAC_BIAS_PARIQS_SHIFT)
#define DAC_BIAS_PARIQS_SMALL                                  DAC_BIAS_PARIQS(0)
#define DAC_BIAS_PARIQS_LARGE                                  DAC_BIAS_PARIQS(7)

#define DAC_BIAS_PALIQS_SHIFT                                  (22)
#define DAC_BIAS_PALIQS_MASK                                   (0x7 << DAC_BIAS_PALIQS_SHIFT)
#define DAC_BIAS_PALIQS(x)                                     ((x) << DAC_BIAS_PALIQS_MASK)
#define DAC_BIAS_PALIQS_SMALL                                  DAC_BIAS_PALIQS(0)
#define DAC_BIAS_PALIQS_LARGE                                  DAC_BIAS_PALIQS(7)

#define DAC_BIAS_OPINVIQS_SHIFT                                (19) /* DAC INV OP output stage driver and current control */
#define DAC_BIAS_OPINVIQS_MASK                                 (0x7 << DAC_BIAS_OPINVIQS_SHIFT)

#define DAC_BIAS_OPDAIQS_SHIFT                                 (16) /* DAC I2V OP output stage driver control */
#define DAC_BIAS_OPDAIQS_MASK                                  (0x7 << DAC_BIAS_OPDAIQS_SHIFT)

#define DAC_BIAS_IPARP_SHIFT                                   (13) /* PA RP OP bias control */
#define DAC_BIAS_IPARP_MASK                                    (0x7 << DAC_BIAS_IPARP_SHIFT)

#define DAC_BIAS_IPA_SHIFT                                     (10) /* PA LP LN RN OP bias control */
#define DAC_BIAS_IPA_MASK                                      (0x7 << DAC_BIAS_IPA_SHIFT)

#define DAC_BIAS_IOPCM1_SHIFT                                  (8) /* CM1 op bias control */
#define DAC_BIAS_IOPCM1_MASK                                   (0x3 << DAC_BIAS_IOPCM1_SHIFT)

#define DAC_BIAS_IOPVB_SHIFT                                   (6) /* VB op bias control */
#define DAC_BIAS_IOPVB_MASK                                    (0x3 << DAC_BIAS_IOPVB_SHIFT)

#define DAC_BIAS_OPINV_IB_SHIFT                                (3) /* DAC INV op bias control */
#define DAC_BIAS_OPINV_IB_MASK                                 (0x7 << DAC_BIAS_OPINV_IB_SHIFT)

#define DAC_BIAS_OPI2V_IB_SHIFT                                (0) /* DAC I2V OP bias control */
#define DAC_BIAS_OPI2V_IB_MASK                                 (0x7 << DAC_BIAS_OPI2V_IB_SHIFT)

/***************************************************************************************************
 * PCM_BUF_CNT
 */
#define PCM_BUF_CNT_IP                                         BIT(18) /* DAC sample counter overflow irq pending */
#define PCM_BUF_CNT_IE                                         BIT(17) /* DAC sample counter overflow irq enable */
#define PCM_BUF_CNT_EN                                         BIT(16) /* DAC sample counter enable */
#define PCM_BUF_CNT_CNT_SHIFT                                  (0)
#define PCM_BUF_CNT_CNT_MASK                                   (0xFFFF << PCM_BUF_CNT_CNT_SHIFT)

/***************************************************************************************************
 * DAC FEATURES CONGIURATION
 */

/**
  * If enable the DAC_AUTO_MUTE_EN will has small noise when hw auto mute channel.
  * TODO: open soft mute function DAC_ANALOG1[20:19] and DAC_ANALOG1[15] when DAC_AUTO_MUTE_EN enabled.
  */
//#define DAC_AUTO_MUTE_EN
//#define DAC_SDM_MUTE_EN

#define DAC_OSR_DEFAULT                                        (DAC_OSR_64X)

#ifdef CONFIG_DAC_AUTOMUTE_COUNT
#define DAC_AUTOMUTE_CNT_DEFAULT                               (CONFIG_DAC_AUTOMUTE_COUNT)
#else
#define DAC_AUTOMUTE_CNT_DEFAULT                               (DAC_AUTOMUTE_2048)
#endif

#define DAC_SDM_CNT_DEFAULT                                    (DAC_SDM_MUTE_256)
#define DAC_SDM_THRESHOLD                                      (DAC_SDM_HIGH_16BITS)

#define DAC_VOL_TO_CNT_DEFAULT                                 (0)
#define DAC_VOL_SET_TIMEOUT_MS                                 (5000)

#define DAC_FIFO_LEVEL                                         (8)

#define DAC_PCMBUF_MAX_CNT                                     (0x800)

#define DAC_PCMBUF_DEFAULT_IRQ                                 (PCM_BUF_CTL_PCMBHEIE)

#define DAC_PCMBUF_WAIT_TIMEOUT_MS                             (65) /* PCMBUF 2k samples spends 62.5ms in 16Kfs */

#define VOL_MUTE_MIN_DB                                        (-800000)

#define VOL_DB_TO_INDEX(x)                                     (((x) + 374) / 375)
#define VOL_INDEX_TO_DB(x)                                     ((x) * 375)

#define DAC_SAMPLE_CNT_IRQ_SYNC_US                             (100)
#define DAC_SAMPLE_CNT_IRQ_TIMEOUT_MS                          (350) /* 65536 samples in 96K stereo mode spends 341ms */

#define DAC_SINGLE_END_BIAS_DEFAULT                            (0x26c24964) /* This bias is dedicated to deal with the difference ground noise between the left and right channels */
#define DAC_VRO_BIAS_DEFAULT                                   DAC_SINGLE_END_BIAS_DEFAULT /* The same as single-end mode */
#define DAC_DIFF_BIAS_DEFAULT                                  DAC_SINGLE_END_BIAS_DEFAULT /* The same as single-end mode */

/*
 * @struct acts_audio_dac
 * @brief DAC controller hardware register
 */
struct acts_audio_dac {
	volatile u32_t digctl; /* DAC digital and control */
	volatile u32_t fifoctl; /* DAC FIFO control */
	volatile u32_t stat; /* DAC state */
	volatile u32_t fifodat; /* DAC FIFO data */
	volatile u32_t pcm_buf_ctl; /* PCM buffer control */
	volatile u32_t pcm_buf_stat; /* PCM buffer state */
	volatile u32_t pcm_buf_thres_he; /* threshold to generate half empty signal */
	volatile u32_t pcm_buf_thres_hf; /* threshold to generate half full signal */
	volatile u32_t vol_lch; /* volume left channel control */
	volatile u32_t vol_rch; /* volume right channel control */
	volatile u32_t anactl0; /* DAC analog control register 0 */
	volatile u32_t anactl1; /* DAC analog control register 1 */
	volatile u32_t bias; /* DAC bias control */
	volatile u32_t pcm_buf_cnt; /* PA volume control */
};

/*
 * struct dac_channel_info
 * @brief The DAC channel dynamic information
 */
struct dac_dynamic_info {
	int (*callback)(void *cb_data, u32_t reason); /* PCM Buffer IRQs callback */
	void *cb_data; /* callback user data */
	u8_t sample_rate; /* The sample rate setting refer to enum audio_sr_sel_e */
	u32_t fifo_cnt; /* DAC FIFO hardware counter max value is 0xFFFF */
	u32_t fifo_cnt_timestamp; /* Record the timestamp of DAC FIFO counter overflow irq */
	u8_t dac_fifo_busy : 1; /* DAC FIFO is busy as gotten by I2STX or SPDIFTX */
	u8_t vol_set_mute : 1; /* The flag of the volume setting less than #VOL_MUTE_MIN_DB event*/
};

/*
 * struct dac_private_data
 * @brief Physical DAC device private data
 */
struct dac_private_data {
	struct device *parent_dev;        /* Indicates the parent audio device */
#ifdef DAC_VOL_SOFT_STEP_IRQ
	struct k_sem voll_done;           /* left volume set done */
	struct k_sem volr_done;           /* right volume set done */
#endif
	struct dac_dynamic_info info;     /* dynamic information */
};

/*
 * enum a_dac_osr_e
 * @brief DAC over sample rate
 */
typedef enum {
	DAC_OSR_32X = 0,
	DAC_OSR_64X,
	DAC_OSR_128X,
	DAC_OSR_256X
} a_dac_osr_e;

/*
 * enum a_dac_sdmcnt_e
 * @brief SDM(noise detect mute) counter selection
 */
typedef enum {
	DAC_SDM_MUTE_256 = 0,
	DAC_SDM_MUTE_512
} a_dac_sdmcnt_e;

/*
 * enum a_dac_sdmndth_e
 * @brief DAC SDM noise detection threshold
 */
typedef enum {
	DAC_SDM_HIGH_16BITS = 0,
	DAC_SDM_HIGH_15BITS,
	DAC_SDM_HIGH_14BITS,
	DAC_SDM_HIGH_13BITS
} a_dac_sdmndth_e;

/*
 * enum a_dac_mute_cnt_e
 * @brief DAC auto mute counter selection
 */
typedef enum {
	DAC_AUTOMUTE_512 = 0, /* 10ms at 48ksr */
	DAC_AUTOMUTE_1024, /* 20ms at 48ksr */
	DAC_AUTOMUTE_2048, /* 40ms at 48ksr */
	DAC_AUTOMUTE_4096 /* 80ms at 48ksr */
} a_dac_mute_cnt_e;

/*
 * enum a_lr_chl_e
 * @brief Left/Right channel selection
 */
typedef enum {
	LEFT_CHANNEL_SEL = (1 << 0),
	RIGHT_CHANNEL_SEL = (1 << 1)
} a_lr_chl_e;

/*
 * enum a_pa_mode_e
 * @brief PA output mode setting
 * @note If physical layout is DAC_SINGLE_END_LAYOUT, select PA_NORMAL_26VPP/PA_NORMAL_16VPP;
 *       If physical layout is DAC_VRO_LAYOUT, select PA_DIRECT_26VPP/PA_DIRECT_16VPP;
 *       If physical layout is DAC_DIFFERENTIAL_LAYOUT, select PA_DIFF_52VPP/PA_DIFF_16VPP;
 */
typedef enum {
	PA_NORMAL_26VPP = 0,
	PA_NORMAL_16VPP,
	PA_DIRECT_26VPP,
	PA_DIRECT_16VPP,
	PA_DIFF_52VPP,
	PA_DIFF_16VPP = 0xD
} a_pa_mode_e;

static void phy_dac_irq_enable(struct phy_audio_device *dev);
static void phy_dac_irq_disable(struct phy_audio_device *dev);

/*
 * @brief Get the DAC controller base address
 * @param void
 * @return The base address of the DAC controller
 */
static inline struct acts_audio_dac *get_dac_base(void)
{
	return (struct acts_audio_dac *)AUDIO_DAC_REG_BASE;
}

/*
 * @brief Dump the DAC relative registers
 * @param void
 * @return void
 */
static void dac_dump_regs(void)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	struct acts_audio_dac *dac_reg = get_dac_base();

	SYS_LOG_INF("** dac contoller regster **");
	SYS_LOG_INF("             BASE: %08x", (u32_t)dac_reg);
	SYS_LOG_INF("       DAC_DIGCTL: %08x", dac_reg->digctl);
	SYS_LOG_INF("      DAC_FIFOCTL: %08x", dac_reg->fifoctl);
	SYS_LOG_INF("         DAC_STAT: %08x", dac_reg->stat);
	SYS_LOG_INF("     DAC_DAT_FIFO: %08x", dac_reg->fifodat);
	SYS_LOG_INF("      PCM_BUF_CTL: %08x", dac_reg->pcm_buf_ctl);
	SYS_LOG_INF("     PCM_BUF_STAT: %08x", dac_reg->pcm_buf_stat);
	SYS_LOG_INF(" PCM_BUF_THRES_HE: %08x", dac_reg->pcm_buf_thres_he);
	SYS_LOG_INF(" PCM_BUF_THRES_HF: %08x", dac_reg->pcm_buf_thres_hf);
	SYS_LOG_INF("          VOL_LCH: %08x", dac_reg->vol_lch);
	SYS_LOG_INF("          VOL_RCH: %08x", dac_reg->vol_rch);
	SYS_LOG_INF("      DAC_ANALOG0: %08x", dac_reg->anactl0);
	SYS_LOG_INF("      DAC_ANALOG1: %08x", dac_reg->anactl1);
	SYS_LOG_INF("         DAC_BIAS: %08x", dac_reg->bias);
	SYS_LOG_INF("      PCM_BUF_CNT: %08x", dac_reg->pcm_buf_cnt);
	SYS_LOG_INF("    AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
	SYS_LOG_INF("    AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
	SYS_LOG_INF("       CMU_DACCLK: %08x", sys_read32(CMU_DACCLK));
#endif
}

/*
 * @brief Disable the DAC FIFO
 * @param void
 * @return void
 */
static void dac_fifo_disable(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	dac_reg->fifoctl = 0;
	dac_reg->pcm_buf_ctl = 0;
}

/*
 * @brief Check the DAC FIFO is working or not
 * @param dev: the physical audio device
 * @return If true the DAC FIFO is working
 */
static inline bool is_dac_fifo_working(struct phy_audio_device *dev)
{
	struct dac_private_data *priv = (struct dac_private_data *)dev->private;

	if (priv->info.dac_fifo_busy)
		return true;

	return false;
}

/*
 * @brief Check the DAC FIFO is error
 * @param void
 * @return If true the DAC FIFO is error
 */
static bool check_dac_fifo_error(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();

	if (dac_reg->stat & DAC_STAT_FIFO_ER) {
		dac_reg->stat |= DAC_STAT_FIFO_ER;
		return true;
	}

	return false;
}

/*
 * @brief Check and wait the DAC FIFO is empty or not
 * @param void
 * @return void
 */
static void wait_dac_fifo_empty(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();

	while (!(dac_reg->stat & DAC_STAT_DAFEIP))
		;

	/* clear status */
	dac_reg->stat |= DAC_STAT_DAFEIP;
}

/*
 * @brief Enable the DAC FIFO
 * @param sel: select the fifo input source, e.g. CPU or DMA
 * @param width: specify the transfer width when in dma mode
 * @return void
 */
static void dac_fifo_enable(audio_fifouse_sel_e sel, audio_dma_width_e width)
{
	struct acts_audio_dac *dac_reg = get_dac_base();

	if (FIFO_SEL_CPU == sel) {
		dac_reg->fifoctl = (DAC_FIFOCTL_DAFIS_CPU | DAC_FIFOCTL_DAFEIE
							| DAC_FIFOCTL_DAFRT);
	} else if (FIFO_SEL_DMA == sel) {
		dac_reg->fifoctl = (DAC_FIFOCTL_DAFIS_DMA | DAC_FIFOCTL_DAFEDE
							| DAC_FIFOCTL_DAFRT);
		if (DMA_WIDTH_16BITS == width)
			dac_reg->fifoctl |= DAC_FIFOCTL_DACFIFO_DMAWIDTH;
	}
}

/*
 * @brief Get the available length to filled into the PCMBUF
 * @param void
 * @return The available length of the PCMBUF
 */
static u32_t dac_pcmbuf_avail_length(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t key;
	u32_t len;

	key = irq_lock();
	len = (dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) >> PCM_BUF_STAT_PCMBS_SHIFT;
	irq_unlock(key);
	SYS_LOG_DBG("PCMBUF avail length 0x%x", len);
	return len;
}

/*
 * @brief Wait the pcmbuf empty
 * @param void
 * @retval Negative value on error, 0 on success.
 */
static int dac_pcmbuf_wait_empty(u32_t timeout_ms)
{
	u32_t start_time, curr_time;

	start_time = k_uptime_get();
	while (dac_pcmbuf_avail_length() < (DAC_PCMBUF_MAX_CNT - 1)) {
		curr_time = k_uptime_get_32();
		if ((curr_time - start_time) >= timeout_ms) {
			SYS_LOG_ERR("wait pcmbuf empty timeout 0x%x",
				dac_pcmbuf_avail_length());
			return -ETIMEDOUT;
		}
		k_sleep(2);
	}

	/* sleep 1ms for stereo 1 sample output */
	k_sleep(1);

	return 0;
}

/*
 * @brief Enable the DAC digital function
 * @param osr: select the over sample rate(OSR)
 * @param type: choose the stream type
 * @channel_type: channel type selection
 * @return 0 on success, negative errno code on fail
 */
static int dac_digital_enable(a_dac_osr_e osr, audio_ch_mode_e type, u8_t channel_type)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg = dac_reg->digctl;

	reg &= ~(DAC_DIGCTL_ENDITH | DAC_DIGCTL_ENDITH
			| DAC_DIGCTL_OSRDA_MASK | DAC_DIGCTL_INTFRS_MASK
			| DAC_DIGCTL_DACHNUM | DAC_DIGCTL_MULT_DEVICE_MASK);

	if ((STEREO_MODE != type) && (MONO_MODE != type))
		return -EINVAL;

	/* OSRDA and INTFRS set as multiple of 8 normally */
	reg |= (DAC_DIGCTL_DDEN | DAC_DIGCTL_ENDITH
		| DAC_DIGCTL_OSRDA(osr) | DAC_DIGCTL_INTFRS(osr + 2));

	if (MONO_MODE == type)
		reg |= DAC_DIGCTL_DACHNUM;


#ifdef CONFIG_DAC_LR_MIX
		if (STEREO_MODE != type) {
			SYS_LOG_ERR("MIX function can only be used in mono mode");
			return -EPERM;
		}
		reg |= DAC_DIGCTL_DALRMIX;
#endif

	if ((channel_type & AUDIO_CHANNEL_I2STX)
		&& (channel_type & AUDIO_CHANNEL_SPDIFTX)) {
		SYS_LOG_INF("Enable DAC linkage with I2STX and SPDIFTX");
		reg |= DAC_DIGCTL_MULT_DEVICE(3);
	} else if (channel_type & AUDIO_CHANNEL_I2STX) {
		SYS_LOG_INF("Enable DAC linkage with I2STX");
		reg |= DAC_DIGCTL_MULT_DEVICE(1);
	} else if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
		SYS_LOG_INF("Enable DAC linkage with SPDIFTX");
		reg |= DAC_DIGCTL_MULT_DEVICE(2);
	}

	dac_reg->digctl = reg;

	return 0;
}

/*
 * @brief Claim that there is spdif(128fs) under linkage mode.
 * @return void
 */
static void dac_digital_claim_128fs(bool en)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	if (en)
		dac_reg->digctl |= (DAC_DIGCTL_AUDIO_128FS_256FS16 | DAC_DIGCTL_DDEN);
	else
		dac_reg->digctl &= ~DAC_DIGCTL_AUDIO_128FS_256FS16; /* don't disable digital-en until fifo put */
}

/*
 * @brief Disable the DAC digital function
 * @param void
 * @return void
 */
static void dac_digital_disable(void)
{
	/*Here is not allow disable DAC digital part to avoid the noise during open and close*/
	struct acts_audio_dac *dac_reg = get_dac_base();
	dac_reg->digctl &= ~DAC_DIGCTL_MULT_DEVICE_MASK;
}

/*
 * @brief PCM BUF0(RAM4) clock enable
 * @param void
 * @return void
 */
static void dac_pcmbuf0_clk_enable(void)
{
	u32_t reg;

	/* enable the PCMBUF0 clock */
	reg = sys_read32(CMU_MEMCLKEN);
	reg |= (1 << CMU_MEMCLKEN_RAM4CLKEN);
	sys_write32(reg, CMU_MEMCLKEN);

	/* select the clock of PCMBUF0 to be DAC_CLK*/
	reg = sys_read32(CMU_MEMCLKSRC);
	reg &= ~(CMU_MEMCLKSRC_RAM4CLKSRC_MASK);
	reg |= (2 << CMU_MEMCLKSRC_RAM4CLKSRC_SHIFT);
	sys_write32(reg, CMU_MEMCLKSRC);
}

/*
 * @brief PCM BUF1(RAM1) clock enable and select the DAC_CLK
 * @param void
 * @return void
 */
static void dac_pcmbuf1_clk_enable(void)
{
	u32_t reg;

	/* enable the PCMBUF1 clock */
	reg = sys_read32(CMU_MEMCLKEN);
	reg |= (1 << CMU_MEMCLKEN_PCMRAM1CLKEN);
	sys_write32(reg, CMU_MEMCLKEN);

	/* select the clock of PCMBUF1 to be DAC_CLK*/
	reg = sys_read32(CMU_MEMCLKSRC);
	reg |= (1 << CMU_MEMCLKSRC_PCMRAM1CLKSRC);
	sys_write32(reg, CMU_MEMCLKSRC);
}

/*
 * @brief PCM BUF1(RAM1) clock disable and select CPU CLK
 * @param flag: indicates the requirement to disable pcmbuf1 clock
 * @return void
 */
static void dac_pcmbuf1_clk_disable(bool flag)
{
	u32_t reg;

	if (flag) {
		/* disable the PCMBUF1 clock */
		reg = sys_read32(CMU_MEMCLKEN);
		reg &= ~(1 << CMU_MEMCLKEN_PCMRAM1CLKEN);
		sys_write32(reg, CMU_MEMCLKEN);
	}

	/* select the clock of PCMBUF1 to be CPU_CLK*/
	reg = sys_read32(CMU_MEMCLKSRC);
	reg &= ~(1 << CMU_MEMCLKSRC_PCMRAM1CLKSRC);
	sys_write32(reg, CMU_MEMCLKSRC);
}

/*
 * @brief PCM BUF configuration
 * @param width: the PCM BUF data width
 * @return 0 on success, negative errno code on fail
 */
static int dac_pcmbuf_config(audio_ch_width_e width)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg = dac_reg->pcm_buf_ctl;

#if (CONFIG_PCMBUF_HE_THRESHOLD >= DAC_PCMBUF_MAX_CNT)
#error "Error on PCMBUF HE threshold setting"
#endif

#if (CONFIG_PCMBUF_HF_THRESHOLD >= DAC_PCMBUF_MAX_CNT)
#error "Error on PCMBUF HF threshold setting"
#endif

	dac_pcmbuf0_clk_enable();

	/* PCMBUF1 only used when 24 bits data width */
	if (CHANNEL_WIDTH_16BITS == width) {
		dac_pcmbuf1_clk_disable(false); /* only switch the pcmbuf1 to be CPU_CLK */
		reg &= ~PCM_BUF_CTL_DATWL_MASK;
		reg |= PCM_BUF_CTL_DATWL_16;
	} else if (CHANNEL_WIDTH_24BITS == width) {
		dac_pcmbuf1_clk_enable();
		reg &= ~PCM_BUF_CTL_DATWL_MASK;
		reg |= PCM_BUF_CTL_DATWL_24;
	} else {
		SYS_LOG_ERR("invalid pcmbuf width %d", width);
		return -EINVAL;
	}

	reg &= ~PCM_BUF_CTL_IRQ_MASK;

	/* Enable PCMBUF IRQs */
	reg |= DAC_PCMBUF_DEFAULT_IRQ;

	dac_reg->pcm_buf_ctl = reg;

	dac_reg->pcm_buf_thres_he = CONFIG_PCMBUF_HE_THRESHOLD;
	dac_reg->pcm_buf_thres_hf = CONFIG_PCMBUF_HF_THRESHOLD;

	/* Clean all pcm buf irqs pending */
	dac_reg->pcm_buf_stat |= PCM_BUF_STAT_IRQ_MASK;

	SYS_LOG_DBG("ctl:0x%x, thres_he:0x%x thres_hf:0x%x",
				dac_reg->pcm_buf_ctl, dac_reg->pcm_buf_thres_he,
				dac_reg->pcm_buf_thres_hf);

	return 0;
}

static int dac_pcmbuf_threshold_update(dac_threshold_setting_t *thres)
{
	struct acts_audio_dac *dac_reg = get_dac_base();

	if (!thres)
		return -EINVAL;

	if ((thres->hf_thres >= DAC_PCMBUF_MAX_CNT)
		|| (thres->he_thres > thres->hf_thres)) {
		SYS_LOG_ERR("Invalid threshold hf:%d he:%d",
			thres->hf_thres, thres->he_thres);
		return -ENOEXEC;
	}

	dac_reg->pcm_buf_thres_he = thres->he_thres;
	dac_reg->pcm_buf_thres_hf = thres->hf_thres;

	SYS_LOG_INF("new dac threshold => he:0x%x hf:0x%x",
		dac_reg->pcm_buf_thres_he, dac_reg->pcm_buf_thres_hf);

	return 0;
}

/*
 * @brief Enable the function of the pcm buff counter
 * @return void
 */
static void dac_pcmbuf_counter_enable(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_EN;

	/* By default to enbale pcm buf counter overflow IRQ */
	dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IE;

	/* clear sample counter irq pending */
	dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;
	SYS_LOG_INF("DAC FIFO counter function enable");
}

/*
 * @brief Disable the function of the pcm buff counter
 * @param void
 * @return void
 */
static void dac_pcmbuf_counter_disable(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();

	dac_reg->pcm_buf_cnt &= ~(PCM_BUF_CNT_EN | PCM_BUF_CNT_IE);
}

/*
 * @brief Reset the function of the pcm buff counter
 * @param flag: enable the sample counter overflow irq
 * @return void
 */
static void dac_pcmbuf_counter_reset(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();

	dac_reg->pcm_buf_cnt &= ~(PCM_BUF_CNT_EN | PCM_BUF_CNT_IE);

	dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_EN;

	dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IE;
}

/*
 * @brief  Read the DAC PCM BUF sample counter
 * @param  void
 * @return the sample counter of DAC PCM BUF
 */
static u32_t dac_read_pcmbuf_counter(void)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	return dac_reg->pcm_buf_cnt & PCM_BUF_CNT_CNT_MASK;
}

/*
 * @brief DAC left/right channel digital volume setting
 * @param lr_sel: enable the left or right channel
 * @param left_v: volume of left channel
 * @param right_v: volume of right channel
 * @return void
 */
static void dac_digital_volume_set(a_lr_chl_e lr_sel, u8_t left_v, u8_t right_v)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg_l, reg_r;

	if (lr_sel & LEFT_CHANNEL_SEL) {
		reg_l = dac_reg->vol_lch;
		reg_l &= ~VOL_LCH_VOLL_MASK;
		reg_l |= VOL_LCH_VOLL(left_v);
		dac_reg->vol_lch = reg_l;
		SYS_LOG_INF("Left volume set: 0x%x", left_v);
	}

	if (lr_sel & RIGHT_CHANNEL_SEL) {
		reg_r = dac_reg->vol_rch;
		reg_r &= ~VOL_RCH_VOLR_MASK;
		reg_r |= VOL_RCH_VOLR(right_v);
		dac_reg->vol_rch = reg_r;
		SYS_LOG_INF("Right volume set: 0x%x", right_v);
	}
}

/*
 * @brief Get DAC left/right channel digital volume
 * @param lr_sel: select the left or right channel
 * @return The left or right volume value
 */
static u16_t dac_digital_volume_get(a_lr_chl_e lr_sel)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u8_t vol;

	if (lr_sel & LEFT_CHANNEL_SEL)
		vol = dac_reg->vol_lch & VOL_LCH_VOLL_MASK;
	else
		vol = dac_reg->vol_rch & VOL_RCH_VOLR_MASK;

	return vol;
}

/*
 * @brief DAC left channel volume soft step config
 * @param adj_cnt: soft stepping gain control sample which the same as the sample rate.
 * @param to_cnt: 8X of ADJ_CNT or 128 of ADJ_CNT
 * @return void
 */
static void dac_volume_left_soft_cfg(u8_t adj_cnt, u8_t to_cnt, bool irq_flag)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg;

	/* Clear soft step done IRQ pending */
	if (dac_reg->vol_lch & VOL_LCH_SOFT_STEP_DONE)
		dac_reg->vol_lch |= VOL_LCH_SOFT_STEP_DONE;

	reg = dac_reg->vol_lch;
	/* Clear ADJ_CNT, TO_CNT, VOLL_IRQ_EN */
	reg &= ~(0x3FF << VOL_LCH_ADJ_CNT_SHIFT);
	reg |= VOL_LCH_ADJ_CNT(adj_cnt);

	if (to_cnt)
		reg |= VOL_LCH_TO_CNT_128X;
	else
		reg |= VOL_LCH_TO_CNT_8X;

	if (irq_flag)
		reg |= VOL_LCH_VOLL_IRQ_EN;
	else
		reg &= ~VOL_LCH_VOLL_IRQ_EN;

	reg |= VOL_LCH_SOFT_CFG_DEFAULT;

	dac_reg->vol_lch = reg;
}

/*
 * @brief DAC right channel volume soft step config
 * @param adj_cnt: soft stepping gain control sample which the same as the sample rate.
 * @param to_cnt: 8X of ADJ_CNT or 128 of ADJ_CNT
 * @return void
 */
static void dac_volume_right_soft_cfg(u8_t adj_cnt, u8_t to_cnt, bool irq_flag)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg;

	/* Clear soft step done IRQ pending */
	if (dac_reg->vol_rch & VOL_RCH_SOFT_STEP_DONE)
		dac_reg->vol_rch |= VOL_RCH_SOFT_STEP_DONE;

	reg = dac_reg->vol_rch;
	/* Clear ADJ_CNT, TO_CNT, VOLL_IRQ_EN */
	reg &= ~(0x3FF << VOL_RCH_ADJ_CNT_SHIFT);
	reg |= VOL_RCH_ADJ_CNT(adj_cnt);

	if (to_cnt)
		reg |= VOL_RCH_TO_CNT_128X;
	else
		reg |= VOL_RCH_TO_CNT_8X;

	if (irq_flag)
		reg |= VOL_RCH_VOLR_IRQ_EN;
	else
		reg &= ~VOL_RCH_VOLR_IRQ_EN;

	reg |= VOL_RCH_SOFT_CFG_DEFAULT;

	dac_reg->vol_rch = reg;
}

/*
 * @brief DAC SDM(noise detect mute) configuration
 * @param cnt: SDM counter such as 256 or 512 selection
 * @param thres: The threshold of SDM noise dectection
 * @return void
 */
#ifdef DAC_SDM_MUTE_EN
static void dac_sdm_mute_cfg(a_dac_sdmcnt_e cnt, a_dac_sdmndth_e thres)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg = dac_reg->digctl;

	reg &= ~(DAC_DIGCTL_SDMNDTH_MASK | DAC_DIGCTL_SDMCNT);
	reg |= DAC_DIGCTL_SDMNDTH(thres);

	if (DAC_SDM_MUTE_512 == cnt)
		reg |= DAC_DIGCTL_SDMCNT;

	/* Reset SDM after has detected noise
	 * NOTE: When sample rate are 8k/11k/12k/16k shall enable this bit
	 */
	reg |= DAC_DIGCTL_SDMREEN;
	dac_reg->digctl = reg;
	SYS_LOG_INF("DAC SDM mute function enable");
}
#endif

/*
 * @brief DAC auto mute configuration
 * @param cnt: The auto mute counter selection
 * @return void
 */
#ifdef DAC_AUTO_MUTE_EN
static void dac_automute_cfg(a_dac_mute_cnt_e cnt)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg = dac_reg->digctl;
	reg &= ~DAC_DIGCTL_AMCNT_MASK;
	reg |= DAC_DIGCTL_AMCNT(cnt);
	/* Auto mute enable */
	reg |= DAC_DIGCTL_AMEN;
	dac_reg->digctl = reg;
	SYS_LOG_INF("DAC auto mute function enable");
}
#endif

/*
 * @brief DAC enable mute or unmute control.
 * @param mute_en: The flag of mute or unmute enable.
 * @return void
 */
static void dac_mute_control(bool mute_en)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	if (mute_en)
		dac_reg->anactl0 |= DAC_ANACTL0_ZERODT; /* DAC L/R channel will output zero data */
	else
		dac_reg->anactl0 &= ~DAC_ANACTL0_ZERODT;
}

/*
 * @brief DAC analog configuration when single end physical connection
 * @param lr_sel: enable the left or right channel refer to a_lr_chl_e
 * @return 0 on success, negative errno code on fail
 */
#ifdef CONFIG_DAC_SINGLE_END_LAYOUT
static int dac_analog_single_end_cfg(u8_t lr_sel)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg0, reg1;
	a_pa_mode_e pa_mode;

	reg1 = dac_reg->anactl1;
	/* disable DAC LP/LN/RP playback mute */
	reg1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN | DAC_ANACTL1_DPBMRP);

	/* PA DFC options [18:21] need to keep as default value */
	reg0 = dac_reg->anactl0;

	/* DAC + PA current bias enable */
	reg0 |= DAC_ANACTL0_BIASEN;

#if CONFIG_PA_OUTPUT_MODE == 0
	pa_mode = PA_NORMAL_26VPP;
#else
	pa_mode = PA_NORMAL_16VPP;
#endif

	reg0 &= ~DAC_ANACTL0_MODE_SETTING_MASK;
	reg0 |= DAC_ANACTL0_MODE_SETTING(pa_mode);

	if (lr_sel & LEFT_CHANNEL_SEL) {
		/* PA L & LP output stage enable */
		reg0 |= DAC_ANACTL0_PALPOSEN;
		/* PA L & LP op enable */
		reg0 |= DAC_ANACTL0_PALPEN;
		/* DAC L channel INV enable */
		reg0 |= DAC_ANACTL0_DAINVENL;
		/* DAL L channel enable */
		reg0 |= DAC_ANACTL0_DAENL;
		/* PA LP output volume select enable */
		reg0 |= DAC_ANACTL0_SW1LP;
		/* DAC LP playback mute enable */
		reg1 |= DAC_ANACTL1_DPBMLP;
	}

	if (lr_sel & RIGHT_CHANNEL_SEL) {
		/* PA R & LN output stage enable */
		reg0 |= DAC_ANACTL0_PALNOSEN;
		/* PA R & LN op enable */
		reg0 |= DAC_ANACTL0_PALNEN;
		/* DAC R channel INV enable */
		reg0 |= DAC_ANACTL0_DAINVENR;
		/* DAL R channel enable */
		reg0 |= DAC_ANACTL0_DAENR;
		/* PA LN output volume select enable */
		reg0 |= DAC_ANACTL0_SW1LN;
		/* DAC LN playback mute enable */
		reg1 |= DAC_ANACTL1_DPBMLN;
	}

	dac_reg->bias = DAC_SINGLE_END_BIAS_DEFAULT;

	dac_reg->anactl0 = reg0;
	dac_reg->anactl1 = reg1;

	return 0;
}
#endif

/*
 * @brief DAC analog configuration when VRO physical connection
 * @param lr_sel: enable the left or right channel; refer to enum a_lr_chl_e
 * @return 0 on success, negative errno code on fail
 */
#ifdef CONFIG_DAC_VRO_LAYOUT
static int dac_analog_vro_cfg(u8_t lr_sel)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg0, reg1;
	a_pa_mode_e pa_mode;

	reg1 = dac_reg->anactl1;
	/* disable DAC LP/LN/RP playback mute */
	reg1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN | DAC_ANACTL1_DPBMRP);

	/* PA DFC options [18:21] need to keep as default value */
	reg0 = dac_reg->anactl0;

	/* DAC + PA current bias enable */
	reg0 |= DAC_ANACTL0_BIASEN;
	/* PA RP output stage and op enable (VRO) */
	reg0 |= (DAC_ANACTL0_PARPEN | DAC_ANACTL0_PARPOSEN);

#if CONFIG_PA_OUTPUT_MODE == 0
	pa_mode = PA_DIRECT_26VPP;
#else
	pa_mode = PA_DIRECT_16VPP;
#endif

	reg0 &= ~DAC_ANACTL0_MODE_SETTING_MASK;
	reg0 |= DAC_ANACTL0_MODE_SETTING(pa_mode);

	if (lr_sel & LEFT_CHANNEL_SEL) {
		/* PA L & LP output stage enable */
		reg0 |= DAC_ANACTL0_PALPOSEN;
		/* PA L & LP op enable */
		reg0 |= DAC_ANACTL0_PALPEN;
		/* DAC L channel INV enable */
		reg0 |= DAC_ANACTL0_DAINVENL;
		/* DAL L channel enable */
		reg0 |= DAC_ANACTL0_DAENL;
		/* PA LP output volume select enable */
		reg0 |= DAC_ANACTL0_SW1LP;
		/* DAC LP playback mute enable */
		reg1 |= DAC_ANACTL1_DPBMLP;
	}

	if (lr_sel & RIGHT_CHANNEL_SEL) {
		/* PA R & LN output stage enable */
		reg0 |= DAC_ANACTL0_PALNOSEN;
		/* PA R & LN op enable */
		reg0 |= DAC_ANACTL0_PALNEN;
		/* DAC R channel INV enable */
		reg0 |= DAC_ANACTL0_DAINVENR;
		/* DAL R channel enable */
		reg0 |= DAC_ANACTL0_DAENR;
		/* PA LN output volume select enable */
		reg0 |= DAC_ANACTL0_SW1LN;
		/* DAC LN playback mute enable */
		reg1 |= DAC_ANACTL1_DPBMLN;
	}

	dac_reg->bias = DAC_VRO_BIAS_DEFAULT;

	dac_reg->anactl0 = reg0;
	dac_reg->anactl1 = reg1;

	return 0;
}
#endif

/*
 * @brief DAC analog configuration when differential physical connection
 * @param lr_sel: enable the left or right channel; refer to enum a_lr_chl_e
 * @return 0 on success, negative errno code on fail
 */
#ifdef CONFIG_DAC_DIFFERENCTIAL_LAYOUT
static int dac_analog_diff_cfg(u8_t lr_sel)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg0, reg1;
	a_pa_mode_e pa_mode;

	reg1 = dac_reg->anactl1;
	/* disable DAC LP/LN/RP playback mute */
	reg1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN | DAC_ANACTL1_DPBMRP);

	/* PA DFC options [18:21] need to keep as default value */
	reg0 = dac_reg->anactl0;

	/* DAC + PA current bias enable */
	reg0 |= DAC_ANACTL0_BIASEN;

#if CONFIG_PA_OUTPUT_MODE == 0
	pa_mode = PA_DIFF_52VPP;
#else
	pa_mode = PA_DIFF_16VPP;
#endif

	reg0 &= ~DAC_ANACTL0_MODE_SETTING_MASK;
	reg0 |= DAC_ANACTL0_MODE_SETTING(pa_mode);

	if (lr_sel & LEFT_CHANNEL_SEL) {
		/* PA L & LP / R & LN output stage and op enable */
		reg0 |= (DAC_ANACTL0_PALPEN | DAC_ANACTL0_PALPOSEN
				| DAC_ANACTL0_PALNEN | DAC_ANACTL0_PALNOSEN);
		/* DAL L channel enable */
		reg0 |= DAC_ANACTL0_DAENL;
		/* PA LP output volume ln/lp select enable */
		reg0 |= (DAC_ANACTL0_SW1LP | DAC_ANACTL0_SW1LN);
		/* DAC LP / LN playback mute enable */
		reg1 |= (DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN);
	}

	if (lr_sel & RIGHT_CHANNEL_SEL) {
		/* PA RP / RN output stage and op enable */
		reg0 |= (DAC_ANACTL0_PARPEN | DAC_ANACTL0_PARPOSEN
				| DAC_ANACTL0_PARNEN | DAC_ANACTL0_PARNOSEN);
		/* DAL R channel enable */
		reg0 |= DAC_ANACTL0_DAENR;
		/* PA LP output volume rn/rp select enable */
		reg0 |= (DAC_ANACTL0_SW1RP | DAC_ANACTL0_SW1RN);
		/* DAC RP playback mute enable */
		reg1 |= DAC_ANACTL1_DPBMRP;
	}

	dac_reg->bias = DAC_DIFF_BIAS_DEFAULT;

	dac_reg->anactl0 = reg0;
	dac_reg->anactl1 = reg1;

	return 0;

}
#endif

/*
 * @brief ADC L/R channel loopback to DAC enable
 * @param lr_sel: enable the left or right channel refer to a_lr_chl_e
 * @param lr_mix:  ADCL channel mix ADCR channel enable
 * @return 0 on success, negative errno code on failure
 */
#ifdef DAC_LOOPBACK_EN
static void dac_loopback_cfg(u8_t lr_sel, bool lr_mix)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	u32_t reg = dac_reg->digctl;

	/* Clear  L/R mix, ADCR loopback to DAC, ADCL loopback to DAC */
	reg &= ~(DAC_DIGCTL_ADC_LRMIX | DAC_DIGCTL_AD2DALPEN_R | DAC_DIGCTL_AD2DALPEN_L);

	if (lr_sel & LEFT_CHANNEL_SEL)
		reg |= DAC_DIGCTL_AD2DALPEN_L;

	if (lr_sel & RIGHT_CHANNEL_SEL)
		reg |= DAC_DIGCTL_AD2DALPEN_R;

	if (lr_mix)
		reg |= DAC_DIGCTL_ADC_LRMIX;
	else
		reg &= ~DAC_DIGCTL_ADC_LRMIX;

	dac_reg->digctl = reg;

	SYS_LOG_INF("DAC loopback lr_mix status:%d", lr_mix);
}
#endif

/*
 * @brief Translate the volume in db format to DAC hardware volume level.
 * @param vol: the volume of left channel in db
 * @retval the DAC hardware volume level
 */
static u16_t dac_volume_db_to_level(s32_t vol)
{
	u32_t level = 0;
	if (vol < 0) {
		vol = -vol;
		level = VOL_DB_TO_INDEX(vol);
		if (level > 0xBF)
			level = 0;
		else
			level = 0xBF - level;
	} else {
		level = VOL_DB_TO_INDEX(vol);
		if (level > 0x40)
			level = 0xFF;
		else
			level = 0xBF + level;
	}
	return level;
}

/*
 * @brief Translate the volume in db format to DAC hardware volume level.
 * @param vol: the volume of left channel in db
 * @retval the DAC hardware volume level
 */
static s32_t dac_volume_level_to_db(u16_t level)
{
	s32_t vol = 0;
	if (level < 0xBF) {
		level = 0xBF - level;
		vol = VOL_INDEX_TO_DB(level);
		vol = -vol;
	} else {
		level = level - 0xBF;
		vol = VOL_INDEX_TO_DB(level);
	}
	return vol;
}

/*
 * @brief DAC left/right channel volume setting
 * @param left_vol: the volume of left channel in db
 * @param right_vol: the volume of right channel in db
 * @param adj_cnt: soft stepping gain control count which keep the same as sample rate
 * @return 0 on success, negative errno code on failure
 */
static int dac_volume_set(struct phy_audio_device *dev, s32_t left_vol, s32_t right_vol, u8_t adj_cnt)
{
	struct dac_private_data *priv = (struct dac_private_data *)dev->private;

#ifdef DAC_VOL_SOFT_STEP_IRQ
	struct acts_audio_dac *dac_reg = get_dac_base();
#endif
	int ret = 0;
	u8_t lr_sel = 0;
	u16_t cur_vol_level, l_vol_level, r_vol_level;

	SYS_LOG_INF("Set volume db [%d, %d]", left_vol, right_vol);

#ifndef CONFIG_LEFT_CHANNEL_MUTE
	if (left_vol != AOUT_VOLUME_INVALID)
		lr_sel |= LEFT_CHANNEL_SEL;
#endif

#ifndef CONFIG_RIGHT_CHANNEL_MUTE
	if (right_vol != AOUT_VOLUME_INVALID)
		lr_sel |= RIGHT_CHANNEL_SEL;
#endif

	if ((left_vol <= VOL_MUTE_MIN_DB) || (right_vol <= VOL_MUTE_MIN_DB)) {
		SYS_LOG_INF("volume [%d, %d] less than mute level %d",
		left_vol, right_vol, VOL_MUTE_MIN_DB);
		dac_mute_control(true);
		priv->info.vol_set_mute = 1;
	} else {
		if (priv->info.vol_set_mute) {
			dac_mute_control(false);
			priv->info.vol_set_mute = 0;
		}
	}

	l_vol_level = dac_volume_db_to_level(left_vol);
	cur_vol_level = dac_digital_volume_get(LEFT_CHANNEL_SEL);
	if (cur_vol_level == l_vol_level) {
		SYS_LOG_DBG("ignore same left volume:%d", cur_vol_level);
		lr_sel &= ~LEFT_CHANNEL_SEL;
	}

	r_vol_level = dac_volume_db_to_level(right_vol);
	cur_vol_level = dac_digital_volume_get(RIGHT_CHANNEL_SEL);
	if (cur_vol_level == r_vol_level) {
		SYS_LOG_DBG("ignore same right volume:%d", cur_vol_level);
		lr_sel &= ~RIGHT_CHANNEL_SEL;
	}

	dac_digital_volume_set(lr_sel, l_vol_level, r_vol_level);

	SYS_LOG_INF("Set volume level [%x, %x]", l_vol_level, r_vol_level);

	if (dac_pcmbuf_avail_length() >= (DAC_PCMBUF_MAX_CNT - 1)) {
		SYS_LOG_DBG("no data in pcmbuf can not enable soft step volume");
		return 0;
	}

#ifdef DAC_VOL_SOFT_STEP_IRQ
	if (lr_sel & LEFT_CHANNEL_SEL)
		dac_volume_left_soft_cfg(adj_cnt, DAC_VOL_TO_CNT_DEFAULT, true);

	if (lr_sel & RIGHT_CHANNEL_SEL)
		dac_volume_right_soft_cfg(adj_cnt, DAC_VOL_TO_CNT_DEFAULT, true);

	if (lr_sel & LEFT_CHANNEL_SEL) {
		if (k_sem_take(&priv->voll_done, K_MSEC(DAC_VOL_SET_TIMEOUT_MS))) {
			SYS_LOG_ERR("Left channel volume:%d set timeout", left_vol);
			dac_dump_regs();
			ret = -EIO;
		}
		/* Disable left soft step IRQ */
		dac_reg->vol_lch &= ~VOL_LCH_VOLL_IRQ_EN;
	}

	if (lr_sel & RIGHT_CHANNEL_SEL) {
		if (k_sem_take(&priv->volr_done, K_MSEC(DAC_VOL_SET_TIMEOUT_MS))) {
			SYS_LOG_ERR("Right channel volume:%d set timeout", right_vol);
			dac_dump_regs();
			ret = -EIO;
		}
		/* Disable right soft step IRQ */
		dac_reg->vol_rch &= ~VOL_RCH_VOLR_IRQ_EN;
	}
#else
	if (lr_sel & LEFT_CHANNEL_SEL)
		dac_volume_left_soft_cfg(adj_cnt, DAC_VOL_TO_CNT_DEFAULT, false);

	if (lr_sel & RIGHT_CHANNEL_SEL)
		dac_volume_right_soft_cfg(adj_cnt, DAC_VOL_TO_CNT_DEFAULT, false);
#endif

	return ret;
}

/*
 * @brief Enable the features that supported by DAC
 * @param void
 * @return void
 */
static void dac_enable_features(void)
{
#ifdef DAC_PCMBUF_CNT_EN
	dac_pcmbuf_counter_enable();
#endif

#ifdef DAC_AUTO_MUTE_EN
	dac_automute_cfg(DAC_AUTOMUTE_CNT_DEFAULT);
#endif

#ifdef DAC_SDM_MUTE_EN
	dac_sdm_mute_cfg(DAC_SDM_CNT_DEFAULT, DAC_SDM_THRESHOLD);
#endif

#ifdef DAC_LOOPBACK_EN
	dac_loopback_cfg(LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL, true);
#endif
}

/*
 * @brief Configure the physical layout within DAC
 * @param void
 * @return 0 on success, negative errno code on failure
 */
static int dac_physical_layout_cfg(void)
{
	int ret = -1;
	u8_t lr_sel = 0;

#ifndef CONFIG_LEFT_CHANNEL_MUTE
	lr_sel |= LEFT_CHANNEL_SEL;
#endif

#ifndef CONFIG_RIGHT_CHANNEL_MUTE
	lr_sel |= RIGHT_CHANNEL_SEL;
#endif

#ifdef CONFIG_DAC_SINGLE_END_LAYOUT
	ret = dac_analog_single_end_cfg(lr_sel);
#elif CONFIG_DAC_VRO_LAYOUT
	ret = dac_analog_vro_cfg(lr_sel);
#elif CONFIG_DAC_DIFFERENCTIAL_LAYOUT
	ret = dac_analog_diff_cfg(lr_sel);
#endif

	return ret;
}

static int phy_dac_prepare_enable(struct phy_audio_device *dev, aout_param_t *out_param)
{
	dac_setting_t *dac_setting = out_param->dac_setting;

	if ((!out_param) || (!out_param->dac_setting)
		|| (!out_param->sample_rate)) {
		SYS_LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if ((dac_setting->channel_mode != STEREO_MODE)
		&& (dac_setting->channel_mode != MONO_MODE)) {
		SYS_LOG_ERR("Invalid channel mode %d", dac_setting->channel_mode);
		return -EINVAL;
	}

	if (!(out_param->channel_type & AUDIO_CHANNEL_DAC)) {
		SYS_LOG_ERR("Invalid channel type %d", out_param->channel_type);
		return -EINVAL;
	}

	if (out_param->outfifo_type != AOUT_FIFO_DAC0) {
		SYS_LOG_ERR("Invalid FIFO type %d", out_param->outfifo_type);
		return -EINVAL;
	}

	if (out_param->reload_setting) {
		SYS_LOG_ERR("DAC FIFO does not support reload mode");
		return -EINVAL;
	}

	/* Enable DAC clock gate */
	acts_clock_peripheral_enable(CLOCK_ID_AUDIO_DAC);

	/* DAC main clock source is alway 256FS */
	if (audio_samplerate_dac_set(out_param->sample_rate)) {
		SYS_LOG_ERR("Failed to config sample rate %d", out_param->sample_rate);
		return -ESRCH;
	}

	return 0;
}

static int phy_dac_enable(struct phy_audio_device *dev, void *param)
{
	aout_param_t *out_param = (aout_param_t *)param;
	dac_setting_t *dac_setting = out_param->dac_setting;
	struct acts_audio_dac *dac_reg = get_dac_base();
	struct dac_private_data *priv = (struct dac_private_data *)dev->private;
	int ret;

	if (is_dac_fifo_working(dev)) {
		SYS_LOG_ERR("The DAC FIFO is using");
		return -EACCES;
	}

	ret = phy_dac_prepare_enable(dev, out_param);
	if (ret) {
		SYS_LOG_ERR("Failed to prepare enable dac err=%d", ret);
		return ret;
	}

	dac_fifo_enable(FIFO_SEL_DMA,
					(out_param->channel_width == CHANNEL_WIDTH_16BITS)
					? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS);

	if (dac_pcmbuf_config(out_param->channel_width)) {
		SYS_LOG_ERR("PCM BUF config error");
		return -ENOEXEC;
	}

	/* The THD+N of small signal is bad when OSR is 64x in differenctial mode,
	 * however when sample rate is below 16kfs, OSR shall be set 64x to avoid noise.
	 */
#ifdef CONFIG_DAC_DIFFERENCTIAL_LAYOUT
	if (out_param->sample_rate > SAMPLE_RATE_16KHZ)
		ret = dac_digital_enable(DAC_OSR_32X, dac_setting->channel_mode,
							out_param->channel_type);
	else
		ret = dac_digital_enable(DAC_OSR_64X, dac_setting->channel_mode,
							out_param->channel_type);
#else
	if (out_param->sample_rate > SAMPLE_RATE_48KHZ)
		ret = dac_digital_enable(DAC_OSR_32X, dac_setting->channel_mode,
							out_param->channel_type);
	else
		ret = dac_digital_enable(DAC_OSR_DEFAULT, dac_setting->channel_mode,
								out_param->channel_type);
#endif
	if (ret) {
		SYS_LOG_ERR("Failed to enable DAC digital err=%d", ret);
		return ret;
	}

	dac_enable_features();

	ret = dac_physical_layout_cfg();
	if (ret) {
		SYS_LOG_ERR("DAC config physical layout error %d", ret);
		return ret;
	}

	/* Record the PCM BUF data callback */
	priv->info.callback = out_param->callback;
	priv->info.cb_data = out_param->cb_data;
	priv->info.sample_rate = out_param->sample_rate;

	priv->info.dac_fifo_busy = 1;

	/* Enable DAC IRQs */
	phy_dac_irq_enable(dev);

	/* Clear FIFO ERROR */
	if (dac_reg->stat & DAC_STAT_FIFO_ER)
		dac_reg->stat |= DAC_STAT_FIFO_ER;

	return dac_volume_set(dev, dac_setting->volume.left_volume,
							dac_setting->volume.right_volume,
							out_param->sample_rate);
}

static int phy_dac_disable_pa(struct phy_audio_device *dev)
{
	struct acts_audio_dac *dac_reg = get_dac_base();
	dac_reg->anactl0 = 0;
	dac_reg->anactl1 = 0;
	acts_clock_peripheral_disable(CLOCK_ID_AUDIO_DAC);
	return 0;
}

static int phy_dac_disable(struct phy_audio_device *dev)
{
	struct dac_private_data *priv = (struct dac_private_data *)dev->private;

	phy_dac_irq_disable(dev);

	dac_pcmbuf_wait_empty(DAC_PCMBUF_WAIT_TIMEOUT_MS);

	dac_digital_disable();

	dac_mute_control(true);

	/* If close the DAC devclk will lead to disable anolog function  at the same time,
	 *  and it is necessary to do the antipop again.
	 */
	//acts_clock_peripheral_disable(CLOCK_ID_AUDIO_DAC);

	memset(&priv->info, 0, sizeof(struct dac_dynamic_info));

	return 0;
}

static int phy_dac_ioctl(struct phy_audio_device *dev, u32_t cmd, void *param)
{
	struct dac_private_data *priv = (struct dac_private_data *)dev->private;
	struct acts_audio_dac *dac_reg = get_dac_base();
	int ret = 0;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		dac_dump_regs();
		break;
	}
	case AOUT_CMD_GET_SAMPLERATE:
	{
		ret = audio_samplerate_dac_get();
		if (ret) {
			SYS_LOG_ERR("Failed to get DAC sample rate err=%d", ret);
			return ret;
		}
		*(audio_sample_rate_e *)param = (audio_sr_sel_e)ret;
		break;
	}
	case AOUT_CMD_SET_SAMPLERATE:
	{
		audio_sample_rate_e val = *(audio_sr_sel_e *)param;
		ret = audio_samplerate_dac_set(val);
		if (ret) {
			SYS_LOG_ERR("Failed to set DAC sample rate err=%d", ret);
			return ret;
		}
		break;
	}
	case AOUT_CMD_OPEN_PA:
	{
		break;
	}
	case AOUT_CMD_CLOSE_PA:
	{
		ret = phy_dac_disable_pa(dev);
		break;
	}
	case AOUT_CMD_OUT_MUTE:
	{
		u8_t flag = *(u8_t *)param;
		if (flag)
			dac_mute_control(true);
		else
			dac_mute_control(false);
		break;
	}
	case AOUT_CMD_GET_SAMPLE_CNT:
	{
		u32_t val;
		val = dac_read_pcmbuf_counter();
		*(u32_t *)param = val + priv->info.fifo_cnt;
		SYS_LOG_DBG("DAC FIFO counter: %d", *(u32_t *)param);
		break;
	}
	case AOUT_CMD_RESET_SAMPLE_CNT:
	{
		u32_t key = irq_lock();
		priv->info.fifo_cnt = 0;
		dac_pcmbuf_counter_reset();
		irq_unlock(key);

		break;
	}
	case AOUT_CMD_ENABLE_SAMPLE_CNT:
	{
		dac_pcmbuf_counter_enable();
		break;
	}
	case AOUT_CMD_DISABLE_SAMPLE_CNT:
	{
		u32_t key = irq_lock();
		dac_pcmbuf_counter_disable();
		priv->info.fifo_cnt = 0;
		irq_unlock(key);

		break;
	}
	case AOUT_CMD_GET_VOLUME:
	{
		u16_t level;
		volume_setting_t *volume = (volume_setting_t *)param;
		level = dac_digital_volume_get(LEFT_CHANNEL_SEL);
		volume->left_volume = dac_volume_level_to_db(level);
		level = dac_digital_volume_get(RIGHT_CHANNEL_SEL);
		volume->right_volume = dac_volume_level_to_db(level);
		SYS_LOG_INF("Get volume [%d, %d]", volume->left_volume, volume->right_volume);
		break;
	}
	case AOUT_CMD_SET_VOLUME:
	{
		volume_setting_t *volume = (volume_setting_t *)param;
		ret = dac_volume_set(dev, volume->left_volume,
				volume->right_volume, priv->info.sample_rate);
		if (ret) {
			SYS_LOG_ERR("Volume set[%d, %d] error=%d",
				volume->left_volume, volume->right_volume, ret);
			return ret;
		}
		break;
	}
	case AOUT_CMD_GET_CHANNEL_STATUS:
	{
		u8_t status = 0;
		if (check_dac_fifo_error()) {
			status = AUDIO_CHANNEL_STATUS_ERROR;
			SYS_LOG_DBG("DAC FIFO ERROR");
		}

		/* When the totally samples number is odd, the 'PCMBS' will end with (DAC_PCMBUF_MAX_CNT - 1) */
		if (dac_pcmbuf_avail_length() < (DAC_PCMBUF_MAX_CNT - 1))
			status |= AUDIO_CHANNEL_STATUS_BUSY;

		*(u8_t *)param = status;
		break;
	}
	case AOUT_CMD_GET_FIFO_LEN:
	{
		*(u32_t *)param = DAC_PCMBUF_MAX_CNT;
		break;
	}
	case AOUT_CMD_GET_FIFO_AVAILABLE_LEN:
	{
		*(u32_t *)param = dac_pcmbuf_avail_length();
		break;
	}
	case AOUT_CMD_GET_APS:
	{
		ret = audio_pll_get_dac_aps();
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
		ret = audio_pll_set_dac_aps(level);
		if (ret) {
			SYS_LOG_ERR("Failed to set audio pll APS err=%d", ret);
			return ret;
		}
		SYS_LOG_DBG("set new aps level %d", level);
		break;
	}
	case AOUT_CMD_SET_DAC_THRESHOLD:
	{
		dac_threshold_setting_t *thres = (dac_threshold_setting_t *)param;
		ret = dac_pcmbuf_threshold_update(thres);
		break;
	}
	case PHY_CMD_FIFO_GET:
	{
		aout_param_t *out_param = (aout_param_t *)param;
		if (!out_param)
			return -EINVAL;

		if (is_dac_fifo_working(dev)) {
			SYS_LOG_ERR("DAC FIFO is using");
			return -EBUSY;
		}

		/* reset dac and enable dac clock */
		acts_reset_peripheral(RESET_ID_AUDIO_DAC);
		acts_clock_peripheral_enable(CLOCK_ID_AUDIO_DAC);

		dac_fifo_enable(FIFO_SEL_DMA,
						(out_param->channel_width == CHANNEL_WIDTH_16BITS)
						? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS);

		ret = dac_pcmbuf_config(out_param->channel_width);
		if (ret) {
			SYS_LOG_ERR("PCMBUF config error:%d", ret);
			return ret;
		}

		priv->info.callback = out_param->callback;
		priv->info.cb_data = out_param->cb_data;
		SYS_LOG_DBG("Enable PCMBUF callback:%p", priv->info.callback);

		phy_dac_irq_enable(dev);

		priv->info.dac_fifo_busy = 1;
		break;
	}
	case PHY_CMD_FIFO_PUT:
	{
		if (is_dac_fifo_working(dev)) {
			dac_pcmbuf_wait_empty(DAC_PCMBUF_WAIT_TIMEOUT_MS);
			dac_fifo_disable();
			phy_dac_irq_disable(dev);
			if (dac_reg->digctl & DAC_DIGCTL_DDEN)
				dac_reg->digctl &= ~DAC_DIGCTL_DDEN;
			memset(&priv->info, 0, sizeof(struct dac_dynamic_info));
		}
		priv->info.dac_fifo_busy = 0;
		break;
	}
	case PHY_CMD_DAC_WAIT_EMPTY:
	{
		wait_dac_fifo_empty();
		break;
	}
	case PHY_CMD_CLAIM_WITH_128FS:
	{
		dac_digital_claim_128fs(true);
		break;
	}
	case PHY_CMD_CLAIM_WITHOUT_128FS:
	{
		dac_digital_claim_128fs(false);
		break;
	}
	default:
		SYS_LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;
	}

	return ret;
}

static int phy_dac_init(struct phy_audio_device *dev, struct device *parent_dev)
{
	struct dac_private_data *priv = (struct dac_private_data *)dev->private;
	memset(priv, 0, sizeof(struct dac_private_data));

	priv->parent_dev = parent_dev;

#ifdef DAC_VOL_SOFT_STEP_IRQ
	k_sem_init(&priv->voll_done, 0, 1);
	k_sem_init(&priv->volr_done, 0, 1);
#endif

	SYS_LOG_INF("PHY DAC probe");

	return 0;
}

void phy_dac_isr(void* arg)
{
	struct phy_audio_device *dev = (struct phy_audio_device *)arg;
	struct acts_audio_dac *dac_reg = get_dac_base();
	struct dac_private_data *priv = (struct dac_private_data *)dev->private;
	u32_t stat, pending = 0;
#ifdef DAC_VOL_SOFT_STEP_IRQ
	u32_t voll_stat, volr_stat;

	voll_stat = dac_reg->vol_lch;
	volr_stat = dac_reg->vol_rch;
#endif

	stat = dac_reg->pcm_buf_stat;

	SYS_LOG_DBG("STAT:0x%x pcmbuf counter:0x%x",
		stat, dac_reg->pcm_buf_cnt);

	if (dac_reg->pcm_buf_cnt & PCM_BUF_CNT_IP) {
		priv->info.fifo_cnt += (AOUT_FIFO_CNT_MAX + 1);
		/* Here we need to wait 100us for the synchronization of audio clock fields */
		k_busy_wait(DAC_SAMPLE_CNT_IRQ_SYNC_US);
		dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;
		/**
		  * FIXME: If the sample counter is just equal to 0xFFFF and not any more input data,
		  * The pcmbuf sample counter irq will keep happening, at this time we has to reset
		  * the pcmbuf sample counter function.
		  */
		if (priv->info.fifo_cnt_timestamp) {
			if ((k_uptime_get_32() - priv->info.fifo_cnt_timestamp)
				< DAC_SAMPLE_CNT_IRQ_TIMEOUT_MS) {
				dac_pcmbuf_counter_reset();
			}
		}
		priv->info.fifo_cnt_timestamp = k_uptime_get();
	}

	if ((stat & PCM_BUF_STAT_PCMBEIP)
		&& (dac_reg->pcm_buf_ctl & PCM_BUF_CTL_PCMBEPIE)) {
		pending |= AOUT_DMA_IRQ_TC;
		dac_reg->pcm_buf_ctl &= ~PCM_BUF_CTL_PCMBEPIE;
	}

	if (stat & PCM_BUF_STAT_PCMBHEIP) {
		pending |= AOUT_DMA_IRQ_HF;
		/* Wait until there is half empty irq happen and then start to detect empty irq */
		if (!(dac_reg->pcm_buf_ctl & PCM_BUF_CTL_PCMBEPIE))
			dac_reg->pcm_buf_ctl |= PCM_BUF_CTL_PCMBEPIE;
	}

#ifdef DAC_VOL_SOFT_STEP_IRQ
	if (voll_stat & VOL_LCH_SOFT_STEP_DONE) {
		dac_reg->vol_lch |= VOL_LCH_SOFT_STEP_DONE;
		k_sem_give(&priv->voll_done);
	}

	if (volr_stat & VOL_RCH_SOFT_STEP_DONE) {
		dac_reg->vol_rch |= VOL_RCH_SOFT_STEP_DONE;
		k_sem_give(&priv->volr_done);
	}
#endif

	dac_reg->pcm_buf_stat = stat;

	if (pending && priv->info.callback)
		priv->info.callback(priv->info.cb_data, pending);
}

static struct dac_private_data dac_priv;

const struct phy_audio_operations dac_aops = {
	.audio_init = phy_dac_init,
	.audio_enable = phy_dac_enable,
	.audio_disable = phy_dac_disable,
	.audio_ioctl = phy_dac_ioctl,
};

PHY_AUDIO_DEVICE(dac, PHY_AUDIO_DAC_NAME, &dac_aops, &dac_priv);
PHY_AUDIO_DEVICE_FN(dac);

/*
 * @brief Enable DAC IRQ
 * @note DAC IRQ source as shown below:
 *	- DAC FIFO Empty IRQ
 *	- PCM BUF Empty IRQ
 *	- PCM BUF Full IRQ
 *	- PCM BUF Half Empty IRQ
 *	- PCM BUF Half Full IRQ
 *	- VOLL Set IRQ
 *	- VOLR Set IRQ
 *	- PCM_BUF CNT OF IRQ
 */
static void phy_dac_irq_enable(struct phy_audio_device *dev)
{
	ARG_UNUSED(dev);
	if (irq_is_enabled(IRQ_ID_AUDIO_DAC) == 0) {
		IRQ_CONNECT(IRQ_ID_AUDIO_DAC, CONFIG_IRQ_PRIO_HIGH,
			phy_dac_isr, PHY_AUDIO_DEVICE_GET(dac), 0);
		irq_enable(IRQ_ID_AUDIO_DAC);
	}
}

static void phy_dac_irq_disable(struct phy_audio_device *dev)
{
	ARG_UNUSED(dev);
	if (irq_is_enabled(IRQ_ID_AUDIO_DAC) != 0)
		irq_disable(IRQ_ID_AUDIO_DAC);
}
