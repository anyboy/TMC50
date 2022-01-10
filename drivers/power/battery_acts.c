/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery driver
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <adc.h>
#include <misc/byteorder.h>
#include <power_supply.h>
#include <gpio.h>
#include <board.h>
#include "soc.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <logging/sys_log.h>

#define MULTI_USED_UVLO              9

#define V_REF                        (1500)
#define V_LSB_MULT1000               (48*100 * 1000 / 1024)
#define LRADC_LSB_MULT1000               (36*100 * 1000 / 1024)

#define BATTERY_DRV_POLL_INTERVAL             100
#define CHG_STATUS_DETECT_DEBOUNCE_MS         300
#define BAT_MIN_VOLTAGE                       2400
#define BAT_VOL_LSB_MV                        3
#define BAT_VOLTAGE_RESERVE                   (0x1fff)
#define BAT_CAP_RESERVE                       (101)
struct battery_cap {
	u16_t cap;
	u16_t volt;
};
/**
 * BAT_VOLTAGE_TBL_DCNT * BAT_VOLTAGE_TBL_ACNT : the samples number
 * BAT_VOLTAGE_TBL_DCNT + BAT_VOLTAGE_TBL_ACNT : Save space occupied by sample points
 */
#define  BAT_VOLTAGE_TBL_DCNT                 8
#define  BAT_VOLTAGE_TBL_ACNT                 4
#define  BAT_VOLTAGE_FILTER_DROP_DCNT         1
#define  BAT_VOLTAGE_FILTER_DROP_ACNT         1
#define  INDEX_VOL                           (0)
#define  INDEX_CAP                           (INDEX_VOL+1)

#define  CAPACITY_CHECK_INTERVAL_1MINUTE     (600)

struct report_deb_ctr {
	u16_t rise;
	u16_t decline;
	u16_t times;
	u8_t step;
};

struct voltage_tbl {
	u16_t vol_data[BAT_VOLTAGE_TBL_DCNT];
	u16_t d_index;
	u16_t _vol_average[BAT_VOLTAGE_TBL_ACNT];
	u8_t a_index;
	u8_t data_collecting;   /* indicator whether the data is on collecting */
};
struct acts_battery_info {
	struct k_timer timer;

	struct device *gpio_dev;
	struct device *adc_dev;

	struct adc_seq_table seq_tbl;
	struct adc_seq_entry seq_entrys;
	u8_t adcval[2];
	struct voltage_tbl vol_tbl;
	u32_t cur_voltage       : 13;
	u32_t cur_cap           : 7;
	u32_t cur_dc5v_status   : 1;
	u32_t cur_chg_status    : 3;
	u32_t last_cap_report       : 7;
	u32_t last_voltage_report : 13;
	u32_t last_dc5v_report      : 1;
	u32_t last_chg_report       : 3;

	struct report_deb_ctr report_ctr[2];

	const struct battery_cap *cap_tbl;
	bat_charge_callback_t notify;

	u8_t chg_debounce_buf[CHG_STATUS_DETECT_DEBOUNCE_MS/BATTERY_DRV_POLL_INTERVAL];

};

struct acts_battery_config {
	char *adc_name;
	u16_t adc_chan;
	u16_t pin;
	char *gpio_name;

	u16_t poll_interval_ms;
};


#define BATTERY_CAP_TABLE_CNT		12
static const struct battery_cap battery_cap_tbl[BATTERY_CAP_TABLE_CNT] = {
	BOARD_BATTERY_CAP_MAPPINGS
};
static void battery_volt_selectsort(u16_t *arr, u16_t len);
bool bat_charge_callback(bat_charge_event_t event, bat_charge_event_para_t *para, struct acts_battery_info *bat);

#ifdef CONFIG_BATTERY_CAP_CTRL
struct battery_capctr_info g_battery_capctr_info;
u16_t vol_for_capctr[BAT_VOLTAGE_TBL_DCNT];

static void battery_start_charging(struct acts_battery_info *bat)
{
    /* do something to enable charging*/
}

static void battery_stop_charging(struct acts_battery_info *bat)
{
    /* do something to disable charging*/
}
#endif

#if (CONFIG_BATTERY_ACTS_ADC_CHAN == 0)
/**
 *spec 8.4.4.1 BATADC. 1LSB = 4.69mv
 */
static unsigned int battery_adcval_to_voltage(unsigned int adcval)
{
	return V_LSB_MULT1000 * adcval / 1000;
}
#else

/**
 *spec 8.4.4.5 LRADC. 1LSB = 4.69mv
 */
static unsigned int battery_lradcval_to_voltage(unsigned int adcval)
{
	return LRADC_LSB_MULT1000 * adcval / 1000;
}
#endif

/* voltage: mV */
static int __battery_get_voltage(struct acts_battery_info *bat)
{
	unsigned int adc_val, volt_mv;

	adc_read(bat->adc_dev, &bat->seq_tbl);
	adc_val = sys_get_le16(bat->seq_entrys.buffer);

#if CONFIG_BATTERY_ACTS_ADC_CHAN == 0
	volt_mv = battery_adcval_to_voltage(adc_val);
#else
	adc_val = adc_val * 5 / 3;

	volt_mv = battery_lradcval_to_voltage(adc_val);
#endif
	SYS_LOG_DBG("adc: %d volt_mv %d\n", adc_val, volt_mv);

	return volt_mv;
}

static int _battery_get_voltage(struct acts_battery_info *bat)
{
	unsigned int i, sum, num;
	unsigned int volt_mv;

	/* order the hostory data */
	battery_volt_selectsort(bat->vol_tbl.vol_data, BAT_VOLTAGE_TBL_DCNT);

	sum = 0;
	/* drop two min and max data */
	for (i = BAT_VOLTAGE_FILTER_DROP_DCNT;
		i < BAT_VOLTAGE_TBL_DCNT - BAT_VOLTAGE_FILTER_DROP_DCNT; i++) {
		sum += bat->vol_tbl.vol_data[i];
	}
	num = BAT_VOLTAGE_TBL_DCNT - BAT_VOLTAGE_FILTER_DROP_DCNT*2;

	/* Rounded to one decimal place */
	volt_mv = ((sum*10 / num) + 5) / 10;

	/* SYS_LOG_INF("_Avoltage: %d mV",  volt_mv); */

	return volt_mv;
}

/* Get a voltage sample point */
static unsigned int sample_one_voltage(struct acts_battery_info *bat)
{
	unsigned int volt_mv = __battery_get_voltage(bat);

	bat->vol_tbl.vol_data[bat->vol_tbl.d_index] = volt_mv;
	bat->vol_tbl.d_index++;
	bat->vol_tbl.d_index %= BAT_VOLTAGE_TBL_DCNT;

	if (bat->vol_tbl.d_index == 0) {
		bat->vol_tbl._vol_average[bat->vol_tbl.a_index] = _battery_get_voltage(bat);
		bat->vol_tbl.a_index++;
		bat->vol_tbl.a_index %= BAT_VOLTAGE_TBL_ACNT;

		/* data collection finish */
		if ((bat->vol_tbl.data_collecting == 1)
			&& (bat->vol_tbl.a_index == 0))	{
			bat->vol_tbl.data_collecting = 0;
		}
	}

	return volt_mv;
}

static int battery_get_voltage(struct acts_battery_info *bat)
{
	unsigned int i, sum, num;
	u16_t temp_vol_average[BAT_VOLTAGE_TBL_ACNT];

	/* sample one voltage before getting the average voltage to guarantee the divide 0 case when calling battery_get_voltage() */
	sample_one_voltage(bat);

	num = 0;
	sum = 0;
	if (!bat->vol_tbl.data_collecting) {
		for (i = 0; i < BAT_VOLTAGE_TBL_ACNT; i++) {
			temp_vol_average[i] =  bat->vol_tbl._vol_average[i];
		}
		battery_volt_selectsort(temp_vol_average, BAT_VOLTAGE_TBL_ACNT);

		for (i = BAT_VOLTAGE_FILTER_DROP_ACNT;
			i < BAT_VOLTAGE_TBL_ACNT - BAT_VOLTAGE_FILTER_DROP_ACNT; i++) {
			sum += temp_vol_average[i];
		}
		num = BAT_VOLTAGE_TBL_ACNT - BAT_VOLTAGE_FILTER_DROP_ACNT*2;
	} else {
		for (i = 0; i < bat->vol_tbl.d_index ; i++) {
			sum +=  bat->vol_tbl.vol_data[i];
			num++;
		}

		for (i = 0; i < bat->vol_tbl.a_index; i++) {
			sum +=  bat->vol_tbl._vol_average[i];
			num++;
		}
	}

	return ((sum*10 / num + 5) / 10);
}

u32_t voltage2capacit(u32_t volt, struct acts_battery_info *bat)
{
	const struct battery_cap *bc, *bc_prev;
	u32_t  i, cap = 0;

	/* %0 */
	if (volt <= bat->cap_tbl[0].volt)
		return 0;

	/* %100 */
	if (volt >= bat->cap_tbl[BATTERY_CAP_TABLE_CNT - 1].volt)
		return 100;

	for (i = 1; i < BATTERY_CAP_TABLE_CNT; i++) {
		bc = &bat->cap_tbl[i];
		if (volt < bc->volt) {
			bc_prev = &bat->cap_tbl[i - 1];
			/* linear fitting */
			cap = bc_prev->cap + (((bc->cap - bc_prev->cap) *
				(volt - bc_prev->volt)*10 + 5) / (bc->volt - bc_prev->volt)) / 10;

			break;
		}
	}

	return cap;
}

static void bat_voltage_record_reset(struct acts_battery_info *bat)
{
	int d, a;

	for (a = 0; a < BAT_VOLTAGE_TBL_ACNT; a++) {
		for (d = 0; d < BAT_VOLTAGE_TBL_DCNT; d++) {
			bat->vol_tbl.vol_data[d] = 0;
		}
		bat->vol_tbl._vol_average[a] = 0;
	}
	bat->cur_voltage = 0;
	bat->vol_tbl.d_index = 0;
	bat->vol_tbl.a_index = 0;
	bat->vol_tbl.data_collecting = 1;
	bat->cur_cap = 0;

	SYS_LOG_INF("");
}

static int battery_get_capacity(struct acts_battery_info *bat)
{
	int volt;

	volt = battery_get_voltage(bat);

	return voltage2capacit(volt, bat);
}

static int battery_get_charge_status(struct acts_battery_info *bat, const void *cfg)
{
#ifdef BOARD_BATTERY_CHARGING_STATUS_GPIO
	struct device *gpio_dev = bat->gpio_dev;
	const struct acts_battery_config *bat_cfg = cfg;
	u32_t pin_status = 0;
#endif
	u32_t charge_status = 0;

	if (sys_pm_get_power_5v_status()) {
	#ifdef BOARD_BATTERY_CHARGING_STATUS_GPIO
		gpio_pin_read(gpio_dev, bat_cfg->pin, &pin_status);
		if (pin_status == 1) {
			charge_status = POWER_SUPPLY_STATUS_CHARGING;
		} else {
			charge_status = POWER_SUPPLY_STATUS_FULL;
		}
	#endif

		/* check the battery is esisted or not */
		if (__battery_get_voltage(bat) <= BAT_MIN_VOLTAGE)
			charge_status = POWER_SUPPLY_STATUS_BAT_NOTEXIST;
	} else {
		charge_status = POWER_SUPPLY_STATUS_DISCHARGE;
	}


	return charge_status;
}


static void battery_volt_selectsort(u16_t *arr, u16_t len)
{
	u16_t i, j, min;

	for (i = 0; i < len-1; i++)	{
		min = i;
		for (j = i+1; j < len; j++) {
			if (arr[min] > arr[j])
				min = j;
		}
		/* swap */
		if (min != i) {
			arr[min] ^= arr[i];
			arr[i] ^= arr[min];
			arr[min] ^= arr[i];
		}
	}
}


static int bat_report_debounce(u32_t last_val, u32_t cur_val, struct report_deb_ctr *ctr)
{
	if ((cur_val  > last_val) && (cur_val - last_val >= ctr->step)) {
		ctr->rise++;
		ctr->decline = 0;
		/* SYS_LOG_INF("RISE(%s) %d\n", cur_val>100 ? "VOL" : "CAP", ctr->rise); */
	} else if ((cur_val  < last_val) && (last_val - cur_val >= ctr->step)) {
		ctr->decline++;
		ctr->rise = 0;
		/* SYS_LOG_INF("DECLINE(%s) %d\n", cur_val>100 ? "VOL" : "CAP", ctr->decline); */
	}

	if (ctr->decline >= ctr->times) {
		/* SYS_LOG_INF("<DECLINE> %d>%d\n", last_val, cur_val); */
		ctr->decline = 0;
		return 1;
	} else if (ctr->rise >= ctr->times) {
		/* SYS_LOG_INF("<RISE> %d>%d\n", last_val, cur_val); */
		ctr->rise = 0;
		return 1;
	} else {
		return 0;
	}
}

#ifdef CONFIG_BATTERY_CAP_CTRL
static u16_t battery_fast_get_voltage(struct acts_battery_info * bat, u16_t *vol_table, u8_t len)
{
    u8_t i, ret_cap;
    u16_t ret_vol;
    u32_t sum_vol = 0;

    battery_volt_selectsort(vol_table, len);
    for (i = 1; i < len -1; i++) {
        sum_vol += vol_table[i];
    }

    ret_vol = (sum_vol*10 / (len-2) + 5) / 10;
    ret_cap = voltage2capacit(ret_vol, bat);
    return ret_cap;
}

static void battery_capacity_control(struct acts_battery_info *bat, struct device *dev)
{
    const struct acts_battery_config *cfg = dev->config->config_info;
    u8_t temp_index, cur_cap = 0, cur_charge_status;
    static u16_t poll_cnt = 0, charging_flag = 0;

    cur_charge_status = battery_get_charge_status(bat, cfg);

    if (cur_charge_status == POWER_SUPPLY_STATUS_CHARGING) {
        if (charging_flag == 0)
            charging_flag = 1;

        poll_cnt++;
        if (poll_cnt >= CAPACITY_CHECK_INTERVAL_1MINUTE) {
            if (charging_flag == 1) {
                if (poll_cnt == CAPACITY_CHECK_INTERVAL_1MINUTE)
                    battery_stop_charging(bat);

                //delay(another n*BATTERY_DRV_POLL_INTERVAL)
                if (poll_cnt >= CAPACITY_CHECK_INTERVAL_1MINUTE + 1) {
                    temp_index = poll_cnt - (CAPACITY_CHECK_INTERVAL_1MINUTE + 1);
                    vol_for_capctr[temp_index] = __battery_get_voltage(bat);
                }

                if (poll_cnt == CAPACITY_CHECK_INTERVAL_1MINUTE + BAT_VOLTAGE_TBL_DCNT) {
                    poll_cnt = 0;
                    charging_flag = 2;

                    cur_cap = battery_fast_get_voltage(bat, vol_for_capctr, ARRAY_SIZE(vol_for_capctr));
                    SYS_LOG_INF("cur_cap: %d\n", cur_cap);
                    if (cur_cap < g_battery_capctr_info.capctr_maxval) {
                        battery_start_charging(bat);
                        charging_flag = 1;
                    }
                }
            } else {
                temp_index = poll_cnt - CAPACITY_CHECK_INTERVAL_1MINUTE;
                vol_for_capctr[temp_index] = __battery_get_voltage(bat);
                if (poll_cnt == CAPACITY_CHECK_INTERVAL_1MINUTE + BAT_VOLTAGE_TBL_DCNT - 1) {
                    poll_cnt = 0;

                    cur_cap = battery_fast_get_voltage(bat, vol_for_capctr, ARRAY_SIZE(vol_for_capctr));
                    SYS_LOG_INF("cur_cap: %d\n", cur_cap);
                    if (cur_cap <= g_battery_capctr_info.capctr_minval) {
                        battery_start_charging(bat);
                        charging_flag = 1;
                    }
                }
            }
        }
    } else {
        poll_cnt = 0;
        charging_flag = 0;
    }
}
#endif

void battery_acts_voltage_poll(struct acts_battery_info *bat)
{
	unsigned int volt_mv;
	bat_charge_event_para_t para;
	u32_t cap = 0;

	volt_mv = battery_get_voltage(bat);

	/* wait finish data collection */
	if (bat->vol_tbl.data_collecting)
		return;

	/* SYS_LOG_INF("cur_voltage %d volt_mv %d cur_chg_status %d\n", bat->cur_voltage, volt_mv, bat->cur_chg_status); */
	if ((bat->cur_voltage >= volt_mv + BAT_VOL_LSB_MV)
		|| (bat->cur_voltage + BAT_VOL_LSB_MV <= volt_mv)) {
		bat->cur_voltage = volt_mv;
		cap = voltage2capacit(volt_mv, bat);
	}
	if ((cap != 0) && (cap != bat->cur_cap)) {
		bat->cur_cap = cap;
	}

	if ((bat->cur_chg_status != POWER_SUPPLY_STATUS_BAT_NOTEXIST)
		&& (bat->cur_chg_status != POWER_SUPPLY_STATUS_UNKNOWN)) {
		bat->report_ctr[INDEX_CAP].times = 600;   /* unit 100ms */
		bat->report_ctr[INDEX_CAP].step = 5;      /* % */

		if (bat->cur_voltage <= 3400) {
			bat->report_ctr[INDEX_VOL].times = 50;  /* unit 100ms */
			bat->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*3;    /* mv */
		} else if (bat->cur_voltage <= 3700) {
			bat->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*5;    /* mv */
			bat->report_ctr[INDEX_VOL].times = 150;  /* unit 100ms */
		} else {
			bat->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*6;    /* mv */
			bat->report_ctr[INDEX_VOL].times = 350; /* unit 100ms */
		}

		if (bat_report_debounce(bat->last_voltage_report, bat->cur_voltage, &bat->report_ctr[INDEX_VOL])
			|| (bat->last_voltage_report == BAT_VOLTAGE_RESERVE)) {

			para.voltage_val = bat->cur_voltage*1000;
			if (bat_charge_callback(BAT_CHG_EVENT_VOLTAGE_CHANGE, &para, bat))
				bat->last_voltage_report = bat->cur_voltage;
		}

		if (bat_report_debounce(bat->last_cap_report, bat->cur_cap, &bat->report_ctr[INDEX_CAP])
			|| (bat->last_cap_report == BAT_CAP_RESERVE)) {
			para.cap = bat->cur_cap;
			if (bat_charge_callback(BAT_CHG_EVENT_CAP_CHANGE, &para, bat))
				bat->last_cap_report = bat->cur_cap;
		}
	}

}

void dc5v_status_check(struct acts_battery_info *bat)
{
	u8_t temp_dc5v_status;

	if (!sys_pm_get_power_5v_status()) {
		temp_dc5v_status = 0;
	} else {
		temp_dc5v_status = 1;
	}


	if (bat->cur_dc5v_status != temp_dc5v_status) {
		bat->cur_dc5v_status = temp_dc5v_status;
	}

	if (bat->last_dc5v_report != temp_dc5v_status) {
		if (bat_charge_callback((temp_dc5v_status ? BAT_CHG_EVENT_DC5V_IN : BAT_CHG_EVENT_DC5V_OUT), NULL, bat))
			bat->last_dc5v_report = temp_dc5v_status;
	}
}


u8_t debounce(u8_t *debounce_buf, int buf_size, u8_t value)
{
	int  i;

	if (buf_size == 0)
		return 1;

	for (i = 0; i < buf_size - 1; i++) {
		debounce_buf[i] = debounce_buf[i+1];
	}

	debounce_buf[buf_size - 1] = value;

    for (i = 0; i < buf_size; i++) {
		if (debounce_buf[i] != value) {
		    return 0;
		}
	}

	return 1;
}

int charge_status_need_debounce(u32_t charge_status)
{
	if ((charge_status == POWER_SUPPLY_STATUS_CHARGING)
		|| (charge_status == POWER_SUPPLY_STATUS_FULL)) {
		return 1;
	} else {
		return 0;
	}
}

void battery_acts_charge_status_check(struct acts_battery_info *bat, struct device *dev)
{
	const struct acts_battery_config *bat_cfg = dev->config->config_info;
	u32_t charge_status = 0;

	charge_status = battery_get_charge_status(bat, bat_cfg);

	if ((charge_status_need_debounce(charge_status))
		&& (debounce(bat->chg_debounce_buf, sizeof(bat->chg_debounce_buf), charge_status) == FALSE)) {
		return;
	}

	if (charge_status != bat->cur_chg_status) {
		/* if battery has been plug in or pull out, need to reset the voltage record */
		if ((bat->cur_chg_status == POWER_SUPPLY_STATUS_BAT_NOTEXIST) || (charge_status == POWER_SUPPLY_STATUS_BAT_NOTEXIST)) {
			bat_voltage_record_reset(bat);
		}

		/* SYS_LOG_INF("cur_chg_status %d charge_status%d\n", bat->cur_chg_status, charge_status); */
		bat->cur_chg_status = charge_status;
	}

	if (charge_status != bat->last_chg_report) {
		if (charge_status == POWER_SUPPLY_STATUS_FULL) {
			if (bat_charge_callback(BAT_CHG_EVENT_CHARGE_FULL, NULL, bat))
				bat->last_chg_report = charge_status;
		} else if (charge_status == POWER_SUPPLY_STATUS_CHARGING) {
			if (bat_charge_callback(BAT_CHG_EVENT_CHARGE_START, NULL, bat))
				bat->last_chg_report = charge_status;
		}
	}
}


static void battery_acts_poll(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	struct acts_battery_info *bat = dev->driver_data;

	battery_acts_charge_status_check(bat, dev);

	dc5v_status_check(bat);

	battery_acts_voltage_poll(bat);

#ifdef CONFIG_BATTERY_CAP_CTRL
	if (g_battery_capctr_info.capctr_enable_flag)
		battery_capacity_control(bat, dev);
#endif
}

static int battery_acts_get_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct acts_battery_info *bat = dev->driver_data;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		/* val->intval = battery_get_charge_status(bat, dev->config->config_info); */
		val->intval = bat->cur_chg_status;
		return 0;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		return 0;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = battery_get_voltage(bat) * 1000;
		return 0;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = battery_get_capacity(bat);
		return 0;
	case POWER_SUPPLY_PROP_DC5V:
		val->intval = !!sys_pm_get_power_5v_status();
		return 0;
	default:
		return -EINVAL;
	}
}

static void battery_acts_set_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	/* do nothing */
}

bool bat_charge_callback(bat_charge_event_t event, bat_charge_event_para_t *para, struct acts_battery_info *bat)
{
	/* SYS_LOG_INF("event %d notify %p\n", event, bat->notify); */
	if (bat->notify) {
		bat->notify(event, para);
	} else {
		return FALSE;
	}

	return	TRUE;
}

static void battery_acts_register_notify(struct device *dev, bat_charge_callback_t cb)
{
	struct acts_battery_info *bat = dev->driver_data;
	int flag;

	flag = irq_lock();
	SYS_LOG_INF("%p\n", cb);
	if ((bat->notify == NULL) && cb) {
		bat->notify = cb;
	} else {
		SYS_LOG_ERR("notify func already exist!\n");
	}
	irq_unlock(flag);
}

static void battery_acts_enable(struct device *dev)
{
	struct acts_battery_info *bat = dev->driver_data;
	const struct acts_battery_config *cfg = dev->config->config_info;

	SYS_LOG_DBG("enable battery detect");

	adc_enable(bat->adc_dev);
	k_timer_start(&bat->timer, cfg->poll_interval_ms,
		cfg->poll_interval_ms);
}

static void battery_acts_disable(struct device *dev)
{
	struct acts_battery_info *bat = dev->driver_data;

	SYS_LOG_DBG("disable battery detect");

	k_timer_stop(&bat->timer);
	adc_disable(bat->adc_dev);
}

static const struct power_supply_driver_api battery_acts_driver_api = {
	.get_property = battery_acts_get_property,
	.set_property = battery_acts_set_property,
	.register_notify = battery_acts_register_notify,
	.enable = battery_acts_enable,
	.disable = battery_acts_disable,
};

static int battery_acts_init(struct device *dev)
{
	struct acts_battery_info *bat = dev->driver_data;
	const struct acts_battery_config *cfg = dev->config->config_info;

	SYS_LOG_INF("battery initialized\n");

	bat->adc_dev = device_get_binding(cfg->adc_name);
	if (!bat->adc_dev) {
		SYS_LOG_ERR("cannot found ADC device \'batadc\'\n");
		return -ENODEV;
	}

	bat->seq_entrys.sampling_delay = 0;
	bat->seq_entrys.buffer = (u8_t *)&bat->adcval;
	bat->seq_entrys.buffer_length = sizeof(bat->adcval);
	bat->seq_entrys.channel_id = cfg->adc_chan;
	bat->seq_tbl.entries = &bat->seq_entrys;
	bat->seq_tbl.num_entries = 1;
	bat->cap_tbl = battery_cap_tbl;
	bat_voltage_record_reset(bat);
	adc_enable(bat->adc_dev);

#ifdef CONFIG_BATTERY_CAP_CTRL
#ifdef CONFIG_PROPERTY
	if (property_get(CFG_ATT_BAT_INFO, (char *)&g_battery_capctr_info, sizeof(g_battery_capctr_info)) < 0) {
		g_battery_capctr_info.capctr_enable_flag = BATTERY_CAPCTR_DISABLE;
	}
#endif
#endif

#ifdef BOARD_BATTERY_CHARGING_STATUS_GPIO
	bat->gpio_dev = device_get_binding(cfg->gpio_name);
	if (!bat->gpio_dev) {
		SYS_LOG_ERR("cannot found GPIO device \'batadc\'\n");
		return -ENODEV;
	}
	gpio_pin_configure(bat->gpio_dev, cfg->pin, GPIO_DIR_IN);
#endif

	bat->cur_chg_status = POWER_SUPPLY_STATUS_UNKNOWN;
	bat->last_cap_report = BAT_CAP_RESERVE;
	bat->last_voltage_report = BAT_VOLTAGE_RESERVE;

	/* dev->driver_api = &battery_acts_driver_api; */

	k_timer_init(&bat->timer, battery_acts_poll, NULL);
	k_timer_user_data_set(&bat->timer, dev);
	k_timer_start(&bat->timer, 10, cfg->poll_interval_ms);

	return 0;
}

static struct acts_battery_info battery_acts_ddata;

static const struct acts_battery_config battery_acts_cdata = {
	.adc_name = CONFIG_BATTERY_ACTS_ADC_NAME,
	.adc_chan = CONFIG_BATTERY_ACTS_ADC_CHAN,
	.gpio_name = CONFIG_GPIO_ACTS_DEV_NAME,
#ifdef BOARD_BATTERY_CHARGING_STATUS_GPIO
	.pin = BOARD_BATTERY_CHARGING_STATUS_GPIO,
#endif
	.poll_interval_ms = BATTERY_DRV_POLL_INTERVAL,
};


DEVICE_AND_API_INIT(battery, "battery", battery_acts_init,
	    &battery_acts_ddata, &battery_acts_cdata, POST_KERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &battery_acts_driver_api);
