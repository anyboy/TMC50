/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio IN(ADC/I2S-RX/SPDIF-RX) core management layer
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <dma.h>
#include <audio_in.h>
#include "phy_audio_common.h"

#define SYS_LOG_DOMAIN "AIN"
#include <logging/sys_log.h>

#define AIN_SESSION_MAGIC                    (0x11223344)
#define AIN_SESSION_CHECK_MAGIC(x)           ((x) == AIN_SESSION_MAGIC)
#define AIN_SESSION_MAX                      (3)      /* 1 ADC channel + 1 I2SRX channel + 1 SPDIFRX channel */
#define AIN_SESSION_OPEN                     (1 << 0) /* Session open flag */
#define AIN_SESSION_CONFIG                   (1 << 1) /* Session config flag */
#define AIN_SESSION_START                    (1 << 2) /* Session start flag */

/*
 * struct ain_session_t
 * @brief audio in session structure
 */
typedef struct {
	int magic; /* session magic value as AIN_SESSION_MAGIC */
	int dma_chan; /* DMA channel handle */
	u16_t channel_type; /* Indicates the channel type selection */
	u16_t flags; /* session flags */
	int (*callback)(void *cb_data, u32_t reason); /* The callback function which called when #AIN_DMA_IRQ_HF or #AIN_DMA_IRQ_TC events happened */
	void *cb_data;                  /* callback user data */
	u8_t *reload_addr; /* Reload buffer address to transfer */
	u32_t reload_len;  /* The length of the reload buffer */
} ain_session_t;

/*
 * struct aout_drv_data
 * @brief audio in driver data which are software related
 */
struct ain_drv_data {
	struct k_sem lock;
	struct device *dma_dev;
	struct phy_audio_device *adc; /* physical ADC device */
	struct phy_audio_device *i2srx; /* physical I2SRX device */
	struct phy_audio_device *spdifrx; /* physical SPDIFRX device */
};

/*
 * struct aout_config_data
 * @brief audio in config data which are the hardward related
 */
struct ain_config_data {
	u8_t adc_reset_id; /* ADC reset ID */
	u8_t i2srx_reset_id; /* I2S RX reset ID */
	u8_t spdifrx_reset_id; /* SPIDF RX reset ID */
};

static ain_session_t ain_sessions[AIN_SESSION_MAX];

/*
 * @brief checkout if there is the same channel within audio in sessions
 */
static bool ain_session_check(u16_t type)
{
	int i;
	ain_session_t *sess = ain_sessions;

	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if (AIN_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AIN_SESSION_MAX)
			&& (sess->channel_type & type)) {
			SYS_LOG_ERR("Audio channel %d is already in use", type);
			return true;
		}
	}

	return false;
}

/*
 * @brief Get audio in session by specified channel
 */
static ain_session_t *ain_session_get(u16_t type)
{
	ain_session_t *sess = ain_sessions;
	int i;
	if (ain_session_check(type))
		return NULL;

	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if (!(sess->flags & AIN_SESSION_OPEN)) {
			memset(sess, 0, sizeof(ain_session_t));
			sess->magic = AIN_SESSION_MAGIC;
			sess->flags = AIN_SESSION_OPEN;
			sess->channel_type = type;
			return sess;
		}
	}

	return NULL;
}

/*
 * @brief Put audio in session by given session address
 */
static void ain_session_put(ain_session_t *s)
{
	ain_session_t *sess = ain_sessions;
	int i;
	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if ((u32_t)s == (u32_t)sess) {
			memset(s, 0, sizeof(ain_session_t));
			break;
		}
	}
}

/* @brief audio in session lock */
static inline void audio_in_lock(struct device *dev)
{
	struct ain_drv_data *data = dev->driver_data;
	k_sem_take(&data->lock, K_FOREVER);
}

/* @brief audio in session unlock */
static inline void audio_in_unlock(struct device *dev)
{
	struct ain_drv_data *data = dev->driver_data;
	k_sem_give(&data->lock);
}

static int audio_in_init(struct device *dev)
{
#if defined(CONFIG_AUDIO_IN_ADC_SUPPORT) || defined(CONFIG_AUDIO_IN_I2SRX0_SUPPORT) \
		|| defined(CONFIG_AUDIO_IN_SPDIFRX_SUPPORT)
	const struct ain_config_data *cfg = dev->config->config_info;
#endif
	struct ain_drv_data *data = dev->driver_data;

	k_sem_init(&data->lock, 1, 1);

	data->dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (!data->dma_dev) {
		SYS_LOG_ERR("Bind DMA device %s error", CONFIG_DMA_0_NAME);
		return -ENOENT;
	}

	data->adc = GET_AUDIO_DEVICE(adc);
	if (!data->adc) {
		SYS_LOG_WRN("No physical ADC device");
	} else {
		if (phy_audio_init(data->adc, dev)) {
			SYS_LOG_ERR("Failed to probe PHY ADC");
			return -ENXIO;
		}
	}

	data->i2srx = GET_AUDIO_DEVICE(i2srx);
	if (!data->i2srx) {
		SYS_LOG_WRN("No physical I2SRX device");
	} else {
		if (phy_audio_init(data->i2srx, dev)) {
			SYS_LOG_ERR("Failed to probe PHY I2SRX");
			return -ENXIO;
		}
	}

	data->spdifrx = GET_AUDIO_DEVICE(spdifrx);
	if (!data->spdifrx) {
		SYS_LOG_WRN("No physical SPDIFRX device");
	} else {
		if (phy_audio_init(data->spdifrx, dev)) {
			SYS_LOG_ERR("Failed to probe PHY SPDIFRX");
			return -ENXIO;
		}
	}

	/* Set ADC/I2SRX/SPDIFRX controller to be normal */
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
	acts_reset_peripheral(cfg->adc_reset_id);
#endif

#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
	acts_reset_peripheral(cfg->i2srx_reset_id);
#endif

#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
	acts_reset_peripheral(cfg->spdifrx_reset_id);
#endif

	return 0;
}

/* @brief Enable the ADC channel */
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
static int audio_in_enable_adc(struct device *dev, ain_param_t *param)
{
	struct ain_drv_data *data = dev->driver_data;
	struct phy_audio_device *adc = data->adc;

	if (!adc) {
		SYS_LOG_ERR("Physical ADC device is not esixted");
		return -EFAULT;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_18BITS)) {
		SYS_LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	return phy_audio_enable(adc, (void *)param);
}
#endif

/* @brief Enable the I2SRX channel */
#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
static int audio_in_enable_i2srx(struct device *dev, ain_param_t *param)
{
	struct ain_drv_data *data = dev->driver_data;
	struct phy_audio_device *i2srx = data->i2srx;

	if (!i2srx) {
		SYS_LOG_ERR("Physical I2SRX device is not esixted");
		return -EFAULT;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_20BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		SYS_LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	return phy_audio_enable(i2srx, (void *)param);
}
#endif

/* @brief Enable the SPDIFRX channel */
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
static int audio_in_enable_spdifrx(struct device *dev, ain_param_t *param)
{
	struct ain_drv_data *data = dev->driver_data;
	struct phy_audio_device *spdifrx = data->spdifrx;

	if (!spdifrx) {
		SYS_LOG_ERR("Physical SPDIFRX device is not esixted");
		return -EFAULT;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		SYS_LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	return phy_audio_enable(spdifrx, (void *)param);
}
#endif

/* @brief DMA irq callback on reload method */
static void audio_in_dma_reload(struct device *dev, u32_t priv_data, int reason)
{
	ain_session_t *session = (ain_session_t *)priv_data;
	u32_t ret_reson;
	u32_t key;
	ARG_UNUSED(dev);

	if (session && AIN_SESSION_CHECK_MAGIC(session->magic)) {
		if (session->callback) {
			key = irq_lock();
			if (reason == DMA_IRQ_HF) {
				ret_reson = AIN_DMA_IRQ_HF;
			} else {
				ret_reson = AIN_DMA_IRQ_TC;
			}
			session->callback(session->cb_data, ret_reson);
			irq_unlock(key);
		}
	}
}

/* @brief prepare the DMA configuration for the audio in transfer */
static int audio_in_dma_prepare(struct device *dev, ain_session_t *session, ain_param_t *param)
{
	struct ain_drv_data *data = dev->driver_data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	/* request dma channel handle */
	session->dma_chan = dma_request(data->dma_dev, 0xFF);
	if (!session->dma_chan) {
		SYS_LOG_ERR("Failed to request dma channel");
		return -ENXIO;
	}

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;

	/* Only support the DMA reload mode */
	dma_block_cfg.dest_reload_en = 1;

	dma_cfg.head_block = &dma_block_cfg;

	if (CHANNEL_WIDTH_16BITS == param->channel_width)
		dma_cfg.source_data_size = 2; /* DMA data width 16 bits */
	else
		dma_cfg.source_data_size = 4; /* DMA data width 32 bits */

	if (AUDIO_CHANNEL_ADC == session->channel_type) {
		dma_cfg.dma_slot = DMA_ID_AUDIO_ADC_FIFO;
	} else if (AUDIO_CHANNEL_I2SRX == session->channel_type) {
		dma_cfg.dma_slot = DMA_ID_AUDIO_I2S;
	} else if (AUDIO_CHANNEL_SPDIFRX == session->channel_type) {
		dma_cfg.dma_slot = DMA_ID_AUDIO_SPDIF;
	}

	/* Use the DMA irq to notify the status of transfer */
	if (session->callback) {
		dma_cfg.dma_callback = audio_in_dma_reload;
		dma_cfg.callback_data = session;
		dma_cfg.complete_callback_en = 1;
		dma_cfg.half_complete_callback_en = 1;
	}

	if (dma_config(data->dma_dev, session->dma_chan, &dma_cfg)) {
		SYS_LOG_ERR("DMA config error");
		return -EFAULT;
	}

	if (session->reload_addr) {
		SYS_LOG_DBG("reload addr:0x%x, reload len:%d\n", (u32_t)session->reload_addr, session->reload_len);
		dma_reload(data->dma_dev, session->dma_chan,
			0, (u32_t)session->reload_addr, session->reload_len);
	}

	return 0;
}

/* @brief audio out enable and return the session handler */
static void *acts_audio_in_open(struct device *dev, ain_param_t *param)
{
	ain_session_t * session = NULL;
	u8_t channel_type;
	int ret;
	void *rtn_sess = NULL;

	if (!param) {
		SYS_LOG_ERR("NULL parameter");
		return NULL;
	}

	audio_in_lock(dev);

	channel_type = param->channel_type;
	session = ain_session_get(channel_type);
	if (!session) {
		SYS_LOG_ERR("Failed to get audio session (type:%d)", channel_type);
		goto out;
	}

	if (param->reload_setting.reload_addr && !param->reload_setting.reload_len) {
		SYS_LOG_ERR("Reload parameter error addr:0x%x, len:0x%x",
			(u32_t)param->reload_setting.reload_addr, param->reload_setting.reload_len);
		ain_session_put(session);
		goto out;
	}

	session->callback = param->callback;
	session->cb_data = param->cb_data;
	session->reload_addr = param->reload_setting.reload_addr;
	session->reload_len = param->reload_setting.reload_len;

	if (channel_type == AUDIO_CHANNEL_ADC) {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
		ret = audio_in_enable_adc(dev, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_I2SRX) {
#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
		ret = audio_in_enable_i2srx(dev, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_SPDIFRX) {
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
		ret = audio_in_enable_spdifrx(dev, param);
#else
		ret = -ENXIO;
#endif
	} else {
		SYS_LOG_ERR("Invalid channel type %d", channel_type);
		ain_session_put(session);
		goto out;
	}

	if (ret) {
		SYS_LOG_ERR("Enable channel type %d error:%d", channel_type, ret);
		ain_session_put(session);
		goto out;
	}

	ret = audio_in_dma_prepare(dev, session, param);
	if (ret) {
		SYS_LOG_ERR("prepare session dma error %d", ret);
		ain_session_put(session);
		goto out;
	}

	session->flags |=  AIN_SESSION_CONFIG;

	/* The session address is the audio in handler */
	rtn_sess = (void *)session;

	SYS_LOG_INF("channel#%p type:%d opened", session, channel_type);

out:
	audio_in_unlock(dev);
	return rtn_sess;
}

/* @brief Disable the ADC channel */
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
static int audio_in_disable_adc(struct device *dev, ain_session_t *session)
{
	struct ain_drv_data *data = dev->driver_data;
	struct phy_audio_device *adc = data->adc;

	if (!adc) {
		SYS_LOG_ERR("Physical ADC device is not esixted");
		return -EFAULT;
	}

	return phy_audio_disable(adc);
}
#endif

/* @brief Disable the I2SRX channel */
#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
static int audio_in_disable_i2srx(struct device *dev, ain_session_t *session)
{
	struct ain_drv_data *data = dev->driver_data;
	struct phy_audio_device *i2srx = data->i2srx;

	if (!i2srx) {
		SYS_LOG_ERR("Physical I2SRX device is not esixted");
		return -EFAULT;
	}

	return phy_audio_disable(i2srx);
}
#endif

/* @brief Disable the SPDIFRX channel */
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
static int audio_in_disable_spdifrx(struct device *dev, ain_session_t *session)
{
	struct ain_drv_data *data = dev->driver_data;
	struct phy_audio_device *spdifrx = data->spdifrx;

	if (!spdifrx) {
		SYS_LOG_ERR("Physical SPDIFRX device is not esixted");
		return -EFAULT;
	}

	return phy_audio_disable(spdifrx);
}
#endif

/* @brief Disable audio in channel by specified session handler */
static int acts_audio_in_close(struct device *dev, void *handle)
{
	struct ain_drv_data *data = dev->driver_data;
	u8_t channel_type;
	int ret;
	ain_session_t *session = (ain_session_t *)handle;

	if (!handle) {
		SYS_LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	audio_in_lock(dev);

	if (!AIN_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_ERR("Session magic error:0x%x", session->magic);
		ret = -EFAULT;
		goto out;
	}

	channel_type = session->channel_type;
	if (channel_type == AUDIO_CHANNEL_ADC) {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
		ret = audio_in_disable_adc(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_I2SRX) {
#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
		ret = audio_in_disable_i2srx(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_SPDIFRX) {
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
		ret = audio_in_disable_spdifrx(dev, session);
#else
		ret = -ENXIO;
#endif
	} else {
		SYS_LOG_ERR("Invalid channel type %d", channel_type);
		ret = -EINVAL;
		goto out;
	}

	if (ret) {
		SYS_LOG_ERR("Disable channel type %d error:%d", channel_type, ret);
		goto out;
	}

	/* stop and free dma channel resource */
 	dma_stop(data->dma_dev, session->dma_chan);
 	dma_free(data->dma_dev, session->dma_chan);

 	SYS_LOG_INF("session#%p disabled", session);

	ain_session_put(session);

out:
	audio_in_unlock(dev);
	return ret;
}

static int acts_audio_in_control(struct device *dev, void *handle, int cmd, void *param)
{
#if defined(CONFIG_AUDIO_IN_ADC_SUPPORT) || defined(CONFIG_AUDIO_IN_I2SRX0_SUPPORT) \
			|| defined(CONFIG_AUDIO_IN_SPDIFRX_SUPPORT)
	struct ain_drv_data *data = dev->driver_data;
#endif
	ain_session_t *session = (ain_session_t *)handle;
	u8_t channel_type = session->channel_type;
	int ret;

	SYS_LOG_DBG("session#0x%x cmd 0x%x", (u32_t)handle, cmd);

	if (!AIN_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

	if (channel_type == AUDIO_CHANNEL_ADC) {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
		ret = phy_audio_control(data->adc, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_I2SRX) {
#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
		ret = phy_audio_control(data->i2srx, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_SPDIFRX) {
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
		ret = phy_audio_control(data->spdifrx, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else {
		SYS_LOG_ERR("Invalid channel type %d", channel_type);
		ret = -EINVAL;
	}

	return ret;
}

/* @brief Start the audio input transfering */
static int acts_audio_in_start(struct device *dev, void *handle)
{
	struct ain_drv_data *data = dev->driver_data;
	ain_session_t *session = (ain_session_t *)handle;

	if (!handle) {
		SYS_LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	if (!AIN_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

	/* In DMA reload mode only start one time is enough */
	if (session->flags & AIN_SESSION_START) {
		return 0;
	}

	if (!(session->flags & AIN_SESSION_START))
		session->flags |= AIN_SESSION_START;

	return dma_start(data->dma_dev, session->dma_chan);
}

static int acts_audio_in_stop(struct device *dev, void *handle)
{
	struct ain_drv_data *data = dev->driver_data;
	ain_session_t *session = (ain_session_t *)handle;
	int ret = 0;

	if (session && AIN_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_INF("session#%p, audio in stop", session);
		ret = dma_stop(data->dma_dev, session->dma_chan);
	}

	return ret;
}

static struct ain_drv_data audio_in_drv_data;

static struct ain_config_data audio_in_config_data = {
	.adc_reset_id = RESET_ID_AUDIO_ADC,
	.i2srx_reset_id = RESET_ID_I2S0_RX,
	.spdifrx_reset_id = RESET_ID_SPDIF_RX,
};

const struct ain_driver_api ain_drv_api = {
	.ain_open = acts_audio_in_open,
	.ain_close = acts_audio_in_close,
	.ain_control = acts_audio_in_control,
	.ain_start = acts_audio_in_start,
	.ain_stop = acts_audio_in_stop
};

DEVICE_AND_API_INIT(audio_in, CONFIG_AUDIO_IN_ACTS_DEV_NAME, audio_in_init,
	    &audio_in_drv_data, &audio_in_config_data,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ain_drv_api);
