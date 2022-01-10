#include "heap.h"

//#define NDEBUG

uint32_t dspram_convert_to_dsp_index(void *dsp_addr, unsigned int dsp_ram_size, int *first_page, int *last_page)
{
	if((dsp_addr < DSP_ADDR || dsp_addr + dsp_ram_size > DSP_ADDR + DSP_SIZE)
	   && (dsp_addr < HW_RAM_ADDR || dsp_addr + dsp_ram_size > HW_RAM_ADDR + HW_RAM_SIZE))
	{
		return -1;
    }

	if(dsp_addr >= DSP_ADDR && dsp_addr < DSP_ADDR + DSP_SIZE){
    	*first_page = (unsigned int)(dsp_addr - DSP_ADDR) / PAGE_SIZE;
    	*last_page = (unsigned int)(dsp_addr + dsp_ram_size - 1 - DSP_ADDR) / PAGE_SIZE;
		return 0;
	}
#if 0
	else{
    	*first_page = (unsigned int)(dsp_addr - HW_RAM_ADDR) / PAGE_SIZE + POOL1_DSP_NUM;
    	*last_page = (unsigned int)(dsp_addr + dsp_ram_size - 1 - HW_RAM_ADDR) / PAGE_SIZE + POOL1_DSP_NUM;
	 	return 0;
	}
#endif
	return 0;
}

void * dspram_alloc_range(void *dsp_ram_start, unsigned int dsp_ram_size)
{
#if 0
	unsigned int flags;
	int i, first_page, last_page;
	struct page *page;

    if(dspram_convert_to_dsp_index(dsp_ram_start, dsp_ram_size, &first_page, &last_page) < 0)
    {
        printk("covert error\n");
        goto err;
    }

	flags = local_irq_save();
	for(i = first_page; i <= last_page; i++)
	{
		page = &pagepool1[i];

		if(!pagepool_is_page_in_freelist(page))
		{
			local_irq_restore(flags);
			goto err;
		}
	}

	for(i = first_page; i <= last_page; i++)
	{
		page = &pagepool1[i];
		if(pagepool_is_page_in_freelist(page))
		{
			pagepool_freelist_del((zone_ram_dsp << 7) | i);
			freepage_num[zone_ram_dsp]--;
		}
	}
	local_irq_restore(flags);

#ifndef NDEBUG
    printk("<I> %s: 0x%x, 0x%x, call_addr: 0x%x\n",
        __FUNCTION__,
        dsp_ram_start,
        dsp_ram_size,
        __builtin_return_address(0));
#endif

	return (void*)dsp_ram_start;

err:
    printf("<E> failed %s: 0x%x, 0x%x, first %x last %x, call_addr: 0x%x\n",
        __FUNCTION__,
        dsp_ram_start,
        dsp_ram_size,
        first_page,
        last_page,
        __builtin_return_address(0));

    halt();    

    return NULL;
#else
    return (void*)dsp_ram_start;
#endif
}

int dspram_free_range(void *dsp_ram_start, unsigned int dsp_ram_size)
{
#if 0
	unsigned int flags;
	int i, first_page, last_page;
	unsigned char index;
	struct page *page;

    if(dspram_convert_to_dsp_index(dsp_ram_start, dsp_ram_size, &first_page, &last_page) < 0)
    {
        goto err;
    }

	for(i = first_page; i <= last_page; i++)
	{
		page = &pagepool1[i];

		if(pagepool_is_page_in_freelist(page))
			goto err;
	}

	for(i = first_page; i <= last_page; i++)
	{
		index = (zone_ram_dsp << 7) | i;

		page = &pagepool1[i];

		flags = local_irq_save();
		pagepool_freelist_add(index);
		freepage_num[zone_ram_dsp]++;
		page->ibank_no = -1;
		local_irq_restore(flags);
	}

#ifndef NDEBUG
    printk("<I> %s: 0x%x, 0x%x, call_addr: 0x%x\n",
        __FUNCTION__,
        dsp_ram_start,
        dsp_ram_size,
        __builtin_return_address(0));
#endif

	return 0;

err:
    printk("<E> failed %s: 0x%x, 0x%x, call_addr: 0x%x\n",
        __FUNCTION__,
        dsp_ram_start,
        dsp_ram_size,
        __builtin_return_address(0));
    halt();    

    return -EINVAL;
#else
    return 0;
#endif
}


