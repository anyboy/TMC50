/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief loop player app file selector.
 */

#include "loop_player.h"

OS_MUTEX_DEFINE(loop_iter_mutex);

static iterator_t *loop_iter = NULL;
static int _loopplay_filter_dir(const char *path)
{
	return false;	//no folder filted
}

static int _iterator_match_fn(const char *path, int is_dir)
{
	if (!is_dir) {
		/* only support mp3 file */
		return get_music_file_type(path) == MP3_TYPE;
	}

	if (_loopplay_filter_dir(path))
		return false;
	else
		return true;
}

int loopplay_init_iterator(struct loop_play_app_t *lpplayer)
{
	file_iterator_cursor_t cursor;
	file_iterator_param_t param;
	int res = 0;

	os_mutex_lock(&loop_iter_mutex, OS_FOREVER);

	cursor.path = lpplayer->cur_url;

	memset(&param, 0, sizeof(param));
	if (lpplayer->cur_url[0] == 0) {
		SYS_LOG_INF("cur_url null\n");
		cursor.path = NULL;
	}
	param.cursor = &cursor;
	param.topdir = lpplayer->cur_dir;
	param.max_level = MAX_DIR_LEVEL;

	param.match_fn = _iterator_match_fn;

	loop_iter = file_iterator_create(&param);
	if (loop_iter)
		iterator_get_plist_info(loop_iter, &lpplayer->sum_track_no);
	else
		res = -ENOSYS;
	os_mutex_unlock(&loop_iter_mutex);
	SYS_LOG_INF("sum_track_no=%d\n", lpplayer->sum_track_no);
	return res;
}
void loopplay_exit_iterator(void)
{
	os_mutex_lock(&loop_iter_mutex, OS_FOREVER);

	if (loop_iter)
		iterator_destroy(loop_iter);
	loop_iter = NULL;
	os_mutex_unlock(&loop_iter_mutex);
}

const char *loopplay_set_track_no(struct loop_play_app_t *lpplayer)
{
	os_mutex_lock(&loop_iter_mutex, OS_FOREVER);
	if (!loop_iter) {
		SYS_LOG_WRN("iter null\n");
		lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		os_mutex_unlock(&loop_iter_mutex);
		return NULL;
	}
	const char *path = iterator_set_track_no(loop_iter, lpplayer->track_no);

	if (!path) {
		lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		SYS_LOG_WRN("path null\n");
		os_mutex_unlock(&loop_iter_mutex);
		return NULL;
	}

	strncpy(lpplayer->cur_url, path, MAX_URL_LEN);
	lpplayer->cur_url[sizeof(lpplayer->cur_url) - 1] = 0;
	os_mutex_unlock(&loop_iter_mutex);
	return lpplayer->cur_url;
}

const char *loopplay_next_url(struct loop_play_app_t *lpplayer, bool force_switch)
{
	os_mutex_lock(&loop_iter_mutex, OS_FOREVER);

	if (!loop_iter) {
		SYS_LOG_WRN("iter null\n");
		lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		os_mutex_unlock(&loop_iter_mutex);
		return NULL;
	}
	/*manual switch reset restart_iterator_times*/
	if ((lpplayer->music_status != LOOP_STATUS_NULL) && (lpplayer->music_status != LOOP_STATUS_ERROR))
		lpplayer->full_cycle_times = 0;

	const char *path = iterator_next(loop_iter, force_switch, &lpplayer->track_no);

	if (lpplayer->track_no == lpplayer->sum_track_no)
		lpplayer->full_cycle_times++;

	if (!path || lpplayer->full_cycle_times > 2) {
		lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		SYS_LOG_WRN("path null\n");
		os_mutex_unlock(&loop_iter_mutex);
		return NULL;
	}

	strncpy(lpplayer->cur_url, path, MAX_URL_LEN);
	lpplayer->cur_url[sizeof(lpplayer->cur_url) - 1] = 0;
	os_mutex_unlock(&loop_iter_mutex);

	return lpplayer->cur_url;
}
const char *loopplay_prev_url(struct loop_play_app_t *lpplayer)
{
	os_mutex_lock(&loop_iter_mutex, OS_FOREVER);
	if (!loop_iter) {
		SYS_LOG_WRN("iter null\n");
		lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		os_mutex_unlock(&loop_iter_mutex);
		return NULL;
	}
	const char *path = iterator_prev(loop_iter, &lpplayer->track_no);

	if (lpplayer->track_no == lpplayer->sum_track_no)
		lpplayer->full_cycle_times++;

	if (!path || lpplayer->full_cycle_times > 2) {
		lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		SYS_LOG_WRN("path null\n");
		os_mutex_unlock(&loop_iter_mutex);
		return NULL;
	}

	strncpy(lpplayer->cur_url, path, MAX_URL_LEN);
	lpplayer->cur_url[sizeof(lpplayer->cur_url) - 1] = 0;
	SYS_LOG_INF("url:%s\n", lpplayer->cur_url);
	os_mutex_unlock(&loop_iter_mutex);
	return lpplayer->cur_url;
}

int loopplay_set_mode(struct loop_play_app_t *lpplayer, u8_t mode)
{
	os_mutex_lock(&loop_iter_mutex, OS_FOREVER);
	if (!loop_iter) {
		SYS_LOG_WRN("iter null\n");
		lpplayer->mplayer_state = LPPLAYER_STATE_ERROR;
		os_mutex_unlock(&loop_iter_mutex);
		return -ENOSYS;
	}
	return iterator_set_mode(loop_iter, mode);
}
