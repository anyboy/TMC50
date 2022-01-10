/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager PBAP profile.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <sys_event.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include "btservice_api.h"
#include <shell/shell.h>

#define BTMGR_MAX_MAP_NUM	2
#define MGR_MAP_INDEX_TO_APPID(x)		((x)|0x80)
#define MGR_MAP_APPID_TO_INDEX(x)		((x)&(~0x80))

struct btmgr_map_info {
	u8_t app_id;
	struct btmgr_map_cb *cb;
};

static struct btmgr_map_info mgr_map_info[BTMGR_MAX_MAP_NUM];

static void *btmgr_map_find_free_info(void)
{
	u8_t i;

	for (i = 0; i < BTMGR_MAX_MAP_NUM; i++) {
		if (mgr_map_info[i].app_id == 0) {
			mgr_map_info[i].app_id = MGR_MAP_INDEX_TO_APPID(i);
			return &mgr_map_info[i];
		}
	}
	return NULL;
}

static void btmgr_map_free_info(struct btmgr_map_info *info)
{
	memset(info, 0, sizeof(struct btmgr_map_info));
}

static void *btmgr_map_find_info_by_app_id(u8_t app_id)
{
	u8_t i;

	for (i = 0; i < BTMGR_MAX_MAP_NUM; i++) {
		if (mgr_map_info[i].app_id == app_id) {
			return &mgr_map_info[i];
		}
	}
	return NULL;
}

static void btmgr_map_callback(btsrv_map_event_e event, u8_t app_id, void *data, u8_t size)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);

	if (!info) {
		return;
	}

	switch (event) {
	case BTSRV_MAP_CONNECT_FAILED:
		if (info->cb->connect_failed) {
			info->cb->connect_failed(app_id);
		}
		btmgr_map_free_info(info);
		break;
	case BTSRV_MAP_CONNECTED:
		if (info->cb->connected) {
			info->cb->connected(app_id);
		}
		break;
	case BTSRV_MAP_DISCONNECTED:
		if (info->cb->disconnected) {
			info->cb->disconnected(app_id);
		}
		btmgr_map_free_info(info);
		break;
	case BTSRV_MAP_MESSAGES_RESULT:
		if (info->cb->result) {
			info->cb->result(app_id, data, size);
		}
		break;
	}
}

u8_t btmgr_map_client_connect(bd_address_t *bd, char *path, struct btmgr_map_cb *cb)
{
	int ret;
	struct btmgr_map_info *info;
	struct bt_map_connect_param param;

	if (!path || !cb) {
		return 0;
	}

	info = btmgr_map_find_free_info();
	if (!info) {
		return 0;
	}

	info->cb = cb;

	memcpy(&param.bd, bd, sizeof(bd_address_t));
	param.app_id = info->app_id;
	param.map_path = path;
	param.cb = &btmgr_map_callback;
	ret = btif_map_client_connect(&param);
	if (ret) {
		btmgr_map_free_info(info);
		return 0;
	}

	return info->app_id;
}

u8_t btmgr_map_client_set_folder(u8_t app_id,char *path, u8_t flags)
{
	struct bt_map_set_folder_param param;
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);
	
	if (!info) {
		return -EIO;
	}
	
	param.app_id = info->app_id;
	param.map_path = path;
	param.flags = flags;
	
	return btif_map_client_set_folder(&param);
}

u8_t btmgr_map_get_messsage(bd_address_t *bd, char *path, struct btmgr_map_cb *cb)
{
	int ret;
	struct btmgr_map_info *info;
	struct bt_map_get_param param;

	if (!path || !cb) {
		return 0;
	}

	info = btmgr_map_find_free_info();
	if (!info) {
		return 0;
	}

	info->cb = cb;

	memcpy(&param.bd, bd, sizeof(bd_address_t));
	param.app_id = info->app_id;
	param.map_path = path;
	param.cb = &btmgr_map_callback;
	ret = btif_map_get_message(&param);
	if (ret) {
		btmgr_map_free_info(info);
		return 0;
	}

	return info->app_id;
}

int btmgr_map_get_folder_listing(u8_t app_id)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);
	
	if (!info) {
		return -EIO;
	}

	return btif_map_get_folder_listing(info->app_id);
}

int btmgr_map_get_messages_listing(u8_t app_id,u16_t max_cn,u32_t mask)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);
	struct bt_map_get_messages_listing_param param;
	
	if (!info) {
		return -EIO;
	}

    param.app_id = info->app_id;
    param.max_list_count = max_cn;
    param.parameter_mask = mask;
	return btif_map_get_messages_listing(&param);
}

int btmgr_map_abort_get(u8_t app_id)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);

	if (!info) {
		return -EIO;
	}

	return btif_map_abort_get(info->app_id);
}

int btmgr_map_client_disconnect(u8_t app_id)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);

	if (!info) {
		return -EIO;
	}

	return btif_map_client_disconnect(info->app_id);
}
