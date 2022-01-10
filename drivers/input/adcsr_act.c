/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC Sliding rheostat Key driver
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

#define SYS_LOG_DOMAIN "ADC_SR"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <logging/sys_log.h>

static const u16_t SR_ADC_CHAN[] = {SR_1_ADC_CHAN, SR_2_ADC_CHAN};
#define SR_NUM	(ARRAY_SIZE(SR_ADC_CHAN))

struct adcsr_map {
	u16_t adc_val_l;
	u16_t adc_val_r;
};

struct acts_adcsr_data {
	struct k_timer timer;

	struct device *adc_dev;
	struct adc_seq_table seq_tbl;
	struct adc_seq_entry seq_entrys;
	u8_t adc_buf[4];

	s8_t pre_level[SR_NUM];
	s8_t pre_report_level[SR_NUM];

	input_notify_t notify;
};

struct acts_adcsr_config {
	char *adc_name;
	const u16_t *adc_chan;

	u16_t poll_interval_ms;

	u16_t sr_cnt;
	const struct adcsr_map *sr_maps;
};

/*  */
static s16_t ALWAYS_INLINE adcsr_acts_get_srlevel(const struct acts_adcsr_config *cfg,
				     struct acts_adcsr_data *adcsr,
				     int adc_val)
{
	const struct adcsr_map *map = cfg->sr_maps;
	int low = 0;
	int hi = cfg->sr_cnt - 1;
	int mid;

	do {
		mid = (low + hi) >> 1;
		if (adc_val > map[mid].adc_val_r) {
			low = mid + 1;
		} else if (adc_val < map[mid].adc_val_l) {
			hi = mid - 1;
		} else {
			return cfg->sr_cnt - 1 - mid;
		}
	} while (low <= hi);

	return -1;
}

static void adcsr_acts_report_srlevel(struct acts_adcsr_data *adcsr, u16_t sr_id, s8_t value)
{
	struct input_value val;

	if (adcsr->notify) {
		val.type = EV_SR;
		val.code = sr_id;
		val.value = (uint32_t)value;
		SYS_LOG_DBG("report value %d", value);

		adcsr->notify(NULL, &val);
	}
}

static void __ramfunc adcsr_acts_poll(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	const struct acts_adcsr_config *cfg = dev->config->config_info;
	struct acts_adcsr_data *adcsr = dev->driver_data;
	u16_t i, adcval;
	s16_t srlevel;

	/* get adc value */
	for (i = 0; i < SR_NUM; i++) {
		adcsr->seq_entrys.channel_id = SR_ADC_CHAN[i];

		adc_read(adcsr->adc_dev, &adcsr->seq_tbl);

		adcval = sys_get_le16(adcsr->seq_entrys.buffer);

		/* get keylevel */
		srlevel = adcsr_acts_get_srlevel(cfg, adcsr, adcval);
		if ((srlevel != -1) && (srlevel != adcsr->pre_level[i]))
		{
			adcsr->pre_level[i] = srlevel;
			SYS_LOG_DBG("keylevel %d", srlevel);
		}
		if (adcsr->pre_level[i] != adcsr->pre_report_level[i])
		{
			adcsr->pre_report_level[i] = adcsr->pre_level[i];
			adcsr_acts_report_srlevel(adcsr, i, adcsr->pre_level[i]);
		}
	}
}

static void adcsr_acts_enable(struct device *dev)
{
	const struct acts_adcsr_config *cfg = dev->config->config_info;
	struct acts_adcsr_data *adcsr = dev->driver_data;

	SYS_LOG_DBG("enable adcsr");

	/* open ALL sliding rheostat GPIO LRADC function */
	sys_write32((sys_read32(GPIO_CTL(SR_1_GPIO)) & 0xfff0) | GPIO_CTL_AD_SELECT, GPIO_CTL(SR_1_GPIO));
	sys_write32((sys_read32(GPIO_CTL(SR_2_GPIO)) & 0xfff0) | GPIO_CTL_AD_SELECT, GPIO_CTL(SR_2_GPIO));

	adc_enable(adcsr->adc_dev);
	k_timer_start(&adcsr->timer, cfg->poll_interval_ms,
		cfg->poll_interval_ms);
}

static void adcsr_acts_disable(struct device *dev)
{
	struct acts_adcsr_data *adcsr = dev->driver_data;

	SYS_LOG_DBG("disable adcsr");

	k_timer_stop(&adcsr->timer);
	adc_disable(adcsr->adc_dev);

	adcsr->notify = NULL;
}

static void adcsr_acts_register_notify(struct device *dev, input_notify_t notify)
{
	struct acts_adcsr_data *adcsr = dev->driver_data;

	SYS_LOG_DBG("register notify 0x%x", (u32_t)notify);

	adcsr->notify = notify;
}

static void adcsr_acts_unregister_notify(struct device *dev, input_notify_t notify)
{
	struct acts_adcsr_data *adcsr = dev->driver_data;

	SYS_LOG_DBG("unregister notify 0x%x", (u32_t)notify);

	adcsr->notify = NULL;
}

const struct input_dev_driver_api adcsr_acts_driver_api = {
	.enable = adcsr_acts_enable,
	.disable = adcsr_acts_disable,
	.register_notify = adcsr_acts_register_notify,
	.unregister_notify = adcsr_acts_unregister_notify,
};

int adcsr_acts_init(struct device *dev)
{
	const struct acts_adcsr_config *cfg = dev->config->config_info;
	struct acts_adcsr_data *adcsr = dev->driver_data;

	adcsr->adc_dev = device_get_binding(cfg->adc_name);
	if (!adcsr->adc_dev) {
		SYS_LOG_ERR("cannot found adc dev %s\n", cfg->adc_name);
		return -ENODEV;
	}

	adcsr->seq_entrys.sampling_delay = 0;
	adcsr->seq_entrys.buffer = &adcsr->adc_buf[0];
	adcsr->seq_entrys.buffer_length = sizeof(adcsr->adc_buf);

	adcsr->seq_tbl.entries = &adcsr->seq_entrys;
	adcsr->seq_tbl.num_entries = 1;
	memset(adcsr->pre_level, -1, SR_NUM);
	memset(adcsr->pre_report_level, -1, SR_NUM);

	k_timer_init(&adcsr->timer, adcsr_acts_poll, NULL);
	k_timer_user_data_set(&adcsr->timer, dev);

	return 0;
}

static struct acts_adcsr_data adcsr_acts_ddata;

static const struct adcsr_map adcsr_acts_maps[] = {
 	{ .adc_val_l = 0, .adc_val_r = 50 },       // 0
 	{ .adc_val_l = 65, .adc_val_r = 115 },     // 1
 	{ .adc_val_l = 130, .adc_val_r = 170 },    // 2
 	{ .adc_val_l = 185, .adc_val_r = 225 },	   // 3
 	{ .adc_val_l = 240, .adc_val_r = 275 },    // 4
 	{ .adc_val_l = 290, .adc_val_r = 325 },    // 5
 	{ .adc_val_l = 340, .adc_val_r = 375 },    // 6
 	{ .adc_val_l = 390, .adc_val_r = 425 },    // 7
 	{ .adc_val_l = 440, .adc_val_r = 470 },    // 8
 	{ .adc_val_l = 485, .adc_val_r = 515 },	   // 9
 	{ .adc_val_l = 530, .adc_val_r = 560 },    // 10
 	{ .adc_val_l = 575, .adc_val_r = 591 },	   // 11
 	{ .adc_val_l = 605, .adc_val_r = 621 },    // 12
 	{ .adc_val_l = 635, .adc_val_r = 651 },    // 13
 	{ .adc_val_l = 665, .adc_val_r = 681 },    // 14
 	{ .adc_val_l = 695, .adc_val_r = 711 },	   // 15
	{ .adc_val_l = 723, .adc_val_r = 0xffff }, // 16
};


static const struct acts_adcsr_config adcsr_acts_cdata = {
	.adc_name = CONFIG_INPUT_DEV_ACTS_SR_ADC_NAME,
	.adc_chan = &SR_ADC_CHAN[0],

	.poll_interval_ms = 20,

	.sr_cnt = ARRAY_SIZE(adcsr_acts_maps),
	.sr_maps = &adcsr_acts_maps[0],
};

DEVICE_AND_API_INIT(adcsr_acts, CONFIG_INPUT_DEV_ACTS_ADC_SR_NAME,
		    adcsr_acts_init,
		    &adcsr_acts_ddata, &adcsr_acts_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adcsr_acts_driver_api);


