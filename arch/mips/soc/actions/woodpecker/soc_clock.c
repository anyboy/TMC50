/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock interface for Actions SoC
 */

#include <kernel.h>
#include <device.h>
#include <soc.h>

static void acts_clock_peripheral_control(int clock_id, int enable)
{
	unsigned int reg, key;

	if (clock_id > CLOCK_ID_MAX_ID)
		return;

	reg = CMU_DEVCLKEN0 + ((clock_id / 32) * 4);

	key = irq_lock();

	if (enable)
		sys_write32(sys_read32(reg) | (1u << (clock_id % 32)), reg);
	else
		sys_write32(sys_read32(reg) & ~(1u << (clock_id % 32)), reg);

	irq_unlock(key);
}

void acts_clock_peripheral_enable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 1);
}

void acts_clock_peripheral_disable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 0);
}

void acts_deep_sleep_clock_peripheral(const char *id_buffer, int buffer_len)
{
    int i;
    int devclk0, devclk1;

    devclk0 = 0;
    devclk1 = 0;

    for(i = 0; i < buffer_len; i++){
        if(id_buffer[i] >= 32){
            devclk1 |= (1 << (id_buffer[i] - 32));
        }else{
            devclk0 |= (1 << id_buffer[i]);
        }    
    }

    sys_write32(devclk0, CMU_DEVCLKEN0);
    sys_write32(devclk1, CMU_DEVCLKEN1);
}

