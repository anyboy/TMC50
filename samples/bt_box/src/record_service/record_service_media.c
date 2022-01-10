/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record service media
 */

#include <logging/sys_log.h>
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <file_stream.h>
#include <dvfs.h>
#include <thread_timer.h>
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#include <hotplug_manager.h>
#ifdef CONFIG_FS_MANAGER
#include <fs_manager.h>
#endif
#include "record_service.h"
#include <ui_manager.h>
#include <bt_manager.h>
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define DISK_FREE_SPACE_MIN	(1)	/* 1MB */
#define RECORD_DISK_FREE_SPACE_MIN	(10)	/* 10MB */
#define RECORD_FILE_SIZE_MAX	(512)	/* 512MB */

#define SYNC_RECORD_DATA_PERIOD OS_MSEC(20)
#define RECORD_BPINFO_LEN	(64)
#define FILE_FORMAT	".WAV"
#define RECORDER_SRV_READ_CACHE_LEN (0x800)

const char RECORDER_BPINFO_FILE[] = "_record.log";
const char RECORDER_COUNT_TAG[] = "file_count:";
const char RECORDER_COUNT_END_TAG[] = ",";

static struct recorder_service_t *p_recorder_service = NULL;

static media_record_info_t g_record_info;
static bool need_record = false;
OS_MUTEX_DEFINE(media_record_info_mutex);
static char recorder_srv_adpcm_cache[0x1000];


extern io_stream_t recorder_srv_stream_create_ext(void *ring_buff, u32_t ring_buff_size);
extern int recorder_srv_get_cache_addr(io_stream_t handle, unsigned char **buf, int len);
static io_stream_t _recorder_srv_create_outputstream(void)
{
	int ret = 0;

	memset(recorder_srv_adpcm_cache, 0, sizeof(recorder_srv_adpcm_cache));
	io_stream_t output_stream = recorder_srv_stream_create_ext(recorder_srv_adpcm_cache, sizeof(recorder_srv_adpcm_cache));

	if (!output_stream) {
		goto exit;
	}

	ret = stream_open(output_stream, MODE_IN_OUT | MODE_WRITE_BLOCK | MODE_BLOCK_TIMEOUT);
	if (ret) {
		stream_destroy(output_stream);
		output_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", output_stream);
	return output_stream;
}

static void _recorder_service_set_record_info(bool is_record, media_record_info_t *record_info)
{
	os_mutex_lock(&media_record_info_mutex, OS_FOREVER);

	need_record = is_record;
	if (record_info && need_record) {
		g_record_info.format = record_info->format;
		g_record_info.bit_rate = record_info->bit_rate;
		g_record_info.stream = record_info->stream;
	}
	os_mutex_unlock(&media_record_info_mutex);
}
static void _recorder_service_get_record_info(media_record_info_t *record_info)
{
	os_mutex_lock(&media_record_info_mutex, OS_FOREVER);

	if (record_info) {
		record_info->sample_rate = g_record_info.sample_rate;
		record_info->channels = g_record_info.channels;
		record_info->type = g_record_info.type;
	}
	os_mutex_unlock(&media_record_info_mutex);
}

static void _recorder_service_restart_record(void)
{
	struct app_msg new_msg = { 0 };

	new_msg.type = MSG_INPUT_EVENT;
	new_msg.cmd = MSG_RECORDER_START_STOP;
	new_msg.reserve = 1;/* stop service record */

	send_async_msg(APP_ID_MAIN, &new_msg);
	new_msg.reserve = 0;/* start service record */
	send_async_msg(APP_ID_MAIN, &new_msg);
}
static void _recorder_service_set_icon_flash(bool is_flash)
{
#ifdef CONFIG_SEG_LED_MANAGER
	if (p_recorder_service) {
		if (is_flash) {
			if (strncmp(p_recorder_service->dir, "SD:", strlen("SD:")) == 0) {
				seg_led_display_icon(SLED_SD, false);
				seg_led_display_set_flash(FLASH_ITEM(SLED_SD), 1000, FLASH_FOREVER, NULL);
			} else {
				seg_led_display_icon(SLED_USB, false);
				seg_led_display_set_flash(FLASH_ITEM(SLED_USB), 1000, FLASH_FOREVER, NULL);
			}
		} else {
			if (strncmp(p_recorder_service->dir, "SD:", strlen("SD:")) == 0) {
				seg_led_display_set_flash(0, 1000, FLASH_FOREVER, NULL);
				seg_led_display_icon(SLED_SD, true);
			} else {
				seg_led_display_set_flash(0, 1000, FLASH_FOREVER, NULL);
				seg_led_display_icon(SLED_USB, true);
			}
		}
	}
#endif
}

static void _recorder_service_exit_display(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_set_flash(0, 1000, FLASH_FOREVER, NULL);
	if (strncmp(p_recorder_service->dir, "SD:", strlen("SD:")) == 0) {
		if (strncmp(app_manager_get_current_app(), APP_ID_SD_MPLAYER, strlen(APP_ID_SD_MPLAYER)))
			seg_led_display_icon(SLED_SD, false);
	} else {
		if (strncmp(app_manager_get_current_app(), APP_ID_UHOST_MPLAYER, strlen(APP_ID_UHOST_MPLAYER)))
			seg_led_display_icon(SLED_USB, false);
	}
#endif
}
static void _recorder_service_observer_notify(u32_t event, void *data, u32_t len, void *user_data)
{
	media_init_param_t *init_param = (media_init_param_t *)data;

	if (event == PLAYER_EVENT_OPEN) {
		if (init_param->stream_type != AUDIO_STREAM_TTS) {
			os_mutex_lock(&media_record_info_mutex, OS_FOREVER);
			if (need_record) {
				if (!g_record_info.sample_rate) {
					g_record_info.sample_rate = init_param->sample_rate;
					g_record_info.bit_rate = g_record_info.sample_rate * g_record_info.channels * 16 / 4;
					g_record_info.channels = init_param->channels;

					if (init_param->stream_type == AUDIO_STREAM_VOICE) {
						g_record_info.type = MEDIA_RECORD_PLAYBACK_AND_CAPTURE;
					} else {
						g_record_info.type = MEDIA_RECORD_PLAYBACK;
					}

					media_player_start_record(NULL, &g_record_info);
					_recorder_service_set_icon_flash(true);
					SYS_LOG_INF("start record\n");
				} else if (g_record_info.sample_rate != init_param->sample_rate ||
						g_record_info.channels != init_param->channels) {
					/*need to restart record*/
					SYS_LOG_INF("restart record\n");
					g_record_info.sample_rate = init_param->sample_rate;
					g_record_info.channels = init_param->channels;

					if (init_param->stream_type == AUDIO_STREAM_VOICE) {
						g_record_info.type = MEDIA_RECORD_PLAYBACK_AND_CAPTURE;
					} else {
						g_record_info.type = MEDIA_RECORD_PLAYBACK;
					}
					_recorder_service_restart_record();
				} else {
			/*flash icon when prev music is pause*/
					_recorder_service_set_icon_flash(true);
				}
			} else {
				g_record_info.sample_rate = init_param->sample_rate;
				g_record_info.channels = init_param->channels;

				if (init_param->stream_type == AUDIO_STREAM_VOICE) {
					g_record_info.type = MEDIA_RECORD_PLAYBACK_AND_CAPTURE;
				} else {
					g_record_info.type = MEDIA_RECORD_PLAYBACK;
				}
			}
			os_mutex_unlock(&media_record_info_mutex);
		}
	} else if (event == PLAYER_EVENT_STOP) {
		if (init_param->stream_type != AUDIO_STREAM_TTS) {
			os_mutex_lock(&media_record_info_mutex, OS_FOREVER);
			g_record_info.sample_rate = 0;
			g_record_info.channels = 0;
			os_mutex_unlock(&media_record_info_mutex);
			if (need_record) {
				/*icon stop flash,and keep light*/
				_recorder_service_set_icon_flash(false);
			}
		}
	}
}
void recorder_service_set_media_notify(void)
{
	media_player_set_lifecycle_notifier(_recorder_service_observer_notify);

}
static void _recorder_stream_observer_notify(void *observer, int readoff, int writeoff, int total_size,
										unsigned char *buf, int num, stream_notify_type type)
{
	struct app_msg msg = {
		msg.type = MSG_INPUT_EVENT,
		msg.cmd = MSG_RECORDER_START_STOP,
		msg.reserve = 1,
	};

	/*record file more than 512M byte will start next*/
	if (writeoff >= RECORD_FILE_SIZE_MAX * 1024 * 1024) {
		SYS_LOG_INF("%d B more than 512M\n", writeoff);
		if (!p_recorder_service->file_is_full) {
			p_recorder_service->file_is_full = 1;
			/* stop service record */
			send_async_msg(APP_ID_MAIN, &msg);
			msg.reserve = 0,
			/* start service record */
			send_async_msg(APP_ID_MAIN, &msg);
		}
	}
	if (p_recorder_service->disk_space_less) {
		if (writeoff >= (p_recorder_service->disk_space_size - DISK_FREE_SPACE_MIN) * 1024 * 1024) {
			SYS_LOG_WRN("no space\n");
			p_recorder_service->disk_space_less = 0;
			msg.reserve = 1,
			/* stop service record */
			send_async_msg(APP_ID_MAIN, &msg);
		}
	}
}

static u32_t cost = 0;
static void _recorder_srv_read_cache(void)
{
	int sum_len = 0;
	unsigned char *temp_cache_buf = NULL;
	u32_t begin = k_cycle_get_32();

	if (begin - cost > 24000 * (SYNC_RECORD_DATA_PERIOD + 10))
		printk("call timer cost %d us\n", (begin - cost)/24);

	while (stream_get_length(p_recorder_service->cache_stream) >= RECORDER_SRV_READ_CACHE_LEN) {
		if (recorder_srv_get_cache_addr(p_recorder_service->cache_stream, &temp_cache_buf, RECORDER_SRV_READ_CACHE_LEN) < 0) {
			SYS_LOG_ERR("error\n");
			return;
		}
		stream_write(p_recorder_service->output_stream, temp_cache_buf, RECORDER_SRV_READ_CACHE_LEN);
		sum_len += RECORDER_SRV_READ_CACHE_LEN;
	}
	cost = k_cycle_get_32();

	if (cost - begin > 24000 * 20)
		printk("write %d bytes use %d us\n", sum_len, (cost - begin)/24);
}

static void _recorder_sync_data_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	int res = 0;

	if (!p_recorder_service || !p_recorder_service->output_stream || !p_recorder_service->cache_stream)
		return;

	_recorder_srv_read_cache();

	if (++p_recorder_service->read_cache_times < 30000/SYNC_RECORD_DATA_PERIOD)/*30s sync data to disk*/
		return;

	p_recorder_service->read_cache_times = 0;

	res = stream_flush(p_recorder_service->output_stream);

	if (res) {
		SYS_LOG_ERR("sync fail\n");
	} else {
		printk("sync ok\n");
	}
}

struct recorder_service_t *recorder_get_service(void)
{
	return p_recorder_service;
}
static u32_t _recorder_service_get_count_from_name(char *name)
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
static void _recorder_service_set_file_bitmaps(u16_t file_count)
{
	if (file_count && file_count <= MAX_RECORD_FILES) {
		file_count--;
		p_recorder_service->file_bitmaps[file_count / 32] |= 1 << (file_count % 32);
	}
}

static int _recorder_service_update_file_bitmaps(void)
{
	int res;
	u32_t count = 0;
	struct fs_dirent *entry = app_mem_malloc(sizeof(struct fs_dirent));
	fs_dir_t *dirp = app_mem_malloc(sizeof(fs_dir_t));

	if (!entry || !dirp)
		return -ENOMEM;

	memset(p_recorder_service->file_bitmaps, 0, sizeof(p_recorder_service->file_bitmaps));
	/* Verify fs_opendir() */
	res = fs_opendir(dirp, p_recorder_service->dir);
	if (res) {
		SYS_LOG_ERR("Error opening dir %s [%d]\n", p_recorder_service->dir, res);
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

		count = _recorder_service_get_count_from_name(entry->name);
		if (count && count <= MAX_RECORD_FILES) {
			count--;
			p_recorder_service->file_bitmaps[count / 32] |= 1 << (count % 32);
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
		if (res) {
			SYS_LOG_INF("%s not found (res=%d)\n", path, res);
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

static char *_recorder_service_create_file_path(void)
{
	int check_file_times = 1;

	while (check_file_times++ <= MAX_RECORD_FILES) {
		memset(p_recorder_service->file_path, 0, sizeof(p_recorder_service->file_path));
		if (p_recorder_service->file_count >= MAX_RECORD_FILES) {
			p_recorder_service->file_count = 0;
		}

		if ((1 << (p_recorder_service->file_count % 32)) & p_recorder_service->file_bitmaps[p_recorder_service->file_count / 32]) {
			p_recorder_service->file_count++;
			continue;
		}
		snprintf(p_recorder_service->file_path, sizeof(p_recorder_service->file_path), "%s/REC%03u%s", p_recorder_service->dir, ++p_recorder_service->file_count, FILE_FORMAT);
		_recorder_service_set_file_bitmaps(p_recorder_service->file_count);
		return p_recorder_service->file_path;
	}
	SYS_LOG_WRN("file_count %d more than %d\n", p_recorder_service->file_count, MAX_RECORD_FILES);
	p_recorder_service->file_count = 0;
	return NULL;
}

static void _recorder_service_repair_file(void)
{
	char *file_path = NULL;
	io_stream_t stream_wav;
	int res = 0;

#ifdef CONFIG_PROPERTY
	property_get("RECORD_FILE_PATH", p_recorder_service->file_path, sizeof(p_recorder_service->file_path));
#endif
	file_path = strstr(p_recorder_service->file_path, FILE_FORMAT);
	if (!file_path)
		return;
	if (check_file_dir_exists(p_recorder_service->file_path)) {
		SYS_LOG_WRN("file:%s not exist\n", p_recorder_service->file_path);
		return;
	}
	SYS_LOG_INF("path:%s\n", p_recorder_service->file_path);
	stream_wav = file_stream_create((void *)p_recorder_service->file_path);
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

void _recorder_service_get_file_count(void)
{
	char file_name[sizeof(RECORDER_BPINFO_FILE) + strlen(p_recorder_service->dir) + 1];
	char *bp_info_buffer = NULL;
	io_stream_t getbp_stream = NULL;
	char *tmp;
	char *tmp_end;
	int ret = 0;

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "%s/%s", p_recorder_service->dir, RECORDER_BPINFO_FILE);
	/* check the wav_path record file exists  */
	if (check_file_dir_exists(file_name)) {
		SYS_LOG_INF("%s is not exist\n", file_name);
		p_recorder_service->file_count = 0;
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
		tmp = strstr(bp_info_buffer, RECORDER_COUNT_TAG);
		if (!tmp) {
			SYS_LOG_WRN("count null\n");
			goto exit;
		}
		tmp += strlen(RECORDER_COUNT_TAG);
		tmp_end = strstr(tmp, RECORDER_COUNT_END_TAG);
		if (!tmp_end) {
			SYS_LOG_WRN("count not end flag\n");
			goto exit;
		}
		tmp_end[0] = 0;
		p_recorder_service->file_count = atoi(tmp);
		if (p_recorder_service->file_count > MAX_RECORD_FILES)
			SYS_LOG_WRN("count invalid : %d\n", p_recorder_service->file_count);
	} else {
		SYS_LOG_WRN("file: %s is null\n", RECORDER_BPINFO_FILE);
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
static void _recorder_service_set_file_count(void)
{
	char file_name[sizeof(RECORDER_BPINFO_FILE) + strlen(p_recorder_service->dir) + 1];
	char *bp_info_buffer = NULL;
	io_stream_t setbp_stream = NULL;
	int ret = 0;

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "%s/%s", p_recorder_service->dir, RECORDER_BPINFO_FILE);

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
	snprintf(bp_info_buffer, RECORD_BPINFO_LEN, "%s%d,", RECORDER_COUNT_TAG, p_recorder_service->file_count);
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

static int _recorder_service_init(void)
{
	if (p_recorder_service)
		return 0;

	p_recorder_service = app_mem_malloc(sizeof(struct recorder_service_t));
	if (!p_recorder_service) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}

	memset(p_recorder_service, 0, sizeof(struct recorder_service_t));

	if (strncmp(app_manager_get_current_app(), APP_ID_UHOST_MPLAYER, strlen(APP_ID_UHOST_MPLAYER)) == 0) {
		if (fs_manager_get_volume_state("SD:")) {
			strncpy(p_recorder_service->dir, "SD:RECORD", sizeof(p_recorder_service->dir));
		} else {
			goto exit;
		}
	} else if (strncmp(app_manager_get_current_app(), APP_ID_SD_MPLAYER, strlen(APP_ID_SD_MPLAYER)) == 0) {
		if (fs_manager_get_volume_state("USB:")) {
			strncpy(p_recorder_service->dir, "USB:RECORD", sizeof(p_recorder_service->dir));
		} else {
			goto exit;
		}
	} else {
		if (fs_manager_get_volume_state("USB:")) {
			strncpy(p_recorder_service->dir, "USB:RECORD", sizeof(p_recorder_service->dir));
		} else if (fs_manager_get_volume_state("SD:")) {
			strncpy(p_recorder_service->dir, "SD:RECORD", sizeof(p_recorder_service->dir));
		} else {
			goto exit;
		}
	}

	if (check_file_dir_exists(p_recorder_service->dir)) {
		int res = fs_mkdir(p_recorder_service->dir);

		if (res) {
			SYS_LOG_ERR("Error creating dir \"%s\" [%d]\n", p_recorder_service->dir, res);
			goto exit;
		}
	}
	_recorder_service_repair_file();
	_recorder_service_get_file_count();
	u32_t begin = k_cycle_get_32();

	_recorder_service_update_file_bitmaps();
	SYS_LOG_INF("update bitmaps :%d us\n", (k_cycle_get_32() - begin)/24);

	SYS_LOG_INF("ok\n");
	return 0;
exit:
	SYS_LOG_ERR("no disk!\n");
	app_mem_free(p_recorder_service);
	p_recorder_service = NULL;
	return -ENXIO;
}

static void _recorder_srv_sync_cache_to_disk(void)
{
	int len = 0;
	unsigned char *temp_cache_buf = NULL;

	if (!p_recorder_service->output_stream || !p_recorder_service->cache_stream)
		return;
	while ((len = stream_get_length(p_recorder_service->cache_stream)) > 0) {
		SYS_LOG_INF("len:%d\n", len);
		if (len > RECORDER_SRV_READ_CACHE_LEN)
			len = RECORDER_SRV_READ_CACHE_LEN;

		if (recorder_srv_get_cache_addr(p_recorder_service->cache_stream, &temp_cache_buf, len) < 0) {
			SYS_LOG_ERR("error\n");
			break;
		}

		stream_write(p_recorder_service->output_stream, temp_cache_buf, len);

		if (len != RECORDER_SRV_READ_CACHE_LEN) {
			SYS_LOG_INF("len:%d\n", len);
			break;
		}
	}
	media_player_repair_filhdr(NULL, WAV_TYPE, p_recorder_service->output_stream);
}

static void _recorder_service_stop(void)
{
	if (!p_recorder_service)
		return;

	if (thread_timer_is_running(&p_recorder_service->sync_data_timer))
		thread_timer_stop(&p_recorder_service->sync_data_timer);

	_recorder_service_set_record_info(false, NULL);

	media_player_stop_record(NULL);

	_recorder_srv_sync_cache_to_disk();

	if (p_recorder_service->cache_stream) {
		stream_close(p_recorder_service->cache_stream);
		stream_destroy(p_recorder_service->cache_stream);
		p_recorder_service->cache_stream = NULL;
	}

	if (p_recorder_service->output_stream) {
		stream_close(p_recorder_service->output_stream);
		stream_destroy(p_recorder_service->output_stream);
		p_recorder_service->output_stream = NULL;
	}
	p_recorder_service->recording = 0;

	SYS_LOG_INF("ok\n");

}

static int _recorder_service_start(void)
{
	media_record_info_t record_info;
	char *file_path = NULL;

	io_stream_t file_stream = NULL;
	int res = 0;

#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	p_recorder_service->disk_space_size = fs_manager_get_free_capacity(strncmp(p_recorder_service->dir, "SD:", strlen("SD:")) ? "USB:":"SD:");
#endif
	if (p_recorder_service->disk_space_size < RECORD_DISK_FREE_SPACE_MIN) {
		SYS_LOG_WRN("less than [%d]M byte,exit app\n", RECORD_DISK_FREE_SPACE_MIN);
		return -ENOSPC;
	}
	if (p_recorder_service->disk_space_size <= RECORD_FILE_SIZE_MAX) {
		SYS_LOG_WRN("less than [%d]M byte\n", RECORD_FILE_SIZE_MAX);
		p_recorder_service->disk_space_less = 1;
	} else {
		p_recorder_service->disk_space_less = 0;
	}
	p_recorder_service->file_is_full = 0;

	if (p_recorder_service->recording) {
		_recorder_service_stop();
	}

	file_path = _recorder_service_create_file_path();
	if (!file_path) {
		_recorder_service_set_file_count();
		goto exit;
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
	_recorder_service_set_file_count();
	p_recorder_service->cache_stream = _recorder_srv_create_outputstream();
	stream_set_observer(p_recorder_service->cache_stream, NULL, _recorder_stream_observer_notify, STREAM_NOTIFY_WRITE);

	memset(&record_info, 0, sizeof(media_record_info_t));

	_recorder_service_get_record_info(&record_info);

	record_info.format = WAV_TYPE;
	record_info.bit_rate = record_info.sample_rate * record_info.channels * 16 / 4;
	record_info.stream = p_recorder_service->cache_stream;
	if (record_info.sample_rate) {
		res = media_player_start_record(NULL, &record_info);
		if (res) {
			SYS_LOG_ERR("start fail!\n");
			goto exit;
		}
		_recorder_service_set_icon_flash(true);
	} else {
		SYS_LOG_WRN("no media player\n");
		_recorder_service_set_icon_flash(false);
	}
	_recorder_service_set_record_info(true, &record_info);

	p_recorder_service->output_stream = file_stream;
	p_recorder_service->recording = 1;

	SYS_LOG_INF("ok\n");

#ifdef CONFIG_PROPERTY
	property_set("RECORD_FILE_PATH", p_recorder_service->file_path, strlen(p_recorder_service->file_path) + 1);
#endif
	thread_timer_init(&p_recorder_service->sync_data_timer, _recorder_sync_data_timer_handle, NULL);
	thread_timer_start(&p_recorder_service->sync_data_timer, SYNC_RECORD_DATA_PERIOD, SYNC_RECORD_DATA_PERIOD);

	return 0;
exit:
	if (p_recorder_service->cache_stream) {
		stream_close(p_recorder_service->cache_stream);
		stream_destroy(p_recorder_service->cache_stream);
		p_recorder_service->cache_stream = NULL;
	}

	if (p_recorder_service->output_stream) {
		stream_close(p_recorder_service->output_stream);
		stream_destroy(p_recorder_service->output_stream);
		p_recorder_service->output_stream = NULL;
		/* delete the null file */
		fs_unlink(p_recorder_service->file_path);
	}
	return -1;
}

static void _recorder_service_exit(void)
{
	if (!p_recorder_service)
		return;
	_recorder_service_stop();

	_recorder_service_exit_display();

	app_mem_free(p_recorder_service);

	p_recorder_service = NULL;
}
void recorder_service_app_switch_exit(void)
{
	if (!p_recorder_service)
		return;

	struct app_msg msg = { 0 };

	msg.type = MSG_INPUT_EVENT;
	msg.cmd = MSG_RECORDER_START_STOP;
	msg.reserve = 1;/* stop service record */
	send_async_msg(APP_ID_MAIN, &msg);
}

void recorder_service_start_stop(u8_t is_stop)
{
	if (!p_recorder_service && is_stop)
		return;

	if (!p_recorder_service) {
		if (strncmp(app_manager_get_current_app(), APP_ID_RECORDER, strlen(APP_ID_RECORDER)) == 0)
			return;

		if (bt_manager_tws_get_dev_role() != BTSRV_TWS_NONE)
			return;

		#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
			dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "record_srv");
		#endif
		if (_recorder_service_init()) {
			SYS_LOG_ERR("init failed\n");
		#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
			dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "record_srv");
		#endif
			return;
		}
	#ifdef CONFIG_FS_MANAGER
		fs_manager_sdcard_enter_high_speed();
	#endif
		if (_recorder_service_start()) {
			SYS_LOG_ERR("start failed\n");
			_recorder_service_exit();
		#ifdef CONFIG_FS_MANAGER
			fs_manager_sdcard_exit_high_speed();
		#endif
		#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
			dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "record_srv");
		#endif
		}
		SYS_LOG_INF("start finished\n");
	} else {
		_recorder_service_exit();
	#ifdef CONFIG_FS_MANAGER
		fs_manager_sdcard_exit_high_speed();
	#endif
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "record_srv");
	#endif
		SYS_LOG_INF("stop finished\n");
	}
}

void recorder_service_check_disk(const char *disk)
{
	if (p_recorder_service) {
		if (strncmp(p_recorder_service->dir, disk, strlen(disk)) == 0)
			recorder_service_start_stop(0x01);
	}
}

