/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief board init functions
 */

#include <init.h>
#include <gpio.h>
#include <soc.h>
#include "board.h"
#include<device.h>
#include<pwm.h>
#include <input_dev.h>
#include <audio_common.h>
#include <soc_boot.h>
#include <sys_manager.h>


#define PRODUCT_FLAG_TYPE_ATT_CARD  0x4341 /* 'AC' (att card product) */

boot_user_info_t g_boot_user_info;


static const struct acts_pin_config board_pin_config[] = {

#if (CONFIG_FM == 1)
	/* use fm*/
	{8, 0xD | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4) | GPIO_CTL_PULLUP}, /* I2C1_SDA */
	{9, 0xD | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4) | GPIO_CTL_PULLUP}, /* I2C1_SCL */
	{12,	0xe | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* FM_CLK_OUT */
#endif

	/* spi0, nor flash */
	{36, 4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(1)},	/* so2 */
	{37, 4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(1)},	/* so3 */
	{49, 5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(1)},	/* SPI0_SS */
	{50, 5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(1)},	/* SPI0_SCLK */
	{48, 5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(1)},	/* SPI0_MISO */
	{51, 5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(1)},	/* SPI0_MOSI */

	{BOARD_UART_0_RX_PIN, 6 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* UART0_RX */
	{BOARD_UART_0_TX_PIN, 6 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* UART0_TX */


#ifdef CONFIG_AUDIO_OUT_ACTS
	{40,  GPIO_CTL_AD_SELECT},	/* AOUTL/AOUTLP */
	{41,  GPIO_CTL_AD_SELECT},	/* AOUTR/AOUTLN */
#endif

#ifdef CONFIG_UART_ACTS_PORT_1
	/* use uart1 for UART_OTA test */
	{27, 0x7 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* UART1_RX */
	{26, 0x7 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* UART1_TX */
#endif

#ifdef CONFIG_AUDIO_IN_ACTS
	{42,  GPIO_CTL_AD_SELECT},	/* INPUT0L */
	{43,  GPIO_CTL_AD_SELECT},	/* INPUT0R */
	{44,  GPIO_CTL_AD_SELECT},	/* INPUT1L */
	{45,  GPIO_CTL_AD_SELECT},	/* INPUT1R */
	{46,  GPIO_CTL_AD_SELECT},	/* INPUT2L */
	{47,  GPIO_CTL_AD_SELECT},	/* INPUT2R */
#endif

#if defined(CONFIG_DAC_VRO_LAYOUT) || defined(CONFIG_DAC_DIFFERENCTIAL_LAYOUT)
	{38,  GPIO_CTL_AD_SELECT},	/* AOUTRP/VRO */
	{39,  GPIO_CTL_AD_SELECT},	/* AOUTRN/VRO_S */
#endif

#if ((CONFIG_BT_BQB_UART_PORT == 1) || (CONFIG_BT_FCC_UART_PORT == 1))
	/* use uart1 for BQB test and FCC test */
	{14, 0x7 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP_STRONG},	/* UART1_RX */
	{15, 0x7 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP_STRONG},	/* UART1_TX */
#endif

#ifdef CONFIG_I2C_SLAVE_ACTS
	{CONFIG_I2C_SLAVE_0_SCL_PIN,  	0xd | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* I2C0_SCL */
	{CONFIG_I2C_SLAVE_0_SDA_PIN,  	0xd | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* I2C0_SDA */
#endif

#if defined(CONFIG_AUDIO_OUT_I2STX0_SUPPORT) /* i2s tx0 */
	{14,  0x8 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2STX_BCLK */
	{15,  0x8 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2STX_LRCLK */
	{16,  0x8 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2STX_MCLK */
	{17,  0x8 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2STX_DOUT */
#if defined(CONFIG_I2S_5WIRE) || defined(CONFIG_I2S_PSEUDO_5WIRE)
	{28,  0x9 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2SRX_DIN */
#endif
#endif

#if defined(CONFIG_AUDIO_IN_I2SRX0_SUPPORT) /* i2s rx0 */
	{12,  0x9 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2SRX_BCLK */
	{13,  0x9 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2SRX_LRCLK */
	{8,  0x9 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2SRX_MCLK */
	{9,  0x9 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},  /* I2SRX_DIN */
#endif

#if 0
#ifdef CONFIG_MMC_SDCARD
	/* sd0 */
	{1, 0x4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},			/* SD0_CLK */
#ifdef BOARD_SDCARD_USE_INTERNAL_PULLUP
	/* sd0, internal pull-up resistances in SoC */
	{0, 0x4 | GPIO_CTL_SMIT | GPIO_CTL_PULLUP | GPIO_CTL_PADDRV_LEVEL(4)},	/* SD0_CMD */
	{2, 0x4 | GPIO_CTL_SMIT | GPIO_CTL_PULLUP | GPIO_CTL_PADDRV_LEVEL(4)},	/* SD0_D0 */
#else
	/* sd0, external pull-up resistances on the board */
	{0, 0x4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},					/* SD0_CMD */
	{2, 0x4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},					/* SD0_D0 */
#endif
#endif
#else
	{0,  0x80 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},

#endif

#ifdef CONFIG_CEC
	{15,  0xB | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4) | GPIO_CTL_PULLUP},  /* CEC */
#endif

	/* external pa */
#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	{12, 0},	 /* external pa ctl1 */
	{13, 0},	 /* external pa ctl2 */
#endif

#ifdef CONFIG_SPI_1
	{20, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{21, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{22, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{23, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
#endif

#ifdef CONFIG_ENCODER_INPUT
{14, 0},
{15, 0},
#endif

		/*seg led display*/
#if (defined(CONFIG_SEG_LED_DISPLAY)&& !defined(CONFIG_SEG_LCD_TYPE)&& !defined(CONFIG_SPI_1))
	{20, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{21, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{22, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{23, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{24, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{25, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
#if defined(CONFIG_LED_USE_1P2INCH_MAP)
	{14, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},/* LED_COM0 */
	{15, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},/* LED_COM1 */
	{16, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},/* LED_COM2 */
	{17, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},/* LED_COM3 */
	{28, 0x3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4)},/* LED_COM4 */
#endif
#else
#ifdef CONFIG_XSPI1_NOR_ACTS
	{20, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, /* SPI1_SS */
	{21, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, /* SPI1_SCLK */
	{22, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, /* SPI1_MOSI */
	{23, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, /* SPI1_MISO */
#endif
#endif
		/*seg lcd display*/
#ifdef CONFIG_SEG_LCD_TYPE
	{14, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{15, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{16, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{17, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{20, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{21, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{22, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{23, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{24, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
	{25, 0x2 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},

#endif

#ifdef CONFIG_INPUT_DEV_ACTS_IRKEY
	//{27, 0xB | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},	/* IR_RX */
#endif

#ifdef CONFIG_ACTS_TIMER3_CAPTURE
	{TIMER3_CAPTURE_GPIO, 0xE | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP},/* TIMER3_CAPIN */
#endif

#ifdef CONFIG_TRACING_BACKEND_SPI
	{20, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, /* SPI1_SS */
	{21, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, /* SPI1_SCLK */
	{22, 0x5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, /* SPI1_MOSI */
#endif

	{ 2,GPIO_CTL_MFP_GPIO|GPIO_CTL_SMIT|GPIO_CTL_PADDRV_LEVEL(3)|GPIO_CTL_PULLDOWN|GPIO_CTL_GPIO_OUTEN},

};

static const audio_input_map_t board_audio_input_map[] =  {
	{AUDIO_LINE_IN0, INPUT1L_SINGLE_END, INPUT1R_SINGLE_END},
	{AUDIO_LINE_IN1, INPUT2L_SINGLE_END, INPUT2R_SINGLE_END},
	{AUDIO_ANALOG_MIC0, INPUT0LR_DIFF, INPUT_RCH_DISABLE},
	{AUDIO_ANALOG_FM0, INPUT2L_SINGLE_END, INPUT2R_SINGLE_END},
	{AUDIO_DIGITAL_MIC0, INPUT_LCH_DMIC, INPUT_RCH_DMIC},
	{AUDIO_LINE_IN0_AMIC0, INPUT1L_SINGLE_END, INPUT0R_SINGLE_END},
	{AUDIO_ANALOG_FM0_AMIC0, INPUT2L_SINGLE_END, INPUT0R_SINGLE_END},
};

int board_audio_input_function_switch(audio_input_dev_e input_dev, u8_t *lch_input, u8_t *rch_input)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(board_audio_input_map); i++) {
		if (input_dev == board_audio_input_map[i].audio_dev) {
			*lch_input = board_audio_input_map[i].left_input;
			*rch_input = board_audio_input_map[i].right_input;
			break;
		}
	}

	if (i == ARRAY_SIZE(board_audio_input_map)) {
		printk("can not find out audio dev %d\n", input_dev);
		return -ENOENT;
	}

	return 0;
}

#ifdef BOARD_SDCARD_POWER_EN_GPIO

#define SD_CARD_POWER_RESET_MS	80

#define SD0_CMD_PIN		0
#define SD0_D0_PIN		2

static int pinmux_sd0_cmd, pinmux_sd0_d0;
static int sd_card_reset_ms;

static void board_mmc0_pullup_disable(void)
{
	struct device *sd_gpio_dev;

	sd_gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
	if (!sd_gpio_dev)
		return;

	/* backup origin pinmux config */
	acts_pinmux_get(SD0_CMD_PIN, &pinmux_sd0_cmd);
	acts_pinmux_get(SD0_D0_PIN, &pinmux_sd0_d0);

	/* sd_cmd pin output low level to avoid leakage */
	gpio_pin_configure(sd_gpio_dev, SD0_CMD_PIN, GPIO_DIR_OUT);
	gpio_pin_write(sd_gpio_dev, SD0_CMD_PIN, 0);

	/* sd_d0 pin output low level to avoid leakage */
	gpio_pin_configure(sd_gpio_dev, SD0_D0_PIN, GPIO_DIR_OUT);
	gpio_pin_write(sd_gpio_dev, SD0_D0_PIN, 0);
}

static void board_mmc0_set_input(void)
{
	struct device *sd_gpio_dev;

	sd_gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
	if (!sd_gpio_dev)
		return;

	/* cmd/d0 set input state to avoid VCC voltage drop too much */
	gpio_pin_configure(sd_gpio_dev, SD0_CMD_PIN, GPIO_DIR_IN);
	gpio_pin_configure(sd_gpio_dev, SD0_D0_PIN, GPIO_DIR_IN);

	k_sleep(10);
}

static void board_mmc0_pullup_enable(void)
{
	/* restore origin pullup pinmux config */
	acts_pinmux_set(SD0_CMD_PIN, pinmux_sd0_cmd);
	acts_pinmux_set(SD0_D0_PIN, pinmux_sd0_d0);
}

static int board_mmc_power_gpio_reset(struct device *power_gpio_dev, int power_gpio)
{
	gpio_pin_configure(power_gpio_dev, power_gpio,
			   GPIO_DIR_OUT);

	/* 0: power on, 1: power off */
	/* card vcc power off */
	gpio_pin_write(power_gpio_dev, power_gpio, 1);

	/* disable mmc0 pull-up to avoid leakage */
	board_mmc0_pullup_disable();

	k_sleep(sd_card_reset_ms);

	board_mmc0_set_input();

	/* card vcc power on */
	gpio_pin_write(power_gpio_dev, power_gpio, 0);

	k_sleep(10);

	/* restore mmc0 pull-up */
	board_mmc0_pullup_enable();

	return 0;
}
#endif	/* BOARD_SDCARD_POWER_EN_GPIO */

int board_mmc_power_reset(int mmc_id, u8_t cnt)
{
#ifdef BOARD_SDCARD_POWER_EN_GPIO
	struct device *power_gpio_dev;
#endif

	if (mmc_id != 0)
		return 0;

#ifdef BOARD_SDCARD_POWER_EN_GPIO
	power_gpio_dev = device_get_binding(BOARD_SDCARD_POWER_EN_GPIO_NAME);
	if (!power_gpio_dev)
		return -EINVAL;

	sd_card_reset_ms = cnt * SD_CARD_POWER_RESET_MS;
	if (sd_card_reset_ms <= 0)
		sd_card_reset_ms = SD_CARD_POWER_RESET_MS;

	board_mmc_power_gpio_reset(power_gpio_dev, BOARD_SDCARD_POWER_EN_GPIO);

#if defined(BOARD_SDCARD_DETECT_GPIO) && (BOARD_SDCARD_DETECT_GPIO == BOARD_SDCARD_POWER_EN_GPIO)
	/* switch gpio function to input for detecting card plugin */
	gpio_pin_configure(power_gpio_dev, BOARD_SDCARD_DETECT_GPIO, GPIO_DIR_IN);
	acts_pinmux_set(BOARD_SDCARD_DETECT_GPIO, GPIO_CTL_PULLUP);
#endif

#endif	/* BOARD_SDCARD_POWER_EN_GPIO */

	return 0;
}

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE

#define EXTERN_PA_CTL1_PIN  13
#define EXTERN_PA_CTL2_PIN  12
#define EXTERN_PA_CTL_SLEEP_TIME_MS 120

int board_extern_pa_ctl(u8_t pa_class, bool is_on)
{
	struct device *pa_gpio_dev;

	pa_gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
	if (!pa_gpio_dev)
		return -EINVAL;

	if (!is_on) {
		//PA shutoff ,  pa_ctl1=0, pa_ctl2=0
		printk("%d, ext_pa shutoff!\n", __LINE__);
	#ifdef  EXTERN_PA_CTL1_PIN
		gpio_pin_configure(pa_gpio_dev, EXTERN_PA_CTL1_PIN, GPIO_DIR_OUT);
		gpio_pin_write(pa_gpio_dev, EXTERN_PA_CTL1_PIN, 0);
	#endif
	#ifdef EXTERN_PA_CTL2_PIN
		gpio_pin_configure(pa_gpio_dev, EXTERN_PA_CTL2_PIN, GPIO_DIR_OUT);
		gpio_pin_write(pa_gpio_dev, EXTERN_PA_CTL2_PIN, 0);
	#endif

	} else {
		// open
		if (EXT_PA_CLASS_AB == pa_class) {
			printk("%d, ext_pa classAB!\n", __LINE__);
#ifdef	EXTERN_PA_CTL1_PIN
			gpio_pin_configure(pa_gpio_dev, EXTERN_PA_CTL1_PIN, GPIO_DIR_OUT);
			gpio_pin_write(pa_gpio_dev, EXTERN_PA_CTL1_PIN, 1);
#endif
#ifdef EXTERN_PA_CTL2_PIN
			gpio_pin_configure(pa_gpio_dev, EXTERN_PA_CTL2_PIN, GPIO_DIR_OUT);
			gpio_pin_write(pa_gpio_dev, EXTERN_PA_CTL2_PIN, 0);
#endif
		} else if (EXT_PA_CLASS_D == pa_class) {
			// classD mode, pa_ctl1=1,	pa_ctl2=1
			printk("%d, ext_pa classD!\n", __LINE__);
#ifdef	EXTERN_PA_CTL1_PIN
			gpio_pin_configure(pa_gpio_dev, EXTERN_PA_CTL1_PIN, GPIO_DIR_OUT);
			gpio_pin_write(pa_gpio_dev, EXTERN_PA_CTL1_PIN, 1);
#endif
#ifdef EXTERN_PA_CTL2_PIN
			gpio_pin_configure(pa_gpio_dev, EXTERN_PA_CTL2_PIN, GPIO_DIR_OUT);
			gpio_pin_write(pa_gpio_dev, EXTERN_PA_CTL2_PIN, 1);
#endif
		} else {
			printk("%d, ext_pa mode error!\n", __LINE__);
			return -EINVAL;
		}
	}

	k_sleep(EXTERN_PA_CTL_SLEEP_TIME_MS);

	return 0;
}
#endif

#ifdef CONFIG_SOC_MAPPING_PSRAM
void board_init_psram_pins(void)
{
	acts_pinmux_setup_pins(board_pin_psram_config, ARRAY_SIZE(board_pin_psram_config));

}
#endif

#ifdef CONFIG_HOTPLUG_DC5V_DETECT
void board_init_dc5v_detect_pin(void)
{
	u32_t value;
	/* wio0 as digital in */
	value = sys_read32(WIO0_CTL + CONFIG_HOTPLUG_DC5V_WIO*0x04);
	value = value & ~(1 << 4);
	value = value & ~(1 << 6);
	value = value | (1 << 7);
	sys_write32(value, WIO0_CTL + CONFIG_HOTPLUG_DC5V_WIO*0x04);
	k_busy_wait(200);
}
#endif

#define EN_ADFU_PRODUCT_KEY
#define EN_CARD_PRODUCT_KEY
//#define EN_ADFU_PRODUCT_KEY_POST

#ifdef EN_ADFU_PRODUCT_KEY
void check_adfu_product_key(void)
{
	int i;
	int adc_val;

    /* TODO: add usb device plug in detect, to enter adfu require usb device plug in */

	for (i = 0; i < 3; i++) {
		adc_val = sys_read32(ADC_CHAN_DATA_REG(PMUADC_CTL, CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_CHAN));
		if (adc_val > 10) {
			break;
		} else {
			k_busy_wait(10 * 1000);
		}
	}

	if (i >= 3) {
		BROM_PRINTF("adfu key -> adfu\n");
	#ifdef BOARD_POWER_LOCK_GPIO
		sys_write32(GPIO_CTL_GPIO_OUTEN | GPIO_CTL_MFP_GPIO, GPIO_REG_CTL(GPIO_REG_BASE, BOARD_POWER_LOCK_GPIO));
		sys_write32(GPIO_BIT(BOARD_POWER_LOCK_GPIO), GPIO_REG_BSR(GPIO_REG_BASE, BOARD_POWER_LOCK_GPIO));
	#endif
		BROM_ADFU_LAUNCHER(1);
	}
}
#endif

#ifdef EN_CARD_PRODUCT_KEY
void check_card_product_key(void)
{
	int i;
	int adc_val;

	for (i = 0; i < 3; i++) {
		adc_val = sys_read32(ADC_CHAN_DATA_REG(PMUADC_CTL, CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_CHAN));
		if (adc_val > 10) {
			break;
		} else {
			k_busy_wait(10 * 1000);
		}
	}

	if (i >= 3) {
		BROM_PRINTF("adfu key -> card product\n");
	#ifdef BOARD_POWER_LOCK_GPIO
		sys_write32(GPIO_CTL_GPIO_OUTEN | GPIO_CTL_MFP_GPIO, GPIO_REG_CTL(GPIO_REG_BASE, BOARD_POWER_LOCK_GPIO));
		sys_write32(GPIO_BIT(BOARD_POWER_LOCK_GPIO), GPIO_REG_BSR(GPIO_REG_BASE, BOARD_POWER_LOCK_GPIO));
	#endif
		BROM_CARD_LAUNCHER();
	}
}
#endif

void check_card_product_att(void)
{
	if (soc_boot_get_watchdog_is_reboot() == 1) {
		u16_t reboot_type;
		u8_t reason;
		if (!sys_pm_get_reboot_reason(&reboot_type, &reason)) {
			if (REBOOT_REASON_GOTO_PROD_CARD_ATT == reason) {
				sys_pm_set_product_flag(PRODUCT_FLAG_TYPE_ATT_CARD);

				BROM_PRINTF("att -> card product\n");
				BROM_CARD_LAUNCHER();
			}
		}
	}
}

void check_after_card_product_att_finish(void)
{
	if (sys_pm_get_product_flag() == PRODUCT_FLAG_TYPE_ATT_CARD) {
		soc_boot_user_set_card_product_att_reboot(true);
	} else {
		soc_boot_user_set_card_product_att_reboot(false);
	}
}

static int board_keyadc_init(void)
{
	u32_t value = 0;
	u32_t pmuadc_ctl_val;

	/* LRADC1: wio0  */
	value = sys_read32(WIO0_CTL);
	value = value | (1 << 4);
	value = value & (~(1 << 7));
	sys_write32(value, WIO0_CTL);
	k_busy_wait(200);
	/* TODO: add lradc1 init */

	acts_clock_peripheral_enable(CLOCK_ID_LRADC); /*default device clock enable*/
	pmuadc_ctl_val = sys_read32(PMUADC_CTL);
	if (!(pmuadc_ctl_val & BIT(CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_CHAN))) {
		/* enable adc channel if channel is not enabled */
		pmuadc_ctl_val |= BIT(CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_CHAN);
		sys_write32(pmuadc_ctl_val, PMUADC_CTL);
		k_busy_wait(1000);
	}

	return 0;
}

static int board_early_init(struct device *arg)
{
	ARG_UNUSED(arg);

	u32_t value = 0;

	board_keyadc_init();

	check_after_card_product_att_finish();

	check_card_product_att();

#ifdef EN_CARD_PRODUCT_KEY
    check_card_product_key();
#endif

#ifdef EN_ADFU_PRODUCT_KEY
	check_adfu_product_key();
#endif

	/* bandgap has filter resistor  */
	value = sys_read32(BDG_CTL);
	value = (value | (1 << 6));
	sys_write32(value, BDG_CTL);
	k_busy_wait(200);

#ifdef CONFIG_PWM_ACTS
	//acts_pinmux_setup_pins(board_led_pin_config, ARRAY_SIZE(board_led_pin_config));
#endif

#ifndef CONFIG_WATCHDOG
	sys_write32(sys_read32(WD_CTL) & (~(1 << 4)), WD_CTL);
#endif

	acts_pinmux_setup_pins(board_pin_config, ARRAY_SIZE(board_pin_config));

#ifdef BOARD_POWER_LOCK_GPIO
	sys_write32(GPIO_CTL_GPIO_OUTEN | GPIO_CTL_MFP_GPIO, GPIO_REG_CTL(GPIO_REG_BASE, BOARD_POWER_LOCK_GPIO));
	sys_write32(GPIO_BIT(BOARD_POWER_LOCK_GPIO), GPIO_REG_BSR(GPIO_REG_BASE, BOARD_POWER_LOCK_GPIO));
#endif

#ifdef CONFIG_ACTS_TIMER3_CAPTURE
	sys_write32(sys_read32(GPIO_CTL(TIMER3_CAPTURE_GPIO)) | GPIO_CTL_GPIO_INEN, GPIO_CTL(TIMER3_CAPTURE_GPIO));
#endif

#ifdef CONFIG_HOTPLUG_DC5V_DETECT
	board_init_dc5v_detect_pin();
#endif

#ifdef CONFIG_CPU0_EJTAG_ENABLE
	soc_debug_enable_jtag(SOC_JTAG_CPU0, CONFIG_CPU0_EJTAG_GROUP);
#else
	soc_debug_disable_jtag(SOC_JTAG_CPU0);
#endif

	return 0;
}

#ifdef EN_ADFU_PRODUCT_KEY_POST

static int adfukey_press_cnt;
static int adfukey_invalid;
static struct input_value adfukey_last_val;

static void board_key_input_notify(struct device *dev, struct input_value *val)
{
	/* any adc key is pressed? */
	if (val->code == KEY_RESERVED) {
		adfukey_invalid = 1;
		return;
	}

	adfukey_press_cnt++;

	/* save last adfu key value */
	if (adfukey_press_cnt == 1) {
		adfukey_last_val = *val;
		return;
	}

	/* key code must be same with last code and status is pressed */
	if (adfukey_last_val.type != val->type ||
		adfukey_last_val.code != val->code ||
		val->code == 0) {
		adfukey_invalid = 1;
	}
}

static void check_adfu_key(void)
{
	struct device *adckey_dev;

	/* check adfu */
	adckey_dev = device_get_binding(CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME);
	if (!adckey_dev) {
		printk("%s: error \n", __func__);
		return;
	}

	input_dev_enable(adckey_dev);
	input_dev_register_notify(adckey_dev, board_key_input_notify);

	/* wait adfu key */
	k_sleep(80);

	if (adfukey_press_cnt > 0 && !adfukey_invalid) {
		/* check again */
		k_sleep(120);

		if (adfukey_press_cnt > 1 && !adfukey_invalid) {
			printk("adfukey detected, goto adfu!\n");
			/* adfu key is pressed, goto adfu! */
			BROM_ADFU_LAUNCHER(1);
		}
	}

	input_dev_unregister_notify(adckey_dev, board_key_input_notify);
	input_dev_disable(adckey_dev);
}
#endif

static int board_later_init(struct device *arg)
{
	ARG_UNUSED(arg);

	printk("%s %d\n", __func__, __LINE__);

#ifdef CONFIG_USP
	struct device *usp_dev = device_get_binding(BOARD_OUTSPDIF_START_GPIO_NAME);
	gpio_pin_configure(usp_dev, BOARD_OUTSPDIF_START_GPIO_CONTROLLER, GPIO_DIR_OUT );
	gpio_pin_write(usp_dev, BOARD_OUTSPDIF_START_GPIO_CONTROLLER, 0);
#endif

#ifdef EN_ADFU_PRODUCT_KEY_POST
	check_adfu_key();
#endif

	return 0;
}

SYS_INIT(board_early_init, PRE_KERNEL_1, 5);

SYS_INIT(board_later_init, POST_KERNEL, 5);
