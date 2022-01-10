/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
*/

#ifndef __BT_MANAGER_INNER_H__
#define __BT_MANAGER_INNER_H__
#include <stream.h>

void bt_manager_record_halt_phone(void);

void *bt_manager_get_halt_phone(u8_t *halt_cnt);

void bt_manager_startup_reconnect(void);

void bt_manager_disconnected_reason(void *param);

int bt_manager_a2dp_profile_start(void);

int bt_manager_a2dp_profile_stop(void);

int bt_manager_avrcp_profile_start(void);

int bt_manager_avrcp_profile_stop(void);

int bt_manager_hfp_profile_start(void);

int bt_manager_hfp_profile_stop(void);

int bt_manager_hfp_ag_profile_start(void);

int bt_manager_hfp_ag_profile_stop(void);

int bt_manager_hfp_sco_start(void);

int bt_manager_hfp_sco_stop(void);

int bt_manager_spp_profile_start(void);

int bt_manager_spp_profile_stop(void);

int bt_manager_hid_profile_start(void);

int bt_manager_hid_profile_stop(void);

int bt_manager_hid_register_sdp();

int bt_manager_did_register_sdp();

void bt_manager_tws_init(void);

int bt_manager_hfp_init(void);

int bt_manager_hfp_ag_init(void);

int bt_manager_hfp_sco_init(void);

u8_t bt_manager_config_get_tws_limit_inquiry(void);
u8_t bt_manager_config_get_tws_compare_high_mac(void);
u8_t bt_manager_config_get_tws_compare_device_id(void);
u8_t bt_manager_config_get_idle_extend_windown(void);
void bt_manager_config_set_pre_bt_mac(u8_t *mac);
void bt_manager_updata_pre_bt_mac(u8_t *mac);
bool bt_manager_config_enable_tws_sync_event(void);
u8_t bt_manager_config_connect_phone_num(void);
bool bt_manager_config_support_a2dp_aac(void);
bool bt_manager_config_pts_test(void);
u16_t bt_manager_config_volume_sync_delay_ms(void);
u32_t bt_manager_config_bt_class(void);
u16_t *bt_manager_config_get_device_id(void);
int bt_manager_config_expect_tws_connect_role(void);
#endif