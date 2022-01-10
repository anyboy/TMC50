/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tts manager interface
 */
#define SYS_LOG_DOMAIN "TTS"
#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <file_stream.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <tts_manager.h>
#include <sdfs.h>

struct tts_item_t {
	/* 1st word reserved for use by list */
	sys_snode_t node;
	void *player_handle;
	io_stream_t tts_stream;
	u8_t tts_mode;
	u8_t tts_file_name[0];
};

struct tts_manager_ctx_t {
	const struct tts_config_t *tts_config;
	sys_slist_t tts_list;
	struct tts_item_t *tts_curr;
	os_mutex tts_mutex;
	u32_t tts_manager_locked:8;
	u32_t tts_item_num:8;
	tts_event_nodify lisener;

	os_delayed_work stop_work;
};

static struct tts_manager_ctx_t *tts_manager_ctx;

static struct tts_manager_ctx_t *_tts_get_ctx(void)
{
	return tts_manager_ctx;
}

static void _tts_event_nodify_callback(struct app_msg *msg, int result, void *reply)
{
	if (msg->sync_sem) {
		os_sem_give((os_sem *)msg->sync_sem);
	}
}

static void _tts_event_nodify(u32_t event, u8_t sync_mode)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	u8_t *current_app = app_manager_get_current_app();

	if (current_app) {
		struct app_msg msg = {0};

		msg.type = MSG_TTS_EVENT;
		msg.value = event;
		if (sync_mode) {
			os_sem return_notify;

			os_sem_init(&return_notify, 0, 1);
			msg.callback = _tts_event_nodify_callback;
			msg.sync_sem = &return_notify;
			if (!send_async_msg(current_app, &msg)) {
				return;
			}
			os_mutex_unlock(&tts_ctx->tts_mutex);
			if (os_sem_take(&return_notify, OS_FOREVER) != 0) {
				return;
			}
			os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
		} else {
			if (!send_async_msg(current_app, &msg)) {
				return;
			}
		}
	}
}

static io_stream_t _tts_create_stream(struct tts_item_t *item)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	io_stream_t stream = NULL;

	if (tts_ctx->tts_config->tts_storage_media == TTS_SOTRAGE_SDFS) {
		struct buffer_t buffer;

		if (sd_fmap(&item->tts_file_name[0], (void **)&buffer.base, &buffer.length) != 0) {
			goto exit;
		}

		buffer.cache_size = 0;
		stream = buffer_stream_create(&buffer);
		if (!stream) {
			SYS_LOG_ERR("create failed\n");
			goto exit;
		}
	} else if (tts_ctx->tts_config->tts_storage_media == TTS_SOTRAGE_SDCARD) {
	#ifdef CONFIG_FILE_STREAM
		stream = file_stream_create(&item->tts_file_name[0]);
		if (!stream) {
			SYS_LOG_ERR("create failed\n");
			goto exit;
		}
	#endif
	}

	if (stream_open(stream, MODE_IN) != 0) {
		SYS_LOG_ERR("open failed\n");
		stream_destroy(stream);
		stream = NULL;
		goto exit;
	}

	SYS_LOG_INF("tts stream %p tts_item %s\n", stream, &item->tts_file_name[0]);
exit:
	return stream;
}

static void _tts_remove_current_node(struct tts_manager_ctx_t *tts_ctx)
{
	struct tts_item_t *tts_item;

	if (!tts_ctx->tts_curr)
		return;

	tts_item  = tts_ctx->tts_curr;

	if (tts_item->player_handle) {
		media_player_stop(tts_item->player_handle);
		media_player_close(tts_item->player_handle);
		tts_item->player_handle = NULL;
	}

	if (tts_item->tts_stream) {
		stream_close(tts_item->tts_stream);
		stream_destroy(tts_item->tts_stream);
		tts_item->tts_stream = NULL;
	}

	tts_manager_ctx->tts_curr = NULL;

	if (tts_ctx->lisener) {
		os_mutex_unlock(&tts_ctx->tts_mutex);
		tts_ctx->lisener(&tts_item->tts_file_name[0], TTS_EVENT_STOP_PLAY);
		os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
	}

	_tts_event_nodify(TTS_EVENT_STOP_PLAY, 0);

	mem_free(tts_item);
}

static void _tts_manager_play_event_notify(u32_t event, void *data, u32_t len, void *user_data)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	SYS_LOG_DBG("event %d , data %p , len %d\n", event, data, len);

	switch (event) {
	case PLAYBACK_EVENT_STOP_COMPLETE:
		os_delayed_work_submit(&tts_ctx->stop_work, OS_NO_WAIT);
		break;
	}
}

static void _tts_manager_triggle_play_tts(void)
{
	struct app_msg msg = {0};
	msg.type = MSG_TTS_EVENT;
	msg.cmd = TTS_EVENT_START_PLAY;
	send_async_msg("main", &msg);
}

static int _tts_start_play(struct tts_item_t *tts_item)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	int ret = 0;
	media_init_param_t init_param;

	memset(&init_param, 0, sizeof(media_init_param_t));

	tts_ctx->tts_curr = tts_item;

	tts_item->tts_stream = _tts_create_stream(tts_item);
	if (!tts_item->tts_stream) {
		_tts_remove_current_node(tts_ctx);
		_tts_manager_triggle_play_tts();
		goto exit;
	}

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
	init_param.stream_type = AUDIO_STREAM_TTS;
	init_param.efx_stream_type = AUDIO_STREAM_TTS;
	init_param.format = MP3_TYPE;
	init_param.sample_rate = 16;
	init_param.input_stream = tts_item->tts_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = _tts_manager_play_event_notify;

	tts_item->player_handle = media_player_open(&init_param);
	if (!tts_item->player_handle) {
		_tts_remove_current_node(tts_ctx);
		goto exit;
	}

	media_player_play(tts_item->player_handle);

exit:
	return ret;
}

void tts_manager_play_process(void)
{
	sys_snode_t *head;
	struct tts_item_t *tts_item;
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
	SYS_LOG_INF("tts_curr %p\n", tts_ctx->tts_curr);
	if (tts_ctx->tts_curr) {
		goto exit;
	}

	if (sys_slist_is_empty(&tts_ctx->tts_list)) {
		goto exit;
	}

	head  = sys_slist_peek_head(&tts_ctx->tts_list);
	tts_item = CONTAINER_OF(head, struct tts_item_t, node);
	sys_slist_find_and_remove(&tts_ctx->tts_list, head);
	tts_ctx->tts_item_num--;
	if (tts_ctx->lisener) {
		tts_ctx->lisener(&tts_item->tts_file_name[0], TTS_EVENT_START_PLAY);
	}
	_tts_event_nodify(TTS_EVENT_START_PLAY, 1);

	_tts_start_play(tts_item);

exit:
	os_mutex_unlock(&tts_ctx->tts_mutex);
}

static void _tts_manager_stop_work(os_work *work)
{

	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	_tts_remove_current_node(tts_ctx);

	os_mutex_unlock(&tts_ctx->tts_mutex);

	_tts_manager_triggle_play_tts();
}

static struct tts_item_t *_tts_create_item(u8_t *tts_name, u32_t mode)
{
	struct tts_item_t *item = mem_malloc(sizeof(struct tts_item_t) + strlen(tts_name));

	if (!item) {
		goto exit;
	}
	memset(item, 0, sizeof(struct tts_item_t) + strlen(tts_name));
	item->tts_mode = mode;
	strcpy(item->tts_file_name, tts_name);

exit:
	return item;
}

void tts_manager_unlock(void)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->tts_manager_locked--;
	SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);
	os_mutex_unlock(&tts_ctx->tts_mutex);
}

void tts_manager_lock(void)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->tts_manager_locked++;
	SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);
	os_mutex_unlock(&tts_ctx->tts_mutex);
}

bool tts_manager_is_locked(void)
{
	bool result = false;

	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	result = (tts_ctx->tts_manager_locked > 0);

	SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return result;
}

void tts_manager_wait_finished(bool clean_all)
{
	int try_cnt = 10000;
	struct tts_item_t *temp, *tts_item;
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	/**avoid tts wait finished death loop*/
	while (tts_ctx->tts_curr && try_cnt-- > 0) {
		os_sleep(4);
	}

	if (try_cnt == 0) {
		SYS_LOG_WRN("timeout\n");
	}

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	if (!sys_slist_is_empty(&tts_ctx->tts_list) && clean_all) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tts_ctx->tts_list, tts_item, temp, node) {
			if (tts_item) {
				sys_slist_find_and_remove(&tts_ctx->tts_list, &tts_item->node);
				mem_free(tts_item);
			}
		}
		tts_ctx->tts_item_num = 0;

	}

	os_mutex_unlock(&tts_ctx->tts_mutex);
}

int tts_manager_play(u8_t *tts_name, u32_t mode)
{
	int res = 0;
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	struct tts_item_t *item = NULL;

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	if (tts_ctx->tts_manager_locked > 0) {
		SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);
		res = -ENOLCK;
		goto exit;
	}

	if (!strcmp(tts_name,"keytone.pcm")) {
	#ifdef CONFIG_PLAY_KEYTONE
		key_tone_play();
	#endif
		goto exit;
	}

	if (tts_ctx->tts_item_num >= 3) {
		sys_snode_t *head = sys_slist_peek_head(&tts_ctx->tts_list);
		item = CONTAINER_OF(head, struct tts_item_t, node);
		sys_slist_find_and_remove(&tts_ctx->tts_list, head);
		tts_ctx->tts_item_num--;
		SYS_LOG_ERR("drop: tts  %s\n", &item->tts_file_name[0]);
		mem_free(item);
	}

	item = _tts_create_item(tts_name, mode);
	if (!item) {
		res = -ENOMEM;
		goto exit;
	}

	sys_slist_append(&tts_ctx->tts_list, (sys_snode_t *)item);
	tts_ctx->tts_item_num++;

	_tts_manager_triggle_play_tts();
exit:
	os_mutex_unlock(&tts_ctx->tts_mutex);
	return res;
}

static bool block_tts_finised = false;
static void _tts_manager_play_block_event_notify(u32_t event, void *data, u32_t len, void *user_data)
{
	switch (event) {
	case PLAYBACK_EVENT_STOP_COMPLETE:
		block_tts_finised = true;
		break;
	}
}

int tts_manager_play_block(u8_t *tts_name)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	int ret = 0;
	media_init_param_t init_param;
	io_stream_t stream = NULL;
	struct buffer_t buffer;
	void *player_handle = NULL;
	int try_cnt = 0;
	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
	if (sd_fmap(tts_name, (void **)&buffer.base, &buffer.length) != 0) {
		goto exit;
	}

	buffer.cache_size = 0;
	stream = buffer_stream_create(&buffer);
	if (!stream) {
		SYS_LOG_ERR("create failed\n");
		goto exit;
	}

	if (stream_open(stream, MODE_IN) != 0) {
		SYS_LOG_ERR("open failed\n");
		stream_destroy(stream);
		stream = NULL;
		goto exit;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
	init_param.stream_type = AUDIO_STREAM_TTS;
	init_param.efx_stream_type = AUDIO_STREAM_TTS;
	init_param.format = MP3_TYPE;
	init_param.sample_rate = 16;
	init_param.input_stream = stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = _tts_manager_play_block_event_notify;

	player_handle = media_player_open(&init_param);
	if (!player_handle) {
		goto exit;
	}

	block_tts_finised = false;

	media_player_play(player_handle);

	while (!block_tts_finised && try_cnt ++ < 100) {
		os_sleep(100);
	}

	media_player_stop(player_handle);
	media_player_close(player_handle);

exit:
	if (!stream) {
		stream_close(stream);
		stream_destroy(stream);
	}
	os_mutex_unlock(&tts_ctx->tts_mutex);
	return ret;
}

int tts_manager_stop(u8_t *tts_name)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	struct tts_item_t *tts_item;

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	if (!tts_ctx->tts_curr)
		goto exit;

	tts_item = tts_ctx->tts_curr;

	if (tts_item->tts_mode == UNBLOCK_UNINTERRUPTABLE
		|| tts_item->tts_mode == BLOCK_UNINTERRUPTABLE) {
		goto exit;
	}

	_tts_remove_current_node(tts_ctx);

exit:
	os_mutex_unlock(&tts_ctx->tts_mutex);

	_tts_manager_triggle_play_tts();

	return 0;
}

int tts_manager_register_tts_config(const struct tts_config_t *config)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->tts_config = config;

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return 0;
}

int tts_manager_add_event_lisener(tts_event_nodify lisener)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->lisener = lisener;

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return 0;
}

int tts_manager_remove_event_lisener(tts_event_nodify lisener)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->lisener = NULL;

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return 0;
}

const struct tts_config_t tts_config = {
	.tts_storage_media = TTS_SOTRAGE_SDFS,
};

int tts_manager_process_ui_event(int ui_event)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	u8_t *tts_file_name = NULL;
	int mode = 0;

	if (!tts_ctx->tts_config || !tts_ctx->tts_config->tts_map)
		return -ENOENT;

	for (int i = 0; i < tts_ctx->tts_config->tts_map_size; i++) {
		const tts_ui_event_map_t *tts_ui_event_map = &tts_ctx->tts_config->tts_map[i];

		if (tts_ui_event_map->ui_event == ui_event) {
			tts_file_name = (u8_t *)tts_ui_event_map->tts_file_name;
			mode = tts_ui_event_map->mode;
			break;
		}

	}
	SYS_LOG_INF(" %d %s \n",ui_event,tts_file_name);
	if (tts_file_name)
		tts_manager_play(tts_file_name, mode);

	return 0;
}

static struct tts_manager_ctx_t global_tts_manager_ctx;

int tts_manager_init(void)
{
	tts_manager_ctx = &global_tts_manager_ctx;

	memset(tts_manager_ctx, 0, sizeof(struct tts_manager_ctx_t));

	sys_slist_init(&tts_manager_ctx->tts_list);

	os_mutex_init(&tts_manager_ctx->tts_mutex);

	os_delayed_work_init(&tts_manager_ctx->stop_work, _tts_manager_stop_work);

	tts_manager_register_tts_config(&tts_config);

#ifdef CONFIG_PLAY_KEYTONE
	key_tone_manager_init();
#endif

	return 0;
}

int tts_manager_deinit(void)
{
	tts_manager_wait_finished(true);

	os_delayed_work_cancel(&tts_manager_ctx->stop_work);

	return 0;
}


