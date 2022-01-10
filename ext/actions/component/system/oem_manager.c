/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file oem interface for Actions SoC
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "oem_manager"
#include <logging/sys_log.h>

#include <os_common_api.h>
#include <wifi_manager.h>
#include <mem_manager.h>
#include <string.h>
#include <msg_manager.h>
#include <oem_interface.h>
#include <property_manager.h>

int get_device_uuid(char *uuid, int size)
{
	int res = -ENODEV;
	char *id_end = NULL;
#ifdef CONFIG_NVRAM_CONFIG
	if (property_get(CFG_SN_INFO, uuid, size) > 0) {
		id_end = strrchr((const char *)uuid, '_');
		if (id_end != NULL) {
			id_end[0] = '\0';
		}
		SYS_LOG_DBG("CFG_SN_INFO =%s", uuid);
		res = 0;
	}
#endif
	return res;
}

#define OEM_CHIP_ID_KEY            0xaee71001
#define OEM_CHIP_ID_ATS2819V           0x8178bc4e

/**
 * @brief Read SoC chip ID.
 *
 * This routine performs read SoC chipid. OEM vender can use this chipid
 * to distinguish SoC type.
 *
 * @param key OEM ID Key.
 * @param buf Pointer to the memory buffer to put chip ID data. .
 * @param len Number of bytes to read .
 *
 * @return 0 on success, negative errno code on fail
 */
extern int oem_get_chipid(unsigned int key, unsigned char *buf, int len);

int get_XY_coordinate(int *value)
{
	return oem_get_chipid(OEM_CHIP_ID_KEY, (unsigned char *)value, sizeof(int));
}

#define MIN_MAC_ADDR_LEN 6
int get_wifi_mac_addr(char *mac, int size)
{
	int res = 0;

	if (size < MIN_MAC_ADDR_LEN) {
		res = -EINVAL;
		goto exit;
	}

	if (wifi_mgr_get_mac_addr(mac)) {
		res = -ENOENT;
	}

exit:
	return res;
}

