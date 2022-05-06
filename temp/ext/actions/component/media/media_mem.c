/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file media mem interface
 */
#define SYS_LOG_DOMAIN "media"
#include <linker/section_tags.h>
#include <os_common_api.h>
#include <mem_manager.h>
#include <media_service.h>
#include <audio_system.h>

#include "media_mem.h"

struct media_memory_cell {
	u8_t mem_type;
	u32_t mem_base;
	u32_t mem_size;
};

struct media_memory_block {
	u8_t stream_type;
	struct media_memory_cell mem_cell[31];
};
#ifdef CONFIG_MEDIA
/*
 * sbc(msbc) dec: global - 0x0d10, share - 0x0000
 * sbc(msbc) enc: global - 0x0be4, share - 0x0000
 *
 * cvsd dec: global - 0x0308, share - 0x0000
 * cvsd enc: global - 0x0488, share - 0x0000
 */
static char sbc_dec_global_bss[0x0d20] __in_section_unique(SBC_DEC_BUF) __aligned(16);

static char sbc_enc_global_bss[0x0bec] __in_section_unique(SBC_ENC_BUF) __aligned(16);

static char msbc_dec2_global_bss[0x0d20] _NODATA_SECTION(.msbc.dec2.global_bss) __aligned(16);

#ifdef CONFIG_TWS_MONO_MODE
static char sbc_enc2_global_bss[0x0bec] __in_section_unique(SBC_ENC2_BUF) __aligned(16);

static char sbc_dec2_global_bss[0x0d20] _NODATA_SECTION(.btmusic_pcm_bss) __aligned(16);
static char btmusic_pcm_bss[0x4800] _NODATA_SECTION(.btmusic_pcm_bss);
#else
/* btmusic and pcm codec based stream type, linein, usound, etc. */
static char btmusic_pcm_bss[0x3000] _NODATA_SECTION(.btmusic_pcm_bss);
#endif

/*
  * mp3 enc: inbuf - 0x1200, outbuf - 0x0600
  * opus enc: inbuf - 0x280, outbuf - 0x28
  * wav (lpcm) enc: inbuf - 0x200, outbuf - 0x200
  * wav (adpcm) enc: inbuf - 0x1FE4 (2041 sample pairs), outbuf - 0x800
  */
#ifdef CONFIG_RECORD_SERVICE
/* 2 recording sources * 1 channels, or 1 recording source * 2 channels, plus 1 pre/post process frame respectively */
static char wav_enc_adpcm_inbuf[0x1FE4 + 0x400] __in_section_unique(wav_enc.adpcm.inbuf);
static char wav_enc_adpcm_outbuf[0x800] __in_section_unique(wav_enc.adpcm.outbuf);
#endif

static char playback_input_buffer[13 * 1024] __in_section_unique(media.buff.noinit);


#ifdef CONFIG_TOOL_ASQT
/* asqt */
static char asqt_tool_stub_buf[1548] _NODATA_SECTION(tool.asqt.buf.noinit);
#endif

#ifdef CONFIG_TOOL_ECTT
static char ectt_tool_buf[3072] _NODATA_SECTION(tool.ectt.buf.noinit);
#endif

#ifdef CONFIG_ACTIONS_DECODER
static char codec_stack[2048] __aligned(STACK_ALIGN) __in_section_unique(codec.noinit.stack);
#endif

//#ifdef CONFIG_HFP_SPEECH
//#ifdef CONFIG_AEC_TAIL_LENGTH
//#define MSBC_AEC_TAIL_LENGTH (CONFIG_AEC_TAIL_LENGTH)
//#else
//#define MSBC_AEC_TAIL_LENGTH (32)
//#endif
//#define CVSD_AEC_TAIL_LENGTH (MSBC_AEC_TAIL_LENGTH * 2)
//static char hfp_aec_global_bss[10536 * 2 + 1280 * 2 + MSBC_AEC_TAIL_LENGTH * 32 * 6] __hfp_aec_ovl_bss __aligned(4);
//#ifdef CONFIG_OUTPUT_RESAMPLE
//static char hfp_aec_share_bss[3082 * 2] __hfp_aec_ovl_bss __aligned(4);
//#endif
//#endif /* CONFIG_HFP_SPEECH */

#ifdef CONFIG_HFP_PLC
static char hfp_plc_global_bss[2030 * 2 + 120 * 2] __hfp_plc_ovl_bss __aligned(4);
//#ifndef CONFIG_HFP_SPEECH
//static char hfp_plc_share_bss[1950 * 2] __hfp_plc_ovl_bss __aligned(4);
//#endif
#endif /* CONFIG_HFP_PLC */


#ifdef CONFIG_RESAMPLE



static char resample_share_bss[1776] __aligned(4)  _NODATA_SECTION(.resample.share_buf);


//static char resample_frame_bss[3184] __aligned(4)  _NODATA_SECTION(.resample.frame_buf);
static char resample_share_bss2[1776] __aligned(4)  _NODATA_SECTION(.resample.share_buf2);
static char resample_frame_bss[512] __aligned(4)  _NODATA_SECTION(.resample.frame_buf);
static char resample_frame_aps_bss[256] __aligned(4)  _NODATA_SECTION(.resample.frame_buf);
/*static*/ char resample_global_bss[820 + 32] __aligned(4) _NODATA_SECTION(.resample.global_buf);
static char resample_uac_frame_bss[0x480 + 0xC0 +32] __aligned(4) _NODATA_SECTION(.resample.frame_uac_buf);
static char resample_uac_global_bss[820 + 32] __aligned(4) _NODATA_SECTION(.resample.global_uac_buf);
static char resample_aps_bss[1776] __aligned(4)  _NODATA_SECTION(.resample.aps_buf);

#endif
static char cascade_extranel_buf[0x1600] __aligned(4) _NODATA_SECTION(.cascade.extranel_buf);
static char sbc_dec_uac_global_bss[0x0d20] _NODATA_SECTION(.cascade.dec_uac_buf) __aligned(16);;

static const struct media_memory_block media_memory_config[] = {
	{
		.stream_type = AUDIO_STREAM_MUSIC,
		.mem_cell = {
			{.mem_type = OUTPUT_DECODER,  .mem_base = (u32_t)&btmusic_pcm_bss[0x0000], .mem_size = 0x800,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (u32_t)&btmusic_pcm_bss[0x0800], .mem_size = 0x800,},
			{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&btmusic_pcm_bss[0x1000], .mem_size = 0xa00,},
			{.mem_type = DECODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_global_bss[0], .mem_size = sizeof(sbc_dec_global_bss),},

		#ifdef CONFIG_TWS_MONO_MODE
			{.mem_type = TWS_LOCAL_INPUT, .mem_base = (u32_t)&playback_input_buffer[0], .mem_size = sizeof(playback_input_buffer),},
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (u32_t)&btmusic_pcm_bss[0x1800], .mem_size = 0x3000,},
			{.mem_type = SBC_DECODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec2_global_bss[0], .mem_size = sizeof(sbc_dec2_global_bss),},
			{.mem_type = SBC_ENCODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc_global_bss[0], .mem_size = sizeof(sbc_enc_global_bss),},
			{.mem_type = SBC_ENCODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc2_global_bss[0], .mem_size = sizeof(sbc_enc2_global_bss),},
		#else
			{.mem_type = SBC_ENCODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc_global_bss[0], .mem_size = sizeof(sbc_enc_global_bss),},
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (u32_t)&playback_input_buffer[0], .mem_size = sizeof(playback_input_buffer),},
		#endif

		#ifdef CONFIG_TOOL_ECTT
			{.mem_type = TOOL_ECTT_BUF, .mem_base = (u32_t)&ectt_tool_buf[0], .mem_size = sizeof(ectt_tool_buf),},
		#endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_RESAMPLE
		#ifdef CONFIG_OUTPUT_RESAMPLE
			{.mem_type = RESAMPLE_GLOBAL_DATA,  .mem_base = (u32_t)resample_global_bss, .mem_size = sizeof(resample_global_bss),},
		#endif
			{.mem_type = RESAMPLE_SHARE_DATA, .mem_base = (u32_t)resample_share_bss, .mem_size = sizeof(resample_share_bss),},
			{.mem_type = RESAMPLE_FRAME_DATA, .mem_base = (u32_t)resample_frame_bss, .mem_size = sizeof(resample_frame_bss),},
		#endif
		},
	},

	{
		.stream_type = AUDIO_STREAM_LOCAL_MUSIC,
		.mem_cell = {
			{.mem_type = TWS_LOCAL_INPUT, .mem_base = (u32_t)&playback_input_buffer[0], .mem_size = 0x1400,},
			{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x1400], .mem_size = 0x1800,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (u32_t)&playback_input_buffer[0x2c00], .mem_size = 0x0200,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (u32_t)&playback_input_buffer[0x2e00], .mem_size = 0x0200,},

			{.mem_type = SBC_DECODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_global_bss[0], .mem_size = sizeof(sbc_dec_global_bss),},
			{.mem_type = SBC_ENCODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc_global_bss[0], .mem_size = sizeof(sbc_enc_global_bss),},
		#ifdef CONFIG_TWS_MONO_MODE
			{.mem_type = SBC_ENCODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc2_global_bss[0], .mem_size = sizeof(sbc_enc2_global_bss),},
		#endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_TOOL_ECTT
			{.mem_type = TOOL_ECTT_BUF, .mem_base = (u32_t)&ectt_tool_buf[0], .mem_size = sizeof(ectt_tool_buf),},
		#endif

		#ifdef CONFIG_RESAMPLE
		#ifdef CONFIG_OUTPUT_RESAMPLE
			{.mem_type = RESAMPLE_GLOBAL_DATA,  .mem_base = (u32_t)resample_global_bss, .mem_size = sizeof(resample_global_bss),},
		#endif
			{.mem_type = RESAMPLE_SHARE_DATA, .mem_base = (u32_t)resample_share_bss, .mem_size = sizeof(resample_share_bss),},
			{.mem_type = RESAMPLE_FRAME_DATA, .mem_base = (u32_t)resample_frame_bss, .mem_size = sizeof(resample_frame_bss),},
		#endif
		},
	},

	{
		.stream_type = AUDIO_STREAM_LINEIN,
		.mem_cell = {
			{.mem_type = TWS_LOCAL_INPUT, .mem_base = (u32_t)&playback_input_buffer[0], .mem_size = 0x3000,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (u32_t)&btmusic_pcm_bss[0x0000], .mem_size = 0x400,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (u32_t)&btmusic_pcm_bss[0x0400], .mem_size = 0x400,},
			{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&btmusic_pcm_bss[0x0800], .mem_size = 0x400,},

			{.mem_type = INPUT_PCM,       .mem_base = (u32_t)&btmusic_pcm_bss[0x1000], .mem_size = 0x400,},
			{.mem_type = INPUT_ENCBUF,    .mem_base = (u32_t)&btmusic_pcm_bss[0x1800], .mem_size = 0x200,},
			{.mem_type = OUTPUT_CAPTURE,  .mem_base = (u32_t)&btmusic_pcm_bss[0x1A00], .mem_size = 0x200,},
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (u32_t)&btmusic_pcm_bss[0x1C00], .mem_size = 0x400,},

			{.mem_type = SBC_DECODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_global_bss[0], .mem_size = sizeof(sbc_dec_global_bss),},
			{.mem_type = SBC_ENCODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc_global_bss[0], .mem_size = sizeof(sbc_enc_global_bss),},
		#ifdef CONFIG_TWS_MONO_MODE
			{.mem_type = SBC_ENCODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc2_global_bss[0], .mem_size = sizeof(sbc_enc2_global_bss),},
		#endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_RESAMPLE
		#ifdef CONFIG_OUTPUT_RESAMPLE
			{.mem_type = RESAMPLE_GLOBAL_DATA,  .mem_base = (u32_t)resample_global_bss, .mem_size = sizeof(resample_global_bss),},
		#endif
			{.mem_type = RESAMPLE_SHARE_DATA, .mem_base = (u32_t)resample_share_bss, .mem_size = sizeof(resample_share_bss),},
			{.mem_type = RESAMPLE_FRAME_DATA, .mem_base = (u32_t)resample_frame_bss, .mem_size = sizeof(resample_frame_bss),},
		#endif
		},
	},

	{
		.stream_type = AUDIO_STREAM_USOUND,
		.mem_cell = {
			{.mem_type = TWS_LOCAL_INPUT, .mem_base = (u32_t)&playback_input_buffer[0], .mem_size = 0x2400,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (u32_t)&btmusic_pcm_bss[0x0000], .mem_size = 0x400,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (u32_t)&btmusic_pcm_bss[0x0400], .mem_size = 0x400,},
			{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&btmusic_pcm_bss[0x0800], .mem_size = 0x800,},

			{.mem_type = INPUT_CAPTURE_EXT, .mem_base = (u32_t)&cascade_extranel_buf[0x000], .mem_size = 0x400,},
			{.mem_type = EXT_GLOBLE_BUF, .mem_base = (u32_t)&cascade_extranel_buf[0x400], .mem_size = 0x400,},
			{.mem_type = INPUT_EXT_STREAM, .mem_base = (u32_t)&cascade_extranel_buf[0x800], .mem_size = 0x400,},
			{.mem_type = INPUT_EXT_MIX_STREAM, .mem_base = (u32_t)&cascade_extranel_buf[0xC00], .mem_size = 0x200,},

			{.mem_type = INPUT_PCM,       .mem_base = (u32_t)&btmusic_pcm_bss[0x1000], .mem_size = 0x800,},
			{.mem_type = INPUT_ENCBUF,    .mem_base = (u32_t)&btmusic_pcm_bss[0x1800], .mem_size = 0x200,},
			{.mem_type = OUTPUT_CAPTURE,  .mem_base = (u32_t)&btmusic_pcm_bss[0x1A00], .mem_size = 0x200,},
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (u32_t)&btmusic_pcm_bss[0x1C00], .mem_size = 0x1000,},
			{.mem_type = RESAMPLE_UAC_OUT_STREAM, .mem_base = (u32_t)&cascade_extranel_buf[0xE00], .mem_size = 0x200,},
			{.mem_type = RESAMPLE_UAC_IN_STREAM, .mem_base = (u32_t)&cascade_extranel_buf[0x1000], .mem_size = 0x200,},
			{.mem_type = RESAMPLE_OUTPUT_DATA, .mem_base = (u32_t)&cascade_extranel_buf[0x1200], .mem_size = 0x200,},

			{.mem_type = USB_UPLOAD_CACHE, .mem_base = (u32_t)&playback_input_buffer[0x27A0], .mem_size = 0x800,},
			{.mem_type = USB_UPLOAD_PAYLOAD, .mem_base = (u32_t)&playback_input_buffer[0x2FA0], .mem_size = 0x60,},

			{.mem_type = SBC_DECODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_global_bss[0], .mem_size = sizeof(sbc_dec_global_bss),},
			{.mem_type = SBC_DECODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_uac_global_bss[0], .mem_size =sizeof(sbc_dec_uac_global_bss),},
			{.mem_type = SBC_ENCODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc_global_bss[0], .mem_size = sizeof(sbc_enc_global_bss),},
		#ifdef CONFIG_TWS_MONO_MODE
			{.mem_type = SBC_ENCODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc2_global_bss[0], .mem_size = sizeof(sbc_enc2_global_bss),},
		#endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_RESAMPLE
		#ifdef CONFIG_OUTPUT_RESAMPLE
			{.mem_type = RESAMPLE_GLOBAL_DATA,  .mem_base = (u32_t)resample_global_bss, .mem_size = sizeof(resample_global_bss),},
		#endif
			{.mem_type = RESAMPLE_SHARE_DATA, .mem_base = (u32_t)resample_share_bss, .mem_size = sizeof(resample_share_bss),},
			{.mem_type = RESAMPLE_FRAME_DATA, .mem_base = (u32_t)resample_frame_bss, .mem_size = sizeof(resample_frame_bss),},
			{.mem_type = RESAMPLE_SHARE_DATA2, .mem_base = (u32_t)resample_share_bss2, .mem_size = sizeof(resample_share_bss2),},
			{.mem_type = RESAMPLE_SHARE_DATA_APS, .mem_base = (u32_t)resample_aps_bss, .mem_size = sizeof(resample_aps_bss),},
			{.mem_type = RESAMPLE_FRAME_DATA_APS, .mem_base = (u32_t)resample_frame_aps_bss, .mem_size = sizeof(resample_frame_aps_bss),},
			{.mem_type = RESAMPLE_UAC_FRAME_DATA, .mem_base = (u32_t)resample_uac_frame_bss, .mem_size = sizeof(resample_uac_frame_bss),},
			{.mem_type = RESAMPLE_UAC_GLOBAL_DATA, .mem_base = (u32_t)resample_uac_global_bss, .mem_size = sizeof(resample_uac_global_bss),},
		#endif
		},
	},

	{
		.stream_type = AUDIO_STREAM_VOICE,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (u32_t)&playback_input_buffer[0x000], .mem_size = 0x2A8,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (u32_t)&playback_input_buffer[0x2A8], .mem_size = 0x200,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (u32_t)&playback_input_buffer[0x4A8], .mem_size = 0x200,},
			//{.mem_type = OUTPUT_PLAYBACK, .mem_base = (u32_t)&playback_input_buffer[0x3000], .mem_size = 0x400,},
			{.mem_type = INPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x8A8], .mem_size = 0x200,},
			{.mem_type = INPUT_CAPTURE,  .mem_base = (u32_t)&playback_input_buffer[0xAA8], .mem_size = 0x200,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (u32_t)&playback_input_buffer[0xCA8], .mem_size = 0x400,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (u32_t)&playback_input_buffer[0x10A8], .mem_size = 0x078,},
			{.mem_type = AEC_REFBUF0,    .mem_base = (u32_t)&playback_input_buffer[0x1120], .mem_size = 0x400,},

			{.mem_type = OUTPUT_SCO, .mem_base = (u32_t)&playback_input_buffer[0x1520], .mem_size = 0xE8,},
			{.mem_type = TX_SCO,     .mem_base = (u32_t)&playback_input_buffer[0x1608], .mem_size = 0x7C,},
			{.mem_type = RX_SCO,     .mem_base = (u32_t)&playback_input_buffer[0x1684], .mem_size = 0xF0,},
		#ifndef CONFIG_OUTPUT_RESAMPLE
			{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x6A8], .mem_size = 0x200,},		
		#else
			//{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x1780], .mem_size = 0xA00,},
			{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x6A8], .mem_size = 0x200,},
		#endif

		//{.mem_type = INPUT_PCM, 	 .mem_base = (u32_t)&playback_input_buffer[0x2180], .mem_size = 0xA00,},

			{.mem_type = INPUT_CAPTURE_EXT, .mem_base = (u32_t)&cascade_extranel_buf[0x000], .mem_size = 0x600,},
			{.mem_type = RESAMPLE_OUTPUT_DATA, .mem_base = (u32_t)&cascade_extranel_buf[0x600], .mem_size = 0x200,},
			{.mem_type = EXT_GLOBLE_BUF, .mem_base = (u32_t)&cascade_extranel_buf[0x800], .mem_size = 0x400,},
			{.mem_type = INPUT_EXT_STREAM, .mem_base = (u32_t)&cascade_extranel_buf[0xC00], .mem_size = 0x800,},
			{.mem_type = INPUT_EXT_MIX_STREAM, .mem_base = (u32_t)&cascade_extranel_buf[0x1400], .mem_size = 0x200,},

			{.mem_type = SBC_DECODER_GLOBAL_DATA, .mem_base = (u32_t)&msbc_dec2_global_bss[0], .mem_size = sizeof(msbc_dec2_global_bss),},
			{.mem_type = SBC_DECODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_uac_global_bss[0], .mem_size =sizeof(sbc_dec_uac_global_bss),},

#ifdef CONFIG_TWS_MONO_MODE
			{.mem_type = SBC_ENCODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc2_global_bss[0], .mem_size = sizeof(sbc_enc2_global_bss),},
#endif

			{.mem_type = DECODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_global_bss[0], .mem_size = sizeof(sbc_dec_global_bss),},
			{.mem_type = ENCODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc_global_bss[0], .mem_size = sizeof(sbc_enc_global_bss),},
		//#ifdef CONFIG_HFP_SPEECH
			//{.mem_type = AEC_GLOBAL_DATA, .mem_base = (u32_t)&hfp_aec_global_bss[0], .mem_size = sizeof(hfp_aec_global_bss),},
		//#ifndef CONFIG_OUTPUT_RESAMPLE
		//	{.mem_type = AEC_SHARE_DATA,  .mem_base = (u32_t)&playback_input_buffer[0x1780], .mem_size = 3082 * 2,},
		//#else
		//	{.mem_type = AEC_SHARE_DATA,  .mem_base = (u32_t)&hfp_aec_share_bss[0], .mem_size = sizeof(hfp_aec_share_bss),},
		//#endif
		//#endif
		#ifdef CONFIG_HFP_PLC
			{.mem_type = PLC_GLOBAL_DATA, .mem_base = (u32_t)&hfp_plc_global_bss[0], .mem_size = sizeof(hfp_plc_global_bss),},
			{.mem_type = PLC_SHARE_DATA,  .mem_base = (u32_t)&playback_input_buffer[0x1780], .mem_size = 1950 * 2,},

		#endif

		#ifdef CONFIG_TOOL_ASQT
			{.mem_type = TOOL_ASQT_STUB_BUF, .mem_base = (u32_t)&asqt_tool_stub_buf[0], .mem_size = sizeof(asqt_tool_stub_buf),},
			{.mem_type = TOOL_ASQT_DUMP_BUF, .mem_base = (u32_t)&playback_input_buffer[0x26bc], .mem_size = 3072,},
		#endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_RESAMPLE
		#ifdef CONFIG_OUTPUT_RESAMPLE
		{.mem_type = RESAMPLE_GLOBAL_DATA,  .mem_base = (u32_t)resample_global_bss, .mem_size = sizeof(resample_global_bss),},
		#endif
		{.mem_type = RESAMPLE_SHARE_DATA, .mem_base = (u32_t)resample_share_bss, .mem_size = sizeof(resample_share_bss),},
		{.mem_type = RESAMPLE_FRAME_DATA, .mem_base = (u32_t)resample_frame_bss, .mem_size = sizeof(resample_frame_bss),},
		{.mem_type = RESAMPLE_SHARE_DATA_APS, .mem_base = (u32_t)resample_aps_bss, .mem_size = sizeof(resample_aps_bss),},
		{.mem_type = RESAMPLE_FRAME_DATA_APS, .mem_base = (u32_t)resample_frame_aps_bss, .mem_size = sizeof(resample_frame_aps_bss),},
		#endif
		},
	},

	{
		.stream_type = AUDIO_STREAM_TTS,
		.mem_cell = {
			{.mem_type = TWS_LOCAL_INPUT, .mem_base = (u32_t)&playback_input_buffer[0], .mem_size = 0x1400,},
			{.mem_type = OUTPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x1400], .mem_size = 2048,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (u32_t)&playback_input_buffer[0x2c00], .mem_size = 0x0100,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (u32_t)&playback_input_buffer[0x2d00], .mem_size = 0x0100,},

			{.mem_type = SBC_DECODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_dec_global_bss[0], .mem_size = sizeof(sbc_dec_global_bss),},
			{.mem_type = SBC_ENCODER_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc_global_bss[0], .mem_size = sizeof(sbc_enc_global_bss),},
		#ifdef CONFIG_TWS_MONO_MODE
			{.mem_type = SBC_ENCODER_2_GLOBAL_DATA, .mem_base = (u32_t)&sbc_enc2_global_bss[0], .mem_size = sizeof(sbc_enc2_global_bss),},
		#endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_TOOL_ECTT
			{.mem_type = TOOL_ECTT_BUF, .mem_base = (u32_t)&ectt_tool_buf[0], .mem_size = sizeof(ectt_tool_buf),},
		#endif

		},
	},

	{
		.stream_type = AUDIO_STREAM_LOCAL_RECORD,
		.mem_cell = {
			{.mem_type = INPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x0000], .mem_size = 0x800,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (u32_t)&playback_input_buffer[0x0800], .mem_size = 0x2000,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (u32_t)&playback_input_buffer[0x2800], .mem_size = 0x800,},

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif
		},
	},

	{
		.stream_type = AUDIO_STREAM_GMA_RECORD,
		.mem_cell = {
			{.mem_type = INPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x0000], .mem_size = 0x800,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (u32_t)&playback_input_buffer[0x0800], .mem_size = 0x1200,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (u32_t)&playback_input_buffer[0x1A00], .mem_size = 0x600,},

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (u32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif
		},
	},

#ifdef CONFIG_RECORD_SERVICE
	{
		.stream_type = AUDIO_STREAM_BACKGROUND_RECORD,
		.mem_cell = {
			{.mem_type = INPUT_ENCBUF,   .mem_base = (u32_t)&wav_enc_adpcm_inbuf[0],  .mem_size = sizeof(wav_enc_adpcm_inbuf),},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (u32_t)&wav_enc_adpcm_outbuf[0], .mem_size = sizeof(wav_enc_adpcm_outbuf),},
		},
	},
#endif

	{
		.stream_type = AUDIO_STREAM_TWS,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (u32_t)&playback_input_buffer[0], .mem_size = sizeof(playback_input_buffer),},
		},
	},
};

static const struct media_memory_block *_memdia_mem_find_memory_block(int stream_type)
{
	const struct media_memory_block *mem_block = NULL;

	if (stream_type == AUDIO_STREAM_FM
		|| stream_type == AUDIO_STREAM_I2SRX_IN
		|| stream_type == AUDIO_STREAM_SPDIF_IN
		|| stream_type == AUDIO_STREAM_MIC_IN) {
		stream_type = AUDIO_STREAM_LINEIN;
	}

	for (int i = 0; i < ARRAY_SIZE(media_memory_config) ; i++) {
		mem_block = &media_memory_config[i];
		if (mem_block->stream_type == stream_type) {
			return mem_block;
		}
	}

	return NULL;
}

static const struct media_memory_cell *_memdia_mem_find_memory_cell(const struct media_memory_block *mem_block, int mem_type)
{
	const struct media_memory_cell *mem_cell = NULL;

	for (int i = 0; i < ARRAY_SIZE(mem_block->mem_cell) ; i++) {
		mem_cell = &mem_block->mem_cell[i];
		if (mem_cell->mem_type == mem_type) {
			return mem_cell;
		}
	}

	return NULL;
}

void *media_mem_get_cache_pool(int mem_type, int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	const struct media_memory_cell *mem_cell = NULL;
	void *addr = NULL;

	mem_block = _memdia_mem_find_memory_block(stream_type);

	if (!mem_block) {
		goto exit;
	}

	mem_cell = _memdia_mem_find_memory_cell(mem_block, mem_type);

	if (!mem_cell) {
		goto exit;
	}

	return (void *)mem_cell->mem_base;

exit:
	return addr;
}

int media_mem_get_cache_pool_size(int mem_type, int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	const struct media_memory_cell *mem_cell = NULL;
	int mem_size = 0;

	mem_block = _memdia_mem_find_memory_block(stream_type);

	if (!mem_block) {
		goto exit;
	}

	mem_cell = _memdia_mem_find_memory_cell(mem_block, mem_type);

	if (!mem_cell) {
		goto exit;
	}

	return mem_cell->mem_size;

exit:
	return mem_size;
}
#else
void *media_mem_get_cache_pool(int mem_type, int stream_type)
{
	return NULL;
}
int media_mem_get_cache_pool_size(int mem_type, int stream_type)
{
	return 0;
}
#endif
