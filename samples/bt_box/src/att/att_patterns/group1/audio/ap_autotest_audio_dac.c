/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_audio_dac.c
*/

#include "ap_autotest_audio.h"

#define AUDIO_DAC_FILL_MS  8

static short *dac_out_buffer;

void *dac_out_handle;

#if (MY_SAMPLE_RATE == 48)
static const short sin_1khz_1ch[48] =
{
    0x0000, 0x0860, 0x109B, 0x188C,
    0x2014, 0x270D, 0x2D5C, 0x32E5,
    0x378E, 0x3B45, 0x3DF7, 0x3F9A,
    0x4026, 0x3F9A, 0x3DF7, 0x3B44,
    0x378E, 0x32E4, 0x2D5D, 0x270D,
    0x2014, 0x188D, 0x109B, 0x085F,
    0x0000, 0xF7A0, 0xEF66, 0xE774,
    0xDFED, 0xD8F3, 0xD2A4, 0xCD1B,
    0xC871, 0xC4BB, 0xC209, 0xC066,
    0xBFD9, 0xC066, 0xC209, 0xC4BB,
    0xC873, 0xCD1B, 0xD2A3, 0xD8F2,
    0xDFEC, 0xE773, 0xEF65, 0xF7A1,
};
static const short sin_2khz_1ch[24*2] =
{
    0x0000, 0x109A, 0x2014, 0x2D5D,
	0x378E, 0x3DF7, 0x4026, 0x3DF7,
    0x378E, 0x2D5C, 0x2014, 0x109B,
	0x0000, 0xEF65, 0xDFED, 0xD2A4,
	0xC872, 0xC209, 0xBFDA, 0xC209,
	0xC872, 0xD2A4, 0xDFED, 0xEF65,

    0x0000, 0x109A, 0x2014, 0x2D5D,
	0x378E, 0x3DF7, 0x4026, 0x3DF7,
    0x378E, 0x2D5C, 0x2014, 0x109B,
	0x0000, 0xEF65, 0xDFED, 0xD2A4,
	0xC872, 0xC209, 0xBFDA, 0xC209,
	0xC872, 0xD2A4, 0xDFED, 0xEF65,
};
#else
static const short sin_1khz_1ch[16] =
{
    0x0000, 0x188D, 0x2D5C, 0x3B45,
	0x4027, 0x3B44, 0x2D5C, 0x188D,
    0x0000, 0xE773, 0xD2A3, 0xC4BB,
	0xBFDA, 0xC4BC, 0xD2A4, 0xE773,
};
static const short sin_2khz_1ch[8*2] =
{
    0x0000, 0x2D5C, 0x4026, 0x2D5C,
    0x0000, 0xD2A4, 0xBFD9, 0xD2A3,

    0x0000, 0x2D5C, 0x4026, 0x2D5C,
    0x0000, 0xD2A4, 0xBFD9, 0xD2A3,
};
#endif

void init_dac_buffer(short *p_dac_buffer)
{
    int i, j;

    for (j = 0; j < AUDIO_DAC_FILL_MS; j++) {
        for (i = 0; i < MY_SAMPLE_RATE; i++) {
            //left channel 1KHz
            *p_dac_buffer = sin_1khz_1ch[i];
            p_dac_buffer++;
			//right channel 2KHz
            *p_dac_buffer = sin_2khz_1ch[i];
            p_dac_buffer++;
        }
    }
}

static int _audio_dac_callback(void *cb_data, u32_t reason)
{
	hal_aout_channel_write_data(dac_out_handle, (u8_t *)dac_out_buffer, sizeof(sin_1khz_1ch)*2*AUDIO_DAC_FILL_MS);
	return 0;
}

s32_t ap_autotest_audio_dac_start(s32_t db)
{
	audio_out_init_param_t init_param;
	int ret = 0;

	SYS_LOG_INF("audio dac start...");

	dac_out_buffer = app_mem_malloc(sizeof(sin_1khz_1ch)*2*AUDIO_DAC_FILL_MS);
	init_dac_buffer(dac_out_buffer);

	memset(&init_param, 0x0, sizeof(audio_out_init_param_t));
	init_param.out_to_pa = 1;
	init_param.sample_rate = MY_SAMPLE_RATE;
	init_param.channel_type = AUDIO_CHANNEL_DAC;
	init_param.channel_mode = STEREO_MODE;
	init_param.data_width = 16;
	init_param.left_volume = db*1000;
	init_param.right_volume = db*1000;

	init_param.callback = _audio_dac_callback;

	dac_out_handle = hal_aout_channel_open(&init_param);

	ret = hal_aout_channel_write_data(dac_out_handle, (u8_t *)dac_out_buffer, sizeof(sin_1khz_1ch)*2*AUDIO_DAC_FILL_MS);

	hal_aout_channel_start(dac_out_handle);

	return ret;
}

s32_t ap_autotest_audio_dac_stop(void)
{
	hal_aout_channel_stop(dac_out_handle);
	hal_aout_channel_close(dac_out_handle);
	
	app_mem_free(dac_out_buffer);
	return 0;
}

