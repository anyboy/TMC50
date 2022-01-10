/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief actions mpu
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <soc_memctrl.h>
#include <mpu_acts.h>
#include <linker/linker-defs.h>
#include <stack_backtrace.h>

#define SYS_LOG_DOMAIN "mpu"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

extern unsigned int get_user_sp(void);
extern u32_t get_SP(void);

static void _mpu_protect_disable(void)
{
	sys_write32(0, MPUIE_LOW);
	sys_write32(0, MPUIE_HIGH);
}

void mpu_protect_disable(void)
{
#ifdef CONFIG_MPU_EXCEPTION_DRIVEN
	sys_write32(sys_read32(MEMORYCTL) & ~MEMORYCTL_BUSERROR_BIT, MEMORYCTL);
#endif
	_mpu_protect_disable();
}
static void _mpu_clear_all_pending(void)
{
	sys_write32(0xffffffff, MPUIP_LOW);
	sys_write32(0xffffffff, MPUIP_HIGH);
}

static s8_t mpu_enable_count[8];

void mpu_set_address(u32_t start, u32_t end, u32_t flag, u32_t index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;

	base_register += index;

	base_register->MPUBASE = soc_memctrl_cpu_addr_to_bus_addr(start);
	base_register->MPUEND = soc_memctrl_cpu_addr_to_bus_addr(end);
	base_register->MPUCTL = ((flag) << 1);
	if (index < 4) {
		sys_write32((sys_read32(MPUIE_LOW) & ~(0x1f << 8*index)) | (flag << 8*index), MPUIE_LOW);
	} else {
		index -= 4;
		sys_write32((sys_read32(MPUIE_HIGH) & ~(0x1f << 8*index)) | (flag << 8*index), MPUIE_HIGH);
	}
}

static void _mpu_protect_init(void)
{
#ifdef CONFIG_MPU_MONITOR_CACHECODE_WRITE
	/* protect rom section */
	mpu_set_address((unsigned int)&_image_rom_start, (unsigned int)&_image_rom_end - 1,
		(MPU_CPU_WRITE), CONFIG_MPU_MONITOR_CACHECODE_WRITE_INDEX);

	mpu_enable_region(CONFIG_MPU_MONITOR_CACHECODE_WRITE_INDEX);
#endif

#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
	/*  protect ram function  section*/
	mpu_set_address((unsigned int)&_image_text_ramfunc_start, (unsigned int)&_image_text_ramfunc_end - 1,
	(MPU_CPU_WRITE | MPU_DMA_WRITE), CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX);

	mpu_enable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX);
#endif

#ifdef CONFIG_MPU_MONITOR_ROMFUNC_WRITE
	/* protect rom addr section */
	mpu_set_address(CONFIG_FLASH_BASE_ADDRESS, CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_SIZE - 1,
	(MPU_CPU_WRITE), CONFIG_MPU_MONITOR_ROMFUNC_WRITE_INDEX);

	mpu_enable_region(CONFIG_MPU_MONITOR_ROMFUNC_WRITE_INDEX);
#endif

#ifdef CONFIG_MPU_MONITOR_STACK_OVERFLOW
	/* protect stack space section ,default set invalied value */
	mpu_set_address(0, 2048 - 1, MPU_CPU_WRITE, CONFIG_MPU_MONITOR_STACK_OVERFLOW_INDEX);

	mpu_disable_region(CONFIG_MPU_MONITOR_STACK_OVERFLOW_INDEX);
#endif

	_mpu_clear_all_pending();
}

static int mpu_check_thread_stack(void)
{
#if 0
	int len;
	u32_t stack_cur;
	u32_t stack_start;
	struct k_thread *thread = k_current_get();

	stack_start = thread->stack_info.start;

#ifdef CONFIG_MPU_EXCEPTION_DRIVEN
	stack_cur = __get_SP();
#endif

#ifdef CONFIG_MPU_IRQ_DRIVEN
	stack_cur = get_user_sp();
#endif

	len =  stack_cur - stack_start;

	printk("current thread sp %x start %x\n", stack_cur, stack_start);

	return len;
#endif
	return 0;
}

static int mpu_ignore_invalid_dma_fault(void)
{
	unsigned int pending, offset, addr, start, end, dst, size;
	unsigned int ip_regs[2], i;

    /* return no MPU pending*/
	if (!sys_read32(MPUIP_LOW) && !sys_read32(MPUIP_HIGH)) {
		return 1;
	}

    /* return no DMA pending*/
	if (((sys_read32(MPUIP_LOW) & 0x08080808) == 0) && ((sys_read32(MPUIP_HIGH) & 0x08080808) == 0)) {
		return 0;
	}

	ip_regs[0] = MPUIP_LOW;
	ip_regs[1] = MPUIP_HIGH;

	for (i = 0; i < 2; i++) {
		pending = sys_read32(ip_regs[i]);

		offset = (u32_t)MPUCTL0;
		while (pending) {
			if (pending & 0x08)
				break;

			pending >>= 8;
			offset += 16;
		}

		/* read error addr  */
		addr = sys_read32(offset + 12) & (u32_t)0xfffff;
		start = sys_read32(offset + 4) & (u32_t)0xfffff;
		end = sys_read32(offset + 8) & (u32_t)0xfffff;

	    /* check error addr in region */
		if (addr >= start && addr <= end) {
			offset = (u32_t)DMA0DADDR0;
			/* check last dma */
			while (offset <= (u32_t)(DMA0DADDR0 + DMA_ACTS_MAX_CHANNELS * DMA_CHAN_OFFS)) {

			    dst = sys_read32(offset) & (u32_t)0xfffff;
				size = sys_read32(offset + 8);
				if (addr >= dst && addr < (dst + size))
					return -1;

			    offset += 0x100;
			}
		}

		sys_write32(0x08080808, ip_regs[i]);
	}

	return 1;
}


static int mpu_analyse(void)
{
	int retval = 0;

#if CONFIG_SYS_LOG_DEFAULT_LEVEL  > 0
	static const char * const access_reason[] = {
		"cpu ir write",
		"cpu write",
		"dma write",
	};
#endif

	unsigned int mpux_pending, addr, offset, dst, size;
	int  i, dma, len;
	bool stack_overflow_check;
	mpu_base_register_t *base_register;
	struct dma_regs *dma_base_regs;

	_mpu_protect_disable();

	base_register = (mpu_base_register_t *)MPUCTL0;

	for (i = 0; i < CONFIG_MPU_ACTS_MAX_INDEX; i++) {
		if (i < 4) {
			mpux_pending = (sys_read32(MPUIP_LOW) >> (i*8)) & 0x1f;
		} else {
			mpux_pending = (sys_read32(MPUIP_HIGH) >> ((i-4)*8)) & 0x1f;
		}

		if (!mpux_pending)
			continue;

		addr = base_register[i].MPUERRADDR;

		if (mpux_pending & MPU_CPU_WRITE) {
			stack_overflow_check = false;
#ifdef CONFIG_MPU_MONITOR_STACK_OVERFLOW
			if (i == CONFIG_MPU_MONITOR_STACK_OVERFLOW_INDEX)
				stack_overflow_check = true;
#endif
			if (stack_overflow_check) {
				len = mpu_check_thread_stack();
				if (len >= 0) {
					SYS_LOG_ERR("Warning:task%d\' stack only %d bytes left!\n", k_thread_priority_get(k_current_get()), len);
					dump_stack();
					continue;
				} else {
					retval = 1;
					SYS_LOG_ERR("<BUG>:task%d\' stack overflow %d bytes!\n", k_thread_priority_get(k_current_get()), len * -1);
				}
			} else {
				retval = 1;
				SYS_LOG_ERR("<BUG> %s addr 0x%x\n", access_reason[1], addr);
			}
		} else /*if(mpux_pending & MPU_DMA_WRITE)*/ {
			if (mpu_ignore_invalid_dma_fault() > 0)
				continue;
			for (dma = 0; dma < DMA_ACTS_MAX_CHANNELS; dma++) {
				dma_base_regs = (struct dma_regs *) DMA_CHAN(DMA_REG_BASE, dma);
				dst = dma_base_regs->daddr0 & (u32_t)0xfffff;
				size = dma_base_regs->framelen;
				offset = addr & (u32_t)0xfffff;
				if (offset >= dst && offset < (dst + size)) {
				    retval = 1;
				    SYS_LOG_ERR("<BUG> %s addr 0x%x, dma%d: dst=0x%x size=%d\n", access_reason[2], addr, dma, dst, size);
					break;
				}
			}
		}

		#ifndef CONFIG_MPU_EXCEPTION_DRIVEN
		panic("mpu error\n");
		#endif

	}

	return retval;
}

void mpu_protect_clear_pending(int mpu_no)
{
	if (mpu_no < 4) {
		sys_write32(0x1f << (8 * mpu_no), MPUIP_LOW);
	} else {
		mpu_no -= 4;
		sys_write32(0x1f << (8 * mpu_no), MPUIP_HIGH);
	}
}

__ramfunc void mpu_enable_region(unsigned int index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;
	u32_t flags;

	flags = irq_lock();
	if (mpu_enable_count[index] == 0) {
		base_register += index;

		base_register->MPUCTL |= (0x01);
	}
	mpu_enable_count[index]++;
	irq_unlock(flags);
}

__ramfunc void mpu_disable_region(unsigned int index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;
	u32_t flags;

	flags = irq_lock();
	mpu_enable_count[index]--;
	if (mpu_enable_count[index] == 0) {
		base_register += index;

		base_register->MPUCTL &= (~(0x01));
	}
	irq_unlock(flags);
}

int mpu_region_is_enable(unsigned int index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;

	base_register += index;

	return ((base_register->MPUCTL & 0x01) == 1);
}

#ifdef CONFIG_MPU_MONITOR_USER_DATA
int mpu_user_data_monitor(unsigned int start_addr, unsigned int end_addr, int mpu_user_no)
{
    /* protect text section*/
	mpu_set_address((unsigned int)start_addr, (unsigned int)end_addr - 1,
		(MPU_CPU_WRITE | MPU_DMA_WRITE), CONFIG_MPU_MONITOR_USER_DATA_INDEX + mpu_user_no);

	mpu_enable_region(CONFIG_MPU_MONITOR_USER_DATA_INDEX + mpu_user_no);

	return 0;
}

int mpu_user_data_monitor_stop(int mpu_user_no)
{
	mpu_disable_region(CONFIG_MPU_MONITOR_USER_DATA_INDEX + mpu_user_no);

	return 0;
}
#endif

int mpu_handler(void)
{
	int ret = 0;
	/* printk("!!!mpu enter: %x\n", MEMORY_CONTROLLER_REGISTER_2.MPUIP); */
    /* do_mpu_ignore_task_stack(); */
	if (mpu_ignore_invalid_dma_fault() <= 0) {
		ret = mpu_analyse();
	}
	_mpu_clear_all_pending();

	return ret;
}

int mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_mpu_protect_init();

	_mpu_clear_all_pending();

	SYS_LOG_INF("mpu init\n");

#ifdef CONFIG_MPU_IRQ_DRIVEN
	IRQ_CONNECT(IRQ_ID_MPU, CONFIG_IRQ_PRIO_HIGHEST, mpu_handler, 0, 0);
	irq_enable(IRQ_ID_MPU);
#endif

#ifdef CONFIG_MPU_EXCEPTION_DRIVEN
	sys_write32(sys_read32(MEMORYCTL) | MEMORYCTL_BUSERROR_BIT, MEMORYCTL);
#endif

	return 0;
}

SYS_INIT(mpu_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);


