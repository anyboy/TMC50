/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for AUDIO OUT drivers
 */

#ifndef __AUDIO_OUT_H__
#define __AUDIO_OUT_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <audio_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AUDIO OUT Interface
 * @addtogroup AUDIO_OUT
 * @{
 */

/*! @brief Definition for the audio out control commands */
#define AOUT_CMD_GET_SAMPLERATE                               (1)
/*!< Get the channel audio sample rate by specified the audio channel handler.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_SAMPLERATE                               (2)
/*!< Set the channel audio sample rate by the giving audio channel handler.
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_OPEN_PA                                      (3)
/*!< Open PA device.
 * int audio_out_control(dev, NULL, #AOUT_CMD_SET_SAMPLERATE, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_CLOSE_PA                                     (4)
/*!< Close PA device.
 * int audio_out_control(dev, NULL, #AOUT_CMD_CLOSE_PA, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_PA_CLASS_SEL                                 (5)
/*!< Select external PA class AB or class D.
 * int audio_out_control(dev, NULL, #AOUT_CMD_PA_CLASS_SEL, audio_ext_pa_class_e *class)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_OUT_MUTE                                     (6)
/*!< Control output mute
  * int audio_out_control(dev, handle, #AOUT_CMD_OUT_MUTE, u8_t *mute_en)
  * If *mute_en is 1 will mute both the audio left and right channels, otherwise will unmute.
  * Returns 0 if successful and negative errno code if error.
  */

#define AOUT_CMD_GET_SAMPLE_CNT                               (7)
/*!< Get the sample counter from the audio output channel if enabled.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_SAMPLE_CNT, u32_t *count)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note  User need to handle the overflow case.
 */

#define AOUT_CMD_RESET_SAMPLE_CNT                             (8)
/*!< Reset the sample counter function which can retrieve the initial sample counter.
 * int audio_out_control(dev, handle, #AOUT_CMD_RESET_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_ENABLE_SAMPLE_CNT                            (9)
/*!< Enable the sample counter function to the specified audio channel.
 * int audio_out_control(dev, handle, #AOUT_CMD_ENABLE_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DISABLE_SAMPLE_CNT                           (10)
/*!< Disable the sample counter function by giving the audio channel handler.
 * int audio_out_control(dev, handle, #AOUT_CMD_DISABLE_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_VOLUME                                   (11)
/* Get the volume value of the audio channel.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_LEFT_VOLUME, volume_setting_t *volume)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_VOLUME                                   (12)
/*!< Set the volume value to the audio channel.
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_LEFT_VOLUME, volume_setting_t *volume)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_FIFO_LEN                                 (13)
/*!< Get the total length of audio channel FIFO.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_STATUS, u32_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_FIFO_AVAILABLE_LEN                       (14)
/*!< Get the avaliable length of audio channel FIFO that can be filled
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_STATUS, u32_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_CHANNEL_STATUS                           (15)
/*!< Get the audio channel busy status
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_CHANNEL_STATUS, u8 *status)
 * The output 'status' can refer to #AUDIO_CHANNEL_STATUS_BUSY or #AUDIO_CHANNEL_STATUS_ERROR
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_APS                                      (16)
/*!< Get the AUDIO_PLL APS
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_APS, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_APS                                      (17)
/*!< Set the AUDIO_PLL APS for the sample rate tuning
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_VOLUME, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SPDIF_SET_CHANNEL_STATUS                     (18)
/*!< Set the SPDIFTX channel status
 * int audio_out_control(dev, handle, #AOUT_CMD_SPDIF_SET_CHANNEL_STATUS, audio_spdif_ch_status_t *status)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SPDIF_GET_CHANNEL_STATUS                     (19)
/*!< Get the SPDIFTX channel status
 * int audio_out_control(dev, handle, #AOUT_CMD_SPDIF_GET_CHANNEL_STATUS, audio_spdif_ch_status_t *status)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_OPEN_I2STX_DEVICE                            (20)
/*!< Open I2STX device and will enable the MCLK/BCLK/LRCLK clock signals.
 * int audio_out_control(dev, NULL, #AOUT_CMD_OPEN_I2STX_CLK, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_CLOSE_I2STX_DEVICE                           (21)
/*!< Close I2STX device and will disable the MCLK/BCLK/LRCLK clock signals .
 * int audio_out_control(dev, NULL, #AOUT_CMD_CLOSE_I2STX_CLK, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_DAC_THRESHOLD                            (22)
/*!< Set the DAC threshold to control the stream buffer level at different scenes.
 * int audio_out_control(dev, NULL, #AOUT_CMD_SET_DAC_THRESHOLD, dac_threshold_setting_t *thres)
 * Returns 0 if successful and negative errno code if error.
 */

/*!
 * struct volume_setting_t
 * @brief Config the left/right volume setting
 */
typedef struct {
#define AOUT_VOLUME_INVALID             (0xFFFFFFFF)
	/*!< macro to the invalid volume value */
	s32_t left_volume;
		/*!< specifies the volume of left channel which range from -71625(-71.625db) to 24000(24.000db) and if set #AOUT_VOLUME_INVALID to ignore this setting */
	s32_t right_volume;
		/*!< specifies the volume of right channel which range from range from -71625(-71.625db) to 24000(24.000db)  and if set #AOUT_VOLUME_INVALID to ignore this setting */
} volume_setting_t;

/*!
 * struct dac_threshold_setting_t
 * @brief The setting of the DAC PCMBUF threshold
 */
typedef struct {
	u32_t he_thres;
		/*!< The half empty threshold */
	u32_t hf_thres;
		/*!< The half full threshold */
} dac_threshold_setting_t;

/*!
 * struct dac_setting_t
 * @brief The DAC setting parameters
 */
typedef struct {
	audio_ch_mode_e channel_mode;
		/*!< Select the channel mode such as mono or strereo */
	volume_setting_t volume;
		/*!< The left and right volume setting */
} dac_setting_t;

/*!
 * struct i2stx_setting_t
 * @brief The I2STX setting parameters
 */
typedef struct {
#define I2STX_SRD_FS_CHANGE  (1 << 0)
/*!< I2STX SRD(sample rate detect) captures the event that the sample rate has changed
 * int callback(cb_data, #I2STX_SRD_FS_CHANGE, audio_sr_sel_e *sr)
 */
#define I2STX_SRD_WL_CHANGE  (1 << 1)
/*!< I2STX SRD(sample rate detect) captures the event that the effective width length has changed
 * int callback(cb_data, #I2STX_SRD_WL_CHANGE, audio_i2s_srd_wl_e *wl)
 */
#define I2STX_SRD_TIMEOUT    (1 << 2)
/*!< I2STX SRD(sample rate detect) captures the timeout (disconnection) event
 * int callback(cb_data, #I2STX_SRD_TIMEOUT, NULL)
 */
	audio_i2s_mode_e mode;
		/*!< I2S master mode or slave mode selection */
	int (*srd_callback)(void *cb_data, u32_t cmd, void *param);
		/*!< The callback function from I2STX SRD module which worked in the slave mode */
	void *cb_data;
		/*!< Callback user data */
} i2stx_setting_t;

/*!
 * struct spdiftx_setting_t
 * @brief The SPDIFTX setting parameters
 * @note The FIFOs used by SPDIFTX can be #AOUT_FIFO_DAC0 or #AOUT_FIFO_I2STX0.
 * If select the #AOUT_FIFO_DAC0, the clock source will use the division 2 of the DAC clock source.
 * If select the #AOUT_FIFO_I2STX0, there is configurations (CONFIG_SPDIFTX_USE_I2STX_MCLK/CONFIG_SPDIFTX_USE_I2STX_MCLK_DIV2) to control the clock source.
 */
typedef struct {
	audio_spdif_ch_status_t *status; /*!< The channel status setting if has. If setting NULL, low level driver will use a default channel status value*/
} spdiftx_setting_t;

/*!
 * struct aout_param_t
 * @brief The audio out configuration parameters
 */
typedef struct {
#define AOUT_DMA_IRQ_HF         (1 << 0)     /*!< DMA irq half full flag */
#define AOUT_DMA_IRQ_TC         (1 << 1)     /*!< DMA irq transfer completly flag */
	u8_t sample_rate;
		/*!< The sample rate setting and can refer to enum audio_sr_sel_e */
	u16_t channel_type;
		/*!< Indicates the channel type selection and can refer to #AUDIO_CHANNEL_DAC, #AUDIO_CHANNEL_I2STX, #AUDIO_CHANNEL_SPDIFTX*/
	audio_ch_width_e channel_width;
		/*!< The channel effective data width */
	audio_outfifo_sel_e outfifo_type;
		/*!< Indicates the used output fifo type */
	dac_setting_t *dac_setting;
		/*!< The DAC function setting if has */
	i2stx_setting_t *i2stx_setting;
		/*!< The I2STX function setting if has */
	spdiftx_setting_t *spdiftx_setting;
		/*!< The SPDIFTX function setting if has */
	int (*callback)(void *cb_data, u32_t reason);
		/*!< The callback function which conrespondingly with the events such as #AOUT_PCMBUF_IP_HE or #AOUT_PCMBUF_IP_HF etc.*/
	void *cb_data;
		/*!< Callback user data */
	audio_reload_t *reload_setting;
		/*!< The reload mode setting and if don't use this mode, please let 'reload_setting = NULL' */
} aout_param_t;

/*!
 * struct aout_driver_api
 * @brief The sturcture to define audio out driver API.
 */
struct aout_driver_api {
	void* (*aout_open)(struct device *dev, aout_param_t *param);
	int (*aout_close)(struct device *dev, void *handle);
	int (*aout_start)(struct device *dev, void *handle);
	int (*aout_write)(struct device *dev, void *handle, u8_t *buffer, u32_t length);
	int (*aout_stop)(struct device *dev, void *handle);
	int (*aout_control)(struct device *dev, void *handle, int cmd, void *param);
};

/*!
 * @brief Open the audio output channel by specified parameters
 * @param dev: The audio out device
 * @param param: The audio out parameter setting
 * @return The audio out handle
 */
static inline void* audio_out_open(struct device *dev, aout_param_t *setting)
{
	const struct aout_driver_api *api = dev->driver_api;

	return api->aout_open(dev, setting);
}

/*!
 * @brief Disable the audio out channel by specified handle
 * @param dev: The audio out device
 * @param handle: The audio out handle
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_out_close(struct device *dev, void *handle)
{
	const struct aout_driver_api *api = dev->driver_api;

	return api->aout_close(dev, handle);
}

/*!
 * @brief Disable the audio out channel by specified handle
 * @param dev: The audio out device
 * @param handle: The audio out handle
 * @param cmd: The audio out command
 * @param param: The audio out in/out parameters corresponding with the commands
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_out_control(struct device *dev, void *handle, int cmd, void *param)
{
	const struct aout_driver_api *api = dev->driver_api;

	return api->aout_control(dev, handle, cmd, param);
}

/*!
 * @brief Start the audio out channel
 * @param dev: The audio out device
 * @param handle: The audio out handle
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_out_start(struct device *dev, void *handle)
{
	const struct aout_driver_api *api = dev->driver_api;

	return api->aout_start(dev, handle);
}

/*!
 * @brief Write data to the audio out channel
 * @param dev: The audio out device
 * @param handle: The audio out handle
 * @param buffer: The buffer to be output
 * @param length: The length of the buffer
 * @return 0 on success, negative errno code on fail
 * @note There are 2 mode to transfer the output data.
 * One is reload mode which is using the same buffer and length and call acts_aout_start one time,
 * the other is direct mode which use different buffer/length and call acts_aout_start separatly.
 */
static inline int audio_out_write(struct device *dev, void *handle, u8_t *buffer, u32_t length)
{
	const struct aout_driver_api *api = dev->driver_api;

	return api->aout_write(dev, handle, buffer, length);
}

/*!
 * @brief Stop the audio out channel
 * @param dev: The audio out device
 * @param handle: The audio out handle
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_out_stop(struct device *dev, void *handle)
{
	const struct aout_driver_api *api = dev->driver_api;

	return api->aout_stop(dev, handle);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __AUDIO_OUT_H__ */
