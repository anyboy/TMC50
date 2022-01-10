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

static int _usp_get_parser_state(usp_handle_t *usp_handle)
{
	return usp_handle->parser.state;
}

static void _usp_set_parser_state(usp_handle_t *usp_handle, int state)
{
	usp_handle->parser.state = state;
}

static void usp_trigger_rx(usp_handle_t *usp_handle, u8_t *buf, int len)
{
	if(usp_handle->recv_trigger_handler){
		usp_handle->recv_trigger_handler(buf, len);
	}
}

static void _usp_recv_data_process(usp_handle_t *usp_handle, u32_t len) {
	int ret;
	if (NULL == usp_handle->recv_data_handler || !len) {
    	return;
  	}

	ret = usp_handle->recv_data_handler(usp_handle->parser.frame_buffer, len);

	if(ret != 0){
		usp_handle->parser.err_cnt++;
		USP_TRACE_ERROR("drop pkg %d", usp_handle->parser.err_cnt);
	}
}

static int _usp_get_recv_protocol_type(unsigned char recv_c)
{
	return (recv_c >> 4);
}

int _usp_check_pkg_type_valid(usp_handle_t *usp_handle, u8_t *data_buffer, int len, int *remain_len)
{
	int i;
	int recv_protocol_type;
	usp_packet_head_t usp_head;

	if(len >= 1){
		if(data_buffer[0] != USP_PACKET_TYPE_DATA && data_buffer[0] != USP_PACKET_TYPE_ISO \
			 && data_buffer[0] != USP_PACKET_TYPE_RSP){
			return -1;
		}

		*remain_len = 5;
	}

	if(len >= 2){
		recv_protocol_type = _usp_get_recv_protocol_type(data_buffer[1]);

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
					USP_TRACE_ERROR("unkown protocol %d", recv_protocol_type);
					return -1;
				}
			}
	    }


		*remain_len = 4;
	}

	if(len >= 3){
	    usp_handle->parser.length_temp = data_buffer[2];

		*remain_len = 3;
	}

	if(len >= 4){
	    usp_handle->parser.length_temp += ((u32_t)data_buffer[3] << 8);

		if(usp_handle->parser.length_temp >= sizeof(usp_handle->parser.frame_buffer)){
			usp_handle->parser.length_temp = 0;
			return -1;
		}

		*remain_len = 2;
	}

	if(len >= 5){
		*remain_len = 1;
	}

	if(len == 6){

		memcpy(&usp_head, usp_handle->parser.frame_buffer, sizeof(usp_head));

		if(usp_head.crc8_val != crc8((u8_t *)&usp_head, USP_PACKET_HEAD_SIZE - 1)){
			return -1;
		}else{
			usp_handle->parser.protocol_type = usp_head.protocol_type;

			usp_handle->parser.seq_no = usp_head.sequence_number;

			usp_handle->parser.length = usp_head.payload_len;
		}

		*remain_len = 0;
	}

	return 0;
}

int _usp_parse_pkg_header(usp_handle_t *usp_handle, u32_t *next_len, u32_t *next_state, u32_t *next_offset)
{
	int i;
	int remain_len;
	int ret = -1;
	int valid_len;

	remain_len = 0;

	//print_buffer(usp_handle->parser.frame_buffer, 1, usp_handle->parser.index, 16, -1);

	/* check if valid header or not */
	valid_len = usp_handle->parser.index;
	for(i = 0; i < valid_len; i++){
		ret = _usp_check_pkg_type_valid(usp_handle, &usp_handle->parser.frame_buffer[i], usp_handle->parser.index, &remain_len);
		if(ret == 0){
			break;
		}
		usp_handle->parser.index--;
	}

	//printk("ret %d payload len %d valid %d remain %d\n", ret, usp_handle->parser.length, usp_handle->parser.index, remain_len);

	/* check valid header */
	if(ret == 0){
		if(!remain_len){
			if(usp_handle->parser.length){
				/*if wait ack,then received data, set busy mask no used current */
				if(usp_handle->parser.wait_ack){
					UspSetAckSyncBusyFlag(usp_handle);
					UspWakeSendThread(usp_handle);
				}

				if(usp_handle->parser.index == sizeof(usp_packet_head_t)){
					/* include data crc16 */
					*next_len = usp_handle->parser.length + 2;
					*next_state = USP_RX_STATUS_DATA;
					*next_offset = usp_handle->parser.index;
					usp_handle->parser.index += *next_len;
				}else{
					*next_len = USP_PARSER_HEADER_LEN;
					*next_state = USP_RX_STATUS_HEADER;
					*next_offset = 0;
					usp_handle->parser.index = *next_len;
				}
			}else{
				_usp_recv_data_process(usp_handle, usp_handle->parser.index);
				*next_len = USP_PARSER_HEADER_LEN;
				*next_state = USP_RX_STATUS_HEADER;
				*next_offset = 0;
				usp_handle->parser.index = *next_len ;
			}
		}else{
			memcpy(&usp_handle->parser.frame_buffer[0], &usp_handle->parser.frame_buffer[i], usp_handle->parser.index);
			*next_len = remain_len;
			*next_state = USP_RX_STATUS_HEADER;
			*next_offset = usp_handle->parser.index;
			usp_handle->parser.index += *next_len;
		}
	}else{
		/* no valid header, drop all data*/
		//usp_handle->parser.err_cnt++;
		//if (usp_handle->parser.err_cnt != 0){
		//	print_buffer(usp_handle->parser.frame_buffer, 1, usp_handle->parser.index, 16, -1);
		//	printk("err: cnt %d\n", usp_handle->parser.err_cnt);
		//}

		*next_len = USP_PARSER_HEADER_LEN;
		*next_offset = 0;
		*next_state = USP_RX_STATUS_HEADER;
		usp_handle->parser.index = *next_len;
	}

	//printk("next off %d state %d next len %d total %d\n", *next_offset, *next_state, *next_len, usp_handle->parser.index);

	return ret;

}

void UspRecvIsrDmaRoutinue(usp_handle_t *usp_handle)
{
    u32_t next_len, next_state, next_offset;

    //printk("rx status %d %d\n", _usp_get_parser_state(usp_handle), _usp_get_cur_index(usp_handle));

    switch (_usp_get_parser_state(usp_handle))
    {
		case USP_RX_STATUS_HEADER:
        _usp_parse_pkg_header(usp_handle, &next_len, &next_state, &next_offset);
		break;

        case USP_RX_STATUS_DATA:
		_usp_recv_data_process(usp_handle, usp_handle->parser.index);
		next_state = USP_RX_STATUS_HEADER;
		next_offset = 0;
		next_len = USP_PARSER_HEADER_LEN;
		usp_handle->parser.index = next_len;
		usp_handle->parser.length = 0;
        break;

        case USP_RX_STATUS_NULL:
		default:
		next_state = USP_RX_STATUS_HEADER;
		next_offset = 0;
		next_len = USP_PARSER_HEADER_LEN;
		usp_handle->parser.index = next_len;
        break;
    }

	_usp_set_parser_state(usp_handle, next_state);
    usp_trigger_rx(usp_handle, &usp_handle->parser.frame_buffer[next_offset], next_len);
	return;

}

