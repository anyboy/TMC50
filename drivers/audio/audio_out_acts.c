/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio OUT(DAC/I2S-TX/SPDIF-TX) core management layer
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <dma.h>
#include <board.h>
#include <audio_common.h>
#include <audio_out.h>
#include "phy_audio_common.h"

#define SYS_LOG_DOMAIN "AOUT"
#include <logging/sys_log.h>

#define AOUT_SESSION_MAGIC                 (0x1a2b3c4d)
#define AOUT_SESSION_CHECK_MAGIC(x)        ((x) == AOUT_SESSION_MAGIC)
#define AOUT_SESSION_MAX                   (3)      /* 1 DAC channel + 1 I2STX channel + 1 SPDIFTX channel */
#define AOUT_SESSION_OPEN                  (1 << 0) /* Session open flag */
#define AOUT_SESSION_CONFIG                (1 << 1) /* Session config flag */
#define AOUT_SESSION_START                 (1 << 2) /* Session start flag */

#define AOUT_SESSION_WAIT_PA_TIMEOUT       (2000) /* Timeout of waitting pa session */
#define AOUT_SESSION_DMA_TIMEOUT_US        (1000000) /* Timeout of DMA transmission in microseconds */

#define AOUT_IS_FIFO_CMD(x) ((AOUT_CMD_GET_SAMPLE_CNT == x) \
		|| (AOUT_CMD_RESET_SAMPLE_CNT == x) \
		|| (AOUT_CMD_ENABLE_SAMPLE_CNT == x) \
		|| (AOUT_CMD_DISABLE_SAMPLE_CNT == x) \
		|| (AOUT_CMD_GET_CHANNEL_STATUS == x) \
		|| (AOUT_CMD_GET_FIFO_LEN == x) \
		|| (AOUT_CMD_GET_FIFO_AVAILABLE_LEN == x) \
		|| (AOUT_CMD_GET_VOLUME == x) \
		|| (AOUT_CMD_SET_VOLUME == x) \
		|| (AOUT_CMD_GET_APS == x) \
		|| (AOUT_CMD_SET_APS == x) \
		|| (AOUT_CMD_SET_DAC_THRESHOLD == x))

/*
 * struct aout_session_t
 * @brief audio out session structure
 */
typedef struct {
	int magic; /* session magic value as AOUT_SESSION_MAGIC */
	int dma_chan; /* DMA channel handle */
	audio_outfifo_sel_e outfifo_type; /* Indicates the used output fifo type */
	u16_t channel_type; /* Indicates the channel type selection */
	u16_t flags; /* session flags */
	u8_t sample_rate; /* The sample rate setting */
	int (*callback)(void *cb_data, u32_t reason);
	void *cb_data;
	u8_t *reload_addr; /* Reload buffer address to transfer */
	u32_t reload_len;  /* The length of the reload buffer */
	u8_t reload_en : 1; /* The flag of reload mode enable */
} aout_session_t;

/*
 * struct aout_drv_data
 * @brief audio out driver data which are software related
 */
struct aout_drv_data {
	struct k_sem lock;
	struct device *dma_dev;
	struct phy_audio_device *dac; /* physical DAC device */
	struct phy_audio_device *i2stx; /* physical I2STX device */
	struct phy_audio_device *spdiftx; /* physical SPDIFTX device */
	u8_t pa_session_indicator : 1; /* indicates under the pa session operation stage */
};

/*
 * struct aout_config_data
 * @brief audio out config data which are the hardward related
 */
struct aout_config_data {
	u8_t dac_reset_id; /* DAC reset ID */
	u8_t i2stx_reset_id; /* I2S TX reset ID */
	u8_t spdiftx_reset_id; /* SPIDF TX reset ID */
};

static aout_session_t aout_sessions[AOUT_SESSION_MAX];

/*
 * @brief checkout if there is the same channel within audio out sessions
 */
static bool aout_session_check(u16_t type)
{
	int i;
	aout_session_t *sess = aout_sessions;

	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if (AOUT_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AOUT_SESSION_OPEN)
			&& (sess->channel_type & type)) {
			SYS_LOG_ERR("Audio channel %d is already in use", type);
			return true;
		}
	}

	return false;
}

/*
 * @brief Get audio out session by specified channel
 */
static aout_session_t *aout_session_get(u16_t type)
{
	aout_session_t *sess = aout_sessions;
	int i;
	if (aout_session_check(type))
		return NULL;

	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if (!(sess->flags & AOUT_SESSION_OPEN)) {
			memset(sess, 0, sizeof(aout_session_t));
			sess->magic = AOUT_SESSION_MAGIC;
			sess->flags = AOUT_SESSION_OPEN;
			sess->channel_type = type;
			return sess;
		}
	}

	return NULL;
}

/*
 * @brief Put audio out session by given session address
 */
static void aout_session_put(aout_session_t *s)
{
	aout_session_t *sess = aout_sessions;
	int i;
	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if ((u32_t)s == (u32_t)sess) {
			memset(s, 0, sizeof(aout_session_t));
			break;
		}
	}
}

/* @brief audio out session lock */
static inline void audio_out_lock(struct device *dev)
{
	struct aout_drv_data *data = dev->driver_data;
	k_sem_take(&data->lock, K_FOREVER);
}

/* @brief audio out session unlock */
static inline void audio_out_unlock(struct device *dev)
{
	struct aout_drv_data *data = dev->driver_data;
	k_sem_give(&data->lock);
}

#if defined(CONFIG_AUDIO_OUT_DAC_SUPPORT) || defined(CONFIG_AUDIO_POWERON_OPEN_I2STX)
static int acts_audio_dummy_callback(void *cb_data, u32_t reason)
{
	ARG_UNUSED(cb_data);
	ARG_UNUSED(reason);

	return 0;
}
#endif

#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
static int acts_audio_wait_pa_session(struct device *dev, u32_t timeout_ms)
{
	struct aout_drv_data *data = dev->driver_data;
	u32_t start_time, curr_time;

	start_time = k_uptime_get();
	while (data->pa_session_indicator) {
		curr_time = k_uptime_get_32();
		if ((curr_time - start_time) >= timeout_ms) {
			SYS_LOG_ERR("wait pa session timeout");
			return -ETIMEDOUT;
		}
		k_sleep(2);
	}

	return 0;
}

static void *acts_audio_open_pa_session(struct device *dev)
{
	aout_param_t aout_setting = {0};
	dac_setting_t dac_setting = {0};
	void *handle;

	aout_setting.sample_rate = SAMPLE_RATE_48KHZ;
	aout_setting.channel_type = AUDIO_CHANNEL_DAC;
	aout_setting.channel_width = CHANNEL_WIDTH_16BITS;
	aout_setting.callback = acts_audio_dummy_callback;
	aout_setting.cb_data = (void *)dev;
	dac_setting.channel_mode = MONO_MODE;
	aout_setting.dac_setting = &dac_setting;

	handle = audio_out_open(dev, &aout_setting);

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	struct aout_drv_data *data = dev->driver_data;
	if (handle)
		data->pa_session_indicator = 1;
#endif

	return handle;
}

static void acts_audio_close_pa_session(struct device *dev, void *hdl)
{
	audio_out_close(dev, hdl);
#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	struct aout_drv_data *data = dev->driver_data;
	data->pa_session_indicator = 0;
#endif
}

static int acts_audio_open_pa(struct device *dev)
{
	void *pa_session = NULL;
	u32_t pcm_data = 0;
	u8_t i = 0;

	pa_session = acts_audio_open_pa_session(dev);
	if (!pa_session) {
		SYS_LOG_ERR("Failed to open pa session");
		return -ENXIO;
	}

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	board_extern_pa_ctl(CONFIG_EXTERN_PA_CLASS, true);
#endif

	/* FIXME: it is necessary to send some samples(zero data) when startup
	*   to avoid the noise with probability.
	*/
	for (i = 0; i < 16; i++)
		audio_out_write(dev, pa_session, (u8_t *)&pcm_data, 4);

	acts_audio_close_pa_session(dev, pa_session);

	SYS_LOG_INF("Open PA successfully");

	return 0;
}

static int acts_audio_close_pa(struct device *dev)
{
	void *pa_session = NULL;
	struct aout_drv_data *data = dev->driver_data;

	pa_session = acts_audio_open_pa_session(dev);
	if (!pa_session) {
		SYS_LOG_ERR("Failed to open pa session");
		return -ENXIO;
	}

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	board_extern_pa_ctl(CONFIG_EXTERN_PA_CLASS, false);
#endif

	acts_audio_close_pa_session(dev, pa_session);

	phy_audio_control(data->dac, AOUT_CMD_CLOSE_PA, NULL);

	SYS_LOG_INF("Close PA successfully");

	return 0;
}
#endif

static int acts_audio_pa_class_select(struct device *dev, u8_t pa_class)
{
	ARG_UNUSED(dev);
#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	return board_extern_pa_ctl(pa_class, true);
#else
	return -1;
#endif
}

#ifdef CONFIG_AUDIO_POWERON_OPEN_I2STX
static int acts_audio_open_i2stx_dev(struct device *dev)
{
	aout_param_t aout_setting = {0};
	i2stx_setting_t i2stx_setting = {0};
	void *handle;

	aout_setting.sample_rate = SAMPLE_RATE_48KHZ;
	aout_setting.channel_type = AUDIO_CHANNEL_I2STX;
	aout_setting.channel_width = CHANNEL_WIDTH_16BITS;
	aout_setting.callback = acts_audio_dummy_callback;
	aout_setting.cb_data = (void *)dev;
	i2stx_setting.mode = I2S_MASTER_MODE;
	aout_setting.i2stx_setting = &i2stx_setting;

	handle = audio_out_open(dev, &aout_setting);

	/* close interface will not disable i2stx truly*/
	audio_out_close(dev, handle);

	return 0;
}

static int acts_audio_close_i2stx_dev(struct device *dev)
{
	struct aout_drv_data *data = dev->driver_data;
	phy_audio_control(data->i2stx, AOUT_CMD_CLOSE_I2STX_DEVICE, NULL);

	return 0;
}
#endif

static int audio_out_init(struct device *dev)
{
	const struct aout_config_data *cfg = dev->config->config_info;
	struct aout_drv_data *data = dev->driver_data;

	k_sem_init(&data->lock, 1, 1);

	data->dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (!data->dma_dev) {
		SYS_LOG_ERR("Bind DMA device %s error", CONFIG_DMA_0_NAME);
		return -ENOENT;
	}

	data->dac = GET_AUDIO_DEVICE(dac);
	if (!data->dac) {
		SYS_LOG_WRN("No physical DAC device");
	} else {
		if (phy_audio_init(data->dac, dev)) {
			SYS_LOG_ERR("Failed to probe PHY DAC");
			return -ENXIO;
		}
	}

	data->i2stx = GET_AUDIO_DEVICE(i2stx);
	if (!data->i2stx) {
		SYS_LOG_WRN("No physical I2STX device");
	} else {
		if (phy_audio_init(data->i2stx, dev)) {
			SYS_LOG_ERR("Failed to probe PHY I2STX");
			return -ENXIO;
		}
	}

	data->spdiftx = GET_AUDIO_DEVICE(spdiftx);
	if (!data->spdiftx) {
		SYS_LOG_WRN("No physical SPDIFTX device");
	} else {
		if (phy_audio_init(data->spdiftx, dev)) {
			SYS_LOG_ERR("Failed to probe PHY SPDIFTX");
			return -ENXIO;
		}
	}

	/* Set DAC/I2STX/SPDIFTX controller to be normal
	 * Because the DAC FIFO and I2STX FIFO will be used commonly, just to reset by default.
	 */
	acts_reset_peripheral(cfg->dac_reset_id);
	acts_reset_peripheral(cfg->i2stx_reset_id);

#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
	acts_reset_peripheral(cfg->spdiftx_reset_id);
#endif

	/* clear pa session indicator */
	data->pa_session_indicator = 0;

	/* open internal and external pa */
#ifdef CONFIG_AUDIO_POWERON_OPEN_PA
	acts_audio_open_pa(dev);
#endif

#ifdef CONFIG_AUDIO_POWERON_OPEN_I2STX
	acts_audio_open_i2stx_dev(dev);
#endif

	return 0;
}

/* @brief Enable the DAC channel */
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
static int audio_out_enable_dac(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	u8_t channel_type = param->channel_type;
	struct aout_drv_data *data = dev->driver_data;
	struct phy_audio_device *dac = data->dac;
	dac_setting_t *dac_setting = param->dac_setting;

	if (!dac) {
		SYS_LOG_ERR("Physical DAC device is not esixted");
		return -ENXIO;
	}

	if (!dac_setting) {
		SYS_LOG_ERR("DAC setting is NULL");
		return -EINVAL;
	}

	/* Check the DAC FIFO usage */
	if (param->outfifo_type != AOUT_FIFO_DAC0) {
		SYS_LOG_ERR("Channel type invalid %d", param->outfifo_type);
		return -EINVAL;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		SYS_LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	/* Linkage with I2STX */
	if (channel_type & AUDIO_CHANNEL_I2STX)
		phy_audio_enable(data->i2stx, (void *)param);

	/* Linkage with SPDIFTX */
	if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
		phy_audio_enable(data->spdiftx, (void *)param);
		phy_audio_control(dac, PHY_CMD_CLAIM_WITH_128FS, NULL);
	}

	return phy_audio_enable(dac, (void *)param);
}
#endif

/* @brief Claim the use of the DAC/I2STX FIFO */
#if defined(CONFIG_AUDIO_OUT_I2STX0_SUPPORT) || defined(CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT)
static int audio_out_fifo_request(struct phy_audio_device *phy_dev, aout_param_t *param)
{
	if (!phy_dev)
		return -EINVAL;

	if (phy_audio_control(phy_dev, PHY_CMD_FIFO_GET, param)) {
		SYS_LOG_ERR("The request FIFO is busy");
		return -EBUSY;
	}

	return 0;
}

/* @brief Release the use of the DAC/I2STX FIFO */
static int audio_out_fifo_release(struct phy_audio_device *phy_dev)
{
	if (!phy_dev)
		return -EINVAL;

	if (phy_audio_control(phy_dev, PHY_CMD_FIFO_PUT, NULL)) {
		SYS_LOG_ERR("Release DAC FIFO failed");
		return -EFAULT;
	}

	return 0;
}
#endif

/* @brief Enable the I2STX channel */
#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
static int audio_out_enable_i2stx(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	u8_t channel_type = param->channel_type;
	struct aout_drv_data *data = dev->driver_data;
	struct phy_audio_device *i2stx = data->i2stx;
	i2stx_setting_t *i2stx_setting = param->i2stx_setting;
	int ret;

	if (!i2stx) {
		SYS_LOG_ERR("Physical I2STX device is not esixted");
		return -ENXIO;
	}

	if (!i2stx_setting) {
		SYS_LOG_ERR("I2STX setting is NULL");
		return -EINVAL;
	}

	/* Check the DAC FIFO usage */
	if ((param->outfifo_type != AOUT_FIFO_DAC0)
		&& (param->outfifo_type != AOUT_FIFO_I2STX0)) {
		SYS_LOG_ERR("Channel type invalid %d", param->outfifo_type);
		return -EINVAL;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)
		&& (param->channel_width != CHANNEL_WIDTH_20BITS)) {
		SYS_LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	if (AOUT_FIFO_DAC0 == param->outfifo_type) {
		if (audio_out_fifo_request(data->dac, param)) {
			SYS_LOG_ERR("Failed to take DAC FIFO");
			return -ENXIO;
		}
	}

	/* Linkage with SPDIFTX */
	if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
		phy_audio_enable(data->spdiftx, param);
		phy_audio_control(data->dac, PHY_CMD_CLAIM_WITH_128FS, NULL);
	}

	ret = phy_audio_enable(i2stx, (void *)param);
	if (ret) {
		if (AOUT_FIFO_DAC0 == param->outfifo_type)
			audio_out_fifo_release(data->dac);
	}

	return ret;
}
#endif

/* @brief Enable the SPDIFTX channel */
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
static int audio_out_enable_spdiftx(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	struct aout_drv_data *data = dev->driver_data;
	struct phy_audio_device *spdiftx = data->spdiftx;
	spdiftx_setting_t *spdiftx_setting = param->spdiftx_setting;
	audio_dma_width_e width;
	int ret;

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		SYS_LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	if (param->channel_width == CHANNEL_WIDTH_16BITS)
		width = DMA_WIDTH_16BITS;
	else
		width = DMA_WIDTH_32BITS;

	if (!spdiftx) {
		SYS_LOG_ERR("Physical SPDIFTX device is not esixted");
		return -EFAULT;
	}

	if (!spdiftx_setting) {
		SYS_LOG_ERR("SPDIFTX setting is NULL");
		return -EINVAL;
	}

	/* Check the DAC FIFO usage */
	if ((param->outfifo_type != AOUT_FIFO_DAC0)
		&& (param->outfifo_type != AOUT_FIFO_I2STX0)) {
		SYS_LOG_ERR("Channel type invalid %d", param->outfifo_type);
		return -EINVAL;
	}

	if (AOUT_FIFO_DAC0 == param->outfifo_type) {
		if (audio_out_fifo_request(data->dac, param)) {
			SYS_LOG_ERR("Failed to take DAC FIFO");
			return -ENXIO;
		}
	} else if (AOUT_FIFO_I2STX0 == param->outfifo_type) {
		if (audio_out_fifo_request(data->i2stx, param)) {
			SYS_LOG_ERR("Failed to take I2STX FIFO");
			return -ENXIO;
		}
	}

	phy_audio_control(data->dac, PHY_CMD_CLAIM_WITH_128FS, NULL);

	ret = phy_audio_enable(spdiftx, (void *)param);
	if (ret) {
		if (AOUT_FIFO_DAC0 == param->outfifo_type)
			audio_out_fifo_release(data->dac);
		else if (AOUT_FIFO_I2STX0 == param->outfifo_type)
			audio_out_fifo_release(data->i2stx);
	}

	return ret;
}
#endif

/* @brief DMA irq callback on direct method */
static void audio_out_dma_direct(struct device *dev, u32_t priv_data, int reason)
{
	aout_session_t *session = (aout_session_t *)priv_data;
	u32_t key;
	ARG_UNUSED(dev);

	if (session && AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		/* Only notify when DMA transfer completly */
		if ((reason == DMA_IRQ_TC) && session->callback) {
			key = irq_lock();
			session->callback(session->cb_data, AOUT_DMA_IRQ_TC);
			irq_unlock(key);
		}
	}
}

/* @brief DMA irq callback on reload method */
static void audio_out_dma_reload(struct device *dev, u32_t priv_data, int reason)
{
	aout_session_t *session = (aout_session_t *)priv_data;
	//struct aout_drv_data *data = dev->driver_data;
	u32_t ret_reson;
	int ret;

	ARG_UNUSED(dev);

	if (session && AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		if (session->callback) {
			if (reason == DMA_IRQ_HF) {
				ret_reson = AOUT_DMA_IRQ_HF;
			} else if (reason == DMA_IRQ_TC){
				ret_reson = AOUT_DMA_IRQ_TC;
			} else {
				SYS_LOG_ERR("Unknown DMA reson %d", reason);
				dma_stop(dev, session->dma_chan);
				return ;
			}
			ret = session->callback(session->cb_data, ret_reson);
			if (ret < 0)
				dma_stop(dev, session->dma_chan);
		}
	}
}

/* @brief prepare the DMA configuration for the audio out transfer */
static int audio_out_dma_prepare(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	struct aout_drv_data *data = dev->driver_data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	/* request dma channel handle */
	session->dma_chan = dma_request(data->dma_dev, 0xFF);
	if (!session->dma_chan) {
		SYS_LOG_ERR("Failed to request dma channel");
		return -ENXIO;
	}

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_burst_length = 8;
	dma_cfg.dest_burst_length = 8;

	dma_block_cfg.source_reload_en = session->reload_en;

	dma_cfg.head_block = &dma_block_cfg;

	if (CHANNEL_WIDTH_16BITS == param->channel_width)
		dma_cfg.source_data_size = 2;
	else
		dma_cfg.source_data_size = 4;

	if (AOUT_FIFO_DAC0 == session->outfifo_type) {
		dma_cfg.dma_slot = DMA_ID_AUDIO_DAC_FIFO;
		/* DAC FIFO uses the PCM BUF irq to notify the status of transfer */
	} else if (AOUT_FIFO_I2STX0 == session->outfifo_type) {
		dma_cfg.dma_slot = DMA_ID_AUDIO_I2S;
		/* I2STX FIFO uses the DMA irq to notify the status of transfer */
		if (session->callback) {
			if (session->reload_en)
				dma_cfg.dma_callback = audio_out_dma_reload;
			else
				dma_cfg.dma_callback = audio_out_dma_direct;
			dma_cfg.callback_data = session;
			dma_cfg.complete_callback_en = 1;
			dma_cfg.half_complete_callback_en = 1;
		}
	}

	if (dma_config(data->dma_dev, session->dma_chan, &dma_cfg)) {
		SYS_LOG_ERR("DMA config error");
		return -EFAULT;
	}

	if (session->reload_en) {
		dma_reload(data->dma_dev, session->dma_chan,
			(u32_t)session->reload_addr, 0, session->reload_len);
	}

	return 0;
}

/* @brief audio out open and return the session handler */
static void *acts_audio_out_open(struct device *dev, aout_param_t *param)
{
	aout_session_t *session = NULL;
	u8_t channel_type;
	int ret;

	if (!param) {
		SYS_LOG_ERR("NULL parameter");
		return NULL;
	}

	/* There is a 120ms yield scheduer under extern pa operation and need to wait until the pa session close */
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	if (acts_audio_wait_pa_session(dev, AOUT_SESSION_WAIT_PA_TIMEOUT))
		return NULL;
#endif

	audio_out_lock(dev);

	channel_type = param->channel_type;
	session = aout_session_get(channel_type);
	if (!session) {
		SYS_LOG_ERR("Failed to get audio session (type:%d)", channel_type);
		audio_out_unlock(dev);
		return NULL;
	}

	if (!param->callback) {
		SYS_LOG_ERR("Channel callback is NULL");
		goto err;
	}

	session->outfifo_type = param->outfifo_type;
	session->sample_rate = param->sample_rate;
	session->channel_type = channel_type;
	session->callback = param->callback;
	session->cb_data = param->cb_data;

	if (param->reload_setting) {
		if ((!param->reload_setting->reload_addr)
			|| (!param->reload_setting->reload_len)) {
			SYS_LOG_ERR("Invalid reload parameter addr:0x%x, len:0x%x",
				(u32_t)param->reload_setting->reload_addr,
				param->reload_setting->reload_len);
			goto err;
		}
		session->reload_en = 1;
		session->reload_addr = param->reload_setting->reload_addr;
		session->reload_len = param->reload_setting->reload_len;
		SYS_LOG_INF("In reload mode [0x%08x %d]",
			(u32_t)session->reload_addr, session->reload_len);
	} else {
		session->reload_en = 0;
	}

	if (channel_type & AUDIO_CHANNEL_DAC) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
		ret = audio_out_enable_dac(dev, session, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_I2STX) {
#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
		ret = audio_out_enable_i2stx(dev, session, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
		ret = audio_out_enable_spdiftx(dev, session, param);
#else
		ret = -ENXIO;
#endif
	} else {
		SYS_LOG_ERR("Invalid channel type %d", channel_type);
		goto err;
	}

	if (ret) {
		SYS_LOG_ERR("Enable channel type %d error:%d", channel_type, ret);
		goto err;
	}

	ret = audio_out_dma_prepare(dev, session, param);
	if (ret) {
		SYS_LOG_ERR("prepare session dma error %d", ret);
		goto err;
	}

	session->flags |=  AOUT_SESSION_CONFIG;

	SYS_LOG_INF("channel#%p type:%d fifo:%d sr:%d opened",
				session, channel_type,
				param->outfifo_type, param->sample_rate);

	audio_out_unlock(dev);
	return (void *)session;
err:
	audio_out_unlock(dev);
	aout_session_put(session);
	return NULL;
}

/* @brief Disable the DAC channel */
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
static int audio_out_disable_dac(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->driver_data;
	struct phy_audio_device *dac = data->dac;

	if (!dac) {
		SYS_LOG_ERR("Physical DAC device is not esixted");
		return -EFAULT;
	}

	if (session->channel_type & AUDIO_CHANNEL_SPDIFTX)
		phy_audio_control(dac, PHY_CMD_CLAIM_WITHOUT_128FS, NULL);

	return phy_audio_disable(dac);
}
#endif

/* @brief Disable the I2STX channel */
#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
static int audio_out_disable_i2stx(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->driver_data;
	struct phy_audio_device *i2stx = data->i2stx;

	if (!i2stx) {
		SYS_LOG_ERR("Physical I2STX device is not esixted");
		return -EFAULT;
	}

	if (AOUT_FIFO_DAC0 == session->outfifo_type)
		audio_out_fifo_release(data->dac);

	if (session->channel_type & AUDIO_CHANNEL_SPDIFTX)
		phy_audio_control(data->dac, PHY_CMD_CLAIM_WITHOUT_128FS, NULL);

	return phy_audio_disable(i2stx);
}
#endif

/* @brief Disable the SPDIFTX channel */
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
static int audio_out_disable_spdiftx(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->driver_data;
	struct phy_audio_device *spdiftx = data->spdiftx;

	if (!spdiftx) {
		SYS_LOG_ERR("Physical SPDIFTX device is not esixted");
		return -EFAULT;
	}

	if (AOUT_FIFO_DAC0 == session->outfifo_type) {
		audio_out_fifo_release(data->dac);
	} else if (AOUT_FIFO_I2STX0 == session->outfifo_type) {
		audio_out_fifo_release(data->i2stx);
	}

	phy_audio_control(data->dac, PHY_CMD_CLAIM_WITHOUT_128FS, NULL);

	return phy_audio_disable(spdiftx);
}
#endif

/* @brief Close audio out channel by specified session handler */
static int acts_audio_out_close(struct device *dev, void *handle)
{
	struct aout_drv_data *data = dev->driver_data;
	u8_t channel_type;
	int ret;
	aout_session_t *session = (aout_session_t *)handle;

	if (!handle) {
		SYS_LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	audio_out_lock(dev);

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_ERR("Session magic error:0x%x", session->magic);
		ret = -EFAULT;
		goto out;
	}

	channel_type = session->channel_type;
	if (channel_type & AUDIO_CHANNEL_DAC) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
		ret = audio_out_disable_dac(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_I2STX) {
#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
		ret = audio_out_disable_i2stx(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
		ret = audio_out_disable_spdiftx(dev, session);
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

	aout_session_put(session);

out:
	audio_out_unlock(dev);
	return ret;
}

static int acts_audio_out_control(struct device *dev, void *handle, int cmd, void *param)
{
#if defined(CONFIG_AUDIO_OUT_DAC_SUPPORT) \
			|| defined(CONFIG_AUDIO_OUT_I2STX0_SUPPORT) \
			|| defined(CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT)
	struct aout_drv_data *data = dev->driver_data;
#endif
	aout_session_t *session = (aout_session_t *)handle;
	u8_t channel_type = session->channel_type;
	int ret;

	SYS_LOG_DBG("session#0x%x cmd 0x%x", (u32_t)handle, cmd);

#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	if (AOUT_CMD_OPEN_PA == cmd)
		return acts_audio_open_pa(dev);

	if (AOUT_CMD_CLOSE_PA == cmd)
		return acts_audio_close_pa(dev);
#endif

	if (AOUT_CMD_PA_CLASS_SEL == cmd)
		return acts_audio_pa_class_select(dev, *(u8_t *)param);

#ifdef CONFIG_AUDIO_POWERON_OPEN_I2STX
	if (AOUT_CMD_OPEN_I2STX_DEVICE == cmd)
		return acts_audio_open_i2stx_dev(dev);

	if (AOUT_CMD_CLOSE_I2STX_DEVICE == cmd)
		return acts_audio_close_i2stx_dev(dev);
#endif

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

	/* In the case of the commands according to the FIFO attribute */
	if (AOUT_IS_FIFO_CMD(cmd)) {
		if (AOUT_FIFO_DAC0 == session->outfifo_type) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
			ret = phy_audio_control(data->dac, cmd, param);
#else
			ret = -ENXIO;
#endif
		} else if (AOUT_FIFO_I2STX0 == session->outfifo_type) {
#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
			ret = phy_audio_control(data->i2stx, cmd, param);
#else
			ret = -ENXIO;
#endif
		} else {
			ret = -ENXIO;
		}
		goto out;
	}

	if (channel_type & AUDIO_CHANNEL_DAC) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
		ret = phy_audio_control(data->dac, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_I2STX) {
#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
		ret = phy_audio_control(data->i2stx, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
		ret = phy_audio_control(data->spdiftx, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else {
		SYS_LOG_ERR("Invalid channel type %d", channel_type);
		ret = -EINVAL;
	}

out:
	return ret;
}

static int acts_audio_out_start(struct device *dev, void *handle)
{
	struct aout_drv_data *data = dev->driver_data;
	aout_session_t *session = (aout_session_t *)handle;

	if (!handle) {
		SYS_LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

	/* In DMA reload mode only start one time is enough */
	if (session->reload_en
		&& (session->flags & AOUT_SESSION_START)) {
		return 0;
	}

	if (!(session->flags & AOUT_SESSION_START)) {
		if (session->channel_type == AUDIO_CHANNEL_I2STX) {
			phy_audio_control(data->i2stx, PHY_CMD_CHANNEL_START, NULL);
		}
		session->flags |= AOUT_SESSION_START;
	}

	return dma_start(data->dma_dev, session->dma_chan);
}

/* @brief wait dac FIFO empty */
static int wait_dacfifo_empty(struct device *dev)
{
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	struct aout_drv_data *data = dev->driver_data;
	int ret;

	/* TODO add timeout mechanism */
	ret = phy_audio_control(data->dac, PHY_CMD_DAC_WAIT_EMPTY, NULL);
	if (ret) {
		SYS_LOG_ERR("wait empty error");
	}

	return ret;
#else
	return -ENXIO;
#endif
}

/* @brief Write the data to the audio output channel */
static int acts_audio_out_write(struct device *dev, void *handle, u8_t *buffer, u32_t length)
{
	struct aout_drv_data *data = dev->driver_data;
	aout_session_t *session = (aout_session_t *)handle;
	int ret;

	if (!handle || !buffer || !length) {
		SYS_LOG_ERR("Invalid parameters (%p, %p, %d)", handle, buffer, length);
		return -EINVAL;
	}

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

	if (session->reload_en) {
		SYS_LOG_INF("Reload mode can start directly");
		return 0;
	}

	if (!(session->flags & AOUT_SESSION_CONFIG)) {
		SYS_LOG_ERR("session not conifg");
		return -EPERM;
	}

	ret = dma_reload(data->dma_dev, session->dma_chan, (u32_t)buffer, 0, length);
	if (ret) {
		SYS_LOG_ERR("dma reload error %d", ret);
		return ret;
	}

	ret = audio_out_start(dev, handle);
	if (ret)
		SYS_LOG_ERR("dma start error %d", ret);

	if (AOUT_FIFO_DAC0 == session->outfifo_type) {
		ret = dma_wait_timeout(data->dma_dev, session->dma_chan, AOUT_SESSION_DMA_TIMEOUT_US);
		if (ret) {
			SYS_LOG_ERR("wait dma error %d", ret);
			return -EIO;
		}
		wait_dacfifo_empty(dev);
	}

	return ret;
}

static int acts_audio_out_stop(struct device *dev, void *handle)
{
	struct aout_drv_data *data = dev->driver_data;
	aout_session_t *session = (aout_session_t *)handle;
	int ret = 0;

	if (session && AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		SYS_LOG_INF("session#%p, audio out stop", session);
		ret = dma_stop(data->dma_dev, session->dma_chan);
	}

	return ret;
}

static struct aout_drv_data audio_out_drv_data;

static struct aout_config_data audio_out_config_data = {
	.dac_reset_id = RESET_ID_AUDIO_DAC,
	.i2stx_reset_id = RESET_ID_I2S0_TX,
	.spdiftx_reset_id = RESET_ID_SPDIF_TX,
};

const struct aout_driver_api aout_drv_api = {
	.aout_open = acts_audio_out_open,
	.aout_close = acts_audio_out_close,
	.aout_control = acts_audio_out_control,
	.aout_start = acts_audio_out_start,
	.aout_write = acts_audio_out_write,
	.aout_stop = acts_audio_out_stop
};

DEVICE_AND_API_INIT(audio_out, CONFIG_AUDIO_OUT_ACTS_DEV_NAME, audio_out_init,
	    &audio_out_drv_data, &audio_out_config_data,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &aout_drv_api);
