/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>
#include <logging/sys_log.h>

char syslog_log_level = CONFIG_SYS_USER_LOG_DEFAULT_LEVEL;

void syslog_set_log_level(int log_level)
{
	syslog_log_level = log_level;
}

void syslog_hook_default(const char *fmt, ...)
{
	(void)(fmt);  /* Prevent warning about unused argument */
}

void (*syslog_hook)(const char *fmt, ...) = (void(*)(const char *fmt, ...))printk;

void syslog_hook_install(void (*hook)(const char *, ...))
{
	syslog_hook = hook;
}
