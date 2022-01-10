/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app define
 */

#ifndef _APP_DEFINES_H
#define _APP_DEFINES_H

#define APP_ID_MAIN				"main"
#define APP_ID_BTMUSIC			"btmusic"
#define APP_ID_BTCALL			"btcall"

#define APP_ID_TWS				"tws"
#define APP_ID_DMAAPP			"dma_app"

#define APP_ID_OTA				"ota"
#define APP_ID_SD_MPLAYER		"sd_mplayer"
#define APP_ID_UHOST_MPLAYER	"uhost_mplayer"
#define APP_ID_NOR_MPLAYER		"nor_mplayer"
#define APP_ID_LOOP_PLAYER		"loop_player"
#define APP_ID_LINEIN			"linein"
#define APP_ID_SPDIF_IN			"spdif_in"
#define APP_ID_I2SRX_IN			"i2srx_in"
#define APP_ID_MIC_IN			"mic_in"
#define APP_ID_RECORDER			"recorder"
#define APP_ID_USOUND			"usound"
#define APP_ID_ALARM			"alarm"
#define APP_ID_FM				"fm"
#define APP_ID_AIS_APP		    "gma_app"
#define APP_ID_ATT				"att"
#define APP_ID_OTA				"ota"

#define APP_ID_DEFAULT		APP_ID_BTMUSIC
/*
 * app id switch list
 */
#define app_id_list {\
						APP_ID_BTMUSIC,\
						APP_ID_UHOST_MPLAYER,\
					 	APP_ID_SD_MPLAYER,\
						APP_ID_NOR_MPLAYER,\
						APP_ID_LOOP_PLAYER,\
						APP_ID_RECORDER,\
						APP_ID_LINEIN,\
						APP_ID_USOUND,\
						APP_ID_SPDIF_IN,\
						APP_ID_I2SRX_IN,\
						APP_ID_MIC_IN,\
						APP_ID_TWS,\
						APP_ID_BTCALL,\
						APP_ID_ALARM,\
						APP_ID_AIS_APP,\
						APP_ID_FM,\
						APP_ID_ATT,\
						APP_ID_OTA,\
					}

#endif  // _APP_DEFINES_H


