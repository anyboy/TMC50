#include <kernel.h>
#include "heap.h"
#include <mpu_acts.h>

rom_buddy_data_t __coredata g_rom_buddy_data;

static void buddy_halt(void)
{
	panic("memory buddy_error");
}
void buddy_rom_data_init(void)
{
#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
    mpu_disable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX); 
#endif

    g_rom_buddy_data.printf = printk;
    g_rom_buddy_data.halt = buddy_halt;
    g_rom_buddy_data.pagepool_convert_index_to_addr = pagepool_convert_index_to_addr;

#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
    mpu_enable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX); 
#endif	
}
