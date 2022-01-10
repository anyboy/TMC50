/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file soc_uart.c
*/

#include "att_pattern_header.h"
#include <sys_io.h>
#include "uart_reg_def.h"

unsigned char print_uart_idx;
unsigned char rx_uart_idx;

/**
 * \brief             uart initial
 *
 * \param uart_idx    0 is uart0, 1 is uart1
 * \param io_idx      gpio port
 * \param bard_rate   unit is 1Hz
 */
void uart_init(int uart_idx, int io_idx, int bard_rate)
{
    unsigned int io_reg = GPIO0_CTL + (4 * io_idx);
    unsigned int tx_mfp;
    unsigned int uart_baud_div;
    unsigned int tmp_uart_ctl;

    print_uart_idx = uart_idx;

    if (uart_idx == 0) {
        tx_mfp = 6;

        //save and reset uart1
        sys_write32(sys_read32(MRCR0) & (~(1 << MRCR0_UART0_RESET)), MRCR0);
        sys_write32(sys_read32(MRCR0) | (1 << MRCR0_UART0_RESET), MRCR0);

        //save and enable clk
        sys_write32(sys_read32(CMU_DEVCLKEN0)  | (1 << CMU_DEVCLKEN0_UART0CLKEN), CMU_DEVCLKEN0);
    } else {
        tx_mfp = 7;

        //save and reset uart1
        sys_write32(sys_read32(MRCR0) & (~(1 << MRCR0_UART1_RESET)), MRCR0);
        sys_write32(sys_read32(MRCR0) | (1 << MRCR0_UART1_RESET), MRCR0);

        //save and enable clk
        sys_write32(sys_read32(CMU_DEVCLKEN0)  | (1 << CMU_DEVCLKEN0_UART1CLKEN), CMU_DEVCLKEN0);
    }

    //set mfp
    sys_write32((sys_read32(io_reg) & (~GPIO0_CTL_MFP_MASK)) | (tx_mfp | (4 << GPIO0_CTL_PADDRV_SHIFT)), io_reg);

    uart_baud_div = (CONFIG_HOSC_CLK_MHZ * 1000 * 1000 / bard_rate);
    uart_baud_div = uart_baud_div | (uart_baud_div << 16);

    tmp_uart_ctl = (1 << UART0_CTL_TXENABLE) | (1 << UART0_CTL_TX_FIFO_EN) | (1 << UART0_CTL_TXDE) | (1 << UART0_CTL_EN)
            | (0 << UART0_CTL_RDIC_SHIFT) | (2 << UART0_CTL_TDIC_SHIFT)
            | (0 << UART0_CTL_PRS_SHIFT) | (0 << UART0_CTL_STPS) | (3 << UART0_CTL_DWLS_SHIFT);

    if (uart_idx == 0) {
        sys_write32(uart_baud_div, UART0_BR);
        sys_write32(tmp_uart_ctl, UART0_CTL);
    } else {
        sys_write32(uart_baud_div, UART1_BR);
        sys_write32(tmp_uart_ctl, UART1_CTL);
    }
}

/**
 * \brief             uart output, it will wait for all data flush out and then return
 *
 * \param s           output buffer
 * \param len         output buffer length
 */
void uart_puts(char *s, unsigned int len)
{
    if (print_uart_idx == 0) {
        while (*s != 0 && len != 0) {
            /* Wait for tx fifo not full */
            while (sys_read32(UART0_STA) & (1 << UART0_STA_TFFU));

            sys_write32((unsigned int)(*s), UART0_TXDAT);

            s++;
            len--;
        }

        /* Wait for fifo to flush out */
        while (sys_read32(UART0_STA) & (1 << UART0_STA_UTBB));
    } else {
        while (*s != 0 && len != 0) {
            /* Wait for tx fifo not full */
            while (sys_read32(UART1_STA) & (1 << UART0_STA_TFFU));

            sys_write32((unsigned int)(*s), UART1_TXDAT);

            s++;
            len--;
        }

        /* Wait for fifo to flush out */
        while (sys_read32(UART1_STA) & (1 << UART0_STA_UTBB));
    }
}

/**
 * \brief             uart rx initial
 *
 * \param uart_idx    0 is uart0, 1 is uart1
 * \param io_idx      gpio port
 * \param bard_rate   unit is 1Hz
 */
void uart_rx_init(int uart_idx, int io_idx, int bard_rate)
{
    unsigned int io_reg = GPIO0_CTL + (4 * io_idx);
    unsigned int rx_mfp;
    unsigned int uart_baud_div;
    unsigned int tmp_uart_ctl;

    rx_uart_idx = uart_idx;

    if (uart_idx == 0) {
        rx_mfp = 6;

        if ((sys_read32(MRCR0) & (1 << MRCR0_UART0_RESET)) == 0) {
            //save and reset uart1
            sys_write32(sys_read32(MRCR0) & (~(1 << MRCR0_UART0_RESET)), MRCR0);
            sys_write32(sys_read32(MRCR0) | (1 << MRCR0_UART0_RESET), MRCR0);

            //save and enable clk
            sys_write32(sys_read32(CMU_DEVCLKEN0)  | (1 << CMU_DEVCLKEN0_UART0CLKEN), CMU_DEVCLKEN0);
        }
    } else {
        rx_mfp = 7;

        if ((sys_read32(MRCR0) | (1 << MRCR0_UART1_RESET)) == 0) {
            //save and reset uart1
            sys_write32(sys_read32(MRCR0) & (~(1 << MRCR0_UART1_RESET)), MRCR0);
            sys_write32(sys_read32(MRCR0) | (1 << MRCR0_UART1_RESET), MRCR0);

            //save and enable clk
            sys_write32(sys_read32(CMU_DEVCLKEN0)  | (1 << CMU_DEVCLKEN0_UART1CLKEN), CMU_DEVCLKEN0);
        }
    }

    //set mfp
    sys_write32((sys_read32(io_reg) & (~GPIO0_CTL_MFP_MASK)) | (rx_mfp | (4 << GPIO0_CTL_PADDRV_SHIFT)), io_reg);

    uart_baud_div = (CONFIG_HOSC_CLK_MHZ * 1000 * 1000 / bard_rate);
    uart_baud_div = uart_baud_div | (uart_baud_div << 16);

    if (uart_idx == 0)
        tmp_uart_ctl = sys_read32(UART0_CTL);
    else
        tmp_uart_ctl = sys_read32(UART1_CTL);

    tmp_uart_ctl |= (1 << UART0_CTL_RXENABLE) | (1 << UART0_CTL_RX_FIFO_EN) | (1 << UART0_CTL_RXDE) | (1 << UART0_CTL_EN)
            | (0 << UART0_CTL_RDIC_SHIFT) | (2 << UART0_CTL_TDIC_SHIFT)
            | (0 << UART0_CTL_PRS_SHIFT) | (0 << UART0_CTL_STPS) | (3 << UART0_CTL_DWLS_SHIFT);

    if (uart_idx == 0) {
        sys_write32(uart_baud_div, UART0_BR);
        sys_write32(tmp_uart_ctl, UART0_CTL);
    } else {
        sys_write32(uart_baud_div, UART1_BR);
        sys_write32(tmp_uart_ctl, UART1_CTL);
    }
}

/**
 * \brief             uart rx read data
 *
 * \param buf         rx read buffer
 * \param max_len     max length
 */
unsigned int uart_gets(char *buf, unsigned int max_len)
{
    unsigned int uart_rx_bytes = 0;
    unsigned int uart_base = (rx_uart_idx == 0)? UART0_BASE: UART1_BASE;

    while ((uart_rx_bytes < max_len)
        && ((sys_read32(uart_base + (UART0_STA - UART0_BASE)) & (1<<UART0_STA_RFEM)) == 0)) {
        buf[uart_rx_bytes++] = (unsigned char) sys_read32(uart_base + (UART0_RXDAT - UART0_BASE));
    }

    return uart_rx_bytes;
}

