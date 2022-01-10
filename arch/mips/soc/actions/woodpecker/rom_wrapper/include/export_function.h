/********************************************************************************
 *                            USDK(ZS283A_clean)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-10-20-PM3:29:02             1.0             build this file
 ********************************************************************************/
/*!
 * \file     export_function.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-10-20-PM3:29:02
 *******************************************************************************/

#ifndef EXPORT_FUNCTION_H_
#define EXPORT_FUNCTION_H_

typedef void* (*api_memset)(void * s, int c, unsigned int count);

typedef void* (*api_memcpy)(void *dest, const void *src, unsigned int count);

typedef int (*api_vsnprintf)(char* buf, unsigned int size, unsigned int linesep, const char* fmt, va_list args);

typedef void (*api_uart_init)(unsigned int io_reg);

typedef void (*api_uart_puts)(char *s, unsigned int len);

typedef unsigned int (*api_brom_card_read)(unsigned int,unsigned int,unsigned char*);

typedef void (*api_adfu_launcher)(void);

typedef void (*api_uart_launcher)(void);

typedef void (*api_nor_launcher)(void);

typedef void (*api_card_launcher)(void);

typedef void (*api_launch)(unsigned int type, unsigned int addr);

typedef void (*api_brom_sysinit)(void);

typedef void (*api_brom_poweron)(void);

typedef int (*api_verify_signature)(const unsigned char *key, const unsigned char *sig, const unsigned char *data, unsigned int len);

typedef unsigned int (*api_image_data_check)(unsigned char*);

typedef unsigned int (*api_image_security_data_check)(unsigned char* buf);

typedef int (*api_sha256_init)(void* ctx);

typedef int (*api_sha256_update)(void* ctx, const void* data, int len);

typedef const unsigned char*(*api_sha256_final)(void *ctx);

typedef const unsigned char * (*api_sha256_hash)(const void* data, int len, unsigned char* digest);

//timestamp export functions

typedef void (*api_timestamp_init)(unsigned int timer_num);

typedef unsigned int (*api_timestamp_get_us)(void);

typedef unsigned int (*api_timestamp_ticks_get)(void);

typedef void (*api_time_delay_ticks)(unsigned int ticks);

typedef void (*api_udelay)(unsigned int us);

typedef void (*api_mdelay)(unsigned int ms);

typedef void (*api_nor_mfp_select)(unsigned int index);

typedef void (*api_card_mfp_save)(void *data, unsigned int index, unsigned int data_width);

typedef void (*api_card_mfp_recovery)(void *data, unsigned int index, unsigned int data_width);

typedef void (*api_nor_reset)(unsigned int work_mode);

typedef void (*api_nor_wakeup_init)(void);

//uart protocol export functions
typedef int (*api_uart_connect)(void);

typedef int (*api_uart_wait_connect)(unsigned int timeout_ms);

typedef int (*api_uart_disconnect)(void);

typedef int (*api_uart_open)(unsigned int protocol_type);

typedef void (*api_uart_protocol_inquiry)(void);

typedef int (*api_uart_command_parse)(unsigned char uart_cmd, unsigned int para_length);

typedef void (*api_set_uart_protocol_state)(int state);

typedef int (*api_uart_protocol_rx_fsm)(unsigned char first_data);

typedef int (*api_uart_fsm_register)(void *uart_fsm_item, unsigned int direction);

typedef int (*api_uart_fsm_unregister)(void *uart_fsm_item, unsigned int direction);

typedef int (*api_uart_protocol_write_payload)(unsigned char *user_payload, unsigned int size, unsigned short protocol);

typedef int (*api_uart_protocol_read_payload)(unsigned char *user_payload, unsigned int size, unsigned short protocol);

typedef int (*api_uart_data_packet_receive)(void *recv_head, unsigned char *payload, unsigned int size, unsigned short protocol);

typedef unsigned char (*api_crc8)(unsigned char *p, unsigned int counter);

typedef unsigned short (*api_crc16)(unsigned char* ptr, unsigned int len);

typedef void (*api_enable_irq)(unsigned int irq_num);

typedef void (*api_disable_irq)(unsigned int irq_num);

typedef void (*api_irq_set_prio)(unsigned int irq, unsigned int prio);

typedef void (*api_card_reset)(void);

typedef unsigned int (*api_card_send_cmd)(unsigned int cmd, unsigned int param,unsigned int ctl);

typedef int (*api_spimem_xfer)(void *si, unsigned char cmd, unsigned int addr,
                 int addr_len, void *buf, int length,
                 unsigned char dummy_len, unsigned int flag);

typedef void (*api_spimem_continuous_read_reset)(void *si);


typedef void (*api_null)(void);

struct export_api_ops
{
    const char *                                       build_info;
    api_memset                                          p_memset;
    api_memcpy                                          p_memcpy;
    api_vsnprintf                                       p_vsnprintf;
    api_uart_init                                       p_uart_init;
    api_uart_puts                                       p_uart_puts;
    api_brom_card_read                                  p_brom_card_read;
    api_adfu_launcher                                   p_adfu_launcher;
    api_uart_launcher                                   p_uart_launcher;
    api_nor_launcher                                    p_nor_launcher;
    api_card_launcher                                   p_card_launcher;
    api_launch                                          p_launch;
    api_brom_sysinit                                    p_brom_sysinit;
    api_brom_poweron                                    p_brom_poweron;
    api_verify_signature                                p_verify_signature;
    api_image_data_check                                p_image_data_check;
    api_image_security_data_check                       p_image_security_data_check;
    api_sha256_init                                     p_sha256_init;
    api_sha256_update                                   p_sha256_update;
    api_sha256_final                                    p_sha256_final;
    api_sha256_hash                                     p_sha256_hash;
    api_timestamp_init                                  p_timestamp_init;
    api_timestamp_get_us                                p_timestamp_get_us;
    api_timestamp_ticks_get                             p_timestamp_ticks_get;
    api_time_delay_ticks                                p_time_delay_ticks;
    api_udelay                                          p_udelay;
    api_mdelay                                          p_mdelay;
    api_nor_mfp_select                                  p_nor_mfp_select;
    api_card_mfp_save                                   p_card_mfp_save;
    api_card_mfp_recovery                               p_card_mfp_recovery;
    api_nor_reset                                       p_nor_reset;
    api_nor_wakeup_init                                 p_nor_wakeup_init;
    api_uart_connect                                    p_uart_connect;
    api_uart_wait_connect                               p_uart_wait_connect;
    api_uart_disconnect                                 p_uart_disconnect;
    api_uart_open                                       p_uart_open;
    api_uart_protocol_inquiry                           p_uart_protocol_inquiry;
    api_uart_command_parse                              p_uart_command_parse;
    api_set_uart_protocol_state                         p_set_uart_protocol_state;
    api_uart_protocol_rx_fsm                            p_uart_protocol_rx_fsm;
    api_uart_fsm_register                               p_uart_fsm_register;
    api_uart_fsm_unregister                             p_uart_fsm_unregister;
    api_uart_protocol_write_payload                     p_uart_protocol_write_payload;
    api_uart_protocol_read_payload                      p_uart_protocol_read_payload;
    api_uart_data_packet_receive                        p_uart_data_packet_receive;
    api_crc8                                            p_crc8;
    api_crc16                                           p_crc16;
    api_enable_irq                                      p_enable_irq;
    api_disable_irq                                     p_disable_irq;
    api_irq_set_prio                                    p_irq_set_prio;
    api_card_reset                                      p_card_reset;
    api_card_send_cmd                                   p_card_send_cmd;
    //reserve 10 interfaces for externsion
    void  *                                             p_spinor_api;
    api_spimem_xfer                                     p_spimem_xfer;
    api_spimem_continuous_read_reset                    p_spimem_continuous_read_reset;
    api_null                                            p_api_null3;
    api_null                                            p_api_null4;
    api_null                                            p_api_null5;
    api_null                                            p_api_null6;
    api_null                                            p_api_null7;
    api_null                                            p_api_null8;
    api_null                                            p_api_null9;
};



#endif /* EXPORT_FUNCTION_H_ */
