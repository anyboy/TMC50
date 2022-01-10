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
 * \brief   DMA physical driver
 * \author
 * \version  1.0
 * \date  2018-1-5-3:23:20
 *******************************************************************************/
#include "rom_vdma_andes.h"

static struct dma *stat_dma;

extern unsigned int __get_PSR(void);

static inline uint32_t DMA_PSRAM_ADDR_CONVERT(unsigned int cpu_addr)
{
    u32_t psram_bus_addr;

#ifdef CONFIG_MIPS
    psram_bus_addr = cpu_addr & ~0xe0000000;
#else
    psram_bus_addr = cpu_addr;
#endif

    if(psram_bus_addr >= sys_read32(SPI1_DCACHE_START_ADDR) &&
       psram_bus_addr < sys_read32(SPI1_DCACHE_END_ADDR)) {
        psram_bus_addr -= sys_read32(SPI1_DCACHE_START_ADDR);
        psram_bus_addr += PSRAM_DMA_BASE_ADDR;

	return psram_bus_addr;
    }

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

void rom_dma_set_irq(unsigned int dma_no, unsigned int irq_enable)
{
    if (irq_enable) {
        sys_write32(sys_read32(DMAIE) | (0x101 << dma_no), DMAIE);

        sys_write32((0x101 << dma_no), DMAIP);
    } else {
        sys_write32(sys_read32(DMAIE) & (~(0x101 << dma_no)), DMAIE);

        sys_write32((0x101 << dma_no), DMAIP);

    }
}

void acts_dma_start(dma_reg_t *dma_reg)
{
    //if(dma_reg->framelen >= 32){
    //    dma_reg->ctl &= (~(1 << DMA0CTL_TRM));
    //}else{
    //    dma_reg->ctl |= (1 << DMA0CTL_TRM);
    //}

    dma_reg->start |= (1 << DMA0START_DMASTART);
}

void acts_dma_stop(dma_reg_t *dma_reg)
{
    u32_t reg_ctl = dma_reg->ctl;

    dma_reg->ctl &= (~(1 << DMA0CTL_RELOAD));

    dma_reg->start &= (~(1 << DMA0START_DMASTART));

    rom_dma_set_irq(dma_reg_to_num(dma_reg), 1);

    dma_reg->ctl |= (reg_ctl & (1 << DMA0CTL_RELOAD));
}

void rom_dma_stop(dma_reg_t *dma_reg)
{
    dma_reg->ctl &= (~(1 << DMA0CTL_RELOAD));

    dma_reg->start &= (~(1 << DMA0START_DMASTART));

    rom_dma_set_irq(dma_reg_to_num(dma_reg), 0);
}

static void set_dma_data(struct dma_controller *dmac, struct dma *dma)
{
    dmac->dma = dma;
    dma->dmac_num = dmac - rom_dma_global_p->dmacs;
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

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) rom_dma_global_p;

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

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) rom_dma_global_p;

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

    dmac = rom_dma_global_p->dmacs + dma->dmac_num;

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

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) rom_dma_global_p;

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

    if(dma != NULL){
        set_dma_state(dma, DMA_STATE_CONFIG);

        rom_dma_set_irq(dma_no, 1);

        dma_reg->saddr0 = dma->dma_regs.saddr0;
        dma_reg->daddr0 = dma->dma_regs.daddr0;
        dma_reg->framelen = dma->dma_regs.framelen;

        if (dma->dma_regs.ctl & (1 << DMA0CTL_AUDIOTYPE)) {
            //separated stored in the memory
            if ((dma->dma_regs.ctl & (0xf << 8)) == 0) {
                dma_reg->daddr1 = dma->dma_regs.saddr0;
            } else {
                dma_reg->saddr1 = dma->dma_regs.daddr0;
            }
        }

        dma_reg->start = 0;

        dma_reg->ctl = dma->dma_regs.ctl;
    }

    acts_dma_start(dma_reg);
}

int dma_start_request_trans(struct dma_controller *dmac)
{
    struct dma *dma;
    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) rom_dma_global_p;

    if (!list_empty(&p_rom_data->dma_req_list)) {
        dma = (struct dma *) list_first_entry(&p_rom_data->dma_req_list, struct dma, list);
        list_del(&dma->list);
        set_dma_data(dmac, dma);
        p_rom_data->dma_api->reg_write_start(dma, dmac->dma_chan_num);

        return 0;
    }

    return -1;
}

void rom_dma_irq_handle(unsigned int irq)
{
    unsigned int pending;
    int phy_dma_no;
    struct dma_controller *dmac;
    struct dma *dma;
    unsigned int flags;
    void (*dma_irq_handler)(int irq, unsigned int type, void *pdata);

    //rom_dma_data_t *p_rom_data = (rom_dma_data_t *) rom_dma_global_p;

    flags = irq_lock();

    phy_dma_no = irq - IRQ_ID_DMA0;

    dmac = dma_get_intstatus(phy_dma_no);

    dma = dmac->dma;

    pending = (sys_read32(DMAIP) & (0x101 << phy_dma_no));

    sys_write32(pending, DMAIP);

    if (pending & (0x100 << phy_dma_no)) {
        set_dma_state(dma, DMA_STATE_IRQ_HF);

        if (dma->dma_irq_handler) {
            dma_irq_handler = dma->dma_irq_handler;
            irq_unlock(flags);
            dma_irq_handler(irq, DMA_IRQ_HF, dma->pdata);
            flags = irq_lock();
        }
    }
    if (pending & (0x1 << phy_dma_no)) {
        set_dma_state(dma, DMA_STATE_IRQ_TC);

        if ((dma->dma_regs.ctl & (1 << DMA0CTL_RELOAD)) == 0) {
            free_dma_xfer(dma);

            if (dma_start_request_trans(dmac) < 0) {
                rom_dma_controller_free(dmac, dma);
                sys_write32(sys_read32(DMAIE) & (~pending), DMAIE);
            }
        } else {
            dma->reload_count++;
        }

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

    rom_dma_data_t *p_rom_data = (rom_dma_data_t *) rom_dma_global_p;

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;

        flags = irq_lock();

        dmac = rom_dma_controller_alloc(dma);

        if (dmac != NULL) {
            p_rom_data->dma_api->reg_write_start(dma, dmac->dma_chan_num);

        } else {
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

    //rom_dma_data_t *p_rom_data = (rom_dma_data_t *) rom_dma_global_p;

    if (is_virtual_dma_handle(handle)) {
        flags = irq_lock();

        dma = (struct dma *) handle;

        if (dma == stat_dma){
            stat_dma = NULL;
        }

        if (get_dma_state(dma) == DMA_STATE_WAIT) {
            list_del_init(&dma->list);

            free_dma_xfer(dma);

        } else {

            dmac = rom_dma_controller_is_alloc(dma);

            if (dmac != NULL) {
                reg = dma_num_to_reg(dmac->dma_chan_num);

                rom_dma_stop(reg);

                free_dma_xfer(dma);

                if (dma_start_request_trans(dmac) < 0) {
                    rom_dma_controller_free(dmac, dma);
                }
            }
        }

        irq_unlock(flags);
    } else {
        reg = (dma_reg_t *) handle;

        rom_dma_stop(reg);

        //p_rom_data->free_irq(IRQ_ID_DMA0 + dma_reg_to_num(reg));
    }
}

int rom_dma_config(int handle, uint32_t ctl, dma_irq_handler_t irq_handle, void *pdata)
{
    struct dma *dma;

    dma_reg_t *reg;

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;
        if (irq_handle != NULL) {
            dma->dma_irq_handler = irq_handle;
            dma->pdata = pdata;
        }

        dma->dma_regs.ctl = ctl;
    } else {
        reg = (dma_reg_t *) handle;

        if (irq_handle != NULL) {
            rom_dma_set_irq(dma_reg_to_num(reg), 1);
            //rom_global_p->request_irq(IRQ_ID_DMA0 + dma_reg_to_num(reg), (irq_handler_t) irq_handle);
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

        if (regs->ctl & (1 << DMA0CTL_AUDIOTYPE)) {
            //separated stored in the memory
            if ((regs->ctl & (0xf << 8)) != 0) {
                regs->saddr1 = saddr;
            }
        }

        regs->daddr0 = daddr;

        regs->framelen = frame_len;
    }

    return 0;
}

void rom_do_dma_irq_if_irq_disabled(void)
{
    unsigned int phy_dma_no;

    if (_arch_irq_is_locked()) {
        int i;

        for (i = 0; i < rom_dma_global_p->dma_num; i++) {
            if (rom_dma_global_p->dmacs[i].channel_assigned == 1) {
                phy_dma_no = rom_dma_global_p->dmacs[i].dma_chan_num;

                if ((sys_read32(DMAIP) & (0x101 << phy_dma_no))
                        && ((sys_read32(DMAIE) & (0x101 << phy_dma_no)))) {
                    rom_dma_irq_handle(IRQ_ID_DMA0 + phy_dma_no);
                }
            }
        }
    }
}

#define MEMORY_CTL     (0xc00a0000)

bool rom_dma_query_complete(int dma_handle)
{
    struct dma *dma;

    int ret_val;

    if (is_virtual_dma_handle(dma_handle)) {
        rom_dma_global_p->dma_api->dma_irq_dispatch();

        dma = (struct dma *) dma_handle;

        ret_val = (get_dma_state(dma) == DMA_STATE_IDLE);
    } else {
        ret_val = ((((dma_reg_t *) dma_handle)->start & 0x01) == 0);
    }

    if (ret_val == 1){
        return ((sys_read32(MEMORY_CTL) & (1 << 25)) == 0);
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

    if (is_virtual_dma_handle(handle)) {
        dma = (struct dma *) handle;

        flags = irq_lock();

        dmac = rom_dma_controller_alloc(dma);

        if (dmac != NULL) {

            dma_reg = dma_num_to_reg(dmac->dma_chan_num);

            if(dma != NULL){
                set_dma_state(dma, DMA_STATE_CONFIG);

                rom_dma_set_irq(dmac->dma_chan_num, 1);

                dma_reg->saddr0 = dma->dma_regs.saddr0;
                dma_reg->daddr0 = dma->dma_regs.daddr0;
                dma_reg->framelen = dma->dma_regs.framelen;

                if (dma->dma_regs.ctl & (1 << DMA0CTL_AUDIOTYPE)) {
                    //separated stored in the memory
                    if ((dma->dma_regs.ctl & (0xf << 8)) == 0) {
                        dma_reg->daddr1 = dma->dma_regs.saddr0;
                    } else {
                        dma_reg->saddr1 = dma->dma_regs.daddr0;
                    }
                }

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

