/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

#define BOARD_SDCARD_USE_INTERNAL_PULLUP		/* use SoC internal pull-up resistance */
#if !defined(CONFIG_AUDIO_IN_I2SRX0_SUPPORT) && !defined(CONFIG_I2S_5WIRE) && !defined(CONFIG_I2S_PSEUDO_5WIRE)
#define BOARD_SDCARD_DETECT_GPIO_NAME		"GPIO_0"
#define BOARD_SDCARD_DETECT_GPIO		28	/* detect */
#define BOARD_SDCARD_POWER_EN_GPIO_NAME		"GPIO_0"
#define BOARD_SDCARD_POWER_EN_GPIO		28	/* detect gpio is also used as power en gpio */
#endif

#ifndef CONFIG_INPUT_DEV_ACTS_IRKEY
#define BOARD_USB_HOST_VBUS_EN_GPIO_NAME	"GPIO_0"
#define BOARD_USB_HOST_VBUS_EN_GPIO		0	/* Vbus power en gpio for USB host */
#define BOARD_USB_HOST_VBUS_EN_GPIO_VALUE	1	/* Value to enable */
#endif

#ifdef CONFIG_PANEL_ST7789V
#define BOARD_PANEL_ST7789V_RES		16
#define BOARD_PANEL_ST7789V_DC		23
#define BOARD_PANEL_ST7789V_CS		17
#endif

#ifdef CONFIG_OUTSIDE_FLASH
#define BOARD_OUTSIDE_FALSH_CS_PIN_CONTROLLER		20
#define BOARD_OUTSIDE_FALSH_DC_PIN_DRIVER		23
#endif

#ifdef CONFIG_PANEL_SSD1306
#define BOARD_PANEL_SSD1306_DC		23
#define BOARD_PANEL_SSD1306_RES		16
#define BOARD_PANEL_SSD1306_CS		17
#endif

#ifdef CONFIG_ENCODER_INPUT
#define COMA_GPION      15
#define COMB_GPION      14
#endif

#ifdef CONFIG_INPUT_DEV_ACTS_ADC_SR
#define SR_1_GPIO		14
#define SR_1_ADC_CHAN	ADC_ID_CH7
#define SR_2_GPIO		15
#define SR_2_ADC_CHAN	ADC_ID_CH9
#endif

#ifdef CONFIG_USP
#define BOARD_OUTSPDIF_START_GPIO_NAME		"GPIO_0"
#define BOARD_OUTSPDIF_START_GPIO_CONTROLLER		1
#endif

#ifdef CONFIG_ACTS_TIMER3_CAPTURE
#define TIMER3_CAPTURE_GPIO			(14)
#endif

//#define BOARD_LINEIN_DETECT_GPIO_NAME	"GPIO_0"
//#define BOARD_LINEIN_DETECT_GPIO		9	/*line in dectect gpio */
//#define BOARD_LINEIN_DETECT_GPIO_VALUE	0	/* Value to plugin */
//#define BOARD_POWER_LOCK_GPIO          13
/* USB DPDM */
#define BOARD_USB_DM_PIN				10
#define BOARD_USB_DP_PIN				11

#define BOARD_UART_0_TX_PIN				(6)
#define BOARD_UART_0_RX_PIN				(5)

#define BOARD_ADCKEY_KEY_MAPPINGS	\
	{KEY_ADFU,      0x05}, \
	{KEY_MENU,		0x3E},	\
	{KEY_TBD,	        0xa7},	\
	{KEY_PREVIOUSSONG,      0x129},	\
	{KEY_NEXTSONG,	        0x186},	\
	{KEY_VOLUMEDOWN,          0x216},\
	{KEY_VOLUMEUP,	0x2A0},


//#define BOARD_BATTERY_CHARGING_STATUS_GPIO	15

#define BOARD_BATTERY_CAP_MAPPINGS      \
	{0, 3200},  \
	{5, 3300},  \
	{10, 3400}, \
	{20, 3550}, \
	{30, 3650}, \
	{40, 3750}, \
	{50, 3800}, \
	{60, 3850}, \
	{70, 3900}, \
	{80, 3950}, \
	{90, 4000}, \
	{100, 4050},

#ifndef BOARD_LINEIN_DETECT_GPIO
#define LINEIN_DETEC_ADVAL_RANGE \
        700,    \
        800

#endif

#define HOLDABLE_KEY  {KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_NEXTSONG, KEY_PREVIOUSSONG}
#define MUTIPLE_CLIK_KEY  {KEY_POWER, KEY_TBD, KEY_NEXTSONG, KEY_PREVIOUSSONG}

/* led id, gpio , pwm id, active level*/
typedef struct {
    u8_t led_id;
    u8_t led_pin;
    u8_t led_pwm;
    u8_t active_level;
}led_map;

/**gpio 8 9 used by psram */
#if !defined(CONFIG_SOC_SERIES_WOODPECKERFPGA) //fpga don't care display

/*
#define BOARD_LED_MAP   \
	{   0,          24,          5,                  1},	\
	{   1,          25,          6,                  1},

#define BOARD_PWM_MAP   \
    {24,     0xc},  \
    {25,     0xc},
#endif
*/
#define BOARD_LED_MAP   \
    /*led_id , led_pin. led_pwm,  active_level    */\
	{   0,          2,          0xff,             1},	
#endif


#ifdef CONFIG_INPUT_DEV_ACTS_IRKEY
/* irkey protocol:
 * 0: 9012, 1: NEC, 2: RC5, 3: RC6
 */
#define BOARD_IRKEY_PROTOCOL		1
#define BOARD_IRKEY_USER_CODE		0xff40 /* IRC Demo : 10 moons */

#define BOARD_IRKEY_KEY_MAPPINGS	\
	{KEY_MENU,			0x45},	\
	{KEY_PREVIOUSSONG,	0x10},	\
	{KEY_NEXTSONG,		0x11},	\
	{KEY_POWER,			0x0d},	\
	{KEY_VOLUMEUP,		0x15},	\
	{KEY_VOLUMEDOWN,	0x1c},	\
	{KEY_TBD,			0xf3},	\
	{KEY_POWER_IR,		0x4d},	\
	{KEY_CH_ADD,		0x0b},	\
	{KEY_CH_SUB,		0x0e},	\
	{KEY_NUM0,			0x00},	\
	{KEY_NUM1,			0x01},	\
	{KEY_NUM2,			0x02},	\
	{KEY_NUM3,			0x03},	\
	{KEY_NUM4,			0x04},	\
	{KEY_NUM5,			0x05},	\
	{KEY_NUM6,			0x06},	\
	{KEY_NUM7,			0x07},	\
	{KEY_NUM8,			0x08},	\
	{KEY_NUM9,			0x09},
#endif

/*!
 * enum audio_input_dev_e
 * @brief Audio input devices enumration
 */
typedef enum {
	AUDIO_LINE_IN0 = 0,
	AUDIO_LINE_IN1,
	AUDIO_LINE_IN2,
	AUDIO_LINE_IN0_AMIC0,
	AUDIO_ANALOG_MIC0,
	AUDIO_ANALOG_MIC1,
	AUDIO_ANALOG_FM0,
	AUDIO_ANALOG_FM0_AMIC0,
	AUDIO_DIGITAL_MIC0,
	AUDIO_DIGITAL_MIC1,
	AUDIO_INPUT_DEV_NONE = 0xFF,
} audio_input_dev_e;

#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
#define BOARD_SPDIFTX_MAP {16,  0xA | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},
#endif

#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
#define BOARD_SPDIFRX_MAP {17,  0xA | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},
#endif

extern int board_extern_pa_ctl(u8_t pa_class, bool is_on);

extern void board_init_psram_pins(void);
int board_audio_input_function_switch(audio_input_dev_e audio_dev,
												u8_t *lch_input, u8_t *rch_input);

#endif /* __INC_BOARD_H */
