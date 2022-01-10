/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble manager.
*/

#ifndef __BT_MANAGER_BLE_H__
#define __BT_MANAGER_BLE_H__
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

/**
 * @defgroup bt_manager_ble_apis Bt Manager Ble APIs
 * @ingroup bluetooth_system_apis
 * @{
 */

/** ble register manager structure */
struct ble_reg_manager {
	void (*link_cb)(u8_t *mac, u8_t connected);
	struct bt_gatt_service gatt_svc;
	sys_snode_t node;
};

/**
 * @brief get ble mtu
 *
 * This routine provides to get ble mtu
 *
 * @return ble mtu
 */
u16_t bt_manager_get_ble_mtu(void);

/**
 * @brief bt manager send ble data
 *
 * This routine provides to bt manager send ble data
 *
 * @param chrc_attr
 * @param des_attr 
 * @param data pointer of send data
 * @param len length of data
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_ble_send_data(struct bt_gatt_attr *chrc_attr,
					struct bt_gatt_attr *des_attr, u8_t *data, u16_t len);

/**
 * @brief ble disconnect
 *
 * This routine do ble disconnect
 *
 * @return  N/A
 */
void bt_manager_ble_disconnect(void);

/**
 * @brief ble service register
 *
 * This routine provides ble service register
 *
 * @param le_mgr ble register info 
 *
 * @return  N/A
 */
void bt_manager_ble_service_reg(struct ble_reg_manager *le_mgr);

/**
 * @brief Set ble idle interval
 *
 * This routine Set ble idle interval
 *
 * @param interval idle interval (unit: 1.25ms)
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_ble_set_idle_interval(u16_t interval);

/**
 * @brief init btmanager ble
 *
 * This routine init btmanager ble
 *
 * @return  N/A
 */
void bt_manager_ble_init(void);

/**
 * @brief notify ble a2dp play state
 *
 * This routine notify ble a2dp play state
 *
 * @param play a2dp play or stop
 *
 * @return  N/A
 */
void bt_manager_ble_a2dp_play_notify(bool play);

/**
 * @brief notify ble hfp play state
 *
 * This routine notify ble hfp play state
 *
 * @param play hfp play or stop
 *
 * @return  N/A
 */
void bt_manager_ble_hfp_play_notify(bool play);

/**
 * @brief halt ble
 *
 * This routine disable ble adv
 *
 * @return  N/A
 */
void bt_manager_halt_ble(void);

/**
 * @brief resume ble
 *
 * This routine enable ble adv
 *
 * @return  N/A
 */
void bt_manager_resume_ble(void);

/**
 * @} end defgroup bt_manager_ble_apis
 */
#endif  // __BT_MANAGER_BLE_H__
