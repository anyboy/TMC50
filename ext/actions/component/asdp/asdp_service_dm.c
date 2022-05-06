/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA bluetooth backend interface
 */

#include <kernel.h>
#include <string.h>
#include <stream.h>
#include <soc.h>
#include <mem_manager.h>
#include <init.h>
#include <bt_manager.h>
#include <sys_manager.h>
#include <audio_policy.h>
#include <audio_system.h>
#include <audio_track.h>
#include <usp_protocol.h>
#include <lib_asdp.h>
#include <asdp_adaptor.h>
#include "asdp_inner.h"
#include "asdp_parser.h"
#include "asdp_transfer.h"
#include "audio_system.h"

#ifdef CONFIG_USOUND_APP
#include "usb_audio.h"
#endif

//extern void system_message_notify(uint8_t msg_type, uint8_t msg_cmd);

struct asdp_dev_mgr_ctx {
	u8_t protocol_version;
	u16_t max_frame_size;
	u16_t mtu_size;
	u16_t interval;
};

static struct asdp_dev_mgr_ctx g_asdp_dev_mgr_ctx;

struct asdp_dev_mgr_ctx *get_asdp_dev_mgr_ctx(void)
{
	return &g_asdp_dev_mgr_ctx;
}

int asdp_cmd_h2d_connect_negotiation(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	struct tlv tlv;
	u16_t offset;
	u16_t tlv_send;

	tlv_buf = adsp_get_tlv_buf();
	offset = 0;
	tlv_send = 0;
	struct asdp_dev_mgr_ctx *ctx = get_asdp_dev_mgr_ctx();

	while (offset < len) {
		offset += asdp_tlv_unpack_data(tlv_list+offset, &tlv);
		switch (tlv.type) {
		case 0x01:
			tlv_send +=
			    asdp_tlv_pack_data(tlv_buf+tlv_send, 0x01, tlv.len,
					       ctx->protocol_version);
			ASDP_INF("protocol version: 0x%d",
				    ctx->protocol_version);
			break;
		case 0x02:
			tlv_send +=
			    asdp_tlv_pack_data(tlv_buf+tlv_send, 0x02, tlv.len,
					       ctx->max_frame_size);
			ASDP_INF("max frame size: 0x%d",
				    ctx->max_frame_size);
			break;
		case 0x03:
			tlv_send +=
			    asdp_tlv_pack_data(tlv_buf+tlv_send, 0x03, tlv.len,
					       ctx->mtu_size);
			ASDP_INF("mtu size: 0x%d", ctx->mtu_size);
			break;
		case 0x04:
			tlv_send +=
			    asdp_tlv_pack_data(tlv_buf+tlv_send, 0x04, tlv.len,
					       ctx->interval);
			ASDP_INF("interval: 0x%d", ctx->interval);
			break;
		default:
			ASDP_ERR("Not implemented tlv, type=%d.", tlv.type);
			break;
		}
	}

	if(tlv_send > 0){
		err = asdp_send_cmd(ASDP_DEVICE_MANAGER_SERVICE_ID,
				ASDP_DM_CONNECT_NEGOTIATION,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return err;
		}
	}

	return 0;
}

int asdp_cmd_h2d_service_negotiation(u8_t* tlv_list, u16_t len)
{
	ASDP_ERR("Not implemented.");

	return 0;
}


/*获取蓝牙状态*/
u32_t asdp_dm_get_bt_status(void)
{
	int ret = DM_BT_STATUS_NONE;

	int status = (bt_manager_hfp_get_status() != BT_STATUS_NONE)? bt_manager_hfp_get_status() : bt_manager_get_status();

	switch (status) {
		case BT_STATUS_INCOMING:
			ret = DM_BT_STATUS_INCOMING;
			break;

		case BT_STATUS_OUTGOING:
			ret = DM_BT_STATUS_OUTGOING;
			break;

		case BT_STATUS_ONGOING:
			ret = DM_BT_STATUS_ONGOING;
			break;

		case BT_STATUS_MULTIPARTY:
			ret = DM_BT_STATUS_MULTIPARTY;
			break;

		case BT_STATUS_WAIT_PAIR:
			ret = DM_BT_STATUS_WAIT_PAIR;
			break;

		case BT_STATUS_MASTER_WAIT_PAIR:
			ret = DM_BT_STATUS_MASTER_WAIT_PAIR;
			break;

		case BT_STATUS_PAUSED:
			ret = DM_BT_STATUS_PAUSED;
			break;

		case BT_STATUS_PLAYING:
			ret = DM_BT_STATUS_PLAYING;
			break;

		case BT_STATUS_SIRI:
			ret = DM_BT_STATUS_SIRI;
			break;

		case BT_STATUS_3WAYIN:
			ret = DM_BT_STATUS_3WAYIN;
			break;

		default:
			ret = DM_BT_STATUS_NONE;
			break;

	}

	return ret;
}
u8_t asdp_dm_get_a2dp_status(void)
{
	return bt_manager_get_status();
}
u8_t asdp_dm_get_hfp_status(void)
{
	return bt_manager_hfp_get_status();
}
u8_t asdp_dm_get_scn_status(void)
{
	return 1;
}

extern int btsrv_rdm_get_dev_role(void);
u8_t asdp_dm_get_ccn_status(void)
{
	int num = bt_manager_get_connected_dev_num();
	if (num == 0) {
		if (btsrv_rdm_get_dev_role() == BTSRV_TWS_SLAVE) {
			num = 1;
		}
	}
	//SYS_LOG_INF("tws:%d, num:%d\n", btsrv_rdm_get_dev_role(), num);
	return num;
}

u8_t asdp_dm_get_callring_status(void)
{
	return 0;
}
u32_t asdp_dm_get_ota_len(void)
{
	return 0;
	//return asdp_sppble_get_data_len();
}

u32_t asdp_dm_get_usb_status(void)
{

#ifndef CONFIG_USOUND_APP
	return USB_STATUS_DISCONNECTED;
#else
	return usound_get_uac_status();
#endif

}

int asdp_samplerate = 0;

u32_t asdp_set_state_sample_rate( u32_t sample_rate)
{
	asdp_samplerate = sample_rate;

	printk("%s s rate:%d\n", __FUNCTION__, asdp_samplerate);
	return 0;
}

u32_t asdp_dm_get_samplerate(void)
{

	u32_t sample_rate = 0;

	if (bt_manager_hfp_get_status() == BT_STATUS_INCOMING ||
	  bt_manager_hfp_get_status() == BT_STATUS_OUTGOING ||
	  bt_manager_hfp_get_status() == BT_STATUS_ONGOING ||
	  bt_manager_hfp_get_status() == BT_STATUS_3WAYIN ||
	  bt_manager_hfp_get_status() == BT_STATUS_SIRI ) {
		sample_rate = bt_manager_sco_get_sample_rate();
		return sample_rate;
	}

	if (bt_manager_get_status() == BT_STATUS_PLAYING || bt_manager_get_status() == BT_STATUS_PAUSED) {
		sample_rate = bt_manager_a2dp_get_sample_rate();
	}

	if (asdp_samplerate != 0) {
		sample_rate = asdp_samplerate;
	}

	return sample_rate;

}

int asdp_cmd_h2d_battery_capacity(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;
	u16_t offset;
	struct tlv tlv;

	offset = 0;
	while (offset < len) {
		offset += asdp_tlv_unpack_data(tlv_list+offset, &tlv);
		switch (tlv.type) {
		case 0x01:
			asdp_set_system_battery_capacity(tlv.val[0]);
			break;
		default:
			ASDP_ERR("Not implemented tlv, type=%d.", tlv.type);
			break;
		}
	}

	tlv_buf = adsp_get_tlv_buf();
	tlv_send = asdp_tlv_pack_ack(tlv_buf);
	if (tlv_send > 0) {
		err = asdp_send_cmd(ASDP_DEVICE_MANAGER_SERVICE_ID,
				ASDP_DM_SET_BATTERY_CAPACITY,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return err;
		}
	}

	return 0;
}
int asdp_cmd_h2d_vol_plus(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;

	ASDP_INF("To do volume plus.");
        //system_local_key_volume_ctrl(AUDIO_STREAM_MUSIC, true);

	asdp_bt_contol(ASDP_BM_VOLUME_ADD);

	tlv_buf = adsp_get_tlv_buf();
	tlv_send = asdp_tlv_pack_ack(tlv_buf);
	if (tlv_send > 0) {
		err = asdp_send_cmd(ASDP_DEVICE_MANAGER_SERVICE_ID,
				ASDP_DM_VOLUME_ADD,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return err;
		}
	}

	return 0;
}
int asdp_cmd_h2d_vol_minus(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;

	ASDP_INF("To do volume minus.");
        //system_local_key_volume_ctrl(AUDIO_STREAM_MUSIC, false);

	asdp_bt_contol(ASDP_BM_VOLUME_SUB);

	tlv_buf = adsp_get_tlv_buf();
	tlv_send = asdp_tlv_pack_ack(tlv_buf);
	if (tlv_send > 0) {
		err = asdp_send_cmd(ASDP_DEVICE_MANAGER_SERVICE_ID,
				ASDP_DM_VOLUME_SUB,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return err;
		}
	}

	return 0;
}
int asdp_cmd_h2d_vol_set(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;
	u16_t offset;
	u8_t vol_max=0;
	u8_t vol=0;
	struct tlv tlv;

	offset = 0;
	while (offset < len) {
		offset += asdp_tlv_unpack_data(tlv_list+offset, &tlv);
		switch (tlv.type) {
		case 0x01:
			vol_max = tlv.val[0];
			ASDP_INF("Max volume = %d.", vol_max);
			break;
		case 0x02:
			vol = tlv.val[0];
			ASDP_INF("Max volume = %d.", vol);
			break;
		default:
			ASDP_ERR("Not implemented tlv, type=%d.", tlv.type);
			break;
		}
	}

	ASDP_INF("To set volume, %d/%d.", vol, vol_max);
	if(vol_max != 0){
		//system_local_key_volume_set(AUDIO_STREAM_MUSIC, (MAX_AUDIO_VOL_LEVEL*vol/vol_max));
	}

	tlv_buf = adsp_get_tlv_buf();
	tlv_send = asdp_tlv_pack_ack(tlv_buf);
	if (tlv_send > 0) {
		err = asdp_send_cmd(ASDP_DEVICE_MANAGER_SERVICE_ID,
				ASDP_DM_SET_VOLUME,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return err;
		}
	}

	return 0;
}
int asdp_cmd_h2d_status(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;

	tlv_buf = adsp_get_tlv_buf();
	tlv_send = 0;

	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_SS_BT, 4, asdp_dm_get_bt_status());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_SS_A2DP, 1, asdp_dm_get_a2dp_status());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_SS_HFP, 1, asdp_dm_get_hfp_status());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_SS_SCN, 1, asdp_dm_get_scn_status());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_SS_CCN, 1, asdp_dm_get_ccn_status());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_SS_CALLRING, 1, asdp_dm_get_callring_status());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_SS_OTA, 4, asdp_dm_get_ota_len());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_USB_STATUS, 4, asdp_dm_get_usb_status());
	tlv_send +=
		asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_GET_SAMPLERATE, 4, asdp_dm_get_samplerate());

	if (tlv_send > 0) {
		ASDP_DBG("To send System Status.");
		//print_buffer(tlv_buf, 1, tlv_send, 16, -1);
		err = asdp_send_cmd(ASDP_DEVICE_MANAGER_SERVICE_ID,
				ASDP_DM_GET_BT_STATUS,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return err;
		}
	}

	return 0;
}

int asdp_cmd_h2d_power_off(u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;
	u16_t offset;
	struct tlv tlv;
	bool shutoff = false;

	offset = 0;
	while (offset < len) {
		offset += asdp_tlv_unpack_data(tlv_list+offset, &tlv);
		switch (tlv.type) {
		case 0x01:
			//shut off
			ASDP_INF("Get shut off command.");
			shutoff = true;
			break;
		default:
			ASDP_ERR("Not implemented tlv, type=%d.", tlv.type);
			break;
		}
	}

	tlv_buf = adsp_get_tlv_buf();
	if (shutoff) {
		tlv_send = asdp_tlv_pack_ack(tlv_buf);
		tlv_send += asdp_tlv_pack_data(tlv_buf+tlv_send, TLV_TYPE_DM_POWER_OFF_DELAY, 2, 500);
	} else {
		tlv_send = asdp_tlv_pack_nak(tlv_buf);
	}
	if (tlv_send > 0) {
		err = asdp_send_cmd(ASDP_DEVICE_MANAGER_SERVICE_ID,
				ASDP_DM_POWER_OFF,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return err;
		}

	}

	if (shutoff) {
		ASDP_INF("To do pre power off jobs.");
		//system_message_notify(MSG_POWER_OFF, 0);
	}

	return 0;
}

static const struct asdp_port_cmd device_manager_cmds[] = {
	{ASDP_DM_CONNECT_NEGOTIATION, asdp_cmd_h2d_connect_negotiation},
	{ASDP_DM_SERVICE_SUPPORT_LIST, asdp_cmd_h2d_service_negotiation},
	{ASDP_DM_GET_BT_STATUS, asdp_cmd_h2d_status},
	{ASDP_DM_SET_BATTERY_CAPACITY, asdp_cmd_h2d_battery_capacity},
	{ASDP_DM_VOLUME_ADD, asdp_cmd_h2d_vol_plus},
	{ASDP_DM_VOLUME_SUB, asdp_cmd_h2d_vol_minus},
	{ASDP_DM_SET_VOLUME, asdp_cmd_h2d_vol_set},
	{ASDP_DM_POWER_OFF, asdp_cmd_h2d_power_off},
};

static int asdp_device_manager_process_command(u8_t* asdp_frame, u32_t len)
{
	int i, err;
	struct asdp_port_head *head = (struct asdp_port_head *)asdp_frame;

	for (i = 0; i < ARRAY_SIZE(device_manager_cmds); i++) {
		if (head->cmd == device_manager_cmds[i].cmd) {
			err = device_manager_cmds[i].handler(asdp_frame + sizeof(struct asdp_port_head), head->param_len);
			if (err) {
				ASDP_ERR("cmd_handler %p, err: %d",
					    &device_manager_cmds[i], err);
			}else{
				ASDP_DBG("Handle command successfully.");
			}
			break;
		}
	}

	if (i == ARRAY_SIZE(device_manager_cmds)) {
		ASDP_ERR("invalid asdp_cmd: %d, i=%d", head->cmd, i);
		return -EIO;
	}

	return err;
}

static const struct asdp_port_srv device_manage_svc = {
	.srv_id = ASDP_DEVICE_MANAGER_SERVICE_ID,
	.handler = asdp_device_manager_process_command,
};

int asdp_device_manager_service_init(struct device *dev)
{
	ARG_UNUSED(dev);

	struct asdp_dev_mgr_ctx *ctx = get_asdp_dev_mgr_ctx();

	ctx->protocol_version = 0x01;
	ctx->max_frame_size = 0x800 + sizeof(struct asdp_port_head);
	ctx->mtu_size = ctx->max_frame_size;
	ctx->interval = 0;

	asdp_register_service_handle(&device_manage_svc);
	SYS_LOG_INF("success!");

	return 0;
}

//41 == CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 1
SYS_INIT(asdp_device_manager_service_init, POST_KERNEL, 41);

