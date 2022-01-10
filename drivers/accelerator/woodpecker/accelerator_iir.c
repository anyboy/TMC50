/*
 * Copyright (c) 2020 Actions Semi Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "accelerator_inner.h"

int accelerator_process_iir(struct device *dev, void *args)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;
	struct acts_accelerator_controller *accelerator = cfg->base;
	struct accelerator_iir_param *param = args;
	const int samples = param->inout_len / 4; /* 32-bit */
	const int hist_len = param->level * 5 * 4;
	const int table_len = param->level * 6 * 4;
	int res = 0;

	/* validate parameters */
	if ((param->level > IIR_CFG_LEVEL_MAX) ||
		(param->channel > IIR_CFG_CH_MAX) ||
		(samples > IIR_CFG_IN_SAMPLES_MAX) ||
		((u32_t)param->hist_inbuf & 0x3) || ((u32_t)param->hist_outbuf & 0x3) ||
		((u32_t)param->inout_buf & 0x3) || (param->inout_len & 0x3)) {
		SYS_LOG_ERR("level=%d; hist=%p,%p; inout=%p,%d",
				param->level, param->hist_inbuf, param->hist_outbuf,
				param->inout_buf, param->inout_len);
		return -EINVAL;
	}

	/* configure accelerator mode */
	accelerator->mode = MODE_CLK_SEL_IIR | MODE_SEL_IIR | MODE_IRQ_EN | MODE_IRQ_PD;

	/* start iir (32-bit) */
	accelerator->iir_cfg = IIR_CFG_IN_SAMPLES(samples) |
			IIR_CFG_LEVEL(param->level) | IIR_CFG_CH(param->channel) |
			FIR_CFG_EN | IIR_CFG_IRQ_EN | IIR_CFG_IRQ_PD;

	/* input history coeff */
	if (param->hist_inbuf) {
		accelerator->iir_cfg |= IIR_CFG_COEF_READ;
		res = accelerator_dma_transfer(dev, param->hist_inbuf, hist_len + table_len, true);
		if (res)
			goto out_exit;
	}

	/* input data */
	accelerator->iir_cfg |= IIR_CFG_DATA_READ;
	res = accelerator_dma_transfer(dev, param->inout_buf, param->inout_len, true);
	if (res)
		goto out_exit;

	/* wait fir computing done */
	 res = k_sem_take(&data->work_done, 10);
	if (res)
		goto out_exit;

	/* output history coeff */
	accelerator->iir_cfg |= IIR_CFG_COEF_WB;
	res = accelerator_dma_transfer(dev, param->hist_outbuf, hist_len, false);
	if (res)
		goto out_exit;

	/* output result data */
	accelerator->fir_cfg |= IIR_CFG_DATA_WB;
	res = accelerator_dma_transfer(dev, param->inout_buf, param->inout_len, false);
	if (res)
		goto out_exit;

	/* wait the whole done */
	res = k_sem_take(&data->work_done, 10);
	if (res)
		goto out_exit;

	/* clear mode */
	accelerator->mode = (accelerator->mode & ~MODE_CLK_SEL_MASK) | MODE_CLK_SEL_NONE;
out_exit:
	accelerator_check_error(dev, res);
	return res;
}
