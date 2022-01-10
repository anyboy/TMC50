/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AUDIO physical common API
 */

#ifndef __PHY_AUDIO_COMMON_H__
#define __PHY_AUDIO_COMMON_H__

#include <device.h>

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define PHY_AUDIO_DAC_NAME "DAC"
#define PHY_AUDIO_ADC_NAME "ADC"
#define PHY_AUDIO_I2STX_NAME "I2STX"
#define PHY_AUDIO_I2SRX_NAME "I2SRX"
#define PHY_AUDIO_SPDIFTX_NAME "SPDIFTX"
#define PHY_AUDIO_SPDIFRX_NAME "SPDIFRX"

/* @brief Definition for the physical audio control internal commands */
#define PHY_CMD_BASE                         (0xFF)

#define PHY_CMD_DUMP_REGS                    (PHY_CMD_BASE + 1)
/* Dump the audio in/out controller regiser for debug
 *  int phy_audio_control(dev, #PHY_CMD_DUMP_REGS, NULL)
 */

#define PHY_CMD_FIFO_GET                     (PHY_CMD_BASE + 2)
/* Get a FIFO for using from the speicified audio channel
 *  int phy_audio_control(dev, #PHY_CMD_FIFO_GET, void *param)
 */

#define PHY_CMD_FIFO_PUT                     (PHY_CMD_BASE + 3)
/* Put the FIFO to the speicified audio channel
 *  int phy_audio_control(dev, #PHY_CMD_FIFO_PUT, NULL)
 */

#define PHY_CMD_DAC_WAIT_EMPTY               (PHY_CMD_BASE + 4)
/* Check and wait the empty pending of DAC FIFO
 *  int phy_audio_control(dev, #PHY_CMD_DAC_CHECK_EMPTY, NULL)
 */

#define PHY_CMD_CLAIM_WITH_128FS             (PHY_CMD_BASE + 5)
/* Claim that current audio channels exist 128fs channel typically spdiftx which used in multi-linkage mode
 *  int phy_audio_control(dev, #PHY_CMD_CLAIM_WITH_128FS, NULL)
 */

#define PHY_CMD_CLAIM_WITHOUT_128FS          (PHY_CMD_BASE + 6)
/* Claim that current audio channels do not exist 128fs channel typically spdiftx which used in multi-linkage mode
 *  int phy_audio_control(dev, #PHY_CMD_CLAIM_WITHOUT_128FS, NULL)
 */

#define PHY_CMD_I2STX_IS_OPENED              (PHY_CMD_BASE + 7)
/* Get the open status of i2stx channel
 *  int phy_audio_control(dev, #PHY_CMD_I2STX_IS_OPENED, u8_t *status)
 */

#define PHY_CMD_I2SRX_IS_OPENED              (PHY_CMD_BASE + 8)
/* Get the open status of i2srx channel
 *  int phy_audio_control(dev, #PHY_CMD_I2SRX_IS_OPENED, u8_t *status)
 */

#define PHY_CMD_I2S_ENABLE_5WIRE             (PHY_CMD_BASE + 9)
/* Enable I2STX/I2SRX 5 wires mode and within this mode rx follows tx simultaneously.
 *  int phy_audio_control(dev, #PHY_CMD_I2S_ENABLE_5WIRE, NULL)
 *  NOTE!! I2SRX and I2STX shall be configured firstly, and then enable 5 wires mode, at last enable tx_en.
 */

#define PHY_CMD_I2STX_DISABLE_DEVICE         (PHY_CMD_BASE + 10)
/* Disable I2STX hardware resouces include BCLK/LRCLK output signals.
 *  int phy_audio_control(dev, #PHY_CMD_I2STX_DISABLE_DEVICE, NULL)
 */

#define PHY_CMD_CHANNEL_START                (PHY_CMD_BASE + 11)
/* Notify the physical driver to start the channel.
 *  int phy_audio_control(dev, #PHY_CMD_CHANNEL_START, NULL)
 */

/*
 * enum audio_fifouse_sel_e
 * @brief FIFO use object select
 */
typedef enum {
	FIFO_SEL_CPU = 0,
	FIFO_SEL_DMA
} audio_fifouse_sel_e;

/*
 * enum audio_dma_width_e
 * @brief DMA transfer width configured
 */
typedef enum {
	DMA_WIDTH_32BITS = 0,
	DMA_WIDTH_16BITS
} audio_dma_width_e;

/*
 * enum audio_i2s_srd_period_e
 * @brief I2S sample rate detect period selection
 */
typedef enum {
	I2S_SRD_2LRCLK = 0,
	I2S_SRD_4LRCLK
} audio_i2s_srd_period_e;


struct phy_audio_device;

/*
 * @struct phy_audio_operations
 * @brief Audio physical operations
 */
struct phy_audio_operations {
	int (*audio_init)(struct phy_audio_device *dev, struct device *parent_dev);
	int (*audio_enable)(struct phy_audio_device *dev, void *param);
	int (*audio_disable)(struct phy_audio_device *dev);
	int (*audio_ioctl)(struct phy_audio_device *dev, u32_t cmd, void *param);
};

/*
 * @struct phy_audio_device
 * @brief Physical audio abstract device
 */
struct phy_audio_device {
	const char *name; /* audio driver name of the physical channel */
	const struct phy_audio_operations *audio_ops; /* the common operations of the physical audio */
	void *private; /* the private data */
};

/*
 * @brief Define to alloc the physical audio deivce resource
 */
#define PHY_AUDIO_DEVICE(dev_name, drv_name, ops, priv) \
	static struct phy_audio_device __phy_a_ ## dev_name = { \
		.name = (drv_name), \
		.audio_ops = (ops), \
		.private = (void *)priv \
	}

/*
 * @brief Get the physical audio device object
 */
#define PHY_AUDIO_DEVICE_NAME_GET(name) (__phy_a_ ## name)
/*
 * @brief Get the address of the physical audio device object
 */
#define PHY_AUDIO_DEVICE_GET(name) (&PHY_AUDIO_DEVICE_NAME_GET(name))

/*
 * @brief The function of getting the physical audio device by name
 */
#define PHY_AUDIO_DEVICE_FN(name) \
	struct phy_audio_device * get_audio_device_ ## name (void) { \
		return PHY_AUDIO_DEVICE_GET(name); \
	}

/*
 * @brief The common function to get the audio device by the specific name
 */
#define GET_AUDIO_DEVICE(name) (get_audio_device_ ## name())

#define GET_AUDIO_DEVICE_NULL(name) \
	static inline struct phy_audio_device * get_audio_device_ ## name(void) { \
		return NULL; \
	}

/*
 * @brief physical audio initialization interface
 * @param dev: The physical audio device object
 * @param parent_dev: The parent audio device
 * @return 0 on success, negative errno code on fail
 */
static inline int phy_audio_init(struct phy_audio_device *dev, struct device *parent_dev)
{
	if (!dev)
		return -EINVAL;

	return dev->audio_ops->audio_init(dev, parent_dev);
}

/*
 * @brief physical audio enable interface
 * @param dev: The physical audio device object
 * @param param: The parameters to enable physical audio channel
 * @return 0 on success, negative errno code on fail
 */
static inline int phy_audio_enable(struct phy_audio_device *dev, void *param)
{
	if (!dev)
		return -EINVAL;

	return dev->audio_ops->audio_enable(dev, param);
}

/*
 * @brief physical audio disable interface
 * @param dev: The physical audio device object
 * @return 0 on success, negative errno code on fail
 */
static inline int phy_audio_disable(struct phy_audio_device *dev)
{
	if (!dev)
		return -EINVAL;

	return dev->audio_ops->audio_disable(dev);
}

/*
 * @brief physical audio control interface
 * @param dev: The physical audio device object
 * @param param: The parameters to control physical audio channel
 * @return 0 on success, negative errno code on fail
 */
static inline int phy_audio_control(struct phy_audio_device *dev, u32_t cmd, void *param)
{
	if (!dev)
		return -EINVAL;

	return dev->audio_ops->audio_ioctl(dev, cmd, param);
}

/*
 * @brief Get the audio DAC device
 * @param parrent: specify the parrent device of the DAC
 * @return NULL on fail and others on success
 */
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
struct phy_audio_device *get_audio_device_dac(void);
#else
GET_AUDIO_DEVICE_NULL(dac)
#endif

/*
 * @brief Get the audio ADC device
 * @param parrent: specify the parrent device of the ADC
 * @return NULL on fail and others on success
 */
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
struct phy_audio_device *get_audio_device_adc(void);
#else
GET_AUDIO_DEVICE_NULL(adc)
#endif
/*
 * @brief Get the audio I2STX device
 * @param parrent: specify the parrent device of the I2STX
 * @return NULL on fail and others on success
 */
#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
struct phy_audio_device *get_audio_device_i2stx(void);
#else
GET_AUDIO_DEVICE_NULL(i2stx)
#endif

/*
 * @brief Get the audio I2SRX device
 * @param parrent: specify the parrent device of the I2SRX
 * @return NULL on fail and others on success
 */
#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
struct phy_audio_device *get_audio_device_i2srx(void);
#else
GET_AUDIO_DEVICE_NULL(i2srx)
#endif

/*
 * @brief Get the audio SPDIFTX device
 * @param parrent: specify the parrent device of the SPDIFTX
 * @return NULL on fail and others on success
 */
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
struct phy_audio_device *get_audio_device_spdiftx(void);
#else
GET_AUDIO_DEVICE_NULL(spdiftx)
#endif

/*
 * @brief Get the audio SPDIFRX device
 * @param parrent: specify the parrent device of the SPDIFRX
 * @return NULL on fail and others on success
 */
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
struct phy_audio_device *get_audio_device_spdifrx(void);
#else
GET_AUDIO_DEVICE_NULL(spdifrx)
#endif

#endif /* __PHY_AUDIO_COMMON_H__ */
