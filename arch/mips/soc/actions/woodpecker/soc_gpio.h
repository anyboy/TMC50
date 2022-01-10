/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file GPIO/PINMUX configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_GPIO_H_
#define	_ACTIONS_SOC_GPIO_H_

#define GPIO_MAX_PIN_NUM			53
#define GPIO_MAX_GROUPS				(((GPIO_MAX_PIN_NUM) + 31) / 32)

#define GPIO_WIO_MAX_PIN_NUM			3
#define GPIO_WIO_REG_BASE			(GPIO_REG_BASE + 0x200)

#define GPIO_CTL0				0x0
#define GPIO_ODAT0				0x100
#define GPIO_BSR0				0x108
#define GPIO_BRR0				0x110
#define GPIO_IDAT0				0x118
#define GPIO_IRQ_PD0				0x120

#define GPIO_REG_CTL(base, pin)			((base) + GPIO_CTL0 + (pin) * 4)
#define GPIO_REG_ODAT(base, pin)		((base) + GPIO_ODAT0 + (pin) / 32 * 4)
#define GPIO_REG_IDAT(base, pin)		((base) + GPIO_IDAT0 + (pin) / 32 * 4)
#define GPIO_REG_BSR(base, pin)			((base) + GPIO_BSR0 + (pin) / 32 * 4)
#define GPIO_REG_BRR(base, pin)			((base) + GPIO_BRR0 + (pin) / 32 * 4)
#define GPIO_REG_IRQ_PD(base, pin)		((base) + GPIO_IRQ_PD0 + (pin) / 32 * 4)
#define GPIO_BIT(pin)				(1 << ((pin) % 32))

#define GPIO_CTL_MFP_SHIFT			(0)
#define GPIO_CTL_MFP_MASK			(0xf << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP_GPIO			(0x0 << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_AD_SELECT			(0x1 << 4)
#define GPIO_CTL_SMIT				(0x1 << 5)
#define GPIO_CTL_GPIO_OUTEN			(0x1 << 6)
#define GPIO_CTL_GPIO_INEN			(0x1 << 7)
#define GPIO_CTL_PULL_MASK			(0xf << 8)
#define GPIO_CTL_PULLUP_STRONG		(0x1 << 11)
#define GPIO_CTL_PULLDOWN			(0x1 << 9)
#define GPIO_CTL_PULLUP				(0x1 << 8)
#define GPIO_CTL_PADDRV_SHIFT			(12)
#define GPIO_CTL_PADDRV_LEVEL(x)		((x) << GPIO_CTL_PADDRV_SHIFT)
#define GPIO_CTL_PADDRV_MASK			GPIO_CTL_PADDRV_LEVEL(0x7)
#define GPIO_CTL_INTC_EN			(0x1 << 20)
#define GPIO_CTL_INC_TRIGGER_SHIFT		(21)
#define GPIO_CTL_INC_TRIGGER(x)			((x) << GPIO_CTL_INC_TRIGGER_SHIFT)
#define GPIO_CTL_INC_TRIGGER_MASK		GPIO_CTL_INC_TRIGGER(0x7)
#define GPIO_CTL_INC_TRIGGER_RISING_EDGE	GPIO_CTL_INC_TRIGGER(0x0)
#define GPIO_CTL_INC_TRIGGER_FALLING_EDGE	GPIO_CTL_INC_TRIGGER(0x1)
#define GPIO_CTL_INC_TRIGGER_DUAL_EDGE		GPIO_CTL_INC_TRIGGER(0x2)
#define GPIO_CTL_INC_TRIGGER_HIGH_LEVEL		GPIO_CTL_INC_TRIGGER(0x3)
#define GPIO_CTL_INC_TRIGGER_LOW_LEVEL		GPIO_CTL_INC_TRIGGER(0x4)
#define GPIO_CTL_INTC_MASK			(0x1 << 25)

#define GPIO_CTL(x)                 (GPIO_REG_BASE + 4 * (x))

#define PINMUX_MODE_MASK			(GPIO_CTL_MFP_MASK | GPIO_CTL_AD_SELECT | \
						 GPIO_CTL_PULL_MASK | GPIO_CTL_PADDRV_MASK | \
						 GPIO_CTL_SMIT)

#define WIO_INDEX		            56
#define WIO0_CTL_WIODAT             16

/* GPIO&MFP bits */
#define GPIO50_CTL_AD_SELECT          4
#define GPIO51_CTL_AD_SELECT          4
#define GPIO52_CTL_AD_SELECT          4


#endif /* _ACTIONS_SOC_GPIO_H_	*/
