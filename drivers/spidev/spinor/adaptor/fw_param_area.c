#include <sdk.h>
#include <spi/spinor.h>
#include <word_checksum.h>

extern bool __spinor_buf_is_not_all_ff(unsigned long long *buf, int uint64_len);

static unsigned char basic_param_block_programed_sectors, basic_param_block_valid_sector;

int fw_param_area_write(void *buf, unsigned int offset, unsigned int size)
{
	void *tmp_buf;

	printk("%s offset %d, size %d\n", __FUNCTION__, offset, size);
	if(basic_param_block_programed_sectors >= SPINOR_BLOCK_SECTORS)
		return -EPERM;
	if(offset + size > 512 - 4)
		return -EPERM;

	tmp_buf = mem_malloc(512);
	if(tmp_buf == NULL)
		return -ENOMEM;

	sys_critical_enter(SYS_CRITICAL_TYPE_THREAD);
	if(basic_param_block_valid_sector >= BLOCK0_FISRT_WRITABLE_SECTOR)
		spinor_read(basic_param_block_valid_sector, tmp_buf, 1, 1);
	else
		memset(tmp_buf, 0xff, 512);
	memcpy(tmp_buf + 4 + offset, buf, size);
	*((unsigned int*)tmp_buf) = word_checksum(tmp_buf + 4, (512 - 4) / 4) + 0x1234;

    spinor_basic_protect_disable();
	spinor_write(basic_param_block_programed_sectors, tmp_buf, 1, 1);
    spinor_basic_protect_enable();

	basic_param_block_valid_sector = basic_param_block_programed_sectors;
	basic_param_block_programed_sectors++;
	sys_critical_exit(SYS_CRITICAL_TYPE_THREAD, 0);

	mem_free(tmp_buf);
	printk("programed %d valid %d\n", basic_param_block_programed_sectors, basic_param_block_valid_sector);
	return 0;
}

int fw_param_area_read(void *buf, unsigned int offset, unsigned int size)
{
	char *tmp_buf;

	if(offset + size > 512 - 4)
		return -EPERM;
	if(basic_param_block_valid_sector < BLOCK0_FISRT_WRITABLE_SECTOR)
		return -ENOENT;

	tmp_buf = mem_malloc(512);
	if(tmp_buf == NULL)
		return -ENOMEM;

	spinor_read(basic_param_block_valid_sector, tmp_buf, 1, 1);
	memcpy(buf, tmp_buf + 4 + offset, size);

	mem_free(tmp_buf);
	return 0;
}

/*Use dichotomy to confirm the number of sectors programmed */
static int fw_param_area_get_programed_sector_num(void *buf)
{
	unsigned char sector = BLOCK0_FISRT_WRITABLE_SECTOR, low_sector = BLOCK0_FISRT_WRITABLE_SECTOR;
	unsigned char high_sector = SPINOR_BLOCK_SECTORS - 1, is_all_ff;

	while (low_sector <= high_sector )
	{
		sector = (low_sector + high_sector) / 2;

		spinor_read(sector, buf, 1, 0);
		is_all_ff = !__spinor_buf_is_not_all_ff(buf, 512 / 8);
		if(is_all_ff)
			high_sector = sector - 1;
		else
			low_sector = sector + 1;
	}

	if(is_all_ff)
		return sector;

	return (sector + 1);
}

int fw_param_area_init(void)
{
	void *buf = mem_malloc(512);
	if(buf == NULL)
		return -ENOMEM;

	sys_critical_enter(SYS_CRITICAL_TYPE_THREAD);
	basic_param_block_programed_sectors = fw_param_area_get_programed_sector_num(buf);
	basic_param_block_valid_sector = basic_param_block_programed_sectors - 1;
	for(; basic_param_block_valid_sector >= BLOCK0_FISRT_WRITABLE_SECTOR; basic_param_block_valid_sector--)
	{
		spinor_read(basic_param_block_valid_sector, buf, 1, 1);
		if(*((unsigned int *)buf) == calculate_word_checksum(buf + 4, (512 - 4) / 4) + 0x1234)
			break;
	}
	sys_critical_exit(SYS_CRITICAL_TYPE_THREAD, 0);

	mem_free(buf);
	printk("init programed%d valid %d\n", basic_param_block_programed_sectors, basic_param_block_valid_sector);
	return 0;
}

