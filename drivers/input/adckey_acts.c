/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC Keyboard driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <adc.h>
#include <input_dev.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <board.h>

#define SYS_LOG_DOMAIN "ADC_KEY"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_INPUT_DEV_LEVEL
#include <logging/sys_log.h>

struct adckey_map {
	u16_t key_code;
	u16_t adc_val;
};

struct acts_adckey_data {
	struct k_timer timer;

	struct device *adc_dev;
	struct adc_seq_table seq_tbl;
	struct adc_seq_entry seq_entrys;
	u8_t adc_buf[4];

	int scan_count;
	u16_t prev_keycode;
	u16_t prev_stable_keycode;

	input_notify_t notify;
};

struct acts_adckey_config {
	char *adc_name;
	u16_t adc_chan;

	u16_t poll_interval_ms;
	u16_t sample_filter_dep;

	u16_t key_cnt;
	const struct adckey_map *key_maps;
};

/* adc_val -> key_code */
static u16_t adckey_acts_get_keycode(const struct acts_adckey_config *cfg,
				     struct acts_adckey_data *adckey,
				     int adc_val)
{
	const struct adckey_map *map = cfg->key_maps;
	int i;

	for (i = 0; i < cfg->key_cnt; i++) {
		if (adc_val < map->adc_val)
			return map->key_code;

		map++;
	}

	return KEY_RESERVED;
}

static void adckey_acts_report_key(struct acts_adckey_data *adckey,
				   int key_code, int value)
{
	struct input_value val;

	if (adckey->notify) {
		val.type = EV_KEY;
		val.code = key_code;
		val.value = value;

		SYS_LOG_DBG("report key_code %d value %d",
			key_code, value);

		adckey->notify(NULL, &val);
	}
}

static void adckey_acts_poll(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	const struct acts_adckey_config *cfg = dev->config->config_info;
	struct acts_adckey_data *adckey = dev->driver_data;
	u16_t keycode, adcval;

	/* get adc value */
	adc_read(adckey->adc_dev, &adckey->seq_tbl);

	adcval = sys_get_le16(adckey->seq_entrys.buffer);

	/* get keycode */
	keycode = adckey_acts_get_keycode(cfg, adckey, adcval);

	SYS_LOG_DBG("adcval %d, keycode %d", adcval, keycode);

	/* no key is pressed or releasing */
	if (keycode == KEY_RESERVED &&
	    adckey->prev_stable_keycode == KEY_RESERVED)
		return;

	if (keycode == adckey->prev_keycode) {
		adckey->scan_count++;
		if (adckey->scan_count == cfg->sample_filter_dep) {
			/* previous key is released? */
			if (adckey->prev_stable_keycode != KEY_RESERVED
				&& keycode != adckey->prev_stable_keycode)
				adckey_acts_report_key(adckey,
					adckey->prev_stable_keycode, 0);

			/* a new key press? */
			if (keycode != KEY_RESERVED)
				adckey_acts_report_key(adckey, keycode, 1);

			/* clear counter for new checking */
			adckey->prev_stable_keycode = keycode;
			adckey->scan_count = 0;
		}
	} else {
		/* new key pressed? */
		adckey->prev_keycode = keycode;
		adckey->scan_count = 0;
	}
}

static void adckey_acts_enable(struct device *dev)
{
	const struct acts_adckey_config *cfg = dev->config->config_info;
	struct acts_adckey_data *adckey = dev->driver_data;

	SYS_LOG_DBG("enable adckey");

	adc_enable(adckey->adc_dev);
	k_timer_start(&adckey->timer, cfg->poll_interval_ms,
		cfg->poll_interval_ms);
}

static void adckey_acts_disable(struct device *dev)
{
	struct acts_adckey_data *adckey = dev->driver_data;

	SYS_LOG_DBG("disable adckey");

	k_timer_stop(&adckey->timer);
	adc_disable(adckey->adc_dev);

	//adckey->notify = NULL;
}

static void adckey_acts_register_notify(struct device *dev, input_notify_t notify)
{
	struct acts_adckey_data *adckey = dev->driver_data;

	SYS_LOG_DBG("register notify 0x%x", (u32_t)notify);

	adckey->notify = notify;
}

static void adckey_acts_unregister_notify(struct device *dev, input_notify_t notify)
{
	struct acts_adckey_data *adckey = dev->driver_data;

	SYS_LOG_DBG("unregister notify 0x%x", (u32_t)notify);

	adckey->notify = NULL;
}

const struct input_dev_driver_api adckey_acts_driver_api = {
	.enable = adckey_acts_enable,
	.disable = adckey_acts_disable,
	.register_notify = adckey_acts_register_notify,
	.unregister_notify = adckey_acts_unregister_notify,
};

int adckey_acts_init(struct device *dev)
{
	const struct acts_adckey_config *cfg = dev->config->config_info;
	struct acts_adckey_data *adckey = dev->driver_data;

	adckey->adc_dev = device_get_binding(cfg->adc_name);
	if (!adckey->adc_dev) {
		SYS_LOG_ERR("cannot found adc dev %s\n", cfg->adc_name);
		return -ENODEV;
	}

	adckey->seq_entrys.sampling_delay = 0;
	adckey->seq_entrys.buffer = &adckey->adc_buf[0];
	adckey->seq_entrys.buffer_length = sizeof(adckey->adc_buf);
	adckey->seq_entrys.channel_id = cfg->adc_chan;

	adckey->seq_tbl.entries = &adckey->seq_entrys;
	adckey->seq_tbl.num_entries = 1;

	k_timer_init(&adckey->timer, adckey_acts_poll, NULL);
	k_timer_user_data_set(&adckey->timer, dev);

	return 0;
}

static struct acts_adckey_data adckey_acts_ddata;

static const struct adckey_map adckey_acts_keymaps[] = {
#ifdef BOARD_ADCKEY_KEY_MAPPINGS
	BOARD_ADCKEY_KEY_MAPPINGS
#endif
};

static const struct acts_adckey_config adckey_acts_cdata = {
	.adc_name = CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_NAME,
	.adc_chan = CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_CHAN,

	.poll_interval_ms = 10,//20,
	.sample_filter_dep = 3,

	.key_cnt = ARRAY_SIZE(adckey_acts_keymaps),
	.key_maps = &adckey_acts_keymaps[0],
};

DEVICE_AND_API_INIT(adckey_acts, CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME,
		    adckey_acts_init,
		    &adckey_acts_ddata, &adckey_acts_cdata,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adckey_acts_driver_api);
