/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <kernel.h>
#include <device.h>
#include <spi.h>
#include <misc/__assert.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

#ifdef CONFIG_TRACING_BACKEND_SPI_BY_SPI_DRIVER
struct spi_config trace_spi_cfg;
#endif

static void tracing_backend_spi_output(
		const struct tracing_backend *backend,
		u8_t *data, u32_t length)
{
	//print_buffer(data, 1, length, 16, (unsigned int)data);
#ifdef CONFIG_TRACING_BACKEND_SPI_BY_SPI_DRIVER
	struct spi_buf tx_buf;

	tx_buf.buf = data;
	tx_buf.len = length;

	spi_write(&trace_spi_cfg, &tx_buf, 1);
#else
#ifdef CONFIG_TRACING_BACKEND_SPI_USE_DMA
	sys_write32(0x80080822, 0xc00f0000);

	/* use dma3 */
	sys_write32(0x00228900, 0xc0010300);
	sys_write32((u32_t)data, 0xc0010308);
	sys_write32(length, 0xc0010318);
	sys_write32(0x1, 0xc0010304);

	while (sys_read32(0xc0010304) & 0x1)
		;
#else
	__ASSERT((((unsigned int)data & 0x3) == 0) && ((length & 0x3) == 0), "unaligned addr");

	sys_write32(0x40080022, 0xc00f0000);

	/* use 32bit width FIFO for performance */
	while (length > 0) {
		if (!(sys_read32(0xc00f0004) & (1 << 5))) {
			sys_write32(*(unsigned int *)data, 0xc00f0008);
			length -= 4;
			data += 4;
		}
	}
#endif
	while (sys_read32(0xc00f0004) & 0x1)
		;

	sys_write32(0x8, 0xc00f0000);
#endif
}

static void tracing_backend_spi_init(void)
{
#ifdef CONFIG_TRACING_BACKEND_SPI_BY_SPI_DRIVER
	trace_spi_cfg.dev = device_get_binding(CONFIG_TRACING_BACKEND_SPI_NAME);
	__ASSERT(trace_spi_cfg.dev, "spi backend binding failed");

	trace_spi_cfg.frequency = 24000000,
	trace_spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LINES_SINGLE;
#else
	sys_write32(sys_read32(0xc0001008) | (1 << 5), 0xc0001008);
	sys_write32(sys_read32(0xc0000000) | (1 << 5), 0xc0000000);
	sys_write32(0x10e, 0xc0001024); /* clk: 16MHz */
	sys_write32(0x8, 0xc00f0000);
#endif
}

const struct tracing_backend_api tracing_backend_spi_api = {
	.init = tracing_backend_spi_init,
	.output  = tracing_backend_spi_output
};

TRACING_BACKEND_DEFINE(tracing_backend_spi, tracing_backend_spi_api);
