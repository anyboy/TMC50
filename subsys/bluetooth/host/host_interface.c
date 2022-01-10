/** @file
 * @brief Bluetooth host interface.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <bluetooth/host_interface.h>
#include "hci_core.h"
#include "conn_internal.h"

int hostif_set_negative_prio(void)
{
	int prio = 0;
	if(!k_is_in_isr()){
		prio = k_thread_priority_get(k_current_get());
		if (prio >= 0) {
			k_thread_priority_set(k_current_get(), -1);
		}
	}

	return prio;
}

void hostif_revert_prio(int prio)
{
	if(!k_is_in_isr()){
		if (prio >= 0) {
			k_thread_priority_set(k_current_get(), prio);
		}
	}
}

int hostif_bt_init_class(u32_t classOfDevice)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_init_class_of_device(classOfDevice);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_init_device_id(u16_t *did)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_init_device_id(did);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_enable(bt_ready_cb_t cb)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_enable(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_disable(void)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_disable();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  bt_br_discovery_cb_t cb)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_discovery_start(param, cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_discovery_stop(void)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_discovery_stop();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_scan_enable(u8_t scan)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_scan_enable(scan);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_iac(bool limit_iac)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_iac(limit_iac);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_page_scan_activity(u16_t interval, u16_t windown)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_page_scan_activity(interval, windown);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_inquiry_scan_type(u8_t type)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_inquiry_scan_type(type);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_page_scan_type(u8_t type)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_page_scan_type(type);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_remote_name_request(const bt_addr_t *addr, bt_br_remote_name_cb_t cb)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_remote_name_request(addr, cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_inquiry_scan_activity(u16_t interval, u16_t windown)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_inquiry_scan_activity(interval, windown);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

void hostif_bt_conn_cb_register(struct bt_conn_cb *cb)
{
#ifdef CONFIG_BT_HCI
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_cb_register(cb);
	hostif_revert_prio(prio);
#endif
}

int hostif_bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_get_info(conn, info);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

u8_t hostif_bt_conn_get_type(const struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	u8_t type;
	int prio;

	prio = hostif_set_negative_prio();
	type = bt_conn_get_type(conn);
	hostif_revert_prio(prio);

	return type;
#else
	return 0;
#endif
}

u16_t hostif_bt_conn_get_acl_handle(const struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	u16_t handle;
	int prio;

	prio = hostif_set_negative_prio();
	handle = bt_conn_get_acl_handle(conn);
	hostif_revert_prio(prio);

	return handle;
#else
	return 0;
#endif
}

struct bt_conn *hostif_bt_conn_get_conn_by_handle(u16_t handle )
{
#ifdef CONFIG_BT_HCI
	struct bt_conn* conn;
	int prio;

	prio = hostif_set_negative_prio();
	conn = bt_conn_lookup_handle(handle);
	if(conn)
		bt_conn_unref(conn);
	hostif_revert_prio(prio);

	return conn;
#else
	return NULL;
#endif
}

struct bt_conn *hostif_bt_conn_get_acl_conn_by_sco(const struct bt_conn *sco_conn)
{
#ifdef CONFIG_BT_BREDR
	struct bt_conn *conn;
	int prio;

	prio = hostif_set_negative_prio();
	conn = bt_conn_get_acl_conn_by_sco(sco_conn);
	hostif_revert_prio(prio);

	return conn;
#else
	return NULL;
#endif
}

int hostif_bt_conn_disconnect(struct bt_conn *conn, u8_t reason)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_disconnect(conn, reason);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

struct bt_conn *hostif_bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param)
{
#ifdef CONFIG_BT_BREDR
	struct bt_conn *conn;
	int prio;

	prio = hostif_set_negative_prio();
	conn = bt_conn_create_br(peer, param);
	hostif_revert_prio(prio);

	return conn;
#else
	return NULL;
#endif
}

struct bt_conn *hostif_bt_conn_br_acl_connecting(const bt_addr_t *addr)
{
#ifdef CONFIG_BT_BREDR
	struct bt_conn *conn;
	int prio;

	prio = hostif_set_negative_prio();
	conn = bt_conn_br_acl_connecting(addr);
	hostif_revert_prio(prio);

	return conn;
#else
	return NULL;
#endif
}

int hostif_bt_conn_create_sco(struct bt_conn *br_conn)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_req_codec_connection(br_conn);
	if (ret != 0) {
		/* HF not connect or AG device, just do sco connect */
		ret = bt_conn_create_sco(br_conn);
	}
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_send_sco_data(struct bt_conn *conn, u8_t *data, u8_t len)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_send_sco_data(conn, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

void hostif_bt_conn_force_exit_sniff(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_force_exit_sniff(conn);
	hostif_revert_prio(prio);
#endif
}

bool hostif_bt_conn_is_in_sniff(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	bool sniff;
	int prio;

	prio = hostif_set_negative_prio();
	sniff = bt_conn_is_in_sniff(conn);
	hostif_revert_prio(prio);

	return sniff;
#else
	return false;
#endif
}

int hostif_bt_conn_send_connectionless_data(struct bt_conn *conn, u8_t *data, u16_t len)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_send_connectionless_data(conn, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_conn_ready_send_data(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_conn_ready_send_data(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_conn_pending_pkt(struct bt_conn *conn, u8_t *host_pending, u8_t *controler_pending)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_conn_pending_pkt(conn, host_pending, controler_pending);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_role_discovery(struct bt_conn *conn, u8_t *role)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_role_discovery(conn, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_switch_role(struct bt_conn *conn, u8_t role)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_switch_role(conn, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_set_supervision_timeout(struct bt_conn *conn, u16_t timeout)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_set_supervision_timeout(conn, timeout);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

void hostif_bt_conn_set_sniff_check_enable(bool enable)
{
#ifdef CONFIG_BT_BREDR
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_set_sniff_check_enable(enable);
	hostif_revert_prio(prio);
#endif
}

int hostif_bt_hfp_hf_register_cb(struct bt_hfp_hf_cb *cb)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_hf_send_cmd(struct bt_conn *conn, enum bt_hfp_hf_at_cmd cmd, char *user_cmd)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_send_cmd(conn, cmd, user_cmd);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_hf_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_hf_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_register_cb(struct bt_hfp_ag_cb *cb)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_update_indicator(u8_t index, u8_t value)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_update_indicator(index, value);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_get_indicator(u8_t index)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_get_indicator(index);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_send_event(struct bt_conn *conn, char *event, u16_t len)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_send_event(conn, event, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

void hostif_bt_hfp_ag_display_indicator(void)
{
#ifdef CONFIG_BT_HFP_AG
	int prio;

	prio = hostif_set_negative_prio();
	bt_hfp_ag_display_indicator();
	hostif_revert_prio(prio);
#endif
}

int hostif_bt_a2dp_connect(struct bt_conn *conn, u8_t role)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_connect(conn, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
			      u8_t media_type, u8_t role)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_register_endpoint(endpoint, media_type, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_halt_endpoint(struct bt_a2dp_endpoint *endpoint, bool halt)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_halt_endpoint(endpoint, halt);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_register_cb(struct bt_a2dp_app_cb *cb)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_start(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_start(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_suspend(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_suspend(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_reconfig(struct bt_conn *conn, struct bt_a2dp_media_codec *codec)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_reconfig(conn, codec);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_send_delay_report(struct bt_conn *conn, u16_t delay_time)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_send_delay_report(conn, delay_time);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_send_audio_data(struct bt_conn *conn, u8_t *data, u16_t len)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_send_audio_data(conn, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

struct bt_a2dp_media_codec *hostif_bt_a2dp_get_seted_codec(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	struct bt_a2dp_media_codec *codec;
	int prio;

	prio = hostif_set_negative_prio();
	codec = bt_a2dp_get_seted_codec(conn);
	hostif_revert_prio(prio);

	return codec;
#else
	return NULL;
#endif
}

u8_t hostif_bt_a2dp_get_a2dp_role(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	u8_t role;
	int prio;

	prio = hostif_set_negative_prio();
	role = bt_a2dp_get_a2dp_role(conn);
	hostif_revert_prio(prio);

	return role;
#else
	return BT_A2DP_CH_UNKOWN;
#endif
}

int hostif_bt_spp_send_data(u8_t spp_id, u8_t *data, u16_t len)
{
#ifdef CONFIG_BT_SPP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_spp_send_data(spp_id, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

u8_t hostif_bt_spp_connect(struct bt_conn *conn, u8_t *uuid)
{
#ifdef CONFIG_BT_SPP
	u8_t spp_id;
	int prio;

	prio = hostif_set_negative_prio();
	spp_id = bt_spp_connect(conn, uuid);
	hostif_revert_prio(prio);

	return spp_id;
#else
	return -EIO;
#endif
}

int hostif_bt_spp_disconnect(u8_t spp_id)
{
#ifdef CONFIG_BT_SPP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_spp_disconnect(spp_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

u8_t hostif_bt_spp_register_service(u8_t *uuid)
{
#ifdef CONFIG_BT_SPP
	u8_t spp_id;
	int prio;

	prio = hostif_set_negative_prio();
	spp_id = bt_spp_register_service(uuid);
	hostif_revert_prio(prio);

	return spp_id;
#else
	return 0;
#endif
}

int hostif_bt_spp_register_cb(struct bt_spp_app_cb *cb)
{
#ifdef CONFIG_BT_SPP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_spp_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_cttg_register_cb(struct bt_avrcp_app_cb *cb)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_cttg_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_cttg_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_cttg_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_cttg_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_cttg_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_pass_through_cmd(struct bt_conn *conn, avrcp_op_id opid)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_pass_through_cmd(conn, opid);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_pass_through_continue_cmd(struct bt_conn *conn,
										avrcp_op_id opid, bool start)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_pass_through_continue_cmd(conn, opid, start);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_tg_notify_change(struct bt_conn *conn, u8_t play_state)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_tg_notify_change(conn, play_state);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_get_id3_info(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_get_id3_info(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

u8_t hostif_bt_pbap_client_get_phonebook(struct bt_conn *conn, char *path, struct bt_pbap_client_user_cb *cb)
{
#ifdef CONFIG_BT_PBAP_CLIENT
	u8_t user_id;
	int prio;

	prio = hostif_set_negative_prio();
	user_id = bt_pbap_client_get_phonebook(conn, path, cb);
	hostif_revert_prio(prio);

	return user_id;
#else
	return 0;
#endif
}

int hostif_bt_pbap_client_abort_get(struct bt_conn *conn, u8_t user_id)
{
#ifdef CONFIG_BT_PBAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_pbap_client_abort_get(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

u8_t hostif_bt_map_client_connect(struct bt_conn *conn,char *path,struct bt_map_client_user_cb *cb)
{
#ifdef CONFIG_BT_MAP_CLIENT
	u8_t user_id;
	int prio;

	prio = hostif_set_negative_prio();
	user_id = bt_map_client_connect(conn,path,cb);
	hostif_revert_prio(prio);

	return user_id;
#else
	return 0;
#endif
}

u8_t hostif_bt_map_client_get_message(struct bt_conn *conn, char *path, struct bt_map_client_user_cb *cb)
{
#ifdef CONFIG_BT_MAP_CLIENT
	u8_t user_id;
	int prio;

	prio = hostif_set_negative_prio();
	user_id = bt_map_client_get_message(conn, path, cb);
	hostif_revert_prio(prio);

	return user_id;
#else
	return 0;
#endif
}

int hostif_bt_map_client_abort_get(struct bt_conn *conn, u8_t user_id)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_abort_get(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_map_client_disconnect(struct bt_conn *conn, u8_t user_id)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_disconnect(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_map_client_set_folder(struct bt_conn *conn, u8_t user_id,char *path,u8_t flags)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_set_folder(conn, user_id,path,flags);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_map_client_get_folder_listing(struct bt_conn *conn, u8_t user_id)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_get_folder_listing(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}


int hostif_bt_map_client_get_messages_listing(struct bt_conn *conn, u8_t user_id,u16_t max_list_count,u32_t parameter_mask)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_get_messages_listing(conn, user_id,max_list_count,parameter_mask);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_register_sdp(struct bt_sdp_attribute * hid_attrs,int attrs_size)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_register_sdp(hid_attrs, attrs_size);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_register_cb(struct bt_hid_app_cb *cb)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_send_ctrl_data(struct bt_conn *conn,u8_t report_type,u8_t * data,u16_t len)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_send_ctrl_data(conn,report_type, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_send_intr_data(struct bt_conn *conn,u8_t report_type,u8_t * data,u16_t len)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_send_intr_data(conn,report_type, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_send_rsp(struct bt_conn *conn,u8_t status)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_send_rsp(conn,status);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_did_register_sdp(struct device_id_info *device_id)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_did_register_sdp(device_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_adv_start(param, ad, ad_len, sd, sd_len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_adv_stop(void)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_adv_stop();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_le_param_update(conn, param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_service_register(struct bt_gatt_service *svc)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_service_register(svc);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

u16_t hostif_bt_gatt_get_mtu(struct bt_conn *conn)
{
#ifdef CONFIG_BT_LE_ATT
	u16_t mtu;
	int prio;

	prio = hostif_set_negative_prio();
	mtu = bt_gatt_get_mtu(conn);
	hostif_revert_prio(prio);

	return mtu;
#else
	return 0;
#endif
}

int hostif_bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_indicate(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_notify(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   const void *data, u16_t len)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_notify(conn, attr, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_exchange_mtu(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_scan_start(param, cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_scan_stop(void)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_scan_stop();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_broadcast_link_key_seed(u8_t *seed, u8_t len)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_broadcast_link_key_seed(seed, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_reserved_lt_addr(u8_t lt_addr)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_reserved_lt_addr(lt_addr);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_CSB_broadcast(u8_t enable, u8_t lt_addr, u16_t interval_min, u16_t interval_max)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_CSB_broadcast(enable, lt_addr, interval_min, interval_max);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_CSB_receive(u8_t enable, struct bt_hci_evt_sync_train_receive *param, u16_t timeout)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_CSB_receive(enable, param, timeout);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_write_sync_train_param(u16_t interval_min, u16_t interval_max, u32_t trainTO, u8_t service_data)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_write_sync_train_param(interval_min, interval_max, trainTO, service_data);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_start_sync_train(void)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_start_sync_train();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_CSB_date(u8_t lt_addr, u8_t *data, u8_t len)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_CSB_date(lt_addr, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_receive_sync_train(u8_t *mac, u16_t timeout, u16_t windown, u16_t interval)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_receive_sync_train(mac, timeout, windown, interval);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_broadcase_by_acl(u16_t handle, u8_t *data, u16_t len)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_broadcase_by_acl(handle, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}
