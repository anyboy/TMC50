/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief loop play app fun, get file head&tail length
 */

#include "loop_player.h"


#define MP3_TAG_ID3V1_LENGTH	(128)

static int _lp_get_id3v1_length(io_stream_t loop_fstream)
{	
	char id3v1_head[5];
	int offset_backup;

	/* id3v1 tag locates at file tail, with a fixed-size */
	offset_backup = stream_tell(loop_fstream);
	stream_seek(loop_fstream, -MP3_TAG_ID3V1_LENGTH, SEEK_DIR_END);

	stream_read(loop_fstream, id3v1_head, sizeof(id3v1_head));
	stream_seek(loop_fstream, offset_backup, SEEK_DIR_BEG);
	/* id3v1 tag starts with string "TAG" */
	if (0 == strncmp(id3v1_head, "TAG", strlen("TAG")))
		return MP3_TAG_ID3V1_LENGTH;
	else
		return 0;
}

/* line 53 might be a problem, for haven't tested yet */
/* apev2 is not common */
static int _lp_get_apev2_length(io_stream_t loop_fstream, u32_t offset)
{
#if 0
	int offset_backup;
	u32_t apev2_len = 0
	char apev2_tag[32];

	/* get apev2 tag size from its tag-head, which is 32bytes */
	offset_backup = stream_tell(loop_fstream);
	stream_seek(loop_fstream, -(offset + 32), SEEK_DIR_END);

	stream_read(loop_fstream, apev2_tag, sizeof(apev2_tag));
	stream_seek(loop_fstream, offset_backup, SEEK_DIR_BEG);

	/* apev2 tag starts with string "APETAGEX" */
	if (0 == strncmp(apev2_tag, "APETAGEX", strlen("APETAGEX"))) {
		apev2_len = apev2_tag[12] | (apev2_tag[13] << 8) | (apev2_tag[14] << 16) | (apev2_tag[15] << 24);//fix
		apev2_len += 32;
	}

	return apev2_len;
#endif
	return 0;
}

/* file tail length is length of tag id3v1 and tag apev2, if have */
static void loop_get_file_tail_length(io_stream_t loop_fstream, u32_t *file_tail_len)
{
	u32_t id3v1_length = 0, apev2_length = 0;

	id3v1_length = _lp_get_id3v1_length(loop_fstream);

	/* apev2 locates at file tail, followed by id3v1 */
	apev2_length = _lp_get_apev2_length(loop_fstream, id3v1_length);

	*file_tail_len = id3v1_length + apev2_length;
}

static void loop_update_file_info(struct local_player_t *localplayer, struct loopfs_file_info *lps)
{
	int res = 0;
	media_info_t info;

	res = mplayer_update_media_info(localplayer, &info);
	if (res == 0) {
		lps->head = info.file_hdr_len;
		lps->bitrate = info.avg_bitrate;
	} else {
		lps->head = 0;
		lps->bitrate = 0;
	}
}

void loopplay_set_stream_info(struct loop_play_app_t *lpplayer, void *info, u8_t info_id)
{
	if (info_id == LOOPFS_FILE_INFO) {
		struct loopfs_file_info lps_info;
		loop_get_file_tail_length(lpplayer->localplayer->file_stream, &lps_info.tail);
		loop_update_file_info(lpplayer->localplayer, &lps_info);
		loop_fstream_get_info(&lps_info, info_id);
	} else {
		loop_fstream_get_info(info, info_id);
	}
}
