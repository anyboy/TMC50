/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief local player breakpoint.
 */

#include "record.h"
#include <property_manager.h>

void recplayer_bp_info_save(const char *dir, struct recplayer_bp_info_t recplayer_bp_info)
{
	int ret = 0;

	SYS_LOG_INF("bp time:%d ms,file:%d bytes\n", recplayer_bp_info.bp_info.time_offset, recplayer_bp_info.bp_info.file_offset);

#ifdef CONFIG_PROPERTY
	ret = property_set("RECORDER_BP_INFO", (char *)&recplayer_bp_info, sizeof(struct recplayer_bp_info_t));
#endif
	if (ret < 0) {
		SYS_LOG_ERR("failed to set BP, ret %d\n", ret);
	}
}

int recplayer_bp_info_resume(const char *dir, struct recplayer_bp_info_t *recplayer_bp_info)
{
	int res = -EINVAL;

#ifdef CONFIG_PROPERTY
	res = property_get("RECORDER_BP_INFO", (char *)recplayer_bp_info, sizeof(struct recplayer_bp_info_t));
#endif
	SYS_LOG_INF("bp:time_offset=%d,file_offset=%d,file_path:%s\n",
	recplayer_bp_info->bp_info.time_offset, recplayer_bp_info->bp_info.file_offset, recplayer_bp_info->file_path);

	return res;
}
