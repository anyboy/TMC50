/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <kernel.h>
#include <device.h>
#include <misc/__assert.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

static void tracing_backend_hexdump_output(
		const struct tracing_backend *backend,
		u8_t *data, u32_t length)
{
	print_buffer(data, 1, length, 16, (unsigned int)data);
}

static void tracing_backend_hexdump_init(void)
{
}

const struct tracing_backend_api tracing_backend_hexdump_api = {
	.init = tracing_backend_hexdump_init,
	.output  = tracing_backend_hexdump_output
};

TRACING_BACKEND_DEFINE(tracing_backend_hexdump, tracing_backend_hexdump_api);
