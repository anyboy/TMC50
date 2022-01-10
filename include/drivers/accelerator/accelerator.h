/*
 * Copyright (c) 2020 Actions Semi Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for Accelerator drivers
 */

#ifndef __ACCELERATOR_H__
#define __ACCELERATOR_H__

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Accelerator Driver Interface
 * @defgroup Accelerator_interface Accelerator Driver Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Accelerator mode.
 */
enum accelerator_mode {
	ACCELERATOR_MODE_FFT,
	ACCELERATOR_MODE_FIR,
	ACCELERATOR_MODE_IIR,
};

struct accelerator_fft_param {
	/* 0: fft, 1: ifft */
	u8_t mode;
	/* fft samples per frame */
	u8_t frame_type;
	u8_t sample_bits;
	/* input pcm buffer (buf address must be 4 bytes aligned) */
	void *input_buf;
	int input_samples;
	/* output pcm buffer (buf address must be 4 bytes aligned) */
	void *output_buf;
	int output_samples;
};

struct accelerator_fir_param {
	/* resample mode */
	int mode;
	/* history buffer (buf address must be 4 bytes aligned) */
	void *hist_buf;
	int hist_len;
	/* input pcm buffer (buf address must be 4 bytes aligned) */
	void *input_buf;
	int input_len;
	/* output pcm buffer (buf address must be 4 bytes aligned) */
	void *output_buf;
	int output_len;
};

struct accelerator_iir_param {
	u8_t level;
	u8_t channel;
	/* history buffer (buf address must be 4 bytes aligned) */
	void *hist_inbuf;
	void *hist_outbuf;
	/* input/output pcm buffer (buf address must be 4 bytes aligned) */
	void *inout_buf;
	int inout_len;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

/**
 * @typedef accelerator_api_open
 * @brief Prototype definition for accelerator_driver_api "open"
 */
typedef int (*accelerator_api_open)(struct device *dev);

/**
 * @typedef accelerator_api_close
 * @brief Prototype definition for accelerator_driver_api "close"
 */
typedef int (*accelerator_api_close)(struct device *dev);

/**
 * @typedef accelerator_api_process
 * @brief Prototype definition for accelerator_driver_api "process"
 */
typedef int (*accelerator_api_process)(struct device *dev, int mode, void *args);

struct accelerator_driver_api {
	accelerator_api_open open;
	accelerator_api_close close;
	accelerator_api_process process;
};

/**
 * @endcond
 */

/**
 * @brief Open the accelerator device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int accelerator_open(struct device *dev)
{
	const struct accelerator_driver_api *api = dev->driver_api;

	return api->open(dev);
}

/**
 * @brief Close the accelerator device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int accelerator_close(struct device *dev)
{
	const struct accelerator_driver_api *api = dev->driver_api;

	return api->close(dev);
}

/**
 * @brief Close the accelerator device.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param mode Accelerator mode, see enum accelerator_mode.
 * @param args Pointer to the arguments for specific accelerator mode.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int accelerator_process(struct device *dev, int mode, void *args)
{
	const struct accelerator_driver_api *api = dev->driver_api;

	return api->process(dev, mode, args);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ACCELERATOR_H__ */
