/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-10-12-PM4:51:13             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_pmu.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-10-12-PM4:51:13
 *******************************************************************************/

#ifndef SOC_PMU_H_
#define SOC_PMU_H_

#define     VOUT_CTL0                                                         (PMU_REG_BASE+0x00)
#define     VOUT_CTL1                                                         (PMU_REG_BASE+0x04)
#define     BDG_CTL                                                           (PMU_REG_BASE+0x08)
#define     SYSTEM_SET                                                        (PMU_REG_BASE+0x0C)
#define     POWER_CTL                                                         (PMU_REG_BASE+0x10)
#define     WKEN_CTL                                                          (PMU_REG_BASE+0x14)
#define     WAKE_PD                                                           (PMU_REG_BASE+0x18)
#define     ONOFF_KEY                                                         (PMU_REG_BASE+0x1C)
#define     SPD_CTL                                                           (PMU_REG_BASE+0x20)
#define     DCDC_CTL1                                                         (PMU_REG_BASE+0x24)
#define     DCDC_CTL2                                                         (PMU_REG_BASE+0x28)
#define     PMUADC_CTL                                                        (PMU_REG_BASE+0x2C)
#define     BATADC_DATA                                                       (PMU_REG_BASE+0x30)
#define     SENSADC_DATA                                                      (PMU_REG_BASE+0x34)
#define     MUXADC_DATA                                                       (PMU_REG_BASE+0x38)
#define     SVCCADC_DATA                                                      (PMU_REG_BASE+0x3C)
#define     LRADC1_DATA                                                       (PMU_REG_BASE+0x40)
#define     LRADC2_DATA                                                       (PMU_REG_BASE+0x44)
#define     LRADC3_DATA                                                       (PMU_REG_BASE+0x48)
#define     LRADC4_DATA                                                       (PMU_REG_BASE+0x4C)
#define     LRADC5_DATA                                                       (PMU_REG_BASE+0x50)
#define     LRADC6_DATA                                                       (PMU_REG_BASE+0x54)
#define     LRADC7_DATA                                                       (PMU_REG_BASE+0x58)
#define     LRADC8_DATA                                                       (PMU_REG_BASE+0x5C)
#define     LRADC9_DATA                                                       (PMU_REG_BASE+0x60)
#define     LRADC10_DATA                                                      (PMU_REG_BASE+0x64)
#define     LRADC11_DATA                                                      (PMU_REG_BASE+0x68)
#define     LRADC12_DATA                                                      (PMU_REG_BASE+0x6C)

#define     ADC_DATA_BASE   PMUADC_CTL

#define     ONOFF_KEY_ONOFF_PRESS_0         0
#define     PMUADC_CTL_LRADC1_EN            5
#define     LRADC1_DATA_LRADC1_MASK         (0x3FF<<0)

#define     BDG_CTL_BDG_FILTER              6
#define     BDG_CTL_BDG_PDR                 5
#define     BDG_CTL_BDG_VOL_SHIFT           0
#define     BDG_CTL_BDG_VOL_MASK            (0x1F << BDG_CTL_BDG_VOL_SHIFT)

unsigned int soc_pmu_get_vdd_voltage();
void soc_pmu_set_vdd_voltage(unsigned int volt_mv);
void pmu_init(void);
void soc_pmu_set_seg_res_sel(unsigned int set_val);
void soc_pmu_set_seg_dip_vcc_en(unsigned int set_val);
void soc_pmu_set_seg_led_en(unsigned int set_val);

void pmu_spll_init(void);

#endif /* SOC_PMU_H_ */
