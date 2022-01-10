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
 * \file     usp_protocol_init.c
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


const usp_protocol_info_t usp_protocol_info =
{
    .protocol_version   = 0x140,
    .idVendor           = 0xACCA,
    .reserved0          = 0,
    .retry_count        = USP_PROTOCOL_MAX_RETRY_COUNT,
    .reserved1          = 0,
    .timeout            = USP_PROTOCOL_RX_TIMEOUT,
    .tx_max_payload     = USP_PAYLOAD_MAX_SIZE,
    .rx_max_payload     = USP_PAYLOAD_MAX_SIZE,
};




void InitUSPProtocol(usp_handle_t *usp_handle)
{
    if (!usp_handle->usp_protocol_global_data.protocol_init_flag)
    {
        // usp_handle->usp_protocol_global_data.connected              = 0;
        usp_handle->usp_protocol_global_data.protocol_init_flag     = 1;
        // usp_handle->usp_protocol_global_data.out_seq_number         = 0;
        //未连接阶段,retry次数减小,防止陷入时间太长
        usp_handle->usp_protocol_global_data.max_retry_count        = 1;
        usp_handle->usp_protocol_global_data.rx_timeout             = USP_PROTOCOL_RX_TIMEOUT;

        usp_handle->usp_protocol_global_data.tx_max_payload_size    = USP_PAYLOAD_MAX_SIZE;
        usp_handle->usp_protocol_global_data.rx_max_payload_size    = USP_PAYLOAD_MAX_SIZE;

        // USP_Tx_Init(USP_CPU_TRANSFER | USP_DATA_INQUIRY, NULL, NULL);
        //通过CPU轮询方式,处理USP RX端数据
        // USP_Rx_Init(USP_CPU_TRANSFER | USP_DATA_INQUIRY, NULL, NULL);

        /* 配置 USP stub 通信波特率 */
        // USP_Configure(USP_PROTOCOL_DEFAULT_BAUDRATE);
    }
}




void ExitUSPProtocol(usp_handle_t *usp_handle)
{
    if (usp_handle->usp_protocol_global_data.protocol_init_flag)
    {
        usp_handle->usp_protocol_global_data.protocol_init_flag     = 0;
        // USP_Rx_Exit();
        // USP_Tx_Exit();
    }
}



static void InitUSPOsHooks(usp_handle_t *usp_handle, const usp_os_interface_t *os_api) {
  	if (NULL == os_api){
		return;
	}

	usp_handle->os_api = os_api;

	if(os_api->sem_alloc && os_api->sem_init){
		usp_handle->app_mutex = usp_handle->os_api->sem_alloc();
		usp_handle->os_api->sem_init(usp_handle->app_mutex, 1);

		usp_handle->sync_lock = usp_handle->os_api->sem_alloc();
		usp_handle->os_api->sem_init(usp_handle->sync_lock, 0);

		usp_handle->hw_mutex = usp_handle->os_api->sem_alloc();
		usp_handle->os_api->sem_init(usp_handle->hw_mutex, 1);

		usp_handle->usp_protocol_global_data.sem_hooks_registered = true;
	}

}

void InitUSPProtocolFD(usp_handle_t *usp_handle, usp_init_param_t *param)
{
    if (!usp_handle->usp_protocol_global_data.protocol_init_flag)
    {
    	InitUSPOsHooks(usp_handle, param->os_api);

        usp_handle->usp_protocol_global_data.protocol_init_flag     = 1;
        //未连接阶段,retry次数减小,防止陷入时间太长
        usp_handle->usp_protocol_global_data.max_retry_count        = 1;
        usp_handle->usp_protocol_global_data.rx_timeout             = USP_PROTOCOL_RX_TIMEOUT;

        usp_handle->usp_protocol_global_data.tx_max_payload_size    = USP_PAYLOAD_MAX_SIZE;
        usp_handle->usp_protocol_global_data.rx_max_payload_size    = USP_PAYLOAD_MAX_SIZE;

		memset(&usp_handle->protocol_type, 0xff, sizeof(usp_handle->protocol_type));

		usp_handle->protocol_type[0] = USP_PROTOCOL_TYPE_FUNDAMENTAL;

		usp_handle->recv_frame_handler = param->handler;

		memcpy(&usp_handle->api, param->api, sizeof(usp_phy_interface_t));

		usp_handle->usp_protocol_global_data.in_seq_number = 0xf;

		usp_handle->parser.last_recv_packet_seq = 0xf;

		usp_handle->recv_data_handler = param->recv_data_handler;

		usp_handle->recv_trigger_handler = param->recv_trigger_handler;

		UspRecvIsrDmaRoutinue(usp_handle);
	}
}
