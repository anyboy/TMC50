/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <nvram_config.h>
#include <system_recovery.h>

#define SYS_LOG_DOMAIN "recovery"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <logging/sys_log.h>

#define RECOVERY_CONFIG_ENTER_RECOVERY		"RCFG_ENTER_RECOVERY"
#define RECOVERY_CONFIG_OTA_STATUS		"RCFG_OTA_STATUS"
#define RECOVERY_CONFIG_BOOT_FLAG		"RCFG_BOOT_FLAG"

#define OTA_STATUS_IN_PROCESS			0x52504e49 /* 'INPR' */
#define OTA_STATUS_DONE				0x454e4f44 /* 'DONE */

#define FLAG_ENTER_RECOVERY			0x4f434552 /* 'RECO */

/* the reason for why we need enter recovery */
static int system_recovery_reason;

int system_recovery_get_reason(void)
{
	return system_recovery_reason;
}

void system_recovery_reboot_recovery(void)
{
	int data;

	data = FLAG_ENTER_RECOVERY;
#ifdef CONFIG_NVRAM_CONFIG
	nvram_config_set(RECOVERY_CONFIG_ENTER_RECOVERY, &data, sizeof(data));
#endif
	/* boot to recovery */
	sys_pm_reboot(REBOOT_TYPE_GOTO_RECOVERY | REBOOT_REASON_OTA_FINISHED);
}

#ifdef CONFIG_SYSTEM_RECOVERY_CHECK_OTA_FLAG
int system_recovery_get_ota_flag(void)
{
	int data, ret;
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_get(RECOVERY_CONFIG_OTA_STATUS, &data, sizeof(data));
#endif
	if (ret < 0) {
		/* no ota flag? */
		if (ret == -ENOENT)
			return 0;
		return ret;
	}

	/* has ota flag */
	return 1;
}

int system_recovery_clear_ota_flag(void)
{
	int ret;

	SYS_LOG_INF("clear ota flag");

	/* remove ota flag config */
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_set(RECOVERY_CONFIG_OTA_STATUS, NULL, 0);
	if (ret) {
		SYS_LOG_ERR("failed to clear boot flag");
		return ret;
	}
#endif
	return 0;
}

int system_recovery_set_ota_flag(void)
{
	int data, ret;

	SYS_LOG_INF("set ota flag");
#ifdef CONFIG_NVRAM_CONFIG
	data = OTA_STATUS_IN_PROCESS;
	ret = nvram_config_set(RECOVERY_CONFIG_OTA_STATUS, &data, sizeof(data));
	if (ret) {
		SYS_LOG_ERR("failed to set ota flag");
		return ret;
	}
#endif
	return 0;
}
#endif

#ifdef CONFIG_SYSTEM_RECOVERY_CHECK_BOOT_FLAG
int system_recovery_get_boot_flag(void)
{
	int data, ret;
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_get(RECOVERY_CONFIG_BOOT_FLAG, &data, sizeof(data));
#endif
	if (ret < 0) {
		/* initial value */
		data = 0;
	}

	return data;
}

int system_recovery_clear_boot_flag(void)
{
	int data, ret;

	SYS_LOG_INF("clear boot flag");
#ifdef CONFIG_NVRAM_CONFIG
	data = 0;
	ret = nvram_config_set(RECOVERY_CONFIG_BOOT_FLAG, &data, sizeof(data));
	if (ret < 0) {
		SYS_LOG_ERR("failed to clear boot flag");
		return ret;
	}
#endif
	return 0;
}

int system_recovery_set_boot_flag(void)
{
	int data, ret;

	SYS_LOG_INF("set boot flag");

	/* get original boot flag */
	data = system_recovery_get_boot_flag();

	/* increase boot times, need clear after boot successfully in application */
	data++;
#ifdef CONFIG_NVRAM_CONFIG
	SYS_LOG_INF("set boot flag: %d", data);
	ret = nvram_config_set(RECOVERY_CONFIG_BOOT_FLAG, &data, sizeof(data));
	if (ret) {
		SYS_LOG_ERR("failed to set boot flag");
		return ret;
	}
#endif
	return 0;
}

int system_recovery_check_boot_flag(void)
{
	int data;

	SYS_LOG_DBG("check boot flag");

	data = system_recovery_get_boot_flag();
	if (data > RECOVERY_MAX_FAILED_BOOT_TIMES) {
		/* failed to boot too much, need enter recovery! */
		SYS_LOG_ERR("failed to boot %d times, enter recovery!", data);
		return 1;
	}

	return 0;
}
#endif

#ifdef CONFIG_SYSTEM_RECOVERY_APP
static int check_recovery_flags(void)
{
	unsigned int data;
	int ret;

	SYS_LOG_DBG("check recovery flag");
#ifdef CONFIG_NVRAM_CONFIG
	/* check enter recovery flag */
	ret = nvram_config_get(RECOVERY_CONFIG_ENTER_RECOVERY, &data, sizeof(data));
	if (ret > 0 && data == FLAG_ENTER_RECOVERY) {
		/* enter recovery forcely! */
		SYS_LOG_ERR("recovery flag is set, goto recovery!");
	#ifdef CONFIG_NVRAM_CONFIG
		/* clear enter recovery flag */
		nvram_config_set(RECOVERY_CONFIG_ENTER_RECOVERY, NULL, 0);
	#endif
		return RECOVERY_REASON_FORCELY;
	}
#endif

#ifdef CONFIG_SYSTEM_RECOVERY_CHECK_OTA_FLAG
	/* check ota-in-process flag */
	ret = system_recovery_get_ota_flag();
	if (ret) {
		/* OTA is not finished, need enter recovery! */
		SYS_LOG_ERR("OTA is not done normally, goto recovery!");
		return RECOVERY_REASON_OTA_FAILED;
	}
#endif

#ifdef CONFIG_SYSTEM_RECOVERY_CHECK_BOOT_FLAG
	/* check boot flag */
	ret = system_recovery_check_boot_flag();
	if (ret) {
		/* OTA is not finished, need enter recovery! */
		SYS_LOG_ERR("normal boot failed, goto recovery!");
		return RECOVERY_REASON_BOOT_FAILED;
	}

	/* set boot flag */
	ret = system_recovery_set_boot_flag();
	if (ret) {
		SYS_LOG_ERR("failed to set boot flag, goto recovery");
		return RECOVERY_REASON_BAD_CONFIG;
	}
#endif

	/* enter recovery forcely! */
	SYS_LOG_INF("normal boot");

	return RECOVERY_REASON_BOOT_NORMAL;
}
#endif

static int system_recovery_init(struct device *dev)
{
	int reason;

	ARG_UNUSED(dev);

	SYS_LOG_INF("int system recovery");

#ifdef CONFIG_SYSTEM_RECOVERY_APP
	reason = check_recovery_flags();
	if (reason == RECOVERY_REASON_BOOT_NORMAL) {
		/* boot normally */
		sys_pm_reboot(REBOOT_TYPE_GOTO_WIFISYS | REBOOT_REASON_OTA_FINISHED);

		/* never run to here */
	} else {
		SYS_LOG_ERR("enter recovery: reason %d", reason);
	}

#else
	reason = RECOVERY_REASON_BOOT_NORMAL;
#endif

	system_recovery_reason = reason;

	return 0;
}

SYS_INIT(system_recovery_init, PRE_KERNEL_1, CONFIG_SYSTEM_RECOVERY_INIT_PRIORITY);
