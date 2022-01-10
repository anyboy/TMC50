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

static void se_init(const u8_t *key, u32_t key_bits)
{
	u32_t temp_value;
	int i, j, key_words;

	key_words = key_bits/32;

	/* SE CLK = HOSC/1 */
	sys_write32((0 << 8) | (0 << 0), CMU_SECCLK);

	acts_clock_peripheral_enable(CLOCK_ID_SEC);

	acts_reset_peripheral(RESET_ID_SEC);

	sys_write32((0 << 0), SE_FIFOCTRL);
	sys_write32((1 << 3), AES_CTRL);

	//set aes key
	for (i = 0; i < key_words; i++) {
		temp_value = 0;
		for (j = 0; j < 4; j++)
			temp_value |= (((u32_t)key[i*4 + j]) << (j*8));
		sys_write32(temp_value, AES_KEY0 + i*0x04);
	}

	sys_write32(0x03020100, AES_IV0);
	sys_write32(0x07060504, AES_IV1);
	sys_write32(0x0b0a0908, AES_IV2);
	sys_write32(0x0f0e0d0c, AES_IV3);
}

static void se_deinit(void)
{
	sys_write32(sys_read32(AES_CTRL) & ~(1 << 3), AES_CTRL);

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
		printk("dma_request in chan fail\n");
		return -1;
	}

	memset(&config_data, 0, sizeof(config_data));
	memset(&head_block, 0, sizeof(head_block));
	config_data.channel_direction = MEMORY_TO_PERIPHERAL;
	config_data.dma_slot = DMA_ID_AES_FIFO;
	config_data.source_data_size = 4;

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

static int se_config_dma_out(void)
{
	struct dma_config config_data;
	struct dma_block_config head_block;

	g_acts_se_data.dma_dev = device_get_binding(CONFIG_DMA_0_NAME);

	if (!g_acts_se_data.dma_dev) {
		printk("Bind DMA device %s error\n", CONFIG_DMA_0_NAME);
		return -1;
	}

	g_acts_se_data.dma_chan_out = dma_request(g_acts_se_data.dma_dev, 0xff);
	if (!g_acts_se_data.dma_chan_out) {
		printk("dma_request out chan fail\n");
		return -1;
	}

	memset(&config_data, 0, sizeof(config_data));
	memset(&head_block, 0, sizeof(head_block));
	config_data.channel_direction = PERIPHERAL_TO_MEMORY;
	config_data.dma_slot = DMA_ID_AES_FIFO;
	config_data.dest_data_size = 4;

	config_data.dma_callback = NULL;
	config_data.callback_data = NULL;
	config_data.complete_callback_en = 0;

	head_block.source_address = (unsigned int)NULL;
	head_block.dest_address = (unsigned int)NULL;
	head_block.block_size = 0;
	head_block.source_reload_en = 0;

	config_data.head_block = &head_block;

	dma_config(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan_out, &config_data);

	return 0;
}

static int se_free_dma(void)
{
	dma_stop(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);
	dma_free(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);
	g_acts_se_data.dma_chan = 0;

	dma_stop(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan_out);
	dma_free(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan_out);
	g_acts_se_data.dma_chan_out = 0;

	return 0;
}

/**
 * \brief  IC inner aes module init
 */
static const u8_t aes_key_128bits[16] = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c};

int se_aes_init(void)
{
	int ret = 0;
	se_init(aes_key_128bits, 128);
	ret = se_config_dma();
	if (ret < 0)
		return ret;
	ret = se_config_dma_out();
	return ret;
}

/**
 * \brief  IC inner aes module deinit
 */
int se_aes_deinit(void)
{
	se_free_dma();
	se_deinit();
	return 0;
}

/**
 * \brief               aes encrypt/decrypt use IC inner aes module
 *
 * \param mode          1 encrypt, 0 decrypt
 * \param in_buf        input buffer address, must 16bytes align, caller must pad align self
 * \param in_len        input buffer length
 * \param out_buf       output buffer address
 *
 * \return              output length
 */
u32_t se_aes_process(int mode, u8_t *in_buf, u32_t in_len, u8_t *out_buf)
{
	sys_write32((1<<16)|(0<<17)|(2<<2)|(mode<<0), AES_MODE); //aes CBC mode,key size 128bit & from registers
	sys_write32(in_len, AES_LEN);

	dma_reload(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan, (u32_t)in_buf, AES_INFIFO, in_len);
	dma_reload(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan_out, AES_OUTFIFO, (u32_t)out_buf, in_len);

	sys_write32(sys_read32(AES_INOUTFIFOCTL)|(1<<5)|(1<<1), AES_INOUTFIFOCTL); //enable DRQ

	dma_start(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);
	dma_start(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan_out);

	sys_write32(sys_read32(AES_CTRL) | (1<<0), AES_CTRL); 	//enable AES

	dma_wait_finished(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan);

	while((sys_read32(AES_CTRL) & (1<<31)) == 0);		   	//wait aes finished
	sys_write32(sys_read32(AES_CTRL) | (1<<31), AES_CTRL); 	//clear aes finished pending

	dma_wait_finished(g_acts_se_data.dma_dev, g_acts_se_data.dma_chan_out);

	sys_write32(sys_read32(AES_CTRL) | (1<<1), AES_CTRL);  	//AES reset
	while((sys_read32(AES_CTRL) & (1<<1)));					//wait AES reset finished

	return in_len;
}

/* sample code, aes cbc

	key 128 : 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	iv      : 0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c

	encrypt : 0x22, 0xe7, 0x27, 0x44, 0xd4, 0x83, 0x9b, 0x8c, 0x48, 0x03, 0xfe, 0x8c, 0x30, 0x75, 0x3a, 0x88,
	          0x42, 0x4e, 0xaa, 0x6b, 0x6f, 0x84, 0x16, 0xa0, 0x0b, 0x5a, 0x4b, 0x38, 0x13, 0xd3, 0x9b, 0xec

	u8_t temp_buffer_input[32];
	u8_t temp_buffer_encrypt_output[32];
	u8_t temp_buffer_decrypt_output[32];

	if (se_aes_init() >= 0) {
		memcpy(temp_buffer_input, "aes cbc test str", 16);
		memcpy(temp_buffer_input + 16, "aes cbc test str", 16);
		se_aes_process(1, temp_buffer_input, 32, temp_buffer_encrypt_output);
		se_aes_process(0, temp_buffer_encrypt_output, 32, temp_buffer_decrypt_output);
		se_aes_deinit();
	}

*/

