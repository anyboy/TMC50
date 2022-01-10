#include "spinor_logical_inner.h"

int spinor_logical_new_physical_area(unsigned int *size, spinor_partition_t *partition)
{
    unsigned char flag;
	unsigned char i, phyblk, dirty_index;
	unsigned char logical_phyblk, logical_blknum, physical_phyblk, physical_blknum;
	unsigned int flash_address;
	int freeblk_num;

	physical_blknum = (*size + SPINOR_BLOCK_SIZE - 1) / SPINOR_BLOCK_SIZE;
	sem_wait(&spinor_logical_semphore);

	__spinor_logical_merge_log_block_and_replace_datablock(1);

	freeblk_num = spinor_logical->free_in_index - spinor_logical->free_out_index;
	freeblk_num = freeblk_num > 0 ? freeblk_num : freeblk_num + spinor_logical->total_blknum;
	if(physical_blknum >= freeblk_num)
	{
		sem_post(&spinor_logical_semphore);
		return -ENOMEM;
	}

	logical_phyblk = spinor_logical->logical_phyblk;
	logical_blknum = spinor_logical->logical_blknum - physical_blknum;
	physical_phyblk = spinor_logical->physical_phyblk;
	if(logical_phyblk > 1)
	{
		physical_phyblk -= physical_blknum;
		flash_address = physical_phyblk * SPINOR_BLOCK_SIZE;
	}
	else
	{
		logical_phyblk += physical_blknum;
		flash_address = (physical_phyblk + spinor_logical->physical_blknum) * SPINOR_BLOCK_SIZE;
	}

	flag = 0;
	dirty_index = spinor_logical->free_in_index;
	for(i = 0; i < freeblk_num; i++)
	{
		if(spinor_logical->free_out_index >= spinor_logical->total_blknum)
			spinor_logical->free_out_index = 0;
		if(spinor_logical->free_out_index == spinor_logical->dirty_in_index)
			flag = 1;
		if(flag == 0)
			dirty_index = spinor_logical->free_in_index;

		phyblk = spinor_logical->free_table[spinor_logical->free_out_index++];
		if(phyblk >= logical_phyblk && phyblk < logical_phyblk + logical_blknum)
		{
			if(spinor_logical->free_in_index >= spinor_logical->total_blknum)
				spinor_logical->free_in_index = 0;
			spinor_logical->free_table[spinor_logical->free_in_index++] = phyblk;
			continue;
		}

		if(flag)
			spinor_basic_block_erase(phyblk * SPINOR_BLOCK_SIZE);
	}
	spinor_logical->dirty_in_index = dirty_index;
	if(spinor_logical->dirty_in_index >= spinor_logical->total_blknum)
		spinor_logical->dirty_in_index = 0;

	for(i = 0; i < spinor_logical->logical_blknum - 1; i++)
	{
		phyblk = spinor_logical->logical_table[i].phyblk;
		if(phyblk == BLOCK_NULL || (phyblk >= logical_phyblk && phyblk < logical_phyblk + logical_blknum))
			continue;

		__spinor_logical_new_log_block(i);
		__spinor_logical_merge_log_block_and_replace_datablock(0);
	}

	spinor_logical->logical_phyblk = logical_phyblk;
	spinor_logical->logical_blknum = logical_blknum;
	spinor_logical->physical_phyblk = physical_phyblk;
	spinor_logical->physical_blknum += physical_blknum;
	spinor_logical->logical_size = (spinor_logical->logical_blknum - 1) * DATA_SIZE_PER_LOGICAL_BLOCK;

	partition->logical_phyblk = spinor_logical->logical_phyblk;
	partition->logical_blknum = spinor_logical->logical_blknum;
	partition->physical_phyblk = spinor_logical->physical_phyblk;
	partition->physical_blknum = spinor_logical->physical_blknum;

	sem_post(&spinor_logical_semphore);

	*size = physical_blknum * SPINOR_BLOCK_SIZE;
	return flash_address;
}

static int __spinor_logical_erase_block_if_used(unsigned char phyblk)
{
	spinor_basic_fast_read_sectors(phyblk * SPINOR_BLOCK_SECTORS, (void*)spinor_logical->tmp, 1, 0);
	if(__spinor_buf_is_not_all_ff((void*)spinor_logical->tmp, 512 / 8))
	{
		spinor_basic_block_erase(phyblk * SPINOR_BLOCK_SIZE);
		return 1;
	}
	spinor_basic_fast_read_sectors((phyblk * SPINOR_BLOCK_SECTORS) + 1, (void*)spinor_logical->tmp, 1, 0);
	if(__spinor_buf_is_not_all_ff((void*)spinor_logical->tmp, 512 / 8))
	{
		spinor_basic_block_erase(phyblk * SPINOR_BLOCK_SIZE);
		return 1;
	}
	return 0;
}

int spinor_logical_check_physical_area(unsigned int flash_start_addr, unsigned int size)
{
	uint8 del_phyblk, del_blknum;

	if((flash_start_addr % SPINOR_BLOCK_SIZE) || (size % SPINOR_BLOCK_SIZE))
		return -EINVAL;

	del_phyblk = flash_start_addr / SPINOR_BLOCK_SIZE;
	del_blknum = size / SPINOR_BLOCK_SIZE;
	if(del_phyblk < spinor_logical->physical_phyblk
		|| del_phyblk + del_blknum > spinor_logical->physical_phyblk + spinor_logical->physical_blknum)
		return -EPERM;

    return 0;
}

int spinor_logical_del_physical_area(unsigned int flash_start_addr, unsigned int size, spinor_partition_t *partition)
{
	uint8 tmp, phyblk, del_phyblk, del_blknum;
	short blk_num, flag;

	printk("%s %d\n", __FUNCTION__, __LINE__);
	if((flash_start_addr % SPINOR_BLOCK_SIZE) || (size % SPINOR_BLOCK_SIZE))
		return -EINVAL;

	del_phyblk = flash_start_addr / SPINOR_BLOCK_SIZE;
	del_blknum = size / SPINOR_BLOCK_SIZE;
	if(del_phyblk < spinor_logical->physical_phyblk
		|| del_phyblk + del_blknum > spinor_logical->physical_phyblk + spinor_logical->physical_blknum)
		return -EPERM;

	sem_wait(&spinor_logical_semphore);

	for(blk_num = spinor_logical->logical_blknum - 1; blk_num >= 0; blk_num--)
	{
		phyblk = spinor_logical->logical_table[blk_num].phyblk;
		if(phyblk != BLOCK_NULL && (phyblk >= del_phyblk && phyblk < del_phyblk + del_blknum))
			goto err;
	}
	blk_num = spinor_logical->free_in_index - spinor_logical->free_out_index;
	blk_num = blk_num > 0 ? blk_num : blk_num + spinor_logical->total_blknum;
	tmp = spinor_logical->free_out_index;

	flag = 0;
	for(; blk_num > 0; blk_num--)
	{
		if(tmp >= spinor_logical->total_blknum)
			tmp = 0;
		if(tmp == spinor_logical->dirty_in_index)
			flag = 1;
		phyblk = spinor_logical->free_table[tmp++];
		if(phyblk >= del_phyblk && phyblk < del_phyblk + del_blknum)
			goto err;
		if(flag)
			spinor_basic_block_erase(phyblk * SPINOR_BLOCK_SIZE);
	}

	for(tmp = 0; tmp < SPINOR_MAX_LOG_NUM; tmp++)
	{
		phyblk = spinor_logical->log_table[tmp].phyblk;
		if(phyblk != BLOCK_NULL && (phyblk >= del_phyblk && phyblk < del_phyblk + del_blknum))
			goto err;
	}

	__spinor_logical_flush();

	printk("(%x,%x,%x,%x)\n", spinor_logical->logical_phyblk, spinor_logical->logical_blknum, spinor_logical->physical_phyblk, spinor_logical->physical_blknum);
	if(spinor_logical->logical_phyblk > 1)
	{
		spinor_logical->physical_phyblk += del_blknum;
	}
	else
	{
		spinor_logical->logical_phyblk -= del_blknum;
	}
	spinor_logical->logical_blknum += del_blknum;
	spinor_logical->physical_blknum -= del_blknum;
	spinor_logical->logical_size = (spinor_logical->logical_blknum - 1) * DATA_SIZE_PER_LOGICAL_BLOCK;

	for(; del_blknum > 0; del_blknum--, del_phyblk++)
	{
		__spinor_logical_erase_block_if_used(del_phyblk);

		if(spinor_logical->free_in_index >= spinor_logical->total_blknum)
			spinor_logical->free_in_index = 0;
		spinor_logical->free_table[spinor_logical->free_in_index++] = del_phyblk;
	}
	spinor_logical->dirty_in_index = spinor_logical->free_in_index;
	if(spinor_logical->dirty_in_index >= spinor_logical->total_blknum)
		spinor_logical->dirty_in_index = 0;

	partition->logical_phyblk = spinor_logical->logical_phyblk;
	partition->logical_blknum = spinor_logical->logical_blknum;
	partition->physical_phyblk = spinor_logical->physical_phyblk;
	partition->physical_blknum = spinor_logical->physical_blknum;

	printk("(%x,%x,%x,%x)\n", spinor_logical->logical_phyblk, spinor_logical->logical_blknum, spinor_logical->physical_phyblk, spinor_logical->physical_blknum);
	sem_post(&spinor_logical_semphore);
	return 0;

err:
	sem_post(&spinor_logical_semphore);
	return -EPERM;
}

int spinor_logical_trim_physical_area(unsigned int flash_start_addr, unsigned int size)
{
	uint8 trim_phyblk, trim_blknum;

	if((flash_start_addr % SPINOR_BLOCK_SIZE) || (size % SPINOR_BLOCK_SIZE))
		return -EINVAL;

	trim_phyblk = flash_start_addr / SPINOR_BLOCK_SIZE;
	trim_blknum = size / SPINOR_BLOCK_SIZE;
	if(trim_phyblk < spinor_logical->physical_phyblk
		|| trim_phyblk + trim_blknum > spinor_logical->physical_phyblk + spinor_logical->physical_blknum)
		return -EPERM;

	sem_wait(&spinor_logical_semphore);

	__spinor_logical_flush();

	for(trim_phyblk += trim_blknum - 1; trim_blknum > 0; trim_blknum--, trim_phyblk--)
	{
		spinor_basic_fast_read_sectors(((trim_phyblk + 1) * SPINOR_BLOCK_SECTORS) - 1, (void*)spinor_logical->tmp, 1, 0);
		if(__spinor_buf_is_not_all_ff((void*)spinor_logical->tmp, 512 / 8))
			break;

		if(__spinor_logical_erase_block_if_used(trim_phyblk))
		{
			trim_blknum--;
			break;
		}
	}
	sem_post(&spinor_logical_semphore);

	return (trim_blknum * SPINOR_BLOCK_SIZE);
}

