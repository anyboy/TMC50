/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       usp_protocol_inner.h
 *  \brief      usp(univesal serial protocol) head file
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-10-19
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#ifndef __USP_PROTOCOL_INNER__
#define __USP_PROTOCOL_INNER__

#include "usp_def_config.h"
#include "crc.h"
#include "usp_protocol.h"

#ifdef USP_DEBUG_ENABLE
#define USP_DEBUG_PRINT            SYS_LOG_INF
#else
#define USP_DEBUG_PRINT(...)       do {} while(0)
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#define USP_ERROR 	SYS_LOG_ERR
#define USP_WARNING 	SYS_LOG_WRN
#define USP_DEBUG 	SYS_LOG_INF

/* Define trace levels */
#define USP_TRACE_LEVEL_NONE 0    /* No trace messages to be generated    */
#define USP_TRACE_LEVEL_ERROR 1   /* Error condition trace messages       */
#define USP_TRACE_LEVEL_WARNING 2 /* Warning condition trace messages     */
#define USP_TRACE_LEVEL_INFO 3 	 /* info messages                  */
#define USP_TRACE_LEVEL_DEBUG 4   /* Full debug messages                  */

#define USP_TRACE_ERROR(fmt,...)                                      \
	{																 \
	  if (USP_TRACE_LEVEL >= USP_TRACE_LEVEL_ERROR)				\
		USP_ERROR(fmt,##__VA_ARGS__); \
	}

#define USP_TRACE_WARNING(fmt,...)                                      \
  {                                                                \
    if (USP_TRACE_LEVEL >= USP_TRACE_LEVEL_WARNING)               \
      USP_WARNING(fmt,##__VA_ARGS__); \
  }

#define USP_TRACE_DEBUG(fmt,...)                                      \
  {                                                                \
    if (USP_TRACE_LEVEL >= USP_TRACE_LEVEL_DEBUG)               \
      USP_DEBUG(fmt,##__VA_ARGS__); \
  }


//工具发起连接时,先发送32byte连续的 0x63,设备收到4B连续0x63后,发起 usp connect 请求
#define USP_PC_CONNECT_MAGIC        (0x63636363)

#ifndef _ASSEMBLER_
typedef struct
{
    u16_t protocol_version;                 ///< 0x0100: 1.00
    u16_t idVendor;                         ///< 0xACCA: ACTIONS
    u16_t reserved0;
    u8_t  retry_count;                      ///< data transfer err(CRC check fail), max retry count
    u8_t  reserved1;
    u16_t timeout;                          ///< acknowledge timeout: ms unit
    u16_t tx_max_payload;                   ///< current device tx packet max payload size
    u16_t rx_max_payload;                   ///< current device rx packet max payload size
}usp_protocol_info_t;

extern const usp_protocol_info_t usp_protocol_info;


typedef enum __USP_COMMAND
{
    USP_CONNECT                =  ('c'),    ///< usp connect
    USP_DISCONNECT             =  ('d'),    ///< usp disconnect
    USP_REBOOT                 =  ('R'),    ///< reboot device
    USP_OPEN_PROTOCOL          =  ('o'),    ///< usp open protocol connection
    USP_SET_BAUDRATE           =  ('b'),    ///< set usp communication baudrate
    USP_SET_PAYLOAD_SIZE       =  ('p'),    ///< set usp packet max payload size
    USP_INQUIRY_STATUS         =  ('i'),    ///< inquiry usp communication status
    USP_REPORT_PROTOCOL_INFO   =  ('r'),    ///< report usp protocol infomation
}USP_PREDEFINED_COMMAND;


enum
{
    USP_RX_STATUS_NULL,
    USP_RX_STATUS_HEADER,
    USP_RX_STATUS_DATA,
};

#define USP_PARSER_HEADER_LEN (sizeof(usp_packet_head_t))

/**
 *  \brief Set USP connection flag.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void SetUSPConnectionFlag(usp_handle_t *usp_handle);

/**
 *  \brief Set USP disconnection flag.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void SetUSPDisConnectionFlag(usp_handle_t *usp_handle);


/**
 *  \brief Parse usp predefine_command.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] usp_cmd     received usp command.
 *  \param [in] para_length paramenter length, byte unit.
 *  \return     execution status \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
USP_PROTOCOL_STATUS ParseUSPCommand(usp_handle_t *usp_handle, u8_t usp_cmd, u32_t para_length);


/**
 *  \brief send 1 data packet(witch ACK) to peer USP device, maybe send predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be send payload buffer pointer.
 *  \param [in] size        To be send payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \param [in] command     USP fundamental command
 *  \return     if successfully send payload to peer device.
 *                  - value >= 0: has been successful send payload bytes
 *                  - value < 0:  error occurred
 *
 *  \details
 */
USP_PROTOCOL_STATUS SendUSPDataPacket(usp_handle_t *usp_handle, u8_t *payload, u32_t size, u8_t command);


/**
 *  \brief Sending ISO packet to peer device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be send payload buffer pointer.
 *  \param [in] size        To be send payload number, byte unit.
 *  \return None
 *
 *  \details
 */
void SendUSPISOPacket(usp_handle_t *usp_handle, u8_t *payload, u32_t size);


/**
 *  \brief send 1 data packet to peer USP device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be send payload buffer pointer.
 *  \param [in] size        To be send payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \param [in] type        USP packet type, \ref USP_PACKET_TYPE_E
 *  \param [in] command     USP fundamental command
 *  \return None
 *
 *  \details
 */
void SendingPacket(usp_handle_t *usp_handle, u8_t *payload, u32_t size, USP_PACKET_TYPE_E type, u8_t command);


/**
 *  \brief receiving packet head data, and verify it, first data has been received.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] head        received packet head data buffer.
 *  \return Packet head data received and verify result, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingPacketHeadExcepttype(usp_handle_t *usp_handle, usp_packet_head_t *head);


/**
 *  \brief receiving packet head data, and verify it.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] head        received packet head data buffer.
 *  \return Packet head data received and verify result, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingPacketHead(usp_handle_t *usp_handle, usp_packet_head_t *head);


/**
 *  \brief Receiving usp payload data, and check if CRC16 is correct.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be received payload buffer pointer.
 *  \param [in] payload_len To be received payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \return execution status \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingPayload(usp_handle_t *usp_handle, u8_t *payload, u32_t payload_len);


/**
 *  \brief sending respond packet to peer USP device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] status      respond status code, \ref USP_PROTOCOL_STATUS.
 *  \return None
 *
 *  \details
 */
void SendingResponse(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS status);


/**
 *  \brief receiving respond data, and verify it.
 *
 *  \param [in] usp_handle USP protocol handler.
 *  \return Respond data received and verify result, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingResponse(usp_handle_t *usp_handle);


/**
 *  \brief discard all received USP data.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return     Discard received data bytes number.
 *
 *  \details
 */
int DiscardReceivedData(usp_handle_t *usp_handle);


/**
 *  \brief report usp protocol infomation to peer device, \ref usp_protocol_info_t.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void ReportUSPProtocolInfo(usp_handle_t *usp_handle);

/**
 *  \brief report usp protocol infomation to peer device, \ref usp_protocol_info_t.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void ReportUSPProtocolInfoFD(usp_handle_t *usp_handle);


/**
 *  \brief reboot system
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] delaytime_ms reboot delay time
 *
 *  \details
 */
void UspSysReboot(usp_handle_t *usp_handle, u32_t delaytime_ms);

/**
 *  \brief check write fifo finished
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void UspCheckwriteFinish(usp_handle_t *usp_handle);

/**
 *  \brief USP waiting send frame ack(full duplex mode using in sending thread)
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
int UspWaitACK(usp_handle_t *usp_handle);

/**
 *  \brief USP wakeup sending thread(full duplex mode using in sending thread)
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
int UspWakeSendThread(usp_handle_t *usp_handle);

/**
 *  \brief USP before sending clear sync flag for sync
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void UspClrAckSyncFlag(usp_handle_t *usp_handle);

/**
 *  \brief USP after receive ack, set sync flag for sync
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void UspSetAckSyncFlag(usp_handle_t *usp_handle);

/**
 *  \brief USP after receive data frame before ack, set busy flag for sync
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void UspSetAckSyncBusyFlag(usp_handle_t *usp_handle);

/**
 *  \brief USP get data paylod in data buffer
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] data_buffer  data frame buffer
 *
 *  \details
 */
u8_t* UspGetDataPayload(usp_handle_t *usp_handle, u8_t *data_buffer);

/**
 *  \brief USP send nak frame for sender
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] seq  receing data seq
 *
 *  \details
 */
void UspSendNakFrame(usp_handle_t *usp_handle, int seq);

/**
 *  \brief USP send ack frame for sender
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] seq  receing data seq

 *  \details
 */
void UspSendAckFrame(usp_handle_t *usp_handle, int seq);

/**
 *  \brief USP after receive data frame, callback upper layer for data process
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] data_buffer  data frame buffer
*
 *  \details
 */
void UspNotifyUpperLayerFrameData(usp_handle_t *usp_handle, u8_t *data_buffer);

#endif /*_ASSEMBLER_*/
#endif  /* __USP_PROTOCOL_INNER__ */

