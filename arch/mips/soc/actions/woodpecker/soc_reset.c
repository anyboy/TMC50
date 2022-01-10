/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset interface for Actions SoC
 */

#include <kernel.h>
#include <soc.h>

static void acts_reset_peripheral_control(int reset_id, int assert)
{
	unsigned int reg, key;

	if (reset_id > RESET_ID_MAX_ID)
		return;

	reg = RMU_MRCR0 + ((reset_id / 32) * 4);

	key = irq_lock();

	if (assert)
		sys_write32(sys_read32(reg) & ~(1u << (reset_id % 32)), reg);
	else
		sys_write32(sys_read32(reg) | (1u << (reset_id % 32)), reg);

	irq_unlock(key);
}

void acts_reset_peripheral_assert(int reset_id)
{
	acts_reset_peripheral_control(reset_id, 1);
}

void acts_reset_peripheral_deassert(int reset_id)
{
	acts_reset_peripheral_control(reset_id, 0);
}

void acts_reset_peripheral(int reset_id)
{
	acts_reset_peripheral_assert(reset_id);
	acts_reset_peripheral_deassert(reset_id);
}

void acts_deep_sleep_peripheral(char *reset_id_buffer, int buffer_len)
{
    int i;
    int mrcr0, mrcr1;

    mrcr0 = 0;
    mrcr1 = 0;

    for(i = 0; i < buffer_len; i++){
        if(reset_id_buffer[i] >= 32){
            mrcr1 |= (1 << (reset_id_buffer[i] - 32));
        }else{
            mrcr0 |= (1 << reset_id_buffer[i]);
        }    
    }

    sys_write32(mrcr0, RMU_MRCR0);
    sys_write32(mrcr1, RMU_MRCR1);
}

