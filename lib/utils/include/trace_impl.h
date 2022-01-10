/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file trace impl interface
 */

#ifndef TRACE_IMPL_H_
#define TRACE_IMPL_H_

#define TRACE_TEMP_BUF_SIZE   128

#define TRACE_HEX_BUF_SIZE    128

#define TRACE_PRINT_BUF_SIZE  (1024)

typedef enum
{
    TRACE_TRANSPORT_UART = (1 << 0),
    TRACE_TRANSPORT_USB = (1 << 1),
    TRACE_TRANSPORT_SPI = (1 << 2),
    TRACE_TRANSPORT_NORFLASH = (1 << 3),
    TRACE_TRANSPORT_FILE = (1 << 4),
}trace_transport_type_e;

typedef enum
{
    TRACE_EVENT_TX = 1,
    TRACE_EVENT_TX_COMPLETED,
}trace_msg_id_e;

typedef struct
{
    cbuf_t cbuf;
#ifdef CONFIG_TRACE_EVENT
    cbuf_t cbuf_binary;
#endif
    uint16_t drop_bytes;
    uint8_t  sending;
    uint8_t transport;
    uint8_t trace_mode;
    uint8_t init;
    uint8_t irq_print_disable;
    uint8_t dma_print_disable;
    uint8_t in_trace;
    uint8_t nested;
    uint8_t panic;
    uint32_t caller;
    cbuf_dma_t dma_setting;

#ifdef CONFIG_TRACE_PROFILE_ENABLE
    uint32_t hex_time;
    uint32_t print_time;
    uint32_t convert_time;
#endif
}trace_ctx_t;

extern trace_ctx_t g_trace_ctx;

#define get_trace_ctx() (&g_trace_ctx)

static inline int trace_transport_support(trace_ctx_t *trace_ctx, int transport)
{
    return trace_ctx->transport & transport;
}

extern uint32_t trace_binary_read(void *buf, int read_len);

void trace_set_panic(void);

#endif /* TRACE_IMPL_H_ */
