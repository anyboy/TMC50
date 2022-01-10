/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hex to string interface
 */
#ifndef __HEX_STR_H__
#define __HEX_STR_H__

void hex_to_str(char *pszDest, char *pbSrc, int nLen);

void str_to_hex(char *pbDest, char *pszSrc, int nLen);

#endif