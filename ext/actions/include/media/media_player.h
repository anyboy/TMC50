/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Media player interface
 */

#ifndef __MEDIA_PLAYER_H__
#define __MEDIA_PLAYER_H__
#include <media_type.h>
#include <stream.h>
#include <media_service.h>
#include "energy_statistics.h"

/**
 * @defgroup media_player_apis Media Player APIs
 * @ingroup media_system_apis
 * @{
 */

/** media player type */
typedef enum {
	/** media player type: only support playback */
	MEDIA_PLAYER_TYPE_PLAYBACK = 0x01,
	/** media player type: only support capture */
	MEDIA_PLAYER_TYPE_CAPTURE = 0x02,
	/** media player type: only support capture and playback */
	MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK = 0x03,
} media_player_type_e;

/**media event type*/
typedef enum {
	PLAYBACK_EVENT_OPEN,
	PLAYBACK_EVENT_DATA_INDICATE,
	PLAYBACK_EVENT_PAUSE,
	PLAYBACK_EVENT_RESUME,
	PLAYBACK_EVENT_SEEK,
	PLAYBACK_EVENT_STOP_INTERUPT,
	PLAYBACK_EVENT_STOP_COMPLETE,
	PLAYBACK_EVENT_STOP_ERROR,
	PLAYBACK_EVENT_CLOSE,

	CAPTURE_EVENT_OPEN,
	CAPTURE_EVENT_DATA_INDICATE,
	CAPTURE_EVENT_PAUSE,
	CAPTURE_EVENT_RESUME,
	CAPTURE_EVENT_STOP_INTERUPT,
	CAPTURE_EVENT_STOP_COMPLETE,
	CAPTURE_EVENT_STOP_ERROR,
	CAPTURE_EVENT_CLOSE,
	CAPTURE_EVENT_ASR,
	CAPTURE_EVENT_VAD,

	/* player lifecycle event */
	PLAYER_EVENT_OPEN,
	PLAYER_EVENT_PLAY,
	PLAYER_EVENT_PAUSE,
	PLAYER_EVENT_RESUNE,
	PLAYER_EVENT_STOP,
	PLAYER_EVENT_CLOSE,
} media_event_type_e;

/** media capture mode */
typedef enum {
	PLAYBACK_MODE_MASK = 0xf,

	CAPTURE_MODE_ENCODE =  0x1 << 4,
	CAPTURE_MODE_ASR    =  0x2 << 4,
	CAPTURE_MODE_MASK   =  0xf << 4,
} media_mode_e;

/** media player structure */
typedef struct {
	/** type of media player @see media_player_type_e */
	u8_t type;
	/** flag of media player */
	u8_t flag;
	/** tws flag of media player */
	u8_t is_tws;
	/** dvfs level */
	u8_t dvfs_level;
	/** handle of media service*/
	void *media_srv_handle;
} media_player_t;


/**
 * @brief open new media player
 *
 * This routine provides to open new media player,
 *  support three type of media player &media_player_type_e
 *
 * @param init_param init param for open media player
 * @details	init_param.format play back format &media_type_e;
 *	init_param.sample_rate play back sample rate
 *	init_param.input_indicator input stream indicator, if set input stream ,indicator may set 0;
 *	init_param.input_stream input stream for playback, if set input indicator, input stream may create by service;
 *	init_param.input_stream_size input stream max size
 *	init_param.input_start_threshold input stream start threshold;
 *	init_param.input_stop_threshold input stream stop threshold;
 *	init_param.event_notify_handle playback event notify handle;
 *	init_param.output_indicator output stream indicator, if set output stream ,indicator may set 0;
 *	init_param.output_stream output stream for playback, if set output indicator, output stream may create by service;
 *	init_param.capture_format capture encode format;
 *	init_param.capture_sample_rate_input capture input sample rate;
 *	init_param.capture_sample_rate_output capture out sample rate;
 *	init_param.capture_input_indicator = AUDIO_STREAM_VOICE;
 *	init_param.capture_input_stream = NULL;
 *	init_param.capture_output_indicator = 0;
 *	init_param.capture_output_stream = upload_stream;
 *	init_param.capture_event_notify_handle capture event notify handle;
 *	init_param.dumpable indicate support dump data;
 *	init_param.support_tws indicate support tws;
 *	init_param.support_csb indicate support csb;
 *	init_param.aec_enable indicate support aec;
 *
 * @return handle of new media player
 */
media_player_t *media_player_open(media_init_param_t *init_param);

int media_player_set_extern_stream(media_player_t *handle, media_init_ext_param_t *ext_param);

/**
 * @brief start play for media player
 *
 * This routine provides to start meida player play
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_play(media_player_t *handle);

/**
 * @brief pause for media player
 *
 * This routine provides to pause meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_pause(media_player_t *handle);

/**
 * @brief resume for media player
 *
 * This routine provides to resume meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_resume(media_player_t *handle);

/**
 * @brief seek for media player
 *
 * This routine provides to seek meida player.
 * only supported when local music play or net music play.
 *
 * @param handle handle of media player
 * @param info media seek info @see media_seek_info_t
 *
 * @return 0 excute successed , others failed
 */
int media_player_seek(media_player_t *handle, media_seek_info_t *info);

/**
 * @brief stop for media player
 *
 * This routine provides to stop meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_stop(media_player_t *handle);

/**
 * @brief close for media player
 *
 * This routine provides to close meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_close(media_player_t *handle);

/**
 * @brief Query media player parameter
 *
 * This routine provides to query meida player parameter
 *
 * @param handle handle of media player
 * @param pname query parameter name
 * @param param address of parameter to store query result
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_get_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Configure media player parameter
 *
 * This routine provides to configure meida player parameter
 *
 * @param handle handle of media player
 * @param pname parameter name
 * @param param address of parameter buffer
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_set_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Query media service global parameter
 *
 * This routine provides to query media service global parameter
 *
 * @param handle handle of media player, can be NULL
 * @param pname query parameter name
 * @param param address of parameter to store query result
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_get_global_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Configure media service global parameter
 *
 * This routine provides to configure media service global parameter
 *
 * @param handle handle of media player, can be NULL
 * @param pname parameter name
 * @param param address of parameter buffer
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_set_global_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Get media player media info
 *
 * This routine provides to get meida player media info
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info address to store media info
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_get_mediainfo(media_player_t *handle, media_info_t *info)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_MEDIAINFO, info, sizeof(*info));
}

/**
 * @brief Get media player playing breakpoint
 *
 * This routine provides to get meida player breakpoint
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info  address to store breakpoint info
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_get_breakpoint(media_player_t *handle, media_breakpoint_info_t *info)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_BREAKPOINT, info, sizeof(*info));
}

/**
 * @brief Get media player output mode
 *
 * This routine provides to get meida player output mode. This output mode is
 * on the upper data stream of effect output mode.
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param mode  address to store output mode, see media_output_mode_e
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_get_output_mode(media_player_t *handle, int *mode)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_OUTPUT_MODE, mode, sizeof(*mode));
}

/**
 * @brief Get media player output energy detect info
 *
 * This routine provides to get meida player output energy info.
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info  address to store energy detect info, see energy_detect_info_t
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_get_energy_detect(media_player_t *handle, energy_detect_info_t *info)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_ENERGY_DETECT, info, sizeof(*info));
}

/**
 * @brief Set media player energy filter param.
 * 
 * This routine provides to set media player energy filter param.
 *
 * @param handle handle of media player
 * @param info	address of energy filter structure
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_set_energy_filter(media_player_t *handle, energy_filter_t *info)
{
	return media_player_set_parameter(handle, MEDIA_PARAM_ENERGY_FILTER, (void *)info, sizeof(*info));
}

/**
 * @brief Set media player energy detect param.
 * 
 * This routine provides to set media player energy detect param.
 *
 * @param handle handle of media player
 * @param param	address of energy detect param structure
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_set_energy_detect(media_player_t *handle, energy_detect_param_t *param)
{
	return media_player_set_parameter(handle, MEDIA_PARAM_ENERGY_DETECT, (void *)param, sizeof(*param));
}

/**
 * @brief get effect output mode for media player
 *
 * This routine provides to get effect output mode for meida player
 *
 * @param handle handle of media player
 * @param mode   output mode, see media_effect_output_mode_e
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_get_effect_output_mode(media_player_t *handle, int *mode)
{
	return media_player_get_parameter(handle, MEDIA_EFFECT_EXT_GET_DAE_OUTPUT_MODE, mode, sizeof(*mode));
}

/**
 * @brief Get media player freq point energy.
 *
 * This routine provides to get media player freq point energy.
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info   store the freq point energy, including the number of points and energy values.
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_get_freqpoint_energy(media_player_t *handle, media_freqpoint_energy_info_t *info)
{
	return media_player_get_parameter(handle, MEDIA_EFFECT_EXT_GET_FREQPOINT_ENERGY, info, sizeof(*info));
}

/**
 * @brief Set media player energy freq point.
 *
 * This routine provides to set media player energy freq point.
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info   set the energy freq points, including the number of points and freq values.
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_energy_freqpoint(media_player_t *handle, media_freqpoint_energy_info_t *info)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_ENERGY_FREQPOINT, info, sizeof(*info));
}

/**
 * @brief Set media player output mode
 *
 * This routine provides to set meida player output mode. This output mode is
 * on the upper data stream of effect output mode.
 *
 * Special case of media_player_set_parameter
 *
 * @param handle handle of media player
 * @param mode  output mode, see media_output_mode_e
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_set_output_mode(media_player_t *handle, int mode)
{
	return media_player_set_parameter(handle, MEDIA_PARAM_OUTPUT_MODE, (void *)mode, 0);
}

/**
 * @brief set volume for media player
 *
 * This routine provides to set volume of meida player
 *
 * @param handle handle of media player
 * @param vol_l da volume level of left channel
 * @param vol_r da volume level of right channel
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_volume(media_player_t *handle, int vol_l, int vol_r)
{
	unsigned int volume = ((vol_r & 0xff) << 8) | (vol_l & 0xff);

	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_VOLUME, (void *)volume, 0);
}

/**
 * @brief set mic mute for media player
 *
 * This routine provides to set mic mute for meida player
 *
 * @param handle handle of media player
 * @param mute bool for mute or unmute
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_mic_mute(media_player_t *handle, bool mute)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_MIC_MUTE, (void *)mute, 0);
}

/**
 * @brief set hfp connected flag for media player
 *
 * This routine provides to  set hfp connected flag for meida player
 *
 * @param handle handle of media player
 * @param connected bool for connected or unconnected
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_hfp_connected(media_player_t *handle, bool connected)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_HFP_CONNECTED, (void *)connected, 0);
}

/**
 * @brief set effect enable for media player
 *
 * This routine provides to enable or disable audio effects for meida player
 *
 * @param handle handle of media player
 * @param enable enable or disable audio effect
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_effect_enable(media_player_t *handle, bool enable)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_DAE_ENABLE, (void *)enable, 0);
}

/**
 * @brief set effect bypass for media player
 *
 * This routine provides to bybpass audio effects for meida player, only
 * retain fade in/out and output mode config.
 *
 * @param handle handle of media player
 * @param bypass bypass audio effect or not
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_effect_bypass(media_player_t *handle, bool bypass)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_DAE_BYPASS, (void *)bypass, 0);
}

/**
 * @brief set effect output mode for media player
 *
 * This routine provides to set effect output mode for meida player
 *
 * @param handle handle of media player
 * @param mode   output mode, see media_effect_output_mode_e
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_effect_output_mode(media_player_t *handle, int mode)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_DAE_OUTPUT_MODE, (void *)mode, 0);
}

/**
 * @brief update effect param for media player
 *
 * This routine provides to update effect param for media player
 *
 * @param handle handle of media player
 * @param param Address of effect param
 * @param size Size of effect param
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_update_effect_param(media_player_t *handle, void *param, unsigned int size)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_UPDATE_PARAM, param, size);
}

/**
 * @brief fade in for media player
 *
 * This routine provides to fade in for media player
 *
 * @param handle handle of media player
 * @param fade_time_ms fadein time in ms
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_fade_in(media_player_t *handle, int fade_time_ms)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_FADEIN, (void *)fade_time_ms, 0);
}

/**
 * @brief fade out for media player
 *
 * This routine provides to fade out for media player
 *
 * @param handle handle of media player
 * @param fade_time_ms fadeout time in ms
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_fade_out(media_player_t *handle, int fade_time_ms)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_FADEOUT, (void *)fade_time_ms, 0);
}

/**
 * @brief dump data for media player
 *
 * This routine provides to dump data for media player
 *
 * @param handle handle of media player
 * @param num number of tags to dump
 * @param tags data tag array for dumpable data
 * @param bufs ring buf array to store dump data, set NULL to stop dumping
 *
 * @return 0 excute successed , others failed
 */
int media_player_dump_data(media_player_t *handle, int num, const u8_t tags[], struct acts_ringbuf *bufs[]);

/**
 * @brief get current dumpable media player
 *
 * This routine provides to get current dumpable media player
 *
 * @return handle of media player @see media_player_t
 */
media_player_t *media_player_get_current_dumpable_player(void);

/**
 * @brief get current main media player
 *
 * This routine provides to get current main media player
 *
 * @return handle of media player @see media_player_t
 */
media_player_t *media_player_get_current_main_player(void);

/**
 * @brief repqir file header
 *
 * This routine provides repair file header.
 *
 * @param handle of media player, can be NULL
 * @format file format
 * @data_length data length of file
 * @fstream stream standing for the file
 *
 * @return 0 excute successed , others failed
 */
int media_player_repair_filhdr(media_player_t *handle, u8_t format, io_stream_t fstream);

/**
 * @brief start recording
 *
 * This routine provides start recording.
 *
 * @param handle of media player, can be NULL
 * @info  record info
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_start_record(media_player_t *handle, media_record_info_t *info)
{
	return media_player_set_global_parameter(handle, MEDIA_PARAM_SET_RECORD, info, sizeof(*info));
}

/**
 * @brief stop background recording
 *
 * This routine provides start background recording.
 *
 * @param handle of media player, can be NULL
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_stop_record(media_player_t *handle)
{
	return media_player_set_global_parameter(handle, MEDIA_PARAM_SET_RECORD, NULL, 0);
}

/**
 * @brief Set media player lifecycle notifier
 *
 * This routine provides set lifecycle notifier, like player open/close.
 *
 *@param notify lifecycle event notify.
 *
 * @return 0 excute successed, others failed
 */
int media_player_set_lifecycle_notifier(media_srv_event_notify_t notify);

/**
 * @brief switch media player effet param
 *
 * This routine provides switch media player effet param.
 *
 *@param handle  of media player
 *@param efx_name  efx name of audio effect
 *
 * @return 0 excute successed, others failed
 */
int media_player_switch_effect_param(media_player_t *handle, const u8_t *efx_name);

/**
 * @} end defgroup media_player_apis
 */
#endif  /* __MEDIA_PLAYER_H__ */
