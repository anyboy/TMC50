#include "heap.h"
#if defined(CONFIG_KALLSYMS)
#include <kallsyms.h>
#endif

#define MAP_BUDDY_DUMP 0


static void (*dump_mem_callback)(int32_t prio, uint32_t size, uint32_t in_isr);


static void print_malloc_info(void *addr, int size, int alloc_size,struct buddy_debug_info *debug_info)
{
    if(debug_info->prio != INVALID_THREAD_PRIO){
#ifdef CONFIG_KALLSYMS
        printk("%p  %4d(%4d %2d)\t%8p", addr, alloc_size, size, debug_info->prio, PTR_INFLATE(debug_info->caller));
        print_symbol(" %s\n", (unsigned long)PTR_INFLATE(debug_info->caller));
#else
        printk("%p  %4d(%4d %2d)\t%8p\n", addr, alloc_size, size, debug_info->prio, PTR_INFLATE(debug_info->caller));
#endif
    }else{
#ifdef CONFIG_KALLSYMS
        printk("%p  %4d(%4d %2d(*))\t%8p", addr, alloc_size, size, debug_info->prio, PTR_INFLATE(debug_info->caller));
        print_symbol(" %s\n", (unsigned long)PTR_INFLATE(debug_info->caller));
#else
        printk("%p  %4d(%4d %2d(*))\t%8p\n", addr, alloc_size, size, debug_info->prio, PTR_INFLATE(debug_info->caller));
#endif        
    }

    k_busy_wait(500);

}

void dump_show_info(void *addr, int size, unsigned long *p_info, unsigned int print_detail)
{
	unsigned short alloc_size;
    struct buddy_debug_info buddy_debug;

    memcpy(&buddy_debug, p_info, sizeof(buddy_debug));

	alloc_size = buddy_debug.size;
	if(buddy_debug.size_mask != (unsigned short)(~(buddy_debug.size + (unsigned short)0x1234)))
		alloc_size = size;

    if(print_detail){
        print_malloc_info(addr, size, alloc_size, &buddy_debug);
    }

	if(dump_mem_callback){
	    if(buddy_debug.prio != INVALID_THREAD_PRIO){
            dump_mem_callback(buddy_debug.prio, size, false);
	    }else{
            dump_mem_callback(0, size, true);
	    }       
	}
}

void dump_mem_register_callback(void (*cb)(int32_t prio, uint32_t alloc_size, uint32_t in_isr))
{
    dump_mem_callback = cb;    
}

static int get_nsize_and_mask_canvas(struct buddy* self, int index, int node_size
#if MAP_BUDDY_DUMP == 1
						, char *canvas
#endif
	)
{
	int nsize;

#if MAP_BUDDY_DUMP == 1
	int i, offset;

	offset = (index + 1) * node_size - MAX_INDEX;
	for (i = offset; i < offset + node_size; i++)
		canvas[i] = '*';
#endif

	nsize = node_size;
	for(index = RIGHT_SIBLING_LEFT_LEAF(index), node_size /= 2; node_size; node_size /= 2)
	{

		if(getMax(self, index) == 0)
		{

			if(rom_buddy_getType(self, index))
				break;

			else if(index > MAX_INDEX)
			{
				nsize += node_size;
#if MAP_BUDDY_DUMP == 1
				offset = index - MAX_INDEX + 1;
				canvas[offset] = '*';
#endif
			}

			else if(getMax(self, LEFT_LEAF(index)) && getMax(self, RIGHT_LEAF(index)))
			{
				nsize += node_size;
#if MAP_BUDDY_DUMP == 1
				offset = (index + 1) * node_size - MAX_INDEX;
				for (i = offset; i < offset + node_size; i++)
					canvas[i] = '*';
#endif


				index = RIGHT_SIBLING_LEFT_LEAF(index);
			}
			else
			{
				index = LEFT_LEAF(index);
			}
		}

		else if(getMax(self, index) == node_size)
			break;

		else
		{
			index = LEFT_LEAF(index);
		}
	}

	return nsize;
}

bool buddy_dump(uint8_t buddy_no, uint32_t print_detail)
{
	void *page;
	struct buddy* self;
#if MAP_BUDDY_DUMP == 1
	char canvas[MAX_INDEX + 1];
#endif
	int32_t index;
	uint32_t node_size, offset, nsize;

	page = pagepool_convert_index_to_addr(buddy_no);
	self = (struct buddy*)(page + PAGE_SIZE - SELF_SIZE);
	if(self->page_no != buddy_no)
	{
		printk("wrong buddy_no %d\n", buddy_no);
		return false;
	}

#if MAP_BUDDY_DUMP == 1
	memset(canvas, '-', sizeof(canvas));
#endif
	node_size = MAX_INDEX * 2;

	for(index = 0; index < 2 * MAX_INDEX - 1; index++)
	{
		if(IS_POWER_OF_2(index + 1))
		{
			node_size /= 2;
		}

		if(getMax(self, index) == 0 && rom_buddy_getType(self, index))
		{
			if(index > MAX_INDEX)
			{
				offset = index - MAX_INDEX + 1;
#if MAP_BUDDY_DUMP == 1
				canvas[offset] = '*';
#else
				dump_show_info(page + (offset * UNIT_SIZE), UNIT_SIZE
						, ((unsigned long *)(page + (offset * UNIT_SIZE) + UNIT_SIZE - 8)), \
						print_detail);
#endif
			}
			else if(getMax(self, LEFT_LEAF(index)) && getMax(self, RIGHT_LEAF(index)))
			{

				offset = (index + 1) * node_size - MAX_INDEX;

				nsize = get_nsize_and_mask_canvas(self, index, node_size
#if MAP_BUDDY_DUMP == 1
								, canvas
#endif
					);


				if(offset == SELF_INDEX)
					continue;

#if MAP_BUDDY_DUMP == 0
                //printk("offset %d\n", offset);
				dump_show_info(page + (offset * UNIT_SIZE), nsize * UNIT_SIZE \
						, ((unsigned long *)(page + (offset * UNIT_SIZE) + (nsize * UNIT_SIZE) - 8)), \
						print_detail);
#endif
			}
		}
	}
#if MAP_BUDDY_DUMP == 1
	canvas[MAX_INDEX] = '\0';
	printk("buddy_no:%d\n", buddy_no);
	printk(canvas);
	printk("\n");
#endif
	return true;
}

