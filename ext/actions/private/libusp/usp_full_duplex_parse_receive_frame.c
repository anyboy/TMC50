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

static int _is_acked_packet(usp_packet_head_t *head) {
  	return (head->type == USP_PACKET_TYPE_RSP && head->predefine_command == USP_PROTOCOL_OK);
}

static int _is_naked_packet(usp_packet_head_t *head) {
  	return (head->type == USP_PACKET_TYPE_RSP && head->predefine_command == USP_PROTOCOL_ERR);
}

static void _usp_set_current_received_acked_seq(usp_handle_t *usp_handle, short seq) {
  	usp_handle->usp_protocol_global_data.in_seq_number = seq;
}

static short _usp_get_current_received_acked_seq(usp_handle_t *usp_handle) {
  	return usp_handle->usp_protocol_global_data.in_seq_number;
}

static int _usp_get_current_sended_sequence(usp_handle_t *usp_handle) {
	if(usp_handle->usp_protocol_global_data.out_seq_number == 0){
		return 0xf;
	}else{
		return usp_handle->usp_protocol_global_data.out_seq_number - 1;
	}
}

static int _usp_packet_payload_valid(usp_handle_t *usp_handle, u8_t *data_buffer) {
	usp_data_pakcet_t data_packet;

	memcpy(&data_packet, data_buffer, sizeof(usp_data_pakcet_t));

	if(data_packet.head.payload_len){
		u8_t *payload_data = UspGetDataPayload(usp_handle, data_buffer);
		if(crc16(payload_data, data_packet.head.payload_len) != data_packet.crc16_val){
			return -1;
		}
	}

  	return 0;
}

static int _is_usp_same_received_frame(usp_handle_t *usp_handle, usp_packet_head_t *head) {
  	int samed;
  	samed = (usp_handle->parser.last_recv_packet_seq  == head->sequence_number);
  	usp_handle->parser.last_recv_packet_seq = head->sequence_number;
  	return samed;
}

void UspCheckFrameProcess(usp_handle_t *usp_handle, u8_t *data_buffer, u32_t len) {
	usp_packet_head_t head;

	if (NULL == usp_handle->recv_frame_handler) {
    	return;
  	}

	memcpy(&head, data_buffer, sizeof(usp_packet_head_t));

	/* check is ack/nak frame or data frame */
	if (_is_acked_packet(&head)) {
		//USP_TRACE_DEBUG("recv seq %d sended %d last recv %d", head.sequence_number, _usp_get_current_sended_sequence(usp_handle), _usp_get_current_received_acked_seq(usp_handle));
	    /* one sequence can only wakeup once */
	    if (head.sequence_number == _usp_get_current_sended_sequence(usp_handle) &&
	        head.sequence_number != _usp_get_current_received_acked_seq(usp_handle)) {
	      	UspSetAckSyncFlag(usp_handle);
	      	_usp_set_current_received_acked_seq(usp_handle, head.sequence_number);
	      	UspWakeSendThread(usp_handle);
	    }
	    return;
	}

  	if (_is_naked_packet(&head)) {
    	if (head.sequence_number == _usp_get_current_sended_sequence(usp_handle)) {
	      	UspWakeSendThread(usp_handle);
    	}
    	return;
  	}

	/* unpack data buffer */
  	if (0 != _usp_packet_payload_valid(usp_handle, data_buffer)) {
		//USP_TRACE_DEBUG("bad crc16 %d", head.sequence_number);
    	UspSendNakFrame(usp_handle, head.sequence_number);
    	return;
  	}

  	/* send ack */
  	UspSendAckFrame(usp_handle, head.sequence_number);

  	/* notify application when not ack frame nor same frame */
  	if (!_is_usp_same_received_frame(usp_handle, &head)) {
		UspNotifyUpperLayerFrameData(usp_handle, data_buffer);
  	}
}


