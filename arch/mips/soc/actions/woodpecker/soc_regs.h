/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file register address define for Actions SoC
 */

#ifndef	_ACTIONS_SOC_REGS_H_
#define	_ACTIONS_SOC_REGS_H_


#define RMU_REG_BASE			        0xc0000000
#define CMU_REG_BASE			        0xc0001000
#define EFUSE_BASE						0xc0100000
#define ADC_REG_BASE			        0xc002002c //PMUADC_CTL
#define DMA_REG_BASE			        0xc0010000
#define FFT_REG_BASE			        0xc01e0000
#define AUDIO_DAC_REG_BASE		        0xc0050000
#define AUDIO_ADC_REG_BASE		        0xc0051000
#define AUDIO_I2S_REG_BASE		        0xc0052000
#define AUDIO_I2STX0_REG_BASE			0xc0052000
#define AUDIO_I2SRX0_REG_BASE			0xc0053000
#define AUDIO_I2SRX1_REG_BASE			0xc0053000
#define AUDIO_SPDIFTX_REG_BASE		    0xc0054000
#define AUDIO_SPDIFRX_REG_BASE		    0xc0055000
#define MMC0_REG_BASE			        0xc0060000
#define MMC1_REG_BASE			        0xc0070000
#define GPIO_REG_BASE			        0xc0090000
#define SE_REG_BASE                     0xc00e0000
#define SEG_LED_REG_BASE		        0xc0190000
#define SPI0_REG_BASE			        0xc0110000
#define SPI1_REG_BASE			        0xc00f0000
#define RTC_REG_BASE			        0xc0120000
#define I2C0_REG_BASE			        0xc0130000
#define I2C1_REG_BASE			        0xc0150000
#define IRC_REG_BASE			        0xc01c0050
#define CEC_REG_BASE					0xc01d0000
#define ASRC_REG_BASE			        0xcccccccc //no support
#define PWM_REG_BASE			        0xc01f0000
#define UART0_REG_BASE			        0xc01a0000
#define UART1_REG_BASE			        0xc01b0000
#define SPI2_REG_BASE			        0xcccccccc //no support
#define CMU_DIGITAL_REG_BASE            0xc0001000
#define CMU_ANALOG_REG_BASE             0xc0000100
#define DSP_MAILBOX_REG_BASE            0xcccccccc //no support
#define SPICACHE_REG_BASE				0xc0140000
#define SPI1_DCACHE_REG_BASE            0xcccccccc //no support


#define MEMORY_CONTROLLER_REG_BASE     0xc00a0000
#define INT_REG_BASE                   0xc00b0000
#define SEG_SREEN_REG_BASE             0xc0190000
#define PMU_REG_BASE                   0xC0020000


/* EFUSE Address */
#define     EFUSE_CTL0					(EFUSE_BASE+0x00)
#define     EFUSE_CTL1					(EFUSE_BASE+0x04)
#define     EFUSE_CTL2					(EFUSE_BASE+0x08)
#define     EFUSE_DATA0					(EFUSE_BASE+0x0C)
#define     EFUSE_DATA1					(EFUSE_BASE+0x10)
#define     EFUSE_DATA2					(EFUSE_BASE+0x14)
#define     EFUSE_DATA3					(EFUSE_BASE+0x18)
#define     EFUSE_DATA4					(EFUSE_BASE+0x1C)
#define     EFUSE_DATA5					(EFUSE_BASE+0x20)
#define     EFUSE_DATA6					(EFUSE_BASE+0x24)
#define     EFUSE_DATA7					(EFUSE_BASE+0x28)

/* INTC registers */
#define M4K_WAKEUP_EN0		(INT_REG_BASE+0x40)
#define M4K_WAKEUP_EN1		(INT_REG_BASE+0x44)

/* RMU registers */
#define RMU_MRCR0			(RMU_REG_BASE+0x0)
#define RMU_MRCR1			(RMU_REG_BASE+0x04)

/* CMU Analog registers */
#define HOSC_CTL			(CMU_ANALOG_REG_BASE+0x00)
#define LOSC_CTL			(CMU_ANALOG_REG_BASE+0x04)
#define SPLL_CTL			(CMU_ANALOG_REG_BASE+0x10)
#define CORE_PLL_CTL		(CMU_ANALOG_REG_BASE+0x14)
#define AUDIO_PLL0_CTL		(CMU_ANALOG_REG_BASE+0x18)
#define AUDIO_PLL1_CTL		(CMU_ANALOG_REG_BASE+0x1C)
#define SPLL_DEBUG			(CMU_ANALOG_REG_BASE+0x20)
#define CORE_PLL_DEBUG		(CMU_ANALOG_REG_BASE+0x24)
#define CORE_PLL_DEBUG1		(CMU_ANALOG_REG_BASE+0x28)

/* PMU registers */
#define PMU_POWER_CTL		(PMU_REG_BASE+0x10)
#define PMU_WKEN_CTL		(PMU_REG_BASE+0x14)
#define PMU_WAKE_PD			(PMU_REG_BASE+0x18)
#define PMU_ONOFF_KEY		(PMU_REG_BASE+0x1C)
#define PMU_VOUT_CTL		(PMU_REG_BASE+0x00)
#define PMU_VOUT_CTL1		(PMU_REG_BASE+0x04)

/* timer0 */
#define T0_CTL				0xc0120100
#define T0_VAL				0xc0120104
#define T0_CNT				0xc0120108

/* timer1 */
#define T1_CTL				0xc0120120
#define T1_VAL				0xc0120124
#define T1_CNT				0xc0120128

/* timer2 */
#define TIMER2_CAPTURE_BASE 0xc0120140

/* timer3 */
#define TIMER3_CAPTURE_BASE 0xc0120160

/* watch dog */
#define WD_CTL				0xc012001c

/* wio0 */
#define WIO0_CTL            0xc0090200

/* wio1 */
#define WIO1_CTL            0xc0090204

/* wio2 */
#define WIO2_CTL            0xc0090208

/* jtag ctl */
#define JTAG_CTL            0xc0090300

#define DSP_PAGE_ADDR0      0xcccccccc //no support
#define DSP_VCT_ADDR        0xcccccccc //no support

#define RTC_REGUPDATE       (RTC_REG_BASE+0x04)
#define RTC_BAK2            (RTC_REG_BASE+0x38)
#define RTC_BAK3            (RTC_REG_BASE+0x3C)
#define HCL_4HZ				(RTC_REG_BASE+0x240)

#define INTC_PD0            (INT_REG_BASE+0x00000000)
#define INTC_PD1            (INT_REG_BASE+0x00000004)
#define INTC_EN0            (INT_REG_BASE+0x00000010)
#define INTC_EN1            (INT_REG_BASE+0x00000014)
#define INTC_MSK0           (INT_REG_BASE+0x00000010)
#define INTC_MSK1           (INT_REG_BASE+0x00000014)
#define INTC_CFG0_0         (INT_REG_BASE+0x00000020)
#define INTC_CFG0_1         (INT_REG_BASE+0x00000024)
#define INTC_CFG0_2         (INT_REG_BASE+0x00000028)
#define INTC_CFG1_0         (INT_REG_BASE+0x00000030)
#define INTC_CFG1_1         (INT_REG_BASE+0x00000034)
#define INTC_CFG1_2         (INT_REG_BASE+0x00000038)

#define INT_REQ_INT_OUT     0xcccccccc //no support
#define INT_REQ_IN          0xcccccccc //no support
#define INT_REQ_IN_PD       0xcccccccc //no support
#define INT_REQ_OUT         0xcccccccc //no support

static inline void delay(volatile unsigned int count)
{
	while(count-- != 0);
}

#endif /* _ACTIONS_SOC_REGS_H_	*/
