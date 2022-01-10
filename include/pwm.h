/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public PWM Driver APIs
 */

#ifndef __PWM_H__
#define __PWM_H__

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_ACCESS_BY_PIN	0
#define PWM_ACCESS_ALL		1

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

/**
 * @Macro to define the max cycles within a PWM period.
 */
#define PWM_CYCLES_MAX				(0xFFFF)

/**
 * @brief Provides a type to hold PWM configuration flags.
 */
typedef u8_t pwm_flags_t;

enum {
	/**start voltage level low */
	START_VOLTAGE_LOW = 0,
	/**start voltage level high */
	START_VOLTAGE_HIGH,
};

/**
 * @struct pwm_breath_ctrl_t
 * @brief Control parameters on PWM breath mode
 */
typedef struct {
#define PWM_BREATH_RISE_TIME_DEFAULT	(500)
#define PWM_BREATH_DOWN_TIME_DEFAULT	(500)
#define PWM_BREATH_HIGH_TIME_DEFAULT	(500)
#define PWM_BREATH_LOW_TIME_DEFAULT	(2000)
	u16_t rise_time_ms;
	u16_t down_time_ms;
	u16_t high_time_ms;
	u16_t low_time_ms;
} pwm_breath_ctrl_t;

/**
 * @struct pwm_program_ctrl_t
 * @brief Control parameters on PWM control mode
 * @note It only supports one PWM channel to work on program mode at the same time in actions IC.
 */
typedef struct {
#define PWM_PROGRAM_DMA_IRQ_HF	(1 << 0)
#define PWM_PROGRAM_DMA_IRQ_TC	(1 << 1)
	u32_t period_cycles;
	u16_t *ram_buf;
	u32_t ram_buf_len;
	int (*program_callback)(void *cb_data, u8_t reason);
	void *cb_data;
	u8_t reload_en : 1;
} pwm_program_ctrl_t;

/**
 * @typedef pwm_pin_set_t
 * @brief Callback API upon setting the pin
 * See @a pwm_pin_set_cycles() for argument description
 */
typedef int (*pwm_pin_set_t)(struct device *dev, u32_t pwm,
			     u32_t period_cycles, u32_t pulse_cycles, u8_t pwm_flags_t);

/**
 * @typedef pwm_get_cycles_per_sec_t
 * @brief Callback API upon getting cycles per second
 * See @a pwm_get_cycles_per_sec() for argument description
 */
typedef int (*pwm_get_cycles_per_sec_t)(struct device *dev, u32_t pwm,
					u64_t *cycles);

/**
 * @typedef pwm_set_breath_t
 * @brief Callback API upon setting pwm breath mode.
 * See @a pwm_set_breath_mode() for argument description
 */
typedef int (*pwm_set_breath_t)(struct device *dev, u32_t pwm, pwm_breath_ctrl_t *ctrl);

/**
 * @typedef pwm_set_program_t
 * @brief Callback API upon setting pwm breath mode.
 * See @a pwm_set_program_mode() for argument description
 */
typedef int (*pwm_set_program_t)(struct device *dev, u32_t pwm, pwm_program_ctrl_t *ctrl);

/**
 * @typedef pwm_pin_stop_t
 * @brief Callback API upon stopping the specified pwm.
 * See @a pwm_pin_stop() for argument description
 */
typedef int (*pwm_pin_stop_t)(struct device *dev, u32_t pwm);

/** @brief PWM driver API definition. */
struct pwm_driver_api {
	pwm_pin_set_t pin_set;
	pwm_get_cycles_per_sec_t get_cycles_per_sec;
	pwm_set_breath_t set_breath;
	pwm_set_program_t set_program;
	pwm_pin_stop_t pin_stop;
};

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in clock cycle) set to the PWM. HW specific.
 * @param pulse Pulse width (in clock cycle) set to the PWM. HW specific.
 * @param flags Flags for pin configuration (polarity).
 *
 * @note The period shall be less than PWM_CYCLES_MAX in actsions IC.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_cycles(struct device *dev, u32_t pwm,
				     u32_t period, u32_t pulse, pwm_flags_t flags)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->pin_set(dev, pwm, period, pulse, flags);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in micro second) set to the PWM.
 * @param pulse Pulse width (in micro second) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_usec(struct device *dev, u32_t pwm,
				   u32_t period, u32_t pulse, pwm_flags_t flags)
{
	struct pwm_driver_api *api;
	u64_t period_cycles, pulse_cycles, cycles_per_sec;

	api = (struct pwm_driver_api *)dev->driver_api;

	if (api->get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / USEC_PER_SEC;
	if (period_cycles >= ((u64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / USEC_PER_SEC;
	if (pulse_cycles >= ((u64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return api->pin_set(dev, pwm, (u32_t)period_cycles,
			    (u32_t)pulse_cycles, flags);
}

/**
 * @brief Get the clock rate (cycles per second) for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Pointer to the memory to store clock rate (cycles per sec).
 *		 HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_get_cycles_per_sec(struct device *dev, u32_t pwm,
					 u64_t *cycles)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->get_cycles_per_sec(dev, pwm, cycles);
}

/**
 * @brief Set the pwm breath mode for for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param ctrl Pointer to the control parameters of pwm breath mode.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_set_breath_mode(struct device *dev, u32_t pwm, pwm_breath_ctrl_t *ctrl)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->set_breath(dev, pwm, ctrl);
}

/**
 * @brief Set the pwm program mode for for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param ctrl Pointer to control parameter for pwm program mode.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_set_program_mode(struct device *dev, u32_t pwm, pwm_program_ctrl_t *ctrl)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->set_program(dev, pwm, ctrl);
}


/**
 * @brief Stop the PWM output by the specified pwm pin
 * @note This API cooperates with pwm_set_program_mode() function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_stop(struct device *dev, u32_t pwm)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->pin_stop(dev, pwm);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __PWM_H__ */
