/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/util.h>
#include <dsp.h>
#include "dsp_inner.h"
#include "dsp_image.h"

int dsp_acts_request_image(struct device *dev, const struct dsp_imageinfo *image, int type)
{
	struct dsp_acts_data *dsp_data = dev->driver_data;

	if (type >= ARRAY_SIZE(dsp_data->images)) {
		printk("%s: dsp image <%s> invalid type %d\n", __func__, image->name, type);
		return -EINVAL;
	}

	uint32_t entry_point = UINT32_MAX;
	if (load_dsp_image(image->ptr, image->size, &entry_point)) {
		printk("%s: cannot load dsp image <%s>\n", __func__, image->name);
		return -EINVAL;
	}

	printk("%s: dsp image <%s> loaded, type=%d, entry_point=0x%x\n",
			__func__, image->name, type, entry_point);

	if (type == DSP_IMAGE_MAIN) {
		clear_dsp_pageaddr();

		/* set DSP_VECTOR_ADDRESS */
		set_dsp_vector_addr(entry_point);
	}

	memcpy(&dsp_data->images[type], image, sizeof(*image));
	return 0;
}

int dsp_acts_release_image(struct device *dev, int type)
{
	struct dsp_acts_data *dsp_data = dev->driver_data;

	if (type >= ARRAY_SIZE(dsp_data->images))
		return -EINVAL;

	printk("%s: dsp image <%s> free, type=%d\n",
		__func__, dsp_data->images[type].name, type);
	memset(&dsp_data->images[type], 0, sizeof(dsp_data->images[0]));

	return 0;
}

int dsp_acts_handle_image_pagemiss(struct device *dev, u32_t epc)
{
	unsigned int bank_group = IMG_BANK_GROUP(epc);
	unsigned int bank_index = IMG_BANK_INDEX(epc);
	unsigned int bank_no = BANK_NO(bank_group, bank_index);
	unsigned int type = IMG_TYPE(epc);
	struct dsp_acts_data *dsp_data = dev->driver_data;
	const struct dsp_imageinfo *image = &dsp_data->images[type];

	if (type >= DSP_NUM_IMAGE_TYPES || image == NULL) {
		printk("%s: invalid epc 0x%08x\n", __func__, epc);
		return -EFAULT;
	}

	printk("%s: dsp image <%s> page miss, epc=0x%08x, type=%x, bank_group=%u, bank_index=%u\n",
		   __func__, image->name, epc, type, bank_group, bank_index);

	if (load_dsp_image_bank(image->ptr, image->size, bank_no)) {
		printk("cannot load bank[%u] for epc 0x%08x\n", bank_no, epc);
		return DSP_FAIL;
	}

	/* epc is word unit */
	unsigned int page_addr = (epc * 2) & 0xfff80000;

	//check tlb is bit19~bit31
	if (sys_read32(DSP_PAGE_ADDR0 + (bank_group << 2)) == page_addr)
		return -EBUSY;
	else
		sys_write32(page_addr, DSP_PAGE_ADDR0 + (bank_group << 2));

	return DSP_DONE;
}

int dsp_acts_handle_image_pageflush(struct device *dev, u32_t epc)
{
	unsigned int bank_group = IMG_BANK_GROUP(epc);
	unsigned int bank_index = IMG_BANK_INDEX(epc);
	unsigned int type = IMG_TYPE(epc);
	struct dsp_acts_data *dsp_data = dev->driver_data;
	const struct dsp_imageinfo *image = &dsp_data->images[type];

	if (type >= DSP_NUM_IMAGE_TYPES || image == NULL) {
		printk("%s: invalid epc 0x%08x\n", __func__, epc);
		return -EFAULT;
	}

	printk("%s: dsp image <%s> page flush, epc=0x%08x, type=%x, bank_group=%u, bank_index=%u\n",
		   __func__, image->name, epc, type, bank_group, bank_index);

    sys_write32(0, DSP_PAGE_ADDR0 + (bank_group << 2)); 

    return DSP_DONE;	
}
