/** @file
 * @brief USB UART driver
 *
 * A UART driver which use USB CDC ACM protocol implementation.
 */

/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <uart.h>
#include <console/uart_usb.h>
#include <drivers/usb/usb_phy.h>
#include <usb/class/usb_cdc.h>

#define UART_USB_SEND_TIMEOUT_US (1000)

typedef struct {
	struct device *usb_dev;
	u8_t state : 1; /* 1: initialized 0: uninitialized */
	u8_t tx_done : 1; /* 1: tx done 0: tx not done  */
} uart_usb_dev_t, *p_uart_dev_t;

static p_uart_dev_t uart_usb_get_dev(void)
{
	static uart_usb_dev_t uart_usb_dev = {0};
	return &uart_usb_dev;
}

void uart_usb_update_tx_done(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();
	p_uart_dev->tx_done = 1;
}

int uart_usb_send(const u8_t *data, int len)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();
	u32_t start, duration;
	int ret;

	if (!p_uart_dev->state)
		return -ESRCH;

	uart_irq_tx_enable(p_uart_dev->usb_dev);

	p_uart_dev->tx_done = 0;
	ret = uart_fifo_fill(p_uart_dev->usb_dev, data, len);
	if (ret < 0)
		return ret;

	/* wait tx done */
	start = k_cycle_get_32();
	while (!p_uart_dev->tx_done) {
		duration = k_cycle_get_32() - start;
		if (SYS_CLOCK_HW_CYCLES_TO_NS_AVG(duration, 1000)
			> UART_USB_SEND_TIMEOUT_US) {
			uart_irq_tx_disable(p_uart_dev->usb_dev);
			return -ETIMEDOUT;
		}
	}
	uart_irq_tx_disable(p_uart_dev->usb_dev);

	return 0;
}

int uart_usb_receive(u8_t *data, int max_len)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();
	int rlen;

	memset(data, 0, max_len);

	rlen = uart_fifo_read(p_uart_dev->usb_dev, data, max_len);
	if (rlen < 0) {
		uart_irq_rx_disable(p_uart_dev->usb_dev);
		return -EIO;
	}

	/* exceptional that data received too much and drain all data */
	if (rlen >= max_len) {
		while (uart_fifo_read(p_uart_dev->usb_dev, data, max_len) > 0);
		rlen = 0;
	}

	return rlen;
}

bool uart_usb_is_enabled(void)
{
	return true;
}

int uart_usb_init(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	if (!p_uart_dev->state) {
		p_uart_dev->usb_dev = device_get_binding(CONFIG_CDC_ACM_PORT_NAME);
		if (!p_uart_dev->usb_dev) {
			printk("failed to bind usb dev:%s\n", CONFIG_CDC_ACM_PORT_NAME);
			return -ENODEV;
		}

		if (usb_cdc_acm_init(p_uart_dev->usb_dev)) {
			printk("failed to init CDC ACM device\n");
			return -EFAULT;
		}

		p_uart_dev->state = 1;
		p_uart_dev->tx_done = 0;
		printk("uart usb init ok\n");
	}

	return 0;
}

void uart_usb_uninit(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	if (p_uart_dev->state) {
		usb_cdc_acm_exit();
		memset(p_uart_dev, 0, sizeof(uart_usb_dev_t));
		printk("uart usb uninit ok\n");
	}
}
