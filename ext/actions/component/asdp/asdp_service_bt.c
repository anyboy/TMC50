/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ASDP bluetooth service.
 */

#include <kernel.h>
#include <string.h>
#include <stream.h>
#include <soc.h>
#include <mem_manager.h>
#include <init.h>
#include <bt_manager.h>
#include <sys_manager.h>
#include <msg_manager.h>
#include <audio_policy.h>
#include <usp_protocol.h>
#include <asdp_adaptor.h>
#include <lib_asdp.h>
#include <misc/byteorder.h>
#include "asdp_inner.h"
#include "asdp_parser.h"
#include "asdp_transfer.h"

int asdp_bm_play(u8_t* tlv_list, u16_t len)
{
	if(1 == asdp_fix_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_PLAY, tlv_list, len)) {
		asdp_bt_contol(ASDP_BM_PLAY);
	}

	return 0;
}

int asdp_bm_pause(u8_t* tlv_list, u16_t len)
{
	if(1 == asdp_fix_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_PAUSE, tlv_list, len)) {
		asdp_bt_contol(ASDP_BM_PAUSE);
	}

	return 0;
}

int asdp_bm_prev(u8_t* tlv_list, u16_t len)
{
	if(1 == asdp_fix_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_PREV, tlv_list, len)) {
		asdp_bt_contol(ASDP_BM_PREV);
	}

	return 0;
}

int asdp_bm_next(u8_t* tlv_list, u16_t len)
{
	if(1 == asdp_fix_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_NEXT, tlv_list, len)) {
		asdp_bt_contol(ASDP_BM_NEXT);
	}

	return 0;
}

int asdp_bm_calling_accept(u8_t* tlv_list, u16_t len)
{
	if(1 == asdp_fix_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_CALLING_ACCEPT, tlv_list, len)) {
		asdp_bt_contol(ASDP_BM_CALLING_ACCEPT);
	}
	return 0;
}

int asdp_bm_calling_hangup(u8_t* tlv_list, u16_t len)
{
	if(1 == asdp_fix_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_CALLING_HANGUP, tlv_list, len)) {
		asdp_bt_contol(ASDP_BM_CALLING_HANGUP);
	}
	return 0;
}

int asdp_bm_calling_reject(u8_t* tlv_list, u16_t len)
{
	if(1 == asdp_fix_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_CALLING_REJECT, tlv_list, len)) {
		asdp_bt_contol(ASDP_BM_CALLING_REJECT);
	}
	return 0;
}

int asdp_bm_set_connectable(u8_t* tlv_list, u16_t len)
{
	asdp_no_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_SET_CONNECTABLE);
	asdp_bt_contol(ASDP_BM_SET_CONNECTABLE);
	return 0;
}

int asdp_bm_disconnect(u8_t* tlv_list, u16_t len)
{
	asdp_no_tlv_parse(ASDP_BT_MANAGER_SERVICE_ID, ASDP_BM_DISCONNECT_ALL_DEVICE);
	//asdp_bt_contol(ASDP_BM_DISCONNECT_ALL_DEVICE);
	asdp_bt_contol(ASDP_BM_SET_CONNECTABLE);
	return 0;
}

int asdp_bm_set_visible(u8_t* tlv_list, u16_t len){return 0;}
int asdp_bm_set_bt_name(u8_t* tlv_list, u16_t len){return 0;}
int asdp_bm_set_ble_name(u8_t* tlv_list, u16_t len){return 0;}
int asdp_bm_set_bt_mac(u8_t* tlv_list, u16_t len){return 0;}
int asdp_bm_get_bt_name(u8_t* tlv_list, u16_t len){return 0;}
int asdp_bm_get_ble_name(u8_t* tlv_list, u16_t len){return 0;}
int asdp_bm_get_bt_mac(u8_t* tlv_list, u16_t len){return 0;}

int asdp_bm_read_sppble_data(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;
	u16_t offset;
	u32_t max_len;
	u32_t read_len;
	struct tlv tlv;

	read_len = adsp_get_tlv_buf_len() - 3;
	offset = 0;
	while (offset < len) {
		offset += asdp_tlv_unpack_data(tlv_list+offset, &tlv);
		switch (tlv.type) {
		case TLV_TYPE_BM_OTA_DATA_MAX_LEN:
			max_len = sys_get_le32(tlv.val);
			ASDP_DBG("Read ota data max len=%d.", max_len);
			if(read_len > max_len) {
				read_len = max_len;
			}
			break;
		default:
			ASDP_WRN("Not implemented tlv, type=%d.", tlv.type);
			break;
		}
	}

	tlv_buf = adsp_get_tlv_buf();
	ASDP_DBG("To read ota data .");
	tlv_send = asdp_sppble_read_data(tlv_buf + 3, read_len);
	if (tlv_send > 0) {
		tlv_send += asdp_tlv_pack_tl(tlv_buf, TLV_TYPE_BM_OTA_DATA, tlv_send);

	} else {
		tlv_send = asdp_tlv_pack_tl(tlv_buf, TLV_TYPE_BM_OTA_DATA, 0);
	}

	err = asdp_send_cmd(ASDP_BT_MANAGER_SERVICE_ID,
			ASDP_BM_READ_SPPBLE_DATA,
			tlv_send);
	if (err) {
		ASDP_ERR("failed to send cmd, error=%d", err);
		return -1;
	}

	return 0;

}

int asdp_bm_write_sppble_data(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;
	u16_t offset;
	struct tlv tlv;
	bool cmd = false;

	offset = 0;
	while (offset < len) {
		offset += asdp_tlv_unpack_data(tlv_list+offset, &tlv);
		switch (tlv.type) {
		case TLV_TYPE_BM_OTA_DATA:
			if(tlv.len > 0){
				cmd = true;
			}
			break;
		default:
			ASDP_WRN("Not implemented tlv, type=%d.", tlv.type);
			break;
		}
	}

	tlv_buf = adsp_get_tlv_buf();
	if (cmd) {
		tlv_send = asdp_tlv_pack_ack(tlv_buf);
	} else {
		tlv_send = asdp_tlv_pack_nak(tlv_buf);
	}
	if (tlv_send > 0) {
		err = asdp_send_cmd(ASDP_BT_MANAGER_SERVICE_ID,
				ASDP_BM_WRITE_SPPBLE_DATA,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return -1;
		}

	}

	//Write data after response.
	if (cmd) {
		ASDP_DBG("To write ota data, len=%d.", tlv.len);
		asdp_sppble_write_data(tlv.val, tlv.len);
	}

	return 0;
}

static const struct asdp_port_cmd my_service_cmds[] = {

	{ASDP_BM_PLAY, asdp_bm_play},
	{ASDP_BM_PAUSE, asdp_bm_pause},
	{ASDP_BM_PREV,  asdp_bm_prev},
	{ASDP_BM_NEXT,  asdp_bm_next},
	{ASDP_BM_CALLING_ACCEPT,  asdp_bm_calling_accept},
	{ASDP_BM_CALLING_HANGUP,  asdp_bm_calling_hangup},
	{ASDP_BM_CALLING_REJECT,  asdp_bm_calling_reject},
	{ASDP_BM_SET_VISIBLE,  asdp_bm_set_visible},
	{ASDP_BM_SET_CONNECTABLE,  asdp_bm_set_connectable},
	{ASDP_BM_DISCONNECT_ALL_DEVICE,  asdp_bm_disconnect},
	{ASDP_BM_SET_BT_NAME,  asdp_bm_set_bt_name},
	{ASDP_BM_SET_BLE_NAME,  asdp_bm_set_ble_name},
	{ASDP_BM_SET_BT_MAC,  asdp_bm_set_bt_mac},
	{ASDP_BM_GET_BT_NAME,  asdp_bm_get_bt_name},
	{ASDP_BM_GET_BLE_NAME,  asdp_bm_get_ble_name},
	{ASDP_BM_GET_BT_MAC,  asdp_bm_get_bt_mac},

	//{ASDP_BM_READ_SPPBLE_DATA,  asdp_bm_read_sppble_data},
	//{ASDP_BM_WRITE_SPPBLE_DATA,  asdp_bm_write_sppble_data},
};

static int asdp_bt_service_process_command(u8_t* asdp_frame, u32_t len)
{
	int i, err;
	struct asdp_port_head *head = (struct asdp_port_head *)asdp_frame;

	for (i = 0; i < ARRAY_SIZE(my_service_cmds); i++) {
		if (head->cmd == my_service_cmds[i].cmd) {
			err = my_service_cmds[i].handler(asdp_frame + sizeof(struct asdp_port_head), head->param_len);
			if (err) {
				ASDP_ERR("cmd_handler %p, err: %d",
					    &my_service_cmds[i], err);
			}else{
				ASDP_DBG("Handle command successfully.");
			}
			break;
		}
	}

	if (i == ARRAY_SIZE(my_service_cmds)) {
		ASDP_ERR("invalid asdp_cmd: %d, i=%d", head->cmd, i);
		return -EIO;
	}

	return err;
}

static const struct asdp_port_srv bt_svc = {
	.srv_id = ASDP_BT_MANAGER_SERVICE_ID,
	.handler = asdp_bt_service_process_command,
};

int asdp_bt_service_init(struct device *dev)
{
	ARG_UNUSED(dev);
	asdp_register_service_handle(&bt_svc);

	SYS_LOG_INF("successful");
	return 0;
}

//41 == CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 1
SYS_INIT(asdp_bt_service_init, POST_KERNEL, 41);

