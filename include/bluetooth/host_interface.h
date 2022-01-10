/** @file
 * @brief Bluetooth host interface header file.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __HOST_INTERFACE_H
#define __HOST_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/storage.h>
#include <bluetooth/sdp.h>
#include <bluetooth/a2dp.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hfp_hf.h>
#include <bluetooth/hfp_ag.h>
#include <bluetooth/avrcp_cttg.h>
#include <bluetooth/spp.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/pbap_client.h>
#include <bluetooth/map_client.h>
#include <bluetooth/hid.h>
#include <bluetooth/device_id.h>

#define hostif_bt_addr_to_str(a, b, c)		bt_addr_to_str(a, b, c)
#define hostif_bt_addr_le_to_str(a, b, c)	bt_addr_le_to_str(a, b, c)
#define hostif_bt_addr_cmp(a, b)			bt_addr_cmp(a, b)
#define hostif_bt_addr_copy(a, b)			bt_addr_copy(a, b)

#ifdef CONFIG_BT_HCI
#define hostif_bt_conn_ref(a)				bt_conn_ref(a)
#define hostif_bt_conn_unref(a)				bt_conn_unref(a)

#define hostif_bt_storage_register(a)		bt_storage_register(a)
#define hostif_bt_read_ble_name(a, b)		bt_read_ble_name(a, b)
#else
static inline struct bt_conn *hostif_bt_conn_ref(struct bt_conn *conn)
{
	return NULL;
}

#define hostif_bt_conn_unref(a)
#define hostif_bt_storage_register(a)

static inline u8_t hostif_bt_read_ble_name(u8_t *name, u8_t len)
{
	return 0;
}
#endif

/** @brief set caller to negative priority
 *
 *  @param void.
 *
 *  @return caller priority.
 */
int hostif_set_negative_prio(void);

/** @brief revert caller priority
 *
 *  @param prio.
 *
 *  @return void.
 */
void hostif_revert_prio(int prio);

/************************ hostif hci core *******************************/
/** @brief Set bt device class
 *
 *  set bt device class. Must be the called before hostif_bt_enable.
 *
 *  @param classOfDevice bt device class
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_init_class(u32_t classOfDevice);

/** @brief Initialize device id
 *
 * @param device id
 *
 * @return Zero if done successfully, other indicator failed.
 */
int hostif_bt_init_device_id(u16_t *did);

/** @brief Enable Bluetooth
 *
 *  Enable Bluetooth. Must be the called before any calls that
 *  require communication with the local Bluetooth hardware.
 *
 *  @param cb Callback to notify completion or NULL to perform the
 *  enabling synchronously.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_enable(bt_ready_cb_t cb);

/** @brief Disable Bluetooth */
int hostif_bt_disable(void);

/** @brief Start BR/EDR discovery
 *
 *  Start BR/EDR discovery (inquiry) and provide results through the specified
 *  callback. When bt_br_discovery_cb_t is called it indicates that discovery
 *  has completed. If more inquiry results were received during session than
 *  fits in provided result storage, only ones with highest RSSI will be
 *  reported.
 *
 *  @param param Discovery parameters.
 *  @param cb Callback to notify discovery results.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  bt_br_discovery_cb_t cb);

/** @brief Stop BR/EDR discovery.
 *
 *  Stops ongoing BR/EDR discovery. If discovery was stopped by this call
 *  results won't be reported
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_discovery_stop(void);

/** @brief Direct set scan mode.
 *
 * Can't used bt_br_write_scan_enable and
 * bt_br_set_discoverable/bt_br_set_discoverable at same time.
 *
 *  @param scan: scan mode.
 *
 *  @return Negative if fail set to requested state or requested state has been
 *  already set. Zero if done successfully.
 */
int hostif_bt_br_write_scan_enable(u8_t scan);

/** @brief BR write iac.
 *
 * @param limit_iac, true: use limit iac, false: use general iac.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_iac(bool limit_iac);

/** @brief BR write page scan activity
 *
 * @param interval: page scan interval.
 * @param windown: page scan windown.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_page_scan_activity(u16_t interval, u16_t windown);

/** @brief BR write inquiry scan type
 *
 * @param type: 0:Standard Scan, 1:Interlaced Scan
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_inquiry_scan_type(u8_t type);

/** @brief BR write page scan type
 *
 * @param type: 0:Standard Scan, 1:Interlaced Scan
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_page_scan_type(u8_t type);

/** @brief request remote bluetooth name
 *
 * @param addr remote bluetooth mac address
 * @param cb callback function for report remote bluetooth name
 *
 * @return Zero if done successfully, other indicator failed.
 */
int hostif_bt_remote_name_request(const bt_addr_t *addr, bt_br_remote_name_cb_t cb);

/** @brief BR write inquiry scan activity
 *
 * @param interval: inquiry scan interval.
 * @param windown: inquiry scan windown.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_inquiry_scan_activity(u16_t interval, u16_t windown);

/************************ hostif conn *******************************/
/** @brief Register connection callbacks.
 *
 *  Register callbacks to monitor the state of connections.
 *
 *  @param cb Callback struct.
 */
void hostif_bt_conn_cb_register(struct bt_conn_cb *cb);

/** @brief Get connection info
 *
 *  @param conn Connection object.
 *  @param info Connection info object.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int hostif_bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info);

/** @brief Get connection type
 *
 *  @param conn Connection object.
 *
 *  @return connection type.
 */
u8_t hostif_bt_conn_get_type(const struct bt_conn *conn);

/** @brief Get connection acl handle
 *
 *  @param conn Connection object.
 *
 *  @return connection acl handle.
 */
u16_t hostif_bt_conn_get_acl_handle(const struct bt_conn *conn);

/** @brief Get acl connection by sco connection
 *
 *  @param sco_conn Connection object.
 *
 *  @return acl connection.
 */
struct bt_conn *hostif_bt_conn_get_acl_conn_by_sco(const struct bt_conn *sco_conn);

/** @brief Disconnect from a remote device or cancel pending connection.
 *
 *  Disconnect an active connection with the specified reason code or cancel
 *  pending outgoing connection.
 *
 *  @param conn Connection to disconnect.
 *  @param reason Reason code for the disconnection.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int hostif_bt_conn_disconnect(struct bt_conn *conn, u8_t reason);

/** @brief Initiate an BR/EDR connection to a remote device.
 *
 *  Allows initiate new BR/EDR link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param peer  Remote address.
 *  @param param Initial connection parameters.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *hostif_bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param);

/** @brief Check address is in connecting
 *
 *  @param peer  Remote address.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *hostif_bt_conn_br_acl_connecting(const bt_addr_t *addr);

/** @brief Initiate an SCO connection to a remote device.
 *
 *  Allows initiate new SCO link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param br_conn  br connection object.
 *
 *  @return Zero for success, non-zero otherwise..
 */
int hostif_bt_conn_create_sco(struct bt_conn *br_conn);

/** @brief Send sco data to conn.
 *
 *  @param conn  Sco connection object.
 *  @param data  Sco data pointer.
 *  @param len  Sco data length.
 *
 *  @return  Zero for success, non-zero otherwise..
 */
int hostif_bt_conn_send_sco_data(struct bt_conn *conn, u8_t *data, u8_t len);

/** @brief bluetooth force conn exit sniff
 *
 * @param conn bt_conn conn
 */
void hostif_bt_conn_force_exit_sniff(struct bt_conn *conn);

/** @brief bluetooth conn is in sniff
 *
 * @param conn bt_conn conn
 *
 * @return true if in sinff, false not in sniff.
 */
bool hostif_bt_conn_is_in_sniff(struct bt_conn *conn);

/** @brief Bt send connectionless data.
 *
 *  @param conn  Connection object.
 *  @param data send data.
 *  @param len data len.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_send_connectionless_data(struct bt_conn *conn, u8_t *data, u16_t len);

/** @brief check br conn ready for send data.
 *
 *  @param conn  Connection object.
 *
 *  @return  0: no ready , 1:  ready.
 */
int hostif_bt_br_conn_ready_send_data(struct bt_conn *conn);

/** @brief Get conn pending packet.
 *
 * @param conn  Connection object.
 * @param host_pending Host pending packet.
 * @param controler_pending Controler pending packet.
 *
 *  @return  0: sucess, other:  error.
 */
int hostif_bt_br_conn_pending_pkt(struct bt_conn *conn, u8_t *host_pending, u8_t *controler_pending);

/** @brief Bt role discovery.
 *
 *  @param conn  Connection object.
 *  @param role  return role.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_role_discovery(struct bt_conn *conn, u8_t *role);

/** @brief Bt switch role.
 *
 *  @param conn  Connection object.
 *  @param role  Switch role.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_switch_role(struct bt_conn *conn, u8_t role);

/** @brief Bt set supervision timeout.
 *
 *  @param conn  Connection object.
 *  @param timeout.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_set_supervision_timeout(struct bt_conn *conn, u16_t timeout);

/** @brief set enable/disable sniff check
 *
 *  @param enable  true: enable, false disable.
 */
void hostif_bt_conn_set_sniff_check_enable(bool enable);

/************************ hostif hfp *******************************/
/** @brief Register HFP HF call back function
 *
 *  Register Handsfree callbacks to monitor the state and get the
 *  required HFP details to display.
 *
 *  @param cb callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_register_cb(struct bt_hfp_hf_cb *cb);

/** @brief Handsfree client Send AT
 *
 *  Send specific AT commands to handsfree client profile.
 *
 *  @param conn Connection object.
 *  @param cmd AT command to be sent.
 *  @param user_cmd user command
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_send_cmd(struct bt_conn *conn, enum bt_hfp_hf_at_cmd cmd, char *user_cmd);

/** @brief Handsfree connect AG.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_connect(struct bt_conn *conn);

/** @brief Handsfree disconnect AG.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_disconnect(struct bt_conn *conn);

/************************ hostif hfp *******************************/
/** @brief Register HFP AG call back function
 *
 *  Register Handsfree callbacks to monitor the state and get the
 *  required HFP details to display.
 *
 *  @param cb callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_register_cb(struct bt_hfp_ag_cb *cb);

/** @brief Update hfp ag indicater
 *
 *  @param index (enum hfp_hf_ag_indicators ).
 *  @param value indicater value for index.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_update_indicator(u8_t index, u8_t value);

/** @brief get hfp ag indicater
 *
 *  @param index (enum hfp_hf_ag_indicators ).
 *
 *  @return 0 and positive in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_get_indicator(u8_t index);

/** @brief hfp ag send event to hf
 *
 *  @param conn  Connection object.
 *  @param event Event char string.
 * @param len Event char string length.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_send_event(struct bt_conn *conn, char *event, u16_t len);

/** @brief HFP AG connect HF.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_connect(struct bt_conn *conn);

/** @brief HFP AG disconnect HF.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_disconnect(struct bt_conn *conn);

/** @brief HFP AG display current indicators. */
void hostif_bt_hfp_ag_display_indicator(void);

/************************ hostif a2dp *******************************/
/** @brief A2DP Connect.
 *
 *  @param conn Pointer to bt_conn structure.
 *  @param role a2dp as source or sink role
 *
 *  @return 0 in case of success and error code in case of error.
 *  of error.
 */
int hostif_bt_a2dp_connect(struct bt_conn *conn, u8_t role);

/** @brief A2DP Disconnect.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return 0 in case of success and error code in case of error.
 *  of error.
 */
int hostif_bt_a2dp_disconnect(struct bt_conn *conn);

/** @brief Endpoint Registration.
 *
 *  @param endpoint Pointer to bt_a2dp_endpoint structure.
 *  @param media_type Media type that the Endpoint is.
 *  @param role Role of Endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int hostif_bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
			      u8_t media_type, u8_t role);

/** @brief halt/resume registed endpoint.
 *
 *  This function is used for halt/resume registed endpoint
 *
 *  @param endpoint Pointer to bt_a2dp_endpoint structure.
 *  @param halt true: halt , false: resume;
 *
 *  @return 0 in case of success and error code in case of error.
 */
int hostif_bt_a2dp_halt_endpoint(struct bt_a2dp_endpoint *endpoint, bool halt);

/* Register app callback */
int hostif_bt_a2dp_register_cb(struct bt_a2dp_app_cb *cb);

/* Start a2dp play */
int hostif_bt_a2dp_start(struct bt_conn *conn);

/* Suspend a2dp play */
int hostif_bt_a2dp_suspend(struct bt_conn *conn);

/* Reconfig a2dp codec config */
int hostif_bt_a2dp_reconfig(struct bt_conn *conn, struct bt_a2dp_media_codec *codec);

/* Send delay report to source, delay_time: 1/10 milliseconds */
int hostif_bt_a2dp_send_delay_report(struct bt_conn *conn, u16_t delay_time);

/* Send a2dp audio data */
int hostif_bt_a2dp_send_audio_data(struct bt_conn *conn, u8_t *data, u16_t len);

/* Get a2dp seted codec */
struct bt_a2dp_media_codec *hostif_bt_a2dp_get_seted_codec(struct bt_conn *conn);

/* Get a2dp role(source or sink) */
u8_t hostif_bt_a2dp_get_a2dp_role(struct bt_conn *conn);

/************************ hostif spp *******************************/
/* Spp send data */
int hostif_bt_spp_send_data(u8_t spp_id, u8_t *data, u16_t len);

/* Spp connect */
u8_t hostif_bt_spp_connect(struct bt_conn *conn, u8_t *uuid);

/* Spp disconnect */
int hostif_bt_spp_disconnect(u8_t spp_id);

/* Spp register service */
u8_t hostif_bt_spp_register_service(u8_t *uuid);

/* Spp register call back */
int hostif_bt_spp_register_cb(struct bt_spp_app_cb *cb);

/************************ hostif avrcp *******************************/
/* Register avrcp app callback function */
int hostif_bt_avrcp_cttg_register_cb(struct bt_avrcp_app_cb *cb);

/* Connect to avrcp TG device */
int hostif_bt_avrcp_cttg_connect(struct bt_conn *conn);

/* Disonnect avrcp */
int hostif_bt_avrcp_cttg_disconnect(struct bt_conn *conn);

/* Send avrcp control key */
int hostif_bt_avrcp_ct_pass_through_cmd(struct bt_conn *conn, avrcp_op_id opid);

/* Send avrcp control continue key(AVRCP_OPERATION_ID_REWIND, AVRCP_OPERATION_ID_FAST_FORWARD) */
int hostif_bt_avrcp_ct_pass_through_continue_cmd(struct bt_conn *conn,
										avrcp_op_id opid, bool start);

/* Target notify change to controller */
int hostif_bt_avrcp_tg_notify_change(struct bt_conn *conn, u8_t play_state);

/* get current playback track id3 info */
int hostif_bt_avrcp_ct_get_id3_info(struct bt_conn *conn);

/************************ hostif pbap *******************************/
/* Get phonebook */
u8_t hostif_bt_pbap_client_get_phonebook(struct bt_conn *conn, char *path, struct bt_pbap_client_user_cb *cb);

/* Abort get phonebook */
int hostif_bt_pbap_client_abort_get(struct bt_conn *conn, u8_t user_id);

u8_t hostif_bt_map_client_connect(struct bt_conn *conn,char *path,struct bt_map_client_user_cb *cb);

int hostif_bt_map_client_abort_get(struct bt_conn *conn, u8_t user_id);

int hostif_bt_map_client_disconnect(struct bt_conn *conn, u8_t user_id);

int hostif_bt_map_client_set_folder(struct bt_conn *conn, u8_t user_id,char *path,u8_t flags);

u8_t hostif_bt_map_client_get_message(struct bt_conn *conn, char *path, struct bt_map_client_user_cb *cb);

int hostif_bt_map_client_get_messages_listing(struct bt_conn *conn, u8_t user_id,u16_t max_list_count,u32_t parameter_mask);

int hostif_bt_map_client_get_folder_listing(struct bt_conn *conn, u8_t user_id);

/************************ hostif hid *******************************/
/* register sdp hid service record */
int hostif_bt_hid_register_sdp(struct bt_sdp_attribute * hid_attrs,int attrs_size);

/* Register hid app callback function */
int hostif_bt_hid_register_cb(struct bt_hid_app_cb *cb);

/* Connect to hid device */
int hostif_bt_hid_connect(struct bt_conn *conn);

/* Disonnect hid */
int hostif_bt_hid_disconnect(struct bt_conn *conn);

/* send hid data by ctrl channel */
int hostif_bt_hid_send_ctrl_data(struct bt_conn *conn,u8_t report_type,u8_t * data,u16_t len);

/* send hid data by intr channel */
int hostif_bt_hid_send_intr_data(struct bt_conn *conn,u8_t report_type,u8_t * data,u16_t len);

/* send hid rsp by ctrl channel */
int hostif_bt_hid_send_rsp(struct bt_conn *conn,u8_t status);

/* register device id info on sdp */
int hostif_bt_did_register_sdp(struct device_id_info *device_id);

/************************ hostif ble *******************************/
/** @brief Start advertising
 *
 *  Set advertisement data, scan response data, advertisement parameters
 *  and start advertising.
 *
 *  @param param Advertising parameters.
 *  @param ad Data to be used in advertisement packets.
 *  @param ad_len Number of elements in ad
 *  @param sd Data to be used in scan response packets.
 *  @param sd_len Number of elements in sd
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len);

/** @brief Stop advertising
 *
 *  Stops ongoing advertising.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_le_adv_stop(void);

/** @brief Update the connection parameters.
 *
 *  @param conn Connection object.
 *  @param param Updated connection parameters.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int hostif_bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param);

/** @brief Register GATT service.
 *
 *  Register GATT service. Applications can make use of
 *  macros such as BT_GATT_PRIMARY_SERVICE, BT_GATT_CHARACTERISTIC,
 *  BT_GATT_DESCRIPTOR, etc.
 *
 *  @param svc Service containing the available attributes
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_service_register(struct bt_gatt_service *svc);

/** @brief Get ATT MTU for a connection
 *
 *  Get negotiated ATT connection MTU, note that this does not equal the largest
 *  amount of attribute data that can be transferred within a single packet.
 *
 *  @param conn Connection object.
 *
 *  @return MTU in bytes
 */
u16_t hostif_bt_gatt_get_mtu(struct bt_conn *conn);

/** @brief Indicate attribute value change.
 *
 *  Send an indication of attribute value change.
 *  Note: This function should only be called if CCC is declared with
 *  BT_GATT_CCC otherwise it cannot find a valid peer configuration.
 *
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Indicate parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params);

/** @brief Notify attribute value change.
 *
 *  Send notification of attribute value change, if connection is NULL notify
 *  all peer that have notification enabled via CCC otherwise do a direct
 *  notification only the given connection.
 *
 *  The attribute object must be the so called Characteristic Value Descriptor,
 *  its usually declared with BT_GATT_DESCRIPTOR after BT_GATT_CHARACTERISTIC
 *  and before BT_GATT_CCC.
 *
 *  @param conn Connection object.
 *  @param attr Characteristic Value Descriptor attribute.
 *  @param data Pointer to Attribute data.
 *  @param len Attribute value length.
 */
int hostif_bt_gatt_notify(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   const void *data, u16_t len);

/** @brief Exchange MTU
 *
 *  This client procedure can be used to set the MTU to the maximum possible
 *  size the buffers can hold.
 *
 *  NOTE: Shall only be used once per connection.
 *
 *  @param conn Connection object.
 *  @param params Exchange MTU parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params);

/** @brief Start (LE) scanning
 *
 *  Start LE scanning with given parameters and provide results through
 *  the specified callback.
 *
 *  @param param Scan parameters.
 *  @param cb Callback to notify scan results.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb);

/** @brief Stop (LE) scanning.
 *
 *  Stops ongoing LE scanning.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_le_scan_stop(void);

/** @brief CSB interface */
int hostif_bt_csb_set_broadcast_link_key_seed(u8_t *seed, u8_t len);
int hostif_bt_csb_set_reserved_lt_addr(u8_t lt_addr);
int hostif_bt_csb_set_CSB_broadcast(u8_t enable, u8_t lt_addr, u16_t interval_min, u16_t interval_max);
int hostif_bt_csb_set_CSB_receive(u8_t enable, struct bt_hci_evt_sync_train_receive *param, u16_t timeout);
int hostif_bt_csb_write_sync_train_param(u16_t interval_min, u16_t interval_max, u32_t trainTO, u8_t service_data);
int hostif_bt_csb_start_sync_train(void);
int hostif_bt_csb_set_CSB_date(u8_t lt_addr, u8_t *data, u8_t len);
int hostif_bt_csb_receive_sync_train(u8_t *mac, u16_t timeout, u16_t windown, u16_t interval);
int hostif_bt_csb_broadcase_by_acl(u16_t handle, u8_t *data, u16_t len);

struct bt_conn *hostif_bt_conn_get_conn_by_handle(u16_t handle );

#endif /* __HOST_INTERFACE_H */
