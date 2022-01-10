/********************************************************************************
 *                            USDK(US283A_master)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-1-5-3:23:20             1.0             build this file
 ********************************************************************************/
/*!
 * \file     rom_dma_ats283x.c
 * \brief    DMA physical driver
 * \author
 * \version  1.0
 * \date  2018-1-5-3:23:20
 *******************************************************************************/
#include "com_vdma_woodpecker.h"

static struct dma *stat_dma;

extern unsigned int __get_PSR(void);

static inline uint32_t DMA_PSRAM_ADDR_CONVERT(unsigned int cpu_addr)
{
    return cpu_addr;
}

int is_virtual_dma_handle(int dma_handle)
{
    if ((dma_handle & 0xc0000000) == 0xc0000000) {
        return 0;
    } else {
        return 1;
    }
}

dma_reg_t *dma_num_to_reg(int dma_no)
{
    return (dma_reg_t *) (DMA_CHANNEL_REG_BASE + (dma_no << DMA_REG_SHIFT));
}

int dma_reg_to_num(dma_reg_t *reg)
{
    return (((int) reg - DMA_CHANNEL_REG_BASE) >> DMA_REG_SHIFT);
}

int dma_alloc_channel(int channel)
{
    return (int) dma_num_to_reg(channel);
}

void rom_dma_set_irq(unsigned int dma_no, unsigned int irq_enable, unsigned int half_irq_enable)
{
    if (irq_enable) {
        sys_write32(sys_read32(DMAIE) | (0x1 << dma_no), DMAIE);

		// wirte 1 to clear DMAIP
        sys_write32((0x1 << dma_no), DMAIP);
    } else {
        sys_write32(sys_read32(DMAIE) & (~(0x1 << dma_no)), DMAIE);

        // wirte 1 to clear DMAIP
        sys_write32((0x1 << dma_no), DMAIP);
    }

    if (half_irq_enable) {
        sys_write32(sys_read32(DMAIE) | (0x10000 << dma_no), DMAIE);

        // wirte 1 to clear DMAIP
        sys_write32((0x10000 << dma_no), DMAIP);
    } else {
        sys_write32(sys_read32(DMAIE) & (~(0x10000 << dma_no)), DMAIE);

        // wirte 1 to clear DMAIP
        sys_write32((0x10000 << dma_no), DMAIP);
    }

}

void acts_dma_start(dma_reg_t *dma_reg)
{
    //if(dma_reg->framelen >= 32){
    //    dma_reg->ctl &= (~(1 << DMA0CTL_TRM));
    //}else{
    //  // if length is less than 32 bytes, use the single mode
    //    dma_reg->ctl |= (1 << DMA0CTL_TRM);
    //}

    //start DMA transmission
    dma_reg->start |= (1 << DMA0START_DMASTART);
}

void acts_dma_stop(dma_reg_t *dma_reg)
{
    u32_t reg_ctl = dma_reg->ctl;

	// clear reload flag
    dma_reg->ctl &= (~(1 << DMA0CTL_RELOAD));

	// stop DMA transmission
    while(dma_reg->start & (1 << DMA0START_DMASTART)){
        dma_reg->start &= (~(1 << DMA0START_DMASTART));
    }

	// clear DMA IE and pending
    rom_dma_set_irq(dma_reg_to_num(dma_reg), 1, 1); /* ONLY FOR ASRC */

    dma_reg->ctl |= (reg_ctl & (1 << DMA0CTL_RELOAD));
}

void rom_dma_stop(dma_reg_t *dma_reg)
{
	// clear reload flag
    dma_reg->ctl &= (~(1 << DMA0CTL_RELOAD));

    // stop DMA transmission
    while(dma_reg->start & (1 << DMA0START_DMASTART)){
        dma_reg->start &= (~(1 << DMA0START_DMASTART));
    }

    // clear DMA IE and pending
    rom_dma_set_irq(dma_reg_to_num(dma_reg), 0, 0);
}

static void set_dma_data(struct dma_controller *dmac, struct dma *dma)
{
    dmac->dma = dma;
    dma->dmac_num = dmac - dma_global_data_p->dmacs;
}

static inline void free_dma_controller(struct dma_controller *dmac)
{
    dmac->dma = NULL;
}

static inline void free_dma_xfer(struct dma *dma)
{
    dma->dmac_num = DMA_INVALID_DMACS_NUM;
    set_dma_state(dma, DMA_STATE_IDLE);
}

static void free_dma_data(struct dma_controller *dmac, struct dma *dma)
{
    free_dma_controller(dmac);
    free_dma_xfer(dma);
}

struct dma_controller *rom_dma_controller_alloc(struct dma *dma)
{
    int i;

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) dma_global_data_p;

    for (i = 0; i < p_rom_data->dma_num; i++) {
        if (p_rom_data->dmacs[i].channel_assigned == 0) {
            p_rom_data->dmacs[i].channel_assigned = 1;
            set_dma_data(&(p_rom_data->dmacs[i]), dma);
            return &(p_rom_data->dmacs[i]);
        }
    }

    return NULL;
}

struct dma *rom_dma_controller_get_dma_handle(u32_t dma_reg)
{
    int i;

    int dma_no = dma_reg_to_num((dma_reg_t *)dma_reg);

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) dma_global_data_p;

    for (i = 0; i < p_rom_data->dma_num; i++) {
        if (p_rom_data->dmacs[i].dma_chan_num == dma_no && p_rom_data->dmacs[i].channel_assigned) {
            return (p_rom_data->dmacs[i].dma);
        }
    }

    return NULL;
}


struct dma_controller *rom_dma_controller_is_alloc(struct dma *dma)
{
    struct dma_controller *dmac;

    if (dma->dmac_num == DMA_INVALID_DMACS_NUM) {
        return NULL;
    }

    dmac = dma_global_data_p->dmacs + dma->dmac_num;

    if (dmac->channel_assigned == 1) {
        return dmac;
    }

    return NULL;
}

void rom_dma_controller_free(struct dma_controller *dmac, struct dma *dma)
{
    dmac->channel_assigned = 0;
    free_dma_data(dmac, dma);
}

struct dma_controller *dma_get_intstatus(unsigned int channel_no)
{
    int i;

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) dma_global_data_p;

    for (i = 0; i < p_rom_data->dma_num; i++) {
        if (p_rom_data->dmacs[i].channel_assigned == 1 && p_rom_data->dmacs[i].dma_chan_num == channel_no) {
            return &(p_rom_data->dmacs[i]);
        }
    }

    return NULL;
}

void rom_dma_regs_write_and_start(struct dma *dma, int dma_no)
{
    dma_reg_t *dma_reg = dma_num_to_reg(dma_no);
    struct dma_wrapper_data *wrapper_data;

    if(dma != NULL){
        set_dma_state(dma, DMA_STATE_CONFIG);

        wrapper_data = (struct dma_wrapper_data *)(dma->pdata);

        /* vdma depends upon comp irq to update state of complete */
        if (wrapper_data)
            rom_dma_set_irq(dma_no, wrapper_data->complete_callback_en, wrapper_data->half_complete_callback_en);
        else
            rom_dma_set_irq(dma_no, 1, 0);

        dma_reg->saddr0 = dma->dma_regs.saddr0;
        dma_reg->daddr0 = dma->dma_regs.daddr0;
        dma_reg->framelen = dma->dma_regs.framelen;

        dma_reg->start = 0;

        dma_reg->ctl = dma->dma_regs.ctl;
    }

    acts_dma_start(dma_reg);
}

int dma_start_request_trans(struct dma_controller *dmac)
{
    struct dma *dma;
    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) dma_global_data_p;

    if (!list_empty(&p_rom_data->dma_req_list)) {
        dma = (struct dma *) list_first_entry(&p_rom_data->dma_req_list, struct dma, list);
        list_del(&dma->list);
        set_dma_data(dmac, dma);
        p_rom_data->dma_api->reg_write_start(dma, dmac->dma_chan_num);

        return 0;
    }

    return -1;
}

static void dma_dump_dma_regs(int dma_chan_no)
{
	struct dma_regs *dma_chan_regs = (struct dma_regs *) DMA_CHAN(DMA_REG_BASE, dma_chan_no);

	printk("dump dma chan %d regs %p :\n", dma_chan_no, dma_chan_regs);
	printk("ctl             = 0x%08x\n", dma_chan_regs->ctl);
	printk("start           = 0x%08x\n", dma_chan_regs->start);
	printk("saddr0          = 0x%08x\n", dma_chan_regs->saddr0);
	printk("daddr0          = 0x%08x\n", dma_chan_regs->daddr0);
	printk("framelen        = 0x%08x\n", dma_chan_regs->framelen);
	printk("frame_remainlen = 0x%08x\n", dma_chan_regs->frame_remainlen);
}

void rom_dma_irq_handle(unsigned int irq)
{
    unsigned int pending;
    unsigned int irq_en;
    int phy_dma_no;
    struct dma_controller *dmac;
    struct dma *dma;
    unsigned int flags;
    void (*dma_irq_handler)(int irq, unsigned int type, void *pdata);

    //rom_dma_data_t *p_rom_data = (rom_dma_data_t *) dma_global_data_p;

    flags = irq_lock();

    if (irq >= IRQ_ID_DMA5)	{
        phy_dma_no = 5 + (irq - IRQ_ID_DMA5);
    } else {
        phy_dma_no = irq - IRQ_ID_DMA0;
    }

    dmac = dma_get_intstatus(phy_dma_no);

    dma = dmac->dma;

	// only clear dma pending of this channel
    pending = (sys_read32(DMAIP) & (0x10001 << phy_dma_no));

    sys_write32(pending, DMAIP);

    irq_en = sys_read32(DMAIE);
    if ((pending & irq_en) == (0x10001 << phy_dma_no)) {
		// If there are half empty and half full IRQs esisted simultaneously, data will possible be crashed.
        printk("error : dma[%d] half & comp irq pending both\n", phy_dma_no);
		dma_dump_dma_regs(phy_dma_no);
    }

    if ((pending & irq_en) & (0x10000 << phy_dma_no)) {
        set_dma_state(dma, DMA_STATE_IRQ_HF);

        //half complete irq
        if (dma->dma_irq_handler) {
            dma_irq_handler = dma->dma_irq_handler;
            irq_unlock(flags);
            dma_irq_handler(irq, DMA_IRQ_HF, dma->pdata);
            flags = irq_lock();
        }
    }
    if ((pending & irq_en) & (0x1 << phy_dma_no)) {
        set_dma_state(dma, DMA_STATE_IRQ_TC);

		// re-schedule  when not reload mode
        if ((dma->dma_regs.ctl & (1 << DMA0CTL_RELOAD)) == 0) {
			// set up the transmission finish flag
            free_dma_xfer(dma);

            if (dma_start_request_trans(dmac) < 0) {
                rom_dma_controller_free(dmac, dma);
                sys_write32(sys_read32(DMAIE) & (~pending), DMAIE);
            }
        } else {
            dma->reload_count++;
        }

		// full irq
        if (dma->dma_irq_handler) {
            dma_irq_handler = dma->dma_irq_handler;
            irq_unlock(flags);
            dma->dma_irq_handler(irq, DMA_IRQ_TC, dma->pdata);
            flags = irq_lock();
        }
    }

    irq_unlock(flags);
 //  return IRQ_HANDLED;
}

int rom_dma_start(int handle)
{
    unsigned int flags;

    int ret_val = 0;

    struct dma *dma;

    struct dma_controller *dmac;

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) dma_global_data_p;

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;

        flags = irq_lock();

        dmac = rom_dma_controller_alloc(dma);

        if (dmac != NULL) {
            p_rom_data->dma_api->reg_write_start(dma, dmac->dma_chan_num);

        } else {
			/* There is no free space inside dma controller, and add this request into the dma request list */
            set_dma_state(dma, DMA_STATE_WAIT);
            list_add_tail(&dma->list, &p_rom_data->dma_req_list);
            ret_val = -1;
        }

        irq_unlock(flags);
    } else {
        p_rom_data->dma_api->reg_write_start(NULL, dma_reg_to_num((dma_reg_t *) handle));
    }

    return ret_val;
}

void rom_dma_abort(int handle)
{
    unsigned int flags;

    struct dma *dma;

    struct dma_controller *dmac;

    dma_reg_t *reg;

    //rom_dma_data_t *p_rom_data = (rom_dma_data_t *) dma_global_data_p;

    if (is_virtual_dma_handle(handle)) {
        flags = irq_lock();

        dma = (struct dma *) handle;

        if (dma == stat_dma){
            stat_dma = NULL;
        }

        if (get_dma_state(dma) == DMA_STATE_WAIT) {
			// remove from dma list
            list_del_init(&dma->list);

            free_dma_xfer(dma);

        } else {
			// check if this channel is allocated by dma controller
            dmac = rom_dma_controller_is_alloc(dma);

            if (dmac != NULL) {
                reg = dma_num_to_reg(dmac->dma_chan_num);

                rom_dma_stop(reg);

                free_dma_xfer(dma);

				// abort one DMA channel and then check if there is request thread which is waitting transmission in request list
                if (dma_start_request_trans(dmac) < 0) {
                    rom_dma_controller_free(dmac, dma);
                }
            }
        }

        irq_unlock(flags);
    } else {
        reg = (dma_reg_t *) handle;

        rom_dma_stop(reg);
    }
}

int rom_dma_config(int handle, uint32_t ctl, dma_irq_handler_t irq_handle, void *pdata)
{
    struct dma *dma;

    dma_reg_t *reg;

    struct dma_wrapper_data *wrapper_data = (struct dma_wrapper_data *) pdata;

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;
        if (irq_handle != NULL) {
            dma->dma_irq_handler = irq_handle;
            dma->pdata = wrapper_data;
        }

        dma->dma_regs.ctl = ctl;
    } else {
        reg = (dma_reg_t *) handle;

        if (irq_handle != NULL) {
			// allow dma interrupt
            if (wrapper_data)
                rom_dma_set_irq(dma_reg_to_num(reg), wrapper_data->complete_callback_en, wrapper_data->half_complete_callback_en);
            else
                rom_dma_set_irq(dma_reg_to_num(reg), 1, 1);
        }

        reg->ctl = ctl;
    }

    return 0;
}

int rom_dma_frame_config(int dma_handle, uint32_t saddr, uint32_t daddr, uint32_t frame_len)
{
    struct dma *dma;
    dma_reg_t *regs;

    saddr = DMA_PSRAM_ADDR_CONVERT(saddr);
    daddr = DMA_PSRAM_ADDR_CONVERT(daddr);

    if (is_virtual_dma_handle(dma_handle)) {
        dma = (struct dma *) dma_handle;

        dma->dma_regs.saddr0 = saddr;
        dma->dma_regs.daddr0 = daddr;
        dma->dma_regs.framelen = frame_len;

    } else {
        regs = (dma_reg_t *) dma_handle;

        regs->saddr0 = saddr;
        regs->daddr0 = daddr;

        regs->framelen = frame_len;
    }

    return 0;
}

void rom_do_dma_irq_if_irq_disabled(void)
{
    unsigned int phy_dma_no;

	/* if called under interrupt, need to handle interrupt response by itself */
    if (_arch_irq_is_locked()) {
        int i;

        for (i = 0; i < dma_global_data_p->dma_num; i++) {
            if (dma_global_data_p->dmacs[i].channel_assigned == 1) {
                phy_dma_no = dma_global_data_p->dmacs[i].dma_chan_num;

				// DMA interrupt enable and also pending bit is set, start to response DMA irq.
                if ((sys_read32(DMAIP) & (0x10001 << phy_dma_no))
                        && ((sys_read32(DMAIE) & (0x10001 << phy_dma_no)))) {
                    if (phy_dma_no >= 5) {
                        rom_dma_irq_handle(IRQ_ID_DMA5 + (phy_dma_no - 5));
                    } else {
                        rom_dma_irq_handle(IRQ_ID_DMA0 + phy_dma_no);
                    }
                }
            }
        }
    }
}

bool rom_dma_query_complete(int dma_handle)
{
    struct dma *dma;

    int ret_val;

    if (is_virtual_dma_handle(dma_handle)) {
        dma_global_data_p->dma_api->dma_irq_dispatch();

        dma = (struct dma *) dma_handle;

        ret_val = (get_dma_state(dma) == DMA_STATE_IDLE);
    } else {
        ret_val = ((((dma_reg_t *) dma_handle)->start & (1 << DMA0START_DMASTART)) == 0);
    }

    if (ret_val == 1){
        return 1;
    }else{
        return 0;
    }
}

int rom_dma_wait_complete(int handle, unsigned int timeout_us)
{
    unsigned int cycles = 0, prev_cycle;
    u32_t timeout_cycles = (u32_t)(
		(u64_t)timeout_us *
		(u64_t)sys_clock_hw_cycles_per_sec /
		(u64_t)USEC_PER_SEC);

    prev_cycle = k_cycle_get_32();

    while ((timeout_us == 0 || cycles < timeout_cycles)) {
        if (rom_dma_query_complete(handle)) {
            break;
        }

        cycles = k_cycle_get_32() - prev_cycle;
    }

    if (timeout_us != 0 && cycles >= timeout_cycles) {
        return -ETIMEDOUT;
    }

    return 0;
}

int acts_dma_pre_start(int handle)
{
    unsigned int flags;

    struct dma *dma;

    struct dma_controller *dmac;

    dma_reg_t *dma_reg = NULL;

    struct dma_wrapper_data *wrapper_data = NULL;

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;

        flags = irq_lock();

        dmac = rom_dma_controller_alloc(dma);

        if (dmac != NULL) {

            dma_reg = dma_num_to_reg(dmac->dma_chan_num);

            if(dma != NULL){
                set_dma_state(dma, DMA_STATE_CONFIG);

                /* vdma depends upon comp irq to update state of complete */
                wrapper_data = (struct dma_wrapper_data *)(dma->pdata);
                if (wrapper_data)
                    rom_dma_set_irq(dmac->dma_chan_num, wrapper_data->complete_callback_en, wrapper_data->half_complete_callback_en);
                else
                    rom_dma_set_irq(dmac->dma_chan_num, 1, 0);

                dma_reg->saddr0 = dma->dma_regs.saddr0;
                dma_reg->daddr0 = dma->dma_regs.daddr0;
                dma_reg->framelen = dma->dma_regs.framelen;

                dma_reg->start = 0;

                dma_reg->ctl = dma->dma_regs.ctl;
            }
        }

        irq_unlock(flags);
    } else{
        dma_reg = (dma_reg_t *)handle;
    }

    return (int)dma_reg;
}


__ramfunc int acts_dma_get_samples(int handle)
{
    dma_reg_t *dma_reg = (dma_reg_t *)handle;

    if(!stat_dma){
        stat_dma = rom_dma_controller_get_dma_handle((u32_t)dma_reg);
    }

    return stat_dma->dma_regs.framelen - dma_reg->frame_remainlen + stat_dma->reload_count * stat_dma->dma_regs.framelen;
}


__ramfunc int acts_dma_get_samples_param(int handle, unsigned int *framelen,unsigned int *remainlen, unsigned short *reload_count)
{
    dma_reg_t *dma_reg = (dma_reg_t *)handle;

    if(!stat_dma){
        stat_dma = rom_dma_controller_get_dma_handle((u32_t)dma_reg);
    }

    if(stat_dma)
    {
        *framelen = stat_dma->dma_regs.framelen;
        *remainlen = dma_reg->frame_remainlen;
        *reload_count = stat_dma->reload_count;
        return 0;
    }
    else
    {
        return -1;
    }

}

int acts_dma_get_transfer_length(int handle)
{
    struct dma *dma;

    struct dma_controller *dmac;

    unsigned int transfer_length = 0;

    dma_reg_t *dma_reg = NULL;

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;

        dmac = rom_dma_controller_alloc(dma);

        if (dmac != NULL) {
            dma_reg = dma_num_to_reg(dmac->dma_chan_num);
            transfer_length = dma->dma_regs.framelen - dma_reg->frame_remainlen + dma->reload_count * dma->dma_regs.framelen;
        }
    } else{
        dma_reg = (dma_reg_t *)handle;

        dma = rom_dma_controller_get_dma_handle((u32_t)dma_reg);

        if(dma){
            transfer_length = dma->dma_regs.framelen - dma_reg->frame_remainlen + dma->reload_count * dma->dma_regs.framelen;
        }
    }

    //printk("remain %d loop %d total %d\n", dma_reg->frame_remainlen, dma->reload_count, transfer_length);

    return transfer_length;
}

int acts_dma_get_phy_channel(int handle)
{
    struct dma *dma;

    struct dma_controller *dmac;

    dma_reg_t *dma_reg = NULL;

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;

        dmac = rom_dma_controller_alloc(dma);

        if (dmac != NULL) {
            dma_reg = dma_num_to_reg(dmac->dma_chan_num);

            return (int)dma_reg;
        }
    } else{
        return handle;
    }

    return 0;
}


const dma_rom_api_t dma_rom_api =
{
    .config = rom_dma_config,
    .frame_config = rom_dma_frame_config,
    .start = rom_dma_start,
    .abort = rom_dma_abort,
    .dma_irq_dispatch = rom_do_dma_irq_if_irq_disabled,
    .query_complete = rom_dma_query_complete,
    .wait_complete = rom_dma_wait_complete,
    .reg_write_start = rom_dma_regs_write_and_start,
};

