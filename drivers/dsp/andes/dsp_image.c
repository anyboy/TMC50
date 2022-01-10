/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/util.h>

#ifdef LOAD_IMAGE_FROM_FS
#include <sys/types.h>
#include <fs/fs_interface.h>
#include <fs/fat_fs.h>
#include <fs.h>
#endif /* LOAD_IMAGE_FROM_FS */

#include "dsp_image.h"

#ifdef CONFIG_MIPS
#define CPU_BASE 0xBFC00000
#else
#define CPU_BASE 0x00000000
#endif

struct dsp2cpu_table {
	uint32_t dsp_base;
	uint32_t cpu_base;
	uint32_t length;
};

static const struct dsp2cpu_table dsp2cpu_table[] = {
	/* DSP SRAM */
	{0x00000, CPU_BASE + 0x60000, 0x1BFFF},
	{0x10000, CPU_BASE + 0x40000, 0x1FFFF},
	{0x40000000, CPU_BASE + 0x30000, 0x89FF},
#ifdef CONFIG_SOC_MAPPING_PSRAM_FOR_OS
	/* PSRAM code & data */
	{0x48000000, CONFIG_SOC_MAPPING_PSRAM_ADDR, CONFIG_SOC_MAPPING_PSRAM_SIZE},
#endif
};

uint32_t addr_cpu2dsp(uint32_t addr, bool is_code)
{
	uint32_t dsp_addr = 0;
	for (int i = 0; i < ARRAY_SIZE(dsp2cpu_table); i++) {
		if (addr >= dsp2cpu_table[i].cpu_base &&
			(addr - dsp2cpu_table[i].cpu_base) < dsp2cpu_table[i].length) {
			dsp_addr = dsp2cpu_table[i].dsp_base + (addr - dsp2cpu_table[i].cpu_base) / 2;
			break;
		}
	}

	/**data addr = code addr + 0x20000 Except for psram addr*/
	if (!is_code && dsp_addr < 0x48000000) {
		dsp_addr += 0x20000;
	}

	return dsp_addr;
}

uint32_t addr_dsp2cpu(uint32_t addr, bool is_code)
{

	/* just whether is psram address */
	if ((addr & BIT(30)) == 0)
		addr &= BIT_MASK(IMG_BANK_INDEX_SHIFT);

	/**data addr = code addr + 0x20000 Except for psram addr*/
	if (!is_code && addr < 0x48000000 && addr != 0) {
		addr -= 0x20000;
	}

	for (int i = 0; i < ARRAY_SIZE(dsp2cpu_table); i++) {
		if (addr >= dsp2cpu_table[i].dsp_base &&
			(addr - dsp2cpu_table[i].dsp_base) < dsp2cpu_table[i].length / 2)
			return dsp2cpu_table[i].cpu_base + (addr - dsp2cpu_table[i].dsp_base) * 2;
	}

	printk("invalid dsp addr 0x%x\n", addr);
	return UINT32_MAX;
}

static int load_scntable(const void *image, unsigned int offset, bool is_code)
{
	const struct IMG_scnhdr *scnhdr = image + offset;

	if (scnhdr->size == 0)
		return -ENODATA;

	for (; scnhdr->size > 0; ) {
		uint32_t cpu_addr = addr_dsp2cpu(scnhdr->addr, is_code);
		if (cpu_addr == UINT32_MAX) {
			printk("invalid address 0x%08x\n", scnhdr->addr);
			return -EFAULT;
		}

		//printk("load: offset=0x%08x, size=0x%08x, dsp_addr=0x%08x, cpu_addr=0x%08x\n",
		//	   (unsigned int)(scnhdr->data - (const uint8_t *)image),
		//	   scnhdr->size, scnhdr->addr, cpu_addr);

		memcpy((void*)cpu_addr, scnhdr->data, scnhdr->size);
		scnhdr = (struct IMG_scnhdr*)&scnhdr->data[scnhdr->size];
	}

	return 0;
}

int load_dsp_image_bank(const void *image, size_t size, unsigned int bank_no)
{
	const struct IMG_filehdr *filehdr = image;

	if (bank_no >= NUM_BANKS) {
		printk("invalid bank number %u\n", bank_no);
		return -EINVAL;
	}

	assert(size > filehdr->bank_scnptr[bank_no][0]);
	assert(size > filehdr->bank_scnptr[bank_no][1]);

	if (!filehdr->bank_scnptr[bank_no][0] &&
		!filehdr->bank_scnptr[bank_no][1]) {
		printk("bank[%u] empty\n", bank_no);
		return -ENODATA;
	}

	if (filehdr->bank_scnptr[bank_no][0]) {
		if (load_scntable(image, filehdr->bank_scnptr[bank_no][0], true)) {
			printk("failed to load bank[%u] code\n", bank_no);
			return -EINVAL;
		}
	}

	if (filehdr->bank_scnptr[bank_no][1]) {
		if (load_scntable(image, filehdr->bank_scnptr[bank_no][1], false)) {
			printk("failed to load bank[%u] data\n", bank_no);
			return -EINVAL;
		}
	}

	return 0;
}

int load_dsp_image(const void *image, size_t size, uint32_t *entry_point)
{
	const struct IMG_filehdr *filehdr = image;

	/* FIXME: use sys_get_le32 to handle unaligned 32bit access insead ? */
	if ((uint32_t)image & 0x3) {
		printk("image load address %p should be aligned to 4, "
			"could use __attribute__((__aligned__(x)))\n", image);
		return -EFAULT;
	}

	assert(size > sizeof(*filehdr));
	assert(size > filehdr->code_scnptr);

	if (filehdr->magic != IMAGE_MAGIC('y', 'q', 'h', 'x')) {
		printk("invalid dsp magic 0x%08x\n", filehdr->magic);
		return -EINVAL;
	}

	if (filehdr->code_scnptr) {
		if (load_scntable(image, filehdr->code_scnptr, true)) {
			printk("failed to load code\n");
			return -EINVAL;
		}
	}

	if (filehdr->data_scnptr) {
		if (load_scntable(image, filehdr->data_scnptr, false)) {
			printk("failed to load data\n");
			return -EINVAL;
		}
	}

	if (entry_point)
		*entry_point = filehdr->entry_point;

	return 0;
}

#ifdef LOAD_IMAGE_FROM_FS

static int load_scntable_from_file(fs_file_t *file, long offset, bool is_code)
{
	if (offset <= 0)
		return 0;

	if (fs_seek(file, offset, FS_SEEK_SET)) {
		printk("seek_file failed\n");
		return -EIO;
	}

	do {
		struct IMG_scnhdr scnhdr;
		ssize_t ssize = fs_read(file, &scnhdr, sizeof(scnhdr));
		if (ssize < sizeof(scnhdr.size)) {
			printk("invalid image\n");
			return -EINVAL;
		}

		if (scnhdr.size == 0)
			break;

		uint32_t cpu_addr = addr_dsp2cpu(scnhdr.addr, is_code);
		if (cpu_addr == UINT32_MAX) {
			printk("invalid address 0x%08x\n", scnhdr.addr);
			return -EFAULT;
		}

		//printk("load: offset=0x%08x, size=0x%08x, dsp_addr=0x%08x, cpu_addr=0x%08x",
		//		(unsigned int)fs_tell(file), scnhdr.size, scnhdr.addr, cpu_addr);

		ssize = fs_read(file, (void *)cpu_addr, scnhdr.size);
		if (ssize < scnhdr.size)
			return -EIO;
	} while (true);

	return 0;
}

int load_dsp_image_bank_from_file(void *filp, unsigned int bank_no)
{
	fs_file_t *file = filp;
	struct IMG_filehdr filehdr;
	ssize_t ssize;

	fs_seek(file, 0, FS_SEEK_SET);

	ssize = fs_read(file, &filehdr, sizeof(filehdr));
	if (ssize < sizeof(filehdr)) {
		printk("invalid image\n");
		return -EINVAL;
	}

	if (!filehdr.bank_scnptr[bank_no][0] && !filehdr.bank_scnptr[bank_no][1]) {
		printk("bank[%u] empty\n", bank_no);
		return -ENODATA;
	}

	if (filehdr.bank_scnptr[bank_no][0]) {
		if (load_scntable_from_file(file, filehdr.bank_scnptr[bank_no][0], true)) {
			printk("failed to load bank[%u] code\n", bank_no);
			return -EINVAL;
		}
	}

	if (filehdr.bank_scnptr[bank_no][1]) {
		if (load_scntable_from_file(file, filehdr.bank_scnptr[bank_no][1], false)) {
			printk("failed to load bank[%u] data\n", bank_no);
			return -EINVAL;
		}
	}

	return 0;
}

int load_dsp_image_from_file(void *filp, uint32_t *entry_point)
{
	fs_file_t *file = filp;
	struct IMG_filehdr filehdr;
	ssize_t ssize;

	fs_seek(file, 0, FS_SEEK_SET);

	ssize = fs_read(file, &filehdr, sizeof(filehdr));
	if (ssize < sizeof(filehdr)) {
		printk("invalid image\n");
		return -EINVAL;
	}

	if (memcmp(filehdr.magic, "yqhx", 4)) {
		printk("dsp magic error\n");
		return -EINVAL;
	}

	if (filehdr.code_scnptr) {
		if (load_scntable_from_file(file, filehdr.code_scnptr, true)) {
			printk("failed to load code\n");
			return -EINVAL;
		}
	}

	if (filehdr.data_scnptr) {
		if (load_scntable_from_file(file, filehdr.data_scnptr, false)) {
			printk("failed to load data\n");
			return -EINVAL;
		}
	}

	if (entry_point)
		*entry_point = filehdr.entry_point;

	return 0;
}

#endif /* LOAD_IMAGE_FROM_FS */
