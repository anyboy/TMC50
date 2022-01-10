
/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief hotplog detect device driver interface
 */

#ifndef __INCLUDE_HOTPLOG_H__
#define __INCLUDE_HOTPLOG_H__

#include <stdint.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

enum linein_state
{
    LINEIN_NONE,
    LINEIN_IN,
    LINEIN_OUT
};

struct hotplog_detect_driver_api {
	int (*detect_state)(struct device *dev, int *state);
};

static inline int hotplog_detect_state(struct device *dev, int *state)
{
	const struct hotplog_detect_driver_api *api = dev->driver_api;

	return api->detect_state(dev, state);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_HOTPLOG_H__ */
