/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2019-1-2-7:01:06             1.0             build this file
 ********************************************************************************/
/*!
 * \file     rom_export_api.c
 * \brief
 * \author
 * \version  1.0
 * \date  2019-1-2-7:01:06
 *******************************************************************************/
#include <soc.h>
#include <export_function.h>

static struct export_api_ops * const rom_api = (struct export_api_ops *)CONFIG_ROM_TABLE_ENTRY_ADDR;

void* rom_memset(void * s, int c, unsigned int count)
{
    return rom_api->p_memset(s, c, count);
}

void* rom_memcpy(void *dest, const void *src, unsigned int count)
{
    return rom_api->p_memcpy(dest, src, count);
}

int rom_vsnprintf(char* buf, size_t size, unsigned int type, const char* fmt, va_list args)
{
    return  rom_api->p_vsnprintf(buf, size, type, fmt, args);
}

void rom_uart_init(uint32_t io_reg)
{
    rom_api->p_uart_init(io_reg);
}

void rom_uart_puts(char *s, unsigned int len)
{
    rom_api->p_uart_puts(s, len);
}

int rom_printf(const char *fmt, ...)
{
	va_list args;
	char buf[100];
	int trans_len;

	va_start(args, fmt);

	trans_len = rom_vsnprintf(buf, 100, 0, fmt, args);

	va_end( args );

	rom_uart_puts(buf, trans_len);

	return trans_len;
}

unsigned int rom_card_read(unsigned int addr,unsigned int sector_len,unsigned char* buf)
{
    return  rom_api->p_brom_card_read(addr, sector_len, buf);
}

void rom_adfu_launcher(void)
{
    rom_api->p_adfu_launcher();
}

void rom_uart_launcher(void)
{
    rom_api->p_uart_launcher();
}

void rom_nor_launcher(void)
{
    rom_api->p_nor_launcher();
}

void rom_card_launcher(void)
{
    rom_api->p_card_launcher();
}

void rom_launch(unsigned int type, unsigned int addr)
{
    rom_api->p_launch(type, addr);
}

void rom_sysinit(void)
{
    rom_api->p_brom_sysinit();
}

void rom_poweron(void)
{
    rom_api->p_brom_poweron();
}

int rom_verify_signature(const unsigned char *key, const unsigned char *sig, const unsigned char *data, unsigned int len)
{
    return rom_api->p_verify_signature(key, sig, data, len);
}

unsigned int rom_image_data_check(unsigned char* data)
{
    return rom_api->p_image_data_check(data);
}

unsigned int rom_image_security_data_check(unsigned char* data)
{
    return rom_api->p_image_security_data_check(data);
}

int rom_sha256_init(void* ctx)
{
    return rom_api->p_sha256_init(ctx);
}

int rom_sha256_update(void* ctx, const void* data, int len)
{
    return rom_api->p_sha256_update(ctx, data, len);
}

const unsigned char* rom_sha256_final(void *ctx)
{
    return rom_api->p_sha256_final(ctx);
}

const unsigned char* rom_sha256_hash(const void* data, int len, uint8_t* digest)
{
    return rom_api->p_sha256_hash(data, len, digest);
}

void rom_timestamp_init(unsigned int timer_num)
{
    return rom_api->p_timestamp_init(timer_num);
}

unsigned int rom_get_timestamp_us(void)
{
    return rom_api->p_timestamp_get_us();
}

void rom_enable_irq(uint32_t irq_num)
{
    rom_api->p_enable_irq(irq_num);
}

void rom_disable_irq(uint32_t irq_num)
{
    rom_api->p_disable_irq(irq_num);
}

int rom_irq_set_prio(unsigned int irq,unsigned int prio)
{
    rom_api->p_irq_set_prio(irq, prio);
    return 0;
}

void rom_card_mfp_save(void *data, unsigned int select, unsigned int data_width)
{
    rom_api->p_card_mfp_save(data, select, data_width);
}

void rom_card_mfp_restore(void *data, unsigned int select, unsigned int data_width)
{
    rom_api->p_card_mfp_recovery(data, select, data_width);
}






