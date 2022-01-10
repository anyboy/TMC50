/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system audio policy
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <system_app.h>
#include <string.h>
#include <audio_system.h>
#include <audio_policy.h>

#ifdef CONFIG_BT_TWS_US281B
#define MAX_AUDIO_VOL_LEVEL 32
#else
#define MAX_AUDIO_VOL_LEVEL 32
#endif

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#ifdef CONFIG_MEDIA
#if (MAX_AUDIO_VOL_LEVEL == 32)

/* unit: 0.1 dB */
const short voice_da_table[MAX_AUDIO_VOL_LEVEL] = {
	-1000, -440, -420, -400, -380, -360, -340, -320,
	 -300, -280, -260, -240, -225, -210, -195, -180,
	 -165, -150, -135, -120, -110, -100,  -90,  -80,
	  -70,  -60,  -50,  -40,  -30,  -20,  -10,    0,
};

/* unit: 0.001 dB */
const int voice_pa_table[MAX_AUDIO_VOL_LEVEL] = {
	-5000, -5000, -5000, -5000, -5000, -5000, -5000, -5000,
	-5000, -5000, -5000, -5000, -5000, -5000, -5000, -5000,
	-5000, -5000, -5000, -5000, -5000, -5000, -5000, -5000,
	-5000, -5000, -5000, -5000, -5000, -5000, -5000, -5000
};

/* unit: 0.1 dB */
const short music_da_table[MAX_AUDIO_VOL_LEVEL] = {
	-80, -80, -80, -80, -80, -80, -80, -80,
	-80, -80, -80, -80, -80, -80, -80, -80,
	-80, -80, -80, -72, -66, -60, -54, -48,
	-42, -36, -30, -24, -18, -12, -6, 0
};

/* unit: 0.001 dB */
const int music_pa_table[MAX_AUDIO_VOL_LEVEL] = {
	-60000, -52125, -46125, -40875, -36000, -31125, -28125, -25125,
	-22125, -19875, -18000, -16125, -14625, -13125, -11625, -10125,
	-8625, -7125, -5625, -4875, -4500, -4125, -3750, -3375,
	-2625, -2250, -1875, -1500, -1125, -750, -375, 0
};
#else

/* unit: 0.1 dB */
const short voice_da_table[MAX_AUDIO_VOL_LEVEL] = {
	-440, -400, -360, -320, -280, -240, -210, -180,
	-150, -120, -100,  -80,  -60,  -40,  -20,    0,
};

/* unit: 0.001 dB */
const int voice_pa_table[MAX_AUDIO_VOL_LEVEL] = {
	-5000, -5000, -5000, -5000,
	-5000, -5000, -5000, -5000,
	-5000, -5000, -5000, -5000,
	-5000, -5000, -5000, -5000,
};

/* unit: 0.1 dB */
const short music_da_table[MAX_AUDIO_VOL_LEVEL] = {
	-80, -80, -80, -72, -66, -60, -54, -48,
	-42, -36, -30, -24, -18, -12, -6, 0
};

/* unit: 0.001 dB */
const int music_pa_table[MAX_AUDIO_VOL_LEVEL] = {
	-100000, -49000, -39000, -33000, -28000, -24000, -21000, -18000,
	-15000, -12000, -10000, -8000,  -6000, -4000, -2000, 0,
};

#endif


const short usound_da_table[16] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

const int usound_pa_table[16] = {
       -59000, -44000, -38000, -33900,
       -29000, -24000, -22000, -19400,
       -17000, -14700, -12300, -10000,
       -7400,  -5000,  -3000,  0,
};

#ifdef CONFIG_TOOL_ASET
static aset_volume_table_v2_t aset_volume_table;
#endif

#ifdef CONFIG_TOOL_ASQT
static asqt_ext_info_t asqt_ext_info;
#endif


static const struct audio_policy_t system_audio_policy = {
#ifdef CONFIG_AUDIO_OUTPUT_I2S
	.audio_out_channel = AUDIO_CHANNEL_I2STX,
#elif CONFIG_AUDIO_OUTPUT_SPDIF
	.audio_out_channel = AUDIO_CHANNEL_SPDIFTX,
#elif CONFIG_AUDIO_OUTPUT_DAC_AND_I2S
	.audio_out_channel = AUDIO_CHANNEL_DAC | AUDIO_CHANNEL_I2STX,
#else
	.audio_out_channel = AUDIO_CHANNEL_DAC,
#endif

	.audio_in_linein_gain = 0, // 0db
	.audio_in_fm_gain = 0, // 0db
	.audio_in_mic_gain = 365, // 36.5db

#ifdef CONFIG_AEC_TAIL_LENGTH
	.voice_aec_tail_length_16k = CONFIG_AEC_TAIL_LENGTH,
	.voice_aec_tail_length_8k = CONFIG_AEC_TAIL_LENGTH * 2,
#endif

	.tts_fixed_volume = 1,
	.volume_saveto_nvram = 1,

	.audio_out_volume_level = MAX_AUDIO_VOL_LEVEL - 1,
	.music_da_table = music_da_table,
	.music_pa_table = music_pa_table,
	.voice_da_table = voice_da_table,
	.voice_pa_table = voice_pa_table,
	.usound_da_table = usound_da_table,
	.usound_pa_table = usound_pa_table,

#ifdef CONFIG_TOOL_ASET
	.aset_volume_table = &aset_volume_table,
#endif
#ifdef CONFIG_TOOL_ASQT
	.asqt_ext_info = &asqt_ext_info,
#endif
};

#endif /* CONFIG_MEDIA */

void system_audio_policy_init(void)
{
#ifdef CONFIG_MEDIA
	audio_policy_register(&system_audio_policy);

#ifdef CONFIG_OUTPUT_RESAMPLE
	if ((system_audio_policy.audio_out_channel & AUDIO_CHANNEL_I2STX) == AUDIO_CHANNEL_I2STX
	|| (system_audio_policy.audio_out_channel & AUDIO_CHANNEL_SPDIFTX) == AUDIO_CHANNEL_SPDIFTX) {
		audio_system_set_output_sample_rate(48);
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "media");
		SYS_LOG_INF("dvfs level %d\n", DVFS_LEVEL_HIGH_PERFORMANCE);
#endif
	}
#endif
#endif

}
