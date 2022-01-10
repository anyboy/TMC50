/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file down load service interface
 */

#ifndef __DOWNLOAD_SERVICE_H__
#define __DOWNLOAD_SERVICE_H__

/**
 * @defgroup download_service_apis DownLoad Service APIs
 * @ingroup mem_managers
 * @{
 */

 typedef enum
{
	DOWN_RET_ERROR = -1,
	DOWN_RET_COMPLETE //0,
}download_ret;

typedef enum
{
	DOWN_NULL,
	DOWN_TRACKPLAY,
	DOWN_TRACKLIST,
	DOWN_TASK,
}download_type;


typedef struct
{
	int downlistid;
	int downtrackid;
	uint8_t format;
	bool seek_down;
	uint16_t downtrackindex;

}downtrackinfo_t;

typedef struct
{
	download_type downtype;

	io_stream_t source_stream;
	io_stream_t dest_stream;

	union
	{
		downtrackinfo_t downinfo;
		int(*task_func)(bool* run_flag);
	};


}download_parm_t;

typedef int (*TASK_FUNC)(bool*);

int download_stop(download_type downtype,bool sync_stop);

int download_start(download_parm_t parm,MSG_CALLBAK callback,bool sync_down);

void download_add_task(MSG_CALLBAK callback,TASK_FUNC task_func);

int download_service_stop(void);

bool check_down_flag();

download_type get_cur_down_type();

downtrackinfo_t* get_cur_down_trackinfo();

/**
 * @} end defgroup download_service_apis
 */
#endif
