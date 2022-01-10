#include "spinor_logical_inner.h"

extern void spinor_logical_rebuild_table(void);

static void __spinor_logical_struct_release(void)
{
	if(!spinor_logical)
		return;

	if(spinor_logical->tmp)
		mem_free(spinor_logical->tmp);
	if(spinor_logical->logical_table)
		mem_free(spinor_logical->logical_table);
	if(spinor_logical->free_table)
		mem_free(spinor_logical->free_table);
	if(spinor_logical->tmp_table)
		mem_free(spinor_logical->tmp_table);

	mem_free(spinor_logical);
	spinor_logical = NULL;
}

void spinor_logical_load_info(int *phyblk, int *total_blknum)
{
	if(firmware_flash_lba_offset == SPINOR_BLOCK_SECTORS)
	{
		*phyblk = (firmware_flash_lba_offset + firmware_flash_sectors + SPINOR_BLOCK_SECTORS - 1) / SPINOR_BLOCK_SECTORS;
        if(spinor_protect_block_num != 0 && *phyblk < spinor_protect_block_start + spinor_protect_block_num)
        {
            *phyblk = spinor_protect_block_start + spinor_protect_block_num;
        }
		*total_blknum = spinor_id.block_num - *phyblk;
	}

	else
	{
		*phyblk = 1;
		*total_blknum = (firmware_flash_lba_offset / SPINOR_BLOCK_SECTORS) - *phyblk;
	}

	if(*phyblk > 255)
	{
        *phyblk = 255;
	}

	if(*total_blknum > 255)
	{
        *total_blknum = 255;
	}
	printk("t%d,f(%d,%d),v(%d,%d),p(%d,%d)\n", spinor_id.block_num
        , firmware_flash_lba_offset / SPINOR_BLOCK_SECTORS, (firmware_flash_sectors + SPINOR_BLOCK_SECTORS - 1) / SPINOR_BLOCK_SECTORS
        , *phyblk, *total_blknum
        , spinor_protect_block_start, spinor_protect_block_num);
}

static int __spinor_logical_struct_create(void)
{
	int i, phyblk, total_blknum;

	spinor_logical_load_info(&phyblk, &total_blknum);
	if(phyblk > 255 || total_blknum > 255)
		return -EINVAL;
	if(total_blknum < 2)
    {
        printk("mem_free block num %d\n", total_blknum);
		return -ENOSPC;
    }

	spinor_logical = mem_malloc(sizeof(*spinor_logical));
	if(!spinor_logical)
		return -ENOMEM;
	memset(spinor_logical, 0, sizeof(*spinor_logical));

	spinor_logical->total_blknum = total_blknum;
	spinor_logical->tmp = mem_malloc(sizeof(*spinor_logical->tmp));
	spinor_logical->logical_table = mem_malloc(total_blknum * sizeof(struct logical_block_info));
	spinor_logical->free_table = mem_malloc(total_blknum);
	spinor_logical->tmp_table = mem_malloc(total_blknum);


	if(!spinor_logical->tmp
		|| !spinor_logical->logical_table || !spinor_logical->free_table || !spinor_logical->tmp_table)
	{
		__spinor_logical_struct_release();
		return -ENOMEM;
	}
	memset(spinor_logical->logical_table, 0, total_blknum * sizeof(struct logical_block_info));

	spinor_logical->logical_phyblk = phyblk;
	spinor_logical->logical_blknum = total_blknum;
	spinor_logical->logical_size = (total_blknum - 1) * DATA_SIZE_PER_LOGICAL_BLOCK;
	if(spinor_logical->logical_phyblk > 1)
		spinor_logical->physical_phyblk = phyblk + total_blknum;
	else
		spinor_logical->physical_phyblk = phyblk;

	for(i = 0; i < SPINOR_MAX_LOG_NUM; i++)
		spinor_logical->log_table[i].last_write_sector = -1;
	spinor_logical->log_info = &spinor_logical->log_table[0];
	return 0;
}

int spinor_logical_init(void)
{
	int result;

	sem_dynamic_create(&spinor_logical_semphore, 1);

	sem_wait(&spinor_logical_semphore);
	if(spinor_logical == NULL)
	{
		result = __spinor_logical_struct_create();
		if(result)
		{
			sem_post(&spinor_logical_semphore);
			return result;
		}
		spinor_logical_rebuild_table();
		mem_free(spinor_logical->tmp_table);
		spinor_logical->tmp_table = NULL;
	}

	spinor_logical->inited_count++;
	result = spinor_logical->logical_size;
	sem_post(&spinor_logical_semphore);

	return result;
}

void spinor_logical_release(void)
{
	sem_wait(&spinor_logical_semphore);
	if(spinor_logical != NULL)
	{
		spinor_logical->inited_count--;
		if(spinor_logical->inited_count == 0)
		{
			__spinor_logical_struct_release();
			spinor_logical = NULL;
		}
	}
	sem_post(&spinor_logical_semphore);
}

int spinor_logical_get_logical_size(void)
{
	return spinor_logical->logical_size;
}


