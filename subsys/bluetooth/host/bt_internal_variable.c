/** @file bt_internal_variable.c
 * @brief Bluetooth internel variables.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hfp_hf.h>
#include <bluetooth/a2dp.h>
#include <bluetooth/avrcp_cttg.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "rfcomm_internal.h"
#include "at.h"
#include "hfp_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"
#include "avrcp_internal.h"
#include "hid_internal.h"

#include "bt_internal_variable.h"

/* For controler acl tx
 * must large than(le acl data pkts + br acl data pkts)
 */
#define BT_ACL_TX_MAX		CONFIG_BT_CONN_TX_MAX
#define BT_BR_SIG_COUNT		(CONFIG_BT_MAX_BR_CONN*2)
#define BT_BR_RESERVE_PKT	3		/* Avoid send data used all pkts */
#define BT_LE_RESERVE_PKT	1		/* Avoid send data used all pkts */

/* rfcomm */
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
#define RFCOMM_MAX_CREDITS_BUF         (CONFIG_BT_ACL_RX_COUNT - 1)
#else
#define RFCOMM_MAX_CREDITS_BUF         (CONFIG_BT_RX_BUF_COUNT - 1)
#endif

#if (RFCOMM_MAX_CREDITS_BUF > 7)
#define RFCOMM_MAX_CREDITS			7
#else
#define RFCOMM_MAX_CREDITS			RFCOMM_MAX_CREDITS_BUF
#endif

#ifdef CONFIG_BT_AVRCP_VOL_SYNC
#define CONFIG_SUPPORT_AVRCP_VOL_SYNC		1
#else
#define CONFIG_SUPPORT_AVRCP_VOL_SYNC		0
#endif

#ifdef CONFIG_BT_PTS_TEST
#define BT_PTS_TEST_MODE		1
#else
#define BT_PTS_TEST_MODE		0
#endif

#if (CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3)
#define BT_STACK_DEBUG_LOG		1
#else
#define BT_STACK_DEBUG_LOG		0
#endif

#ifdef CONFIG_BT_RFCOMM
#define BT_RFCOMM_L2CAP_MTU		CONFIG_BT_RFCOMM_L2CAP_MTU
#else
#define BT_RFCOMM_L2CAP_MTU		CONFIG_BT_L2CAP_RX_MTU
#endif

/* hfp */
#define HFP_POOL_COUNT		(CONFIG_BT_MAX_BR_CONN*2)
#define SDP_CLIENT_DISCOVER_USER_BUF_LEN		256

/* Bt rx/tx data pool size */
#define BT_DATA_POOL_RX_SIZE		(4*1024)		/* TODO: Better  calculate by CONFIG */
#define BT_DATA_POOL_TX_SIZE		(4*1024)		/* TODO: Better  calculate by CONFIG */

const struct bt_inner_value_t bt_inner_value = {
	.max_conn = CONFIG_BT_MAX_CONN,
	.br_max_conn = CONFIG_BT_MAX_BR_CONN,
	.br_reserve_pkts = BT_BR_RESERVE_PKT,
	.le_reserve_pkts = BT_LE_RESERVE_PKT,
	.acl_tx_max = BT_ACL_TX_MAX,
	.rfcomm_max_credits = RFCOMM_MAX_CREDITS,
	.avrcp_vol_sync = CONFIG_SUPPORT_AVRCP_VOL_SYNC,
	.pts_test_mode = BT_PTS_TEST_MODE,
	.debug_log = BT_STACK_DEBUG_LOG,
	.l2cap_tx_mtu = CONFIG_BT_L2CAP_TX_MTU,
	.rfcomm_l2cap_mtu = BT_RFCOMM_L2CAP_MTU,
	.avdtp_rx_mtu = BT_AVDTP_MAX_MTU,
	.hf_features = BT_HFP_HF_SUPPORTED_FEATURES,
	.ag_features = BT_HFP_AG_SUPPORTED_FEATURES,
};

BT_DATA_POOL_DEFINE(host_rx_pool, 1, BT_DATA_POOL_RX_SIZE, &continue_data_pool_opt);
BT_DATA_POOL_DEFINE(host_tx_pool, 1, BT_DATA_POOL_TX_SIZE, &continue_data_pool_opt);

BT_BUF_POOL_DEFINE(acl_tx_pool, CONFIG_BT_L2CAP_TX_BUF_COUNT,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_L2CAP_TX_USER_DATA_SIZE, NULL, &host_tx_pool);

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
BT_BUF_POOL_DEFINE(br_sig_pool, BT_BR_SIG_COUNT,
		    BT_L2CAP_BUF_SIZE(L2CAP_BR_MIN_MTU),
		    BT_BUF_USER_DATA_MIN, NULL, &host_tx_pool);

/* hfp */
BT_BUF_POOL_DEFINE(hfp_pool, HFP_POOL_COUNT,
			 BT_RFCOMM_BUF_SIZE(BT_HF_CLIENT_MAX_PDU),
			 BT_BUF_USER_DATA_MIN, NULL, &host_tx_pool);

BT_BUF_POOL_DEFINE(sdp_client_discover_pool, CONFIG_BT_MAX_BR_CONN,
			 SDP_CLIENT_DISCOVER_USER_BUF_LEN, BT_BUF_USER_DATA_MIN, NULL, &host_tx_pool);

/* L2cap br */
struct bt_l2cap_br bt_l2cap_br_pool[CONFIG_BT_MAX_BR_CONN]__in_section_unique(bthost_bss);

/* conn */
struct bt_conn_tx conn_tx_link[BT_ACL_TX_MAX] __in_section_unique(bthost_bss);
struct bt_conn conns[CONFIG_BT_MAX_CONN] __in_section_unique(bthost_bss);
/* CONFIG_BT_MAX_SCO_CONN == CONFIG_BT_MAX_BR_CONN */
struct bt_conn sco_conns[CONFIG_BT_MAX_SCO_CONN] __in_section_unique(bthost_bss);

/* hfp, Wait todo: Can use union to manager  hfp_hf_connection and hfp_ag_connection, for reduce memory */
struct bt_hfp_hf hfp_hf_connection[CONFIG_BT_MAX_BR_CONN] __in_section_unique(bthost_bss);
struct bt_hfp_ag hfp_ag_connection[CONFIG_BT_MAX_BR_CONN] __in_section_unique(bthost_bss);

/* a2dp */
struct bt_avdtp_conn avdtp_conn[CONFIG_BT_MAX_BR_CONN] __in_section_unique(bthost_bss);

/* avrcp */
struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_BR_CONN] __in_section_unique(bthost_bss);

/* rfcomm */
struct bt_rfcomm_session bt_rfcomm_connection[CONFIG_BT_MAX_BR_CONN] __in_section_unique(bthost_bss);

/* hid */
struct bt_hid_conn hid_conn __in_section_unique(bthost_bss);

void bt_internal_pool_init(void)
{
	net_data_pool_reinitializer(&host_rx_pool);
	net_data_pool_reinitializer(&host_tx_pool);

	net_buf_pool_reinitializer(&acl_tx_pool, CONFIG_BT_L2CAP_TX_BUF_COUNT);
	net_buf_pool_reinitializer(&br_sig_pool, BT_BR_SIG_COUNT);
	net_buf_pool_reinitializer(&hfp_pool, HFP_POOL_COUNT);
	net_buf_pool_reinitializer(&sdp_client_discover_pool, CONFIG_BT_MAX_BR_CONN);
}
