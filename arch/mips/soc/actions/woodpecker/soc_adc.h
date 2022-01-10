/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ADC configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_ADC_H_
#define	_ACTIONS_SOC_ADC_H_


#define	ADC_ID_BATV			0
#define ADC_ID_SENSOR       1
#define ADC_ID_MUXADC       2
#define ADC_ID_SVCC         3
#define	ADC_ID_CH1			4
#define	ADC_ID_CH2			5
#define	ADC_ID_CH3			6
#define	ADC_ID_CH4			7
#define	ADC_ID_CH5			8
#define	ADC_ID_CH6			9
#define	ADC_ID_CH7			10
#define	ADC_ID_CH8			11
#define	ADC_ID_CH9			12
#define	ADC_ID_CH10			13
#define	ADC_ID_CH11			14
#define	ADC_ID_CH12			15

#define	ADC_ID_MAX_ID			ADC_ID_CH12

/* ADC_ID_BATV just after PMUADC_CTL(base), so add 0x04 */
#define ADC_CHAN_DATA_REG(base, chan)	((volatile u32_t)(base) + 0x4 + (chan) * 0x4)

#ifndef _ASMLANGUAGE

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_ADC_H_	*/
