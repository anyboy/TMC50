/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <random.h>
#include <drivers/rand32.h>
#include <init.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <errno.h>
#include <soc.h>
#include <misc/printk.h>
#include <clock_control.h>
#include <clock_control/stm32_clock_control.h>

struct random_stm32_rng_dev_cfg {
	struct stm32_pclken pclken;
};

struct random_stm32_rng_dev_data {
	RNG_TypeDef *rng;
	struct device *clock;
};

#define DEV_DATA(dev) \
	((struct random_stm32_rng_dev_data *)(dev)->driver_data)

#define DEV_CFG(dev) \
	((struct random_stm32_rng_dev_cfg *)(dev)->config->config_info)

static void random_stm32_rng_reset(RNG_TypeDef *rng)
{
	__ASSERT_NO_MSG(rng != NULL);

	/* Reset RNG as described in RM0090 Reference manual
	 * section 24.3.2 Error management.
	 */
	LL_RNG_ClearFlag_CEIS(rng);
	LL_RNG_ClearFlag_SEIS(rng);

	LL_RNG_Disable(rng);
	LL_RNG_Enable(rng);
}

static int random_stm32_got_error(RNG_TypeDef *rng)
{
	__ASSERT_NO_MSG(rng != NULL);

	if (LL_RNG_IsActiveFlag_CECS(rng)) {
		return 1;
	}

	if (LL_RNG_IsActiveFlag_SECS(rng)) {
		return 1;
	}

	return 0;
}

static int random_stm32_wait_ready(RNG_TypeDef *rng)
{
	/* Agording to the reference manual it takes 40 periods
	 * of the RNG_CLK clock signal between two consecutive
	 * random numbers. Also RNG_CLK may not be smaller than
	 * HCLK/16. So it should not take more than 640 HCLK
	 * ticks. Assuming the CPU can do 1 instruction per HCLK
	 * the number of times to loop before the RNG is ready
	 * is less than 1000. And that is when assumming the loop
	 * only takes 1 instruction. So looping a million times
	 * should be more than enough.
	 */

	int timeout = 1000000;

	__ASSERT_NO_MSG(rng != NULL);

	while (!LL_RNG_IsActiveFlag_DRDY(rng)) {
		if (random_stm32_got_error(rng)) {
			return -EIO;
		}

		if (timeout-- == 0) {
			return -ETIMEDOUT;
		}

		k_yield();
	}

	if (random_stm32_got_error(rng)) {
		return -EIO;
	} else {
		return 0;
	}
}

static int random_stm32_rng_get_entropy(struct device *dev, u8_t *buffer,
					u16_t length)
{
	struct random_stm32_rng_dev_data *dev_data;
	int n = sizeof(u32_t);
	int res;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buffer != NULL);

	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev_data != NULL);

	/* if the RNG has errors reset it before use */
	if (random_stm32_got_error(dev_data->rng)) {
		random_stm32_rng_reset(dev_data->rng);
	}

	while (length > 0) {
		u32_t rndbits;
		u8_t *p_rndbits = (u8_t *)&rndbits;

		res = random_stm32_wait_ready(dev_data->rng);
		if (res < 0)
			return res;

		rndbits = LL_RNG_ReadRandData32(dev_data->rng);

		if (length < sizeof(u32_t))
			n = length;

		for (int i = 0; i < n; i++) {
			*buffer++ = *p_rndbits++;
		}

		length -= n;
	}

	return 0;
}

static int random_stm32_rng_init(struct device *dev)
{
	struct random_stm32_rng_dev_data *dev_data;
	struct random_stm32_rng_dev_cfg *dev_cfg;

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);
	dev_cfg = DEV_CFG(dev);

	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(dev_cfg != NULL);

	dev_data->clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(dev_data->clock != NULL);

	clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&dev_cfg->pclken);

	LL_RNG_Enable(dev_data->rng);

	return 0;
}

static const struct random_driver_api random_stm32_rng_api = {
	.get_entropy = random_stm32_rng_get_entropy
};

static const struct random_stm32_rng_dev_cfg random_stm32_rng_config = {
	.pclken	= { .bus = STM32_CLOCK_BUS_AHB2,
		    .enr = LL_AHB2_GRP1_PERIPH_RNG },
};

static struct random_stm32_rng_dev_data random_stm32_rng_data = {
	.rng = RNG,
};

DEVICE_AND_API_INIT(random_stm32_rng, CONFIG_RANDOM_NAME,
		    random_stm32_rng_init,
		    &random_stm32_rng_data, &random_stm32_rng_config,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &random_stm32_rng_api);

u32_t sys_rand32_get(void)
{
	u32_t output;
	int rc;

	rc = random_stm32_rng_get_entropy(DEVICE_GET(random_stm32_rng),
					  (u8_t *) &output, sizeof(output));
	__ASSERT_NO_MSG(!rc);

	return output;
}

