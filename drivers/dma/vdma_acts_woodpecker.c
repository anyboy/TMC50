/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief virtual dma controller driver for Actions SoC
 */

#include <kernel.h>
#include <dma.h>
#include <assert.h>
#include <soc.h>
#include <string.h>
#include "com_vdma_woodpecker.h"

#define SYS_LOG_DOMAIN "DMA"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DMA_LEVEL
#include <logging/sys_log.h>

rom_dma_data_t dma_global_data;
rom_dma_data_t *dma_global_data_p;

struct dma_controller dmacs[CONFIG_VDMA_ACTS_PCHAN_NUM];

// There is one dma channel dedicated for printk
static struct dma_wrapper_data wrapper_data[CONFIG_VDMA_ACTS_VCHAN_NUM + 1];

K_MEM_SLAB_DEFINE(vdma_pool, sizeof(struct dma), CONFIG_VDMA_ACTS_VCHAN_NUM, 4);

static void dma_acts_irq(int irq, int type, void *pdata);
static void dma_fixed_acts_irq(int irq);
extern dma_rom_api_t dma_rom_api;

void install_dma_phy_channel(unsigned int channel_num)
{
	rom_dma_data_t *rom_data = (rom_dma_data_t *) dma_global_data_p;

    if (channel_num >= 5) {
		irq_enable(IRQ_ID_DMA5 + (channel_num - 5));
    } else {
    	irq_enable(IRQ_ID_DMA0 + channel_num);
    }

	rom_data->dmacs[rom_data->dma_num].dma_chan_num = channel_num;
	rom_data->dmacs[rom_data->dma_num].channel_assigned = 0;
	rom_data->dmacs[rom_data->dma_num].dma = NULL;

	rom_data->dma_num++;
}

void unistall_dma_phy_channel(unsigned int channel_num)
{
	int i;
	rom_dma_data_t *rom_data = (rom_dma_data_t *) dma_global_data_p;

	if (channel_num >= 5) {
		irq_disable(IRQ_ID_DMA5 + (channel_num - 5));
	} else {
    	irq_disable(IRQ_ID_DMA0 + channel_num);
    }

	for (i = 0; i < CONFIG_VDMA_ACTS_VCHAN_NUM; i++) {
		if (rom_data->dmacs[i].dma_chan_num == channel_num) {
			rom_data->dmacs[i].dma_chan_num = 0xff;
			rom_data->dmacs[i].channel_assigned = 1;
			rom_data->dmacs[i].dma = NULL;

			rom_data->dma_num--;
		}
	}
}

static struct dma_wrapper_data * dma_wrapper_data_alloc(u32_t id)
{
	int i;

	for (i = 0; i < CONFIG_VDMA_ACTS_VCHAN_NUM; i++) {
		if (wrapper_data[i].dma_handle == id)
			break;
	}

	if (i == CONFIG_VDMA_ACTS_VCHAN_NUM) {
		for (i = 0; i < CONFIG_VDMA_ACTS_VCHAN_NUM; i++) {
			if (wrapper_data[i].dma_handle == 0)
				break;
		}
		if (i == CONFIG_VDMA_ACTS_VCHAN_NUM)
			return NULL;
	}

	return &wrapper_data[i];
}

static void dma_wrapper_data_free(u32_t id)
{
	int i;

	for (i = 0; i < CONFIG_VDMA_ACTS_VCHAN_NUM; i++) {
		if (wrapper_data[i].dma_handle == id) {
			wrapper_data[i].dma_handle = 0;
			break;
		}
	}
}

int dma_acts_request(struct device *dev, u32_t id)
{
	if (id != DMA_INVALID_DMACS_NUM) {
		return dma_alloc_channel(id);
	} else {
		struct dma *dma;
		int err;

		err = k_mem_slab_alloc(&vdma_pool, (void **)&dma, K_FOREVER);
		if (err || dma == NULL)
			return 0;

		memset(dma, 0, sizeof(*dma));
		INIT_LIST_HEAD(&dma->list);

		dma->dmac_num = DMA_INVALID_DMACS_NUM;
		set_dma_state(dma, DMA_STATE_ALLOC);

		return (int)dma;
	}
}

void dma_acts_free(struct device *dev, u32_t id)
{
	dma_wrapper_data_free(id);

	if(is_virtual_dma_handle(id)){
		k_mem_slab_free(&vdma_pool, (void **)&id);
	}
}

static int dma_acts_config(struct device *dev, u32_t id,
			    struct dma_config *config)
{
	struct dma_block_config *head_block = config->head_block;
	struct dma_wrapper_data *wrapper_data;
	u32_t ctl;
	int data_width = 1;

    if(id == 0){
        SYS_LOG_ERR("invalid dma chan");
        return -EINVAL;
    }

	if (head_block->block_size > DMA_ACTS_MAX_DATA_ITEMS) {
		SYS_LOG_ERR("DMA error: Data size too big: %d",
		       head_block->block_size);
		return -EINVAL;
	}

	if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		ctl = DMA_CTL_SRC_TYPE(DMA_ID_MEMORY) |
		      DMA_CTL_DST_TYPE(config->dma_slot) | (1 << DMA_CTL_DAM_SHIFT);

	} else if (config->channel_direction == PERIPHERAL_TO_MEMORY)  {
		ctl = DMA_CTL_SRC_TYPE(config->dma_slot) |
		      DMA_CTL_DST_TYPE(DMA_ID_MEMORY) | (1 << DMA_CTL_SAM_SHIFT);
	} else {
		ctl = DMA_CTL_SRC_TYPE(DMA_ID_MEMORY) |
		      DMA_CTL_DST_TYPE(DMA_ID_MEMORY);
	}

	if (config->source_data_size) {
		data_width = config->source_data_size;
	}

	if (config->dest_data_size) {
		data_width = config->dest_data_size;
	}

	if (head_block->source_reload_en || head_block->dest_reload_en) {
		ctl |= DMA_CTL_RELOAD;
	}

	switch (data_width) {
	case 2:
		ctl |= DMA_CTL_TWS_16BIT;
		break;
	case 4:
		ctl |= DMA_CTL_TWS_32BIT;
		break;
	case 1:
	default:
		ctl |= DMA_CTL_TWS_8BIT;
		break;
	}

	if (config->source_burst_length == 1 || config->dest_burst_length == 1) {
		ctl |= DMA_CTL_TRMODE_SINGLE;
	}

	if (config->dma_sam) {
		ctl |= (1 << DMA_CTL_SAM_SHIFT);
	}

	dma_global_data_p->dma_api->frame_config(id,
		(u32_t)head_block->source_address,
		(u32_t)head_block->dest_address,
		(u32_t)head_block->block_size);

	if (config->complete_callback_en || config->half_complete_callback_en ||
	    config->error_callback_en) {
		wrapper_data = dma_wrapper_data_alloc(id);
		if (!wrapper_data) {
			SYS_LOG_ERR("no VDMA resource");
			return -ENOSPC;
		}

		wrapper_data->dma_handle = id;
		wrapper_data->dma_callback = config->dma_callback;
		wrapper_data->callback_data = config->callback_data;
		wrapper_data->complete_callback_en = config->complete_callback_en;
		wrapper_data->half_complete_callback_en = config->half_complete_callback_en;

		dma_global_data_p->dma_api->config(id, ctl,
			(dma_irq_handler_t)dma_acts_irq, (void *)wrapper_data);

	} else {
		dma_global_data_p->dma_api->config(id, ctl, NULL, NULL);
	}

	return 0;
}

static int dma_acts_reload(struct device *dev, u32_t id, u32_t saddr, u32_t daddr, u32_t frame_len)
{
    if(id == 0){
        SYS_LOG_ERR("invalid dma chan");
        return -EINVAL;
    }

	return dma_global_data_p->dma_api->frame_config(id, saddr, daddr, frame_len);
}

static int dma_acts_start(struct device *dev, u32_t id)
{
    if(id == 0){
        SYS_LOG_ERR("invalid dma chan");
        return -EINVAL;
    }

	return dma_global_data_p->dma_api->start(id);
}

static int dma_acts_stop(struct device *dev, u32_t id)
{
    if(id == 0){
        SYS_LOG_ERR("invalid dma chan");
        return -EINVAL;
    }

	dma_global_data_p->dma_api->abort(id);
	return 0;
}

u32_t dma_acts_get_remain_size(struct device *dev, u32_t id)
{
    struct dma *dma;

    if (is_virtual_dma_handle(id)){
        dma = (struct dma *) id;

        if(get_dma_state(dma) == DMA_STATE_ALLOC){
            return 0;
        }
    }

	if (dma_global_data_p->dma_api->query_complete(id)){
		return 0;
	} else {
		return 1;
	}
}

int dma_acts_wait_timeout(struct device *dev, u32_t id, u32_t timeout)
{
	return dma_global_data_p->dma_api->wait_complete(id, timeout);
}

static int dma_acts_prepare_start(struct device *dev, u32_t id)
{
	return acts_dma_pre_start(id);
}

static int dma_acts_get_transfer_length(struct device *dev, u32_t id)
{
	return acts_dma_get_transfer_length(id);
}

static int dma_acts_get_phy_channel(struct device *dev, u32_t id)
{
    return acts_dma_get_phy_channel(id);
}


const struct dma_driver_api dma_acts_driver_api = {
	.config			= dma_acts_config,
	.start			= dma_acts_start,
	.stop			= dma_acts_stop,
	.get_remain_size	= dma_acts_get_remain_size,
	.reload			= dma_acts_reload,
	.request		= dma_acts_request,
	.free			= dma_acts_free,
	.wait_timeout   = dma_acts_wait_timeout,
	.prepare_start  = dma_acts_prepare_start,
	.get_transfer_length = dma_acts_get_transfer_length,
	.get_phy_dma_channel = dma_acts_get_phy_channel,
};

int dma_acts_init(struct device *dev)
{
	dma_global_data_p = &dma_global_data;
	rom_dma_data_t *rom_data = (rom_dma_data_t *) dma_global_data_p;
	int i;

	acts_clock_peripheral_enable(CLOCK_ID_DMA);
	acts_reset_peripheral(RESET_ID_DMA);

	rom_data->dmacs = dmacs;
	rom_data->dma_api = &dma_rom_api;
	//rom_data->irq_save = _arch_irq_lock;
	//rom_data->irq_restore = _arch_irq_unlock;

	INIT_LIST_HEAD(&rom_data->dma_req_list);

	/* FIXME: use CONFIG_VDMA_ACTS_PCHAN_NUM */
#if (CONFIG_VDMA_ACTS_PCHAN_START < 2 || (CONFIG_VDMA_ACTS_PCHAN_START + CONFIG_VDMA_ACTS_PCHAN_NUM) > 8)
#error dma_acts_init: Invalid physical dma channel config
#endif

#if (defined CONFIG_XSPI1_NOR_ACTS && defined CONFIG_XSPI1_ACTS_USE_DMA)
#if (CONFIG_VDMA_ACTS_PCHAN_START == 2)
#error dma_acts_init: Invalid physical dma channel config, need keep DMA2 for spi1 nor
#endif
#endif

	// DMA0 is allocated to BTC and DAM1 is allocated to printk.
    IRQ_CONNECT(IRQ_ID_DMA1, CONFIG_IRQ_PRIO_HIGH, dma_fixed_acts_irq, IRQ_ID_DMA1, 0);
    irq_enable(IRQ_ID_DMA1);

	if (CONFIG_VDMA_ACTS_PCHAN_START <= 2) {
		IRQ_CONNECT(IRQ_ID_DMA2, CONFIG_IRQ_PRIO_HIGH, rom_dma_irq_handle, IRQ_ID_DMA2, 0);
	}

	if (CONFIG_VDMA_ACTS_PCHAN_START <= 3) {
		IRQ_CONNECT(IRQ_ID_DMA3, CONFIG_IRQ_PRIO_HIGH, rom_dma_irq_handle, IRQ_ID_DMA3, 0);
	}

	/* vdma num at least 4 */
	IRQ_CONNECT(IRQ_ID_DMA4, CONFIG_IRQ_PRIO_HIGH, rom_dma_irq_handle, IRQ_ID_DMA4, 0);
	IRQ_CONNECT(IRQ_ID_DMA5, CONFIG_IRQ_PRIO_HIGH, rom_dma_irq_handle, IRQ_ID_DMA5, 0);
	IRQ_CONNECT(IRQ_ID_DMA6, CONFIG_IRQ_PRIO_HIGH, rom_dma_irq_handle, IRQ_ID_DMA6, 0);
	IRQ_CONNECT(IRQ_ID_DMA7, CONFIG_IRQ_PRIO_HIGH, rom_dma_irq_handle, IRQ_ID_DMA7, 0);

	for (i = CONFIG_VDMA_ACTS_PCHAN_START;
	     i < (CONFIG_VDMA_ACTS_PCHAN_NUM + CONFIG_VDMA_ACTS_PCHAN_START);
	     i++) {
		install_dma_phy_channel(i);
	}

	return 0;
}


DEVICE_AND_API_INIT(vdma_acts_andes, CONFIG_DMA_0_NAME, dma_acts_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_DMA_ACTS_DEVICE_INIT_PRIORITY,
		    &dma_acts_driver_api);

static void dma_acts_irq(int irq, int type, void *pdata)
{
	struct dma_wrapper_data *wrapper_data = (struct dma_wrapper_data *)pdata;

	wrapper_data->dma_callback((struct device *)DEVICE_GET(vdma_acts_andes),
				   (u32_t)wrapper_data->callback_data,
				   type);
}

extern dma_reg_t *dma_num_to_reg(int dma_no);

static void dma_fixed_acts_irq(int irq)
{
	int i;
	int phy_dma_no;
	uint32_t pending;

	struct dma_wrapper_data *p_wrapper_data = NULL;

	if (irq >= IRQ_ID_DMA5) {
		phy_dma_no = 5 + (irq - IRQ_ID_DMA5);
	} else {
		phy_dma_no = irq - IRQ_ID_DMA0;
	}

	// only clear dma pending bit
    pending = (sys_read32(DMAIP) & (0x10001 << phy_dma_no));

    sys_write32(pending, DMAIP);

	for (i = 0; i < CONFIG_VDMA_ACTS_VCHAN_NUM; i++) {
		if (wrapper_data[i].dma_handle == (int)dma_num_to_reg(phy_dma_no)){
		    p_wrapper_data = &wrapper_data[i];
        }
	}

    if (p_wrapper_data){
        if (pending & (0x10000 << phy_dma_no)){
            p_wrapper_data->dma_callback((struct device *)DEVICE_GET(vdma_acts_andes),
                   (u32_t)p_wrapper_data->callback_data,
                   DMA_IRQ_HF);
        }

        if (pending & (0x1 << phy_dma_no)){
            p_wrapper_data->dma_callback((struct device *)DEVICE_GET(vdma_acts_andes),
                   (u32_t)p_wrapper_data->callback_data,
                   DMA_IRQ_TC);
        }
    }
}
