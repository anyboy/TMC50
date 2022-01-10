/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file alarm manager interface
 */

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "alarm_manager"
#include <logging/sys_log.h>
#endif
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rtc.h>
#include "stream.h"
#include "mem_manager.h"
#include <alarm_manager.h>
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
#include <soc.h>

#define ALARM_DEFAULF_TIME	(7*60*60)/*default alarm time 07:00:00*/
static struct alarm_manager al_manager;
alarm_callback call_back;
static u8_t get_alarm_times;

static void alarm_manager_callback(const void *cb_data);

static void tm_to_time(struct rtc_time *tm, u32_t *time)
{
	*time = (tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec;
}
static void tm_add_one_day(struct rtc_time *tm)
{
	tm->tm_mday += 1;
	if (tm->tm_mday > rtc_month_days(tm->tm_mon - 1, tm->tm_year - 1900)) {
		/* alarm day 1-31 */
		tm->tm_mday = 1;
		tm->tm_mon += 1;
		if (tm->tm_mon >= 13) {
			/* alarm mon 1-12 */
			tm->tm_mon = 1;
			tm->tm_year += 1;
		}
	}
}
static void time_to_tm(struct rtc_time *tm, u32_t time)
{
	tm->tm_hour = time / 3600;
	tm->tm_min = time / 60 - tm->tm_hour * 60;
	tm->tm_sec = time % 60;
}

int find_and_set_alarm(void)
{
	int ret = 0;
	int i = 0;
	int earliest_alarm = 0;
	struct alarm_info *alarm = NULL;
	struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);
	u32_t cur_time = 0;
	struct rtc_alarm_config config = {0};

	if (!rtc) {
		SYS_LOG_ERR("no alarm\n");
		return -1;
	}
	ret = rtc_get_time(rtc, &config.alarm_time);
	print_rtc_time(&config.alarm_time);
	if (ret) {
		SYS_LOG_ERR("get curtime error\n");
		return -1;
	}
	tm_to_time(&config.alarm_time, &cur_time);
	/*need to free current alarm*/
	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_OK) {
			al_manager.alarm[i].state = ALARM_STATE_FREE;
			break;
		}
	}
	i = 0;
	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		alarm = &al_manager.alarm[i];

		if ((alarm->state == ALARM_STATE_FREE)
				&& (alarm->alarm_time > cur_time)) {
			if (earliest_alarm == 0) {
				earliest_alarm = alarm->alarm_time;
			} else if (earliest_alarm > alarm->alarm_time) {
				earliest_alarm = alarm->alarm_time;
			}
		}
	}
	if (earliest_alarm == 0) {
		/*if all alarm is earlier than current time*/
		i = 0;
		for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
			alarm = &al_manager.alarm[i];
			if ((alarm->state == ALARM_STATE_FREE)
					&& (alarm->alarm_time <= cur_time)) {
				if (earliest_alarm == 0) {
					earliest_alarm = alarm->alarm_time;
				} else if (earliest_alarm > alarm->alarm_time) {
					earliest_alarm = alarm->alarm_time;
				}
			}
		}
	}
	if (earliest_alarm != 0) {
		time_to_tm(&config.alarm_time, earliest_alarm);
		if (earliest_alarm < cur_time) {
			tm_add_one_day(&config.alarm_time);
		}
		config.cb_fn = alarm_manager_callback;
		config.cb_data = NULL;
		ret = rtc_set_alarm(rtc, &config, true);
		if (ret) {
			SYS_LOG_ERR("set alarm error ret=%d\n", ret);
			return -1;
		}
		i = 0;
		for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
			alarm = &al_manager.alarm[i];
			if ((alarm->state == ALARM_STATE_FREE)
				&& (alarm->alarm_time == earliest_alarm)) {
				alarm->state = ALARM_STATE_OK;
				break;
			}
		}
	}
#ifdef CONFIG_PROPERTY
	ret = property_set(CFG_ALARM_INFO,
				 (char *)&al_manager, sizeof(struct alarm_manager));
#endif
	if (ret < 0) {
		SYS_LOG_ERR("failed to set config %s, ret %d\n",
						 CFG_ALARM_INFO, ret);
		return -1;
	}
	return 0;
}

static int is_alarm_exist(uint32_t time)
{
	int i = 0;

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if ((al_manager.alarm[i].alarm_time == time)
				&& (al_manager.alarm[i].state != ALARM_STATE_NULL)) {
			return 1;
		}
	}
	return 0;
}

static int is_need_set_alarm(struct device *dev, uint32_t time)
{
	int i = 0;
	u32_t cur_time = 0;
	struct rtc_time tm = {0};

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_OK) {
			break;
		}
	}
	if (i == MAX_ALARM_SUPPORT) {
		return 1;
	}
	int ret = rtc_get_time(dev, &tm);

	if (ret) {
		SYS_LOG_ERR("get curtime error\n");
		return 0;
	}
	tm_to_time(&tm, &cur_time);
	if (time > cur_time) {
		for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
			if (al_manager.alarm[i].state == ALARM_STATE_OK) {
				if (time < al_manager.alarm[i].alarm_time) {
					al_manager.alarm[i].state = ALARM_STATE_FREE;
					return 1;
				} else if (al_manager.alarm[i].alarm_time < cur_time) {
					al_manager.alarm[i].state = ALARM_STATE_FREE;
					return 1;
				}
				break;
			}
		}
	} else if (time < cur_time) {
		for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
			if (al_manager.alarm[i].state == ALARM_STATE_OK) {
				if ((time < al_manager.alarm[i].alarm_time)
					&& (al_manager.alarm[i].alarm_time < cur_time)) {
					al_manager.alarm[i].state = ALARM_STATE_FREE;
					return 1;
				}
				break;
			}
		}
	}
	return 0;
}

static int check_alarm_time(int time)
{
	if (time < 0 || time >= 86400)
		return -EINVAL;
	return 0;
}
static void alarm_manager_callback(const void *cb_data)
{
	int i = 0;

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_OK) {
			al_manager.alarm[i].state = ALARM_STATE_FREE;
			break;
		}
	}

	if (call_back != NULL) {
		call_back();
	}
}

int alarm_manager_init(void)
{
	struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm\n");
		return -1;
	}
	rtc_enable(rtc);
#if 0
	struct alarm_time tm;
	u32_t time_stamp = system_clock_init();

	alarm_time_to_tm(time_stamp, &tm)

	if (alarm_set_time(alarm, &tm)) {
		SYS_LOG_ERR("Failed to config RTC alarm\n");
		return -1;
	}
#endif
	memset(&al_manager, 0, sizeof(struct alarm_manager));
#ifdef CONFIG_PROPERTY
	int ret = property_get(CFG_ALARM_INFO,
				(char *)&al_manager, sizeof(struct alarm_manager));
#endif
	for (int i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_NULL &&
			al_manager.alarm[i].alarm_time != ALARM_DEFAULF_TIME) {
			al_manager.alarm[i].alarm_time = ALARM_DEFAULF_TIME;
		}
		SYS_LOG_DBG("alarm[%d]:state=%d,alarm_time=%d\n",
			i, al_manager.alarm[i].state, al_manager.alarm[i].alarm_time);
	}

	if (ret < 0) {
		SYS_LOG_ERR("failed to get config %s, ret %d\n", CFG_ALARM_INFO, ret);
		return -1;
	}

	return 0;
}

int alarm_manager_get_time(struct rtc_time *tm)
{
	struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);
	int ret = rtc_get_time(rtc, tm);

	if (ret) {
		SYS_LOG_ERR("get time error ret=%d\n", ret);
		return -1;
	}
	tm->tm_year += 1900;
	tm->tm_mon += 1;
	return ret;
}
int alarm_manager_set_time(struct rtc_time *tm)
{
	struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm device\n");
		return -1;
	}
	tm->tm_year -= 1900;
	tm->tm_mon -= 1;
	int ret = rtc_set_time(rtc, tm);

	if (ret) {
		SYS_LOG_ERR("set time error ret=%d\n", ret);
		return -1;
	}
	print_rtc_time(tm);
	return ret;
}
int alarm_manager_get_alarm(struct rtc_time *tm, bool *is_on)
{
	int ret = system_get_alarm(tm, is_on);

	if (ret) {
		SYS_LOG_ERR("get alarm error ret=%d\n", ret);
		return -1;
	}
	return ret;
}
int alarm_manager_set_alarm(struct rtc_time *tm)
{
	struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm device\n");
		return -1;
	}
	int ret = system_add_alarm(rtc, tm);

	if (ret) {
		SYS_LOG_ERR("set alarm error ret=%d\n", ret);
		return -1;
	}
	get_alarm_times--;
	return ret;
}
int alarm_manager_delete_alarm(struct rtc_time *tm)
{
	struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm device\n");
		return -1;
	}
	int ret = system_delete_alarm(rtc, tm);

	if (ret) {
		SYS_LOG_ERR("delete alarm error ret=%d\n", ret);
		return -1;
	}
	print_rtc_time(tm);
	return ret;
}

int system_add_alarm(struct device *dev, struct rtc_time *tm)
{
	int ret = 0;
	int i = 0;
	u32_t time = 0;
	struct rtc_alarm_config config = {0};

	tm->tm_mon--;/*need to -1*/
	if (rtc_valid_tm(tm)) {
		SYS_LOG_ERR("Bad time structure\n");
		return -ENOEXEC;
	}

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_NULL) {
			break;
		}
	}

	if (i == MAX_ALARM_SUPPORT) {
		SYS_LOG_ERR("alarm full!\n");
		return -2;
	}

	tm_to_time(tm, &time);
	if (is_alarm_exist(time)) {
		SYS_LOG_INF("alarm existed!\n");
		return -3;
	}
	if (is_need_set_alarm(dev, time)) {
		config.alarm_time.tm_year = tm->tm_year - 1900;
		config.alarm_time.tm_mon = tm->tm_mon;/*don't need to -1*/
		config.alarm_time.tm_mday = tm->tm_mday;
		config.alarm_time.tm_hour = tm->tm_hour;
		config.alarm_time.tm_min = tm->tm_min;
		config.alarm_time.tm_sec = tm->tm_sec;
		config.cb_fn = alarm_manager_callback;
		config.cb_data = NULL;

		SYS_LOG_INF("hour=%d,min=%d\n",
					config.alarm_time.tm_hour, config.alarm_time.tm_min);

		ret = rtc_set_alarm(dev, &config, true);
		if (ret) {
			SYS_LOG_ERR("set alarm error ret=%d\n", ret);
			return -1;
		}
		al_manager.alarm[i].state = ALARM_STATE_OK;
	} else {
		al_manager.alarm[i].state = ALARM_STATE_FREE;
		SYS_LOG_INF("add alarm:hour=%d,min=%d\n", tm->tm_hour, tm->tm_min);
	}
	al_manager.alarm[i].alarm_time = time;

#ifdef CONFIG_PROPERTY
	ret = property_set(CFG_ALARM_INFO,
				(char *)&al_manager, sizeof(struct alarm_manager));
	if (ret < 0) {
		SYS_LOG_ERR("failed to set config %s, ret %d\n", CFG_ALARM_INFO, ret);
		return -1;
	}
#endif
	return 0;
}

int system_delete_alarm(struct device *dev, struct rtc_time *tm)
{
	int i = 0;
	int ret = 0;
	u32_t time = 0;
	bool need_delete = false;
	struct rtc_alarm_config config = {0};

	tm_to_time(tm, &time);
	SYS_LOG_INF("delect time=%d\n", time);
	for (int j = 0; j < MAX_ALARM_SUPPORT; j++) {
		SYS_LOG_DBG("al_manager.alarm[%d].state=%d"
					",al_manager.alarm[%d].alarm_time=%d\n",
			j, al_manager.alarm[j].state, j, al_manager.alarm[j].alarm_time);
	}

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if ((al_manager.alarm[i].state != ALARM_STATE_NULL)
				&& (al_manager.alarm[i].alarm_time == time)) {
			need_delete = true;
			break;
		}
	}
	if (need_delete) {
		if (al_manager.alarm[i].state == ALARM_STATE_OK) {
			/*need remove alarm from alarm*/
			ret = rtc_set_alarm(dev, &config, false);
			if (ret) {
				SYS_LOG_ERR("set alarm error ret=%d\n", ret);
				return -1;
			}
			al_manager.alarm[i].state = ALARM_STATE_NULL;
			if (!find_and_set_alarm())
				return 0;
		} else {
			al_manager.alarm[i].state = ALARM_STATE_NULL;
		}
	#ifdef CONFIG_PROPERTY
		ret = property_set(CFG_ALARM_INFO,
				(char *)&al_manager, sizeof(struct alarm_manager));
		if (ret < 0) {
		SYS_LOG_ERR("failed to set config %s, ret %d\n",
					CFG_ALARM_INFO, ret);
			return -1;
		}
	#endif
	}
	return 0;
}

int system_get_alarm(struct rtc_time *tm, bool *is_on)
{
	for (int i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (get_alarm_times >= MAX_ALARM_SUPPORT)
			get_alarm_times = 0;
		SYS_LOG_INF("get_alarm_times=%d,alarm_time=%d\n",
			get_alarm_times, al_manager.alarm[get_alarm_times].alarm_time);
		if (check_alarm_time(al_manager.alarm[get_alarm_times].alarm_time)) {
			get_alarm_times++;
		} else {
			time_to_tm(tm, al_manager.alarm[get_alarm_times].alarm_time);
			SYS_LOG_INF("hour=%d,min=%d\n", tm->tm_hour, tm->tm_min);
			if (al_manager.alarm[get_alarm_times].state) {
				*is_on = true;
			} else {
				*is_on = false;
			}
			get_alarm_times++;
			break;
		}
	}
	return 0;
}

int system_registry_alarm_callback(alarm_callback callback)
{
	call_back = callback;
	return 0;
}
extern int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src);
bool alarm_wakeup_source_check(void)
{
	union sys_pm_wakeup_src src = {0};

	if (!sys_pm_get_wakeup_source(&src)) {
		SYS_LOG_INF("alarm=%d\n", src.t.alarm);
		if (src.t.alarm) {
			return true;
		}
	}
	return false;
}

