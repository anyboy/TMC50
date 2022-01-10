/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA backend interface
 */

#ifndef __OTA_BACKEND_H__
#define __OTA_BACKEND_H__

#define OTA_BACKEND_TYPE_UNKNOWN		(0)
#define OTA_BACKEND_TYPE_CARD			(1)
#define OTA_BACKEND_TYPE_BLUETOOTH		(2)
#define OTA_BACKEND_TYPE_TEMP_PART		(3)

#define OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID	(0x10001)
#define OTA_BACKEND_IOCTL_REPORT_PROCESS		(0x10002)

#define OTA_BACKEND_UPGRADE_STATE	(1)
#define OTA_BACKEND_UPGRADE_PROGRESS	(2)

struct ota_backend;

typedef void (*ota_backend_notify_cb_t)(struct ota_backend *backend, int cmd, int state);

/**
 * @brief Logger backend API.
 */
struct ota_backend_api
{
	struct ota_backend * (*init)(ota_backend_notify_cb_t cb, void *init_param);
	void (*exit)(struct ota_backend *backend);
	int (*open)(struct ota_backend *backend);
	int (*read)(struct ota_backend *backend, int offset, void *buf, int size);
	int (*ioctl)(struct ota_backend *backend, int cmd, unsigned int param);
	int (*close)(struct ota_backend *backend);
};

struct ota_backend {
	struct ota_backend_api *api;
	int type;
	ota_backend_notify_cb_t cb;
};

static inline int ota_backend_get_type(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);

	return backend->type;
}

static inline int ota_backend_ioctl(struct ota_backend *backend, int cmd,
				    unsigned int param)
{
	 __ASSERT_NO_MSG(backend);

	 if (backend->api->ioctl) {
		 return backend->api->ioctl(backend, cmd, param);
	 }

	return 0;
}

static inline int ota_backend_read(struct ota_backend *backend, int offset,
				   void *buf, int size)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->read(backend, offset, buf, size);
}

static inline int ota_backend_open(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->open(backend);
}

static inline int ota_backend_close(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->close(backend);
}

static inline void ota_backend_exit(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);
}

static inline int ota_backend_init(struct ota_backend *backend, int type,
				   struct ota_backend_api *api,
				   ota_backend_notify_cb_t cb)
{
	 __ASSERT_NO_MSG(backend);

	backend->type = type;
	backend->api = api;
	backend->cb = cb;

	return 0;
}

#endif /* __OTA_BACKEND_H__ */
