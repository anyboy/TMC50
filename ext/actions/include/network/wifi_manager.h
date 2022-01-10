/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file wifi manager interface 
 */

#ifndef __WIFI_MANGER_H__
#define __WIFI_MANGER_H__

/**
 * @defgroup wifi_manager_apis app wifi Manager APIs
 * @ingroup msg_managers
 * @{
 */

/** Callbacks to report wifi manager's events*/
typedef enum {
	/** notifying that wifi connected*/
	WIFI_EVENT_CONNECTED,
	/** notifying that wifi disconnected*/
	WIFI_EVENT_DISCONNECTED,
} wifi_event_e;

typedef void (*wifi_event_callback)(wifi_event_e event);

/**
 * @brief wifi manager init
 *
 * This routine calls init wifi manager
 *
 * @return true success ,false failed
 */

bool wifi_mgr_init(wifi_event_callback callback);

/**
 * @brief wifi connect to target ssid ap
 *
 * This routine calls wifi connected to target ssid ap used password.
 *
 *
 * @return 0 connected success
 * @return others connected failed
 */

int wifi_mgr_connect_to(char * ssid, char * password);
/**
 * @brief wifi cancel current connecting
 *
 * This routine calls cancel current connecting
 *
 *
 * @return 0 cancel success
 * @return others cancel failed
 */

int wifi_mgr_connect_cancel(void);

/**
 * @brief wifi disconnected current connected ap
 *
 * This routine calls disconnected current connected ap , can't called by ISR.
 *
 * @return 0 set success
 * @return others set failed
 */

int wifi_mgr_disconnect(void);

/**
 * @brief get wifi modules mac addr
 *
 * This routine calls get wifi modules mac addr , can't called by ISR.
 * @param mac pointer to store wifi mac addr
 *
 * @return 0 set success
 * @return others set failed
 */

int wifi_mgr_get_mac_addr(char *mac);

/**
 * @brief get wifi modules mac addr
 *
 * This routine calls get wifi modules mac addr , can't called by ISR.
 * @param ipaddr pointer to store ip addr
 * @param size size of ipaddr buffer
 *
 * @return 0 set success
 * @return others set failed
 */

int wifi_mgr_get_ip_addr(char *ipaddr, int size);

/**
 * @brief get connected ap' ssid
 *
 * This routine calls get connected ap' ssid , can't called by ISR.
 * @param ssid the pointer to store ssid
 *
 * @return 0 set success
 * @return others set failed
 */

int wifi_mgr_get_connected_ssid(char *ssid);

/**
 * @brief get WiFi signal intensity
 *
 * This routine calls get WiFi signal intensity , can't called by ISR.
 * signal intensity rang is -20dbm ~ -100dbm
 *
 * @return 0 get failed
 * @return -20dbm ~ -100dbm wifi signal intensity
 */

int wifi_mgr_get_rssi(void);

/**
 * @} end defgroup wifi_manager_apis
 */
#endif