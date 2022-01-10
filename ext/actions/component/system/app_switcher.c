/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief app switch manager.
 */

#include <os_common_api.h>
#include <string.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_manager.h>
#include <hotplug_manager.h>
#include "app_switch.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "app_switcher"
#include <logging/sys_log.h>


struct app_switch_node_t {
	const char *app_name;
	u8_t pluged;
} app_switch_node_t;

struct app_switch_ctx_t {
	u8_t locked;
	u8_t force_locked;
	u8_t cur_index;
	u8_t last_index;
	u8_t app_num;
	u8_t app_actived_num;
	struct app_switch_node_t app_list[0];
};

OS_MUTEX_DEFINE(app_switch_mutex);

struct app_switch_ctx_t *g_switch_ctx = NULL;

static inline struct app_switch_ctx_t *_app_switch_get_ctx(void)
{
	return g_switch_ctx;
}

static inline u8_t _app_switch_get_app_id(char *app_name)
{
	int i = 0;
	int result = -1;
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);

	for (i = 0; i < switch_ctx->app_num; i++) {
		struct app_switch_node_t *app_node = &switch_ctx->app_list[i];

		if (!memcmp(app_node->app_name, app_name, strlen(app_name))) {
			result = i;
			break;
		}
	}

	os_mutex_unlock(&app_switch_mutex);
	return result;
}

u8_t *app_switch_get_app_name(u8_t appid)
{
	u8_t *result = NULL;
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);

	if (appid < switch_ctx->app_num) {
		struct app_switch_node_t *app_node = &switch_ctx->app_list[appid];

		result = (u8_t *)app_node->app_name;
	}

	os_mutex_unlock(&app_switch_mutex);
	return result;
}
extern void recorder_service_app_switch_exit(void);
extern void alarm_need_exit(void);

int app_switch(void *app_name, u32_t switch_mode, bool force_switch)
{
	int ret = 0;
	int i = 0;
#ifdef CONFIG_ESD_MANAGER
	u8_t app_id = -1;
#endif
	int target_app_index = -1;
	struct app_switch_node_t *app_node = NULL;
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "switch");
#endif

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);

	if (switch_ctx->force_locked) {
		SYS_LOG_INF("force locked %d \n", switch_ctx->force_locked);
		goto exit;
	}

#ifdef CONFIG_RECORD_SERVICE
	recorder_service_app_switch_exit();
#endif
#ifdef CONFIG_ALARM_APP
	alarm_need_exit();
#endif

	if (app_name && !strcmp(app_name, app_manager_get_current_app())) {
		SYS_LOG_INF("current app %s is you need\n", (char *)app_name);
		goto exit;
	}

	if (switch_ctx->locked && !force_switch) {
		SYS_LOG_INF("locked %d\n", switch_ctx->locked);
		goto exit;
	}

	if (switch_ctx->app_actived_num <= 1
		&& g_switch_ctx->cur_index != 0xff
		&& switch_ctx->app_list[switch_ctx->cur_index].pluged
		&& !force_switch) {
		SYS_LOG_INF("app_actived_num %d ",
						switch_ctx->app_actived_num);
		SYS_LOG_INF("cur_index %d pluged %d",
						switch_ctx->cur_index,
						switch_ctx->app_list[switch_ctx->cur_index].pluged);
		goto exit;
	}

next:
	if (!app_name) {
		if (switch_mode == APP_SWITCH_NEXT) {
			target_app_index = (switch_ctx->cur_index + 1)
											 % switch_ctx->app_num;
		} else if (switch_mode == APP_SWITCH_PREV) {
			target_app_index = (switch_ctx->cur_index - 1 + switch_ctx->app_num)
											 % switch_ctx->app_num;
		} else {
			target_app_index = switch_ctx->last_index;
		}
	} else {
		for (i = 0; i < switch_ctx->app_num; i++) {
			app_node = &switch_ctx->app_list[i];
			if (!memcmp(app_node->app_name, app_name, strlen(app_name))
				&& (app_node->pluged || force_switch)) {
				target_app_index = i;
			}
		}
	}

	if (target_app_index == switch_ctx->cur_index
		|| target_app_index < 0) {
		if (switch_ctx->cur_index < switch_ctx->app_num) {
			SYS_LOG_INF("currren app is %s",
					switch_ctx->app_list[switch_ctx->cur_index].app_name);
		}
		ret = 0xff;
		goto exit;
	}

	switch_ctx->last_index = switch_ctx->cur_index;
	switch_ctx->cur_index = target_app_index;
	app_node = &switch_ctx->app_list[switch_ctx->cur_index];

	if (!app_node->pluged && !force_switch) {
		/**if last is not exit ,go to prev */
		if (switch_mode == APP_SWITCH_LAST) {
			switch_mode = APP_SWITCH_PREV;
		}
		goto next;
	}

	SYS_LOG_INF("name:%s", app_node->app_name);

	ret = app_manager_active_app((char *)app_node->app_name);

#ifdef CONFIG_ESD_MANAGER
	app_id = target_app_index;
	esd_manager_save_scene(TAG_APP_ID, &app_id, 1);
#endif

exit:
	os_mutex_unlock(&app_switch_mutex);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "switch");
#endif
	return ret;
}

void app_switch_add_app(const char *app_name)
{
	int i = 0;
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);

	for (i = 0; i < switch_ctx->app_num; i++) {
		struct app_switch_node_t *app_node = &switch_ctx->app_list[i];

		if (!memcmp(app_node->app_name, app_name, strlen(app_name))
				&& !app_node->pluged) {
			app_node->pluged = 1;
			switch_ctx->app_actived_num++;
			SYS_LOG_INF("name:%s plug in", app_node->app_name);
		}
	}

	os_mutex_unlock(&app_switch_mutex);
}

void app_switch_remove_app(const char *app_name)
{
	int i = 0;
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);

	for (i = 0; i < switch_ctx->app_num; i++) {
		struct app_switch_node_t *app_node = &switch_ctx->app_list[i];

		if (!memcmp(app_node->app_name, app_name, strlen(app_name))
				&& app_node->pluged) {
			app_node->pluged = 0;
			switch_ctx->app_actived_num--;
			SYS_LOG_INF("name:%s plug out ", app_node->app_name);
		}
	}

	os_mutex_unlock(&app_switch_mutex);
}

void app_switch_lock(u8_t reason)
{
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);
	switch_ctx->locked |= reason;
	os_mutex_unlock(&app_switch_mutex);
}

void app_switch_unlock(u8_t reason)
{
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);
	switch_ctx->locked &= ~reason;
	os_mutex_unlock(&app_switch_mutex);
}

void app_switch_force_lock(u8_t reason)
{
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);
	switch_ctx->force_locked |= reason;
	os_mutex_unlock(&app_switch_mutex);
}

void app_switch_force_unlock(u8_t reason)
{
	struct app_switch_ctx_t *switch_ctx = _app_switch_get_ctx();

	os_mutex_lock(&app_switch_mutex, OS_FOREVER);
	switch_ctx->force_locked &= ~reason;
	os_mutex_unlock(&app_switch_mutex);
}

int app_switch_init(const char **app_id_switch_list, int app_num)
{
	int i = 0;
	int ctx_size = sizeof(struct app_switch_ctx_t)
						+ sizeof(struct app_switch_node_t) * app_num;

	g_switch_ctx = (struct app_switch_ctx_t *)mem_malloc(ctx_size);
	if (!g_switch_ctx) {
		SYS_LOG_ERR("need mem %d bytes ", ctx_size);
		return -ENOMEM;
	}

	memset(g_switch_ctx, 0, ctx_size);

	for (i = 0; i < app_num; i++) {
		struct app_switch_node_t *app_node = &g_switch_ctx->app_list[i];

		app_node->app_name = app_id_switch_list[i];
		app_node->pluged = 0;
	}

	g_switch_ctx->cur_index = 0xff;
	g_switch_ctx->last_index = 0xff;
	g_switch_ctx->app_num = app_num;
	g_switch_ctx->app_actived_num = 0;
	g_switch_ctx->locked = 0;
	g_switch_ctx->force_locked = 0;
	return 0;
}

