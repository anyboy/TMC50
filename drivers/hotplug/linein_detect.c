
/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief linein detect driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <adc.h>
#include <hotplug.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <board.h>
#include <gpio.h>

#define SYS_LOG_DOMAIN "LINEIN_DETECT"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LINEIN_DETECT_LEVEL
#include <logging/sys_log.h>

struct linein_detect_info {
#ifndef BOARD_LINEIN_DETECT_GPIO
	struct device *adc_dev;
	struct adc_seq_table seq_tbl;
	struct adc_seq_entry seq_entrys;
	u8_t adcval[4];
#else
	struct device *dectect_gpio_dev;
#endif
};

struct linein_detect_config {
	char *adc_name;
	u16_t adc_chan;
};

#ifndef BOARD_LINEIN_DETECT_GPIO
struct linein_vol_range {
	u16_t min;
	u16_t max;
};

struct linein_vol_range linein_vol = {
#ifdef LINEIN_DETEC_ADVAL_RANGE
	LINEIN_DETEC_ADVAL_RANGE
#endif
};

static unsigned int linein_detect_adcval_to_state(unsigned int adcval)
{
	if (adcval >= linein_vol.min && adcval <= linein_vol.max) {
		return LINEIN_IN;
	} else if (adcval > linein_vol.max) {
		return LINEIN_OUT;
	}
	return LINEIN_NONE;
}
#endif

static int linein_detect_state(struct device *dev, int *state)
{
	struct linein_detect_info *linein_detect = dev->driver_data;
#ifdef BOARD_LINEIN_DETECT_GPIO
	int err, value;
	err = gpio_pin_read(linein_detect->dectect_gpio_dev, BOARD_LINEIN_DETECT_GPIO,
			    &value);
	if (err) {
		return 0;
	}

	if (value == BOARD_LINEIN_DETECT_GPIO_VALUE) {
		*state = LINEIN_IN;
	} else {
		*state = LINEIN_OUT;
	}
#else
	int adc_val;

	adc_read(linein_detect->adc_dev, &linein_detect->seq_tbl);
	adc_val = sys_get_le16(linein_detect->seq_entrys.buffer);

	*state = linein_detect_adcval_to_state(adc_val);
#endif
	return 0;
}

static const struct hotplog_detect_driver_api linein_detect_driver_api = {
	.detect_state = linein_detect_state,
};

static int linein_detect_init(struct device *dev)
{
	struct linein_detect_info *linein_detect = dev->driver_data;

#ifdef BOARD_LINEIN_DETECT_GPIO
	linein_detect->dectect_gpio_dev = device_get_binding(BOARD_LINEIN_DETECT_GPIO_NAME);
	if (!linein_detect->dectect_gpio_dev) {
		SYS_LOG_ERR("device binding failed %s\n", BOARD_LINEIN_DETECT_GPIO_NAME);
		return -ENODEV;
	}
	gpio_pin_configure(linein_detect->dectect_gpio_dev, BOARD_LINEIN_DETECT_GPIO, GPIO_DIR_IN | GPIO_PUD_PULL_UP);
#else
	const struct linein_detect_config *cfg = dev->config->config_info;
	SYS_LOG_DBG("linein_detect initialized\n");

	linein_detect->adc_dev = device_get_binding(cfg->adc_name);
	if (!linein_detect->adc_dev) {
		SYS_LOG_ERR("cannot found ADC device %s\n", cfg->adc_name);
		return -ENODEV;
	}

	linein_detect->seq_entrys.sampling_delay = 0;
	linein_detect->seq_entrys.buffer = (u8_t *)&linein_detect->adcval;
	linein_detect->seq_entrys.buffer_length = sizeof(linein_detect->adcval);
	linein_detect->seq_entrys.channel_id = cfg->adc_chan;

	linein_detect->seq_tbl.entries = &linein_detect->seq_entrys;
	linein_detect->seq_tbl.num_entries = 1;

	dev->driver_api = &linein_detect_driver_api;

	adc_enable(linein_detect->adc_dev);
#endif
	return 0;
}

static struct linein_detect_info linein_detect_ddata;

static const struct linein_detect_config linein_detect_cdata = {
	.adc_name = CONFIG_HOTPLUG_LINEIN_DETECT_ADC_NAME,
	.adc_chan = CONFIG_HOTPLUG_LINEIN_DETECT_ADC_CHAN,
};

DEVICE_AND_API_INIT(linein_detect, "linein_detect",
		    linein_detect_init,
		    &linein_detect_ddata, &linein_detect_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &linein_detect_driver_api);
