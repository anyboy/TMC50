#include <kernel.h>
#include <init.h>
#include <device.h>
#include <irq.h>
#include <soc.h>
#include <mmc/mmc.h>
#include <mmc/sdio.h>
#include <porting.h>
#include "sdio_port.h"
#include "sdio_def.h"

#include <log.h>

//#define SDIO_PORT_DEBUG

#define SDIO_INDEX 0

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static u32 sdio_block_size;

typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL; 

static struct sdio_func * g_sdio_func = NULL;

//#define HANDLE_DATA_OVERLAP

#ifdef HANDLE_DATA_OVERLAP
static u8 data_overlap[SDIO_DEFAULT_BLOCK_SIZE];
#endif

u8 sdio_readb_cmd52(u32 addr)
{
    u8 out;
    u32 ret;
    struct sdio_func *func = g_sdio_func;

    if (func == 0)
    {
        SDIO_ERROR("sdio_readb_cmd52(read), func = NULL\r\n");
        return 0;
    }

    out = 0;
    sdio_claim_host(func);
    out = sdio_readb(func, addr, &ret);
    sdio_release_host(func);
    if (0 != ret)
    {
        SDIO_ERROR("sdio_readb(read), ret = %d\r\n", ret);
        return 0;
    }

	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_readb_cmd52", addr, out);

    return out;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{

    u8 in;
    u32 ret;
    struct sdio_func *func = g_sdio_func;

    if (func == 0)
    {
        SDIO_ERROR("sdio_writeb_cmd52(read), func = NULL\r\n");
        return false;
    }
    in  = data;
    sdio_claim_host(func);
    sdio_writeb(func, in, addr, &ret);
    sdio_release_host(func);
    if (0 != ret)
    {
        SDIO_ERROR("sdio_writeb_cmd52(write), ret = %d\r\n", ret);
        return false;
    }

	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_writeb_cmd52", addr, data);

    return true;

}

bool sdio_read_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 rx_blocks = 0, blksize = 0;
    int ret = 0;
    struct sdio_func *func = g_sdio_func;

    if (func == 0)
    {
        SDIO_ERROR("sdio_read_cmd53(read), func = NULL\r\n");
        return false;
    }

#ifndef SDIO_NO_NEED_ALIGN
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte, data = 0x%08x, size = %d\r\n", (unsigned int)dat, size);
        return false;
    }
#else
    if (size & 3)
    {
        SDIO_ERROR("len must be align 4 byte, size = %d\r\n", (int)size);
        return false;
    }
#endif

    if (size > sdio_block_size)
    {
        rx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    }
    else
    {
        blksize = size;
        rx_blocks = 1;
    }

    sdio_claim_host(func);

    /* data overlap */
#ifdef HANDLE_DATA_OVERLAP
    if (rx_blocks*blksize > size)
    {
        OS_MemCPY(data_overlap, dat + size, rx_blocks*blksize - size);
    }
#endif

    ret = sdio_memcpy_fromio(func, dat, dataPort, rx_blocks*blksize);

    /* data overlap restore */
#ifdef HANDLE_DATA_OVERLAP
    if (rx_blocks*blksize > size)
    {
        OS_MemCPY(dat + size, data_overlap, rx_blocks*blksize - size);
    }
#endif

    sdio_release_host(func);

	if (0 != ret)
	{
        SDIO_ERROR("sdio_memcpy_fromio(read), ret = %d\r\n", ret);
	    return false;
	}

	SDIO_TRACE("%-20s : 0x%x, %p, 0x%02x\n", "sdio_read_cmd53", dataPort, dat, (u32_t)size);

	return true;
}

bool sdio_write_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 tx_blocks = 0, blksize = 0;
    u32 ret = 0;
    struct sdio_func *func = g_sdio_func;

    if (func == 0)
    {
        SDIO_ERROR("sdio_read_cmd53(read), func = NULL\r\n");
        return false;
    }
#ifndef SDIO_NO_NEED_ALIGN
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte, data = 0x%08x, size = %d\r\n", (unsigned int)dat, size);
        return false;
    }
#else
    if (size & 3)
    {
        SDIO_ERROR("len must be align 4 byte, size = %d\r\n", (int)size);
        return false;
    }
#endif
    if (size > sdio_block_size)
    {
        tx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    }
    else
    {
        blksize = size;
        tx_blocks = 1;
    }
    sdio_claim_host(func);
    ret = sdio_memcpy_toio(func, dataPort, dat, tx_blocks*blksize);
    sdio_release_host(func);
    if (0 != ret)
    {
        SDIO_ERROR("sdio_memcpy_toio(write), ret = %d\r\n", ret);
        return false;
    }

	SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "sdio_write_cmd53", dataPort, dat, (u32_t)size);

	return true;
}

static void sdio_host_isr (struct sdio_func *func)
{
    if (g_sdio_isr_func)
    {
        //printk("%s %d\r\n", __func__, __LINE__);
        g_sdio_isr_func();
    }
}

bool sdio_host_enable_isr(bool enable)
{
    int ret = 0;
    struct sdio_func *func = g_sdio_func;

    if  (func == NULL)
    {
        SDIO_ERROR("sdio_get_func g_sdio_func = NULL\r\n");
        return false;
    }

    if (enable)
    {
        if (!sdio_writeb_cmd52(REG_INT_MASK, SDIO_REG_INT_MASK))
        {
            ret = 1;
        }
    }
    else
    {
        if (!sdio_writeb_cmd52(REG_INT_MASK, 0xff))
        {
            ret = 1;
        }
    }

    if (ret)
    {
        SDIO_ERROR("sdio_host_enable_isr(%d), ret = %d\r\n", enable, ret);
        return false;
    }

    return true;
}

bool sdio_host_detect_card(void)
{
    printk("sdio_host_detect_card \r\n");
    return true;
}

bool is_truly_isr()
{
    return true;
}

bool sdio_host_init(void (*sdio_isr)(void))
{
    u32 ret = 0;

    /* scan sdio bus */
    ret = sdio_bus_scan(SSV_WIFI_SDIO_BUS_ID);
    if (ret)
    {
        LOG_PRINTF("Cannot found device on sdio bus %d\n\r", SSV_WIFI_SDIO_BUS_ID);
		return ret;
    }

    /* sdio control init*/
    g_sdio_func = sdio_get_func(SDIO_INDEX);
    if (g_sdio_func == NULL)
    {
        SDIO_ERROR("sdio_get_func g_sdio_func = NULL\r\n");
        return false;
    }

    sdio_claim_host(g_sdio_func);
	ret = sdio_enable_func(g_sdio_func);
	sdio_release_host(g_sdio_func);
    if (ret)
    {
        SDIO_ERROR("sdio_enable_func(%d), ret = %d\r\n", SDIO_INDEX, ret);
        return false;
    }

    /* set block size */
    sdio_claim_host(g_sdio_func);
    sdio_block_size = SDIO_DEFAULT_BLOCK_SIZE;
    ret = sdio_set_block_size(g_sdio_func, SDIO_DEFAULT_BLOCK_SIZE);
	sdio_release_host(g_sdio_func);
    if (ret)
    {
        SDIO_ERROR("sdio_set_block_size(%d), ret = %d\r\n", SDIO_DEFAULT_BLOCK_SIZE, ret);
        return false;
    }
    
    g_sdio_isr_func = sdio_isr;
 
    #if (BUS_TEST_MODE == 0)
    /* enable sdio control interupt */
    /* install isr here */
    sdio_claim_host(g_sdio_func);
    ret =  sdio_claim_irq(g_sdio_func, sdio_host_isr);
    sdio_release_host(g_sdio_func);
    if (ret)
    {
        SDIO_ERROR("sdio_host_init, ret = %d\r\n", ret);
        return false;
    }
    #endif

    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING)) 
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

    return true;
}

#ifdef SDIO_PORT_DEBUG
#define test_read_register(reg) (*((volatile UINT32*)(reg)))
static volatile ftsdc021_reg *gpRegSD0 = (ftsdc021_reg*) SDC_FTSDC021_0_PA_BASE;

void sdio_in_read_reg(u32 addr)
{
    u8 value;
    value = sdio_readb_cmd52(addr);
	printk("0x%08x, 0x%02x\n",  addr, value);
}

void sdio_read_sdio_reg()
{
	printk("IntrEn = 0x%04x\n",  gpRegSD0->IntrEn);
	printk("IntrSigEn = 0x%04x\n",  gpRegSD0->IntrSigEn);
	printk("IntrSts = 0x%04x\n",  gpRegSD0->IntrSts);
	printk("ErrSts = 0x%04x\n",  gpRegSD0->ErrSts);

}

#endif

