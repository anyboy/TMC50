/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record app media
 */

#include "record.h"
#include "media_mem.h"
#include "tts_manager.h"
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif


#define DISK_FREE_SPACE_MIN	(1)	/* 1MB */
#define RECORD_DISK_FREE_SPACE_MIN	(10)	/* 10MB */
#define RECORD_FILE_SIZE_MAX	(512)	/* 512MB */
#define READ_CACHE_TIME_PERIOD OS_MSEC(50)
#define RECORD_BPINFO_LEN	(64)

const char RECORD_BPINFO_FILE[] = "_record.log";
const char COUNT_TAG[] = "file_count:";
const char COUNT_END_TAG[] = ",";

static char record_adpcm_cache[0x2800] _NODATA_SECTION(.btmusic_pcm_bss);
char record_cache_read_temp[0x800] _NODATA_SECTION(.btmusic_pcm_bss);

extern io_stream_t record_stream_create_ext(void *ring_buff, u32_t ring_buff_size);

static io_stream_t _recorder_create_outputstream(void)
{
	int ret = 0;

	memset(record_adpcm_cache, 0, sizeof(record_adpcm_cache));
	io_stream_t output_stream = record_stream_create_ext(record_adpcm_cache, sizeof(record_adpcm_cache));

	if (!output_stream) {
		goto exit;
	}

	ret = stream_open(output_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(output_stream);
		output_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", output_stream);
	return output_stream;
}

static void record_stream_observer_notify(void *observer, int readoff, int writeoff, int total_size,
									unsigned char *buf, int num, stream_notify_type type)
{
	struct recorder_app_t *record = recorder_get_app();

	if (!record)
		return;
	/*record file more than 512M byte will start next*/
	if (writeoff >= RECORD_FILE_SIZE_MAX * 1024 * 1024) {
		if (!record->file_is_full) {
			struct app_msg msg = { 0 };

			msg.type = MSG_INPUT_EVENT;
			msg.cmd = MSG_RECORDER_SAVE_AND_START_NEXT;
			send_async_msg(APP_ID_RECORDER, &msg);
			record->file_is_full = 1;
		}
		SYS_LOG_INF("%d bytes\n", writeoff);
	}

	if (record->disk_space_less) {
		if (writeoff >= (record->disk_space_size - DISK_FREE_SPACE_MIN) * 1024 * 1024) {
			struct app_msg msg_new = { 0 };

			/* change to next app */
			msg_new.type = MSG_INPUT_EVENT;
			msg_new.cmd = MSG_SWITCH_APP;
			send_async_msg(APP_ID_MAIN, &msg_new);
			record->disk_space_less = 0;

			SYS_LOG_WRN("no space\n");
		}
	}
}

static u32_t _record_get_count_from_name(char *name)
{
	char *tmp;
	char *temp;

	tmp = strstr(name, "REC");
	if (!tmp) {
		return 0;
	} else {
		tmp += strlen("REC");
		temp = strstr(tmp, FILE_FORMAT);
		if (!temp)
			return 0;

		temp[0] = 0;
		return atoi(tmp);
	}
}
static void _record_set_file_bitmaps(struct recorder_app_t *record, u16_t file_count)
{
	if (file_count && file_count <= MAX_RECORD_FILES) {
		file_count--;
		record->file_bitmaps[file_count / 32] |= 1 << (file_count % 32);
	}
}

int record_update_file_bitmaps(struct recorder_app_t *record, const char *dir)
{
	int res;
	u32_t count = 0;
	struct fs_dirent *entry = app_mem_malloc(sizeof(struct fs_dirent));
	fs_dir_t *dirp = app_mem_malloc(sizeof(fs_dir_t));

	if (!entry || !dirp)
		return -ENOMEM;

	memset(record->file_bitmaps, 0, sizeof(record->file_bitmaps));
	/* Verify fs_opendir() */
	res = fs_opendir(dirp, dir);
	if (res) {
		SYS_LOG_ERR("Error opening dir %s [%d]\n", dir, res);
		app_mem_free(entry);
		app_mem_free(dirp);
		return -EINVAL;
	}

	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(dirp, entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry->name[0] == 0)
			break;

		if (entry->type != FS_DIR_ENTRY_FILE)
			continue;

		count = _record_get_count_from_name(entry->name);
		if (count && count <= MAX_RECORD_FILES) {
			count--;
			record->file_bitmaps[count / 32] |= 1 << (count % 32);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(dirp);

	app_mem_free(entry);
	app_mem_free(dirp);
	return 0;
}

static int check_file_dir_exists(const char *path)
{
	int res = 0;
	struct fs_dirent *entry = NULL;

	/* Verify fs_stat() */
	entry = app_mem_malloc(sizeof(struct fs_dirent));
	if (entry) {
		res = fs_stat(path, entry);
		if (res || entry->type != FS_DIR_ENTRY_FILE) {
			app_mem_free(entry);
			return -ENOENT;
		}
		app_mem_free(entry);
	} else {
		SYS_LOG_ERR("malloc faild\n");
		return -ENOENT;
	}
	return res;
}

static char *_record_create_file_path(struct recorder_app_t *record)
{
	int check_file_times = 1;

	while (check_file_times++ <= MAX_RECORD_FILES) {
		memset(record->recplayer_bp_info.file_path, 0, sizeof(record->recplayer_bp_info.file_path));
		if (record->file_count >= MAX_RECORD_FILES) {
			record->file_count = 0;
		}

		if ((1 << (record->file_count % 32)) & record->file_bitmaps[record->file_count / 32]) {
			record->file_count++;
			continue;
		}
		snprintf(record->recplayer_bp_info.file_path, sizeof(record->recplayer_bp_info.file_path), "%s/REC%03u%s", record->dir, ++record->file_count, FILE_FORMAT);
		_record_set_file_bitmaps(record, record->file_count);
		return record->recplayer_bp_info.file_path;
	}
	SYS_LOG_WRN("file_count %d more than %d\n", record->file_count, MAX_RECORD_FILES);
	record->file_count = 0;
	return NULL;
}

void recorder_repair_file(struct recorder_app_t *record)
{
	char *file_path = NULL;
	io_stream_t stream_wav;
	int res = 0;

#ifdef CONFIG_PROPERTY
	property_get("RECORDER_BP_INFO", (char *)&record->recplayer_bp_info, sizeof(record->recplayer_bp_info));
#endif
	file_path = strstr(record->recplayer_bp_info.file_path, FILE_FORMAT);
	if (!file_path)
		return;
	if (check_file_dir_exists(record->recplayer_bp_info.file_path)) {
		SYS_LOG_WRN("file:%s not exist\n", record->recplayer_bp_info.file_path);
		return;
	}
	SYS_LOG_INF("path:%s\n", record->recplayer_bp_info.file_path);
	stream_wav = file_stream_create((void *)record->recplayer_bp_info.file_path);
	if (stream_wav) {
		res = stream_open(stream_wav, MODE_OUT);
		if (res) {
			SYS_LOG_ERR("stream open failed %d\n", res);
			stream_destroy(stream_wav);
			stream_wav = NULL;
			return;
		}
	} else {
		SYS_LOG_ERR("create fail\n");
		return;
	}
	media_player_repair_filhdr(NULL, WAV_TYPE, stream_wav);

	stream_close(stream_wav);
	stream_destroy(stream_wav);
}
void recorder_get_file_count(struct recorder_app_t *record)
{
	char file_name[sizeof(RECORD_BPINFO_FILE) + strlen(record->dir) + 1];
	char *bp_info_buffer = NULL;
	io_stream_t getbp_stream = NULL;
	char *tmp;
	char *tmp_end;
	int ret = 0;

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "%s/%s", record->dir, RECORD_BPINFO_FILE);
	/* check the wav_path record file exists  */
	if (check_file_dir_exists(file_name)) {
		SYS_LOG_INF("%s not exist\n", file_name);
		record->file_count = 0;
		return;
	}

	bp_info_buffer = app_mem_malloc(RECORD_BPINFO_LEN);
	if (!bp_info_buffer) {
		SYS_LOG_ERR("malloc failed\n");
		return;
	}
	SYS_LOG_DBG("file name:%s\n", file_name);
	getbp_stream = file_stream_create((void *)file_name);
	if (getbp_stream) {
		ret = stream_open(getbp_stream, MODE_IN);
		if (ret) {
			SYS_LOG_ERR("stream open failed %d\n", ret);
			stream_destroy(getbp_stream);
			getbp_stream = NULL;
			goto exit;
		}
	} else {
		SYS_LOG_ERR("stream create failed !!\n");
		goto exit;
	}
	stream_read(getbp_stream, bp_info_buffer, RECORD_BPINFO_LEN);
	SYS_LOG_DBG("bp_buffer :%s\n", bp_info_buffer);
	if (strlen(bp_info_buffer) > 0) {
		tmp = strstr(bp_info_buffer, COUNT_TAG);
		if (!tmp) {
			SYS_LOG_WRN("count null\n");
			goto exit;
		}
		tmp += strlen(COUNT_TAG);
		tmp_end = strstr(tmp, COUNT_END_TAG);
		if (!tmp_end) {
			SYS_LOG_WRN("count not end flag\n");
			goto exit;
		}
		tmp_end[0] = 0;
		record->file_count = atoi(tmp);
		if (record->file_count > MAX_RECORD_FILES)
			SYS_LOG_WRN("count invalid : %d\n", record->file_count);
	} else {
		SYS_LOG_WRN("file: %s is null\n", RECORD_BPINFO_FILE);
		goto exit;
	}
	stream_close(getbp_stream);
	stream_destroy(getbp_stream);
	getbp_stream = NULL;

	app_mem_free(bp_info_buffer);
	bp_info_buffer = NULL;
	return;
exit:
	if (getbp_stream) {
		stream_close(getbp_stream);
		stream_destroy(getbp_stream);
		getbp_stream = NULL;
	}
	if (bp_info_buffer) {
		app_mem_free(bp_info_buffer);
		bp_info_buffer = NULL;
	}
}
void recorder_set_file_count(struct recorder_app_t *record)
{
	char file_name[sizeof(RECORD_BPINFO_FILE) + strlen(record->dir) + 1];
	char *bp_info_buffer = NULL;
	io_stream_t setbp_stream = NULL;
	int ret = 0;

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "%s/%s", record->dir, RECORD_BPINFO_FILE);

	bp_info_buffer = app_mem_malloc(RECORD_BPINFO_LEN);
	if (!bp_info_buffer) {
		SYS_LOG_ERR("malloc failed\n");
		return;
	}
	SYS_LOG_DBG("file:%s\n", file_name);
	setbp_stream = file_stream_create((void *)file_name);
	if (setbp_stream) {
		ret = stream_open(setbp_stream, MODE_OUT);
		if (ret) {
			SYS_LOG_ERR("stream open failed %d\n", ret);
			stream_destroy(setbp_stream);
			setbp_stream = NULL;
			goto exit;
		}
	} else {
		SYS_LOG_ERR("stream create failed !!\n");
		goto exit;
	}
	memset(bp_info_buffer, 0, RECORD_BPINFO_LEN);
	snprintf(bp_info_buffer, RECORD_BPINFO_LEN, "%s%d,", COUNT_TAG, record->file_count);
	stream_write(setbp_stream, bp_info_buffer, RECORD_BPINFO_LEN);

	stream_close(setbp_stream);
	stream_destroy(setbp_stream);
	setbp_stream = NULL;

	app_mem_free(bp_info_buffer);
	bp_info_buffer = NULL;
	return;
exit:
	if (setbp_stream) {
		stream_close(setbp_stream);
		stream_destroy(setbp_stream);
		setbp_stream = NULL;
	}
	if (bp_info_buffer) {
		app_mem_free(bp_info_buffer);
		bp_info_buffer = NULL;
	}
}

int recorder_get_status(void)
{
	struct recorder_app_t *record = recorder_get_app();

	if (!record)
		return RECORDER_STATUS_NULL;

	if (!record->record_or_play) {
		if (record->recording)
			return record->recording;
		else
			return RECORDER_STATUS_STOP;
	} else {
		if (!record->music_state)
			return RECFILE_STATUS_STOP;
		else
			return record->music_state;
	}
	return 0;
}
static u8_t record_sample_rate_kh = 48;
void recorder_sample_rate_set(u8_t sample_rate_kh)
{
	record_sample_rate_kh = sample_rate_kh;
	SYS_LOG_INF("record_sample_rate_kh= %d ", record_sample_rate_kh);
}
static u8_t _recorder_sample_rate_get(void)
{
	return record_sample_rate_kh;
}

void recorder_start(struct recorder_app_t *record)
{
	media_init_param_t init_param;
	char *file_path = NULL;
	struct app_msg msg = { 0 };

	io_stream_t file_stream = NULL;
	int res = 0;
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	record->disk_space_size = fs_manager_get_free_capacity(strncmp(record->dir, "SD:", strlen("SD:")) ? "USB:":"SD:");
#endif
	if (record->disk_space_size < RECORD_DISK_FREE_SPACE_MIN) {
		SYS_LOG_WRN("less than [%d]M byte,exit app\n", RECORD_DISK_FREE_SPACE_MIN);
		goto exit_app;
	}
	if (record->disk_space_size <= RECORD_FILE_SIZE_MAX) {
		SYS_LOG_WRN("less than [%d]M byte\n", RECORD_FILE_SIZE_MAX);
		record->disk_space_less = 1;
	} else {
		record->disk_space_less = 0;
	}

	record->file_is_full = 0;
#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(true);
#endif
	if (record->player) {
		recorder_stop(record);
	}

	file_path = _record_create_file_path(record);
	if (!file_path) {
		recorder_set_file_count(record);
		goto exit_app;
	}
	SYS_LOG_INF("file:%s\n", file_path);
	file_stream = file_stream_create((void *)file_path);
	if (file_stream) {
		res = stream_open(file_stream, MODE_OUT);
		if (res) {
			SYS_LOG_ERR("stream open failed %d\n", res);
			stream_destroy(file_stream);
			file_stream = NULL;
			goto exit;
		}
	} else {
		SYS_LOG_ERR("stream create failed !!\n");
		goto exit;
	}
	recorder_set_file_count(record);

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_LOCAL_RECORD;
	init_param.support_tws = 0;

	init_param.capture_format = WAV_TYPE;
	init_param.capture_sample_rate_input = _recorder_sample_rate_get();
	init_param.capture_sample_rate_output = _recorder_sample_rate_get();
	init_param.capture_channels = 1;
	init_param.capture_bit_rate = init_param.capture_sample_rate_input * init_param.capture_channels * 16 / 4;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = _recorder_create_outputstream();

	record->player = media_player_open(&init_param);
	if (!record->player) {
		SYS_LOG_ERR("open failed\n");
		goto exit;
	}

	record->cache_stream = init_param.capture_output_stream;
	record->recorder_stream = file_stream;

	stream_set_observer(record->cache_stream, NULL, record_stream_observer_notify, STREAM_NOTIFY_WRITE);

	media_player_play(record->player);

	record->recording = RECORDER_STATUS_START;

	SYS_LOG_INF("%p ok\n", record->player);

#ifdef CONFIG_PROPERTY
	property_set("RECORDER_BP_INFO", (char *)&record->recplayer_bp_info, sizeof(record->recplayer_bp_info));
#endif

	thread_timer_start(&record->monitor_timer, MONITOR_TIME_PERIOD, MONITOR_TIME_PERIOD);
	thread_timer_start(&record->read_cache_timer, READ_CACHE_TIME_PERIOD, READ_CACHE_TIME_PERIOD);
	recorder_store_play_state();
	recorder_display_record_start(record);

	return;
exit:
	if (file_stream) {
		stream_close(file_stream);
		stream_destroy(file_stream);
		file_stream = NULL;
		/* delete the null file */
		fs_unlink(record->recplayer_bp_info.file_path);
	}

	if (record->cache_stream) {
		stream_close(record->cache_stream);
		stream_destroy(record->cache_stream);
		record->cache_stream = NULL;
	}

	return;
exit_app:
	msg.type = MSG_INPUT_EVENT;
	msg.cmd = MSG_SWITCH_APP;
	send_async_msg(APP_ID_MAIN, &msg);/* change to next app */
}
static void _recorder_sync_cache_to_disk(struct recorder_app_t *record)
{
	int len = 0;

	if (!record->recorder_stream || !record->cache_stream)
		return;
	while ((len = stream_get_length(record->cache_stream)) > 0) {
		SYS_LOG_INF("len:%d\n", len);
		if (len > sizeof(record_cache_read_temp))
			len = sizeof(record_cache_read_temp);
		len = stream_read(record->cache_stream, record_cache_read_temp, len);
		if (len < 0) {
			SYS_LOG_ERR("error\n");
			break;
		}

		stream_write(record->recorder_stream, record_cache_read_temp, len);

		if (len != sizeof(record_cache_read_temp)) {
			SYS_LOG_INF("len:%d\n", len);
			break;
		}
	}
	media_player_repair_filhdr(NULL, WAV_TYPE, record->recorder_stream);
}
void recorder_stop(struct recorder_app_t *record)
{
	if (!record->player)
		return;

	if (thread_timer_is_running(&record->monitor_timer)) {
		thread_timer_stop(&record->monitor_timer);
		record->record_time = 0;
	}

	media_player_stop(record->player);

	media_player_close(record->player);

	record->recording = RECORDER_STATUS_NULL;

	record->player = NULL;

	if (thread_timer_is_running(&record->read_cache_timer))
		thread_timer_stop(&record->read_cache_timer);

	_recorder_sync_cache_to_disk(record);

	if (record->recorder_stream) {
		stream_close(record->recorder_stream);
		stream_destroy(record->recorder_stream);
		record->recorder_stream = NULL;
	}

	if (record->cache_stream) {
		stream_close(record->cache_stream);
		stream_destroy(record->cache_stream);
		record->recorder_stream = NULL;
	}

	SYS_LOG_INF("ok\n");
}
void recorder_pause(struct recorder_app_t *record)
{
	if (!record->player)
		return;
	media_player_pause(record->player);
	record->recording = RECORDER_STATUS_STOP;

	recorder_store_play_state();

	recorder_display_record_pause(record);

	SYS_LOG_INF("ok\n");
}

void recorder_resume(struct recorder_app_t *record)
{
	if (!record->player)
		return;
	media_player_resume(record->player);
	record->recording = RECORDER_STATUS_START;

	recorder_store_play_state();

	recorder_display_record_start(record);

	SYS_LOG_INF("ok\n");
}
