/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arithmetic.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <media_player.h>
#include <media_effect_param.h>
#include <bluetooth/bt_manager.h>
#include <volume_manager.h>
#include <app_manager.h>
#include "app_defines.h"
#include "tool_app_inner.h"

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define ASET_PARA_BUF_SIZE     (sizeof(aset_para_bin_t))
#define ASET_STUB_RW_BUF_SIZE  (1024)

typedef struct ASET_data {
	const char *curr_app;

	aset_status_t aset_status;
	aset_volume_table_v2_t *volume_table;

	char *aset_para_buf;
	char *stub_rw_buf;
} ASET_data_t;

static void _aset_data_free(struct ASET_data *data);

static struct ASET_data *_aset_data_alloc()
{
	struct ASET_data *data = app_mem_malloc(sizeof(*data));
	if (!data)
		return NULL;

	memset(data, 0, sizeof(*data));

	data->volume_table = audio_policy_get_aset_volume_table();
	if (!data->volume_table)
		goto err_exit;

	data->aset_para_buf = app_mem_malloc(ASET_PARA_BUF_SIZE);
	if (!data->aset_para_buf)
		goto err_exit;

	data->stub_rw_buf = app_mem_malloc(ASET_STUB_RW_BUF_SIZE);
	if (!data->stub_rw_buf)
		goto err_exit;

	return data;
err_exit:
	_aset_data_free(data);
	return NULL;
}

static void _aset_data_free(struct ASET_data *data)
{
	if (data->aset_para_buf)
		app_mem_free(data->aset_para_buf);
	if (data->stub_rw_buf)
		app_mem_free(data->stub_rw_buf);

	app_mem_free(data);
}

static int _aset_stub_read(struct ASET_data *data, uint16_t opcode, void *buf, uint16_t size)
{
	stub_ext_param_t param = {
		.opcode = opcode,
		.payload_len = 0,
		.rw_buffer = data->stub_rw_buf,
	};
	int ret = 0;

	/* plus 2 bytes checksum */
	assert(size + sizeof(stub_ext_cmd_t) + 2 <= ASET_STUB_RW_BUF_SIZE);

	/* send read request */
	ret = stub_ext_write(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("read request failed");
		return ret;
	}

	/* receive data */
	param.payload_len = size;
	ret = stub_ext_read(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("read data failed");
		return ret;
	}

	memcpy(buf, &param.rw_buffer[sizeof(stub_ext_cmd_t)], size);

	/* send ack */
	param.opcode = STUB_CMD_ASET_ACK;
	param.payload_len = 0;
	ret = stub_ext_write(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("write ack failed");
		return ret;
	}

	return 0;
}

static int _aset_stub_write(struct ASET_data *data, uint16_t opcode, const void *buf, uint16_t size)
{
	stub_ext_param_t param = {
		.opcode = opcode,
		.payload_len = size,
		.rw_buffer = data->stub_rw_buf,
	};
	int ret = 0;

	/* plus 2 bytes checksum */
	assert(size + sizeof(stub_ext_cmd_t) + 2 <= ASET_STUB_RW_BUF_SIZE);

	memcpy(&param.rw_buffer[sizeof(stub_ext_cmd_t)], buf, size);

	/* send data */
	ret = stub_ext_write(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("write data failed");
		return ret;
	}

	/* receive ack (timeout has already added in stub driver) */
	param.payload_len = 0;
	ret = stub_ext_read(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("read ack failed");
		return ret;
	}

	if (sys_get_be16(&param.rw_buffer[1]) != STUB_CMD_ASET_ACK) {
		SYS_LOG_ERR("invalid ack 0x%02x", sys_get_be16(&param.rw_buffer[1]));
		return -EINVAL;
	}

	return 0;
}

static int _aset_read_status(struct ASET_data *data)
{
	return _aset_stub_read(data, STUB_CMD_ASET_READ_STATUS,
			       &data->aset_status, sizeof(data->aset_status));
}

static void _aset_config_pc_view(aset_interface_config_t *pCfg)
{
	const char *fw_version = "285A100";

	memset(pCfg, 0, sizeof(*pCfg));
	pCfg->bEQ_v_1_1 = 1;
	pCfg->bLimiter_v_1_0 = 1;
	pCfg->bMDRC_v_2_0 = 1;
	pCfg->bNR_v_1_0 = 1;
	pCfg->bVolumeTable_v_1_0 = 1;

	strncpy((char *)pCfg->szVerInfo, fw_version, sizeof(pCfg->szVerInfo));
}

static int _aset_write_case_info(struct ASET_data *data)
{
	aset_case_info_t *aset_case_info = app_mem_malloc(sizeof(aset_case_info_t));
	if (!aset_case_info)
		return -ENOMEM;

	memset(aset_case_info, 0, sizeof(aset_case_info_t));
	aset_case_info->peq_point_num = ASET_DAE_PEQ_POINT_NUM;
	aset_case_info->b_Max_Volume = audio_policy_get_volume_level();

	_aset_config_pc_view(&aset_case_info->stInterface);
	_aset_stub_write(data, STUB_CMD_ASET_WRITE_STATUS, aset_case_info, sizeof(aset_case_info_t));

	app_mem_free(aset_case_info);
	return 0;
}

static int _aset_upload_application_properties(struct ASET_data *data)
{
	aset_application_properties_t properties;

	memset(&properties, 0, sizeof(properties));

	if (strcmp(data->curr_app, APP_ID_LINEIN)) {
		properties.aux_mode = UNAUX_MODE;
	} else {
		properties.aux_mode = AUX_MODE;
	}

	return _aset_stub_write(data,
				STUB_CMD_ASET_WRITE_APPLICATION_PROPERTIES,
				&properties, sizeof(properties));
}

static int _aset_volume_deal(struct ASET_data *data)
{
	int32_t volume = 0;
	int ret;

	ret = _aset_stub_read(data, STUB_CMD_ASET_READ_VOLUME, &volume, sizeof(volume));
	if (!ret) {
		SYS_LOG_INF("volume %d", volume);

		if (!strcmp(data->curr_app, APP_ID_BTMUSIC))
			system_volume_set(AUDIO_STREAM_MUSIC, volume, false);
		else if (!strcmp(data->curr_app, APP_ID_LINEIN))
			system_volume_set(AUDIO_STREAM_LINEIN, volume, false);
		else
			system_volume_set(AUDIO_STREAM_DEFAULT, volume, false);
	}

	return ret;
}

static int _aset_main_switch_deal(struct ASET_data *data)
{
	int32_t main_switch = 0;
	int ret = 0;

	media_player_t *dump_player = media_player_get_current_dumpable_player();
	if (!dump_player)
		return -EFAULT;

	ret = _aset_stub_read(data, STUB_CMD_ASET_READ_MAIN_SWITCH, &main_switch, sizeof(main_switch));
	if (!ret)
		media_player_set_effect_enable(dump_player, main_switch ? true : false);

	return ret;
}

static int _aset_dae_deal(struct ASET_data *data)
{
	int ret;

	media_player_t *dump_player = media_player_get_current_dumpable_player();
	if (!dump_player)
		return -EFAULT;

	ret = _aset_stub_read(data, STUB_CMD_ASET_READ_DAE_PARAM_285A,
			      data->aset_para_buf, ASET_PARA_BUF_SIZE);
	if (!ret) {
		media_player_update_effect_param(
				dump_player, data->aset_para_buf, ASET_PARA_BUF_SIZE);

		/*
		_aset_stub_write(data, STUB_CMD_ASET_WRITE_DAE_PARAM,
				 data->aset_para_buf, ASET_PARA_BUF_SIZE);
		*/

		SYS_LOG_INF("dae updated");
	}

	return ret;
}

static int _aset_get_volume_table_status(struct ASET_data *data)
{
	int32_t status = -1;
	_aset_stub_read(data, STUB_CMD_ASET_READ_VOLUME_TABLE_STATUS, &status, 4);
	return status;
}

static int _aset_volume_table_deal(struct ASET_data *data)
{
	int32_t *pPATable = data->volume_table->nPAVal;
	int16_t *pDATable = data->volume_table->sDAVal;
	int max_volume = audio_policy_get_volume_level();
	int i;

	if (_aset_stub_read(data, STUB_CMD_ASET_READ_VOLUME_TABLE_DATA_V2,
			    data->volume_table, sizeof(aset_volume_table_v2_t))) {
		return -EIO;
	}

	printk("%s: table (bEnable=%d):\n", __func__, data->volume_table->bEnable);
	for (i = 0; i <= max_volume; i++) {
		printk("[%2d] PA:%6d, DA:%4d\n", i, pPATable[i], pDATable[i]);
	}

	_aset_volume_deal(data);
	return 0;
}

int _aset_cmd_deal(struct ASET_data *data)
{
	if (!data->aset_status.state)
		return 0;

	if (data->aset_status.upload_case_info)
		_aset_write_case_info(data);

	if (data->aset_status.volume_changed)
		_aset_volume_deal(data);

	if (data->aset_status.main_switch_changed)
		_aset_main_switch_deal(data);

	if (data->aset_status.dae_changed)
		_aset_dae_deal(data);

	if (_aset_get_volume_table_status(data) == 1)
		_aset_volume_table_deal(data);

	return 0;
}

int _aset_write_music_data(struct ASET_data *data, void *buf, uint16_t size)
{
	return _aset_stub_write(data, STUB_CMD_ASET_WRITE_MUSIC_DATA, buf, size);	
}

static int _aset_send_ack(struct ASET_data *data)
{
	stub_ext_param_t param = {
		.opcode = STUB_CMD_ASET_ACK,
		.rw_buffer = data->stub_rw_buf,
		.payload_len = 0,
	};

	return stub_ext_write(tool_stub_dev_get(), &param);
}

void tool_aset_loop(void)
{
	struct ASET_data *aset_data = NULL;
	const char *prev_app = NULL;
	media_player_t *prev_player = NULL;
	media_player_t *curr_player = NULL;
	bool reconnect_flag = true;

	SYS_LOG_INF("Enter");

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

	aset_data = _aset_data_alloc();
	if (!aset_data) {
		SYS_LOG_ERR("data_alloc failed");
		goto out_exit;
	}

	if (_aset_send_ack(aset_data)) {
		SYS_LOG_ERR("send_ack failed");
		goto out_exit;
	}

	do {
		os_sleep(10);

		/* polling status */
		if (_aset_read_status(aset_data)) {
			SYS_LOG_ERR("read_status failed");
			break;
		}

		aset_data->curr_app = app_manager_get_current_app();
		if (!strcmp(aset_data->curr_app, APP_ID_BTCALL)) {
			SYS_LOG_ERR("in btcall");
			break;
		}

		if (!prev_app || strcmp(prev_app, aset_data->curr_app)) {
			SYS_LOG_INF("current app is %s", aset_data->curr_app);

			if (!prev_app)
				_aset_write_case_info(aset_data);

			_aset_upload_application_properties(aset_data);

			prev_app = aset_data->curr_app;
			reconnect_flag = true;
		}

		curr_player = media_player_get_current_dumpable_player();
		if (!curr_player) {
			prev_player = NULL;
			continue;
		}

		if (curr_player != prev_player) {
			prev_player = curr_player;
			reconnect_flag = true;
		}

		if (reconnect_flag) {
			reconnect_flag = false;

			/* force update status */
			aset_data->aset_status.state = 1;
			aset_data->aset_status.main_switch_changed = 1;
			aset_data->aset_status.volume_changed = 1;
			aset_data->aset_status.dae_changed = 1;
		}

		_aset_cmd_deal(aset_data);
	} while (!tool_is_quitting());

out_exit:
	if (aset_data)
		_aset_data_free(aset_data);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

	SYS_LOG_INF("Exit");
}
