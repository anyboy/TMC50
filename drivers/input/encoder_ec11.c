#include <sys_event.h>
#include <dvfs.h>
#include "gpio.h"
#include <input_dev.h>
#include <board.h>
#include <brom_interface.h>
#define ROTARY_ENCODER_OVERTIME    1000

#define encoder_FORWARD     0
#define encoder_BACKWAED    1
#define encoder_STOP        2
#define ENCODER_OVERTIME    5

static struct gpio_callback encoder_gpio_cb_a;
static struct gpio_callback encoder_gpio_cb_b;
static struct device *_gpio_dev;

struct rotary_encoder_info_t {


	bool A_level_flag;
	uint16_t A_jitter_timer;

	bool B_level_flag;
	uint16_t B_jitter_timer;

	uint8_t EnStatus;

	input_notify_t notify;
};

typedef void (*gpio_config_irq_t)(struct device *dev);

struct acts_gpio_config {
	u32_t base;
	u32_t irq_num;
	gpio_config_irq_t config_func;
};

static struct rotary_encoder_info_t g_rotary_encode_info;

static void COMa_IRQ_callback(struct device *port, struct gpio_callback *cb, u32_t pins)
{
	int val_a,val_b;
	struct input_value val;

	const struct acts_gpio_config *info = _gpio_dev->config->config_info;

	val_a = !!(sys_read32(GPIO_REG_IDAT(info->base, COMA_GPION)) & GPIO_BIT(COMA_GPION));
	val_b = !!(sys_read32(GPIO_REG_IDAT(info->base, COMB_GPION)) & GPIO_BIT(COMB_GPION));

	if(val_a == g_rotary_encode_info.A_level_flag || BROM_TIMESTAMP_GET_US() - g_rotary_encode_info.A_jitter_timer < ROTARY_ENCODER_OVERTIME)
		return;
	g_rotary_encode_info.A_jitter_timer = BROM_TIMESTAMP_GET_US();

	if(val_a == 1 && val_b == 1) {
		g_rotary_encode_info.EnStatus = encoder_STOP;
	}
	if(g_rotary_encode_info.EnStatus == encoder_STOP) {
		if(val_a == 0 && val_b == 0) {
			if(g_rotary_encode_info.A_level_flag == 1 && g_rotary_encode_info.B_level_flag == 0) {
				g_rotary_encode_info.EnStatus = encoder_BACKWAED;
				val.code = 0;
				val.type = EV_KEY;
				val.value = encoder_BACKWAED;
				g_rotary_encode_info.notify(NULL, &val);
			}
		}
	}
	g_rotary_encode_info.A_level_flag = val_a;
	g_rotary_encode_info.B_level_flag = val_b;
}

static void COMb_IRQ_callback(struct device *port, struct gpio_callback *cb, u32_t pins)
{
	int val_a,val_b;
	struct input_value val;

	const struct acts_gpio_config *info = _gpio_dev->config->config_info;

	val_a = !!(sys_read32(GPIO_REG_IDAT(info->base, COMA_GPION)) & GPIO_BIT(COMA_GPION));
	val_b = !!(sys_read32(GPIO_REG_IDAT(info->base, COMB_GPION)) & GPIO_BIT(COMB_GPION));

	if(val_b == g_rotary_encode_info.B_level_flag || BROM_TIMESTAMP_GET_US() - g_rotary_encode_info.B_jitter_timer < ROTARY_ENCODER_OVERTIME)
		return;
	g_rotary_encode_info.B_jitter_timer = BROM_TIMESTAMP_GET_US();

	if(val_a == 1 && val_b == 1) {
		g_rotary_encode_info.EnStatus = encoder_STOP;
	}
	if(g_rotary_encode_info.EnStatus == encoder_STOP) {
		if(val_a == 0 && val_b == 0) {
			if(g_rotary_encode_info.B_level_flag == 1 && g_rotary_encode_info.A_level_flag == 0) {
				g_rotary_encode_info.EnStatus = encoder_FORWARD;
				val.code = 0;
				val.type = EV_KEY;
				val.value = encoder_FORWARD;
				g_rotary_encode_info.notify(NULL, &val);
			}
		}
	}
	g_rotary_encode_info.A_level_flag = val_a;
	g_rotary_encode_info.B_level_flag = val_b;

}


static void encoder_ec11_enable(struct device *dev)
{
	gpio_pin_enable_callback(_gpio_dev, COMA_GPION);
	gpio_pin_enable_callback(_gpio_dev, COMB_GPION);
}

static void encoder_ec11_disable(struct device *dev)
{
	gpio_pin_disable_callback(_gpio_dev, COMA_GPION);
	gpio_pin_disable_callback(_gpio_dev, COMB_GPION);
}

static void encoder_ec11_register_notify(struct device *dev, input_notify_t notify)
{
	g_rotary_encode_info.notify = notify;
}

static void encoder_ec11_unregister_notify(struct device *dev, input_notify_t notify)
{
	g_rotary_encode_info.notify = NULL;
}

static int encoder_ec11_init(struct device *dev)
{
	g_rotary_encode_info.EnStatus = encoder_STOP;

	_gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
	gpio_pin_configure(_gpio_dev, COMA_GPION, GPIO_CTL_SMIT | GPIO_INT_LEVEL | GPIO_PUD_PULL_UP | GPIO_DIR_IN | GPIO_INT_DEBOUNCE | GPIO_INT | GPIO_INT_EDGE|GPIO_INT_DOUBLE_EDGE);
	gpio_init_callback(&encoder_gpio_cb_a, COMa_IRQ_callback, BIT(COMA_GPION));
	gpio_add_callback(_gpio_dev, &encoder_gpio_cb_a);
	gpio_pin_configure(_gpio_dev, COMB_GPION, GPIO_CTL_SMIT | GPIO_INT_LEVEL | GPIO_PUD_PULL_UP | GPIO_DIR_IN | GPIO_INT_DEBOUNCE | GPIO_INT | GPIO_INT_EDGE|GPIO_INT_DOUBLE_EDGE);
	gpio_init_callback(&encoder_gpio_cb_b, COMb_IRQ_callback, BIT(COMB_GPION));
	gpio_add_callback(_gpio_dev, &encoder_gpio_cb_b);

	return 0;
}

static const struct input_dev_driver_api encoder_ec11_api = {
	.enable = encoder_ec11_enable,
	.disable = encoder_ec11_disable,
	.register_notify = encoder_ec11_register_notify,
	.unregister_notify = encoder_ec11_unregister_notify,
};

DEVICE_AND_API_INIT(encoder_ec11, CONFIG_ENCODER_NAME, encoder_ec11_init,
		NULL, NULL, POST_KERNEL,
		10, &encoder_ec11_api);


