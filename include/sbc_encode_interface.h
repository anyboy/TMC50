/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief sbc encode interface
 */
#ifndef _SBC_ENCODE_INTERFACE_H
#define _SBC_ENCODE_INTERFACE_H

typedef struct
{
	/** Sampling rate in hz */
    int sample_rate;
   /** Number of channels */
    int channels;
    /** Bit rate */
    int bitrate;
    /**
      * Encoding library format,
      * a loaded code library may contain multiple formats
      * such as: IMADPCM
      */
    int audio_format;
    /**
      * Once to obtain the audio encoded data playback time
      * 0 that does not limit, in ms
      */
    int chunk_time;

	int input_buffer;
    int input_len;
    int output_buffer;
    int output_len;
	int output_data_size;
} sbc_encode_param_t;

typedef struct
{
	unsigned long flags;

	uint8_t frequency;
	uint8_t blocks;
	uint8_t subbands;
	uint8_t mode;
	uint8_t allocation;
	uint8_t bitpool;
	uint8_t endian;

	void *priv;
	void *priv_alloc_base;
} sbc_obj_t;

extern int sbc_init(void *p_sbc, unsigned long flags, uint8_t *sbc_encode_buffer);

extern void sbc_finish(void *p_sbc);

extern int sbc_encode_entry(void *p_sbc,void *param,ssize_t *written);

#endif

