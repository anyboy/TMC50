/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hex to string interface
 */
#ifndef __TRANSCODE_H__
#define __TRANSCODE_H__

//http://www.iana.org/assignments/character-sets/character-sets.xhtml
#define CHARACTER_SETS_UTF8        106
#define CHARACTER_SETS_GBK         113
#define CHARACTER_SETS_UNICODE     1010

int gbk_to_utf8(unsigned char *lpGBKStr, unsigned char *lpUTF8Str, int len);

int gbk_to_unicode(unsigned char *lpGBKStr, unsigned short *unicode, int *punicode_len);

int utf8_to_gbk(unsigned char *lpUTF8Str,unsigned char *lpGBKStr, int len);

int utf8_to_unicode(unsigned char *utf8, int utf8_len,
					 unsigned short *unicode, int *punicode_len);

int unicode_to_utf8(unsigned short *unicode, int unicode_len,
					unsigned char *utf8, int *putf8_len);

int mbcs_to_unicode(unsigned char *src_mbcs, unsigned short *dst_uni,
					 int src_len, int *dst_len, int nation_id);

int unicod_to_mbcs(unsigned short *src_ucs, unsigned char *dst_mbcs,
                          int src_len, int *dst_len, int nation_id);
#endif