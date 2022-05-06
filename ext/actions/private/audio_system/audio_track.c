/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio track.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_track.h>
#include <audio_device.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <media_mem.h>
#include <assert.h>
#include <ringbuff_stream.h>
#include <arithmetic.h>

#include <gpio.h>
#include<device.h>

#include "bt_manager.h"

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio track"
#include <logging/sys_log.h>

#define FADE_IN_TIME_MS (60)
#define FADE_OUT_TIME_MS (100)

static u8_t reload_pcm_buff[1024];

extern int stream_read_pcm(asin_pcm_t *aspcm, io_stream_t stream, int max_samples, int debug_space);
extern void *media_resample_hw_open(u8_t channels, u8_t samplerate_in,
		u8_t samplerate_out, int *samples_in, int *samples_out, u8_t stream_type, int cnt);

extern int media_resample_hw_process(void *handle, u8_t channels, void *output_buf[2],
		void *input_buf[2], int input_samples);
extern void media_resample_hw_close(void *handle);

extern void *media_fade_open(u8_t sample_rate, u8_t channels, u8_t is_interweaved);
extern void media_fade_close(void *handle);
extern int media_fade_process(void *handle, void *inout_buf[2], int samples);
extern int media_fade_in_set(void *handle, int fade_time_ms);
extern int media_fade_out_set(void *handle, int fade_time_ms);
extern int media_fade_out_is_finished(void *handle);

extern void *media_mix_open(u8_t sample_rate, u8_t channels, u8_t is_interweaved);
extern void media_mix_close(void *handle);
extern int media_mix_process(void *handle, void *inout_buf[2], void *mix_buf,
			     int samples);

static int _aduio_track_update_output_samples(struct audio_track_t *handle, u32_t length)
{
	SYS_IRQ_FLAGS flags;
	sys_irq_lock(&flags);

	handle->output_samples += (u32_t )(length / handle->frame_size);

	sys_irq_unlock(&flags);

	return handle->output_samples;
}

static int _audio_track_data_mix(struct audio_track_t *handle, s16_t *src_buff, u16_t samples, s16_t *dest_buff)
{
	int ret = 0;
	u16_t mix_num = 0;
	u8_t track_channels = (handle->audio_mode != AUDIO_MODE_MONO) ? 2 : 1;
	asin_pcm_t mix_pcm = {
		.channels = handle->mix_channels,
		.sample_bits = 16,
		.pcm = { handle->res_in_buf[0], handle->res_in_buf[1], },
	};

	if (stream_get_length(handle->mix_stream) <= 0 && handle->res_remain_samples <= 0)
		return 0;

	if (track_channels > 1)
		samples /= 2;

	while (1) {
		u16_t mix_samples = min(samples, handle->res_remain_samples);
		s16_t *mix_buff[2] = {
			handle->res_out_buf[0] + handle->res_out_samples - handle->res_remain_samples,
			handle->res_out_buf[1] + handle->res_out_samples - handle->res_remain_samples,
		};

		/* 1) consume remain samples */
		if (handle->mix_handle && dest_buff == src_buff) {
			media_mix_process(handle->mix_handle, (void **)&dest_buff, mix_buff[0], mix_samples);
			dest_buff += track_channels * mix_samples;
			src_buff += track_channels * mix_samples;
		} else {
			if (track_channels > 1) {
				for (int i = 0; i < mix_samples; i++) {
					*dest_buff++ = (*src_buff++) / 2 + mix_buff[0][i] / 2;
					*dest_buff++ = (*src_buff++) / 2 + mix_buff[1][i] / 2;
				}
			} else {
				for (int i = 0; i < mix_samples; i++) {
					*dest_buff++ = (*src_buff++) / 2 + mix_buff[0][i] / 2;
				}
			}
		}

		handle->res_remain_samples -= mix_samples;
		samples -= mix_samples;
		mix_num += mix_samples;
		if (samples <= 0)
			break;

		/* 2) read mix stream and do resample as required */
		mix_pcm.samples = 0;
		ret = stream_read_pcm(&mix_pcm, handle->mix_stream, handle->res_in_samples, INT32_MAX);
		if (ret <= 0)
			break;

		if (handle->res_handle) {
			u8_t res_channels = min(handle->mix_channels, track_channels);

			if (ret < handle->res_in_samples) {
				memset((u16_t *)mix_pcm.pcm[0] + ret, 0, (handle->res_in_samples - ret) * 2);
				if (res_channels > 1)
					memset((u16_t *)mix_pcm.pcm[1] + ret, 0, (handle->res_in_samples - ret) * 2);
			}

			/* do resample */
			handle->res_out_samples = media_resample_hw_process(handle->res_handle, res_channels,
					(void **)handle->res_out_buf, (void **)mix_pcm.pcm, handle->res_in_samples);
			handle->res_remain_samples = handle->res_out_samples;
		} else {
			handle->res_out_samples = ret;
			handle->res_remain_samples = handle->res_out_samples;
		}
	}

	return mix_num;
}

int audio_track_calc_bt_time(bt_clock_t *bt_clock, bt_clock_t *pre_bt_clock,int count)
{
	int rel_time;

	rel_time = bt_clock->bt_clk - pre_bt_clock->bt_clk;
	rel_time = (rel_time * 312) + (rel_time / 2) + count * 7500;
	if (bt_clock->bt_intraslot >= pre_bt_clock->bt_intraslot) {
		rel_time += (bt_clock->bt_intraslot - pre_bt_clock->bt_intraslot);
	} else {
		rel_time -= (pre_bt_clock->bt_intraslot - bt_clock->bt_intraslot);
	}

	return rel_time;
}


static int _audio_track_request_more_data(void *handle, u32_t reason)
{
	static u8_t printk_cnt;
	struct audio_track_t *audio_track = (struct audio_track_t *)handle;
	int read_len = audio_track->pcm_frame_size / 2;
	int ret = 0;
	bool reload_mode = ((audio_track->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE);
	u8_t *buf = NULL;
	bt_clock_t bt_clock;
	bt_clock_t bt_sco_clock;
	int count;

	s32_t bt_diff = 0;
	u8_t level = audio_track->current_level;

	if (bt_manager_tws_is_hfp_mode()) {
		bt_manager_tws_get_bt_clock(&bt_clock);
		bt_manager_tws_get_sco_bt_clk(&bt_sco_clock,&count);
		if(!audio_track->count){
			audio_track->count = 2;//first irq is 7.5ms
			bt_diff =audio_track_calc_bt_time(&bt_clock,&bt_sco_clock,audio_track->count/2 - count);
			audio_track->rel_diff = bt_diff;
			printk("first %d us\n",audio_track->rel_diff);
		}else{
			audio_track->count++;
			if(audio_track->count % 2 == 0){
				bt_diff =audio_track_calc_bt_time(&bt_clock,&bt_sco_clock,audio_track->count/2 - count);

				if (bt_diff > audio_track->rel_diff) {
					//to quick
					level++;
					if (level <= APS_LEVEL_6) {
						audio_track->current_level = level;
						hal_aout_channel_set_aps(audio_track->audio_handle, audio_track->current_level, APS_LEVEL_AUDIOPLL);
					}
				} else if (bt_diff < audio_track->rel_diff){
					//to low
					level--;
					if (level >= APS_LEVEL_4) {
						audio_track->current_level = level;
						hal_aout_channel_set_aps(audio_track->audio_handle, audio_track->current_level, APS_LEVEL_AUDIOPLL);
					}
				} else {
					audio_track->current_level = APS_LEVEL_5;
					hal_aout_channel_set_aps(audio_track->audio_handle, audio_track->current_level, APS_LEVEL_AUDIOPLL);
				}
			}
		}

	}

	if (reload_mode) {
		if (reason == AOUT_DMA_IRQ_HF) {
			buf = audio_track->pcm_frame_buff;
		} else if (reason == AOUT_DMA_IRQ_TC) {
			buf = audio_track->pcm_frame_buff + read_len;
		}
	} else {
		buf = audio_track->pcm_frame_buff;
		if (bt_manager_tws_is_hfp_mode()) {
			read_len = audio_track->pcm_frame_size / 2;
		} else {
			if (stream_get_length(audio_track->audio_stream) > audio_track->pcm_frame_size) {
					read_len = audio_track->pcm_frame_size;
			} else {
					read_len = audio_track->pcm_frame_size / 2;
			}
		}

		if (audio_track->channel_id == AOUT_FIFO_DAC0) {
			if (reason == AOUT_DMA_IRQ_TC) {
				printk("pcm empty\n");
			}
		}
	}

	if (!bt_manager_tws_is_hfp_mode()) {
		if (audio_track->compensate_samples > 0) {
			/* insert data */
			if (audio_track->compensate_samples < read_len) {
				read_len = audio_track->compensate_samples;
			}
			memset(buf, 0, read_len);
			audio_track->fill_cnt += read_len;
			audio_track->compensate_samples -= read_len;
			goto exit;
		} else if (audio_track->compensate_samples < 0) {
			/* drop data */
			if (read_len > -audio_track->compensate_samples) {
				read_len = -audio_track->compensate_samples;
			}
			if (stream_get_length(audio_track->audio_stream) >= read_len * 2) {
				stream_read(audio_track->audio_stream, buf, read_len);
			_aduio_track_update_output_samples(audio_track, read_len);
			audio_track->fill_cnt -= read_len;
				audio_track->compensate_samples += read_len;
			}
		}
	}

	if (read_len > stream_get_length(audio_track->audio_stream)) {
		if (!audio_track->flushed) {
			memset(buf, 0, read_len);
			audio_track->fill_cnt += read_len;
			//if (!printk_cnt++) {
				printk("F %d\n",read_len);  //读数据错误
				//hal_aout_channel_write_data(audio_track->audio_handle, buf, read_len);
			//}
			goto exit;
		} else {
			read_len = stream_get_length(audio_track->audio_stream);
		}
	}

	ret = stream_read(audio_track->audio_stream, buf, read_len);
	if (ret != read_len) {
		memset(buf, 0, read_len);
		audio_track->fill_cnt += read_len;
		if (!printk_cnt++) {
			printk("F\n");
		}
	} else {
		_aduio_track_update_output_samples(audio_track, read_len);
	}

	if (audio_track->muted) {
		memset(buf, 0, read_len);
		audio_track->fill_cnt += read_len;
		goto exit;
	}


exit:
	if (!reload_mode && read_len > 0) {
#if FADE_OUT_TIME_MS > 0
		if (audio_track->flushed && audio_track->fade_handle) {
			int samples = audio_track->audio_mode == AUDIO_MODE_MONO ?
					    read_len / 2 : read_len / 4;
			media_fade_process(audio_track->fade_handle, (void **)&buf, samples);	
			//printk("F1 \n");
		}
#endif
		hal_aout_channel_write_data(audio_track->audio_handle, buf, read_len);	//写数据
		//printk("F1 %d\n",read_len);  read_len=1024
	}

	if (audio_track->channel_id == AOUT_FIFO_DAC0) {
		/**local music max send 3K pcm data */
		if (audio_track->stream_type == AUDIO_STREAM_LOCAL_MUSIC) {
			if (stream_get_length(audio_track->audio_stream) > audio_track->pcm_frame_size) {
				stream_read(audio_track->audio_stream, buf, audio_track->pcm_frame_size);
				_aduio_track_update_output_samples(audio_track, audio_track->pcm_frame_size);
#if FADE_OUT_TIME_MS > 0
				if (audio_track->flushed && audio_track->fade_handle) {
					int samples = audio_track->audio_mode == AUDIO_MODE_MONO ?
								audio_track->pcm_frame_size / 2 : audio_track->pcm_frame_size / 4;
					media_fade_process(audio_track->fade_handle, (void **)&buf, samples);
				}
#endif
				hal_aout_channel_write_data(audio_track->audio_handle, buf, audio_track->pcm_frame_size);
				
			}

			if (stream_get_length(audio_track->audio_stream) > audio_track->pcm_frame_size) {
				stream_read(audio_track->audio_stream, buf, audio_track->pcm_frame_size);
				_aduio_track_update_output_samples(audio_track, audio_track->pcm_frame_size);
#if FADE_OUT_TIME_MS > 0
				if (audio_track->flushed && audio_track->fade_handle) {
					int samples = audio_track->audio_mode == AUDIO_MODE_MONO ?
								audio_track->pcm_frame_size / 2 : audio_track->pcm_frame_size / 4;
					media_fade_process(audio_track->fade_handle, (void **)&buf, samples);
				}
#endif
				hal_aout_channel_write_data(audio_track->audio_handle, buf, audio_track->pcm_frame_size);
				
			}
		}

		/**last frame send more 2 samples */
		if (audio_track->flushed && stream_get_length(audio_track->audio_stream) == 0) {
			memset(buf, 0, 8);
			hal_aout_channel_write_data(audio_track->audio_handle, buf, 8);
			
		}
	}
	return 0;
}

static void *_audio_track_init(struct audio_track_t *handle)
{
	audio_out_init_param_t aout_param = {0};

#ifdef AOUT_CHANNEL_AA
	if (handle->channel_type == AOUT_CHANNEL_AA) {
		aout_param.aa_mode = 1;
	}
#endif

	aout_param.sample_rate =  handle->output_sample_rate;
	aout_param.channel_type = handle->channel_type;
	aout_param.channel_id =  handle->channel_id;
	aout_param.data_width = 16;
	aout_param.channel_mode = handle->audio_mode;
	aout_param.left_volume = audio_system_get_current_pa_volume(handle->stream_type);
	aout_param.right_volume = aout_param.left_volume;

	aout_param.sample_cnt_enable = 1;

	aout_param.callback = _audio_track_request_more_data;
	aout_param.callback_data = handle;

	if ((handle->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE) {
		aout_param.dma_reload = 1;
		aout_param.reload_len = handle->pcm_frame_size;
		aout_param.reload_addr = handle->pcm_frame_buff;
	}

	return hal_aout_channel_open(&aout_param);
}

static void _audio_track_stream_observer_notify(void *observer, int readoff, int writeoff, int total_size,
										unsigned char *buf, int num, stream_notify_type type)
{
	struct audio_track_t *handle = (struct audio_track_t *)observer;

#if FADE_IN_TIME_MS > 0
	if (handle->fade_handle && type == STREAM_NOTIFY_PRE_WRITE) {
		int samples = handle->audio_mode == AUDIO_MODE_MONO ? num / 2 : num / 4;

		media_fade_process(handle->fade_handle, (void **)&buf, samples);
	}
#endif

	audio_system_mutex_lock();

	if (handle->mix_stream && (type == STREAM_NOTIFY_PRE_WRITE)) {
		_audio_track_data_mix(handle, (s16_t *)buf, num / 2, (s16_t *)buf);
	}
	audio_system_mutex_unlock();
	if (!handle->stared
		&& (type == STREAM_NOTIFY_WRITE)
		&& !audio_track_is_waitto_start(handle)
		&& stream_get_length(handle->audio_stream) >= handle->pcm_frame_size) {

		os_sched_lock();

		if (handle->event_cb)
			handle->event_cb(1, handle->user_data);

		handle->stared = 1;

		int pcm_size = handle->pcm_frame_size;
		if (handle->sample_rate <= 16) {
			pcm_size = 480 + 0x30 * 2 + 16 * 4;
		}

		stream_read(handle->audio_stream, handle->pcm_frame_buff, pcm_size);
		_aduio_track_update_output_samples(handle, pcm_size);

		if ((handle->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE) {
			hal_aout_channel_start(handle->audio_handle);
		} else {
			hal_aout_channel_write_data(handle->audio_handle, handle->pcm_frame_buff, pcm_size);
		}
#ifdef CONFIG_USP
			struct device *usp_dev = device_get_binding(BOARD_OUTSPDIF_START_GPIO_NAME);
			gpio_pin_configure(usp_dev, BOARD_OUTSPDIF_START_GPIO_CONTROLLER, GPIO_DIR_OUT );
			gpio_pin_write(usp_dev, BOARD_OUTSPDIF_START_GPIO_CONTROLLER, 1);
#endif
		os_sched_unlock();
	}
}

struct audio_track_t *audio_track_create(u8_t stream_type, int sample_rate,
									u8_t format, u8_t audio_mode, void *outer_stream,
									void (*event_cb)(u8_t event, void *user_data), void *user_data)
{
	struct audio_track_t *audio_track = NULL;

	//while (audio_system_get_track()) {
	//	os_sleep(2);
	//}

	audio_system_mutex_lock();

	audio_track = mem_malloc(sizeof(struct audio_track_t));
	if (!audio_track)
		goto err_exit;

	memset(audio_track, 0, sizeof(struct audio_track_t));
	audio_track->stream_type = stream_type;
	audio_track->audio_format = format;
	audio_track->audio_mode = audio_mode;
	audio_track->sample_rate = sample_rate;
	audio_track->compensate_samples = 0;

	audio_track->channel_type = audio_policy_get_out_channel_type(stream_type);
	audio_track->channel_id = audio_policy_get_out_channel_id(stream_type);
	audio_track->channel_mode = audio_policy_get_out_channel_mode(stream_type);
	audio_track->volume = audio_system_get_stream_volume(stream_type);
	audio_track->output_sample_rate = audio_system_get_output_sample_rate();
	audio_track->count = 0;
	audio_track->current_level = APS_LEVEL_5;
	audio_track->rel_diff = 0;


	if (!audio_track->output_sample_rate)
		audio_track->output_sample_rate = audio_track->sample_rate;


	if (audio_track->audio_mode == AUDIO_MODE_DEFAULT)
		audio_track->audio_mode = audio_policy_get_out_audio_mode(stream_type);

	if (audio_track->audio_mode == AUDIO_MODE_MONO) {
		audio_track->frame_size = 2;
	} else if (audio_mode == AUDIO_MODE_STEREO) {
		audio_track->frame_size = 4;
	}

	/* dma reload buff */
	if (system_check_low_latencey_mode()) {
		if (audio_track->sample_rate <= 16) {
			audio_track->pcm_frame_size = 256;
		} else {
			audio_track->pcm_frame_size = 512;
		}
	} else {
		audio_track->pcm_frame_size = (sample_rate <= 16) ? 480 : 1024;
	}

	audio_track->pcm_frame_buff = reload_pcm_buff;
	if (!audio_track->pcm_frame_buff)
		goto err_exit;

	audio_track->audio_handle = _audio_track_init(audio_track);
	if (!audio_track->audio_handle) {
		goto err_exit;
	}

	if (audio_system_get_current_volume(audio_track->stream_type) == 0) {
		hal_aout_channel_mute_ctl(audio_track->audio_handle, 1);
	} else {
		hal_aout_channel_mute_ctl(audio_track->audio_handle, 0);
	}

	if (audio_policy_get_out_channel_type(audio_track->stream_type) == AUDIO_CHANNEL_I2STX ||
		audio_policy_get_out_channel_type(audio_track->stream_type) == AUDIO_CHANNEL_SPDIFTX) {
		if (audio_policy_get_out_channel_id(audio_track->stream_type) == AOUT_FIFO_DAC0) {
			audio_track_set_volume(audio_track, audio_system_get_current_volume(audio_track->stream_type));
		}
	}

	if (outer_stream) {
		audio_track->audio_stream = ringbuff_stream_create((struct acts_ringbuf *)outer_stream);
	} else {
		audio_track->audio_stream = ringbuff_stream_create_ext(
									media_mem_get_cache_pool(OUTPUT_PCM, stream_type),
									media_mem_get_cache_pool_size(OUTPUT_PCM, stream_type));
	}

	if (!audio_track->audio_stream) {
		goto err_exit;
	}

	if (stream_open(audio_track->audio_stream, MODE_IN_OUT | MODE_WRITE_BLOCK)) {
		stream_destroy(audio_track->audio_stream);
		audio_track->audio_stream = NULL;
		SYS_LOG_ERR(" stream open failed ");
		goto err_exit;
	}

	stream_set_observer(audio_track->audio_stream, audio_track,
		_audio_track_stream_observer_notify, STREAM_NOTIFY_WRITE | STREAM_NOTIFY_PRE_WRITE);

	if (audio_system_register_track(audio_track)) {
		SYS_LOG_ERR(" registy track failed ");
		goto err_exit;
	}

	if (audio_track->stream_type != AUDIO_STREAM_VOICE) {
		audio_track->fade_handle = media_fade_open(
				audio_track->sample_rate,
				audio_track->audio_mode == AUDIO_MODE_MONO ? 1 : 2, 1);
#if FADE_IN_TIME_MS > 0
		if (audio_track->fade_handle)
			media_fade_in_set(audio_track->fade_handle, FADE_IN_TIME_MS);
#endif
	}

	audio_track->event_cb = event_cb;
	audio_track->user_data = user_data;

	if (system_check_low_latencey_mode() && audio_track->sample_rate <= 16) {
		hal_aout_set_pcm_threshold(audio_track->audio_handle, 0x10, 0x20);
	}

	SYS_LOG_DBG("stream_type : %d", audio_track->stream_type);
	SYS_LOG_DBG("audio_format : %d", audio_track->audio_format);
	SYS_LOG_DBG("audio_mode : %d", audio_track->audio_mode);
	SYS_LOG_DBG("channel_type : %d", audio_track->channel_type);
	SYS_LOG_DBG("channel_id : %d", audio_track->channel_id);
	SYS_LOG_DBG("channel_mode : %d", audio_track->channel_mode);
	SYS_LOG_DBG("input_sr : %d ", audio_track->sample_rate);
	SYS_LOG_DBG("output_sr : %d", audio_track->output_sample_rate);
	SYS_LOG_DBG("volume : %d", audio_track->volume);
	SYS_LOG_DBG("audio_stream : %p", audio_track->audio_stream);
	audio_system_mutex_unlock();
	return audio_track;

err_exit:
	if (audio_track)
		mem_free(audio_track);
	audio_system_mutex_unlock();
	return NULL;
}

int audio_track_destory(struct audio_track_t *handle)
{
	assert(handle);

	SYS_LOG_INF("destory %p begin", handle);
	audio_system_mutex_lock();

	if (audio_system_unregister_track(handle)) {
		SYS_LOG_ERR(" failed\n");
		return -ESRCH;
	}

	if (handle->audio_handle) {
		hal_aout_channel_close(handle->audio_handle);
	}

	if (handle->audio_stream)
		stream_destroy(handle->audio_stream);

	if (handle->fade_handle)
		media_fade_close(handle->fade_handle);

	mem_free(handle);

	audio_system_mutex_unlock();
	SYS_LOG_INF("destory %p ok", handle);
	return 0;
}

int audio_track_start(struct audio_track_t *handle)
{
	assert(handle);

	audio_track_set_waitto_start(handle, false);
	handle->irq_cnt = 0;

	if (!handle->stared) {

		if (bt_manager_tws_get_dev_role() != BTSRV_TWS_NONE &&
			(handle->stream_type == AUDIO_STREAM_VOICE || handle->stream_type == AUDIO_STREAM_USOUND) &&
			bt_manager_tws_check_support_feature(TWS_FEATURE_HFP_TWS) &&
			audio_system_get_record()) {

		 } else if (stream_get_length(handle->audio_stream) < handle->pcm_frame_size) {
			return 0;
		}


		os_sched_lock();

		if (handle->event_cb) {
			handle->event_cb(1, handle->user_data);
		}

		handle->stared = 1;

		stream_read(handle->audio_stream, handle->pcm_frame_buff, handle->pcm_frame_size);

		if ((handle->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE) {
			hal_aout_channel_start(handle->audio_handle);
			_aduio_track_update_output_samples(handle, handle->pcm_frame_size);
		} else {
			int pcm_size = handle->pcm_frame_size;
			if (handle->sample_rate <= 16) {
				pcm_size = 480 +0x30*2+16*4;
			}
			hal_aout_channel_write_data(handle->audio_handle, handle->pcm_frame_buff, pcm_size);
			_aduio_track_update_output_samples(handle, pcm_size);
			if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER &&
			  handle->stream_type == AUDIO_STREAM_USOUND &&
			  bt_manager_tws_is_hfp_mode()) {
				//master do not do plc, but slave need, so we set 120 samples to audio
				pcm_size = 120 * 4;
				hal_aout_channel_write_data(handle->audio_handle, handle->pcm_frame_buff, pcm_size);
			  }
		}

		os_sched_unlock();
	}

	return 0;
}

int audio_track_stop(struct audio_track_t *handle)
{
	assert(handle);

	SYS_LOG_INF("stop %p begin ", handle);
	if (handle->audio_handle)
		hal_aout_channel_stop(handle->audio_handle);

	if (handle->audio_stream)
		stream_close(handle->audio_stream);
		
#ifdef CONFIG_USP
		struct device *usp_dev = device_get_binding(BOARD_OUTSPDIF_START_GPIO_NAME);
		gpio_pin_configure(usp_dev, BOARD_OUTSPDIF_START_GPIO_CONTROLLER, GPIO_DIR_OUT );
		gpio_pin_write(usp_dev, BOARD_OUTSPDIF_START_GPIO_CONTROLLER, 0);

#endif
	SYS_LOG_INF("stop %p ok ", handle);
	return 0;
}

int audio_track_pause(struct audio_track_t *handle)
{
	assert(handle);

	handle->muted = 1;

	stream_flush(handle->audio_stream);

	return 0;
}

int audio_track_resume(struct audio_track_t *handle)
{
	assert(handle);

	handle->muted = 0;

	return 0;
}

int audio_track_mute(struct audio_track_t *handle, int mute)
{
	assert(handle);

    handle->muted = mute;

	return 0;
}

int audio_track_is_muted(struct audio_track_t *handle)
{
	assert(handle);

    return handle->muted;
}

int audio_track_write(struct audio_track_t *handle, unsigned char *buf, int num)
{
	int ret = 0;

	assert(handle && handle->audio_stream);

	ret = stream_write(handle->audio_stream, buf, num);
	if (ret != num) {
		SYS_LOG_WRN(" %d %d\n", ret, num);
	}

	return ret;
}

int audio_track_flush(struct audio_track_t *handle)
{
	int try_cnt = 0;
	SYS_IRQ_FLAGS flags;

	assert(handle);

	sys_irq_lock(&flags);
	if (handle->flushed) {
		sys_irq_unlock(&flags);
		return 0;
	}
	/**wait trace stream read empty*/
	handle->flushed = 1;
	sys_irq_unlock(&flags);

#if FADE_OUT_TIME_MS > 0
	if (handle->fade_handle) {
		int fade_time = stream_get_length(handle->audio_stream) / 2;

		if (handle->audio_mode == AUDIO_MODE_STEREO)
			fade_time /= 2;

		fade_time /= handle->sample_rate;

		audio_track_set_fade_out(handle, fade_time);
	}
#endif

	while (stream_get_length(handle->audio_stream) > 0
			&& try_cnt++ < 100 && handle->stared) {
		if (try_cnt % 10 == 0) {
			SYS_LOG_INF("try_cnt %d stream %d\n", try_cnt,
					stream_get_length(handle->audio_stream));
		}
		os_sleep(2);
	}

	SYS_LOG_INF("try_cnt %d, left_len %d\n", try_cnt,
			stream_get_length(handle->audio_stream));

	audio_track_set_mix_stream(handle, NULL, 0, 1, AUDIO_STREAM_TTS);

	if (handle->audio_stream) {
		stream_close(handle->audio_stream);
	}

	return 0;
}

int audio_track_is_started(void)
{
	struct audio_track_t *handle = audio_system_get_track();
	if (handle == NULL) {
		return 0;
	}

	return handle->stared;
}

int audio_track_set_fade_out(struct audio_track_t *handle, int fade_time)
{
	assert(handle);

#if FADE_OUT_TIME_MS > 0
	if (handle->fade_handle && !handle->fade_out) {

		media_fade_out_set(handle->fade_handle, min(FADE_OUT_TIME_MS, fade_time));
		handle->fade_out = 1;
	}
#endif
	return 0;
}

int audio_track_set_waitto_start(struct audio_track_t *handle, bool wait)
{
	assert(handle);

	handle->waitto_start = wait ? 1 : 0;
	return 0;
}

int audio_track_is_waitto_start(struct audio_track_t *handle)
{
	if (!handle)
		return 0;

	return handle->waitto_start;
}

int audio_track_set_volume(struct audio_track_t *handle, int volume)
{
	u32_t pa_volume = 0;

	assert(handle);

	SYS_LOG_INF(" volume %d\n", volume);
	if (volume) {
		hal_aout_channel_mute_ctl(handle->audio_handle, 0);
		pa_volume = audio_policy_get_pa_volume(handle->stream_type, volume);
		hal_aout_channel_set_pa_vol_level(handle->audio_handle, pa_volume);
	} else {
		hal_aout_channel_mute_ctl(handle->audio_handle, 1);
		if (audio_policy_get_out_channel_type(handle->stream_type) == AUDIO_CHANNEL_I2STX) {
			/* i2s not support mute, so we set lowest volume -71625*/
			pa_volume = -71625;
			hal_aout_channel_set_pa_vol_level(handle->audio_handle, pa_volume);
		}
	}

	handle->volume = volume;
	return 0;
}

int audio_track_set_pa_volume(struct audio_track_t *handle, int pa_volume)
{
	assert(handle);

	SYS_LOG_INF("pa_volume %d\n", pa_volume);

	hal_aout_channel_mute_ctl(handle->audio_handle, 0);
	hal_aout_channel_set_pa_vol_level(handle->audio_handle, pa_volume);

	/* sync back to volume level, though meaningless for AUDIO_STREAM_VOICE at present. */
	handle->volume = audio_policy_get_volume_level_by_db(handle->stream_type, pa_volume / 100);
	return 0;
}

int audio_track_set_mute(struct audio_track_t *handle, bool mute)
{
	assert(handle);

	SYS_LOG_INF(" mute %d\n", mute);

	hal_aout_channel_mute_ctl(handle->audio_handle, mute);

	return 0;
}

int audio_track_get_volume(struct audio_track_t *handle)
{
	assert(handle);

	return handle->volume;
}

int audio_track_set_samplerate(struct audio_track_t *handle, int sample_rate)
{
	assert(handle);

	handle->sample_rate = sample_rate;
	return 0;
}

int audio_track_get_samplerate(struct audio_track_t *handle)
{
	assert(handle);

	return handle->sample_rate;
}

io_stream_t audio_track_get_stream(struct audio_track_t *handle)
{
	assert(handle);

	return handle->audio_stream;
}

int audio_track_get_fill_samples(struct audio_track_t *handle)
{
	assert(handle);
	if (handle->fill_cnt) {
		SYS_LOG_INF("fill_cnt %d\n", handle->fill_cnt);
	}
	return handle->fill_cnt;
}

u32_t audio_track_get_output_samples(struct audio_track_t *handle)
{
	SYS_IRQ_FLAGS flags;
	u32_t ret = 0;

	assert(handle);

	sys_irq_lock(&flags);

	ret = handle->output_samples;

	handle->output_samples = 0;

	sys_irq_unlock(&flags);
	return ret;
}

int audio_track_get_remain_pcm_samples(struct audio_track_t *handle)
{
	assert(handle);

	return hal_aout_channel_get_remain_pcm_samples(handle->audio_handle);
}

int audio_track_compensate_samples(struct audio_track_t *handle, int samples_cnt)
{
	SYS_IRQ_FLAGS flags;

	assert(handle);

	sys_irq_lock(&flags);

	handle->compensate_samples = samples_cnt;

	sys_irq_unlock(&flags);
	return 0;
}

int audio_track_set_mix_stream(struct audio_track_t *handle, io_stream_t mix_stream,
		u8_t sample_rate, u8_t channels, u8_t stream_type)
{
	int res = 0;

	assert(handle);

	audio_system_mutex_lock();

	if (audio_system_get_track() != handle) {
		goto exit;
	}

	if (mix_stream) {
		int frame_buf_size = media_mem_get_cache_pool_size(RESAMPLE_FRAME_DATA, stream_type);
		u8_t track_channels = (handle->audio_mode != AUDIO_MODE_MONO) ? 2 : 1;
		u8_t res_channels = min(channels, track_channels);

		if (sample_rate != handle->sample_rate) {
			int frame_size;

			handle->res_handle = media_resample_hw_open(
					res_channels, sample_rate, handle->sample_rate,
					&handle->res_in_samples, &handle->res_out_samples, stream_type, 1);
			if (!handle->res_handle) {
				SYS_LOG_ERR("media_resample_open failed");
				res = -ENOMEM;
				goto exit;
			}

			frame_size = channels * (ROUND_UP(handle->res_in_samples, 2) + ROUND_UP(handle->res_out_samples, 2));
			handle->res_in_buf[0] = mem_malloc(frame_size * 2);

			if (!handle->res_in_buf[0]) {
				SYS_LOG_ERR("frame mem malloc err");
				media_resample_hw_close(handle->res_handle);
				handle->res_handle = NULL;
				res = -ENOMEM;
				goto exit;
			}

			handle->res_in_buf[1] = (channels > 1) ?
					handle->res_in_buf[0] + ROUND_UP(handle->res_in_samples, 2) : handle->res_in_buf[0];

			handle->res_out_buf[0] = handle->res_in_buf[1] + ROUND_UP(handle->res_in_samples, 2);
			handle->res_out_buf[1] = (res_channels > 1) ?
					handle->res_out_buf[0] + ROUND_UP(handle->res_out_samples, 2) : handle->res_out_buf[0];
		} else {
			handle->res_in_samples = frame_buf_size / 2 / channels;
			handle->res_in_buf[1] = (channels > 1) ?
					handle->res_in_buf[0] + handle->res_in_samples : handle->res_in_buf[0];

			handle->res_out_buf[0] = handle->res_in_buf[0];
			handle->res_out_buf[1] = handle->res_in_buf[1];
		}

		handle->res_out_samples = 0;
		handle->res_remain_samples = 0;

		/* open mix: only support 1 mix channels */
		if (handle->mix_channels == 1) {
			handle->mix_handle = media_mix_open(handle->sample_rate, track_channels, 1);
		}
	}

	handle->mix_stream = mix_stream;
	handle->mix_sample_rate = sample_rate;
	handle->mix_channels = channels;

	if (!handle->mix_stream) {
		if (handle->res_handle) {
			if (handle->res_in_buf[0])
				mem_free(handle->res_in_buf[0]);

			media_resample_hw_close(handle->res_handle);
			handle->res_handle = NULL;
		}

		if (handle->mix_handle) {
			media_mix_close(handle->mix_handle);
			handle->mix_handle = NULL;
		}
	}

	SYS_LOG_INF("mix_stream %p, sample_rate %d->%d\n",
			mix_stream, sample_rate, handle->sample_rate);

exit:
	audio_system_mutex_unlock();
	return res;
}

io_stream_t audio_track_get_mix_stream(struct audio_track_t *handle)
{
	assert(handle);

	return handle->mix_stream;
}
