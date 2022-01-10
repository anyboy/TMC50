/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for Actions SoC
 */
#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <adc.h>
#include <soc.h>

#define SYS_LOG_DOMAIN "ADC"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ADC_LEVEL
#include <logging/sys_log.h>

#define ADC_SENSOR_OFFSET       (0)
#define ADC_OTHER_OFFSET        (1)

/* adc delay time before data is ready */
#define ADC_TRANS_DELAY_US		(1000)

/* don't disable ADC channel to quicken next adc transfer */
#define ADC_TRANS_NO_DELAY

/* dma channel registers */
struct acts_adc_controller {
	volatile u32_t ctrl;
};

struct acts_adc_config {
	u32_t base;
	u8_t clock_id;
};

struct acts_adc_data {
	u32_t enable_cnt;
};

static s8_t adc_sensor_offset, adc_other_offset;

static void adc_acts_enable(struct device *dev)
{
	const struct acts_adc_config *cfg = dev->config->config_info;
	struct acts_adc_data *adc = dev->driver_data;

	if (!adc->enable_cnt) {
		acts_clock_peripheral_enable(cfg->clock_id);
	}
	
	adc->enable_cnt++;
}

static void adc_acts_disable(struct device *dev)
{
	const struct acts_adc_config *cfg = dev->config->config_info;
	struct acts_adc_data *adc = dev->driver_data;

	adc->enable_cnt--;

	if (!adc->enable_cnt) {
		acts_clock_peripheral_disable(cfg->clock_id);
	}
}

__ramfunc static u16_t adc_acts_compensation(u16_t adcval, u8_t chan_id)
{
    s16_t s_adcval = adcval;
    u16_t adcval_com;

    if (chan_id == ADC_ID_SENSOR) {
        s_adcval = s_adcval + adc_sensor_offset;
        if (s_adcval < 0)
            adcval_com = 0;
        else
            adcval_com = (u16_t)s_adcval;
    } else {
        /* Y' = Y * (1 + 2*X / 896) */
        /* rounding by add 5 */
        adcval_com = (u16_t)(((s_adcval * (10000 + adc_other_offset * 2 * 10000 / 896)) / 1000 + 5) / 10);
    }

    return adcval_com;
}

__ramfunc int adc_acts_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	const struct acts_adc_config *cfg = dev->config->config_info;
	struct acts_adc_controller *adc = (struct acts_adc_controller *)cfg->base;
	struct adc_seq_entry *entry;
	u32_t key;
	int i, chan_id;
	u16_t adc_val;

	for (i = 0; i < seq_tbl->num_entries; i++) {
		entry = &seq_tbl->entries[i];
		if (!entry->buffer)
			break;

		chan_id = entry->channel_id;
		if (chan_id > ADC_ID_MAX_ID)
			break;

		if (!(adc->ctrl & BIT(chan_id))) {
			/* enable adc channel if channel is not enabled */
			key = irq_lock();
			adc->ctrl |= BIT(chan_id);
			irq_unlock(key);

			/* wait transfer finished */
			k_busy_wait(ADC_TRANS_DELAY_US);
		}

#if defined(CONFIG_SOC_SERIES_WOODPECKERFPGA) //fpga analog unavailable
		adc_val = 0x371; /* 10bit adc max value*/
#else
		adc_val = sys_read32(ADC_CHAN_DATA_REG(adc, chan_id));
        adc_val = adc_acts_compensation(adc_val, chan_id);
#endif
		sys_put_le16(adc_val, entry->buffer);

#ifndef ADC_TRANS_NO_DELAY
		/* disable adc channel */
		key = irq_lock();
		adc->ctrl &= ~BIT(chan_id);
		irq_unlock(key);
#endif
	}

	return 0;
}

const struct adc_driver_api adc_acts_driver_api = {
	.enable = adc_acts_enable,
	.disable = adc_acts_disable,
	.read = adc_acts_read,
};

int adc_acts_init(struct device *dev)
{
    int offset;

    if (soc_atp_get_pmu_calib(ADC_SENSOR_OFFSET, &offset) < 0)
        adc_sensor_offset = 0;
    else
        adc_sensor_offset = (s8_t)offset;

    if (soc_atp_get_pmu_calib(ADC_OTHER_OFFSET, &offset) < 0)
        adc_other_offset = 0;
    else
        /* sign reversed */
        adc_other_offset = 0 - (s8_t)offset;

    return 0;
}

struct acts_adc_data adc_acts_ddata;

static const struct acts_adc_config adc_acts_cdata = {
	.base = ADC_REG_BASE,
	.clock_id = CLOCK_ID_LRADC,
};

DEVICE_AND_API_INIT(adc_acts, CONFIG_ADC_0_NAME, adc_acts_init,
		    &adc_acts_ddata, &adc_acts_cdata,
		    PRE_KERNEL_2, CONFIG_ADC_INIT_PRIORITY,
		    &adc_acts_driver_api);
