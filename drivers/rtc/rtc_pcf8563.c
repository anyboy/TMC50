/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <rtc.h>
#include <soc.h>
#include <i2c.h>

#define SYS_LOG_DOMAIN "pcf8563"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#define PCF8563_DEFAULT_INIT_TIME	1483200000	/* Sun Jan 01 00:00:00 2017 */

#define PCF8563_I2C_ADDR	0x51

#define PCF8563_REG_ST1		0x00 /* status */
#define PCF8563_REG_ST2		0x01

#define PCF8563_REG_SC		0x02 /* datetime */
#define PCF8563_REG_MN		0x03
#define PCF8563_REG_HR		0x04
#define PCF8563_REG_DM		0x05
#define PCF8563_REG_DW		0x06
#define PCF8563_REG_MO		0x07
#define PCF8563_REG_YR		0x08

#define PCF8563_REG_AMN		0x09 /* alarm */
#define PCF8563_REG_AHR		0x0A
#define PCF8563_REG_ADM		0x0B
#define PCF8563_REG_ADW		0x0C

#define PCF8563_REG_CLKO	0x0D /* clock out */
#define PCF8563_REG_TMRC	0x0E /* timer control */
#define PCF8563_REG_TMR		0x0F /* timer */

#define PCF8563_SC_LV		0x80 /* low voltage */
#define PCF8563_MO_C		0x80 /* century */

/* Driver instance data */
struct pcf8563_info {
	struct device *i2c;

	void (*alarm_cb_fn)(struct device *dev);
};

static unsigned bcd2bin(unsigned char val)
{
	return (val & 0x0f) + (val >> 4) * 10;
}

static unsigned char bin2bcd(unsigned val)
{
	return ((val / 10) << 4) + val % 10;
}
#if 0
static void pcf8563_dump_regs(struct pcf8563_info *pcf8563)
{
	unsigned char buf[16];
	int ret, i;

	printk("pcf8563 registers:\n");
	ret = i2c_burst_read(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_ST1, buf, 16);
	if (ret) {
		SYS_LOG_ERR("failed to read register, ret %d\n", ret);
		return;
	}

	for (i = 0; i < 16; i++) {
		printk("  [%02d] 0x%02x\n", i, buf[i]);
	}
}
#endif

static int pcf8563_get_datetime(struct pcf8563_info *pcf8563, struct rtc_time *tm)
{
	int ret;
	unsigned char buf[7];

	ret = i2c_burst_read(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_SC, buf, 7);
	if (ret) {
		SYS_LOG_ERR("failed to get datetime");
		return ret;
	}

	if (buf[0] & PCF8563_SC_LV) {
		SYS_LOG_ERR("low voltage detected, date/time is not reliable");
	}
	tm->tm_sec = bcd2bin(buf[0] & 0x7f);
	tm->tm_min = bcd2bin(buf[1] & 0x7f);
	tm->tm_hour = bcd2bin(buf[2] & 0x3f); /* rtc hr 0-23 */
	tm->tm_mday = bcd2bin(buf[3] & 0x3f);
	tm->tm_wday = bcd2bin(buf[4] & 0x7);
	tm->tm_mon = bcd2bin(buf[5] & 0x1f) - 1; /* rtc mon 1-12 */
	tm->tm_year = bcd2bin(buf[6]);

	if (tm->tm_year < 70)
		tm->tm_year += 100;	/* assume we are in 1970...2069 */

	/* the clock can give out invalid datetime, but we cannot return
	 * -EINVAL otherwise hwclock will refuse to set the time on bootup.
	 */
	if (rtc_valid_tm(tm) < 0)
		printk("pcf8563: retrieved date/time is not valid.\n");

	return 0;
}

static int pcf8563_set_datetime(struct pcf8563_info *pcf8563, struct rtc_time *tm)
{
	unsigned char buf[8];
	int ret;

	SYS_LOG_INF("set datetime: %04d.%02d.%02d  %02d:%02d:%02d",
		tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	buf[0] = bin2bcd(tm->tm_sec);
	buf[1] = bin2bcd(tm->tm_min);
	buf[2] = bin2bcd(tm->tm_hour);
	buf[3] = bin2bcd(tm->tm_mday);
	buf[4] = tm->tm_wday & 0x07;
	buf[5] = bin2bcd(tm->tm_mon + 1);
	buf[6] =  bin2bcd(tm->tm_year % 100);

	ret = i2c_burst_write(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_SC, buf, 7);
	if (ret) {
		SYS_LOG_ERR("failed to set datetime");
		return ret;
	}

	return 0;
}

static int pcf8563_set_alarm_interrupt(struct pcf8563_info *pcf8563, int enable)
{
	unsigned char val;

	val = enable ? 0x2 : 0x0;
		
	return i2c_reg_write_byte(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_ST2, val);
}

static int pcf8563_set_alarm_time(struct pcf8563_info *pcf8563, struct rtc_time *tm)
{
	int ret;
	unsigned char buf[4];

	SYS_LOG_INF("set alarm: %04d.%02d.%02d  %02d:%02d:%02d",
		tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* disable alarm interrupt */
	ret = pcf8563_set_alarm_interrupt(pcf8563, false);
	if (ret)
		return ret;

	buf[0] = bin2bcd(tm->tm_min);
	buf[1] = bin2bcd(tm->tm_hour);
	buf[2] = bin2bcd(tm->tm_mday);
	buf[3] = (tm->tm_wday & 0x07);
	ret = i2c_burst_write(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_AMN, buf, 4);
	if (ret) {
		SYS_LOG_ERR("failed to set alarm");
		return ret;
	}

	/* enable alarm interrupt */
	ret = pcf8563_set_alarm_interrupt(pcf8563, true);
	if (ret)
		return ret;

	return 0;
}

static int pcf8563_set_alarm(struct device *dev, const uint32_t alarm_val)
{
	struct pcf8563_info *pcf8563 = dev->driver_data;
	struct rtc_time tm;

	SYS_LOG_INF("set alarm: %d", alarm_val);

	rtc_time_to_tm(alarm_val, &tm);

	return pcf8563_set_alarm_time(pcf8563, &tm);
}

static void pcf8563_enable(struct device *dev){}
static void pcf8563_disable(struct device *dev){}

static uint32_t pcf8563_read(struct device *dev)
{
	struct pcf8563_info *pcf8563 = dev->driver_data;
	struct rtc_time tm;
	uint32_t time;
	int ret;

	ret = pcf8563_get_datetime(pcf8563, &tm);
	if (ret) {
		SYS_LOG_ERR("failed to get datetime");
		return 0;
	}

	rtc_tm_to_time(&tm, &time);

	return time;
}

static int pcf8563_set_config(struct device *dev, struct rtc_config *cfg)
{
	struct pcf8563_info *pcf8563 = dev->driver_data;
	struct rtc_time tm;

	if (cfg->init_val) {
		rtc_time_to_tm(cfg->init_val, &tm);
		pcf8563_set_datetime(pcf8563, &tm);
	}
	
	if (cfg->alarm_enable) {
		pcf8563->alarm_cb_fn = cfg->cb_fn;
		pcf8563_set_alarm(dev, cfg->alarm_val);
	} else {
		/* disable alarm */
		pcf8563->alarm_cb_fn = NULL;
		pcf8563_set_alarm_interrupt(pcf8563, false);
	}

	return 0;
}


static uint32_t pcf8563_get_pending_int(struct device *dev)
{
	struct pcf8563_info *pcf8563 = dev->driver_data;
	unsigned char val = 0;
	int pending;

	i2c_reg_read_byte(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_ST2, &val);

	pending = val & 0x4;
	if (pending) {
		/* clear pending */
		val &= ~0x4;
		i2c_reg_write_byte(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_ST2, val);
	}

	return pending;
}

static void pcf8563_check_status(struct pcf8563_info *pcf8563)
{
	unsigned char val = 0;

	i2c_reg_read_byte(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_ST1, &val);
	if (val != 0x08) {
		SYS_LOG_ERR("rtc is stoped, enable it!");

		/* rtc is stop, set to run */
		i2c_reg_write_byte(pcf8563->i2c, PCF8563_I2C_ADDR, PCF8563_REG_ST1, 0x08);
	}
}

static int pcf8563_check_datetime(struct pcf8563_info *pcf8563)
{
	struct rtc_time tm;
	int ret;

	pcf8563_check_status(pcf8563);

	ret = pcf8563_get_datetime(pcf8563, &tm);
	if (ret)
		return ret;

	if (rtc_valid_tm(&tm) < 0) {
		SYS_LOG_ERR("invalid date, set default date");

		rtc_time_to_tm(PCF8563_DEFAULT_INIT_TIME, &tm);
		pcf8563_set_datetime(pcf8563, &tm);
	}

	printk("Current Time: ");
	print_rtc_time(&tm);

	return 0;
}

static const struct rtc_driver_api pcf8563_rtc_api = {
	.enable = pcf8563_enable,
	.disable = pcf8563_disable,
	.read = pcf8563_read,
	.set_config = pcf8563_set_config,
	.set_alarm = pcf8563_set_alarm,
	.get_pending_int = pcf8563_get_pending_int,
};

static int pcf8563_init(struct device *dev)
{
	struct pcf8563_info *pcf8563 = dev->driver_data;

	SYS_LOG_INF("init pcf8563");

	pcf8563->i2c = device_get_binding(CONFIG_RTC_PCF8563_I2C_NAME);
	if (!pcf8563->i2c) {
		SYS_LOG_ERR("Device not found\n");
		return -1;
	}

	pcf8563_check_datetime(pcf8563);

	/* pcf8563_dump_regs(pcf8563); */

	/* clear old alarm pending */
	pcf8563_get_pending_int(dev);

	dev->driver_api = &pcf8563_rtc_api;

	return 0;
}

static struct pcf8563_info pcf8563_info;

DEVICE_INIT(rtc, CONFIG_RTC_0_NAME, pcf8563_init, \
	    &pcf8563_info, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
