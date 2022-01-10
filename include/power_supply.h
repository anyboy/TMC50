/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief power supply device driver interface
 */

#ifndef __INCLUDE_POWER_SUPPLY_H__
#define __INCLUDE_POWER_SUPPLY_H__

#include <stdint.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif


#define BATTERY_CAPCTR_STA_PASSED            (1)
#define BATTERY_CAPCTR_STA_FAILED            (0)
#define BATTERY_CAPCTR_DISABLE               (0)
#define BATTERY_CAPCTR_ENABLE                (1)

enum  power_supply_status{
	POWER_SUPPLY_STATUS_UNKNOWN = 0,
	POWER_SUPPLY_STATUS_BAT_NOTEXIST,
	POWER_SUPPLY_STATUS_DISCHARGE,
	POWER_SUPPLY_STATUS_CHARGING,
	POWER_SUPPLY_STATUS_FULL,
};

enum power_supply_property {
	/* Properties of type `int' */
	POWER_SUPPLY_PROP_STATUS = 0,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
    /* in percents */
	POWER_SUPPLY_PROP_DC5V,
};

typedef enum
{
    BAT_CHG_EVENT_DC5V_IN = 1,
    BAT_CHG_EVENT_DC5V_OUT,
    
    BAT_CHG_EVENT_CHARGE_START,
    BAT_CHG_EVENT_CHARGE_FULL,

    BAT_CHG_EVENT_VOLTAGE_CHANGE,
    BAT_CHG_EVENT_CAP_CHANGE,
} bat_charge_event_t;
	
typedef union {
		uint32_t voltage_val;
		uint32_t cap;
} bat_charge_event_para_t;


typedef void (*bat_charge_callback_t)(bat_charge_event_t event, bat_charge_event_para_t *para);


union power_supply_propval {
	int intval;
	const char *strval;
};

struct battery_capctr_info {
    u8_t capctr_enable_flag;     /* 0-disable, 1-enable */
    u8_t capctr_minval;
    u8_t capctr_maxval;
};

struct power_supply_driver_api {
	int (*get_property)(struct device *dev, enum power_supply_property psp,
			     union power_supply_propval *val);
	void (*set_property)(struct device *dev, enum power_supply_property psp,
			     union power_supply_propval *val);
	void (*register_notify)(struct device *dev, bat_charge_callback_t cb);
	void (*enable)(struct device *dev);
	void (*disable)(struct device *dev);	
};

static inline int power_supply_get_property(struct device *dev, enum power_supply_property psp,
			     union power_supply_propval *val)
{
	const struct power_supply_driver_api *api = dev->driver_api;

	return api->get_property(dev, psp, val);
}

static inline void power_supply_set_property(struct device *dev, enum power_supply_property psp,
			     union power_supply_propval *val)
{
	const struct power_supply_driver_api *api = dev->driver_api;

	api->set_property(dev, psp, val);
}

static inline void power_supply_register_notify(struct device *dev, bat_charge_callback_t cb)
{
	const struct power_supply_driver_api *api = dev->driver_api;

	api->register_notify(dev, cb);
}

static inline void power_supply_enable(struct device *dev)
{
	const struct power_supply_driver_api *api = dev->driver_api;

	api->enable(dev);
}

static inline void power_supply_disable(struct device *dev)
{
	const struct power_supply_driver_api *api = dev->driver_api;

	api->disable(dev);
}


#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_POWER_SUPPLY_H__ */
