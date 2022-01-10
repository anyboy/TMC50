/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio system api
 * @brief
*/

#ifndef __AUDIO_SYSTEM_H__
#define __AUDIO_SYSTEM_H__
#include <audio_out.h>
#include <audio_in.h>
#include <stream.h>
#include <acts_ringbuf.h>
#include "bt_manager.h"

/**
 * @defgroup audio_system_apis Auido System APIs
 * @ingroup media_system_apis
 * @{
 */
typedef enum {
	AUDIO_STREAM_DEFAULT = 1,
	AUDIO_STREAM_MUSIC,
	AUDIO_STREAM_LOCAL_MUSIC,
	AUDIO_STREAM_TTS,
	AUDIO_STREAM_VOICE,
	AUDIO_STREAM_LINEIN,
	AUDIO_STREAM_LINEIN_MIX,
	AUDIO_STREAM_SUBWOOFER,
	AUDIO_STREAM_ASR,
	AUDIO_STREAM_AI,
	AUDIO_STREAM_USOUND,
	AUDIO_STREAM_USOUND_MIX,
	AUDIO_STREAM_USPEAKER,
	AUDIO_STREAM_I2SRX_IN,
	AUDIO_STREAM_SPDIF_IN,
	AUDIO_STREAM_GENERATE_IN,
	AUDIO_STREAM_GENERATE_OUT,
	AUDIO_STREAM_LOCAL_RECORD,
	AUDIO_STREAM_GMA_RECORD,
	AUDIO_STREAM_BACKGROUND_RECORD,
	AUDIO_STREAM_MIC_IN,
	AUDIO_STREAM_FM,
	AUDIO_STREAM_TWS,
	AUDIO_STREAM_EXT,
}audio_stream_type_e;

typedef enum {
	AUDIO_MODE_DEFAULT = 0,
	AUDIO_MODE_MONO,                    //mono->left mono->right
	AUDIO_MODE_STEREO,                  //left->left right->right
}audio_mode_e;

typedef enum {
	AUDIO_STREAM_TRACK = 1,
	AUDIO_STREAM_RECORD,
}audio_stream_mode_e;

typedef enum {
	AUDIO_FORMAT_PCM_8_BIT = 0,
	AUDIO_FORMAT_PCM_16_BIT,
	AUDIO_FORMAT_PCM_24_BIT,
	AUDIO_FORMAT_PCM_32_BIT,
}audio_format_e;

/**
**	aps changer mode
**/
typedef enum {
	/* audjust by audio pll*/
	APS_LEVEL_AUDIOPLL = 0,
	/* audjust by asrc*/
	APS_LEVEL_ASRC,
	APS_MODE_MAX,
} aps_level_mode_e;

/**
 * @cond INTERNAL_HIDDEN
 */

#define MIN_WRITE_SAMPLES    1 * 1024

#define MAX_AUDIO_TRACK_NUM  2
#define MAX_AUDIO_RECORD_NUM 2
#define MAX_AUDIO_DEVICE_NUM 1

#define MAX_VOLUME_VALUE 2
#define MIN_VOLUME_VALUE 1
#define DEFAULT_VOLUME   5

struct audio_track_t {
	u8_t stream_type;
	u8_t audio_format;
	u8_t audio_mode;
	u8_t channel_type;
	u8_t channel_id;
	u8_t channel_mode;
	u8_t sample_rate;
	u8_t output_sample_rate;
	u8_t frame_size;
	u8_t muted:1;
	u8_t stared:1;
	u8_t flushed:1;
	u8_t waitto_start:1;
	/**debug flag*/
	u8_t dump_pcm:1;
	u8_t fill_zero:1;
	u8_t fade_out:1;
	u16_t volume;

	u16_t pcm_frame_size;
	u8_t *pcm_frame_buff;

	io_stream_t audio_stream;
	io_stream_t mix_stream;
	u8_t mix_sample_rate;
	u8_t mix_channels;

	/** audio hal handle*/
	void *audio_handle;

	void (*event_cb)(u8_t, void *);
	void *user_data;

	/* For tws sync fill samples */
	int compensate_samples;
	int fill_cnt;
	u32_t output_samples;

	/* resample */
	void *res_handle;
	int res_in_samples;
	int res_out_samples;
	int res_remain_samples;
	s16_t *res_in_buf[2];
	s16_t *res_out_buf[2];

	/* fade in/out */
	void *fade_handle;

	/* mix */
	void *mix_handle;

	//debug info
	u32_t irq_cnt;
	u8_t track_rate;
	u8_t current_level;
	s32_t rel_diff;
	u32_t count;
};

struct audio_record_t {
	u8_t stream_type;
	u8_t audio_format;
	u8_t audio_mode;
	u8_t channel_type;
	u8_t channel_id;
	u8_t channel_mode;
	u8_t sample_rate;
	u8_t output_sample_rate;
	u8_t frame_size;
	u8_t muted:1;
	u8_t paused:1;
	u8_t first_frame:1;
	/**debug flag*/
	u8_t dump_pcm:1;
	u8_t fill_zero:1;

	s16_t adc_gain;
	s16_t input_gain;
	u16_t volume;
	u16_t pcm_buff_size;
	u8_t *pcm_buff;
	// bit /ms
	u8_t record_rate;
	u8_t irq_start_check;
	s32_t record_start_threshold;
	u32_t irq_cnt;
	u32_t irq_diff_samples;
	bt_clock_t bt_clk;
	s32_t irq_calc;
	/** audio hal handle*/
	void *audio_handle;
	io_stream_t audio_stream;
};

struct audio_system_t {
	os_mutex audio_system_mutex;
	struct audio_track_t *audio_track_pool[MAX_AUDIO_TRACK_NUM];
	struct audio_record_t *audio_record_pool[MAX_AUDIO_RECORD_NUM];
	struct audio_device_t *audio_device_pool[MAX_AUDIO_DEVICE_NUM];
	u8_t audio_track_num;
	u8_t audio_record_num;
	bool microphone_muted;
	u8_t output_sample_rate;
	u8_t capture_output_sample_rate;
	bool master_muted;
	u8_t master_volume;

	u8_t tts_volume;
	u8_t music_volume;
	u8_t voice_volume;
	u8_t linein_volume;
	u8_t fm_volume;
	u8_t i2srx_in_volume;
	u8_t mic_in_volume;
	u8_t spidf_in_volume;
	u8_t usound_volume;
	u8_t lcmusic_volume;
	u8_t max_volume;
	u8_t min_volume;
};

/** cace info ,used for cache stream */
typedef struct
{
	u8_t audio_type;
	u8_t audio_mode;
	u8_t channel_mode;
	u8_t stream_start:1;
	u8_t dma_start:1;
	u8_t dma_reload:1;
	u8_t pcm_buff_owner:1;
	u8_t data_finished:4;
	u16_t dma_send_len;
	u16_t pcm_frame_size;
	struct acts_ringbuf *pcm_buff;
	/**pcm cache*/
	io_stream_t pcm_stream;
} audio_info_t;

typedef enum
{
    APS_OPR_SET          = (1 << 0),
    APS_OPR_ADJUST       = (1 << 1),
    APS_OPR_FAST_SET     = (1 << 2),
}aps_ops_type_e;

typedef enum
{
    APS_STATUS_DEC,
    APS_STATUS_INC,
    APS_STATUS_DEFAULT,
}aps_status_e;

typedef enum
{
	APS_MONITOR_TYPE_UPLOAD,
	APS_MONITOR_TYPE_DOWNLOAD,
	APS_MONITOR_TYPE_MAX,
}aps_monitor_e;

typedef enum {
	APS_RESAMPLE_MODE_UP,
	APS_RESAMPLE_MODE_DOWN,
	APS_RESAMPLE_MODE_DEFAULT,
}aps_resample_mode_e;

typedef enum {
	APS_RESAMPLE_MODE_DOWN_LEVEV3 = -3,     //-5%
	APS_RESAMPLE_MODE_DOWN_LEVEV2,          //-3%
	APS_RESAMPLE_MODE_DOWN_LEVEV1,          //-1%
	APS_RESAMPLE_MODE_LEVEL_DEFAULT,              // 0
	APS_RESAMPLE_MODE_UP_LEVEV1,            //+1%
	APS_RESAMPLE_MODE_UP_LEVEV2,            //+3%
	APS_RESAMPLE_MODE_UP_LEVEV3,            //+5%
}aps_resample_level_e;

typedef struct {
	u8_t current_level;
	u8_t dest_level;
	u8_t aps_status;
	u8_t aps_mode;

	u8_t aps_min_level;
	u8_t aps_max_level;
	u8_t aps_default_level;
	u8_t role;
	u8_t duration;
	u8_t need_aps:1;

	u16_t aps_reduce_water_mark;
	u16_t aps_increase_water_mark;
	struct audio_track_t *audio_track;
	void *tws_observer;
}aps_monitor_info_t;
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief set audio system output sample rate
 *
 * @details This routine provides to set audio system output sample rate,
 *  if set audio system output sample rate, all out put stream may resample to
 *  the target sample rate
 *
 * @param value sample rate
 *
 * @return 0 excute successed , others failed
 */
int audio_system_set_output_sample_rate(int value);
/**
 * @brief get audio system output sample rate
 *
 * @details This routine provides to get audio system output sample rate,
 *
 * @return value of sample rate
 */
int audio_system_get_output_sample_rate(void);

/**
 * @brief set audio system master volume
 *
 * @details This routine provides to set audio system master volume
 *
 * @param value volume value
 *
 * @return 0 excute successed , others failed
 */

int audio_system_set_master_volume(int value);

/**
 * @brief get audio system master volume
 *
 * @details This routine provides to get audio system master volume
 *
 * @return value of volume level
 */

int audio_system_get_master_volume(void);

/**
 * @brief set audio system master mute
 *
 * @details This routine provides to set audio system master mute
 *
 * @param value mute value 1: mute 0: unmute
 *
 * @return 0 excute successed , others failed
 */

int audio_system_set_master_mute(int value);

/**
 * @brief get audio system master mute state
 *
 * @details This routine provides to get audio system master mute state
 *
 * @return  1: audio system muted
 * @return  0: audio system unmuted
 */

int audio_system_get_master_mute(void);

int audio_system_set_stream_volume(int stream_type, int value);

int audio_system_get_stream_volume(int stream_type);

int audio_system_get_current_volume(int stream_type);

int audio_system_set_stream_mute(int stream_type, int value);

int audio_system_get_stream_mute(int stream_type);

int audio_system_mute_microphone(int value);

int audio_system_get_microphone_muted(void);

int audio_system_get_current_pa_volume(int stream_type);

/* @volume in 0.001 dB */
int audio_system_set_stream_pa_volume(int stream_type, int volume);

/* @volume in 0.1 dB */
int audio_system_set_microphone_volume(int stream_type, int volume);

int audio_system_get_max_volume(void);

int audio_system_get_min_volume(void);
int audio_policy_get_upload_start_threshold(u8_t stream_type);

int aduio_system_init(void);
/**
 * @cond INTERNAL_HIDDEN
 */
int audio_system_register_track(struct audio_track_t *audio_track);

int audio_system_unregister_track(struct audio_track_t *audio_track);

int audio_system_register_record(struct audio_record_t *audio_record);

int audio_system_unregister_record(struct audio_record_t *audio_record);

void audio_aps_monitor(int pcm_time, int monitor_type);

void audio_aps_monitor_init(int monitor_type, int stream_type, int ext_stream_type, void *tws_observer, struct audio_track_t *audio_track);

void audio_aps_monitor_init_add_samples(int format, u8_t *need_notify, u8_t *need_sync);

void audio_aps_monitor_exchange_samples(u32_t *ext_add_samples, u32_t *sync_ext_samples);

void audio_aps_notify_decode_err(u16_t err_cnt);

void audio_aps_monitor_deinit(int format, void *tws_observer, struct audio_track_t *audio_track);

void audio_aps_monitor_tws_init(void *tws_observer);

void audio_aps_tws_notify_decode_err(u16_t err_cnt);

void audio_aps_monitor_tws_deinit(void *tws_observer);

aps_monitor_info_t *audio_aps_monitor_get_instance(int aps_monitor_type);

int audio_policy_get_aps_mode(void);

struct audio_track_t * audio_system_get_track(void);
struct audio_record_t *audio_system_get_record(void);


int audio_system_mutex_lock(void);
int audio_system_mutex_unlock(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @} end defgroup audio_system_apis
 */

#endif
