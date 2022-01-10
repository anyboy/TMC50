/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-10-12-4:59:36             1.0             build this file
 ********************************************************************************/
/*!
 * \file     dmac.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-10-12-4:59:36
 *******************************************************************************/

#ifndef _ROM_VDMA_ANDES_H_
#define _ROM_VDMA_ANDES_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <dma.h>
#include <soc_regs.h>
#include <soc_irq.h>
#include <soc_dma.h>
#include <soc_memctrl.h>
#include "rom_vdma_list.h"

#define DMA_CTL_TRMODE_SINGLE	    (0x1 << 17)
#define DMA_CTL_RELOAD			(0x1 << 15)
#define DMA_CTL_SRC_TYPE_SHIFT		0
#define DMA_CTL_SRC_TYPE(x)		((x) << DMA_CTL_SRC_TYPE_SHIFT)
#define DMA_CTL_SRC_TYPE_MASK		DMA_CTL_SRC_TYPE(0xf)
#define DMA_CTL_DST_TYPE_SHIFT		8
#define DMA_CTL_DST_TYPE(x)		((x) << DMA_CTL_DST_TYPE_SHIFT)
#define DMA_CTL_DST_TYPE_MASK		DMA_CTL_DST_TYPE(0xf)
#define DMA_CTL_TWS_SHIFT		13
#define DMA_CTL_TWS(x)			((x) << DMA_CTL_TWS_SHIFT)
#define DMA_CTL_SAM_SHIFT       4
#define DMA_CTL_DAM_SHIFT       12
#define DMA_CTL_TWS_MASK		DMA_CTL_TWS(0x3)
#define DMA_CTL_TWS_8BIT		DMA_CTL_TWS(3)
#define DMA_CTL_TWS_16BIT		DMA_CTL_TWS(2)
#define DMA_CTL_TWS_32BIT		DMA_CTL_TWS(1)
#define DMA_CTL_TWS_64BIT		DMA_CTL_TWS(0)


#define DMA_INVALID_DMACS_NUM       (0xff)

typedef void (*dma_irq_handler_t)(int irq, unsigned int type, void *pdata);

struct dma_controller;

typedef enum
{
    DMA_STATE_ALLOC = 0,
    DMA_STATE_WAIT,
    DMA_STATE_CONFIG,
    DMA_STATE_IRQ_HF,
    DMA_STATE_IRQ_TC,
    DMA_STATE_IDLE,
    DMA_STATE_ABORT,
    DMA_STATE_MAX,
}dma_state_e;

struct dma
{
    struct list_head list;
    struct dma_regs dma_regs;
    void *pdata;
    dma_irq_handler_t dma_irq_handler;
    unsigned char dmac_num;
    unsigned char dma_state;
    unsigned short reload_count;
};

struct dma_controller
{
    unsigned char dma_chan_num;
    unsigned char channel_assigned;
    unsigned char reserved[2];
    struct dma *dma;
};

struct dma_wrapper_data
{
	int  dma_handle;
	void (*dma_callback)(struct device *dev, u32_t data, int reason);
	/* Client callback private data */
	void *callback_data;
};

typedef struct{
    int (*config)(int dma_handle, u32_t ctl, dma_irq_handler_t handle, void *pdata);
    int (*frame_config)(int dma_handle, u32_t saddr, u32_t daddr, u32_t frame_len);
    int (*start)(int dma_handle);
    void (*abort)(int dma_handle);
    void (*dma_irq_dispatch)(void);
    bool (*query_complete)(int dma_handle);
    int (*wait_complete)(int dma_handle, unsigned int timeout_us);
    void (*reg_write_start)(struct dma *dma, int dma_no);
}dma_rom_api_t;

typedef struct{
    uint8_t dma_num;
    struct dma_controller *dmacs;
    struct list_head dma_req_list;
    dma_rom_api_t *dma_api;
} rom_dma_data_t;

extern rom_dma_data_t *rom_dma_global_p;

static inline void set_dma_state(struct dma *dma, unsigned int state)
{
    dma->dma_state = state;
}

static inline unsigned int get_dma_state(struct dma *dma)
{
    return dma->dma_state;
}

void rom_dma_irq_handle(unsigned int irq);
int dma_alloc_channel(int channel);
int is_virtual_dma_handle(int dma_handle);
int acts_dma_pre_start(int handle);
int acts_dma_get_transfer_length(int handle);
int acts_dma_get_phy_channel(int handle);
#endif /* _ROM_VDMA_ANDES_H_ */
