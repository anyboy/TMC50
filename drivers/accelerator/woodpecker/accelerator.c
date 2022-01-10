/*
 * Copyright (c) 2020 Actions Semi Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "accelerator_inner.h"

extern unsigned int soc_freq_get_corepll_freq(void);
extern int dvfs_register_notifier(struct notifier_block *nb);
extern int dvfs_unregister_notifier(struct notifier_block *nb);

static void accelerator_acts_config_irq(void);

static void accelerator_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;
	struct acts_accelerator_controller *accelerator = cfg->base;
	u32_t value;

	value = accelerator->mode;
	if (value & MODE_IRQ_PD) {
		accelerator->mode = value;
		k_sem_give(&data->work_done);
	}

	value = accelerator->fir_cfg;
	if (value & FIR_CFG_IRQ_PD) {
		accelerator->fir_cfg = value;
		k_sem_give(&data->work_done);
	}

	value = accelerator->iir_cfg;
	if (value & IIR_CFG_IRQ_PD) {
		accelerator->iir_cfg = value;
		k_sem_give(&data->work_done);
	}
}

int accelerator_dma_transfer_start(struct device *dev,
			void *buf, int len, bool to_device)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;
	int res = 0;

	if (!data->dma_chan)
		return -ENODEV;

	if (((u32_t)buf & 0x3) || (len <= 0))
		return -EINVAL;

	memset(&data->dma_cfg, 0, sizeof(data->dma_cfg));
	memset(&data->dma_block_cfg, 0, sizeof(data->dma_block_cfg));

	data->dma_cfg.dma_slot = cfg->dma_id;
	data->dma_cfg.dest_data_size = cfg->fifo_width;
	data->dma_cfg.block_count = 1;
	data->dma_cfg.head_block = &data->dma_block_cfg;
	data->dma_block_cfg.block_size = len;

	if (to_device) {
		data->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		data->dma_block_cfg.source_address = (u32_t)buf;
	} else {
		data->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		data->dma_block_cfg.dest_address = (u32_t)buf;
	}

	res = dma_config(data->dma_dev, data->dma_chan, &data->dma_cfg);
	if (res) {
		SYS_LOG_ERR("dma-%d config failed (res=%d)", data->dma_chan, res);
		return res;
	}

	res = dma_start(data->dma_dev, data->dma_chan);
	if (res) {
		SYS_LOG_ERR("dma-%d start failed (res=%d)", data->dma_chan, res);
		return res;
	}

	return 0;
}

int accelerator_dma_transfer_finish(struct device *dev)
{
	struct accelerator_driver_data *data = dev->driver_data;
	int res = 0;

	if (!data->dma_chan)
		return -ENODEV;

	res = dma_wait_timeout(data->dma_dev, data->dma_chan, ACCELERATOR_DMA_TIMEOUT_MS);
	dma_stop(data->dma_dev, data->dma_chan);

	return res;
}

void accelerator_check_error(struct device *dev, int error)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;

	if (error && data->ref_count > 0) {
		acts_reset_peripheral(cfg->clock_id);
		SYS_LOG_ERR("reset (err=%d)", error);
	}
}

static int accelerator_acts_set_clock(struct device *dev)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;

#if USE_FIXED_24M_CLK
	/* hosc, 16/16 */
	sys_write32((0x0 << 8) | 0xf, cfg->clk_reg);
#else
	u32_t core_pll = soc_freq_get_corepll_freq();

	/* FIXME: hardware bug */
	if (core_pll <= 132) {
		/* corepll, 16/16 */
		sys_write32((0x1 << 8) | 0xf, cfg->clk_reg);
	} else {
		/* hosc, 16/16 */
		sys_write32((0x0 << 8) | 0xf, cfg->clk_reg);
	}

	SYS_LOG_INF("core_pll=%u, clk_reg=0x%08x", core_pll, sys_read32(cfg->clk_reg));
#endif

	return 0;
}

#if (USE_FIXED_24M_CLK == 0) && defined(CONFIG_DVFS_DYNAMIC_LEVEL)
static int accelerator_acts_dvfs_notifier_cb(struct notifier_block *self,
		unsigned long state, void *args)
{
	struct dvfs_freqs *freq = args;

	if ((freq->new <= 132) == (freq->old <= 132))
		return NOTIFY_OK;

	/* FIXME: hardware bug */
	switch (state) {
	case DVFS_EVENT_POST_CHANGE:
		if (freq->new <= 132) {
			sys_write32((0x1 << 8) | 0xf, CMU_FFTCLK);
			SYS_LOG_INF("clk 24 -> %d", freq->new);
		}
		break;
	case DVFS_EVENT_PRE_CHANGE:
		if (freq->new > 132) {
			sys_write32((0x0 << 8) | 0xf, CMU_FFTCLK);
			SYS_LOG_INF("clk %d -> 24", freq->old);
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}
#endif /* (USE_FIXED_24M_CLK == 0) && defined(CONFIG_DVFS_DYNAMIC_LEVEL) */

static int accelerator_acts_open(struct device *dev)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;

	if (k_sem_take(&data->lock, 1000)) {
		SYS_LOG_ERR("timeout");
		return -EBUSY;
	}

	if (data->ref_count++ > 0)
		goto out_exit;

	acts_reset_peripheral_assert(cfg->clock_id);
	acts_clock_peripheral_enable(cfg->clock_id);
	acts_reset_peripheral_deassert(cfg->clock_id);
	k_busy_wait(5);

	accelerator_acts_set_clock(dev);

#if (USE_FIXED_24M_CLK == 0) && defined(CONFIG_DVFS_DYNAMIC_LEVEL)
	dvfs_register_notifier(&data->dvfs_notifier);
#endif

out_exit:
	k_sem_give(&data->lock);
	return 0;
}

static int accelerator_acts_close(struct device *dev)
{
	const struct accelerator_config_info *cfg = dev->config->config_info;
	struct accelerator_driver_data *data = dev->driver_data;
	struct acts_accelerator_controller *accelerator = cfg->base;

	if (k_sem_take(&data->lock, 1000)) {
		SYS_LOG_ERR("timeout");
		return -EBUSY;
	}

	if (--data->ref_count > 0)
		goto out_exit;

#if (USE_FIXED_24M_CLK == 0) && defined(CONFIG_DVFS_DYNAMIC_LEVEL)
	dvfs_unregister_notifier(&data->dvfs_notifier);
#endif

	accelerator->mode = 0;

	acts_clock_peripheral_disable(cfg->clock_id);
	acts_reset_peripheral_assert(cfg->clock_id);

out_exit:
	k_sem_give(&data->lock);
	return 0;
}

static int accelerator_acts_process(struct device *dev, int mode, void *args)
{
	struct accelerator_driver_data *data = dev->driver_data;
	int res = 0;

	if (k_sem_take(&data->lock, 1000)) {
		SYS_LOG_ERR("timeout");
		return -EBUSY;
	}

	if (data->ref_count <= 0) {
		res = -ENODEV;
		goto out_exit;
	}

	switch (mode) {
	case ACCELERATOR_MODE_FFT:
		res = accelerator_process_fft(dev, args);
		break;
	case ACCELERATOR_MODE_FIR:
		res = accelerator_process_fir(dev, args);
		break;
	case ACCELERATOR_MODE_IIR:
		res = accelerator_process_iir(dev, args);
		break;
	default:
		res = -EINVAL;
		break;
	}

out_exit:
	k_sem_give(&data->lock);
	return res;
}

static int accelerator_acts_init(struct device *dev)
{
	struct accelerator_driver_data *data = dev->driver_data;

	data->dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (!data->dma_dev) {
		SYS_LOG_ERR("no dma device");
		return -ENODEV;
	}

	data->dma_chan = dma_request(data->dma_dev, 0xff);
	if (!data->dma_chan) {
		SYS_LOG_ERR("dma_request failed");
		return -ENOMEM;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->work_done, 0, 1);

#if (USE_FIXED_24M_CLK == 0) && defined(CONFIG_DVFS_DYNAMIC_LEVEL)
	data->dvfs_notifier.notifier_call = accelerator_acts_dvfs_notifier_cb;
	data->dvfs_notifier.priority = 0;
#endif

	accelerator_acts_config_irq();
	return 0;
}

static const struct accelerator_config_info accelerator_config_data = {
	.base = (void *)FFT_REG_BASE,
	.clk_reg = CMU_FFTCLK,
	.clock_id = CLOCK_ID_FFT,
	.reset_id = RESET_ID_FFT,
	.dma_id = DMA_ID_FFT_FIFO,
	.fifo_width = 4,
};

static struct accelerator_driver_data accelerator_dev_data;

static const struct accelerator_driver_api accelerator_driver_api = {
	.open = accelerator_acts_open,
	.close = accelerator_acts_close,
	.process = accelerator_acts_process,
};

DEVICE_AND_API_INIT(accelerator_acts, CONFIG_ACCELERATOR_ACTS_DEV_NAME,
		accelerator_acts_init, &accelerator_dev_data, &accelerator_config_data,
		POST_KERNEL, CONFIG_ACCELERATOR_ACTS_DEVICE_INIT_PRIORITY,
		&accelerator_driver_api);

static void accelerator_acts_config_irq(void)
{
	IRQ_CONNECT(IRQ_ID_FFT, CONFIG_IRQ_PRIO_NORMAL, accelerator_acts_isr,
			DEVICE_GET(accelerator_acts), 0);
	irq_enable(IRQ_ID_FFT);
}
