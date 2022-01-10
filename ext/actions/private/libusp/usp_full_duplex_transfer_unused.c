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

static int _is_usp_parser_buffer_overflow(usp_handle_t *usp_handle) {
  	return usp_handle->parser.index >= sizeof(usp_handle->parser.frame_buffer);
}

static void _usp_reset_parser_status(usp_handle_t *usp_handle) {
  	usp_handle->parser.index = 0;
  	usp_handle->parser.length = 0;
  	usp_handle->parser.crc = 0;
}

static int _usp_get_cur_index(usp_handle_t *usp_handle)
{
	return usp_handle->parser.index;
}

static int _usp_get_cur_length(usp_handle_t *usp_handle)
{
	return usp_handle->parser.length;
}

static void _usp_save_recv_char(usp_handle_t *usp_handle, unsigned char recv_c)
{
	usp_handle->parser.frame_buffer[usp_handle->parser.index] = recv_c;
	usp_handle->parser.index++;
}

static void _usp_save_recv_payload(usp_handle_t *usp_handle, unsigned char recv_c)
{
	usp_handle->parser.length--;
}

static void _usp_save_recv_payload_crc(usp_handle_t *usp_handle, unsigned char recv_c)
{
	if(_usp_get_cur_index(usp_handle) == (sizeof(usp_packet_head_t) + 1)){
		usp_handle->parser.crc = recv_c;
	}else if(_usp_get_cur_index(usp_handle) == (sizeof(usp_packet_head_t) + 2)){
		usp_handle->parser.crc |= ((u32_t)recv_c << 8);
	}else{

	}
}

static int _usp_check_header_simple(usp_handle_t *usp_handle, unsigned char recv_c)
{
	int i;
	int recv_protocol_type;
	int index = _usp_get_cur_index(usp_handle);

	//first char is packet type, must less than USP_PACKET_TYPE_MAX
	if(index == 1){
		if(recv_c != USP_PACKET_TYPE_DATA && recv_c != USP_PACKET_TYPE_ISO \
			 && recv_c != USP_PACKET_TYPE_RSP){
			return -1;
		}
	}

	//second char is protocol type & seq no
	if(index == 2){
		recv_protocol_type = ((recv_c >> 4) & 0xf);

	    if (recv_protocol_type != USP_PROTOCOL_TYPE_FUNDAMENTAL){
			if (!usp_handle->usp_protocol_global_data.protocol_init_flag){
				return -1;
			}else{
				for(i = 0; i < USP_MAX_PROTOCOL_HANDLER; i++){
					if(usp_handle->protocol_type[i] == recv_protocol_type){
						break;
					}
				}
				if (i == USP_MAX_PROTOCOL_HANDLER){
					//SYS_LOG_ERR("unkown protocol %d", recv_protocol_type);
					return -1;
				}
				usp_handle->parser.protocol_type = recv_protocol_type;
			}
	    }

		usp_handle->parser.seq_no = (recv_c & 0xf);

		//SYS_LOG_INF("seq %d", usp_handle->parser.seq_no);
	}

	if(index == 3){
	    usp_handle->parser.length = recv_c;
	}

	if(index == 4){
	    usp_handle->parser.length += ((u32_t)recv_c << 8);

		if(usp_handle->parser.length >= usp_handle->usp_protocol_global_data.rx_max_payload_size){
			return -1;
		}
	}

	return 0;

}

static int _usp_check_header_valid(usp_handle_t *usp_handle)
{
	usp_packet_head_t usp_head;

	memcpy(&usp_head, usp_handle->parser.frame_buffer, sizeof(usp_head));

	if(usp_head.crc8_val == crc8((u8_t *)&usp_head, USP_PACKET_HEAD_SIZE - 1)){
		return 0;
	}else{
		return -1;
	}
}

int _usp_check_pkg_head(usp_handle_t *usp_handle, u8_t *data_buffer)
{
	int i;
	usp_packet_head_t usp_head;

	memcpy(&usp_head, data_buffer, sizeof(usp_head));

	if(usp_head.type != USP_PACKET_TYPE_DATA && usp_head.type != USP_PACKET_TYPE_ISO \
		&& usp_head.type != USP_PACKET_TYPE_RSP){
		return -1;
	}

    if (usp_head.protocol_type != USP_PROTOCOL_TYPE_FUNDAMENTAL){
		if (!usp_handle->usp_protocol_global_data.protocol_init_flag){
			return -1;
		}else{
			for(i = 0; i < USP_MAX_PROTOCOL_HANDLER; i++){
				if(usp_handle->protocol_type[i] == usp_head.protocol_type){
					break;
				}
			}
			if (i == USP_MAX_PROTOCOL_HANDLER){
				USP_TRACE_ERROR("unkown protocol %d", usp_head.protocol_type);
				return -1;
			}
		}
    }



	if(usp_head.payload_len >= sizeof(usp_handle->parser.frame_buffer)){
		return -1;
	}

	if(usp_head.crc8_val != crc8((u8_t *)&usp_head, USP_PACKET_HEAD_SIZE - 1)){
		return -1;
	}

	usp_handle->parser.protocol_type = usp_head.protocol_type;

	usp_handle->parser.seq_no = usp_head.sequence_number;

    usp_handle->parser.length = usp_head.payload_len;

	return 0;
}


static void _usp_recv_one_frame_process(usp_handle_t *usp_handle) {
	UspCheckFrameProcess(usp_handle, usp_handle->parser.frame_buffer, usp_handle->parser.index);
}

static void usp_parse_recv_bytes(usp_handle_t *usp_handle, unsigned char recv_c) {

	_usp_save_recv_char(usp_handle, recv_c);

  	/* check databuffer not overflow */
  	if (_is_usp_parser_buffer_overflow(usp_handle)) {
	    /* drop remain bytes of this frame */
	    if (usp_handle->parser.length > 1) {
	    	usp_handle->parser.length--;
	      	return;
	    }

    	_usp_reset_parser_status(usp_handle);
    	return;
  	}

	/* smaller than usp packet head, check&collect header information */
  	if (_usp_get_cur_index(usp_handle) < sizeof(usp_packet_head_t)) {
		if(_usp_check_header_simple(usp_handle, recv_c) != 0){
			_usp_reset_parser_status(usp_handle);
			return;
		}
		return;
  	}

  	if(_usp_get_cur_index(usp_handle) == sizeof(usp_packet_head_t)){
		if(_usp_check_header_valid(usp_handle) != 0){
			_usp_reset_parser_status(usp_handle);
			UspSendNakFrame(usp_handle, usp_handle->parser.seq_no);
		}else{
			/* no payload data ? */
			if(usp_handle->parser.length == 0){
				_usp_recv_one_frame_process(usp_handle);
				_usp_reset_parser_status(usp_handle);
			}
		}

		return;
  	}

	if(_usp_get_cur_index(usp_handle) >= (sizeof(usp_packet_head_t) + 1) && \
		_usp_get_cur_index(usp_handle) <= (sizeof(usp_packet_head_t) + 2)){
		_usp_save_recv_payload_crc(usp_handle, recv_c);
		return;
	}

	if(_usp_get_cur_index(usp_handle) > (sizeof(usp_packet_head_t) + 2) \
		&& _usp_get_cur_length(usp_handle) > 0){
		_usp_save_recv_payload(usp_handle, recv_c);
	}

	if(_usp_get_cur_index(usp_handle) > (sizeof(usp_packet_head_t) + 2) \
		&& _usp_get_cur_length(usp_handle) == 0){
		_usp_recv_one_frame_process(usp_handle);
		_usp_reset_parser_status(usp_handle);
	}

	return;
}

void UspRecvIsrCpuRoutinue(usp_handle_t *usp_handle, unsigned char *buf, int len) {
  	int i;

  	for (i = 0; i < len; i++) {
    	usp_parse_recv_bytes(usp_handle, buf[i]);
  	}
}

