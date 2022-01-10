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

static int _usp_get_recv_seq_no(usp_handle_t *usp_handle)
{
	return usp_handle->parser.seq_no;
}

static USP_PROTOCOL_STATUS ReceivingAck(usp_handle_t *usp_handle)
{
    USP_PROTOCOL_STATUS ret;

	if(UspWaitACK(usp_handle) == 0){
		ret = USP_PROTOCOL_OK;
	}else{
		ret = USP_PROTOCOL_DATA_CHECK_FAIL;
	}

    return ret;
}

static void usp_sending_hw_lock(usp_handle_t *usp_handle)
{
	if (usp_handle->hw_mutex) {
	  usp_handle->os_api->sem_wait(usp_handle->hw_mutex);
	}
}

static void usp_sending_hw_unlock(usp_handle_t *usp_handle)
{
	if (usp_handle->hw_mutex) {
	  usp_handle->os_api->sem_post(usp_handle->hw_mutex);
	}
}

static void usp_sending_app_lock(usp_handle_t *usp_handle)
{
	if (usp_handle->app_mutex) {
		usp_handle->os_api->sem_wait(usp_handle->app_mutex);
	}
}

static void usp_sending_app_unlock(usp_handle_t *usp_handle)
{
	if (usp_handle->app_mutex) {
		usp_handle->os_api->sem_post(usp_handle->app_mutex);
	}
}

static int UspGetSendingSeq(usp_handle_t *usp_handle)
{
	usp_sending_app_lock(usp_handle);
	int seq = usp_handle->usp_protocol_global_data.out_seq_number;
	usp_handle->usp_protocol_global_data.out_seq_number++;
	usp_sending_app_unlock(usp_handle);
	return seq;
}

/**
  \brief       send packet head and payload to peer USP device.
  \return      Sending payload bytes num.
*/
static void SendingPacketFD(usp_handle_t *usp_handle, u8_t *payload, u32_t size, USP_PACKET_TYPE_E type, u8_t command, u8_t seq)
{
    u16_t crc16_val = 0;
    u32_t payload_bytes;
    usp_packet_head_t head;

    memset(&head, 0, USP_PACKET_HEAD_SIZE);
    if (NULL == payload)
    {
        payload_bytes = 0;
    }
    else
    {
        payload_bytes = size;
    }

    head.type               = type;
    head.predefine_command  = command;

	if (USP_PACKET_TYPE_RSP != type){
		head.protocol_type  = usp_handle->usp_protocol_global_data.protocol_type;
        head.sequence_number= seq;
        head.payload_len    = payload_bytes;
	    if (command){
	        head.protocol_type  = USP_PROTOCOL_TYPE_FUNDAMENTAL;
	    }
    }else{
        head.protocol_type  = 0x00;
        head.sequence_number= seq;
        head.payload_len    = 0;
	}

    head.crc8_val           = crc8((u8_t*)&head, USP_PACKET_HEAD_SIZE - 1);

    //如无payload,无需再发送CRC16码
    if (0 != payload_bytes)
    {
        //packet head与payload间增加计算时间,留够时间给PC处理
        //计算1KByte CRC16耗时1.03ms(CPU=26M)
        crc16_val = crc16(payload, payload_bytes);
    }

    usp_sending_hw_lock(usp_handle);

	if (USP_PACKET_TYPE_DATA == type) {
    	UspClrAckSyncFlag(usp_handle);
    }

	//print_buffer((u8_t*)&head, 1, USP_PACKET_HEAD_SIZE, 16, -1);

    //sending packet head
    usp_handle->api.write((u8_t*)&head, USP_PACKET_HEAD_SIZE, 0);

    //如无payload,无需再发送CRC16码
    if (0 != payload_bytes)
    {
        //sending CRC16
        usp_handle->api.write((u8_t*)&crc16_val, 2, 0);
        //sending payload data
        usp_handle->api.write(payload, payload_bytes, 0);
    }

    //wait write finish
	UspCheckwriteFinish(usp_handle);

    usp_sending_hw_unlock(usp_handle);
}

/**
  \brief       send 1 data packet(witch ACK) to peer USP device, maybe send predefined command(no payload).
  \return      if successfully send data to peer device.
                 - == 0:    ACK received
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
static USP_PROTOCOL_STATUS SendUSPDataRspSem(usp_handle_t *usp_handle, u8_t command, u8_t type, u8_t seq)
{
    SendingPacketFD(usp_handle, NULL, 0, type, command, seq);

    return 0;
}

/**
  \brief       sending ACK packet to peer USP device.
*/
void SendingResponseSem(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS status)
{
    SendUSPDataRspSem(usp_handle, status, USP_PACKET_TYPE_RSP, _usp_get_recv_seq_no(usp_handle));
}

void UspSendNakFrame(usp_handle_t *usp_handle, int seq)
{
	SendUSPDataRspSem(usp_handle, USP_PROTOCOL_ERR, USP_PACKET_TYPE_RSP, seq);
}

void UspSendAckFrame(usp_handle_t *usp_handle, int seq)
{
	SendUSPDataRspSem(usp_handle, USP_PROTOCOL_OK, USP_PACKET_TYPE_RSP, seq);
}


int WriteUSPDataBasic(usp_handle_t *usp_handle, u8_t *payload, u32_t size, u8_t command)
{
    int status = USP_PROTOCOL_DATA_CHECK_FAIL, retry_count = 0;

	int seq = UspGetSendingSeq(usp_handle);

    do
    {
    	usp_sending_app_lock(usp_handle);

        //接收方明确告知数据校验fail才重传
        if (USP_PROTOCOL_DATA_CHECK_FAIL == status)
        {
            SendingPacketFD(usp_handle, payload, size, USP_PACKET_TYPE_DATA, command, seq);
        }

		status = ReceivingAck(usp_handle);

        if (USP_PROTOCOL_OK == status)
        {
			usp_sending_app_unlock(usp_handle);
            break;
        }

        //only payload check fail need to retry
        if (USP_PROTOCOL_DATA_CHECK_FAIL != status)
        {
			usp_sending_app_unlock(usp_handle);
            USP_DEBUG_PRINT("out err：%d.\n", status);
            break;
        }

		usp_sending_app_unlock(usp_handle);

        retry_count++;
    } while (retry_count < usp_handle->usp_protocol_global_data.max_retry_count);

    return status;
}

/**
  \brief       send 1 data packet(witch ACK) to peer USP device, maybe send predefined command(no payload).
  \return      if successfully send data to peer device.
                 - == 0:    ACK received
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
int WriteUSPDataFD(usp_handle_t *usp_handle, u8_t *payload, u32_t size)
{
	return WriteUSPDataBasic(usp_handle, payload, size, 0);
}

int WriteISODataFD(usp_handle_t *usp_handle, u8_t *payload, u32_t size)
{
	int seq = UspGetSendingSeq(usp_handle);

	usp_sending_app_lock(usp_handle);

    SendingPacketFD(usp_handle, payload, size, USP_PACKET_TYPE_ISO, 0, seq);

	usp_sending_app_unlock(usp_handle);

    return 0;
}

#if 0
void UspMasterConnectReq(usp_handle_t *usp_handle)
{
	int connect_magic = USP_PC_CONNECT_MAGIC;

	if(!UspIsHardwareConnected(usp_handle)){

		usp_sending_hw_lock(usp_handle);

		//sending packet head
		usp_handle->api.write((u8_t*)&connect_magic, sizeof(connect_magic), 0);

		//wait write finish
		UspCheckwriteFinish(usp_handle);

    	usp_sending_hw_unlock(usp_handle);

		usp_handle->os_api->msleep(10);
	}
}
#endif

int UspChangeBaud(usp_handle_t *usp_handle, u32_t baud, u32_t delaytime)
{
    USP_PROTOCOL_STATUS ret;

	u32_t payload_data[2];

	payload_data[0] = baud;
	payload_data[1] = delaytime;

    ret = WriteUSPDataBasic(usp_handle, (u8_t*)&payload_data, sizeof(payload_data), USP_SET_BAUDRATE);

	if(ret == USP_PROTOCOL_OK){
    	usp_handle->api.ioctl(USP_IOCTL_SET_BAUDRATE, (void*)*((u32_t*)baud), NULL);
		usp_handle->os_api->msleep(delaytime);
	}

    return ret;
}

static int UspSendConnectCmd(usp_handle_t *usp_handle)
{
    USP_PROTOCOL_STATUS ret;

    ret = WriteUSPDataBasic(usp_handle, (u8_t*)NULL, 0, USP_CONNECT);

    return ret;
}

void ReportUSPProtocolInfoFD(usp_handle_t *usp_handle)
{
    WriteUSPDataBasic(usp_handle, (u8_t*)&usp_protocol_info, sizeof(usp_protocol_info_t), USP_REPORT_PROTOCOL_INFO);
}



bool ConnectUSPFD(usp_handle_t *usp_handle)
{
    if (usp_handle->usp_protocol_global_data.connected)
    {
        return TRUE;
    }

    if (USP_PROTOCOL_OK == UspSendConnectCmd(usp_handle))
    {
        SetUSPConnectionFlag(usp_handle);
        ReportUSPProtocolInfoFD(usp_handle);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void UspStartConnectRsp(usp_handle_t *usp_handle)
{
    ConnectUSPFD(usp_handle);
    ConnectUSPFD(usp_handle);
    ConnectUSPFD(usp_handle);
}

USP_PROTOCOL_STATUS OpenUSPFD(usp_handle_t *usp_handle, u8_t protocol_type)
{
	int i;
    USP_PROTOCOL_STATUS ret;
    u32_t open_para = protocol_type;

    ret = WriteUSPDataBasic(usp_handle, (u8_t*)&open_para, sizeof(open_para), USP_OPEN_PROTOCOL);

    if (USP_PROTOCOL_OK == ret)
    {
        usp_handle->usp_protocol_global_data.opened = 1;
		usp_handle->usp_protocol_global_data.protocol_type = protocol_type;
		for(i = 0; i < USP_MAX_PROTOCOL_HANDLER; i++){
			if(usp_handle->protocol_type[i] == 0xff){
				usp_handle->protocol_type[i] = protocol_type;
				break;
			}
		}
    }

    return ret;
}


