/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <soc.h>
#include <timer_capture.h>

#define SYS_LOG_DOMAIN "TIMER_CAP"
#include <logging/sys_log.h>

#if (defined CONFIG_ACTS_TIMER2_CAPTURE || defined CONFIG_ACTS_TIMER3_CAPTURE)

#define     TM_CTL_LEVEL              (1<<12)
#define     TM_CTL_DIR(x)             ((x)<<11)
#define     TM_CTL_DIR_DONW           TM_CTL_DIR(0)
#define     TM_CTL_DIR_UP             TM_CTL_DIR(1)
#define     TM_CTL_DIR_MASK           (1<<11)
#define     TM_CTL_MODE(x)            ((x)<<9)
#define     TM_CTL_CAPTURE_IP         (1<<8)
#define     TM_CTL_CAPTURE_SE(x)      ((x)<<6)
#define     TM_CTL_CAPTURE_SE_MASK    (0x3<<6)
#define     TM_CTL_EN                 (1<<5)
#define     TM_CTL_RELO(x)            ((x)<<2)
#define     TM_CTL_ZIEN               (1<<1)
#define     TM_CTL_ZIPD               (1<<0)


/* Timer with capture controller */
struct timer_capture_controller {
	volatile uint32_t ctl;
	volatile uint32_t val;
	volatile uint32_t cnt;
	volatile uint32_t cap;
};

struct acts_timer_capture_data {
	capture_notify_t notify;
	int last_capture_counter; /* -1 means timeout*/
	int last_capture_level;
};

struct acts_timer_capture_config {
	struct timer_capture_controller *base;
	u32_t timeout_val;
	void (*irq_config_func)(struct device *dev);
};

static void acts_timer_capture_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct acts_timer_capture_config *cfg = dev->config->config_info;
	struct acts_timer_capture_data *capture_data = dev->driver_data;
	struct timer_capture_controller *capture = cfg->base;
	u32_t ctl_reg;

	ctl_reg = capture->ctl;

	if (ctl_reg & TM_CTL_CAPTURE_IP) {
		capture_data->last_capture_counter = capture->cap;
		capture_data->last_capture_level = !!(ctl_reg&TM_CTL_LEVEL);
	} else {
		capture_data->last_capture_counter = -1;
	}

	SYS_LOG_DBG("timer capture : %d, level = %d", capture_data->last_capture_counter, capture_data->last_capture_level);

	if (capture_data->notify) {
		capture_data->notify(dev, capture_data->last_capture_counter, capture_data->last_capture_level);
	}

	capture->ctl |= TM_CTL_CAPTURE_IP|TM_CTL_ZIPD;
}

static void acts_timer_capture_start(struct device *dev, capture_signal_e sig, capture_notify_t notify)
{
	const struct acts_timer_capture_config *cfg = dev->config->config_info;
	struct acts_timer_capture_data *capture_data = dev->driver_data;
	struct timer_capture_controller *capture = cfg->base;
	u32_t ctl_reg;

	ctl_reg = capture->ctl;
	ctl_reg &= ~TM_CTL_EN;
	ctl_reg |= TM_CTL_CAPTURE_IP|TM_CTL_ZIPD;
	capture->ctl = ctl_reg;

	capture_data->notify = notify;

	capture->val = cfg->timeout_val;

	capture->ctl = TM_CTL_DIR_UP | TM_CTL_MODE(2) | TM_CTL_CAPTURE_SE(sig) | TM_CTL_RELO(0) | TM_CTL_ZIEN | TM_CTL_EN;
}

static void acts_timer_capture_stop(struct device *dev)
{
	const struct acts_timer_capture_config *cfg = dev->config->config_info;
	struct acts_timer_capture_data *capture_data = dev->driver_data;
	struct timer_capture_controller *capture = cfg->base;
	u32_t ctl_reg;

	ctl_reg = capture->ctl;
	ctl_reg &= ~TM_CTL_EN;
	ctl_reg |= TM_CTL_CAPTURE_IP|TM_CTL_ZIPD;
	capture->ctl = ctl_reg;

	capture_data->notify = NULL;
}

static const struct capture_dev_driver_api timer_capture_driver_api = {
	.start = acts_timer_capture_start,
	.stop = acts_timer_capture_stop,
};

#ifdef CONFIG_ACTS_TIMER2_CAPTURE

static int timer2_capture_init(struct device *dev)
{
	const struct acts_timer_capture_config *cfg = dev->config->config_info;
	//struct acts_timer_capture_data *capture_data = dev->driver_data;
	//struct timer_capture_controller *capture = cfg->base;

	cfg->irq_config_func(dev);

	return 0;
}

static struct acts_timer_capture_data timer2_capture_ddata;

static void timer2_capture_irq_config(struct device *dev);

static const struct acts_timer_capture_config timer2_capture_cdata = {
	.base = (struct timer_capture_controller *)TIMER2_CAPTURE_BASE,
	.irq_config_func = timer2_capture_irq_config,
	.timeout_val = 24*1000*1000, /*capture timeout 1S*/
};

DEVICE_AND_API_INIT(timer2_capture, CONFIG_ACTS_TIMER2_DEV_NAME,
		    timer2_capture_init,
		    &timer2_capture_ddata, &timer2_capture_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &timer_capture_driver_api);

static void timer2_capture_irq_config(struct device *dev)
{
	IRQ_CONNECT(IRQ_ID_TIMER2, CONFIG_IRQ_PRIO_NORMAL, acts_timer_capture_isr, DEVICE_GET(timer2_capture), 0);
	irq_enable(IRQ_ID_TIMER2);
}

#endif

#ifdef CONFIG_ACTS_TIMER3_CAPTURE

static int timer3_capture_init(struct device *dev)
{
	const struct acts_timer_capture_config *cfg = dev->config->config_info;
	//struct acts_timer_capture_data *capture_data = dev->driver_data;
	//struct timer_capture_controller *capture = cfg->base;

	cfg->irq_config_func(dev);

	return 0;
}

static struct acts_timer_capture_data timer3_capture_ddata;

static void timer3_capture_irq_config(struct device *dev);

static const struct acts_timer_capture_config timer3_capture_cdata = {
	.base = (struct timer_capture_controller *)TIMER3_CAPTURE_BASE,
	.irq_config_func = timer3_capture_irq_config,
	.timeout_val = 24*1000*1000, /*capture timeout 1S*/
};

DEVICE_AND_API_INIT(timer3_capture, CONFIG_ACTS_TIMER3_DEV_NAME,
		    timer3_capture_init,
		    &timer3_capture_ddata, &timer3_capture_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &timer_capture_driver_api);

static void timer3_capture_irq_config(struct device *dev)
{
	IRQ_CONNECT(IRQ_ID_TIMER3, CONFIG_IRQ_PRIO_NORMAL, acts_timer_capture_isr, DEVICE_GET(timer3_capture), 0);
	irq_enable(IRQ_ID_TIMER3);
}

#endif


#endif

