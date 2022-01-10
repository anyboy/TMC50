/*
 * Copyright (c) 2020 Actions Semi Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "accelerator_inner.h"

int accelerator_process_fir(struct device *dev, void *args)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;
	struct acts_accelerator_controller *accelerator = cfg->base;
	struct accelerator_fir_param *param = args;
	int output_len = 0;
	int res = 0;

	/* validate parameters */
	if ((param->mode > FIR_CFG_MODE_MAX) || ((u32_t)param->hist_buf & 0x3) ||
		((u32_t)param->output_buf & 0x3) || ((u32_t)param->input_buf & 0x3) ||
		(param->input_len & 0x1) || (param->input_len > FIR_IN_SAMPLES_MAX * 2)) {
		SYS_LOG_ERR("mode=%d; hist=%p,%d; input=%p,%d; output=%p",
				param->mode, param->hist_buf, param->hist_len,
				param->input_buf, param->input_len, param->output_buf);
		return -EINVAL;
	}

	/* configure accelerator mode */
	accelerator->mode = MODE_CLK_SEL_FIR | MODE_SEL_FIR | MODE_IRQ_EN | MODE_IRQ_PD;

	/* configure input samples */
	accelerator->fir_inout = FIR_IN_SAMPLES(param->input_len / 2);

	/* start fir */
	accelerator->fir_cfg = FIR_CFG_MODE_SEL(param->mode) | FIR_CFG_EN | FIR_CFG_IRQ_EN | FIR_CFG_IRQ_PD;

	/* input history coeff */
	accelerator->fir_cfg |= FIR_CFG_COEF_READ;
	res = accelerator_dma_transfer(dev, param->hist_buf, param->hist_len, true);
	if (res)
		goto out_exit;

	/* input data */
	accelerator->fir_cfg |= FIR_CFG_DATA_READ;
	res = accelerator_dma_transfer(dev, param->input_buf, param->input_len, true);
	if (res)
		goto out_exit;

	/* wait fir computing done */
	 res = k_sem_take(&data->work_done, 10);
	if (res)
		goto out_exit;

	output_len = FIR_OUT_SAMPLES(accelerator->fir_inout) * 2;

	/* output history coeff */
	accelerator->fir_cfg |= FIR_CFG_COEF_WB;
	res = accelerator_dma_transfer(dev, param->hist_buf, param->hist_len, false);
	if (res)
		goto out_exit;

	/* output result data */
	accelerator->fir_cfg |= FIR_CFG_DATA_WB;
	res = accelerator_dma_transfer(dev, param->output_buf, output_len, false);
	if (res)
		goto out_exit;

	/* wait the whole done */
	res = k_sem_take(&data->work_done, 10);
	if (res)
		goto out_exit;

	/* clear mode */
	accelerator->mode = (accelerator->mode & ~MODE_CLK_SEL_MASK) | MODE_CLK_SEL_NONE;
	/* output result */
	param->output_len = output_len;
out_exit:
	accelerator_check_error(dev, res);
	return res;
}
