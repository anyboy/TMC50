/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <kernel.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <limits.h>
#include <net/buf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include <net/net_mgmt.h>
#include <misc/printk.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <mem_manager.h>
#include <system_config.h>
#include "wifi_conf.h"
#include "wifi_api.h"
#include "../osdep/net_intf.h"


#define WIFI_NORMAL_MODE		1		/* 1: app operate wifi, 0: wifi test mode */
#define WIFI_JOIN_SCAN_TIMES	2

enum {
	WIFI_EVT_SCAN_RESULT,
	WIFI_EVT_JOIN_RESULT,
	WIFI_EVT_SCAN_DONE,
	WIFI_EVT_SCONFIG_SCAN_DONE,
	WIFI_EVT_LEAVE_RESULT,
	WIFI_EVT_STATUS
};

enum {
	WIFI_STA_DISCONNECTED,
	WIFI_STA_DISCONNECTING,
	WIFI_STA_CONNECTED,
	WIFI_STA_CONNECTING,
	WIFI_STA_INNER_LEAVE_AP
};

typedef struct {
	u8_t rssi;
	u8_t ssid_len;
	u8_t *ssid_buf;
	u8_t *pwd;
	u8_t *addr;
	s32_t state;
} wifi_result;

extern int airkiss_udp_send(void);
extern int airkiss_api_start(void);
extern int airkiss_api_stop(void);

#ifndef CONFIG_TCP_HELPER
/* rtl8189ftv sleep mode used ipReady flag
 *  ipReady  declared in TCP_HELPER,
 */
u8_t ipReady = false;
#endif

/* TODO: interface for app */
static K_MUTEX_DEFINE(cb_handle_mutex);
static wifi_evtcb_fn app_cb_handle = NULL;
static u8_t wifi_sta_state = WIFI_STA_DISCONNECTED;
static u8_t join_ap_security_mode;
static u8_t join_scan_find_ap;

void wifi_scan_ssid_cb(char *ssid, int ssid_len, u8_t security_mode, int rssi)
{
	join_ap_security_mode = security_mode;
	join_scan_find_ap = 1;
	printk("\nssid = %s security_mode:%d, rssi:%d\n", ssid, security_mode, rssi);
}

void wifi_scan_result_cb(char *ssid, u8_t ssid_len, s16_t rssi)
{
	wifi_result cb_result;

	cb_result.rssi = (u8_t)(-rssi);
	cb_result.ssid_buf = ssid;
	cb_result.ssid_len = ssid_len;

	k_mutex_lock(&cb_handle_mutex, K_FOREVER);
	if (app_cb_handle) {
		app_cb_handle(WIFI_EVT_SCAN_RESULT, &cb_result, sizeof(cb_result));
	}
	k_mutex_unlock(&cb_handle_mutex);
}

void wifi_indicate_event_cb(int event_cmd, char *buf, int buf_len, int flags)
{
	wifi_result cb_result;
	u32_t reEvt = 0xFFFF;

	memset(&cb_result, 0, sizeof(cb_result));

	switch (event_cmd) {
		case WIFI_EVENT_CONNECT:
			if (join_ap_security_mode != IW_ENCODE_ALG_CCMP) {
				reEvt = WIFI_EVT_JOIN_RESULT;
				cb_result.state = 0;
				wifi_sta_state = WIFI_STA_CONNECTED;
				//zephyr_dhcp_start();
			}
			printk("wifi connect\n");
			break;
		case WIFI_EVENT_DISCONNECT:
			if (wifi_sta_state != WIFI_STA_INNER_LEAVE_AP) {
				if (wifi_sta_state == WIFI_STA_CONNECTING) {
					reEvt = WIFI_EVT_JOIN_RESULT;
					cb_result.state = 1;
				} else {
					reEvt = WIFI_EVT_LEAVE_RESULT;
					cb_result.state = 0;
				}
				wifi_sta_state = WIFI_STA_DISCONNECTED;
				zephyr_dhcp_stop();
			}
			printk("wifi disconnect\n");
			break;
		case WIFI_EVENT_FOURWAY_HANDSHAKE_DONE:
			reEvt = WIFI_EVT_JOIN_RESULT;
			cb_result.state = 0;
			wifi_sta_state = WIFI_STA_CONNECTED;
			//zephyr_dhcp_start();
			printk("wifi Handshake done\n");
			break;
		case WIFI_EVENT_SCAN_DONE:
			reEvt = WIFI_EVT_SCAN_DONE;
			cb_result.state = 0;
			printk("Scan done\n");
			break;
		default:
			break;
	}

	if (reEvt == 0xFFFF) {
		return;
	}

#if WIFI_NORMAL_MODE
	k_mutex_lock(&cb_handle_mutex, K_FOREVER);
	if (app_cb_handle) {
		app_cb_handle(reEvt, &cb_result, sizeof(cb_result));
	}
	k_mutex_unlock(&cb_handle_mutex);
#endif
}

void wifi_airkiss_event_cb(u8_t *ssid, u8_t ssid_len, u8_t *pwd)
{
	wifi_result cb_result;

	memset(&cb_result, 0, sizeof(cb_result));
	cb_result.ssid_buf = ssid;
	cb_result.pwd = pwd;
	cb_result.ssid_len = ssid_len;
	cb_result.state = 0;

#if WIFI_NORMAL_MODE
	k_mutex_lock(&cb_handle_mutex, K_FOREVER);
	if (app_cb_handle) {
		app_cb_handle(WIFI_EVT_SCONFIG_SCAN_DONE, &cb_result, sizeof(cb_result));
	}
	k_mutex_unlock(&cb_handle_mutex);
#endif
}

void netdev_status_change_cb(void *netif)
{
#ifdef CONFIG_WIFI_AIRKISS
	/* Notify airkiss config wifi success. */
#if WIFI_NORMAL_MODE
	airkiss_udp_send();
#endif
#endif
}

int act_get_connect_ap_status(char *ssid, char *mac, int *rssi)
{
	int get_rssi = 0;

	wifi_get_rssi(&get_rssi);
	if (get_rssi == 0) {
		return -EIO;
	}

	*rssi = get_rssi;
	return 0;
}

void register_wifi_event_cb(wifi_evtcb_fn cbhandle)
{
#if WIFI_NORMAL_MODE
	k_mutex_lock(&cb_handle_mutex, K_FOREVER);
	app_cb_handle = cbhandle;
	k_mutex_unlock(&cb_handle_mutex);
#endif
}

int run_wifi_cmd_arg(int argc, char *argv[])
{
#if WIFI_NORMAL_MODE
	wifi_result cb_result;
	u32_t reEvt = 0xFFFF;
	int i;
	char *cmd_argv[4];

	if (!strcmp(argv[0], "join")) {
		for (i=0; i<WIFI_JOIN_SCAN_TIMES; i++) {
			join_ap_security_mode = 0xFF;
			join_scan_find_ap = 0;
			cmd_argv[0] = "wifi_scan_with_ssid";
			cmd_argv[1] = argv[1];
			cmd_argv[2] = "1024";
			wifi_interactive_cmd(3, cmd_argv);
			if (join_scan_find_ap) {
				break;
			}
		}

		if (!join_scan_find_ap) {
			memset(&cb_result, 0, sizeof(cb_result));
			reEvt = WIFI_EVT_JOIN_RESULT;
			cb_result.state = 1;
			k_mutex_lock(&cb_handle_mutex, K_FOREVER);
			if (app_cb_handle) {
				app_cb_handle(reEvt, &cb_result, sizeof(cb_result));
			}
			k_mutex_unlock(&cb_handle_mutex);

			wifi_sta_state = WIFI_STA_DISCONNECTED;
			return -EIO;
		}

		wifi_sta_state = WIFI_STA_INNER_LEAVE_AP;
		cmd_argv[0] = "wifi_disconnect";
		wifi_interactive_cmd(1, cmd_argv);
		wifi_sta_state = WIFI_STA_CONNECTING;

		if (join_ap_security_mode == IW_ENCODE_ALG_WEP) {
			cmd_argv[0] = "wifi_connect";
			cmd_argv[1] = argv[1];
			cmd_argv[2] = argv[2];
			cmd_argv[3] = "0";
			return wifi_interactive_cmd(4, cmd_argv);
		}else if (join_ap_security_mode == IW_ENCODE_ALG_NONE) {
			if (strlen(argv[2]) == 0) {
				cmd_argv[0] = "wifi_connect";
				cmd_argv[1] = argv[1];
				return wifi_interactive_cmd(2, cmd_argv);
			} else {
				memset(&cb_result, 0, sizeof(cb_result));
				reEvt = WIFI_EVT_JOIN_RESULT;
				cb_result.state = 1;
				k_mutex_lock(&cb_handle_mutex, K_FOREVER);
				if (app_cb_handle) {
					app_cb_handle(reEvt, &cb_result, sizeof(cb_result));
				}
				k_mutex_unlock(&cb_handle_mutex);

				wifi_sta_state = WIFI_STA_DISCONNECTED;
				return -EIO;
			}
		} else {
			cmd_argv[0] = "wifi_connect";
			cmd_argv[1] = argv[1];
			cmd_argv[2] = argv[2];
			return wifi_interactive_cmd(3, cmd_argv);
		}
	} else if (!strcmp(argv[0], "scan")) {
		cmd_argv[0] = "wifi_scan";
		return wifi_interactive_cmd(1, cmd_argv);
	} else {
		printk("%s, wait to support cmd: %s\n", __func__, argv[0]);
	}
#endif

	return 0;
}

int run_wifi_cmd(char *CmdBuffer)
{
#if WIFI_NORMAL_MODE
	char *argv[2];

	if (!strcmp(CmdBuffer, "scan")) {
		argv[0] = "wifi_scan";
		return wifi_interactive_cmd(1, argv);
	} else if (!strcmp(CmdBuffer, "leave")) {
		wifi_sta_state = WIFI_STA_DISCONNECTING;
		argv[0] = "wifi_disconnect";
		return wifi_interactive_cmd(1, argv);
#ifdef CONFIG_WIFI_AIRKISS
	} else if (!strcmp(CmdBuffer, "airkiss")) {
		/* Ensure disconnect  before enter airkiss */
		u8_t pre_wifi_sta = wifi_sta_state;
		wifi_sta_state = WIFI_STA_INNER_LEAVE_AP;
		argv[0] = "wifi_disconnect";
		wifi_interactive_cmd(1, argv);
		wifi_sta_state = pre_wifi_sta;

		/* Enter airkiss */
		return airkiss_api_start();
#endif
	} else if (!strcmp(CmdBuffer, "stamode")) {
#ifdef CONFIG_WIFI_AIRKISS
		/* Exit airkiss */
		return airkiss_api_stop();
#endif
	} else {
		printk("%s, wait to support cmd: %s\n", __func__, CmdBuffer);
	}
#endif

	return 0;
}

#if (WIFI_NORMAL_MODE == 0)
static int rtl_wifi_cmd(int argc, char *argv[])
{
	return wifi_interactive_cmd((argc -1), &argv[1]);
}

extern void wifi_mem_debug_print(void);

static int wifi_debug_mem(int argc, char *argv[])
{
	wifi_mem_debug_print();
	return 0;
}

static int wifi_airkiss_test(int argc, char *argv[])
{
	if (!strcmp("start", argv[1])) {
		airkiss_api_start();
	} else if (!strcmp("stop", argv[1])) {
		airkiss_api_stop();
	}

	return 0;
}

#define WIFI_CMD_SHELL		"wifi"

static struct shell_cmd wifi_commands[] = {
	{ "cmd", rtl_wifi_cmd, "wifi cmd"},
	{ "debug", wifi_debug_mem, "wifi debug mem"},
#ifdef CONFIG_WIFI_AIRKISS
	{ "airkiss", wifi_airkiss_test, "airkiss test"},
#endif
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(WIFI_CMD_SHELL, wifi_commands);
#endif
