/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for AUDIO IN drivers
 */

#ifndef __AUDIO_IN_H__
#define __AUDIO_IN_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <board.h>
#include <audio_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AUDIO IN Interface
 * @addtogroup AUDIO_IN
 * @{
 */

/*! @brief Definition for the audio in control commands */
#define AIN_CMD_SET_ADC_GAIN                                  (1)
/*!< Set the ADC left channel gain value which in dB format.
 * You can get the dB range from #AIN_CMD_GET_ADC_LEFT_GAIN_RANGE and AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE.
 * int audio_in_control(dev, handle, #AIN_CMD_SET_ADC_GAIN, adc_gain *gain)
 * Returns 0 if successful and negative errno code if error.
 */

/*! @brief Definition for the audio in control commands */
#define AIN_CMD_GET_ADC_LEFT_GAIN_RANGE                       (2)
/*!< Get the ADC left channel gain range in dB format.
 * int audio_in_control(dev, handle, #AIN_CMD_GET_ADC_LEFT_GAIN_RANGE, adc_gain_range *range)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE                      (3)
/*!< Get the ADC right channel gain range in dB format.
 * int audio_in_control(dev, handle, #AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE, adc_gain_range *range)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SPDIF_GET_CHANNEL_STATUS                      (4)
/* Get the SPDIFRX channel status
 * int audio_in_control(dev, handle, #AIN_CMD_SPDIF_GET_CHANNEL_STATUS, audio_spdif_ch_status_t *sts)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SPDIF_IS_PCM_STREAM                           (5)
/* Check if the stream that received from spdifrx is the pcm format
 * int audio_in_control(dev, handle, #AIN_CMD_SPDIF_IS_PCM_STREAM, bool *is_pcm)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SPDIF_CHECK_DECODE_ERR                        (6)
/* Check if there is spdif decode error happened
 * int audio_in_control(dev, handle, #AIN_CMD_SPDIF_CHECK_DECODE_ERR, bool *is_err)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_I2SRX_QUERY_SAMPLE_RATE                       (7)
/* Query the i2c master device sample rate and i2srx works in slave mode.
 * int audio_in_control(dev, handle, #AIN_CMD_I2SRX_QUERY_SAMPLE_RATE, audio_sr_sel_e *is_err)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_GET_SAMPLERATE                                (8)
/*!< Get the channel audio sample rate by specified the audio channel handler.
 * int audio_out_control(dev, handle, #AIN_CMD_GET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SET_SAMPLERATE                                (9)
/*!< Set the channel audio sample rate by the giving audio channel handler.
 * int audio_out_control(dev, handle, #AIN_CMD_SET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_GET_APS                                       (10)
/*!< Get the AUDIO_PLL APS
 * int audio_out_control(dev, handle, #AIN_CMD_GET_APS, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SET_APS                                       (11)
/*!< Set the AUDIO_PLL APS for the sample rate tuning
 * int audio_out_control(dev, handle, #AIN_CMD_SET_APS, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

/*!
 * struct adc_gain_range
 * @brief The ADC min and max gain range
 */
typedef struct {
	s16_t min;
		/*!< min gain */
	s16_t max;
		/*!< max gain */
} adc_gain_range;

/*!
 * struct adc_gain
 * @brief The ADC gain setting
 */
typedef struct {
#define ADC_GAIN_INVALID (0xFFFF)
	s16_t left_gain;
		/*!< left gain setting which get the range from audio_in_control by command 'AIN_CMD_GET_ADC_LEFT_GAIN_RANGE'
		* The gain value shall be set by 10 multiple of actual value e.g left_gain=100 means 10 db; If left_gain equal #ADC_GAIN_INVALID will ignore.
		*/
	s16_t right_gain;
		/*!< right gain setting which get the range from audio_in_control by command 'AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE'
		* The gain value shall be set by 10 multiple of actual value e.g right_gain=100 means 10 db; If right_gain equal #ADC_GAIN_INVALID will ignore.
		*/
} adc_gain;

/*!
 * struct adc_setting_t
 * @brief The ADC setting parameters
 */
typedef struct {
	audio_input_dev_e device;
		/*!< ADC input device chooses */
	adc_gain gain;
		/*!< ADC gain setting */
} adc_setting_t;

/*!
 * struct i2srx_setting_t
 * @brief The I2SRX setting parameters
 */
typedef struct {
#define I2SRX_SRD_FS_CHANGE  (1 << 0)
	/*!< I2SRX SRD(sample rate detect) captures the event that the sample rate has changed
	 * int callback(cb_data, #I2STX_SRD_FS_CHANGE, audio_sr_sel_e *sr)
	 */
#define I2SRX_SRD_WL_CHANGE  (1 << 1)
	/*!< I2SRX SRD(sample rate detect) captures the event that the effective width length has changed
	 * int callback(cb_data, #I2STX_SRD_WL_CHANGE, audio_i2s_srd_wl_e *wl)
	 */
#define I2SRX_SRD_TIMEOUT    (1 << 2)
	/*!< I2SRX SRD(sample rate detect) captures the timeout (disconnection) event
	 * int callback(cb_data, #I2STX_SRD_TIMEOUT, NULL)
	 */
	audio_i2s_mode_e mode;
		/* I2S master mode or slave mode selection */
	int (*srd_callback)(void *cb_data, u32_t cmd, void *param);
		/*!< The callback function from I2SRX SRD module which worked in the slave mode */
	void *cb_data;
		/*!< Callback user data */
} i2srx_setting_t;

/*!
 * struct spdifrx_setting_t
 * @brief The SPDIFRX setting parameters
 */
typedef struct {
#define SPDIFRX_SRD_FS_CHANGE  (1 << 0)
	/*!< SPDIFRX SRD(sample rate detect) captures the event that the sample rate has changed.
	 * int callback(cb_data, #SPDIFRX_SRD_FS_CHANGE, audio_sr_sel_e *sr)
	 */
#define SPDIFRX_SRD_TIMEOUT    (1 << 1)
	/*!< SPDIFRX SRD(sample rate detect) timeout (disconnect) event.
	 * int callback(cb_data, #SPDIFRX_SRD_TIMEOUT, NULL)
	 */
	int (*srd_callback)(void *cb_data, u32_t cmd, void *param);
		/*!< sample rate detect callback */
	void *cb_data;
		/*!< callback user data */
} spdifrx_setting_t;

/*!
 * struct ain_param_t
 * @brief The audio in configuration parameters
 */
typedef struct {
#define AIN_DMA_IRQ_HF (1 << 0)
	/*!< DMA irq half full flag */
#define AIN_DMA_IRQ_TC (1 << 1)
	/*!< DMA irq transfer completly flag */
	u8_t sample_rate;
		/*!< The sample rate setting refer to enum audio_sr_sel_e */
	u16_t channel_type;
		/*!< Indicates the channel type selection and can refer to #AUDIO_CHANNEL_ADC, #AUDIO_CHANNEL_I2SRX, #AUDIO_CHANNEL_SPDIFRX*/
	audio_ch_width_e channel_width;
		/*!< The channel effective data width */
	adc_setting_t *adc_setting;
		/*!< The ADC function setting if has */
	i2srx_setting_t *i2srx_setting;
		/*!< The I2SRX function setting if has */
	spdifrx_setting_t *spdifrx_setting;
		/*!< The SPDIFRX function setting if has  */
	int (*callback)(void *cb_data, u32_t reason);
		/*!< The callback function which called when #AIN_DMA_IRQ_HF or #AIN_DMA_IRQ_TC events happened */
	void *cb_data;
		/*!< callback user data */
	audio_reload_t reload_setting;
		/*!< The reload mode setting which is mandatory*/
} ain_param_t;

/*!
 * struct ain_driver_api
 * @brief Public API for audio in driver
 */
struct ain_driver_api {
	void* (*ain_open)(struct device *dev, ain_param_t *param);
	int (*ain_close)(struct device *dev, void *handle);
	int (*ain_start)(struct device *dev, void *handle);
	int (*ain_stop)(struct device *dev, void *handle);
	int (*ain_control)(struct device *dev, void *handle, int cmd, void *param);
};

/*!
 * @brief Open the audio input channel by specified parameters
 * @param dev: The audio in device
 * @param param: The audio in parameter setting
 * @return The audio in handle
 */
static inline void* audio_in_open(struct device *dev, ain_param_t *setting)
{
	const struct ain_driver_api *api = dev->driver_api;

	return api->ain_open(dev, setting);
}

/*!
 * @brief Close the audio in channel by specified handle
 * @param dev: The audio in device
 * @param handle: The audio in handle
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_in_close(struct device *dev, void *handle)
{
	const struct ain_driver_api *api = dev->driver_api;

	return api->ain_close(dev, handle);
}

/*!
 * @brief Disable the audio in channel by specified handle
 * @param dev: The audio in device
 * @param handle: The audio in handle
 * @param cmd: The audio in command
 * @param param: The audio in in/out parameters corresponding with the commands
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_in_control(struct device *dev, void *handle, int cmd, void *param)
{
	const struct ain_driver_api *api = dev->driver_api;

	return api->ain_control(dev, handle, cmd, param);
}

/*!
 * @brief Start the audio in channel
 * @param dev: The audio in device
 * @param handle: The audio in handle
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_in_start(struct device *dev, void *handle)
{
	const struct ain_driver_api *api = dev->driver_api;

	return api->ain_start(dev, handle);
}

/*!
 * @brief Stop the audio in channel
 * @param dev: The audio in device
 * @param handle: The audio in handle
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_in_stop(struct device *dev, void *handle)
{
	const struct ain_driver_api *api = dev->driver_api;

	return api->ain_stop(dev, handle);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __AUDIO_IN_H__ */
