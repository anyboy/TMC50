/** @file bt_internal_variable.h
 * @brief Bluetooth internel variables.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_INTERNAL_VARIABLE_H__
#define __BT_INTERNAL_VARIABLE_H__

struct bt_inner_value_t {
	u32_t max_conn:4;
	u32_t br_max_conn:3;
	u32_t br_reserve_pkts:3;
	u32_t le_reserve_pkts:3;
	u32_t acl_tx_max:5;
	u32_t rfcomm_max_credits:4;
	u32_t avrcp_vol_sync:1;
	u32_t pts_test_mode:1;
	u32_t debug_log:1;
	u16_t l2cap_tx_mtu;
	u16_t rfcomm_l2cap_mtu;
	u16_t avdtp_rx_mtu;
	u16_t hf_features;
	u16_t ag_features;
};

extern const struct bt_inner_value_t bt_inner_value;
#define bt_internal_is_pts_test()		bt_inner_value.pts_test_mode
#define bt_internal_debug_log()			bt_inner_value.debug_log

void bt_internal_pool_init(void);
#endif /* __BT_INTERNAL_VARIABLE_H__ */
