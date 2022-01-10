/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      cailizhen   2020/7/9 19:22:28             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_se_crc.c
 * \brief
 * \author
 * \version  1.0
 * \date  2020/7/9 19:22:41
 *******************************************************************************/
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <misc/printk.h>
#include <string.h>

static struct acts_se_data g_acts_se_data;

static void se_init(void)
{
	/* SE CLK = HOSC/1 */
	sys_write32((0 << 8) | (0 << 0), CMU_SECCLK);

	acts_clock_peripheral_enable(CLOCK_ID_SEC);

	acts_reset_peripheral(RESET_ID_SEC);

	sys_write32((1 << 0), SE_FIFOCTRL);
	sys_write32((1 << 3), CRC_CTRL);
}

static void se_deinit(void)
{
	sys_write32(sys_read32(CRC_CTRL) & ~(1 << 3), CRC_CTRL);

	acts_clock_peripheral_disable(CLOCK_ID_SEC);
}

static int se_config_dma(void)
{
	struct dma_config config_data;
	struct dma_block_config head_block;

	g_acts_se_data.dma_dev = device_get_binding(CONFIG_DMA_0_NAME);

	if (!g_acts_se_data.dma_dev) {
		printk("Bind DMA device %s error\n", CONFIG_DMA_0_NAME);
		return -1;
	}

	g_acts_se_data.dma_chan = dma_request(g_acts_se_data.dma_dev, 0xff);
	if (!g_acts_se_data.dma_chan) {
		printk("dma_request fail\n");
		return -1;
	}

	memset(&config_data, 0, sizeof(config_data));
	memset(&head_block, 0, sizeof(head_block));
	config_data.channel_direction = MEMORY_TO_PERIPHERAL;
	config_data.dma_slot = DMA_ID_AES_FIFO;
	config_data.source_data_size = 1;

	config_data.dma_callback = NULL;
	config_data.callback_data = NULL;
	config_data.complete_callback_en = 0;

	head_block.source_address = (unsigned int)NULL;
	head_block.dest_address = (unsigned int)NULL;
	head_block.block_size = 0;
	head_block.source_reload_en = 0;

	config_data.head_block = &head_block;

	dma_config(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan, &config_data);

	return 0;
}

static int se_free_dma(void)
{
	dma_stop(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);
	dma_free(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);
	g_acts_se_data.dma_chan = 0;

	return 0;
}

/**
 * \brief  IC inner crc module init
 */
int se_crc_init(void)
{
	int ret = 0;
	se_init();
	ret = se_config_dma();
	return ret;
}

/**
 * \brief  IC inner crc module deinit
 */
int se_crc_deinit(void)
{
	int ret = 0;
	ret = se_free_dma();
	se_deinit();
	return ret;
}

enum crc_mode_e
{
	CRC_MODE_CRC16,
	CRC_MODE_CRC32,
};

/**
 * \brief               calculate CRC use IC inner crc module
 *
 * \param buffer        buffer address
 * \param buf_len       buffer length
 * \param crc_initial   initial and continues crc begin value
 * \param last          last time calculate crc, only IC inner crc module care if last time calculate crc
 * \param crc_mode      see enum crc_mode_e
 *
 * \return              CRC result value
 */
static u32_t se_crc_process(u8_t *buffer, u32_t buf_len, u32_t crc_initial, bool last, enum crc_mode_e crc_mode)
{
	u32_t crc_out;

	sys_write32(crc_initial, CRC_DATAINIT);

	if (crc_mode == CRC_MODE_CRC16) {
#if (CONFIG_CRC16_MODE == CRC16_MODE_CCITT)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (0 << 0), CRC_MODE);
		} else {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 13) | (0 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC16_MODE == CRC16_MODE_CCITT_FALSE)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((0 << 0), CRC_MODE);
		} else {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((0 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC16_MODE == CRC16_MODE_XMODEM)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((0 << 0), CRC_MODE);
		} else {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((0 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC16_MODE == CRC16_MODE_X25)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (0 << 0), CRC_MODE);
		} else {
			sys_write32(0xFFFF, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 13) | (0 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MODBUS)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 4) | (0 << 0), CRC_MODE);
		} else {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 13) | (1 << 4) | (0 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC16_MODE == CRC16_MODE_IBM)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 4) | (0 << 0), CRC_MODE);
		} else {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 13) | (1 << 4) | (0 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MAXIM)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 4) | (0 << 0), CRC_MODE);
		} else {
			sys_write32(0xFFFF, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 13) | (1 << 4) | (0 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC16_MODE == CRC16_MODE_USB)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 4) | (0 << 0), CRC_MODE);
		} else {
			sys_write32(0xFFFF, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 13) | (1 << 4) | (0 << 0), CRC_MODE);
		}
#else
		#error "NOT SUPPORT CRC16 MODE!"
#endif
	} else {
#if (CONFIG_CRC32_MODE == CRC32_MODE_CRC32)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 0), CRC_MODE);
		} else {
			sys_write32(0xFFFFFFFF, CRC_DATAOUTXOR);
			sys_write32((1 << 12) | (1 << 13) | (1 << 0), CRC_MODE);
		}
#elif (CONFIG_CRC32_MODE == CRC32_MODE_MPEG2)
		if (last == false) {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 0), CRC_MODE);
		} else {
			sys_write32(0x0, CRC_DATAOUTXOR);
			sys_write32((1 << 0), CRC_MODE);
		}
#else
		#error "NOT SUPPORT CRC32 MODE!"
#endif
	}

	sys_write32(buf_len, CRC_LEN);

	dma_reload(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan, (unsigned int)buffer, CRC_INFIFO, buf_len);

	sys_write32(sys_read32(CRC_INFIFOCTL) | (1 << 1), CRC_INFIFOCTL);

	dma_start(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);

	sys_write32(sys_read32(CRC_CTRL) | (1 << 0), CRC_CTRL);

	dma_wait_finished(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);

	while ((sys_read32(CRC_CTRL) & (1 << 31)) == 0);         //wait crc finished
	sys_write32(sys_read32(CRC_CTRL) | (1 << 31), CRC_CTRL); //clear crc finished pending

	crc_out = sys_read32(CRC_DATAOUT);

	if (last == true) {
		sys_write32(sys_read32(CRC_CTRL) | (1<<1), CRC_CTRL);  	//CRC reset
		while((sys_read32(CRC_CTRL) & (1<<1)));					//wait CRC reset finished
	}

	return crc_out;
}

/**
 * \brief               calculate CRC16 (CRC16_CCITT) use IC inner crc module
 *
 * \param buffer        buffer address
 * \param buf_len       buffer length
 * \param crc_initial   initial and continues crc16 begin value
 * \param last          last time calculate crc16, only IC inner crc module care if last time calculate crc16
 *
 * \return              CRC16 result value
 */
u16_t se_crc16_process(u8_t *buffer, u32_t buf_len, u16_t crc_initial, bool last)
{
	return se_crc_process(buffer, buf_len, crc_initial, last, CRC_MODE_CRC16);
}

/**
 * \brief               calculate CRC32 use IC inner crc module
 *
 * \param buffer        buffer address
 * \param buf_len       buffer length
 * \param crc_initial   initial and continues crc32 begin value
 * \param last          last time calculate crc32, only IC inner crc module care if last time calculate crc32
 *
 * \return              CRC32 result value
 */
u32_t se_crc32_process(u8_t *buffer, u32_t buf_len, u32_t crc_initial, bool last)
{
	return se_crc_process(buffer, buf_len, crc_initial, last, CRC_MODE_CRC32);
}


/**
 * \brief               calculate CRC16 (CRC16_CCITT) use IC inner crc module
 *
 * \param buffer        buffer address
 * \param buf_len       buffer length
 *
 * \return              CRC16 result value
 */
u16_t se_crc16(u8_t *buffer, u32_t buf_len)
{
	u16_t crc16_initial = 0;

#if (CONFIG_CRC16_MODE == CRC16_MODE_CCITT)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_CCITT_FALSE)
	crc16_initial = 0xFFFF;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_XMODEM)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_X25)
	crc16_initial = 0xFFFF;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MODBUS)
	crc16_initial = 0xFFFF;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MAXIM)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MAXIM)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_USB)
	crc16_initial = 0xFFFF;
#else
	#error "NOT SUPPORT CRC16 MODE!"
#endif

	return se_crc16_process(buffer, buf_len, crc16_initial, true);
}


/**
 * \brief               calculate CRC32 use IC inner crc module
 *
 * \param buffer        buffer address
 * \param buf_len       buffer length
 *
 * \return              CRC32 result value
 */
u32_t se_crc32(u8_t *buffer, u32_t buf_len)
{
	u32_t crc32_initial = 0xffffffff;

#if (CONFIG_CRC32_MODE == CRC32_MODE_CRC32)
	crc32_initial = 0xffffffff;
#elif (CONFIG_CRC32_MODE == CRC32_MODE_MPEG2)
	crc32_initial = 0xffffffff;
#else
	#error "NOT SUPPORT CRC32 MODE!"
#endif

	return se_crc32_process(buffer, buf_len, crc32_initial, true);
}


/* sample code, crc32 is 0xA57BD630; crc32-MPEG2 is 0x3590e448

	u8_t temp_buffer[20];
	u32_t crc_32_ret;
	u32_t crc32_initial = 0xffffffff;

#if (CONFIG_CRC32_MODE == CRC32_MODE_CRC32)
	crc32_initial = 0xffffffff;
#elif (CONFIG_CRC32_MODE == CRC32_MODE_MPEG2)
	crc32_initial = 0xffffffff;
#endif

	if (se_crc_init() >= 0) {
		memcpy(temp_buffer, "this is a ", 10);
		crc_32_ret = se_crc32_process(temp_buffer, 10, crc32_initial, 0);
		memcpy(temp_buffer, "crc32 test", 10);
		crc_32_ret = se_crc32_process(temp_buffer, 10, crc_32_ret, 1);

		memcpy(temp_buffer, "this is a crc32 test", 20);
		crc_32_ret = se_crc32(temp_buffer, 20);

		se_crc_deinit();
	}

*/

/* sample code, crc16-CCITT is 0x0CCA; crc16-CCITT-FALSE is 0xC5AA;
				crc16-XMODEM is 0x3312; crc16-X25 is 0xEE5A;
				crc16-MODBUS is 0x84D2; crc16-IBM is 0x9ff6;
				crc16-MAXIM is 0x6009; crc16-USB is 0x7b2d; 

	u8_t temp_buffer[20];
	u16_t crc_16_ret;
	u16_t crc16_initial = 0;

#if (CONFIG_CRC16_MODE == CRC16_MODE_CCITT)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_CCITT_FALSE)
	crc16_initial = 0xFFFF;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_XMODEM)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_X25)
	crc16_initial = 0xFFFF;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MODBUS)
	crc16_initial = 0xFFFF;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MAXIM)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_MAXIM)
	crc16_initial = 0x0;
#elif (CONFIG_CRC16_MODE == CRC16_MODE_USB)
	crc16_initial = 0xFFFF;
#endif

	if (se_crc_init() >= 0) {
		memcpy(temp_buffer, "this is a ", 10);
		crc_16_ret = se_crc16_process(temp_buffer, 10, crc16_initial, 0);
		memcpy(temp_buffer, "crc16 test", 10);
		crc_16_ret = se_crc16_process(temp_buffer, 10, crc_16_ret, 1);

		memcpy(temp_buffer, "this is a crc16 test", 20);
		crc_16_ret = se_crc16(temp_buffer, 20);

		se_crc_deinit();
	}

*/


