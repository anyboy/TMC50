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
 * \file     usp_connect.c
 * \brief    usp protocol
 * \author   zhouxl
 * \par      GENERAL DESCRIPTION:
 *               function related to usp
 * \par      EXTERNALIZED FUNCTIONS:
 *
 * \version 1.0
 * \date  2017-4-7
*******************************************************************************/

#include "usp_protocol_inner.h"

void SetUSPConnectionFlag(usp_handle_t *usp_handle)
{
    usp_handle->usp_protocol_global_data.connected          = 1;
    usp_handle->usp_protocol_global_data.max_retry_count    = USP_PROTOCOL_MAX_RETRY_COUNT;
    usp_handle->usp_protocol_global_data.rx_timeout         = USP_PROTOCOL_RX_TIMEOUT;
}

void SetUSPDisConnectionFlag(usp_handle_t *usp_handle)
{
    usp_handle->usp_protocol_global_data.connected          = 0;
}

bool ConnectUSP(usp_handle_t *usp_handle)
{
    int ret;

    if (usp_handle->usp_protocol_global_data.connected)
    {
        return TRUE;
    }

    if (USP_PROTOCOL_OK == SendUSPDataPacket(usp_handle, (u8_t*)&ret, 0, USP_CONNECT))
    {
        SendingResponse(usp_handle, USP_PROTOCOL_OK);
        SetUSPConnectionFlag(usp_handle);
        ReportUSPProtocolInfo(usp_handle);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}




bool Check_ConnectUSP(usp_handle_t *usp_handle, u32_t timeout_ms)
{
    u32_t connect_magic;

    if (usp_handle->usp_protocol_global_data.connected)
    {
        return TRUE;
    }

    while (timeout_ms != 0)
    {
        if (usp_handle->api.read((u8_t*)&connect_magic, sizeof(connect_magic), 1) == 4)
        {
            if (USP_PC_CONNECT_MAGIC == connect_magic)
            {
                DiscardReceivedData(usp_handle);
                // 第一次用于通知协议类型,不成功后再 try 两次
                // 成功后再 try 不会浪费时间
                ConnectUSP(usp_handle);
                ConnectUSP(usp_handle);
                ConnectUSP(usp_handle);
                break;
            }
        }

        timeout_ms--;
    }

    if (usp_handle->usp_protocol_global_data.connected)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


bool WaitUSPConnect(usp_handle_t *usp_handle, u32_t timeout_ms)
{
    usp_packet_head_t head;

    do
    {
        if ((USP_PROTOCOL_OK == ReceivingPacketHead(usp_handle, &head)) &&
            (USP_PACKET_TYPE_DATA           == head.type)               &&
            (USP_PROTOCOL_TYPE_FUNDAMENTAL  == head.protocol_type))
        {
            ParseUSPCommand(usp_handle, head.predefine_command, head.payload_len);

            if (usp_handle->usp_protocol_global_data.connected)
            {
                return TRUE;
            }
        }

        if (timeout_ms < usp_handle->usp_protocol_global_data.rx_timeout)
        {
            timeout_ms = 0;
        }
        else
        {
            timeout_ms -= usp_handle->usp_protocol_global_data.rx_timeout;
        }
    } while (0 != timeout_ms);

    return FALSE;
}



void ReportUSPProtocolInfo(usp_handle_t *usp_handle)
{
    SendUSPDataPacket(usp_handle, (u8_t*)&usp_protocol_info, sizeof(usp_protocol_info_t), USP_REPORT_PROTOCOL_INFO);
}



