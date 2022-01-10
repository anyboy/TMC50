/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record app file selector.
 */
#include "record.h"

const char FILE_NAME_TAG[] = "/REC";

static int max_file_num(struct recorder_app_t *record, u16_t *max_num)
{
	u16_t temp_count = MAX_RECORD_FILES;

	while (temp_count) {
		temp_count--;
		if ((1 << (temp_count % 32)) & record->file_bitmaps[temp_count / 32]) {
			*max_num = temp_count + 1;
			return 0;
		}
	}
	return -EINVAL;
}

static int prev_file_num(struct recorder_app_t *record, u16_t cur_num, u16_t *prev_num)
{
	u16_t loop_times = 1;

	while (loop_times++ <= MAX_RECORD_FILES) {
		if (cur_num >= MAX_RECORD_FILES)
			cur_num = 0;
		if ((1 << (cur_num % 32)) & record->file_bitmaps[cur_num / 32]) {
			*prev_num = cur_num + 1;
			return 0;
		}
		cur_num++;
	}
	return -EINVAL;
}

static int next_file_num(struct recorder_app_t *record, u16_t cur_num, u16_t *next_num)
{
	cur_num--;

	while (cur_num) {
		cur_num--;
		if ((1 << (cur_num % 32)) & record->file_bitmaps[cur_num / 32]) {
			*next_num = cur_num + 1;
			return 0;
		}
	}
	return -EINVAL;
}

const void *next_file_path(struct recorder_app_t *record)
{
	char *tmp_start;
	char *tmp_end;
	u16_t file_num = 0;
	u16_t next_num = 0;

	tmp_start = strstr(record->recplayer_bp_info.file_path, FILE_NAME_TAG);
	if (!tmp_start) {
		SYS_LOG_WRN(" file path unmatched\n");
		return NULL;
	}
	tmp_start += strlen(FILE_NAME_TAG);
	tmp_end = strstr(tmp_start, FILE_FORMAT);
	if (!tmp_end) {
		SYS_LOG_WRN("format unmatched\n");
		return NULL;
	}
	tmp_end[0] = 0;
	file_num = atoi(tmp_start);
	if (next_file_num(record, file_num, &next_num)) {
		SYS_LOG_WRN("num invalid\n");
		return NULL;
	}
	SYS_LOG_INF("file_count : %d\n", next_num);

	snprintf(record->recplayer_bp_info.file_path, sizeof(record->recplayer_bp_info.file_path), "%s/REC%03u%s", record->dir, next_num, FILE_FORMAT);

	return record->recplayer_bp_info.file_path;
}
const void *prev_file_path(struct recorder_app_t *record)
{
	char *tmp_start;
	char *tmp_end;
	u16_t file_num = 0;
	u16_t prev_num = 0;

	tmp_start = strstr(record->recplayer_bp_info.file_path, FILE_NAME_TAG);
	if (!tmp_start) {
		SYS_LOG_WRN(" file path unmatched\n");
		return NULL;
	}
	tmp_start += strlen(FILE_NAME_TAG);
	tmp_end = strstr(tmp_start, FILE_FORMAT);
	if (!tmp_end) {
		SYS_LOG_WRN("format unmatched\n");
		return NULL;
	}
	tmp_end[0] = 0;
	file_num = atoi(tmp_start);
	if (prev_file_num(record, file_num, &prev_num)) {
		SYS_LOG_WRN("num invalid\n");
		return NULL;
	}

	SYS_LOG_INF("file_count : %d\n", prev_num);
	snprintf(record->recplayer_bp_info.file_path, sizeof(record->recplayer_bp_info.file_path), "%s/REC%03u%s", record->dir, prev_num, FILE_FORMAT);
	return record->recplayer_bp_info.file_path;
}

const char *recplayer_play_next_url(struct recorder_app_t *record, bool force_switch)
{
	/*manual switch reset restart_iterator_times*/
	if ((record->music_state != RECFILE_STATUS_NULL) && (record->music_state != RECFILE_STATUS_ERROR))
		record->restart_iterator_times = 0;

	const char *path = next_file_path(record);

	if (!path) {
		if ((record->mplayer_mode == RECPLAYER_REPEAT_ALL_MODE && record->restart_iterator_times < 1) || force_switch) {
			memset(record->recplayer_bp_info.file_path, 0, sizeof(record->recplayer_bp_info.file_path));

			record->mplayer_state = RECPLAYER_STATE_INIT;
			record->restart_iterator_times++;
			u16_t max_num = 0;

			if (max_file_num(record, &max_num))
				return NULL;
			snprintf(record->recplayer_bp_info.file_path, sizeof(record->recplayer_bp_info.file_path), "%s/REC%03u%s", record->dir, max_num, FILE_FORMAT);
			return record->recplayer_bp_info.file_path;
		} else {
			if (record->mplayer_state == RECPLAYER_STATE_INIT)
				record->mplayer_state = RECPLAYER_STATE_ERROR;
			SYS_LOG_INF("path null\n");
			return NULL;
		}
	}
	return record->recplayer_bp_info.file_path;
}
const char *recplayer_play_prev_url(struct recorder_app_t *record)
{
	const char *path = prev_file_path(record);

	if (!path) {
		SYS_LOG_WRN("path null\n");
		record->mplayer_state = RECPLAYER_STATE_ERROR;
		return NULL;
	}
	return record->recplayer_bp_info.file_path;
}

