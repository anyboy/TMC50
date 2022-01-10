/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief record service.h
 */

#ifndef _RECORDER_SERVICE_H
#define _RECORDER_SERVICE_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "record_service"

#include <logging/sys_log.h>
#include <mem_manager.h>
#include <srv_manager.h>
#include <msg_manager.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <file_stream.h>
#include <thread_timer.h>

#define RECORD_DIR_LEN (10)
#define RECORD_FILE_PATH_LEN	(24)
#define MAX_RECORD_FILES (999)

struct recorder_service_t {
	struct thread_timer sync_data_timer;
	char dir[RECORD_DIR_LEN + 1];
	char file_path[RECORD_FILE_PATH_LEN];
	u32_t file_bitmaps[MAX_RECORD_FILES / 32 + 1];
	u32_t recording : 1;
	u32_t disk_space_less : 1;
	u32_t file_is_full : 1;
	u32_t file_count : 16;
	u32_t read_cache_times : 13;
	u32_t disk_space_size;/*M byte*/

	io_stream_t output_stream;
	io_stream_t cache_stream;
};
struct recorder_service_t *recorder_get_service(void);

#endif  /* _RECORDER_SERVICE_H */

