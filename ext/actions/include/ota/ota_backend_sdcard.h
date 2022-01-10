/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA SDCARD backend interface
 */

#ifndef __OTA_BACKEND_SDCARD_H__
#define __OTA_BACKEND_SDCARD_H__

#include <ota/ota_backend.h>

struct ota_backend_sdcard_init_param {
	const char *fpath;
};

struct ota_backend *ota_backend_sdcard_init(ota_backend_notify_cb_t cb,
		struct ota_backend_sdcard_init_param *param);

#endif /* __OTA_BACKEND_SDCARD_H__ */
