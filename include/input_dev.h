/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief input device driver interface
 */

#ifndef __INCLUDE_INPUT_DEV_H__
#define __INCLUDE_INPUT_DEV_H__

#include <stdint.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Keys and buttons
 */

#define KEY_RESERVED			0
#define KEY_POWER				1
#define KEY_PREVIOUSSONG		2
#define KEY_NEXTSONG			3
#define KEY_VOL					4
#define KEY_VOLUMEUP			5
#define KEY_VOLUMEDOWN			6
#define KEY_MENU				7
#define KEY_CONNECT				8
#define KEY_TBD					9
#define KEY_MUTE				10
#define KEY_PAUSE_AND_RESUME 	11
#define KEY_FOLDER_ADD			12
#define KEY_FOLDER_SUB			13
#define KEY_NEXT_VOLADD			14
#define KEY_PREV_VOLSUB			15
#define KEY_NUM0 				16
#define KEY_NUM1				17
#define KEY_NUM2				18
#define KEY_NUM3				19
#define KEY_NUM4				20
#define KEY_NUM5				21
#define KEY_NUM6				22
#define KEY_NUM7				23
#define KEY_NUM8				24
#define KEY_NUM9				25
#define KEY_CH_ADD				26
#define KEY_CH_SUB				27
#define KEY_PAUSE				28
#define KEY_RESUME				29
#define KEY_POWER_IR			30

#define KEY_F1					40
#define KEY_F2					41
#define KEY_F3					42
#define KEY_F4					43
#define KEY_F5					44
#define KEY_F6					45
#define KEY_ADFU				46


/* We avoid low common keys in module aliases so they don't get huge. */
#define KEY_MIN_INTERESTING	KEY_MUTE

/**/
#define LINEIN_DETECT		100

#define KEY_MAX			0x2ff
#define KEY_CNT			(KEY_MAX+1)

/*
 * Event types
 */

#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03
#define EV_MSC			0x04
#define EV_SW			0x05
#define EV_SR			0x06  /* Sliding rheostat */
#define EV_LED			0x11
#define EV_SND			0x12
#define EV_REP			0x14
#define EV_FF			0x15
#define EV_PWR			0x16
#define EV_FF_STATUS		0x17
#define EV_MAX			0x1f
#define EV_CNT			(EV_MAX+1)

/*
 * Input device types
 */
#define INPUT_DEV_TYPE_KEYBOARD		1
#define INPUT_DEV_TYPE_TOUCHPAD		2

struct input_dev_config {
	uint8_t type;
};


struct input_value {
	uint16_t type;
	uint16_t code;
	uint32_t value;
};

typedef void (*input_notify_t) (struct device *dev, struct input_value *val);

/**
 * @brief Input device driver API
 *
 * This structure holds all API function pointers.
 */
struct input_dev_driver_api {
	void (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	void (*register_notify)(struct device *dev, input_notify_t notify);
	void (*unregister_notify)(struct device *dev, input_notify_t notify);
};

static inline void input_dev_enable(struct device *dev)
{
	const struct input_dev_driver_api *api = dev->driver_api;

	api->enable(dev);
}

static inline void input_dev_disable(struct device *dev)
{
	const struct input_dev_driver_api *api = dev->driver_api;

	api->disable(dev);
}

static inline void input_dev_register_notify(struct device *dev,
					     input_notify_t notify)
{
	const struct input_dev_driver_api *api = dev->driver_api;

	api->register_notify(dev, notify);
}

static inline void input_dev_unregister_notify(struct device *dev,
					       input_notify_t notify)
{
	const struct input_dev_driver_api *api = dev->driver_api;

	api->unregister_notify(dev, notify);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_INPUT_DEV_H__ */
