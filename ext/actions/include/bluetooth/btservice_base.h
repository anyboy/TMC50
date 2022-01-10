/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt service base define.
 * Not need direct include this head file, just need include <btservice_api.h>.
 * this head file include by btservice_api.h
 */

#ifndef _BTSERVICE_BASE_H_
#define _BTSERVICE_BASE_H_

/** bluetooth device address */
typedef struct {
	u8_t  val[6];
} bd_address_t;

#endif
