/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PWM controller driver for Actions SoC
 */
#include <errno.h>
#include <string.h>
#include <kernel.h>
#include <board.h>
#include <pwm.h>
#include <gpio.h>
#include <device.h>
#include <soc.h>
#include <dma.h>
#include <board.h>

#define SYS_LOG_DOMAIN "PWM"
#include <logging/sys_log.h>

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define CMU_PWMCLK_CLKSEL_SHIFT		(9)
#define CMU_PWMCLK_CLKSEL_MASK		(0x3 << CMU_PWMCLK_CLKSEL_SHIFT)
#define CMU_PWMCLK_CLK_SEL(x)		((x) << CMU_PWMCLK_CLKSEL_SHIFT)
#define CMU_PWMCLK_CLKSEL_32K		CMU_PWMCLK_CLK_SEL(0)
#define CMU_PWMCLK_CLKSEL_HOSC		CMU_PWMCLK_CLK_SEL(1)
#define CMU_PWMCLK_CLKSEL_CK64M		CMU_PWMCLK_CLK_SEL(2)

#define CMU_PWMCLK_CLKDIV_SHIFT		(0)
#define CMU_PWMCLK_CLKDIV(x)		((x) << CMU_PWMCLK_CLKDIV_SHIFT)
#define CMU_PWMCLK_CLKDIV_MASK		CMU_PWMCLK_CLKDIV(0x1FF)

#define PWM_CTRL_MODE_SEL_SHIFT		(0)
#define PWM_CTRL_MODE_SEL_MASK      (0x3 << PWM_CTRL_MODE_SEL_SHIFT)
#define PWM_CTRL_MODE_SEL(x)		((x) << PWM_CTRL_MODE_SEL_SHIFT)
#define PWM_CTRL_MODE_SEL_FIXED		PWM_CTRL_MODE_SEL(0)
#define PWM_CTRL_MODE_SEL_BREATH	PWM_CTRL_MODE_SEL(1)
#define PWM_CTRL_MODE_SEL_PROGRAM	PWM_CTRL_MODE_SEL(2)
#define PWM_CTRL_POL_SEL_HIGH		BIT(2)
#define PWM_CTRL_CHANNEL_EN			BIT(3)
#define PWM_CTRL_CHANNEL_START		BIT(4)

#define PWM_BREATH_MODE_C_SHIFT		(0)
#define PWM_BREATH_MODE_C_MASK		(0xFF << PWM_BREATH_MODE_C_SHIFT)
#define PWM_BREATH_MODE_C(x)		((x) << PWM_BREATH_MODE_C_SHIFT)

#define PWM_BREATH_MODE_QU_SHIFT	(0)
#define PWM_BREATH_MODE_QU_MASK		(0xFF << PWM_BREATH_MODE_QU_SHIFT)
#define PWM_BREATH_MODE_QU(x)		((x) << PWM_BREATH_MODE_QU_SHIFT)
#define PWM_BREATH_MODE_QD_SHIFT	(16)
#define PWM_BREATH_MODE_QD_MASK		(0xFF << PWM_BREATH_MODE_QD_SHIFT)
#define PWM_BREATH_MODE_QD(x)		((x) << PWM_BREATH_MODE_QD_SHIFT)

#define PWM_BREATH_MODE_H_SHIFT		(0)
#define PWM_BREATH_MODE_H_MASK		(0xFFFF << PWM_BREATH_MODE_H_SHIFT)
#define PWM_BREATH_MODE_H(x)		((x) << PWM_BREATH_MODE_H_SHIFT)

#define PWM_BREATH_MODE_L_SHIFT		(0)
#define PWM_BREATH_MODE_L_MASK		(0xFFFF << PWM_BREATH_MODE_L_SHIFT)
#define PWM_BREATH_MODE_L(x)		((x) << PWM_BREATH_MODE_L_SHIFT)

#define PWM_DUTYMAX_SHIFT			(0)
#define PWM_DUTYMAX_MASK			(0xFFFF << PWM_DUTYMAX_SHIFT)
#define PWM_DUTYMAX(x)				((x) << PWM_DUTYMAX_SHIFT)

#define PWM_DUTY_SHIFT				(0)
#define PWM_DUTY_MASK				(0xFFFF << PWM_DUTY_SHIFT)
#define PWM_DUTY(x)					((x) << PWM_DUTY_SHIFT)

#define PWM_DMA_CTL_START			BIT(0)
#define PWM_FIFO_CLK_SEL_SHIFT		(4)
#define PWM_FIFO_CLK_SEL_MASK		(0xF << PWM_FIFO_CLK_SEL_SHIFT)
#define PWM_FIFO_CLK_SEL(x)			((x) << PWM_FIFO_CLK_SEL_SHIFT)

#define PWM_FIFOSTA_ERROR			BIT(0)
#define PWM_FIFOSTA_FULL			BIT(1)
#define PWM_FIFOSTA_EMPTY			BIT(2)
#define PWM_FIFOSTA_LEVEL_SHIFT		(3)
#define PWM_FIFOSTA_LEVEL_MASK		(0x3 << PWM_FIFOSTA_LEVEL_SHIFT)

#define PWM_CLK_CYCLES_PER_SEC		(32000)
#define PWM_PIN_CYCLES_PER_SEC		(8000)
#define PWM_PIN_CLK_PERIOD_USEC		(1000000UL / PWM_PIN_CYCLES_PER_SEC)
#define PWM_DUTYMAX_DEFAULT			(16000) /* 1s / PWM_PIN_CYCLES_PER_SEC x PWM_DUTYMAX_DEFAULT = 2s */

#define PWM_PROGRAM_PIN_INVALID		(0xFF)

#define PWM_BREATH_MODE_DEFAULT_C	(32)

#define PWM_DMACTL_REG_OFFSET		(0x0F00)
#define PWM_DMACTL_REG(x)			((struct acts_pwm_dmactl *)((x)->base + PWM_DMACTL_REG_OFFSET))

/* pwm control registers */
struct acts_pwm_chan {
	volatile u32_t ctrl;
	volatile u32_t c;
	volatile u32_t q;
	volatile u32_t h;
	volatile u32_t l;
	volatile u32_t duty_max;
	volatile u32_t duty;
};

/* pwm dma control registers */
struct acts_pwm_dmactl {
	volatile u32_t dmactl;
	volatile u32_t fifodat;
	volatile u32_t fifosta;
};

struct pwm_acts_data {
	struct k_mutex mutex;
	struct device *dma_dev;
	int dma_chan;
	int (*program_callback)(void *cb_data, u8_t reason);
	void *cb_data;
	u8_t program_pin;
};

struct pwm_acts_config {
	u32_t	base;
	u32_t	pwmclk_reg;
	u32_t	pwm_pin_cycles;
	u32_t	num_chans;
	u8_t	clock_id;
	u8_t	reset_id;
};

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct pwm_acts_config * const)(dev)->config->config_info)

#define PWM_CHAN(base, chan_id)							\
	((struct acts_pwm_chan *)((base) + (chan_id * 0x100)))

const struct acts_pin_config board_led_pin_config[] = {
#ifdef BOARD_PWM_MAP
    BOARD_PWM_MAP
#endif
};

const led_map pwmledmaps[] = {
#ifdef BOARD_LED_MAP
    BOARD_LED_MAP
#endif
};

static int pwm_acts_set_pinmux(struct device *dev, u32_t chan)
{
	int i, j;
    for (i = 0; i < ARRAY_SIZE(board_led_pin_config); i++) {
		for (j = 0; j < ARRAY_SIZE(pwmledmaps); j++) {
			if ((chan == pwmledmaps[j].led_pwm)
				&& (board_led_pin_config[i].pin_num == pwmledmaps[j].led_pin)) {
				acts_pinmux_set(board_led_pin_config[i].pin_num,
					board_led_pin_config[i].mode | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3));
				return 0;
			}
		}
    }

	if (i == ARRAY_SIZE(board_led_pin_config)) {
		SYS_LOG_ERR("PWM@%d not in board map table", chan);
		return -ENOENT;
	}

	return 0;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * period_cycles: Period (in timer count)
 * pulse_cycles: Pulse width (in timer count).
 * @param flags Flags for pin configuration (polarity).
 * return 0, or negative errno code
 */
static int pwm_acts_pin_set(struct device *dev, u32_t chan,
			     u32_t period_cycles, u32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	struct pwm_acts_data *data = dev->driver_data;
	struct acts_pwm_chan *pwm_chan;

	SYS_LOG_DBG("PWM@%d set period cycles %d, pulse cycles %d",
		chan, period_cycles, pulse_cycles);

	if (chan >= cfg->num_chans) {
		SYS_LOG_ERR("invalid chan %d", chan);
		return -EINVAL;
	}

	if (pulse_cycles > period_cycles) {
		SYS_LOG_ERR("pulse cycles %d is biger than period's %d",
			pulse_cycles, period_cycles);
		return -EINVAL;
	}

	if (period_cycles > PWM_CYCLES_MAX || period_cycles < 1) {
		SYS_LOG_ERR("period cycles invalid %d (max %d min 1)",
			period_cycles, PWM_CYCLES_MAX);
		return -EINVAL;
	}

	if (pwm_acts_set_pinmux(dev, chan)) {
		SYS_LOG_DBG("set pwm pinmux error");
		return -EFAULT;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	pwm_chan = PWM_CHAN(cfg->base, chan);

	SYS_LOG_DBG("pwm_chan %p", pwm_chan);

	/* disable pwm */
	pwm_chan->ctrl = 0;

	/* setup pwm parameters */
	if (START_VOLTAGE_HIGH == flags)
		pwm_chan->ctrl = PWM_CTRL_POL_SEL_HIGH | PWM_CTRL_MODE_SEL_FIXED;
	else
		pwm_chan->ctrl = PWM_CTRL_MODE_SEL_FIXED;

	/* PWM period = PWM_CLK period x PWM_DUTYMAX */
	pwm_chan->duty_max = PWM_DUTYMAX(period_cycles);

	/* PWM  DUTY = DUTY / PWM_DUTYMAX */
	pwm_chan->duty = pulse_cycles * (pwm_chan->duty_max & PWM_DUTYMAX_MASK) / period_cycles;

	/* enable pwm */
	pwm_chan->ctrl |= (PWM_CTRL_CHANNEL_EN | PWM_CTRL_CHANNEL_START);

	k_mutex_unlock(&data->mutex);

	return 0;
}

/*
 * Get the clock rate (cycles per second) for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM port number
 * cycles: Pointer to the memory to store clock rate (cycles per second)
 *
 * return 0, or negative errno code
 */
static int pwm_acts_get_cycles_per_sec(struct device *dev, u32_t chan,
					u64_t *cycles)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);

	if (chan >= cfg->num_chans) {
		SYS_LOG_ERR("invalid pwm chan %d", chan);
		return -EINVAL;
	}

	*cycles = (u64_t)cfg->pwm_pin_cycles;

	return 0;
}

static int pwm_acts_set_breath_mode(struct device *dev, u32_t chan, pwm_breath_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	struct pwm_acts_data *data = dev->driver_data;
	struct acts_pwm_chan *pwm_chan;
	u32_t period = PWM_PIN_CLK_PERIOD_USEC, qd, qu, high, low;
	pwm_breath_ctrl_t breath_ctrl = {0};

	if (!ctrl) {
		breath_ctrl.rise_time_ms = PWM_BREATH_RISE_TIME_DEFAULT;
		breath_ctrl.down_time_ms = PWM_BREATH_DOWN_TIME_DEFAULT;
		breath_ctrl.high_time_ms = PWM_BREATH_HIGH_TIME_DEFAULT;
		breath_ctrl.low_time_ms = PWM_BREATH_LOW_TIME_DEFAULT;
	} else {
		memcpy(&breath_ctrl, ctrl, sizeof(pwm_breath_ctrl_t));
	}

	SYS_LOG_INF("PWM@%d rise %dms, down %dms, high %dms, low %dms",
		chan, breath_ctrl.rise_time_ms, breath_ctrl.down_time_ms,
		breath_ctrl.high_time_ms, breath_ctrl.low_time_ms);

	if (chan >= cfg->num_chans) {
		SYS_LOG_ERR("invalid chan %d", chan);
		return -EINVAL;
	}

	if (pwm_acts_set_pinmux(dev, chan)) {
		SYS_LOG_DBG("set pwm pinmux error");
		return -EFAULT;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	pwm_chan = PWM_CHAN(cfg->base, chan);

	/* disable pwm */
	pwm_chan->ctrl = 0;

	/* setup pwm parameters */
	pwm_chan->ctrl = PWM_CTRL_POL_SEL_HIGH | PWM_CTRL_MODE_SEL_BREATH;

	/* rise time T1 = QU x C x C x t; C=32, t=PWM_PIN_CLK_PERIOD_USEC */
	qu = breath_ctrl.rise_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C / PWM_BREATH_MODE_DEFAULT_C;
	qu = (qu + period - 1) / period;

	/* down time T2 = QD x C x C x t; C=32, t=PWM_PIN_CLK_PERIOD_USEC */
	qd = breath_ctrl.down_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C / PWM_BREATH_MODE_DEFAULT_C;
	qd = (qd + period - 1) / period;

	/* high level time T3 = H x C x t; C=32, t = PWM_PIN_CLK_PERIOD_USEC */
	high = breath_ctrl.high_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C;
	high = (high + period - 1) / period;

	/* high level time T3 = L x C x t; C=32, t = PWM_PIN_CLK_PERIOD_USEC */
	low = breath_ctrl.low_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C;
	low = (low + period - 1) / period;

	SYS_LOG_INF("QU:%d QD:%d high:%d low:%d", qu, qd, high, low);

	pwm_chan->c = PWM_BREATH_MODE_C(PWM_BREATH_MODE_DEFAULT_C);
	pwm_chan->q = PWM_BREATH_MODE_QU(qu) | PWM_BREATH_MODE_QD(qd);
	pwm_chan->h = PWM_BREATH_MODE_H(high);
	pwm_chan->l = PWM_BREATH_MODE_L(low);

	/* enable pwm */
	pwm_chan->ctrl |= (PWM_CTRL_CHANNEL_EN | PWM_CTRL_CHANNEL_START);

	k_mutex_unlock(&data->mutex);

	return 0;
}

static void pwm_acts_dma_reload(struct device *dev, u32_t priv_data, int reason)
{
	u32_t _reason;
	int ret;
	struct pwm_acts_data *data = (struct pwm_acts_data *)priv_data;

	if (reason == DMA_IRQ_HF) {
		_reason = PWM_PROGRAM_DMA_IRQ_HF;
	} else if (reason == DMA_IRQ_TC) {
		_reason = PWM_PROGRAM_DMA_IRQ_TC;
	} else {
		SYS_LOG_ERR("Unknown DMA reason %d", reason);
		dma_stop(dev, data->dma_chan);
		return ;
	}

	ret = data->program_callback(data->cb_data, _reason);
	if (ret < 0)
		dma_stop(dev, data->dma_chan);
}

static void pwm_acts_dma_direct(struct device *dev, u32_t priv_data, int reason)
{
	struct pwm_acts_data *data = (struct pwm_acts_data *)priv_data;
	ARG_UNUSED(dev);

	if ((reason == DMA_IRQ_TC) && data->program_callback) {
		data->program_callback(data->cb_data, PWM_PROGRAM_DMA_IRQ_TC);
	}
}

static int pwm_acts_dma_prepare(struct device *dev, u32_t chan, pwm_program_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	struct pwm_acts_data *data = dev->driver_data;
	struct acts_pwm_dmactl *dmactrl = PWM_DMACTL_REG(cfg);
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	/* request dma channel handle */
	if (data->dma_chan == -1) {
		data->dma_chan = dma_request(data->dma_dev, 0xFF);
		if (!data->dma_chan) {
			SYS_LOG_ERR("Failed to request dma channel");
			return -ENXIO;
		}
	}

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_burst_length = 8;
	dma_cfg.dest_burst_length = 8;
	dma_cfg.source_data_size = 2;
	dma_cfg.dma_slot = DMA_ID_PWM;
	dma_block_cfg.source_address = (u32_t)ctrl->ram_buf;
	dma_block_cfg.dest_address = (u32_t)&dmactrl->fifodat;

	dma_block_cfg.block_size = ctrl->ram_buf_len;
	dma_block_cfg.source_reload_en = ctrl->reload_en;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;

	if (ctrl->program_callback) {
		if (ctrl->reload_en)
			dma_cfg.dma_callback = pwm_acts_dma_reload;
		else
			dma_cfg.dma_callback = pwm_acts_dma_direct;

		dma_cfg.callback_data = data;
		dma_cfg.complete_callback_en = 1;
		dma_cfg.half_complete_callback_en = 1;

		data->program_callback = ctrl->program_callback;
		data->cb_data = ctrl->cb_data;
	}

	if (dma_config(data->dma_dev, data->dma_chan, &dma_cfg)) {
		SYS_LOG_ERR("DMA config error");
		return -EFAULT;
	}

	return 0;
}

static int pwm_acts_set_program_mode(struct device *dev, u32_t chan, pwm_program_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	struct pwm_acts_data *data = dev->driver_data;
	struct acts_pwm_chan *pwm_chan;
	struct acts_pwm_dmactl *dmactrl = PWM_DMACTL_REG(cfg);
	int ret = 0;

	if (!ctrl) {
		SYS_LOG_ERR("invalid ctrl parameter");
		return -EINVAL;
	}

	if (ctrl->reload_en && !ctrl->program_callback) {
		SYS_LOG_ERR("in reload mode but without program callback");
		return -EINVAL;
	}

	if (ctrl->period_cycles > PWM_CYCLES_MAX || ctrl->period_cycles < 1) {
		SYS_LOG_ERR("period cycles invalid %d (max %d min 1)",
			ctrl->period_cycles, PWM_CYCLES_MAX);
		return -EINVAL;
	}

	SYS_LOG_INF("PWM@%d ram buf%p len %d, reload_en %d",
		chan, ctrl->ram_buf, ctrl->ram_buf_len, ctrl->reload_en);

	if (chan >= cfg->num_chans) {
		SYS_LOG_ERR("invalid chan %d", chan);
		return -EINVAL;
	}

	if (pwm_acts_set_pinmux(dev, chan)) {
		SYS_LOG_DBG("set pwm pinmux error");
		return -EFAULT;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (ctrl->reload_en && (data->program_pin != PWM_PROGRAM_PIN_INVALID)) {
		SYS_LOG_ERR("reload program pin %d is already in used", data->program_pin);
		ret = -EACCES;
		goto out;
	}

	pwm_chan = PWM_CHAN(cfg->base, chan);

	/* disable pwm */
	pwm_chan->ctrl = 0;

	/* setup pwm parameters */
	pwm_chan->ctrl = PWM_CTRL_POL_SEL_HIGH | PWM_CTRL_MODE_SEL_PROGRAM;

	pwm_chan->duty_max = PWM_DUTYMAX(ctrl->period_cycles);

	dmactrl->dmactl = PWM_FIFO_CLK_SEL(chan) | PWM_DMA_CTL_START;

	ret = pwm_acts_dma_prepare(dev, chan, ctrl);
	if (ret) {
		SYS_LOG_DBG("PWM DMA prepare error:%d", ret);
		goto out;
	}

	data->program_pin = chan;

	/* enable pwm */
	pwm_chan->ctrl |= (PWM_CTRL_CHANNEL_EN | PWM_CTRL_CHANNEL_START);

	ret = dma_start(data->dma_dev, data->dma_chan);

	if (!ctrl->reload_en && !ctrl->program_callback)
		dma_wait_finished(data->dma_dev, data->dma_chan);

out:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int pwm_acts_pin_stop(struct device *dev, u32_t chan)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	struct pwm_acts_data *data = dev->driver_data;
	struct acts_pwm_chan *pwm_chan;

	if (chan >= cfg->num_chans) {
		SYS_LOG_ERR("invalid pwm chan %d", chan);
		return -EINVAL;
	}

	pwm_chan = PWM_CHAN(cfg->base, chan);

	k_mutex_lock(&data->mutex, K_FOREVER);
	if (chan == data->program_pin && data->dma_chan != -1) {
		dma_stop(data->dma_dev, data->dma_chan);
		dma_free(data->dma_dev, data->dma_chan);
		data->program_pin = PWM_PROGRAM_PIN_INVALID;
		data->dma_chan = -1;
	}
	pwm_chan->ctrl = 0;
	k_mutex_unlock(&data->mutex);

	SYS_LOG_INF("PWM@%d pin stop", chan);

	return 0;
}

const struct pwm_driver_api pwm_acts_drv_api_funcs = {
	.pin_set = pwm_acts_pin_set,
	.get_cycles_per_sec = pwm_acts_get_cycles_per_sec,
	.set_breath = pwm_acts_set_breath_mode,
	.set_program = pwm_acts_set_program_mode,
	.pin_stop = pwm_acts_pin_stop
};

int pwm_acts_init(struct device *dev)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	struct pwm_acts_data *data = dev->driver_data;
	int i;
	u8_t clk_div;

	/* enable pwm controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);

	/* reset pwm controller */
	acts_reset_peripheral(cfg->reset_id);

	/* clock source: 32K, div= / 4, pwm clock fs 8KHz period 125us */
	clk_div = (PWM_CLK_CYCLES_PER_SEC / PWM_PIN_CYCLES_PER_SEC) - 1;

	/* init PWM clock */
	for (i = 0; i < cfg->num_chans; i++) {
		sys_write32(CMU_PWMCLK_CLKSEL_32K | CMU_PWMCLK_CLKDIV(clk_div), \
			cfg->pwmclk_reg + 4 * i);
	}

	k_mutex_init(&data->mutex);

	data->dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (!data->dma_dev) {
		SYS_LOG_ERR("Bind DMA device %s error", CONFIG_DMA_0_NAME);
		return -ENOENT;
	}
	data->program_pin = PWM_PROGRAM_PIN_INVALID;
	data->dma_chan = -1;

    return 0;
}

static struct pwm_acts_data pwd_acts_dev_data;

static const struct pwm_acts_config pwm_acts_dev_cfg = {
	.base = PWM_REG_BASE,
	.pwmclk_reg = CMU_PWM0CLK,
	.pwm_pin_cycles = PWM_PIN_CYCLES_PER_SEC,
	.num_chans = 9,
	.clock_id = CLOCK_ID_PWM,
	.reset_id = RESET_ID_PWM,
};

DEVICE_AND_API_INIT(pwm_acts, CONFIG_PWM_ACTS_DEV_NAME,
		    pwm_acts_init,
		    &pwd_acts_dev_data, &pwm_acts_dev_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_acts_drv_api_funcs);
