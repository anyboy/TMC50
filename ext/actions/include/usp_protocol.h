/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       usp_protocol.h
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

#ifndef __USP_PROTOCOL_H__
#define __USP_PROTOCOL_H__

#define USP_PAYLOAD_MAX_SIZE            (1024)

//USP最大允许解析协议数
#define USP_MAX_PROTOCOL_HANDLER        (2)

#ifndef _ASSEMBLER_
/**
\brief  usp protocol status of executed operation
*/
typedef enum __USP_PROTOCOL_STATUS
{
    USP_PROTOCOL_OK =  0,               ///< Operation succeeded
    USP_PROTOCOL_BUSY,

    USP_PROTOCOL_ERR = 10,              ///< Unspecified error
    USP_PROTOCOL_TX_ERR,                ///< usp protocol send data error happend
    USP_PROTOCOL_RX_ERR,                ///< usp protocol receive data error happend
    USP_PROTOCOL_DATA_CHECK_FAIL,
    USP_PROTOCOL_PAYLOAD_TOOLARGE,      ///< packet payload length too large to transfer

    USP_PROTOCOL_TIMEOUT = 20,          ///< usp protocol transaction timeout
    USP_PROTOCOL_DISCONNECT,            ///< disconnect USP

    USP_PROTOCOL_NOT_EXPECT_PROTOCOL = 30,  ///< NOT expected usp protocol
    USP_PROTOCOL_NOT_SUPPORT_PROTOCOL,  ///< NOT support usp protocol
    USP_PROTOCOL_NOT_SUPPORT_BAUDRATE,  ///< NOT support usp baudrate
    USP_PROTOCOL_NOT_SUPPORT_CMD,       ///< Not support usp command
}USP_PROTOCOL_STATUS;


/**
\brief  usp protocol packet type
*/
typedef enum
{
    USP_PACKET_TYPE_DATA            = 0x01,
    USP_PACKET_TYPE_ISO             = 0x16,
    USP_PACKET_TYPE_RSP             = 0x05,
    USP_PACKET_TYPE_MAX             = 0x20,
}USP_PACKET_TYPE_E;


/**
\brief  usp protocol type
*/
typedef enum
{
    USP_PROTOCOL_TYPE_FUNDAMENTAL   = 0,
    USP_PROTOCOL_TYPE_STUB          = 1,
    USP_PROTOCOL_TYPE_ASET          = 2,
    USP_PROTOCOL_TYPE_ASDP 	    = 3,
    USP_PROTOCOL_TYPE_ADFU          = 8,
    USP_PROTOCOL_TYPE_MAX           = 15,
}USP_PROTOCOL_TYPE_E;


/**
\brief  usp protocol data transfer direction
*/
typedef enum
{
    USP_PROTOCOL_DIRECTION_TX       = 0,
    USP_PROTOCOL_DIRECTION_RX,
}USP_PROTOCOL_TRANSFER_DIRECTION_E;


/**
\brief  usp protocol connect mode
*/
typedef enum
{
    USP_INITIAL_CONNECTION = 0,     ///< 主动发起连接动作
    USP_CHECK_INITIAL_CONNECTION,   ///< 检测标记, 确认后主动发起连接动作
    USP_WAIT_CONNECTION,            ///< 被动响应连接动作
} USP_CONNECT_MODE_E;


/**
\brief  usp protocol work mode
*/
typedef enum
{
    USP_WORKMODE_HALF_DUPLEX = 0,     ///< 主动发起连接动作
	USP_WORKMODE_FULL_DUPLEX,
} USP_WORK_MODE_E;


typedef struct
{
    u8_t   type;
    u8_t   sequence_number:4;      ///< 发送包编号, 均由发送方管理, 接收方根据此值判断是否是重传的包
    u8_t   protocol_type:4;
    u16_t  payload_len;            ///< 用户数据长度, 不包含命令头数据
    u8_t   predefine_command;      ///< USP最底层协议支持的基本命令
    u8_t   crc8_val;
}usp_packet_head_t;

typedef struct
{
    usp_packet_head_t head;
    u16_t crc16_val;
    u8_t payload[0];
}usp_data_pakcet_t;

typedef struct
{
    u8_t type;
	u8_t predefine_command;
	u16_t protocol_type : 4;
	u16_t payload_len : 12;
    u8_t *payload;
}usp_data_frame_t;

typedef struct
{
    u8_t   type;
    u8_t   reserved[3];
    u8_t   status;                 ///< 应答状态, \ref USP_PROTOCOL_STATUS
    u8_t   crc8_val;
}usp_ack_packet_t;


#define USP_PACKET_HEAD_SIZE       (sizeof(usp_packet_head_t))


typedef struct
{
    u8_t   type;
    u8_t   reserved:4;
    u8_t   protocol_type:4;
    int (*packet_handle_hook)(usp_packet_head_t *head);
}usp_protocol_fsm_item_t;

typedef struct
{
	u8_t state;
	u8_t seq_no;
	u8_t last_recv_packet_seq;
	u8_t protocol_type;
	u8_t wait_ack;
	u16_t length;
	u16_t length_temp;
	u16_t index;
	u16_t crc;
	u16_t max_parser_length;
	u8_t  frame_buffer[USP_PAYLOAD_MAX_SIZE];
	u16_t err_cnt;
}usp_parser_fsm_t;

typedef struct
{
    u8_t    protocol_type;

    // 0: slave; 1: master
    u8_t    master;

    u8_t    protocol_init_flag:1;
    u8_t    connected:1;
    u8_t    opened:1;
    // on some channel(such as USB, SPP), use transparent transfer
    u8_t    transparent:1;
    u8_t    reserved:4;

    u8_t    out_seq_number:4;
    u8_t    in_seq_number:4;

    u8_t    state;
    u8_t    max_retry_count;
    u16_t   rx_timeout;

    u16_t   tx_max_payload_size;
    u16_t   rx_max_payload_size;

	u8_t    work_mode; //0:Half Duplex  1:Full Duplex
	u8_t    sem_hooks_registered;
	u8_t    acked;
} usp_protocol_global_data_t;

enum usp_phy_ioctl_cmd_e
{
    USP_IOCTL_SET_BAUDRATE = 1,
    USP_IOCTL_INQUIRY_STATE,
    USP_IOCTL_ENTER_WRITE_CRITICAL,
    USP_IOCTL_CHECK_WRITE_FINISH,
    USP_IOCTL_REBOOT,
};

typedef struct
{
    int (*read)(u8_t *data, u32_t length, u32_t timeout_ms);
    int (*write)(u8_t *data, u32_t length, u32_t timeout_ms);
    int (*ioctl)(u32_t mode, void *para1, void *para2);
}usp_phy_interface_t;

typedef void (*usp_frame_handler)(usp_data_frame_t *data);
typedef int (*usp_data_handler)(void *data, u32_t len);
typedef void (*usp_rx_trigger_handler)(void *buf, u32_t len);

typedef struct{
	void *(*sem_alloc)(void);
	void  (*sem_destroy)(void *sem);
	int   (*sem_init)(void *sem, unsigned int value);
	int   (*sem_post)(void *sem);
	int   (*sem_wait)(void *sem);
	int   (*sem_timedwait)(void *sem, unsigned int timeout_msecond);
	int   (*msleep)(unsigned int msecond);
}usp_os_interface_t;

typedef struct
{
    usp_protocol_global_data_t usp_protocol_global_data;
    usp_phy_interface_t api;
	const usp_os_interface_t *os_api;
	usp_parser_fsm_t parser;
	u8_t protocol_type[USP_MAX_PROTOCOL_HANDLER];
	usp_frame_handler recv_frame_handler;
	usp_data_handler recv_data_handler;
	usp_rx_trigger_handler recv_trigger_handler;
	void *sync_lock;	//sync with sending thread and reciving thread
 	void *hw_mutex;    //sync with hardware
  	void *app_mutex;   //sync with uper layer
}usp_handle_t;

typedef struct{
	usp_frame_handler handler;
	usp_phy_interface_t *api;
	const usp_os_interface_t *os_api;
	usp_data_handler recv_data_handler;
	usp_rx_trigger_handler recv_trigger_handler;
}usp_init_param_t;


/**
 *  \brief Initial usp protocol.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void InitUSPProtocol(usp_handle_t *usp_handle);


/**
 *  \brief Initial usp protocol using full duplex mode.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] param   init param
 *
 *  \details
 */
void InitUSPProtocolFD(usp_handle_t *usp_handle, usp_init_param_t *param);

/**
 *  \brief exit usp protocol..
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void ExitUSPProtocol(usp_handle_t *usp_handle);


/**
 *  \brief Initial usp connection protocol.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return     Whether usp protocol connection is established.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details
 */
bool ConnectUSP(usp_handle_t *usp_handle);



/**
  *  \brief Initial usp connection protocol.
  *
  *  \param [in] usp_handle  USP protocol handler.
  *  \return	 Whether usp protocol connection is established.
  * 				 TRUE:	 success
  * 				 FALSE:  fail
  *
  *  \details
  */
bool ConnectUSPFD(usp_handle_t *usp_handle);


 /**
 *  \brief Check connection flag and initiate usp connection.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] timeout_ms  Maximun wait time, millisecond unit.
 *  \return     Whether usp protocol connection is established.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details More details
 */
bool Check_ConnectUSP(usp_handle_t *usp_handle, u32_t timeout_ms);


 /**
 *  \brief Wait peer device to initiate usp connection.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] timeout_ms  Maximun wait time, millisecond unit.
 *  \return     Whether usp protocol connection is established.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details More details
 */
bool WaitUSPConnect(usp_handle_t *usp_handle, u32_t timeout_ms);


/**
 *  \brief Disconnect usp protocol.
 *
 *  \param [in] usp_handle      USP protocol handler.
 *  \return     Whether usp protocol disconnect successful.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details
 */
bool DisconnectUSP(usp_handle_t *usp_handle);


/**
 *  \brief Open usp application protocol.
 *
 *  \param [in] usp_handle      USP protocol handler.
 *  \param [in] protocol_type   to be communicate protocol type, \ref in USP_PROTOCOL_TYPE_E.
 *  \return     execution status \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
USP_PROTOCOL_STATUS OpenUSP(usp_handle_t *usp_handle);

/**
 *  \brief Open usp application protocol withing protocol type
 *
 *  \param [in] usp_handle      USP protocol handler.
 *  \param [in] protocol_type   to be communicate protocol type, \ref in USP_PROTOCOL_TYPE_E.
 *  \return     execution status \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
USP_PROTOCOL_STATUS OpenUSPFD(usp_handle_t *usp_handle, u8_t protocol_type);

/**
 *  \brief Set usp protocol state.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] state       USP protocol state, \ref USP_PROTOCOL_STATUS.
 *  \return None
 *
 *  \details
 */
void SetUSPProtocolState(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS state);


/**
 *  \brief USP protocol data packet inquiry.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     USP protocol payload buffer pointer.
 *  \param [in] payload_len USP protocol payload length, byte unit.
 *  \param [in] recv_head   pre received packet head pointer, only type received.
 *  \return successfully received data bytes from peer device.
 *              0   : USP fundamental command receive and handle corectly
 *              < 0 : Error Hanppend, \ref in USP_PROTOCOL_STATUS
 *              > 0 : Successfully received USP protocol payload size.
 *
 *  \details when recv_head is NOT NULL, means received 1byte in the recv_head.
 */
int USP_Protocol_Inquiry(usp_handle_t *usp_handle, u8_t *payload, u32_t payload_len, usp_packet_head_t *recv_head);


/**
 *  \brief send user payload to peer USP device, maybe is predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] user_data   To be send data buffer pointer.
 *  \param [in] size        To be send data number, byte unit.
 *  \return data bytes of successfully send to peer device.
 *
 *  \details
 */
int WriteUSPData(usp_handle_t *usp_handle, u8_t *user_data, u32_t size);


int WriteUSPDataFD(usp_handle_t *usp_handle, u8_t *payload, u32_t size);

/**
 *  \brief send user data to peer USP device, no ACK from peer device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [out]user_data   To be send data buffer pointer.
 *  \param [in] size        To be send data number, byte unit.
 *  \return data bytes of successfully send to peer device.
 *
 *  \details
 */
int WriteUSPISOData(usp_handle_t *usp_handle, u8_t *user_data, u32_t size);

int WriteISODataFD(usp_handle_t *usp_handle, u8_t *payload, u32_t size);

/**
 *  \brief receive 1 payload packet from peer USP device, maybe is predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] recv_head   pre received packet head pointer.
 *  \param [in] payload     To be received payload buffer pointer.
 *  \param [in] size        To be received payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \return if successfully received payload bytes from peer device.
 *              - value >= 0: has been successful received payload bytes
 *              - value < 0: error occurred, -value is \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
int ReceiveUSPDataPacket(usp_handle_t *usp_handle, usp_packet_head_t *recv_head, u8_t *payload, u32_t size);


/**
 *  \brief receive user data from peer USP device, maybe is predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] user_data   To be received data buffer pointer.
 *  \param [in] size        To be received data number, byte unit.
 *  \return successfully received data bytes from peer device.
 *
 *  \details
 */
int ReadUSPData(usp_handle_t *usp_handle, u8_t *user_data, u32_t size);


/**
 *  \brief Send inquiry comamnd to peer device, check if it is still online.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return Peer device status, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS InquiryUSP(usp_handle_t *usp_handle);

/**
 *  \brief USP receing data isr routinue for cpu process, not used current
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] buf     received data buffer
 *  \param [in] buf     received data length
 *
 *  \details
 */
void UspRecvIsrCpuRoutinue(usp_handle_t *usp_handle, unsigned char *buf, int len);

/**
 *  \brief USP receing data isr routinue for DMA process, used current
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void UspRecvIsrDmaRoutinue(usp_handle_t *usp_handle);

/**
 *  \brief Parse usp predefine_command.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] data     received usp command.
 *
 *  \details
 */
USP_PROTOCOL_STATUS USPBasicCommandProcess(usp_handle_t *usp_handle, u8_t usp_cmd, u8_t *payload, u32_t payload_len);

/**
 *  \brief check one USP frame for low layer process
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] buf     received data buffer
 *  \param [in] buf     received data length
 *
 *  \details
 */
void UspCheckFrameProcess(usp_handle_t *usp_handle, u8_t *data_buffer, u32_t len);

/**
 *  \brief check if USP hardware connected
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return [out]  0 not connected 1 connected
 *
 *  \details
 */
int UspIsHardwareConnected(usp_handle_t *usp_handle);

/**
 *  \brief check if USP Drop Some Frame
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return [out]  drop frame number
 *
 *  \details
 */
int UspGetDropFrameCnt(usp_handle_t *usp_handle);

#endif /*_ASSEMBLER_*/
#endif  /* __USP_PROTOCOL_H__ */

