/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file media player interface
 */
#define SYS_LOG_DOMAIN "media"
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <srv_manager.h>
#include <sys_wakelock.h>
#include <bt_manager.h>
#include <string.h>
#include <misc/byteorder.h>
#include "audio_policy.h"
#include "audio_system.h"
#include "media_player.h"
#include "media_service.h"
#include <property_manager.h>
#include <sdfs.h>
#include <media_effect_param.h>
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

static OS_MUTEX_DEFINE(media_srv_mutex);

static int media_player_ref_cnt;

static media_player_t *current_media_dumpable_player;
static media_player_t *current_media_main_player;

/* the media_player_t will be return as parameter "uesr_data" */
static media_srv_event_notify_t pfn_lifecycle_notify;

static void _notify_player_lifecycle_changed(media_player_t *player, u32_t event, void *data, u32_t len)
{
	if (pfn_lifecycle_notify)
		pfn_lifecycle_notify(event, data, len, player);
}

static void _media_service_default_callback(struct app_msg *msg, int result, void *reply)
{
	if (msg->sync_sem) {
		os_sem_give((os_sem *)msg->sync_sem);
	}
}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
static int _media_player_get_dvfs_level(int stream_type, int format, bool is_tws, bool is_dumpable)
{
	int dvfs_level = DVFS_LEVEL_NORMAL;

	if (is_tws) {
		if (stream_type == AUDIO_STREAM_MUSIC && (format == SBC_TYPE)) {
		#ifdef CONFIG_MUSIC_DRC_EFFECT
			dvfs_level = DVFS_LEVEL_PERFORMANCE;
		#else
		#ifdef CONFIG_MUSIC_EFFECT
			dvfs_level = DVFS_LEVEL_NORMAL;
		#else
			dvfs_level = DVFS_LEVEL_NORMAL_NO_EFX;
		#endif
		#endif
		} else {
			dvfs_level = DVFS_LEVEL_HIGH_PERFORMANCE;
		}
	} else {
		switch (stream_type) {
		case AUDIO_STREAM_VOICE:
			if (system_check_low_latencey_mode()) {
				dvfs_level = DVFS_LEVEL_HIGH_PERFORMANCE;
			} else {
				dvfs_level = DVFS_LEVEL_MIDDLE;
			}
			break;
		case AUDIO_STREAM_MUSIC:
			if (format == SBC_TYPE) {
			#ifdef CONFIG_MUSIC_DRC_EFFECT
				dvfs_level = DVFS_LEVEL_MIDDLE;
			#else
			#ifdef CONFIG_MUSIC_EFFECT
				dvfs_level = DVFS_LEVEL_NORMAL_NO_DRC;
			#else
				dvfs_level = DVFS_LEVEL_NORMAL_NO_EFX;
			#endif
			#endif
			} else {
			#ifdef CONFIG_MUSIC_DRC_EFFECT
			if (system_check_low_latencey_mode()) {
				dvfs_level = DVFS_LEVEL_PERFORMANCE;
			} else {
				dvfs_level = DVFS_LEVEL_MIDDLE;
			}
			#else
			#ifdef CONFIG_MUSIC_EFFECT
				dvfs_level = DVFS_LEVEL_NORMAL;
			#else
				dvfs_level = DVFS_LEVEL_NORMAL_NO_DRC;
			#endif
			#endif
			}
			break;
		case AUDIO_STREAM_LOCAL_RECORD:
		case AUDIO_STREAM_FM:
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_MIC_IN:
		#ifdef CONFIG_MUSIC_DRC_EFFECT
			dvfs_level = DVFS_LEVEL_NORMAL;
		#else
			dvfs_level = DVFS_LEVEL_NORMAL_NO_DRC;
		#endif
			break;
		case AUDIO_STREAM_SPDIF_IN:
			dvfs_level = DVFS_LEVEL_HIGH_PERFORMANCE;
			break;
		case AUDIO_STREAM_LOCAL_MUSIC:
			dvfs_level = DVFS_LEVEL_PERFORMANCE;
			break;
		case AUDIO_STREAM_GMA_RECORD:
			dvfs_level = DVFS_LEVEL_PERFORMANCE;
			break;
		case AUDIO_STREAM_USOUND:
			dvfs_level = DVFS_LEVEL_PERFORMANCE;
			break;
		case AUDIO_STREAM_TTS:
			dvfs_level = DVFS_LEVEL_HIGH_PERFORMANCE;
			break;
		}
	}

	return dvfs_level;
}
#endif

static int _media_player_check_audio_effect(media_player_t *handle, int stream_type)
{
	bool effect_enable = true;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
	#ifndef CONFIG_VOICE_EFFECT
		effect_enable = false;
	#endif
		break;
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_TTS:
	#ifndef CONFIG_MUSIC_EFFECT
		effect_enable = false;
	#endif
		break;
	default:
		break;
	}

	if (!effect_enable) {
		media_player_set_effect_enable(handle, effect_enable);
	}

#if CONFIG_MEDIA_EFFECT_OUTMODE != MEDIA_EFFECT_OUTPUT_DEFAULT
	media_player_set_effect_output_mode(handle, CONFIG_MEDIA_EFFECT_OUTMODE);
#endif
	return 0;
}

media_player_t *media_player_open(media_init_param_t *init_param)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	media_srv_init_param_t srv_param;
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	int dvfs_level = DVFS_LEVEL_NORMAL;
#endif
	bool is_tws = false;
#ifdef CONFIG_TWS
	u8_t codec;
#endif

#ifdef CONFIG_PROPERTY
	property_flush_req_deal();
#endif

	media_player_t *handle = mem_malloc(sizeof(media_player_t));
	if (!handle) {
		return NULL;
	}

	/* fall back to media player handle */
	if (!init_param->user_data)
		init_param->user_data = handle;

	os_sem_init(&return_notify, 0, 1);

	srv_param.user_param = init_param;
	srv_param.mediasrv_handle = NULL;

#ifdef CONFIG_TWS
	/**this to get media runtime from media service*/
	srv_param.tws_observer = init_param->support_tws ? bt_manager_tws_get_runtime_observer() : NULL;

	if (srv_param.tws_observer && init_param->stream_type != AUDIO_STREAM_LOCAL_MUSIC) {
		if (init_param->format == AAC_TYPE) {
			codec = 2;		/* BT_A2DP_MPEG2 */
		} else {
			codec = 0;		/* BT_A2DP_SBC */
		}
		bt_manager_tws_notify_start_play(init_param->stream_type, codec, init_param->sample_rate);
	}
#else
	srv_param.tws_observer = NULL;
#endif

#ifdef CONFIG_TWS_MONO_MODE
	srv_param.tws_mode = MEDIA_TWS_MODE_MONO;
#else
	srv_param.tws_mode = MEDIA_TWS_MODE_STEREO;
#endif

	/* for compatibility */
	srv_param.tws_samplerate_44_48_only = 1;

	msg.type = MSG_MEDIA_SRV_OPEN;
	msg.ptr = &srv_param;
	msg.callback = _media_service_default_callback;
	msg.sync_sem = &return_notify;

	os_mutex_lock(&media_srv_mutex, OS_FOREVER);

	is_tws = srv_param.tws_observer ? true : false;

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL

	dvfs_level = _media_player_get_dvfs_level(init_param->stream_type,init_param->format, is_tws, init_param->dumpable);

	SYS_LOG_INF("tws %d type %d dvfs %d\n", is_tws, init_param->stream_type, dvfs_level);
	dvfs_set_level(dvfs_level, "media");
#endif

	if (!send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		goto error_exit;
	}

	if (os_sem_take(&return_notify, OS_FOREVER) != 0) {
		goto error_exit;
	}

	if (!srv_param.mediasrv_handle) {
		goto error_exit;
	}

#ifdef CONFIG_TWS
	if (srv_param.tws_observer && init_param->stream_type == AUDIO_STREAM_LOCAL_MUSIC) {
		codec = 0;		/* BT_A2DP_SBC */
		bt_manager_tws_notify_start_play(init_param->stream_type, codec, init_param->sample_rate);
	}
#endif

	handle->media_srv_handle = srv_param.mediasrv_handle;
	handle->type = init_param->type;
	handle->is_tws = is_tws;

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	handle->dvfs_level = dvfs_level;
#endif

	if (init_param->dumpable && !current_media_dumpable_player) {
		current_media_dumpable_player = handle;
	}

	if (!current_media_main_player)
		current_media_main_player = handle;

	media_player_ref_cnt++;

	_media_player_check_audio_effect(handle, init_param->stream_type);

	if (is_tws) {
#if CONFIG_TWS_AUDIO_OUT_MODE == 0
		int tws_mode = property_get_int("TWS_MODE", 0);
		if (tws_mode == MEDIA_OUTPUT_MODE_LEFT || tws_mode == MEDIA_OUTPUT_MODE_RIGHT) {
			media_player_set_output_mode(handle, tws_mode);
		}
#elif CONFIG_TWS_AUDIO_OUT_MODE == 1
		media_player_set_output_mode(handle, MEDIA_OUTPUT_MODE_DEFAULT);
#elif CONFIG_TWS_AUDIO_OUT_MODE == 2
		media_player_set_output_mode(handle, MEDIA_OUTPUT_MODE_DEFAULT);
		media_player_set_effect_output_mode(handle, MEDIA_EFFECT_OUTPUT_L_R_MIX);
#endif
	}

	os_mutex_unlock(&media_srv_mutex);

	_notify_player_lifecycle_changed(handle, PLAYER_EVENT_OPEN, init_param, sizeof(*init_param));

	return handle;
error_exit:
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	SYS_LOG_INF("unset tws %d type %d dvfs %d\n", is_tws, init_param->stream_type, dvfs_level);
	dvfs_unset_level(dvfs_level, "media");
#endif

	os_mutex_unlock(&media_srv_mutex);
	mem_free(handle);
	return NULL;
}

int media_player_set_extern_stream(media_player_t *handle, media_init_ext_param_t *ext_param)
{
	struct app_msg msg = {0};
	int ret = 0;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	ext_param->handle = handle->media_srv_handle;

	msg.type = MSG_MEDIA_SRV_SET_EXT_MODULE;
	msg.ptr = ext_param;

	ret = send_async_msg(MEDIA_SERVICE_NAME, &msg);
	if (!ret) {
		SYS_LOG_ERR("set external module fault");
	}

	return !ret;
}

int media_player_play(media_player_t *handle)
{
	struct app_msg msg = {0};
	int ret = 0;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	msg.type = MSG_MEDIA_SRV_PLAY;
	msg.ptr = handle->media_srv_handle;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(WAKELOCK_PLAYER);
#endif

	ret = send_async_msg(MEDIA_SERVICE_NAME, &msg);
	if (ret)
		_notify_player_lifecycle_changed(handle, PLAYER_EVENT_PLAY, NULL, 0);

	return !ret;
}

int media_player_stop(media_player_t *handle)
{
	struct app_msg msg = {0};
	int ret = 0;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	msg.type = MSG_MEDIA_SRV_STOP;
	msg.ptr = handle->media_srv_handle;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(WAKELOCK_PLAYER);
#endif

#ifdef CONFIG_TWS
	if (handle->is_tws) {
		bt_manager_tws_notify_stop_play();
	}
#endif

	ret = send_async_msg(MEDIA_SERVICE_NAME, &msg);
	if (ret)
		_notify_player_lifecycle_changed(handle, PLAYER_EVENT_STOP, NULL, 0);

	return !ret;
}

int media_player_pause(media_player_t *handle)
{
	struct app_msg msg = {0};
	int ret = 0;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	msg.type = MSG_MEDIA_SRV_PAUSE;
	msg.ptr = handle->media_srv_handle;

	ret = send_async_msg(MEDIA_SERVICE_NAME, &msg);
	if (ret)
		_notify_player_lifecycle_changed(handle, PLAYER_EVENT_PAUSE, NULL, 0);

	return !ret;
}

int media_player_resume(media_player_t *handle)
{
	struct app_msg msg = {0};
	int ret = 0;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	msg.type = MSG_MEDIA_SRV_RESUME;
	msg.ptr = handle->media_srv_handle;

	ret = send_async_msg(MEDIA_SERVICE_NAME, &msg);
	if (ret)
		_notify_player_lifecycle_changed(handle, PLAYER_EVENT_RESUNE, NULL, 0);

	return !ret;
}

int media_player_seek(media_player_t *handle, media_seek_info_t *info)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	media_srv_param_t *srv_param = NULL;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	srv_param = mem_malloc(sizeof(*srv_param));
	if (!srv_param)
		return -ENOMEM;

	os_sem_init(&return_notify, 0, 1);

	srv_param->handle = handle->media_srv_handle;
	srv_param->param.pbuf = info;
	srv_param->param.plen = sizeof(*info);

	msg.ptr = srv_param;
	msg.type = MSG_MEDIA_SRV_SEEK;
	msg.callback = _media_service_default_callback;
	msg.sync_sem = &return_notify;

	if (false == send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		mem_free(srv_param);
		return -EBUSY;
	}

	if (os_sem_take(&return_notify, OS_FOREVER)) {
		mem_free(srv_param);
		return -ETIME;
	}

	mem_free(srv_param);
	return 0;
}

int media_player_get_parameter(media_player_t *handle,
		int pname, void *param, unsigned int psize)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	media_srv_param_t *srv_param = NULL;
	int ret;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	srv_param = mem_malloc(sizeof(*srv_param));
	if (!srv_param)
		return -ENOMEM;

	os_sem_init(&return_notify, 0, 1);

	srv_param->handle = handle->media_srv_handle;
	srv_param->param.type = pname;
	srv_param->param.pbuf = param;
	srv_param->param.plen = psize;
	srv_param->result = 0;

	msg.ptr = srv_param;
	msg.type = MSG_MEDIA_SRV_GET_PARAMETER;
	msg.callback = _media_service_default_callback;
	msg.sync_sem = &return_notify;

	if (false == send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		mem_free(srv_param);
		return -EBUSY;
	}

	if (os_sem_take(&return_notify, OS_FOREVER)) {
		mem_free(srv_param);
		return -ETIME;
	}

	ret = srv_param->result;
	mem_free(srv_param);
	return ret;
}

/* use synchronized message to avoid mem_malloc for large parameter, whose psize is not zero */
int media_player_set_parameter(media_player_t *handle,
		int pname, void *param, unsigned int psize)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	media_srv_param_t *srv_param = NULL;
	int ret = 0;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	srv_param = mem_malloc(sizeof(media_srv_param_t));
	if (!srv_param)
		return -ENOMEM;

	srv_param->handle = handle->media_srv_handle;
	srv_param->param.type = pname;
	srv_param->param.pbuf = param;
	srv_param->param.plen = psize;
	srv_param->result = 0;

	msg.ptr = srv_param;
	msg.type = MSG_MEDIA_SRV_SET_PARAMETER;

	if (psize > 0) {
		os_sem_init(&return_notify, 0, 1);
		msg.callback = _media_service_default_callback;
		msg.sync_sem = &return_notify;
	}

	if (false == send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		mem_free(srv_param);
		return -EBUSY;
	}

	if (psize > 0) {
		if (os_sem_take(&return_notify, OS_FOREVER)) {
			mem_free(srv_param);
			return -ETIME;
		}

		ret = srv_param->result;
		mem_free(srv_param);
	}

	return ret;
}

int media_player_get_global_parameter(media_player_t *handle,
		int pname, void *param, unsigned int psize)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	media_srv_param_t *srv_param = NULL;
	int ret;

	srv_param = mem_malloc(sizeof(*srv_param));
	if (!srv_param)
		return -ENOMEM;

	os_sem_init(&return_notify, 0, 1);

	srv_param->handle = handle ? handle->media_srv_handle : NULL;
	srv_param->param.type = pname;
	srv_param->param.pbuf = param;
	srv_param->param.plen = psize;
	srv_param->result = 0;

	msg.ptr = srv_param;
	msg.type = MSG_MEDIA_SRV_GET_GLOBAL_PARAMETER;
	msg.callback = _media_service_default_callback;
	msg.sync_sem = &return_notify;

	if (false == send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		mem_free(srv_param);
		return -EBUSY;
	}

	if (os_sem_take(&return_notify, OS_FOREVER)) {
		mem_free(srv_param);
		return -ETIME;
	}

	ret = srv_param->result;
	mem_free(srv_param);
	return ret;
}

/* use synchronized message to avoid mem_malloc for large parameter, whose psize is not zero */
int media_player_set_global_parameter(media_player_t *handle, int pname, void *param, unsigned int psize)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	media_srv_param_t *srv_param = NULL;
	int ret = 0;

	srv_param = mem_malloc(sizeof(media_srv_param_t));
	if (!srv_param)
		return -ENOMEM;

	srv_param->handle = handle ? handle->media_srv_handle : NULL;
	srv_param->param.type = pname;
	srv_param->param.pbuf = param;
	srv_param->param.plen = psize;
	srv_param->result = 0;

	msg.ptr = srv_param;
	msg.type = MSG_MEDIA_SRV_SET_GLOBAL_PARAMETER;

	if (psize > 0) {
		os_sem_init(&return_notify, 0, 1);
		msg.callback = _media_service_default_callback;
		msg.sync_sem = &return_notify;
	}

	if (false == send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		mem_free(srv_param);
		return -EBUSY;
	}

	if (psize > 0) {
		if (os_sem_take(&return_notify, OS_FOREVER)) {
			mem_free(srv_param);
			return -ETIME;
		}

		ret = srv_param->result;
		mem_free(srv_param);
	}

	return ret;
}

int media_player_close(media_player_t *handle)
{
	struct app_msg msg = {0};
	os_sem return_notify;

	os_sem_init(&return_notify, 0, 1);

	msg.type = MSG_MEDIA_SRV_CLOSE;
	msg.ptr = handle->media_srv_handle;
	msg.callback = _media_service_default_callback;
	msg.sync_sem = &return_notify;

	os_mutex_lock(&media_srv_mutex, OS_FOREVER);

	if (!send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		SYS_LOG_ERR("MSG_MEDIA_SRV_CLOSE send failed");
		goto error_exit;
	}

	if (os_sem_take(&return_notify, OS_FOREVER) != 0) {
		goto error_exit;
	}

	if (current_media_dumpable_player == handle)
		current_media_dumpable_player = NULL;
	if (current_media_main_player == handle)
		current_media_main_player = NULL;

	media_player_ref_cnt--;

error_exit:
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	SYS_LOG_INF("dvfs level %d\n", handle->dvfs_level);
	dvfs_unset_level(handle->dvfs_level, "media");
#endif
	os_mutex_unlock(&media_srv_mutex);

	_notify_player_lifecycle_changed(handle, PLAYER_EVENT_CLOSE, NULL, 0);
	mem_free(handle);
	return 0;
}

int media_player_dump_data(media_player_t *handle, int num, const u8_t tags[], struct acts_ringbuf *bufs[])
{
	struct app_msg msg = {0};
	os_sem return_notify;
	media_data_dump_info_t dump_info;
	media_srv_param_t *srv_param = NULL;

	if (!handle)
		handle = current_media_dumpable_player;

	if (!handle || !handle->media_srv_handle) {
		return -EINVAL;
	}

	srv_param = mem_malloc(sizeof(*srv_param));
	if (!srv_param)
		return -ENOMEM;

	os_sem_init(&return_notify, 0, 1);

	dump_info.num = num;
	dump_info.tags = tags;
	dump_info.bufs = bufs;

	srv_param->handle = handle->media_srv_handle;
	srv_param->param.type = 0;
	srv_param->param.pbuf = &dump_info;
	srv_param->param.plen = sizeof(dump_info);

	msg.ptr = srv_param;
	msg.type = MSG_MEDIA_SRV_DUMP_DATA;
	msg.callback = _media_service_default_callback;
	msg.sync_sem = &return_notify;

	if (false == send_async_msg(MEDIA_SERVICE_NAME, &msg)) {
		mem_free(srv_param);
		return -EBUSY;
	}

	if (os_sem_take(&return_notify, OS_FOREVER)) {
		mem_free(srv_param);
		return -ETIME;
	}

	mem_free(srv_param);
	return 0;
}

media_player_t *media_player_get_current_dumpable_player(void)
{
	return current_media_dumpable_player;
}

media_player_t *media_player_get_current_main_player(void)
{
	return current_media_main_player;
}

bool media_player_is_working(void)
{
	return (media_player_ref_cnt != 0);
}

int media_player_set_lifecycle_notifier(media_srv_event_notify_t notify)
{
	os_mutex_lock(&media_srv_mutex, OS_FOREVER);
	pfn_lifecycle_notify = notify;
	SYS_LOG_INF("%p", notify);
	os_mutex_unlock(&media_srv_mutex);
	return 0;
}

int media_player_switch_effect_param(media_player_t *handle, const u8_t *efx_name)
{
	int size = sizeof(aset_para_bin_t);

	const void *vaddr = media_effect_load(efx_name, &size, AUDIO_STREAM_MUSIC);

	SYS_LOG_INF("%s", efx_name);

	return media_player_update_effect_param(handle, (void *)vaddr, size);
}

/* FIXME: Should sync when encoder library's behaviour changed ? */
int media_player_repair_filhdr(media_player_t *handle, u8_t format, io_stream_t fstream)
{
	const u32_t header_size = 512;
	u32_t data_size;

	switch (format) {
	case WAV_TYPE:
		break;
	case MP3_TYPE: /* MP2 actually, no header */
	case OPUS_TYPE:
		return 0;
	default:
		return -EINVAL;
	}

	if (stream_seek(fstream, 0, SEEK_DIR_END))
		return -EIO;

	data_size = stream_tell(fstream);
	if (data_size < header_size)
		return -EINVAL;

	/* lower byte first */
	data_size = sys_cpu_to_le32(data_size - header_size);

	/* override last 4 bytes of header */
	if (stream_seek(fstream, header_size - 4,  SEEK_DIR_BEG))
		return -EIO;

	stream_write(fstream, (void *)&data_size, sizeof(data_size));
	return 0;
}

