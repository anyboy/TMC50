/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file aging_playback.c
*/

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "aging_playback"
#include <logging/sys_log.h>
#endif

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <msg_manager.h>
#include <bt_manager.h>
#include <hotplug_manager.h>
#include <property_manager.h>
#include <thread_timer.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hex_str.h>
#include <global_mem.h>
#include <dvfs.h>
#include <thread_timer.h>
#include "app_defines.h"
#include "app_ui.h"
#include "app_switch.h"

#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <file_stream.h>
#include <audio_track.h>
#include <ringbuff_stream.h>

#include <app_aging_playback.h>

#include <random.h>

#define AGING_PLAYBACK_SR  48

static struct aging_playback_info g_aging_pb_info;
static short stream_fill_block_size;
static short *temp_buff;

static const short sin_1khz_1ch_48f[48] =
{
    0x0001, 0x0860, 0x109B, 0x188C,
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

extern u32_t _media_al_memory_start;
extern u32_t _media_al_memory_end;
extern u32_t _media_al_memory_size;

static short *white_noise_p;
static short write_noise_readptr;
static short write_noise_capacity;

static int gen_white_noise_by_trng(short *buffer, u32_t samples)
{
	struct device *trng_dev;
	int ret;

	trng_dev = device_get_binding(CONFIG_RANDOM_NAME);
	if (!trng_dev) {
		SYS_LOG_ERR("Bind trng device %s error", CONFIG_RANDOM_NAME);
		return -1;
	}

	ret = random_get_entropy(trng_dev, (u8_t *)buffer, samples * 2);

	return ret;
}

static void _aging_track_callback(u8_t event, void *user_data)
{

}

static void _aging_stream_fill(io_stream_t stream)
{
	int ret_val;
	while (stream_get_space(stream) >= stream_fill_block_size) {
		if (g_aging_pb_info.aging_playback_source == 2) {
			memcpy(temp_buff, (u8_t *)sin_1khz_1ch_48f, stream_fill_block_size);
		} else {
			memcpy(temp_buff, (u8_t *)(white_noise_p + write_noise_readptr), stream_fill_block_size);
			write_noise_readptr += (stream_fill_block_size/sizeof(short));
			if (write_noise_readptr >= write_noise_capacity)
				write_noise_readptr = 0;
		}
		ret_val = stream_write(stream, (u8_t *)temp_buff, stream_fill_block_size);
		if (ret_val <= 0)
			break;
	}
}

void aging_playback_app_try(void)
{
    int ret_val, i;
	unsigned int remain_playback_duration; /*unit 1 minute*/
	unsigned int current_playback_consume; /*unit 1 minute*/
	unsigned int start_playback_time;
	struct audio_track_t *aging_track = NULL;
	struct acts_ringbuf *aging_ringbuf;
	io_stream_t aging_stream;
	u32_t *aging_ringbuf_cache;

	SYS_LOG_INF("aging playback app try...");

#if (1)
	ret_val = property_get(CFG_AGING_PLAYBACK_INFO, (char *)&g_aging_pb_info, sizeof(struct aging_playback_info));
	if (ret_val < 0) {
		SYS_LOG_ERR("aging playback info read fail!");
		goto main_exit;
	}
#else
	g_aging_pb_info.aging_playback_status = AGING_PLAYBACK_STA_DOING;
	g_aging_pb_info.aging_playback_source = 1;
	g_aging_pb_info.aging_playback_volume = 15;
	g_aging_pb_info.aging_playback_duration = 1;
	g_aging_pb_info.aging_playback_consume = 0;
#endif

	if (g_aging_pb_info.aging_playback_status != AGING_PLAYBACK_STA_DOING) {
		SYS_LOG_ERR("aging playback info status invalid, %d!",
			g_aging_pb_info.aging_playback_status);
		goto main_exit;
	}
	
	if (g_aging_pb_info.aging_playback_duration <= g_aging_pb_info.aging_playback_consume) {
		SYS_LOG_ERR("aging playback info time invalid, duration %d <= consume %d!",
			g_aging_pb_info.aging_playback_duration, g_aging_pb_info.aging_playback_consume);
		goto main_exit;
	}

	if (g_aging_pb_info.aging_playback_source == 2) {
		/* Sine wave */
		stream_fill_block_size = sizeof(sin_1khz_1ch_48f);
	} else { /* Noise */
		stream_fill_block_size = AGING_PLAYBACK_SR * 2;
		SYS_LOG_INF("noise cache : [0x%x ~ 0x%x], 0x%x\n", (u32_t)(&_media_al_memory_start), \
				(u32_t)(&_media_al_memory_end), (u32_t)(&_media_al_memory_size));
		white_noise_p = (short *)(&_media_al_memory_start);
		write_noise_readptr = 0;
		write_noise_capacity = (u32_t)(&_media_al_memory_size) / stream_fill_block_size * stream_fill_block_size / sizeof(short);

		gen_white_noise_by_trng(white_noise_p, write_noise_capacity);

		/* max singal -6dB */
		for (i = 0; i < write_noise_capacity; i++) {
			*(white_noise_p + i) = *(white_noise_p + i) / 2;
		}
	}
	temp_buff = app_mem_malloc(stream_fill_block_size);

	remain_playback_duration = g_aging_pb_info.aging_playback_duration - g_aging_pb_info.aging_playback_consume;

	/* create audio track */
	aging_ringbuf_cache = app_mem_malloc(0x1000);
	aging_ringbuf = acts_ringbuf_init_ext(aging_ringbuf_cache, 0x1000);

	aging_track = audio_track_create(AUDIO_STREAM_MUSIC, AGING_PLAYBACK_SR,
									 AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO,
									 aging_ringbuf,
									 _aging_track_callback, NULL);

	aging_stream = audio_track_get_stream(aging_track);
	_aging_stream_fill(aging_stream);

	audio_track_set_volume(aging_track, g_aging_pb_info.aging_playback_volume);
	audio_track_start(aging_track);

	start_playback_time = os_uptime_get_32();
	current_playback_consume = 0;

	while (remain_playback_duration > 0) {
		
		/* TOTO : receive event and dispatch here */
		
		if (os_uptime_get_32() - start_playback_time >= 60*1000) {
			current_playback_consume++;
			remain_playback_duration--;
			start_playback_time += 60*1000;
		}

		_aging_stream_fill(aging_stream);

		os_sleep(10);
	}

	/* destroy audio track */
	audio_track_flush(aging_track);
	audio_track_stop(aging_track);
	audio_track_destory(aging_track);
	acts_ringbuf_destroy_ext(aging_ringbuf);
	app_mem_free(aging_ringbuf_cache);
	app_mem_free(temp_buff);
	
	g_aging_pb_info.aging_playback_consume += current_playback_consume;
	if (g_aging_pb_info.aging_playback_consume >= g_aging_pb_info.aging_playback_duration) {
		g_aging_pb_info.aging_playback_status = AGING_PLAYBACK_STA_PASSED;
	}
	ret_val = property_set(CFG_AGING_PLAYBACK_INFO, (char *)&g_aging_pb_info, sizeof(struct aging_playback_info));
	if (ret_val >= 0) {
		property_flush(CFG_AGING_PLAYBACK_INFO);
	}

main_exit:

    return;
}

