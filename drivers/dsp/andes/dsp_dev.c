/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc_clock.h>
#include <soc_reset.h>
#include <soc_cmu.h>
#include <misc/util.h>
#include <dsp.h>
#include "dsp_inner.h"

/* if set, the user can externally shut down the DSP root clock (gated by CMU) */
#define PSU_DSP_IDLE BIT(3)
/* if set, only the DSP Internal core clock is gated */
#define PSU_DSP_CORE_IDLE BIT(2)

static void dsp_acts_isr(void *arg);
static void dsp_acts_irq_enable(void);
static void dsp_acts_irq_disable(void);

static int get_hw_idle(void)
{
	return sys_read32(CMU_DSP_WAIT) & PSU_DSP_IDLE;
}

int wait_hw_idle_timeout(int usec_to_wait)
{
	do {
		if (get_hw_idle())
			return 0;
		if (usec_to_wait-- <= 0)
			break;
		k_busy_wait(1);
	} while (1);

	return -EAGAIN;
}

static int dsp_acts_suspend(struct device *dev)
{
	struct dsp_acts_data *dsp_data = dev->driver_data;

	if (dsp_data->pm_status != DSP_STATUS_POWERON)
		return 0;

	/* already idle ? */
	if (!get_hw_idle()) {
		struct dsp_message message = { .id = DSP_MSG_SUSPEND, };

		k_sem_reset(&dsp_data->msg_sem);

		if (dsp_acts_send_message(dev, &message))
			return -EBUSY;

		/* wait message DSP_MSG_BOOTUP */
		if (k_sem_take(&dsp_data->msg_sem, 20)) {
			printk("dsp image <%s> busy\n", dsp_data->images[DSP_IMAGE_MAIN].name);
			return -EBUSY;
		}

		/* make sure hardware is really idle */
		if (wait_hw_idle_timeout(10))
			printk("DSP wait idle signal timeout\n");
	}

	dsp_do_wait();
	acts_clock_peripheral_disable(CLOCK_ID_DSP);

	dsp_data->pm_status = DSP_STATUS_SUSPENDED;
	printk("DSP suspended\n");
	return 0;
}

static int dsp_acts_resume(struct device *dev)
{
	struct dsp_acts_data *dsp_data = dev->driver_data;
	struct dsp_message message = { .id = DSP_MSG_RESUME, };

	if (dsp_data->pm_status != DSP_STATUS_SUSPENDED)
		return 0;

	acts_clock_peripheral_enable(CLOCK_ID_DSP);
	dsp_undo_wait();

	if (dsp_acts_send_message(dev, &message)) {
		printk("DSP resume failed\n");
		return -EFAULT;
	}

	dsp_data->pm_status = DSP_STATUS_POWERON;
	printk("DSP resumed\n");
	return 0;
}

static int dsp_acts_kick(struct device *dev, u32_t owner, u32_t event, u32_t params)
{
	struct dsp_acts_data *dsp_data = dev->driver_data;
	const struct dsp_acts_config *dsp_cfg = dev->config->config_info;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	struct dsp_message message = {
		.id = DSP_MSG_KICK,
		.owner = owner,
		.param1 = event,
		.param2 = params,
	};

	if (dsp_data->pm_status != DSP_STATUS_POWERON)
		return -EACCES;

	if (dsp_userinfo->task_state != DSP_TASK_SUSPENDED)
		return -EINPROGRESS;

	return dsp_acts_send_message(dev, &message);
}

static int dsp_acts_power_on(struct device *dev, void *cmdbuf)
{
	struct dsp_acts_data *dsp_data = dev->driver_data;
	const struct dsp_acts_config *dsp_cfg = dev->config->config_info;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	int i;

	if (dsp_data->pm_status != DSP_STATUS_POWEROFF)
		return 0;

	if (dsp_data->images[DSP_IMAGE_MAIN].size == 0) {
		printk("%s: no image loaded\n", __func__);
		return -EFAULT;
	}

	if (cmdbuf == NULL) {
		printk("%s: must assign a command buffer\n", __func__);
		return -EINVAL;
	}

	/* set bootargs */
	dsp_data->bootargs.command_buffer = addr_cpu2dsp((u32_t)cmdbuf, false);

	/* assert dsp wait signal */
	dsp_do_wait();

	/* assert reset */
	acts_reset_peripheral_assert(RESET_ID_DSP_ALL);

	/* enable dsp clock*/
	acts_clock_peripheral_enable(CLOCK_ID_DSP);
	acts_clock_peripheral_enable(CLOCK_ID_DSP_COMM);

	/* intialize shared registers */
	dsp_cfg->dsp_mailbox->msg = MAILBOX_MSG(DSP_MSG_NULL, MSG_FLAG_ACK) ;
	dsp_cfg->dsp_mailbox->owner = 0;
	dsp_cfg->cpu_mailbox->msg = MAILBOX_MSG(DSP_MSG_NULL, MSG_FLAG_ACK) ;
	dsp_cfg->cpu_mailbox->owner = 0;
	dsp_userinfo->task_state = DSP_TASK_PRESTART;
	dsp_userinfo->error_code = DSP_NO_ERROR;

	/* set dsp vector_address */
	//set_dsp_vector_addr((unsigned int)DSP_ADDR);
	printk("%s: DSP_VCT_ADDR=0x%x\n", __func__, sys_read32(DSP_VCT_ADDR));

	/* clear all pending */
	clear_dsp_all_irq_pending();

	/* enable dsp irq */
	dsp_acts_irq_enable();

	/* deassert dsp wait signal */
	dsp_undo_wait();

	k_sem_reset(&dsp_data->msg_sem);

	/* deassert reset */
	acts_reset_peripheral_deassert(RESET_ID_DSP_ALL);

	/* wait message DSP_MSG_BOOTUP */
	if (k_sem_take(&dsp_data->msg_sem, 100)) {
		printk("dsp image <%s> cannot boot up\n", dsp_data->images[DSP_IMAGE_MAIN].name);
		goto cleanup;
	}

	dsp_data->pm_status = DSP_STATUS_POWERON;

	/* FIXME: convert userinfo address from dsp to cpu here, since
	 * dsp will never touch it again.
	 */
	dsp_userinfo->func_table = addr_dsp2cpu(dsp_userinfo->func_table, false);
	for (i = 0; i < dsp_userinfo->func_size; i++) {
		volatile u32_t addr = dsp_userinfo->func_table + i * 4;
		if (addr > 0)   /* NULL, no information provided */
			sys_write32(addr_dsp2cpu(sys_read32(addr), false), addr);
	}

	printk("DSP power on\n");
	return 0;
cleanup:
	dsp_acts_irq_disable();
	acts_clock_peripheral_disable(CLOCK_ID_DSP);
	acts_reset_peripheral_assert(RESET_ID_DSP_ALL);
	return -ETIMEDOUT;
}

static int dsp_acts_power_off(struct device *dev)
{
	const struct dsp_acts_config *dsp_cfg = dev->config->config_info;
	struct dsp_acts_data *dsp_data = dev->driver_data;

	if (dsp_data->pm_status == DSP_STATUS_POWEROFF)
		return 0;

	/* assert dsp wait signal */
	dsp_do_wait();

	/* disable dsp irq */
	dsp_acts_irq_disable();

	/* disable dsp clock*/
	acts_clock_peripheral_disable(CLOCK_ID_DSP);

	/* assert reset dsp module */
	acts_reset_peripheral_assert(RESET_ID_DSP_ALL);

	/* deassert dsp wait signal */
	dsp_undo_wait();

	/* clear page mapping */
	clear_dsp_pageaddr();

	dsp_cfg->dsp_userinfo->task_state = DSP_TASK_DEAD;
	dsp_data->pm_status = DSP_STATUS_POWEROFF;

	printk("DSP power off\n");
	return 0;
}

static int acts_request_userinfo(struct device *dev, int request, void *info)
{
	const struct dsp_acts_config *dsp_cfg = dev->config->config_info;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	union {
		struct dsp_request_session *ssn;
		struct dsp_request_function *func;
	} req_info;

	switch (request) {
	case DSP_REQUEST_TASK_STATE:
		*(int *)info = dsp_userinfo->task_state;
		break;
	case DSP_REQUEST_ERROR_CODE:
		*(int *)info = dsp_userinfo->error_code;
		break;
	case DSP_REQUEST_SESSION_INFO:
		req_info.ssn = info;
		req_info.ssn->func_enabled = dsp_userinfo->func_enabled;
		req_info.ssn->func_counter = dsp_userinfo->func_counter;
		req_info.ssn->info = (void *)addr_dsp2cpu(dsp_userinfo->ssn_info, false);
		break;
	case DSP_REQUEST_FUNCTION_INFO:
		req_info.func = info;
		if (req_info.func->id < dsp_userinfo->func_size)
			req_info.func->info = (void *)sys_read32(dsp_userinfo->func_table + req_info.func->id * 4);
		break;
	case DSP_REQUEST_USER_DEFINED:
	default:
		return -EINVAL;
	}

	return 0;
}

static void dsp_acts_isr(void *arg)
{
	/* clear irq pending bits */
	clear_dsp_irq_pending(IRQ_ID_OUT_USER0);

	dsp_acts_recv_message(arg);
}



static int dsp_acts_init(struct device *dev)
{
	const struct dsp_acts_config *dsp_cfg = dev->config->config_info;
	struct dsp_acts_data *dsp_data = dev->driver_data;
	
#ifdef CONFIG_DSP_DEBUG_PRINT	
	dsp_data->bootargs.debug_buf = mcu_to_dsp_data_address(POINTER_TO_UINT(acts_ringbuf_alloc(0x100)));
#endif

	dsp_cfg->dsp_userinfo->task_state = DSP_TASK_DEAD;
	k_sem_init(&dsp_data->msg_sem, 0, 1);
	k_mutex_init(&dsp_data->msg_mutex);
	return 0;
}

static struct dsp_acts_data dsp_acts_data = {
	.bootargs.power_latency = CONFIG_DSP_ACTIVE_POWER_LATENCY_MS * 1000,
};

static const struct dsp_acts_config dsp_acts_config = {
	.dsp_mailbox = (struct dsp_protocol_mailbox *)DSP_M2D_MAILBOX_REGISTER_BASE,
	.cpu_mailbox = (struct dsp_protocol_mailbox *)DSP_D2M_MAILBOX_REGISTER_BASE,
	.dsp_userinfo = (struct dsp_protocol_userinfo *)DSP_USER_REGION_REGISTER_BASE,
};

const struct dsp_driver_api dsp_drv_api = {
	.poweron = dsp_acts_power_on,
	.poweroff = dsp_acts_power_off,
	.suspend = dsp_acts_suspend,
	.resume = dsp_acts_resume,
	.kick = dsp_acts_kick,
	.register_message_handler = dsp_acts_register_message_handler,
	.unregister_message_handler = dsp_acts_unregister_message_handler,
	.request_image = dsp_acts_request_image,
	.release_image = dsp_acts_release_image,
	.send_message = dsp_acts_send_message,
	.request_userinfo = acts_request_userinfo,
};

DEVICE_AND_API_INIT(dsp_acts, CONFIG_DSP_ACTS_DEV_NAME,
		dsp_acts_init, &dsp_acts_data, &dsp_acts_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dsp_drv_api);

static void dsp_acts_irq_enable(void)
{
	IRQ_CONNECT(IRQ_ID_OUT_USER0, CONFIG_IRQ_PRIO_HIGH, dsp_acts_isr, DEVICE_GET(dsp_acts), 0);
	irq_enable(IRQ_ID_OUT_USER0);
}

static void dsp_acts_irq_disable(void)
{
	irq_disable(IRQ_ID_OUT_USER0);
}

#ifdef CONFIG_DSP_DEBUG_PRINT
#define DEV_DATA(dev) \
	((struct dsp_acts_data * const)(dev)->driver_data)

struct acts_ringbuf *dsp_dev_get_print_buffer(void)
{
	struct dsp_acts_data *data = DEV_DATA(DEVICE_GET(dsp_acts));

	return (struct acts_ringbuf *)dsp_data_to_mcu_address(data->bootargs.debug_buf);   
}
#endif
