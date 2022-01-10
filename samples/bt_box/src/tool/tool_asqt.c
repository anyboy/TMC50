/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <audio_system.h>
#include <audio_policy.h>
#include <media_mem.h>
#include <media_player.h>
#include <media_effect_param.h>
#include <app_switch.h>
#include <app_manager.h>
#include <tts_manager.h>
#include <volume_manager.h>
#include <bluetooth/bt_manager.h>
#include <zero_stream.h>
#include "app_defines.h"
#include "tool_app_inner.h"

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define ASQT_DUMP_BUF_SIZE	(1024)
#define ASQT_MAX_DUMP		(3)
#define ASQT_STUB_BUF_SIZE	ROUND_UP(STUB_COMMAND_HEAD_SIZE + 1 + (512 + 1) * ASQT_MAX_DUMP, 2)
#define ASQT_BACKSTORE_BUF_SIZE	(ASQT_STUB_BUF_SIZE + ASQT_DUMP_BUF_SIZE * ASQT_MAX_DUMP)

#define CAPTURE_START_INDEX (2)

//static char asqt_stub_buff[STUB_COMMAND_HEAD_SIZE + 2 + (512 + 2) * 6];
//static char asqt_buf_backstore[6][ASQT_DUMP_BUF_SIZE];
static const uint8_t asqt_tag_table[6] = {
	/* playback tags */
	MEDIA_DATA_TAG_DECODE_OUT1, MEDIA_DATA_TAG_PLC,
	/* capture tags */
	MEDIA_DATA_TAG_DAC1, MEDIA_DATA_TAG_MIC1,
	MEDIA_DATA_TAG_AEC1, MEDIA_DATA_TAG_ENCODE_IN1,
};

/* ASQT upload format*/
/* ------------------------------------------------------------------------------
 * | 1 bytes  | 1 bytes |  size   | 1 bytes |  size   | ... | 1 bytes |  size   |
 * ------------------------------------------------------------------------------
 * | num_tags |  tag_0  | data_0  |  tag_1  | data_1  | ... |  tag_n  | data_n  |
 * ------------------------------------------------------------------------------
 * data size is computed by formula:
 * 		size = (payload - 1 / num_tags) - 1
 * in which,
 * 		payload is the total size of the upload package.
 */

struct ASQT_data {
	uint8_t has_effect_set : 1;
	uint8_t in_simulation : 1;

	uint8_t in_btcall : 1;
	uint8_t last_in_btcall : 1;

	uint8_t run_status : 1;
	uint8_t last_run_status : 1;

	uint8_t upload_status : 1;
	uint8_t last_upload_status : 1;

	uint8_t codec_id;

	/* asqt para */
	asqt_para_bin_t *asqt_para;

	/* asqt dump tag select */
	asqt_ST_ModuleSel modesel;
	struct acts_ringbuf *dump_bufs[ARRAY_SIZE(asqt_tag_table)];

	/* backstore memory for dump_bufs */
	char *dump_backstore;
	/* stub rw buffer */
	char *stub_buf;

	/* for simulation */
	io_stream_t zero_stream;
};

static void _asqt_unprepare_data_upload(struct ASQT_data *data);
static int _asqt_unprepare_simulation_if_required(struct ASQT_data *data);

static struct ASQT_data *_asqt_data_alloc(void)
{
	struct ASQT_data *data = app_mem_malloc(sizeof(*data));
	if (!data)
		return NULL;

	memset(data, 0, sizeof(*data));

	if (media_mem_get_cache_pool_size(TOOL_ASQT_STUB_BUF, AUDIO_STREAM_VOICE) < ASQT_STUB_BUF_SIZE) {
		SYS_LOG_ERR("TOOL_ASQT_STUB_BUF too small");
		app_mem_free(data);
		return NULL;
	}

	if (media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_VOICE) < ASQT_DUMP_BUF_SIZE * ASQT_MAX_DUMP) {
		SYS_LOG_ERR("TOOL_ASQT_DUMP_BUF too small");
		app_mem_free(data);
		return NULL;
	}

	data->stub_buf = media_mem_get_cache_pool(TOOL_ASQT_STUB_BUF, AUDIO_STREAM_VOICE);
	data->dump_backstore = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_VOICE);

	data->asqt_para = app_mem_malloc(sizeof(asqt_para_bin_t));
	if (!data->asqt_para) {
		app_mem_free(data->asqt_para);
		return NULL;
	}

	return data;
}

static void _asqt_data_free(struct ASQT_data *data)
{
	/* cancel the effect, since memory will free */
	if (data->has_effect_set)
		medie_effect_set_user_param(AUDIO_STREAM_VOICE, 0, NULL, 0);

	_asqt_unprepare_data_upload(data);
	_asqt_unprepare_simulation_if_required(data);

	app_mem_free(data->asqt_para);
	app_mem_free(data);
}

static int _asqt_stub_read_data(int opcode, void *buf, unsigned int size)
{
	return stub_get_data(tool_stub_dev_get(), opcode, buf, size);
}

static int _asqt_stub_write_data(int opcode, void *buf, unsigned int size)
{
	int ret = 0;
	int err_cnt = 0;

retry:
	ret = stub_set_data(tool_stub_dev_get(), opcode, buf, size);
	if (ret) {
		if (++err_cnt < 10)
			goto retry;
	}

	return ret;
}

static int _asqt_stub_upload_buffer(void *buf, unsigned int size)
{
	return stub_set_data(tool_stub_dev_get(),
				(void *)STUB_CMD_ASQT_UPLOAD_DATA_NEW, buf, size);
}

static void _asqt_stub_upload_end(void)
{
	stub_set_data(tool_stub_dev_get(), STUB_CMD_ASQT_UPLOAD_DATA_OVER, NULL, 0);
}

static int _asqt_stub_upload_case_info(struct ASQT_data *data)
{
	asqt_interface_config_t *p_config_info =
			(asqt_interface_config_t *)(data->stub_buf + STUB_COMMAND_HEAD_SIZE);
	const char *fw_version = "285A100";

	SYS_LOG_INF("codec type = %d", data->codec_id);

	memset(p_config_info, 0, sizeof(asqt_interface_config_t));
	strncpy((char *)p_config_info->fw_version, fw_version, sizeof(p_config_info->fw_version));
	p_config_info->eq_point_nums = ASQT_DAE_PEQ_POINT_NUM;
	p_config_info->eq_speaker_nums = ASQT_DAE_PEQ_POINT_NUM / 2;
	p_config_info->eq_version = 1;
	p_config_info->sample_rate = (data->codec_id == CVSD_TYPE) ? 0 : 1;

	_asqt_stub_write_data(STUB_CMD_ASQT_WRITE_CONFIG_INFO, data->stub_buf, sizeof(asqt_interface_config_t));
	return 0;
}

static int _asqt_stub_get_status(void)
{
	int32_t status = -1;
	_asqt_stub_read_data(STUB_CMD_ASQT_READ_PC_TOOL_STATUS, &status, 4);
	return status;
}

static int _asqt_stub_get_effect_param_status(void)
{
	int32_t status = -1;
	_asqt_stub_read_data(STUB_CMD_ASQT_GET_EFFECT_PARAM_STATUS, &status, 4);
	return status;
}

// fetch effect param from ASQT, and pass it to media player
static int _asqt_stub_effect_param_deal(struct ASQT_data *data)
{
	int ret = _asqt_stub_read_data(STUB_CMD_ASQT_GET_EFFECT_PARAM, data->asqt_para, sizeof(asqt_para_bin_t));

	if (!ret) {
		media_player_t *player;

		/* override the tail length with audio policy. */
		data->asqt_para->sv_para.aec_prms.tail_length =
				audio_policy_get_record_channel_aec_tail_length(AUDIO_STREAM_VOICE,
						data->codec_id == CVSD_TYPE ? 8 : 16, true);

		player = media_player_get_current_dumpable_player();
		if (data->in_btcall && player)
			media_player_update_effect_param(player, data->asqt_para, sizeof(asqt_para_bin_t));

		if (!data->has_effect_set) {
			medie_effect_set_user_param(AUDIO_STREAM_VOICE, 0, data->asqt_para, sizeof(asqt_para_bin_t));
			data->has_effect_set = 1;
		}

		SYS_LOG_INF("effect updated");
	}

	return ret;
}

static int _asqt_stub_get_case_param_status(void)
{
	int32_t status = -1;
	_asqt_stub_read_data(STUB_CMD_ASQT_GET_PC_CASE_PARAM_STATUS, &status, 4);
	return status;
}

// fetch case param from ASQT, and pass it to media player
static int _asqt_stub_case_param_deal(struct ASQT_data *data)
{
	asqt_stControlParam case_param;
	media_player_t *player;
	int ret = 0;

	player = media_player_get_current_dumpable_player();
	if (!player)
		return -EFAULT;

	ret = _asqt_stub_read_data(STUB_CMD_ASQT_GET_CASE_PARAM_285A, &case_param, sizeof(case_param));
	if (!ret) {
		SYS_LOG_INF("sMaxVolume %d, sVolume %d, sAnalogGain %d, sDigitalGain %d",
				case_param.sMaxVolume, case_param.sVolume, case_param.sAnalogGain, case_param.sDigitalGain);

		system_volume_set(AUDIO_STREAM_VOICE, case_param.sVolume, false);
		audio_system_set_stream_pa_volume(AUDIO_STREAM_VOICE, case_param.sMaxVolume * 100);
		audio_system_set_microphone_volume(AUDIO_STREAM_VOICE, case_param.sAnalogGain + case_param.sDigitalGain);
	}

	return ret;
}

static int _asqt_cmd_deal(struct ASQT_data *data)
{
	if (data->in_btcall && _asqt_stub_get_case_param_status() == 1)
		_asqt_stub_case_param_deal(data);

	if (_asqt_stub_get_effect_param_status() == 1)
		_asqt_stub_effect_param_deal(data);

	return 0;
}

static int _asqt_get_modsel(struct ASQT_data *data, asqt_ST_ModuleSel *modsel)
{
	return _asqt_stub_read_data(STUB_CMD_ASQT_GET_MODESEL, modsel, sizeof(*modsel));
}

static int _asqt_prepare_data_upload(struct ASQT_data *data)
{
	char *dumpbuf = data->dump_backstore;
	int num_tags = 0;

	media_player_t *player = media_player_get_current_dumpable_player();
	if (!player)
		return -EFAULT;

	if (_asqt_get_modsel(data, &data->modesel)) {
		SYS_LOG_ERR("get_modsel failed");
		return -EIO;
	}

	for (int i = 0; i < ARRAY_SIZE(asqt_tag_table); i++) {
		if (!data->modesel.bSelArray[i]) /* selected or not */
			continue;

		if (num_tags++ >= ASQT_MAX_DUMP) {
			SYS_LOG_WRN("exceed max %d tags", ASQT_MAX_DUMP);
			break;
		}

		/* update to data tag */
		data->dump_bufs[i] = acts_ringbuf_init_ext(dumpbuf, ASQT_DUMP_BUF_SIZE);
		if (!data->dump_bufs[i])
			goto fail;

		dumpbuf += ASQT_DUMP_BUF_SIZE;
		SYS_LOG_INF("start idx=%d, tag=%u, buf=%p", i, asqt_tag_table[i], data->dump_bufs[i]);
	}

	return media_player_dump_data(player, ARRAY_SIZE(asqt_tag_table), asqt_tag_table, data->dump_bufs);
fail:
	SYS_LOG_ERR("failed");
	_asqt_unprepare_data_upload(data);
	return -ENOMEM;
}

static void _asqt_unprepare_data_upload(struct ASQT_data *data)
{
	for (int i = 0; i < ARRAY_SIZE(data->dump_bufs); i++) {
		if (data->dump_bufs[i]) {
			SYS_LOG_INF("stop idx=%d, tag=%u, buf=%p", i, asqt_tag_table[i], data->dump_bufs[i]);
			media_player_dump_data(NULL, 1, &asqt_tag_table[i], NULL);
			acts_ringbuf_destroy_ext(data->dump_bufs[i]);
			data->dump_bufs[i] = NULL;
		}
	}
}

static int _asqt_upload_data_range(struct ASQT_data *data, int start, int end, int data_size)
{
	uint8_t *data_head = data->stub_buf + STUB_COMMAND_HEAD_SIZE;
	uint8_t *data_buff = data_head;
	uint8_t num_tags = 0, i;

	for (i = start; i < end; i++) {
		if (!data->dump_bufs[i])
			continue;

		if (acts_ringbuf_length(data->dump_bufs[i]) < data_size)
			return 0;

		num_tags++;
	}

	if (num_tags == 0)
		return 0;

	/* fill number of tags */
	*data_buff++ = num_tags;

	for (i = start; i < end; i++) {
		if (!data->dump_bufs[i])
			continue;

		/* fill tag */
		*data_buff++ = i;
		/* fill data */
		acts_ringbuf_get(data->dump_bufs[i], data_buff, data_size);
		data_buff += data_size;
	}

	return _asqt_stub_upload_buffer(data->stub_buf, (unsigned int)(data_buff - data_head));
}

static int _asqt_upload_data(struct ASQT_data *data)
{
	/* cvsd: 120 bytes per decoding output frame; msbc: 240 bytes per decoding output frame */
	if (data->codec_id == CVSD_TYPE) {
		return _asqt_upload_data_range(data, 0, CAPTURE_START_INDEX, 240) +
				_asqt_upload_data_range(data, CAPTURE_START_INDEX, ARRAY_SIZE(data->dump_bufs), 256);
	} else {
		return _asqt_upload_data_range(data, 0, CAPTURE_START_INDEX, 480) +
				_asqt_upload_data_range(data, CAPTURE_START_INDEX, ARRAY_SIZE(data->dump_bufs), 512);
	}
}

static int _asqt_prepare_simulation_if_required(struct ASQT_data *data)
{
	uint8_t hfp_freq = 0;
	int ret = 0;

	SYS_LOG_INF("analysis_mode = %d", data->asqt_para->sv_para.analysis_mode);

	data->in_simulation = (data->asqt_para->sv_para.analysis_mode == 0);
	if (!data->in_simulation)
		return 0;

	ret = _asqt_stub_read_data(STUB_CMD_ASQT_GET_HFP_FREQ, &hfp_freq, sizeof(hfp_freq));
	if (ret) {
		SYS_LOG_ERR("fail to get hfp freq");
		goto err_out;
	}

	data->zero_stream = zero_stream_create();
	if (!data->zero_stream) {
		ret = -ENOMEM;
		goto err_out;
	}

	if (stream_open(data->zero_stream, MODE_OUT)) {
		stream_destroy(data->zero_stream);
		data->zero_stream = NULL;
		ret = -EIO;
		goto err_out;
	}

	app_switch(APP_ID_BTCALL, APP_SWITCH_CURR, true);
	app_switch_lock(1);

	data->codec_id = (hfp_freq == 0) ? CVSD_TYPE : MSBC_TYPE;

	/* FIXME: worakaround for simulation mode performance.
	 *
	 * PC tool requires reporting the codec_id even in simulaton mode, and after reporting,
	 * the PC tool will consider simply the effect param changed agin. So sync params here
	 * in advance to avoid side effect of the poor usb performace on the simulation mode .
	 */
	_asqt_stub_upload_case_info(data);
	_asqt_cmd_deal(data);

	bt_manager_sco_set_codec_info(data->codec_id, (hfp_freq == 0) ? 8 : 16);

	/* zero_stream will be opened, closed and destroyed by APP_ID_BTCALL */
	bt_manager_event_notify(BT_HFP_ESCO_ESTABLISHED_EVENT, &data->zero_stream, sizeof(data->zero_stream));

	return 0;
err_out:
	data->in_simulation = 0;
	return ret;
}

static int _asqt_unprepare_simulation_if_required(struct ASQT_data *data)
{
	if (!data->in_simulation)
		return 0;

	bt_manager_event_notify(BT_HFP_ESCO_RELEASED_EVENT, NULL, 0);

	app_switch_unlock(1);
	app_switch(NULL, APP_SWITCH_LAST, false);

	if (data->zero_stream) {
		stream_close(data->zero_stream);
		stream_destroy(data->zero_stream);
		data->zero_stream = NULL;
	}

	data->in_simulation = 0;
	return 0;
}

static int _asqt_run_simulation_if_required(struct ASQT_data *data)
{
	/* cvsd: 62 bytes per encoding frame; msbc: 60 bytes per encoding frame */
	const int min_data_size = (data->codec_id == CVSD_TYPE) ? (62 * 4) : (60 * 4);
	int data_size, data_space;
	io_stream_t bt_stream;

	if (!data->in_simulation)
		return 0;

	bt_manager_stream_pool_lock();

	bt_stream = bt_manager_get_stream(STREAM_TYPE_SCO);
	if (!bt_stream)
		goto out_unlock;

	data_space = stream_get_space(bt_stream);
	if (data_space < min_data_size)
		goto out_unlock;

	data_size = min(data_space, ASQT_STUB_BUF_SIZE);

	_asqt_stub_read_data(STUB_CMD_ASQT_DOWN_REMOTE_PHONE_DATA, data->stub_buf, data_size);

	stream_write(bt_stream, data->stub_buf, data_size);

out_unlock:
	bt_manager_stream_pool_unlock();
	return 0;
}

static int _btcall_is_ongoing(void)
{
	if (strcmp(APP_ID_BTCALL, app_manager_get_current_app()))
		return 0;

	if (!media_player_get_current_dumpable_player())
		return 0;

	return 1;
}

void tool_asqt_loop(void)
{
	struct ASQT_data *asqt_data = NULL;

	SYS_LOG_INF("Enter");

#ifdef CONFIG_PLAYTTS
	tts_manager_lock();
#endif

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

	asqt_data = _asqt_data_alloc();
	if (!asqt_data) {
		SYS_LOG_ERR("_asqt_data_alloc failed");
		goto out_exit;
	}

	/* FIXME: update the PC view, with an arbitrary codec_id. */
	asqt_data->codec_id = CVSD_TYPE;
	_asqt_stub_upload_case_info(asqt_data);

	do {
		/* get tool status */
		int status = _asqt_stub_get_status();
		if (status < 0)
			break;

		switch (status) {
		case sUserStart:
			asqt_data->run_status = 1;
			break;
		case sUserStop:
			asqt_data->run_status = 0;
			break;
		case sUserUpdate:
		default:
			break;
		}

		/* get btcall status */
		asqt_data->in_btcall = _btcall_is_ongoing();
		if (asqt_data->in_btcall != asqt_data->last_in_btcall) {
			if (asqt_data->in_btcall) {
				SYS_LOG_INF("in btcall");
				asqt_data->codec_id = (uint8_t)bt_manager_sco_get_codecid();
				if (!asqt_data->in_simulation)
					_asqt_stub_upload_case_info(asqt_data);
			}

			asqt_data->last_in_btcall = asqt_data->in_btcall;
		}

		/* sync simulation mode and dynamic params */
		_asqt_cmd_deal(asqt_data);

		/* simulation */
		if (asqt_data->run_status != asqt_data->last_run_status) {
			SYS_LOG_INF("tool status = %d", asqt_data->run_status);
			if (asqt_data->run_status) {
				_asqt_prepare_simulation_if_required(asqt_data);
			} else {
				_asqt_unprepare_simulation_if_required(asqt_data);
			}

			asqt_data->last_run_status = asqt_data->run_status;
		}

		if (asqt_data->run_status)
			_asqt_run_simulation_if_required(asqt_data);

		/* upload */
		asqt_data->upload_status = asqt_data->run_status && asqt_data->in_btcall;
		if (asqt_data->upload_status != asqt_data->last_upload_status) {
			SYS_LOG_INF("upload status = %d", asqt_data->upload_status);
			if (asqt_data->upload_status) {
				_asqt_prepare_data_upload(asqt_data);
			} else {
				_asqt_unprepare_data_upload(asqt_data);
				_asqt_stub_upload_end();
			}

			asqt_data->last_upload_status = asqt_data->upload_status;
		}

		if (asqt_data->upload_status)
			_asqt_upload_data(asqt_data);

		if (!asqt_data->run_status)
			os_sleep(4);
	} while (!tool_is_quitting());

out_exit:
	if (asqt_data)
		_asqt_data_free(asqt_data);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

#ifdef CONFIG_PLAYTTS
	tts_manager_unlock();
#endif

	SYS_LOG_INF("Exit");
}
