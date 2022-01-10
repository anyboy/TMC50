/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BROM_INTERFACE_H__
#define __BROM_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif


enum
{
    API_BROM_ADFU_LAUNCHER     = 0,
    API_BROM_CARD_LAUNCHER     = 2,
    API_BROM_LOCAL_IRQ_SAVE    = 5,
    API_BROM_LOCAL_IRQ_RESTORE = 6,
    API_BROM_MEMSET            = 7,
    API_BROM_MEMCPY            = 8,
    API_BROM_MEMCMP            = 9,
    API_BROM_US_DELAY          = 10,
    API_BROM_TIMESTAMP_GET_US  = 11,
    API_BROM_PRINTF_CONFIG     = 13,
    API_BROM_PRINTF            = 14,
    API_BROM_PRINTHEX_CONFIG   = 15,
    API_BROM_PRINTHEX          = 16,
    API_BROM_TRACE_SET_MODE    = 17,
    API_BROM_TRACE_OUTPUT      = 18,
    API_BROM_TRACE_VSNPRINTF   = 19,
    API_BROM_TRACE_DMA_TC_ISR  = 20,
    API_BROM_DEVICE_GET_BINDING_SPINOR = 21,
    API_BROM_DEVICE_GET_BINDING_SPIMEM = 22,
    API_BROM_TRACE_SET_UART    = 23,
    API_BROM_TRACE_SET_CBKS    = 24,
};


typedef struct
{
    void (*irq_lock)(void *flags);
    void (*irq_unlock)(const void *flags);
} trace_callbacks_t;


#define _SYMBOL_TABLE_PTR2_               (0xC00A0120)
#define _BROM_GLOB_DATA_ADDR_             (*(volatile u32_t *)_SYMBOL_TABLE_PTR2_)

#define BROM_EXPORT_APIS_ENTRY_START      ((*(u32_t *)_BROM_GLOB_DATA_ADDR_))

#define BROM_EXPORT_APIS_ENTRY_ADDR(x)    (BROM_EXPORT_APIS_ENTRY_START + (4 * (x)))

#define ENTRY_BROM_ADFU_LAUNCHER        (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_ADFU_LAUNCHER)))
#define ENTRY_BROM_CARD_LAUNCHER        (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_CARD_LAUNCHER)))

#define ENTRY_BROM_LOCAL_IRQ_SAVE       (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_LOCAL_IRQ_SAVE)))
#define ENTRY_BROM_LOCAL_IRQ_RESTORE    (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_LOCAL_IRQ_RESTORE)))
#define ENTRY_BROM_MEMSET          (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_MEMSET)))
#define ENTRY_BROM_MEMCPY          (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_MEMCPY)))
#define ENTRY_BROM_MEMCMP          (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_MEMCMP)))
#define ENTRY_BROM_US_DELAY        (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_US_DELAY)))
#define ENTRY_BROM_TIMESTAMP_GET_US     (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_TIMESTAMP_GET_US)))
#define ENTRY_BROM_PRINTF_CONFIG   (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_PRINTF_CONFIG)))
#define ENTRY_BROM_PRINTF          (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_PRINTF)))
#define ENTRY_BROM_PRINTHEX_CONFIG (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_PRINTHEX_CONFIG)))
#define ENTRY_BROM_PRINTHEX        (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_PRINTHEX)))
#define ENTRY_BROM_TRACE_SET_MODE  (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_TRACE_SET_MODE)))
#define ENTRY_BROM_TRACE_OUTPUT    (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_TRACE_OUTPUT)))
#define ENTRY_BROM_TRACE_VSNPRINTF (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_TRACE_VSNPRINTF)))
#define ENTRY_BROM_TRACE_DMA_TC_ISR     (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_TRACE_DMA_TC_ISR)))
#define ENTRY_BROM_TRACE_SET_UART       (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_TRACE_SET_UART)))
#define ENTRY_BROM_TRACE_SET_CBKS       (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_TRACE_SET_CBKS)))

#define ENTRY_BROM_DEVICE_GET_BINDING_SPINOR  (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_DEVICE_GET_BINDING_SPINOR)))
#define ENTRY_BROM_DEVICE_GET_BINDING_SPIMEM  (*(u32_t*)(BROM_EXPORT_APIS_ENTRY_ADDR(API_BROM_DEVICE_GET_BINDING_SPIMEM)))

#define BROM_ADFU_LAUNCHER(a)      ((void(*)(u8_t mode))ENTRY_BROM_ADFU_LAUNCHER)(a)
#define BROM_CARD_LAUNCHER()       ((void(*)(void))ENTRY_BROM_CARD_LAUNCHER)()

#define BROM_LOCAL_IRQ_SAVE()      ((u32_t(*)(void))ENTRY_BROM_LOCAL_IRQ_SAVE)()
#define BROM_LOCAL_IRQ_RESTORE(a)  ((void(*)(u32_t key))ENTRY_BROM_LOCAL_IRQ_RESTORE)(a)

#define BROM_MEMSET(a,b,c)         ((void*(*)(void* , int , u32_t ))ENTRY_BROM_MEMSET)((a), (b), (c))
#define BROM_MEMCPY(a,b,c)         ((void*(*)(void* , const void* , u32_t ))ENTRY_BROM_MEMCPY)((a), (b), (c))
#define BROM_MEMCMP(a,b,c)         ((int(*)(const void* , const void* , u32_t ))ENTRY_BROM_MEMCMP)((a), (b), (c))

#define BROM_UDELAY(a)             ((void(*)(u32_t num))ENTRY_BROM_US_DELAY)(a)
#define BROM_TIMESTAMP_GET_US()    ((u32_t(*)(void))ENTRY_BROM_TIMESTAMP_GET_US)()

#define BROM_PRINTF_CONFIG(a,b,c)  ((int(*)(u32_t print_option, u8_t *debug_buf, u32_t debug_buf_size))ENTRY_BROM_PRINTF_CONFIG)((a), (b), (c))
#define BROM_PRINTF(...)           ((int(*)(const char *fmt, ...))ENTRY_BROM_PRINTF)(__VA_ARGS__)
#define BROM_TRACE_OUTPUT(a,b)     ((void(*)(u8_t *s, u32_t len))ENTRY_BROM_TRACE_OUTPUT)((a),(b))
#define BROM_TRACE_VSNPRINTF(a,b,c,d,e)  ((int(*)(char* buf, int size, u32_t linesep, const char* format, va_list vargs))ENTRY_BROM_TRACE_VSNPRINTF)((a),(b),(c),(d),(e))
#define BROM_TRACE_DMA_TC_ISR()          ((void(*)(void))ENTRY_BROM_TRACE_DMA_TC_ISR)()
#define BROM_TRACE_SET_MODE(a,b,c)       ((int(*)(u8_t mode, u8_t dma_chan_idx, void *cbuf))ENTRY_BROM_TRACE_SET_MODE)((a), (b), (c))
#define BROM_TRACE_SET_UART(a,b,c)       ((int(*)(u8_t uart_idx, u8_t io_idx, u32_t uart_baud))ENTRY_BROM_TRACE_SET_UART)((a), (b), (c))
#define BROM_TRACE_SET_CBKS(a)           ((int(*)(trace_callbacks_t *p_cbks))ENTRY_BROM_TRACE_SET_CBKS)((a))

#define BROM_PRINTHEX_CONFIG(a,b,c)      ((int(*)(u32_t dump_option, u8_t *dump_buf, u32_t dump_buf_size))ENTRY_BROM_PRINTHEX_CONFIG)((a), (b), (c))
#define BROM_PRINTHEX(a,b,c)             ((int(*)(u8_t* leader, void* buf, u32_t len))ENTRY_BROM_PRINTHEX)((a), (b), (c))

#define BROM_DEVICE_GET_BINDING_SPINOR() ((void *(*)(void))ENTRY_BROM_DEVICE_GET_BINDING_SPINOR)()
#define BROM_DEVICE_GET_BINDING_SPIMEM() ((void *(*)(void))ENTRY_BROM_DEVICE_GET_BINDING_SPIMEM)()


#ifdef __cplusplus
}
#endif

#endif /* __BROM_INTERFACE_H__ */
