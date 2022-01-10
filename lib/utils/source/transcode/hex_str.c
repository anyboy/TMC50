/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hex string trancode  interface
 */
#include <ctype.h>
#include "hex_str.h"

void hex_to_str(char *pszDest, char *pbSrc, int nLen)
{
	char ddl, ddh;
	for (int i = 0; i < nLen; i++) {
		ddh = 48 + ((unsigned char)pbSrc[i]) / 16;
		ddl = 48 + ((unsigned char)pbSrc[i]) % 16;

		if (ddh > 57) ddh = ddh + 7;
		if (ddl > 57) ddl = ddl + 7;

		pszDest[i * 2] = ddh;
		pszDest[i * 2 + 1] = ddl;
	}
	pszDest[nLen * 2] = '\0';
}

void str_to_hex(char *pbDest, char *pszSrc, int nLen)
{
	char h1, h2;
	char s1, s2;
	for (int i = 0; i < nLen; i++) {
		h1 = pszSrc[2 * i];
		h2 = pszSrc[2 * i + 1];

		s1 = toupper(h1) - 0x30;
		if (s1 > 9)	s1 -= 7;

		s2 = toupper(h2) - 0x30;
		if (s2 > 9)	s2 -= 7;

		pbDest[i] = ((s1 << 4) | s2);
	}
}
