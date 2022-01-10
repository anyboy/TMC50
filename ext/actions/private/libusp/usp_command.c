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
 * \file     usp_command.c
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



USP_PROTOCOL_STATUS ParseUSPCommand(usp_handle_t *usp_handle, u8_t usp_cmd, u32_t para_length)
{
    u8_t para[32];
    int ret = USP_PROTOCOL_RX_ERR;
    u32_t size;

    if (para_length > ARRAY_SIZE(para))
    {
        size = ARRAY_SIZE(para);
    }
    else
    {
        size = para_length;
    }

    if (USP_PROTOCOL_OK == ReceivingPayload(usp_handle, para, size))
    {
        ret = USP_PROTOCOL_OK;
        switch (usp_cmd)
        {
            case USP_DISCONNECT:
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                ret = USP_PROTOCOL_DISCONNECT;
                break;

            case USP_CONNECT:
                //对端会收到应答后会再回复一个ACK
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                if (USP_PROTOCOL_OK == ReceivingResponse(usp_handle)) {
			SetUSPConnectionFlag(usp_handle);
		}

#ifdef REPORT_USP_PROTOCOL_INFO
                if (0 == usp_handle->usp_protocol_global_data.master)
                {
                    // when in slave, should send its protocol info to master
                    ReportUSPProtocolInfo(usp_handle);
                }
#endif
                break;

            case USP_REPORT_PROTOCOL_INFO:
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                break;

            case USP_OPEN_PROTOCOL:
                ret = USP_PROTOCOL_NOT_SUPPORT_PROTOCOL;
                if (para[0] == usp_handle->usp_protocol_global_data.protocol_type)
                {
                    usp_handle->usp_protocol_global_data.opened = 1;
                    ret = USP_PROTOCOL_OK;
                }
                SendingResponse(usp_handle, ret);
                break;

            case USP_SET_BAUDRATE:
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                //应答之后再去更新波特率
                usp_handle->api.ioctl(USP_IOCTL_SET_BAUDRATE, (void*)sys_get_le32(para), NULL);
                break;

            case USP_INQUIRY_STATUS:
                ret = usp_handle->usp_protocol_global_data.state;
                SendingResponse(usp_handle, ret);
                break;

            case USP_SET_PAYLOAD_SIZE:
		usp_handle->usp_protocol_global_data.tx_max_payload_size = sys_get_le16(para);
		usp_handle->usp_protocol_global_data.rx_max_payload_size = sys_get_le16(para + 2);
		SendingResponse(usp_handle, USP_PROTOCOL_OK);
		break;

            case USP_REBOOT:
		SendingResponse(usp_handle, USP_PROTOCOL_OK);
		UspSysReboot(usp_handle, sys_get_le32(para) + 1);
		break;

            default:
                //not support usp protocol command
                DiscardReceivedData(usp_handle);
                SendingResponse(usp_handle, USP_PROTOCOL_NOT_SUPPORT_CMD);
                ret = USP_PROTOCOL_NOT_SUPPORT_CMD;
                break;
        }
    }

    return ret;
}



void SetUSPProtocolState(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS state)
{
    usp_handle->usp_protocol_global_data.state = state;
}

USP_PROTOCOL_STATUS USPBasicCommandProcess(usp_handle_t *usp_handle, u8_t usp_cmd, u8_t *payload, u32_t payload_len)
{
    int ret = USP_PROTOCOL_RX_ERR;

    switch (usp_cmd)
    {
        case USP_DISCONNECT:
			SetUSPDisConnectionFlag(usp_handle);
            ret = USP_PROTOCOL_DISCONNECT;
            break;

        case USP_CONNECT:
            //对端会收到应答后会再回复一个ACK
            SetUSPConnectionFlag(usp_handle);

#ifdef REPORT_USP_PROTOCOL_INFO
            if (0 == usp_handle->usp_protocol_global_data.master)
            {
                // when in slave, should send its protocol info to master
                ReportUSPProtocolInfoFD(usp_handle);
            }
#endif
            break;

        case USP_REPORT_PROTOCOL_INFO:
            break;

        case USP_OPEN_PROTOCOL:
            ret = USP_PROTOCOL_NOT_SUPPORT_PROTOCOL;
            //if (payload[0] == usp_handle->usp_protocol_global_data.protocol_type)
            {
            	int i;
                usp_handle->usp_protocol_global_data.opened = 1;
				usp_handle->usp_protocol_global_data.protocol_type = payload[0];
				for(i = 0; i < USP_MAX_PROTOCOL_HANDLER; i++){
					if(usp_handle->protocol_type[i] == 0xff){
						usp_handle->protocol_type[i] = payload[0];
						break;
					}
				}
                ret = USP_PROTOCOL_OK;
            }
            break;

        case USP_SET_BAUDRATE:
            //应答之后再去更新波特率
            usp_handle->api.ioctl(USP_IOCTL_SET_BAUDRATE, (void*)sys_get_le32(payload), NULL);
            break;

        case USP_INQUIRY_STATUS:
            ret = usp_handle->usp_protocol_global_data.state;
            break;

        case USP_SET_PAYLOAD_SIZE:
            usp_handle->usp_protocol_global_data.tx_max_payload_size = sys_get_le16(payload);
            usp_handle->usp_protocol_global_data.rx_max_payload_size = sys_get_le16(payload + 2);
            break;

        case USP_REBOOT:
			UspSysReboot(usp_handle, sys_get_le32(payload) + 1);
            break;

        default:
            //not support usp protocol command
            DiscardReceivedData(usp_handle);
            SendingResponse(usp_handle, USP_PROTOCOL_NOT_SUPPORT_CMD);
            ret = USP_PROTOCOL_NOT_SUPPORT_CMD;
            break;
    }

    return ret;
}


