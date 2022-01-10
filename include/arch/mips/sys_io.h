/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MIPS M4K Series memory mapped register I/O operations
 */

#ifndef _MIPS_SYS_IO_H_
#define _MIPS_SYS_IO_H_

#if !defined(_ASMLANGUAGE)

#include <sys_io.h>

/* Memory mapped registers I/O functions */

static inline uint32_t sys_read32(mem_addr_t addr)
{
	return *(volatile uint32_t *)addr;
}


static inline void sys_write32(uint32_t data, mem_addr_t addr)
{
	*(volatile uint32_t *)addr = data;
}

static inline u8_t sys_read8(mem_addr_t addr)
{
	return *(volatile u8_t *)addr;
}

static inline void sys_write8(u8_t data, mem_addr_t addr)
{
	*(volatile u8_t *)addr = data;
}


/* Memory bit manipulation functions */

static inline void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp | (1 << bit);
}

static inline void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp & ~(1 << bit);
}


static inline int sys_test_bit(u32_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	return temp & (1 << bit);
}

static inline int sys_test_and_set_bit(u32_t addr, unsigned int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_set_bit(addr, bit);

	return ret;
}

static inline int sys_test_and_clear_bit(u32_t addr, unsigned int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_clear_bit(addr, bit);

	return ret;
}

static inline void sys_bitfield_set_bit(u32_t addr, unsigned int bit)
{
	/* Doing memory offsets in terms of 32-bit values to prevent
	 * alignment issues
	 */
	sys_set_bit(addr + ((bit >> 5) << 2), bit & 0x1F);
}

static inline void sys_bitfield_clear_bit(u32_t addr, unsigned int bit)
{
	sys_clear_bit(addr + ((bit >> 5) << 2), bit & 0x1F);
}

static inline int sys_bitfield_test_bit(u32_t addr, unsigned int bit)
{
	return sys_test_bit(addr + ((bit >> 5) << 2), bit & 0x1F);
}


static inline int sys_bitfield_test_and_set_bit(u32_t addr, unsigned int bit)
{
	int ret;

	ret = sys_bitfield_test_bit(addr, bit);
	sys_bitfield_set_bit(addr, bit);

	return ret;
}

static inline int sys_bitfield_test_and_clear_bit(u32_t addr, unsigned int bit)
{
	int ret;

	ret = sys_bitfield_test_bit(addr, bit);
	sys_bitfield_clear_bit(addr, bit);

	return ret;
}
#endif /* !_ASMLANGUAGE */

#endif /* _MIPS_SYS_IO_H_ */
