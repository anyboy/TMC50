/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file DMA configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_DMA_H_
#define	_ACTIONS_SOC_DMA_H_

#define	DMA_ID_MEMORY			0
#define	DMA_ID_BASEBAND_RAM		1
#define DMA_ID_MODEM_DEBUG_FIFO_DST  2
#define	DMA_ID_UART0			3
#define DMA_ID_PWM              4
#define DMA_ID_MODEM_DEBUG_FIFO_SRC  4
#define	DMA_ID_AUDIO_DAC_FIFO	5
#define	DMA_ID_AUDIO_ADC_FIFO	5
#define	DMA_ID_SD0			    6
#define	DMA_ID_SD1			    7
#define	DMA_ID_UART1			8
#define	DMA_ID_SPI1			    9
#define	DMA_ID_AUDIO_I2S		10
#define	DMA_ID_AUDIO_SPDIF		11
#define	DMA_ID_AUDIO_LCD		12
#define	DMA_ID_SPI0			    13
#define DMA_ID_FFT_FIFO         14
#define DMA_ID_AES_FIFO         15
#define DMA_ID_TWI0             16
#define DMA_ID_TWI1             17

#define	DMA_ID_MAX_ID			DMA_ID_FFT

#define DMA_GLOB_OFFS			0x0000
#define DMA_CHAN_OFFS			0x0100

#define DMA_CHAN(base, id)		((base) + DMA_CHAN_OFFS + (id) * 0x100)
#define DMA_GLOBAL(base)		((base) + DMA_GLOB_OFFS)

#define DMA_ACTS_MAX_CHANNELS		8

/* Maximum data sent in single transfer (Bytes) */
#define DMA_ACTS_MAX_DATA_ITEMS		0x3ffff

#define DMA_CHANNEL_REG_BASE        (DMA_REG_BASE + 0x100)
#define DMAIP                       (DMA_REG_BASE)
#define DMAIE                       (DMA_REG_BASE+0x4)
#define DMADEBUG                    (DMA_REG_BASE+0x80)

#define DMA0CTL                     (DMA_REG_BASE+0x00000100)
#define DMA0START                   (DMA_REG_BASE+0x00000104)
#define DMA0SADDR0                  (DMA_REG_BASE+0x00000108)
#define DMA0DADDR0                  (DMA_REG_BASE+0x00000110)
#define DMA0BC                      (DMA_REG_BASE+0x00000118)
#define DMA0RC                      (DMA_REG_BASE+0x0000011c)

#define DMA_CTL_OFFSET              (0x00)
#define DMA_START_OFFSET            (0x04)
#define DMA_SADDR_OFFSET            (0x08)
#define DMA_DADDR_OFFSET            (0x10)
#define DMA_BC_OFFSET               (0x18)
#define DMA_RC_OFFSET               (0x1c)

#define DMA_CTL_TWS_SHIFT		20
#define DMA_CTL_TWS(x)			((x) << DMA_CTL_TWS_SHIFT)
#define DMA_CTL_TWS_MASK		DMA_CTL_TWS(0x3)
#define DMA_CTL_TWS_8BIT		DMA_CTL_TWS(2)
#define DMA_CTL_TWS_16BIT		DMA_CTL_TWS(1)
#define DMA_CTL_TWS_32BIT		DMA_CTL_TWS(0)
#define DMA_CTL_RELOAD			(0x1 << 18)
#define DMA_CTL_TRMODE_SINGLE	(0x1 << 17)
#define DMA_CTL_SRC_TYPE_SHIFT		0
#define DMA_CTL_SRC_TYPE(x)		((x) << DMA_CTL_SRC_TYPE_SHIFT)
#define DMA_CTL_SRC_TYPE_MASK		DMA_CTL_SRC_TYPE(0x1f)
#define DMA_CTL_DST_TYPE_SHIFT		8
#define DMA_CTL_DST_TYPE(x)		((x) << DMA_CTL_DST_TYPE_SHIFT)
#define DMA_CTL_DST_TYPE_MASK		DMA_CTL_DST_TYPE(0x1f)
#define DMA_CTL_DAM_SHIFT       15
#define DMA_CTL_SAM_SHIFT       7

#define DMA_REG_SHIFT               8 /* chan step = 0x100 */

#define DMA0START_DMASTART          0
#define DMA0CTL_RELOAD              18


typedef struct dma_regs
{
    volatile unsigned int ctl;
    volatile unsigned int start;
    volatile unsigned int saddr0;
    volatile unsigned int reserveds1;
    volatile unsigned int daddr0;
    volatile unsigned int reservedd1;
    volatile unsigned int framelen;
    volatile unsigned int frame_remainlen;
}dma_reg_t;

#ifndef _ASMLANGUAGE

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_DMA_H_	*/
