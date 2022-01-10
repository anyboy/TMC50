/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file oem interface for Actions SoC
 */

#ifndef	_OEM_INTERFACE_H_
#define	_OEM_INTERFACE_H_

/**
 * @brief Read SoC chip ID.
 *
 * This routine performs read uuid . OEM vender can use this uuid
 * to distinguish device.
 *
 * @param uuid Pointer to the memory buffer to put uuid data .
 * @param size number of bytes to read.
 *
 * @return 0 on success, negative errno code on fail
 */
int get_device_uuid(char *uuid, int size);

/**
 * @brief Read SoC chip ID.
 *
 * This routine performs read SoC chipid. OEM vender can use this chipid
 * to distinguish SoC type.
 *
 * @param value Pointer to interge addr to put chip id.
 *
 * @return 0 on success, negative errno code on fail
 */
int get_XY_coordinate(int *value);

/**
 * @brief get wifi mac addr
 *
 * This routine performs read wifi mac addr ,OEM vender can use this mac addr 
 * to distinguish wifi module.
 *
 * @param mac Pointer to interge addr to put chip id.
 * @param size number of bytes to read.
 *
 * @return 0 on success, negative errno code on fail
 */

int get_wifi_mac_addr(char *mac, int size);

#endif /* _OEM_INTERFACE_H_	*/  
