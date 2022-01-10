/*
 * Copyright (c) 2020 Actions Semi Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "accelerator_inner.h"

int accelerator_process_fft(struct device *dev, void *args)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;
	struct acts_accelerator_controller *accelerator = cfg->base;
	struct accelerator_fft_param *param = args;
	const u8_t sample_bytes = (param->sample_bits > 16) ? 4 : 2;
	int output_samples;
	int res;

	/* validate parameters */
	if (((u32_t)param->output_buf & 0x3) || ((u32_t)param->input_buf & 0x3)) {
		SYS_LOG_ERR("mode=%d; input=%p,%d; output=%p",
				param->mode, param->input_buf, param->input_samples, param->output_buf);
		return -EINVAL;
	}

	/* configure accelerator mode */
	if (param->mode) {
		accelerator->mode = MODE_CLK_SEL_FFT | MODE_SEL_IFFT | MODE_IRQ_EN | MODE_IRQ_PD;
		output_samples = param->input_samples - 2;
	} else {
		accelerator->mode = MODE_CLK_SEL_FFT | MODE_SEL_FFT | MODE_IRQ_EN | MODE_IRQ_PD;
		output_samples = param->input_samples + 2;
	}

	if (param->sample_bits > 16) {
		accelerator->fft_cfg = FFT_CFG_SAMPLES(param->frame_type) |
				FFT_CFG_OUT_FORMAT_32BIT | FFT_CFG_IN_FORMAT_32BIT | FFT_CFG_EN;
	} else {
		accelerator->fft_cfg = FFT_CFG_SAMPLES(param->frame_type) |
				FFT_CFG_OUT_FORMAT_16BIT | FFT_CFG_IN_FORMAT_16BIT | FFT_CFG_EN;
	}

	res = accelerator_dma_transfer(dev, param->input_buf, param->input_samples * sample_bytes, true);
	if (res)
		goto out_exit;

	res = accelerator_dma_transfer(dev, param->output_buf, output_samples * sample_bytes, false);
	if (res)
		goto out_exit;

	/* wait the whole done */
	res = k_sem_take(&data->work_done, 10);
	if (res)
		goto out_exit;

	/* clear mode */
	accelerator->mode = (accelerator->mode & ~MODE_CLK_SEL_MASK) | MODE_CLK_SEL_NONE;
	/* output result */
	param->output_samples = output_samples;
out_exit:
	accelerator_check_error(dev, res);
	return res;
}
