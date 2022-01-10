/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_CLOCK_H_
#define	_ACTIONS_SOC_CLOCK_H_

#define	CLOCK_ID_DMA			0
#define	CLOCK_ID_SD0			1
#define	CLOCK_ID_SD1			2
#define	CLOCK_ID_SPI0			4
#define	CLOCK_ID_SPI1			5
#define	CLOCK_ID_SPI0_CACHE		6
#define	CLOCK_ID_FM_CLK			7
#define	CLOCK_ID_USB			8
#define	CLOCK_ID_SEC			9
#define	CLOCK_ID_LCD			10
#define	CLOCK_ID_SEG_LCD		11
#define	CLOCK_ID_IR			    12
#define	CLOCK_ID_LRADC			13
#define	CLOCK_ID_TIMER0			14
#define CLOCK_ID_TIMER1			CLOCK_ID_TIMER0
#define	CLOCK_ID_PWM			15
#define	CLOCK_ID_I2C0			16
#define	CLOCK_ID_I2C1			17
#define	CLOCK_ID_UART0			18
#define	CLOCK_ID_UART1			19
#define	CLOCK_ID_FFT			20
#define	CLOCK_ID_VAD			21
#define	CLOCK_ID_CEC			22
#define	CLOCK_ID_AUDIO_ADC		24
#define	CLOCK_ID_AUDIO_DAC		25
#define	CLOCK_ID_I2S0_TX		26
#define	CLOCK_ID_I2S0_RX		27
#define	CLOCK_ID_I2S_SRD		28
#define	CLOCK_ID_SPDIF_TX		29
#define	CLOCK_ID_SPDIF_RX		30
#define	CLOCK_ID_EFUSE			31


#define	CLOCK_ID_BT_BB_3K2		32
#define	CLOCK_ID_BT_BB_DIG		33
#define	CLOCK_ID_BT_BB_AHB		34
#define	CLOCK_ID_BT_MODEM_AHB   35
#define	CLOCK_ID_BT_MODEM_DIG	36
#define	CLOCK_ID_BT_MODEM_INTF	37
#define	CLOCK_ID_BT_MODEM_DMA	38
#define	CLOCK_ID_BT_RF			39

#define	CLOCK_ID_MAX_ID			39

#ifndef _ASMLANGUAGE

void acts_clock_peripheral_enable(int clock_id);
void acts_clock_peripheral_disable(int clock_id);
void acts_deep_sleep_clock_peripheral(const char * id_buffer, int buffer_len);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_CLOCK_H_	*/
