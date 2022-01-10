/*
 * Copyright (c) 2020 Actions Semi Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ACCELERATOR_INNER_H__
#define __ACCELERATOR_INNER_H__

#include <soc.h>
#include <kernel.h>
#include <notify.h>
#include <dvfs.h>
#include <string.h>
#include <drivers/accelerator/accelerator.h>
#include <logging/sys_log.h>

/* Mode register */
/* interrupt */
#define MODE_IRQ_PD        BIT(31)
#define MODE_IRQ_EN        BIT(30)
/* clock mode */
#define MODE_CLK_SEL_SHIFT (2)
#define MODE_CLK_SEL(x)    ((x) << MODE_CLK_SEL_SHIFT)
#define MODE_CLK_SEL_MASK  MODE_CLK_SEL(0x3)
#define MODE_CLK_SEL_NONE  MODE_CLK_SEL(0)
#define MODE_CLK_SEL_FFT   MODE_CLK_SEL(1)
#define MODE_CLK_SEL_IIR   MODE_CLK_SEL(2)
#define MODE_CLK_SEL_FIR   MODE_CLK_SEL(3)
/* run mode */
#define MODE_SEL_SHIFT    (0)
#define MODE_SEL(x)       ((x) << MODE_SEL_SHIFT)
#define MODE_SEL_MASK     MODE_SEL(0x3)
#define MODE_SEL_FFT      MODE_SEL(0)
#define MODE_SEL_IFFT     MODE_SEL(1)
#define MODE_SEL_IIR      MODE_SEL(2)
#define MODE_SEL_FIR      MODE_SEL(3)

/* FFT config register */
/* FFT/IFFT samples*/
#define FFT_CFG_SAMPLES_SHIFT (4)
#define FFT_CFG_SAMPLES(x)    ((x) << FFT_CFG_SAMPLES_SHIFT)
#define FFT_CFG_SAMPLES_MASK  FFT_CFG_SAMPLES(0x3)
/* FFT */
#define FFT_CFG_SAMPLES_128   FFT_CFG_SAMPLES(0)
#define FFT_CFG_SAMPLES_256   FFT_CFG_SAMPLES(1)
#define FFT_CFG_SAMPLES_512   FFT_CFG_SAMPLES(2)
/* IFFT */
#define FFT_CFG_SAMPLES_130   FFT_CFG_SAMPLES(0)
#define FFT_CFG_SAMPLES_258   FFT_CFG_SAMPLES(1)
#define FFT_CFG_SAMPLES_514   FFT_CFG_SAMPLES(2)
/* FFT output format */
#define FFT_CFG_OUT_FORMAT_SHIFT (2)
#define FFT_CFG_OUT_FORMAT_MASK  (0x1 << FFT_CFG_OUT_FORMAT_SHIFT)
#define FFT_CFG_OUT_FORMAT_16BIT (0x0 << FFT_CFG_OUT_FORMAT_SHIFT)
#define FFT_CFG_OUT_FORMAT_32BIT (0x1 << FFT_CFG_OUT_FORMAT_SHIFT)
/* FFT input format */
#define FFT_CFG_IN_FORMAT_SHIFT (1)
#define FFT_CFG_IN_FORMAT_MASK  (0x1 << FFT_CFG_IN_FORMAT_SHIFT)
#define FFT_CFG_IN_FORMAT_16BIT (0x0 << FFT_CFG_IN_FORMAT_SHIFT)
#define FFT_CFG_IN_FORMAT_32BIT (0x1 << FFT_CFG_IN_FORMAT_SHIFT)
/* FFT EN */
#define FFT_CFG_EN              BIT(0)

/* IIR config register */
#define IIR_CFG_IRQ_PD          BIT(31)
#define IIR_CFG_IRQ_EN          BIT(30)
#define IIR_CFG_INLEN_ERR       BIT(29)
/* IIR input samples */
#define IIR_CFG_IN_SAMPLES_SHIFT (16)
#define IIR_CFG_IN_SAMPLES(x)    ((x) << IIR_CFG_IN_SAMPLES_SHIFT)
#define IIR_CFG_IN_SAMPLES_MASK  IIR_CFG_IN_SAMPLES(0xfff)
#define IIR_CFG_IN_SAMPLES_MAX   (772)
/* IIR level */
#define IIR_CFG_LEVEL_SHIFT      (8)
#define IIR_CFG_LEVEL(x)         ((x) << IIR_CFG_LEVEL_SHIFT)
#define IIR_CFG_LEVEL_MASK       IIR_CFG_LEVEL(0xf)
#define IIR_CFG_LEVEL_MAX        (14)
/* IIR output data write back to system ram */
#define IIR_CFG_DATA_WB         BIT(5)
/* IIR history coff write back to system ram */
#define IIR_CFG_COEF_WB         BIT(4)
/* IIR input data read from system ram */
#define IIR_CFG_DATA_READ       BIT(3)
/* IIR coff (filter + history) read from system ram */
#define IIR_CFG_COEF_READ       BIT(2)
/* IIR channel */
#define IIR_CFG_CH_SHIFT        (1)
#define IIR_CFG_CH(x)           ((x) << IIR_CFG_CH_SHIFT)
#define IIR_CFG_CH_MASK         IIR_CFG_CH(0x1)
#define IIR_CFG_CH_L            IIR_CFG_CH(0)
#define IIR_CFG_CH_R            IIR_CFG_CH(1)
#define IIR_CFG_CH_MAX          (1)
/* IIR EN */
#define IIR_CFG_EN              BIT(0)

/* FIR config register */
#define FIR_CFG_IRQ_PD          BIT(31)
#define FIR_CFG_IRQ_EN          BIT(30)
/* FIR mode */
#define FIR_CFG_MODE_SHIFT      (8)
#define FIR_CFG_MODE_SEL(x)     ((x) << FIR_CFG_MODE_SHIFT)
#define FIR_CFG_MODE_SEL_MASK   FIR_CFG_MODE_SEL(0xf)
#define FIR_CFG_MODE_I8_O16     FIR_CFG_MODE_SEL(0)
#define FIR_CFG_MODE_I8_O32     FIR_CFG_MODE_SEL(1)
#define FIR_CFG_MODE_I8_O44     FIR_CFG_MODE_SEL(2)
#define FIR_CFG_MODE_I8_O48     FIR_CFG_MODE_SEL(3)
#define FIR_CFG_MODE_I48_O8     FIR_CFG_MODE_SEL(4)
#define FIR_CFG_MODE_I48_O16    FIR_CFG_MODE_SEL(5)
#define FIR_CFG_MODE_I44_O48    FIR_CFG_MODE_SEL(6)
#define FIR_CFG_MODE_I16_O8     FIR_CFG_MODE_SEL(7)
#define FIR_CFG_MODE_MAX        (7)
/* FIR output data write back to system ram */
#define FIR_CFG_DATA_WB         BIT(7)
/* FIR history coef write back to system ram */
#define FIR_CFG_COEF_WB         BIT(6)
/* FIR input data read from system ram */
#define FIR_CFG_DATA_READ       BIT(5)
/* FIR history coef read from system ram */
#define FIR_CFG_COEF_READ       BIT(4)
/* FIR EN */
#define FIR_CFG_EN              BIT(0)

/* FIR IN/OUT Samples register */
#define FIR_OUT_SAMPLES_SHIFT     (16)
#define FIR_OUT_SAMPLES_MASK      (0x7ff << FIR_OUT_SAMPLES_SHIFT)
#define FIR_OUT_SAMPLES(reg)      (((reg) & FIR_OUT_SAMPLES_MASK) >> FIR_OUT_SAMPLES_SHIFT)
#define FIR_OUT_SAMPLES_MAX       (0x7ff)
#define FIR_IN_SAMPLES_SHIFT      (0)
#define FIR_IN_SAMPLES(num)       ((num) << FIR_IN_SAMPLES_SHIFT)
#define FIR_IN_SAMPLES_MASK       FIR_IN_SAMPLES(0x7ff)
#define FIR_IN_SAMPLES_MAX        (0x7ff)

#define ACCELERATOR_BUSY_POLLING(condition, timeout_us, res) \
	do { \
		int time_us = 0; \
		while (1) { \
			k_busy_wait(1); \
			\
			if (condition) { \
				res = 0; \
				break; \
			} \
			\
			if (++time_us >= timeout_us) { \
				res = -ETIME; \
				break; \
			} \
		} \
	} while (0)

struct acts_accelerator_controller {
	volatile u32_t mode;
	volatile u32_t fft_cfg;
	volatile u32_t iir_cfg;
	volatile u32_t fir_cfg;
	volatile u32_t fir_inout;
	volatile u32_t fir_w;
};

struct accelerator_config_info {
	struct acts_accelerator_controller *base;

	u32_t clk_reg;
	u8_t clock_id;
	u8_t reset_id;
	u8_t dma_id;
	u8_t fifo_width;
};

#define ACCELERATOR_TIMEOUT_US     (5000)
#define ACCELERATOR_DMA_TIMEOUT_MS (10)

/* FIXME: clock source select HOST 24MHz */
#define USE_FIXED_24M_CLK 1

struct accelerator_driver_data {
	int ref_count;

	struct k_sem lock;
	struct k_sem work_done;

	struct device *dma_dev;
	int dma_chan;
	struct dma_config dma_cfg;
	struct dma_block_config dma_block_cfg;

#if (USE_FIXED_24M_CLK == 0) && defined(CONFIG_DVFS_DYNAMIC_LEVEL)
	struct notifier_block dvfs_notifier;
#endif
};

void accelerator_check_error(struct device *dev, int error);

int accelerator_dma_transfer_start(struct device *dev,
			void *buf, int len, bool to_device);

int accelerator_dma_transfer_finish(struct device *dev);

static inline int accelerator_dma_transfer(struct device *dev,
			void *buf, int len, bool to_device)
{
	int res = accelerator_dma_transfer_start(dev, buf, len, to_device);
	if (res)
		return res;

	return accelerator_dma_transfer_finish(dev);
}

int accelerator_process_fft(struct device *dev, void *args);
int accelerator_process_fir(struct device *dev, void *args);
int accelerator_process_iir(struct device *dev, void *args);

#endif /* __ACCELERATOR_INNER_H__ */
