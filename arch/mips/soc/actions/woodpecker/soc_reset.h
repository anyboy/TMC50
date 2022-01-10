/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_RESET_H_
#define	_ACTIONS_SOC_RESET_H_

#define	RESET_ID_DMA			0
#define	RESET_ID_SD0			1
#define	RESET_ID_SD1			2
#define	RESET_ID_SPI0			4
#define	RESET_ID_SPI1			5
#define	RESET_ID_SPI0_CACHE		6
#define	RESET_ID_USB			7
#define	RESET_ID_USB2			8
#define	RESET_ID_SEC			9
#define	RESET_ID_LCD			10
#define	RESET_ID_SEG_LCD		11
#define	RESET_ID_IR			    12
#define	RESET_ID_PWM			15
#define	RESET_ID_I2C0			16
#define	RESET_ID_I2C1			17
#define	RESET_ID_UART0			18
#define	RESET_ID_UART1			19
#define	RESET_ID_FFT			20
#define	RESET_ID_VAD			21
#define	RESET_ID_CEC			22
#define	RESET_ID_AUDIO_ADC		24
#define	RESET_ID_AUDIO_DAC		25
#define	RESET_ID_I2S0_TX		26
#define	RESET_ID_I2S0_RX		27
#define	RESET_ID_SPDIF_TX		29
#define	RESET_ID_SPDIF_RX		30
#define	RESET_ID_BIST			31

#define	RESET_ID_BT_BB			32
#define	RESET_ID_BT_MODEM		33
#define	RESET_ID_BT_RF			34

#define	RESET_ID_MAX_ID			34

#ifndef _ASMLANGUAGE

void acts_reset_peripheral_assert(int reset_id);
void acts_reset_peripheral_deassert(int reset_id);
void acts_reset_peripheral(int reset_id);
void acts_deep_sleep_peripheral(char *reset_id_buffer, int buffer_len);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_RESET_H_	*/
