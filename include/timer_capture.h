/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for Timer(With Input Capture Mode) Drivers
 */

#ifndef _TIMER_CAPTURE_H_
#define _TIMER_CAPTURE_H_
#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	CAPTURE_SIGNAL_FALLING,
	CAPTURE_SIGNAL_RISING,
	CAPTURE_SIGNAL_BOTH_EDGES,
} capture_signal_e;

typedef void (*capture_notify_t) (struct device *dev, int capture_val, int capture_level);

/**
 * @brief Timer capture driver API
 *
 * This structure holds all API function pointers.
 */
struct capture_dev_driver_api {
	void (*start)(struct device *dev, capture_signal_e sig, capture_notify_t notify);
	void (*stop)(struct device *dev);
};

/**
 * @brief Start timer capture
 *
 * @param dev: Pointer to the device structure for the driver instance.
 * @param sig: Timer capture trigger signal.
 * @param notify: notify when timer capture trigger.
 *
 *  @return  N/A
 */
static inline void timer_capture_start(struct device *dev, capture_signal_e sig, capture_notify_t notify)
{
	const struct capture_dev_driver_api *api = dev->driver_api;

	api->start(dev, sig, notify);
}

/**
 * @brief Stop timer capture
 *
 * @param dev: Pointer to the device structure for the driver instance.
 *
 *  @return  N/A
 */
static inline void timer_capture_stop(struct device *dev)
{
	const struct capture_dev_driver_api *api = dev->driver_api;

	api->stop(dev);
}

#ifdef __cplusplus
}
#endif

#endif
