/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <random.h>
#include <drivers/rand32.h>
#include <init.h>
#include <string.h>

#include "soc_se.h"

static int random_acts_trng_get_entropy(struct device *dev, u8_t *buffer,
					u16_t length)
{
	u32_t trng_val[2];
	u16_t remain, readptr;

	ARG_UNUSED(dev);

	se_trng_init();

	remain  = length;
	readptr = 0;
	while (remain > 0) {
		se_trng_process(&trng_val[0], &trng_val[1]);
		if (remain >= 8) {
			memcpy(buffer + readptr, (u8_t *)trng_val, 8);
			remain  -= 8;
			readptr += 8;
		} else {
			memcpy(buffer + readptr, (u8_t *)trng_val, remain);
			remain  = 0;
		}
	}

	se_trng_deinit();

	return 0;
}

static const struct random_driver_api random_acts_trng_api_funcs = {
	.get_entropy = random_acts_trng_get_entropy
};

static int random_acts_trng_init(struct device *);

DEVICE_AND_API_INIT(random_acts_trng, CONFIG_RANDOM_NAME,
		    random_acts_trng_init, NULL, NULL,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &random_acts_trng_api_funcs);

static int random_acts_trng_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

u32_t sys_rand32_get(void)
{
	struct device *trng_dev;
	u32_t output;
	int ret;

	trng_dev = device_get_binding(CONFIG_RANDOM_NAME);
	if (!trng_dev)
		return -1;

	ret = random_acts_trng_get_entropy(trng_dev, (u8_t *) &output, sizeof(output));

	return output;
}
