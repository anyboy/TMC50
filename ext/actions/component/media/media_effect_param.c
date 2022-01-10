/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SYS_LOG_DOMAIN "media"
#include <stdio.h>
#include <string.h>
#include <sdfs.h>
#include <audio_system.h>
#include <media_effect_param.h>
#include <audio_policy.h>

#ifdef CONFIG_BT_SERVICE
#include "btservice_api.h"
extern int btif_tws_get_dev_role(void);
#endif

#define BTCALL_EFX_PATTERN			"call.efx"
#define LINEIN_EFX_PATTERN			"linein.efx"
#define MUSIC_DRC_EFX_PATTERN		"musicdrc.efx"
#define MUSIC_NO_DRC_EFX_PATTERN	"music.efx"
#define MUSIC_EXT_VOLUME_TABLE_IMAGE 	"VER:"
#define MUSIC_EXT_VOLUME_TABLE_VERSION	"1000"
static const void *btcall_effect_user_addr;
static const void *linein_effect_user_addr;
static const void *music_effect_user_addr;

int medie_effect_set_user_param(u8_t stream_type, u8_t effect_type, const void *vaddr, int size)
{
	const void **user_addr = NULL;
	int expected_size = 0;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		user_addr = &btcall_effect_user_addr;
		expected_size = sizeof(asqt_para_bin_t);
		break;
	case AUDIO_STREAM_LINEIN:
		user_addr = &linein_effect_user_addr;
		expected_size = sizeof(aset_para_bin_t);
		break;
	default:
		user_addr = &music_effect_user_addr;
		expected_size = sizeof(aset_para_bin_t);
		break;
	}

	if (!user_addr)
		return -EINVAL;

	if (vaddr && (expected_size > 0 && expected_size != size))
		return -EINVAL;

	*user_addr = vaddr;
	return 0;
}

static int media_effect_check_and_load_volume_table(void *param_addr, int param_size, int stream_type)
{
	if (stream_type == AUDIO_STREAM_VOICE) {
		asqt_ext_info_t *asqt_ext_info = audio_policy_get_asqt_ext_info();
		if (param_size != sizeof(asqt_para_bin_t)) {
			SYS_LOG_ERR("SIZE ERROR");
			return -EINVAL;
		}
		if (asqt_ext_info)
			asqt_ext_info->asqt_para_bin = (asqt_para_bin_t *)param_addr;
	} else {
		aset_volume_table_v2_t *volume_table = audio_policy_get_aset_volume_table();
		if (param_size == sizeof(aset_para_bin_t)) {
			SYS_LOG_INF("old version");
			return 0;
		}
		if (param_size > sizeof(aset_para_bin_t) + sizeof(effect_volume_table_info_t)) {
			effect_volume_table_info_t *volume_table_info =
					 (effect_volume_table_info_t *)((u8_t *)param_addr + sizeof(aset_para_bin_t));

			if (memcmp((const char *)&volume_table_info->magic_data, MUSIC_EXT_VOLUME_TABLE_IMAGE, strlen(MUSIC_EXT_VOLUME_TABLE_IMAGE))) {
				SYS_LOG_ERR("IMAG ERROR");
				return -EINVAL;
			}

			if (!memcmp((const char *)&volume_table_info->version, MUSIC_EXT_VOLUME_TABLE_VERSION ,strlen(MUSIC_EXT_VOLUME_TABLE_VERSION))) {
				u8_t *table_addr = (u8_t *)volume_table_info + sizeof(effect_volume_table_info_t);
				memcpy(volume_table->nPAVal, table_addr, volume_table_info->volume_table_level * sizeof(int));

				table_addr += volume_table_info->volume_table_level * sizeof(int);
				memcpy(volume_table->sDAVal, table_addr, volume_table_info->volume_table_level * sizeof(short));
				volume_table->bEnable = true;
			}
		}
	}
	return 0;
}

const void *media_effect_load(const char *efx_pattern, int *expected_size, u8_t stream_type)
{
	char efx_name[16];
	void *vaddr = NULL;
	int size = 0;

	if (strchr(efx_pattern, '%')) {
		int volume = audio_system_get_stream_volume(stream_type);
		for (; volume < 32; volume++) {
			snprintf(efx_name, sizeof(efx_name), efx_pattern, volume);
			if (!sd_fmap(efx_name, &vaddr, &size))
				break;
		}
	} else {
		strncpy(efx_name, efx_pattern, sizeof(efx_name));
		sd_fmap(efx_name, &vaddr, &size);
	}

	if (!vaddr || !size) {
		SYS_LOG_ERR("not found");
		return NULL;
	}

	if(media_effect_check_and_load_volume_table(vaddr, size, stream_type)) {
			SYS_LOG_ERR("format error");
			return NULL;
	}
	*expected_size = size;
	SYS_LOG_INF("%s", efx_name);
	return vaddr;
}

const void *media_effect_get_param(u8_t stream_type, u8_t effect_type, int *effect_size)
{
	const void *user_addr = NULL;
	const char *efx_pattern = NULL;
	int expected_size = 0;

#if (defined (CONFIG_BT_SERVICE) && defined (CONFIG_OUTPUT_RESAMPLE))
	bool is_support_tws_res = btif_tws_get_dev_role() == BTSRV_TWS_NONE? 0 : 1;
#else
	bool is_support_tws_res = 0;
#endif

	switch (stream_type) {
	case AUDIO_STREAM_TTS: /* not use effect file */
		return NULL;
	case AUDIO_STREAM_VOICE:
		user_addr = btcall_effect_user_addr;
		efx_pattern = BTCALL_EFX_PATTERN;
		expected_size = sizeof(asqt_para_bin_t);
		break;
	case AUDIO_STREAM_LINEIN:
		user_addr = linein_effect_user_addr;
		efx_pattern = LINEIN_EFX_PATTERN;
		expected_size = sizeof(aset_para_bin_t);
		break;
	case AUDIO_STREAM_TWS:
		user_addr = music_effect_user_addr;
		efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
		expected_size = sizeof(aset_para_bin_t);
		break;
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_I2SRX_IN:
		user_addr = music_effect_user_addr;
		if (is_support_tws_res) {
			efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
		} else {
			#ifdef CONFIG_MUSIC_DRC_EFFECT
			efx_pattern = MUSIC_DRC_EFX_PATTERN;
			#else
			efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
			#endif
		}
		expected_size = sizeof(aset_para_bin_t);
		break;
	default:
		user_addr = music_effect_user_addr;
	#ifdef CONFIG_MUSIC_DRC_EFFECT
		efx_pattern = MUSIC_DRC_EFX_PATTERN;
	#else
		efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
	#endif
		expected_size = sizeof(aset_para_bin_t);
		break;
	}

	if (!user_addr)
		user_addr = media_effect_load(efx_pattern, &expected_size, stream_type);

	if (user_addr && effect_size)
		*effect_size = expected_size;

	return user_addr;
}
