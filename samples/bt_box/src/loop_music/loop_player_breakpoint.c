/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief loop player breakpoint.
 */

#include "loop_player.h"
#include <property_manager.h>

void _loopplayer_bpinfo_save(char dir[], u16_t track_no)
{
	int ret = 0;
	char bp_tag[12] = {0};

	SYS_LOG_INF("track_no: %d\n", track_no);

	if (strstr(dir, "SD:")) {
		strcpy(bp_tag, "SD_LOOP_BP");
	} else if (strstr(dir, "USB:")) {
		strcpy(bp_tag, "USB_LOOP_BP");
	} else {
		strcpy(bp_tag, "NOR_LOOP_BP");
	}
#ifdef CONFIG_PROPERTY
	ret = property_set(bp_tag, (char *)&track_no, 2);
#endif
	if (ret < 0) {
		SYS_LOG_ERR("failed to set BPINFO %s, ret %d\n", bp_tag, ret);
	}
}

int _loopplayer_bpinfo_resume(char dir[], u16_t *track_no)
{
	int res = -EINVAL;
	char bp_tag[12] = {0};

	if (strstr(dir, "SD:")) {
		strcpy(bp_tag, "SD_LOOP_BP");
	} else if (strstr(dir, "USB:")) {
		strcpy(bp_tag, "USB_LOOP_BP");
	} else {
		strcpy(bp_tag, "NOR_LOOP_BP");
	}
#ifdef CONFIG_PROPERTY
	res = property_get(bp_tag, (char *)track_no, 2);
#endif
	SYS_LOG_INF("track_no: %d\n", *track_no);

	return res;
}
