#ifndef _ASDP_TRANSFER_H_
#define _ASDP_TRANSFER_H_

#include <usp_protocol.h>

enum DM_BT_PLAY_STATUS
{
	DM_BT_STATUS_NONE                  = 0x0001,    // <"空状态">
	DM_BT_STATUS_WAIT_PAIR             = 0x0002,    // <"蓝牙初始化完成，可搜索可连接">

	DM_BT_STATUS_MASTER_WAIT_PAIR      = 0x0004,    // <"主箱等待手机连接">
	DM_BT_STATUS_PAUSED                = 0x0008,    // <"蓝牙连接成功">
	DM_BT_STATUS_PLAYING               = 0x0010,    // <"蓝牙播放状态">
	DM_BT_STATUS_INCOMING              = 0x0020,    // <"蓝牙来电状态">
	DM_BT_STATUS_OUTGOING              = 0x0040,    // <"蓝牙去电状态">
	DM_BT_STATUS_ONGOING               = 0x0080,    // <"蓝牙通话状态">
	DM_BT_STATUS_MULTIPARTY            = 0x0100,    // <"蓝牙三方通话状态">

	DM_BT_STATUS_SIRI                  = 0x0200,    // <"蓝牙语音识别状态">
	DM_BT_STATUS_3WAYIN                = 0x0400,    // <"蓝牙三方来电状态">
};

int asdp_transfer_init(usp_handle_t* usp);
char *adsp_get_tlv_buf(void);
int adsp_get_tlv_buf_len(void);
int asdp_send_cmd(u8_t srv, u8_t cmd, u16_t len);
void asdp_dm_update_media_status(bool ready , int stream_type);

#endif

