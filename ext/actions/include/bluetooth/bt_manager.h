/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
*/

#ifndef __BT_MANAGER_H__
#define __BT_MANAGER_H__
#include <stream.h>
#include "btservice_api.h"
#include "bt_manager_ble.h"
#include <msg_manager.h>

/**
 * @brief system APIs
 * @defgroup bluetooth_system_apis Bluetooth system APIs
 * @{
 * @}
 */

/**
 * @defgroup bt_manager_apis Bt Manager APIs
 * @ingroup bluetooth_system_apis
 * @{
 */
#ifdef CONFIG_BT_MAX_BR_CONN
#define BT_MANAGER_MAX_BR_NUM	CONFIG_BT_MAX_BR_CONN
#else
#define BT_MANAGER_MAX_BR_NUM 1
#endif

/**bt stream type*/
typedef enum {
    STREAM_TYPE_A2DP,
	STREAM_TYPE_LOCAL,
	STREAM_TYPE_SCO,
    STREAM_TYPE_SPP,
    STREAM_TYPE_MAX_NUM,
} bt_stream_type_e;

/**bt event type*/
typedef enum {
	 /* param null*/
    BT_CONNECTION_EVENT = 2,
	 /* dev_addr,param :reason*/
    BT_DISCONNECTION_EVENT,
	 /* param null*/
    BT_A2DP_CONNECTION_EVENT = 4,
	 /* dev_addr,param :reason*/
    BT_A2DP_DISCONNECTION_EVENT,
	/* param null*/
    BT_A2DP_STREAM_START_EVENT,
	 /* param null*/
    BT_A2DP_STREAM_SUSPEND_EVENT,
	 /* param null*/
    BT_A2DP_STREAM_DATA_IND_EVENT,
	 /* param null*/
    BT_HFP_CONNECTION_EVENT = 10,
	 /* param null*/
    BT_HFP_DISCONNECTION_EVENT,
	 /* param null*/
    BT_HFP_ESCO_ESTABLISHED_EVENT = 13,
	 /* param null*/
    BT_HFP_ESCO_RELEASED_EVENT,
	/* param hfp_active_id*/
    BT_HFP_ACTIVEDEV_CHANGE_EVENT,
	/* param:NULL*/
	BT_HFP_CALL_RING_STATR_EVENT,
	/* param:NULL*/
	BT_HFP_CALL_RING_STOP_EVENT,
	/* param:NULL*/
	BT_HFP_CALL_OUTGOING,
	/* param:NULL*/
	BT_HFP_CALL_INCOMING,
	/* param:NULL*/
	BT_HFP_CALL_ONGOING,
	/* param:NULL*/
	BT_HFP_CALL_SIRI_MODE,
	/* param:NULL*/
	BT_HFP_CALL_HUNGUP,
	/* param:NULL*/
	BT_HFP_SIRI_START,
	/* param:NULL*/
	BT_HFP_SIRI_STOP,
	 /* param null*/
    BT_AVRCP_CONNECTION_EVENT,
	 /* param null*/
    BT_AVRCP_DISCONNECTION_EVENT,
	/*param :avrcp_ui.h 6.7 Notification PDUs*/
    BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT,
    /*param :null*/
    BT_AVRCP_TRACK_CHANGED_EVENT,
    /*param : struct id3_info avrcp.h*/
    BT_AVRCP_UPDATE_ID3_INFO_EVENT,
	/* param null*/
    BT_HID_CONNECTION_EVENT,
	/* param null*/
    BT_HID_DISCONNECTION_EVENT,
	/* param hid_active_id*/
    BT_HID_ACTIVEDEV_CHANGE_EVENT,
	/* param null*/
	BT_RMT_VOL_SYNC_EVENT,
	 /* param null*/
    BT_TWS_CONNECTION_EVENT,
	 /* param null*/
    BT_TWS_DISCONNECTION_EVENT,
	/* param null*/
	BT_TWS_CHANNEL_MODE_SWITCH,
	/* param null*/
    BT_REQ_RESTART_PLAY,

	/* This value will transter to different tws version, must used fixed value */
    /* param null*/
	BT_TWS_START_PLAY = 0xE0,
	/* param null*/
	BT_TWS_STOP_PLAY = 0xE1,
} bt_event_type_e;

/** bt play state */
enum BT_PLAY_STATUS {
    BT_STATUS_NONE                  = 0x0100,
    BT_STATUS_WAIT_PAIR             = 0x0200,
	BT_STATUS_CONNECTED			= 0x0400,
	BT_STATUS_DISCONNECTED		= 0x0800,
	BT_STATUS_TWS_WAIT_PAIR         = 0x1000,
	BT_STATUS_TWS_PAIRED            = 0x2000,
	BT_STATUS_TWS_UNPAIRED          = 0x4000,
	BT_STATUS_MASTER_WAIT_PAIR      = 0x8000,

    BT_STATUS_PAUSED                = 0x0001,
    BT_STATUS_PLAYING               = 0x0002,
	BT_STATUS_INCOMING              = 0x0004,
	BT_STATUS_OUTGOING              = 0x0008,
	BT_STATUS_ONGOING               = 0x0010,
	BT_STATUS_MULTIPARTY            = 0x0020,
	BT_STATUS_SIRI                  = 0x0040,
	BT_STATUS_SIRI_STOP             = 0xFFBF,
	BT_STATUS_3WAYIN                = 0x0080,
};


/** bt manager battery report mode*/
enum BT_BATTERY_REPORT_MODE_E {
	/** bt manager battery report mode init*/
    BT_BATTERY_REPORT_INIT = 1,
	/** bt manager battery report mode report data*/
    BT_BATTERY_REPORT_VAL,
};

#define BT_STATUS_A2DP_ALL  (BT_STATUS_NONE \
			    | BT_STATUS_WAIT_PAIR \
			    | BT_STATUS_MASTER_WAIT_PAIR \
			    | BT_STATUS_PAUSED \
			    | BT_STATUS_PLAYING)

#define BT_STATUS_HFP_ALL   (BT_STATUS_INCOMING | BT_STATUS_OUTGOING \
				| BT_STATUS_ONGOING | BT_STATUS_MULTIPARTY | BT_STATUS_SIRI | BT_STATUS_3WAYIN)

/**bt manager spp call back*/
struct btmgr_spp_cb {
	void (*connect_failed)(u8_t channel);
	void (*connected)(u8_t channel, u8_t *uuid);
	void (*disconnected)(u8_t channel);
	void (*receive_data)(u8_t channel, u8_t *data, u32_t len);
};

/**bt manager spp ble strea init param*/
struct sppble_stream_init_param {
	u8_t *spp_uuid;
	void *gatt_attr;
	u8_t attr_size;
	void *tx_chrc_attr;
	void *tx_attr;
	void *tx_ccc_attr;
	void *rx_attr;
	void *connect_cb;
	s32_t read_timeout;
	s32_t write_timeout;
};

struct mgr_pbap_result {
	u8_t type;
	u16_t len;
	u8_t *data;
};

struct mgr_map_result {
	u8_t type;
	u16_t len;
	u8_t *data;
};

struct btmgr_pbap_cb {
	void (*connect_failed)(u8_t app_id);
	void (*connected)(u8_t app_id);
	void (*disconnected)(u8_t app_id);
	void (*result)(u8_t app_id, struct mgr_pbap_result *result, u8_t size);
};

struct btmgr_map_cb {
	void (*connect_failed)(u8_t app_id);
	void (*connected)(u8_t app_id);
	void (*disconnected)(u8_t app_id);
	void (*result)(u8_t app_id, struct mgr_map_result *result, u8_t size);
};

/**
 * @brief bt manager init
 *
 * This routine init bt manager
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_init(void);

/**
 * @brief bt manager deinit
 *
 * This routine deinit bt manager
 *
 * @return N/A
 */
void bt_manager_deinit(void);

/**
 * @brief bt manager check inited
 *
 * This routine check bt manager is inited
 *
 * @return true if inited others false
 */

bool bt_manager_is_inited(void);

/**
 * @brief dump bt manager info
 *
 * This routine dump bt manager info
 *
 * @return N/A
 */
void bt_manager_dump_info(void);

/**
 * @brief bt manager get bt device state
 *
 * This routine provides to get bt device state.
 *
 * @return state of bt device state
 */
int bt_manager_get_status(void);

/**
 * @brief bt manager allow sco connect
 *
 * This routine provides to allow sco connect or not
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_allow_sco_connect(bool allowed);

/**
 * @brief get profile a2dp codec id
 *
 * This routine provides to get profile a2dp codec id .
 *
 * @return codec id of a2dp profile
 */
int bt_manager_a2dp_get_codecid(void);
/**
 * @brief get profile a2dp sample rate
 *
 * This routine provides to get profile a2dp sample rate.
 *
 * @return sample rate of profile a2dp
 */
int bt_manager_a2dp_get_sample_rate(void);

/**
 * @brief check a2dp state
 *
 * This routine use by app to check a2dp playback
 * state,if state is playback ,trigger start event
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_a2dp_check_state(void);

/**
 * @brief Send delay report to a2dp active phone
 *
 * This routine Send delay report to a2dp active phone
 *
 * @param delay_time delay time (unit: 1/10 milliseconds)
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_a2dp_send_delay_report(u16_t delay_time);

/**
 * @brief Control Bluetooth to start playing through AVRCP profile
 *
 * This routine provides to Control Bluetooth to start playing through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_play(void);
/**
 * @brief Control Bluetooth to stop playing through AVRCP profile
 *
 * This routine provides to Control Bluetooth to stop playing through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_stopplay(void);
/**
 * @brief Control Bluetooth to pause playing through AVRCP profile
 *
 * This routine provides to Control Bluetooth to pause playing through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_pause(void);
/**
 * @brief Control Bluetooth to play next through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play next through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_play_next(void);
/**
 * @brief Control Bluetooth to play previous through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play previous through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_play_previous(void);
/**
 * @brief Control Bluetooth to play fast forward through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play fast forward through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_fast_forward(bool start);
/**
 * @brief Control Bluetooth to play fast backward through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play fast backward through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_fast_backward(bool start);
/**
 * @brief Control Bluetooth to sync vol to remote through AVRCP profile
 *
 * This routine provides to Control Bluetooth to sync vol to remote through AVRCP profile.
 *
 * @param vol volume want to sync
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_sync_vol_to_remote(u32_t vol);

/**
 * @brief Control Bluetooth to get current playback track's id3 info through AVRCP profile
 *
 * This routine provides to Control Bluetooth to get id3 info of cur track.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_get_id3_info();

/**
 * @brief get hfp status
 *
 * This routine provides to get hfp status .
 *
 * @return state of hfp status.
 */
int bt_manager_hfp_get_status(void);

/**
 * @brief Control Bluetooth to dial target phone number through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to dial target phone number through HFP profile .
 *
 * @param number number of target phone
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_dial_number(u8_t *number);
/**
 * @brief Control Bluetooth to dial last phone number through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to dial last phone number through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_dial_last_number(void);
/**
 * @brief Control Bluetooth to dial local memory phone number through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to dial local memory phone number through HFP profile .
 *
 * @param location index of local memory phone number
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_dial_memory(int location);
/**
 * @brief Control Bluetooth to volume control through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to volume control through HFP profile .
 *
 * @param type type of opertation
 * @param volume level of volume
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_volume_control(u8_t type, u8_t volume);
/**
 * @brief Control Bluetooth to report battery state through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to report battery state through HFP profile .
 *
 * @param mode mode of operation, 0 init mode , 1 report mode
 * @param bat_val battery level value
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_battery_report(u8_t mode, u8_t bat_val);
/**
 * @brief Control Bluetooth to accept call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to accept call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_accept_call(void);
/**
 * @brief Control Bluetooth to reject call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to reject call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_reject_call(void);
/**
 * @brief Control Bluetooth to hangup current call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hangup current call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_hangup_call(void);
/**
 * @brief Control Bluetooth to hangup another call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hangup another call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_hangup_another_call(void);
/**
 * @brief Control Bluetooth to hold current and answer another call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hold current and answer another call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_holdcur_answer_call(void);
/**
 * @brief Control Bluetooth to hangup current and answer another call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hangup current and answer another call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_hangupcur_answer_call(void);
/**
 * @brief Control Bluetooth to start siri through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to start siri through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_start_siri(void);
/**
 * @brief Control Bluetooth to stop siri through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to stop siri through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_stop_siri(void);

/**
 * @brief Control Bluetooth to sycn time from phone through HFP profile
 *
 * This routine provides to sycn time from phone through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_get_time(void);

/**
 * @brief send user define at command through HFP profile
 *
 * This routine provides to send user define at command through HFP profile .
 *
 * @param command user define at command
 * @param send cmd to active_call or inactive call
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_send_at_command(u8_t *command,u8_t active_call);
/**
 * @brief sync volume through HFP profile
 *
 * This routine provides to sync volume through HFP profile .
 *
 * @param vol volume want to sync
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_sync_vol_to_remote(u32_t vol);
/**
 * @brief switch sound source through HFP profile
 *
 * This routine provides to sswitch sound source HFP profile .

 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_switch_sound_source(void);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief set hfp status
 *
 * This routine provides to set hfp status .
 *
 * @param state state of hfp profile
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_set_status(u32_t state);
/**
 * @brief get profile hfp codec id
 *
 * This routine provides to get profile hfp codec id .
 *
 * @return codec id of hfp profile
 */
/**
 * @cond INTERNAL_HIDDEN
 */
int bt_manager_sco_get_codecid(void);

/**
 * @brief get profile hfp sample rate
 *
 * This routine provides to get profile hfp sample rate.
 *
 * @return sample rate of profile hfp
 */
int bt_manager_sco_get_sample_rate(void);

/**
 * @brief get profile hfp call state
 *
 * This routine provides to get profile hfp call state.
 * @param active_call to get active call or inactive call
 *
 * @return call state of call
 */
int bt_manager_hfp_get_call_state(u8_t active_call);

/**
 * @brief set bt call indicator by app
 *
 * This routine provides to set bt call indicator by app through HFP profile .
 * @param index  call status index to set (enum btsrv_hfp_hf_ag_indicators)
 * @param value  call status
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_ag_update_indicator(enum btsrv_hfp_hf_ag_indicators index, u8_t value);

/**
 * @brief send call event
 *
 * This routine provides to send call event through HFP profile .
 * @param event  call event
 * @param len  event len
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_ag_send_event(u8_t *event, u16_t len);

/**
 * @brief spp reg uuid
 *
 * This routine provides to spp reg uuid, return channel id.
 *
 * @param uuid uuid of spp link
 * @param c call back of spp link
 *
 * @return channel of spp connected
 */
int bt_manager_spp_reg_uuid(u8_t *uuid, struct btmgr_spp_cb *c);
/**
 * @brief spp send data
 *
 * This routine provides to send data througth target spp link
 *
 * @param chl channel of spp link
 * @param data pointer of send data
 * @param len length of send data
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_spp_send_data(u8_t chl, u8_t *data, u32_t len);
/**
 * @brief spp connect
 *
 * This routine provides to connect spp channel
 *
 * @param uuid uuid of spp connect
 *
 * @return 0 excute successed , others failed
 */
u8_t bt_manager_spp_connect(bd_address_t *bd, u8_t *uuid, struct btmgr_spp_cb *spp_cb);

/**
 * @brief spp disconnect
 *
 * This routine provides to disconnect spp channel
 *
 * @param chl channel of spp connect
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_spp_disconnect(u8_t chl);

/**
 * @brief PBAP get phonebook
 *
 * This routine provides to get phonebook
 *
 * @param bd device bluetooth address
 * @param path phonebook path
 * @param cb callback function
 *
 * @return 0 failed, others success with app_id
 */
u8_t btmgr_pbap_get_phonebook(bd_address_t *bd, char *path, struct btmgr_pbap_cb *cb);

/**
 * @brief abort phonebook get
 *
 * This routine provides to abort phonebook get
 *
 * @param app_id app_id
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_abort_get(u8_t app_id);

/**
 * @brief hid send data on intr channel
 *
 * This routine provides to send data throug intr channel
 *
 * @param report_type input or output
 * @param data pointer of send data
 * @param len length of send data
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hid_send_intr_data(u8_t report_type, u8_t *data, u32_t len);

/**
 * @brief hid send data on ctrl channel
 *
 * This routine provides to send data throug ctrl channel
 *
 * @param report_type input or output
 * @param data pointer of send data
 * @param len length of send data
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hid_send_ctrl_data(u8_t report_type, u8_t *data, u32_t len);

/**
 * @brief hid send take photo key msg on intr channel
 *
 * This routine provides to hid send take photo key msg on intr channel
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hid_take_photo(void);

/**
 * @brief bt manager get connected phone device num
 *
 * This routine provides to get connected device num.
 *
 * @return number of connected device
 */
int bt_manager_get_connected_dev_num(void);

/**
 * @brief bt manager clear connected list
 *
 * This routine provides to clear connected list
 * if have bt device connected , disconnect it before clear connected list.
 *
 * @return N/A
 */
void bt_manager_clear_list(int mode);

/**
 * @brief bt manager set stream type
 *
 * This routine set stream type to bt manager
 *
 * @return N/A
 */
void bt_manager_set_stream_type(u8_t stream_type);

/**
 * @brief bt manager set codec
 *
 * This routine set codec to tws, only need by set a2dp stream
 *
 * @return N/A
 */
void bt_manager_set_codec(u8_t codec);

/**
 * @brief Start br disover
 *
 * This routine Start br disover.
 *
 * @param disover parameter
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_br_start_discover(struct btsrv_discover_param *param);

/**
 * @brief Stop br disover
 *
 * This routine Stop br disover.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_br_stop_discover(void);

/**
 * @brief connect to target bluetooth device
 *
 * This routine connect to target bluetooth device with bluetooth mac addr.
 *
 * @param bd bluetooth addr of target bluetooth device
 *                 Mac low address store in low memory address
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_br_connect(bd_address_t *bd);

/**
 * @brief disconnect to target bluetooth device
 *
 * This routine disconnect to target bluetooth device with bluetooth mac addr.
 *
 * @param bd bluetooth addr of target bluetooth device
 *                 Mac low address store in low memory address
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_br_disconnect(bd_address_t *bd);

/**
 * @brief bt manager lock stream pool
 *
 * This routine provides to unlock stream pool
 *
 * @return N/A
 */

void bt_manager_stream_pool_lock(void);
/**
 * @brief bt manager unlock stream pool
 *
 * This routine provides to unlock stream pool
 *
 * @return N/A
 */
void bt_manager_stream_pool_unlock(void);
/**
 * @brief bt manager set target type stream enable or disable
 *
 * This routine provides to set target type stream enable or disable
 * @param type type of stream
 * @param enable true :set stream enable false: set steam disable
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_stream_enable(int type, bool enable);
/**
 * @brief bt manager check target type stream enable
 *
 * This routine provides to check target type stream enable
 * @param type type of stream
 *
 * @return ture stream is enable
 * @return false stream is not enable
 */
bool bt_manager_stream_is_enable(int type);
/**
 * @brief bt manager set target type stream
 *
 * This routine provides to set target type stream
 * @param type type of stream
 * @param stream handle of stream
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_set_stream(int type, io_stream_t stream);
/**
 * @brief bt manager get target type stream
 *
 * This routine provides to get target type stream
 * @param type type of stream
 *
 * @return handle of stream
 */
io_stream_t bt_manager_get_stream(int type);

/**
 * @brief tws wait pair
 *
 * This routine provides to make device enter to tws pair modes
 *
 * @return N/A
 */
void bt_manager_tws_wait_pair(void);

/**
 * @brief tws get device role
 *
 * This routine provides to get tws device role
 *
 * @return device role of tws
 */
int bt_manager_tws_get_dev_role(void);

/**
 * @brief tws get peer version
 *
 * This routine provides to get tws peer version
 *
 * @return tws peer version
 */
u8_t bt_manager_tws_get_peer_version(void);

/**
 * @brief Create a new spp ble stream
 *
 * This routine Create a new spp ble stream
 * @param init param for spp ble stream
 *
 * @return  handle of stream
 */
io_stream_t sppble_stream_create(void *param);


/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief bt manager state notify
 *
 * This routine provides to notify bt state .
 *
 * @param state  state of bt device.
 *
 * @return 0 excute successed , others failed
 */

int bt_manager_state_notify(int state);

/**
 * @brief bt manager event notify
 *
 * This routine provides to notify bt event .
 *
 * @param event_id  id of bt event
 * @param event_data param of bt event
 * @param event_data_size param size of bt event
 *
 * @return 0 excute successed , others failed
 */

int bt_manager_event_notify(int event_id, void *event_data, int event_data_size);

/**
 * @brief bt manager event notify
 *
 * This routine provides to notify bt event .
 *
 * @param event_id  id of bt event
 * @param event_data param of bt event
 * @param event_data_size param size of bt event
 * @param call_cb param callback function to free mem 
 *
 * @return 0 excute successed , others failed
 */

int bt_manager_event_notify_ext(int event_id, void *event_data, int event_data_size , MSG_CALLBAK call_cb);

/**
 * @brief bt manager set bt device state
 *
 * This routine provides to set bt device state.
 * @param state
 *
 * @return 0 excute successed , others failed
 */
 #define TWS_TIMETO_BT_CLOCK(a)            ((a) * 10000 / 3125)
int bt_manager_set_status(int state);
void bt_manager_check_mac_name(void);
void bt_manager_tws_sync_volume_to_slave(u32_t media_type, u32_t music_vol);
void bt_manager_tws_send_command(u8_t *command, int command_len);
void bt_manager_tws_send_sync_command(u8_t *command, int command_len);
void bt_manager_tws_send_event(u8_t event, u32_t event_param);
void bt_manager_tws_notify_start_play(u8_t media_type, u8_t codec_id, u8_t sample_rate);
void bt_manager_tws_notify_stop_play(void);
void bt_manager_sco_set_codec_info(u8_t codec_id, u8_t sample_rate);
tws_runtime_observer_t *bt_manager_tws_get_runtime_observer(void);
void bt_manager_tws_send_event_sync(u8_t event, u32_t event_param);
int bt_manager_allow_sco_connect(bool allowed);
void bt_manager_halt_phone(void);
void bt_manager_resume_phone(void);
void bt_manager_set_connectable(bool connectable);
bool bt_manager_is_auto_reconnect_runing(void);
int bt_manager_get_linkkey(struct bt_linkkey_info *info, u8_t cnt);
int bt_manager_update_linkkey(struct bt_linkkey_info *info, u8_t cnt);
int bt_manager_write_ori_linkkey(bd_address_t *addr, u8_t *link_key);

bool bt_manager_tws_check_is_woodpecker(void);
bool bt_manager_tws_check_support_feature(u32_t feature);
void bt_manager_tws_set_stream_type(u8_t stream_type);
void bt_manager_tws_set_codec(u8_t codec);
void bt_manager_tws_disconnect_and_wait_pair(void);
void bt_manager_tws_channel_mode_switch(void);
void bt_manager_tws_set_output_stream(io_stream_t stream);
void bt_manager_tws_start_callout(u8_t codec);
void bt_manager_tws_stop_call();
int bt_manager_tws_is_hfp_mode();
void bt_manager_tws_disconnect(void);
u32_t bt_manager_tws_get_bt_clock(bt_clock_t* bt_clock);
void bt_manager_tws_get_sco_bt_clk(bt_clock_t * bt_clk,int * count);

u8_t btmgr_map_client_connect(bd_address_t *bd, char *path, struct btmgr_map_cb *cb);
u8_t btmgr_map_client_set_folder(u8_t app_id,char *path, u8_t flags);
int btmgr_map_get_folder_listing(u8_t app_id);
int btmgr_map_get_messages_listing(u8_t app_id,u16_t max_cn,u32_t mask);
int btmgr_map_abort_get(u8_t app_id);
int btmgr_map_client_disconnect(u8_t app_id);
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup bt_manager_apis
 */
#endif

