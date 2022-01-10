/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA upgrade interface
 */

#ifndef __OTA_UPGRADE_H__
#define __OTA_UPGRADE_H__

struct ota_upgrade_info;
struct ota_backend;

enum ota_state
{
	OTA_INIT = 0,
	OTA_RUNNING,
	OTA_DONE,
	OTA_FAIL,
};

typedef void (*ota_notify_t)(int state, int old_state);

struct ota_upgrade_param {
	const char *storage_name;
	ota_notify_t notify;

	/* flags */
	unsigned int flag_use_recovery:1;	/* use recovery */
	unsigned int flag_use_recovery_app:1;	/* use recovery app */
	unsigned int no_version_control:1; /* OTA without version control */
};

struct ota_upgrade_info *ota_upgrade_init(struct ota_upgrade_param *param);
int ota_upgrade_check(struct ota_upgrade_info *ota);
int ota_upgrade_is_in_progress(struct ota_upgrade_info *ota);

int ota_upgrade_attach_backend(struct ota_upgrade_info *ota, struct ota_backend *backend);
void ota_upgrade_detach_backend(struct ota_upgrade_info *ota, struct ota_backend *backend);

#endif /* __OTA_UPGRADE_H__ */
