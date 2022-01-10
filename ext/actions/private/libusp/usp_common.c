/*******************************************************************************
 *                                      US283C
 *                            Module: usp Driver
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       zhouxl    2017-4-7  11:39     1.0             build this file
*******************************************************************************/
/*!
 * \file     usp_transfer.c
 * \brief    USP data transfer
 * \author   zhouxl
 * \par      GENERAL DESCRIPTION:
 *               function related to usp protocol
 * \par      EXTERNALIZED FUNCTIONS:
 *
 * \version 1.0
 * \date  2017-4-7
*******************************************************************************/
#include "usp_protocol_inner.h"

static int _is_sem_hook_registered(usp_handle_t *usp_handle) {
  return usp_handle->usp_protocol_global_data.sem_hooks_registered;
}

static int usp_wait_sleep(usp_handle_t *usp_handle, int sleep_msec) {
  	if (_is_sem_hook_registered(usp_handle)) {
    	return usp_handle->os_api->sem_timedwait(usp_handle->sync_lock, sleep_msec);
  	}

  	usp_handle->sync_lock = NULL;
	while (sleep_msec > 0) {
	    if (usp_handle->sync_lock != NULL){
			break;
		}
	    usp_handle->os_api->msleep(2);
	    sleep_msec -= 2;
	}
  	return 0;
}

int UspWaitACK(usp_handle_t *usp_handle) {
	int wait_cnt = 0;
wait_again:
	//usp_handle->parser.wait_ack = true;
  	usp_wait_sleep(usp_handle, USP_WAIT_ACK_TIMEOUT_MSEC);
	//usp_handle->parser.wait_ack = false;
	SYS_LOG_DBG("ack flag %d", usp_handle->usp_protocol_global_data.acked);

	if(usp_handle->usp_protocol_global_data.acked == 1){
		/*yes, this is a ack*/
		return 0;
	}else if(usp_handle->usp_protocol_global_data.acked == 2){
		/*no, this is a busy mask */
		wait_cnt++;
		if(wait_cnt < 10){
			goto wait_again;
		}
	}

  	return -1;
}

int UspWakeSendThread(usp_handle_t *usp_handle) {
  	if (_is_sem_hook_registered(usp_handle)) {
    	return usp_handle->os_api->sem_post(usp_handle->sync_lock);
  	}

  	usp_handle->sync_lock = (void *)0x1;
  	return 0;
}

int USPIsFullDuplex(usp_handle_t *usp_handle)
{
	return (usp_handle->usp_protocol_global_data.work_mode == USP_WORKMODE_FULL_DUPLEX);
}

int UspIsHardwareConnected(usp_handle_t *usp_handle)
{
	return usp_handle->usp_protocol_global_data.connected;
}

int UspGetDropFrameCnt(usp_handle_t *usp_handle)
{
	return usp_handle->parser.err_cnt;
}

void UspClrAckSyncFlag(usp_handle_t *usp_handle) {
  	usp_handle->usp_protocol_global_data.acked = 0;
}

void UspSetAckSyncFlag(usp_handle_t *usp_handle) {
  	usp_handle->usp_protocol_global_data.acked = 1;
}

void UspSetAckSyncBusyFlag(usp_handle_t *usp_handle) {
  	usp_handle->usp_protocol_global_data.acked = 2;
}

u8_t* UspGetDataPayload(usp_handle_t *usp_handle, u8_t *data_buffer) {
  	return (u8_t *)(data_buffer + USP_PACKET_HEAD_SIZE + 2);
}

void UspNotifyUpperLayerFrameData(usp_handle_t *usp_handle, u8_t *data_buffer)
{
	usp_data_frame_t data;
	usp_data_pakcet_t data_packet;

	memcpy(&data_packet, data_buffer, sizeof(usp_data_pakcet_t));

	data.type = data_packet.head.type;
	data.protocol_type = data_packet.head.protocol_type;
	data.predefine_command = data_packet.head.predefine_command;

	if(data_packet.head.payload_len){
		data.payload_len = data_packet.head.payload_len;
		data.payload = UspGetDataPayload(usp_handle, data_buffer);
	}else{
		data.payload = NULL;
		data.payload_len = 0;
	}

	usp_handle->recv_frame_handler(&data);

}

void UspCheckwriteFinish(usp_handle_t *usp_handle)
{
	int ret;
	while(1){
		ret = usp_handle->api.ioctl(USP_IOCTL_CHECK_WRITE_FINISH, NULL, NULL);

		if(ret == true){
			break;
		}
	}
}

void UspSysReboot(usp_handle_t *usp_handle, u32_t delaytime_ms)
{
	if(usp_handle->os_api){
		usp_handle->os_api->msleep(delaytime_ms);
	}
	usp_handle->api.ioctl(USP_IOCTL_REBOOT, NULL, NULL);
}

