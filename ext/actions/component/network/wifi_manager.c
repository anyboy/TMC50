/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file wifi manager interface
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <net/net_mgmt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(CONFIG_NETWORKING)
#include <net/net_core.h>
#include <net/net_ip.h>
#define NET_LOG_ENABLED 1
#include "net_private.h"
#endif

#define MAX_RETRY_TIMES  2

typedef struct
{
	uint8_t rssi;
	uint8_t ssid_len;
	uint8_t *ssid_buf;
	uint8_t *pwd;
	uint8_t *addr;
	int state;
}callback_result;

enum
{
	WIFI_EVT_SCAN_RESULT,
	WIFI_EVT_JOIN_RESULT,
	WIFI_EVT_SCAN_DONE,
	WIFI_EVT_SCONFIG_SCAN_DONE,
	WIFI_EVT_LEAVE_RESULT,
	WIFI_EVT_STATUS,
};

enum
{
	WIFI_STATUS_UNKNOW = 0,
	WIFI_CONNECTING,
	WIFI_CONNECT_SUCCEEDED,
	WIFI_CONNECT_FAILED,
	WIFI_IP_OK,
	WIFI_DISCONNECTING,
	WIFI_DISCONNECTED,
	WIFI_DISCONNECT_FAILED,
};

struct wifi_manager_info
{
	os_sem wifi_state_sem;
	struct net_mgmt_event_callback mgmt_cb;
	wifi_event_callback callback;
	u8_t wifi_state;
};

static struct wifi_manager_info * wifi_mgr = NULL;

extern void netdev_status_change_cb(struct net_if *netif);
extern void register_wifi_event_cb(void (*evtcb)(unsigned int nEvtId, void *data, unsigned int len));
extern int run_wifi_cmd(char *CmdBuffer);
extern int run_wifi_cmd_arg(int argc, char *argv[]);
extern void net_notify_dhcp_ssid(char *ssid);
extern int act_get_connect_ap_status(char *ssid, char *mac, int *rssi);

static void mgmt_event_handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
	struct net_if_addr *unicast;

	switch (mgmt_event) {
		case NET_EVENT_IPV4_ADDR_ADD:
			if(wifi_mgr)
			{
				netdev_status_change_cb(iface);
				if(wifi_mgr->wifi_state == WIFI_CONNECT_SUCCEEDED)
				{
					if(wifi_mgr->callback)
					{
						wifi_mgr->callback(WIFI_EVENT_CONNECTED);
					}
					wifi_mgr->wifi_state = WIFI_IP_OK;
				}
			}
			break;
		case NET_EVENT_IPV4_ADDR_DEL:
			/* Do nothing, mqtt ping timeout will trigger reconnect */
			break;
		case NET_EVENT_IF_UP:
			break;
		case NET_EVENT_IF_DOWN:
			if (wifi_mgr && (
					wifi_mgr->wifi_state == WIFI_CONNECT_SUCCEEDED ||
					wifi_mgr->wifi_state == WIFI_DISCONNECTING ||
					wifi_mgr->wifi_state == WIFI_IP_OK))
			{
				if(wifi_mgr->callback)
				{
					wifi_mgr->callback(WIFI_EVENT_DISCONNECTED);
				}

				wifi_mgr->wifi_state = WIFI_DISCONNECTED;
			}
			break;
		default:
			break;
	}

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		SYS_LOG_DBG("Link addr : %s\n", net_sprint_ll_addr(iface->link_addr.addr,
							      iface->link_addr.len));

		unicast = &iface->ipv4.unicast[0];
		SYS_LOG_INF("IP addr:%s\n", (unicast->is_used)?
				net_sprint_ipv4_addr(&unicast->address.in_addr) : "none");

		SYS_LOG_DBG("IPv4 gateway : %s\n",
		       net_sprint_ipv4_addr(&iface->ipv4.gw));
		SYS_LOG_DBG("IPv4 netmask : %s\n",
		       net_sprint_ipv4_addr(&iface->ipv4.netmask));
		SYS_LOG_DBG("DHCPv4 lease time : %d\n", iface->dhcpv4.lease_time);
		SYS_LOG_DBG("DHCPv4 renew time : %d\n", iface->dhcpv4.renewal_time);
	}
}

static void __wifi_event_cb(unsigned int evt_id, void *data, unsigned int len)
{
	callback_result *result = (callback_result *)data;

	switch (evt_id) {
		case WIFI_EVT_JOIN_RESULT:
			SYS_LOG_INF("Join ap %s!\n", (0 == result->state)? "success" : "failed");
			if(result->state != 0)
			{
				wifi_mgr->wifi_state = WIFI_CONNECT_FAILED;
			}
			else
			{
				wifi_mgr->wifi_state = WIFI_CONNECT_SUCCEEDED;
			}
			os_sem_give(&wifi_mgr->wifi_state_sem);
			break;
		case WIFI_EVT_SCAN_DONE:
			SYS_LOG_INF("Scan %s!\n", (0 == result->state)? "done" : "failed");
			break;
		case WIFI_EVT_LEAVE_RESULT:
			SYS_LOG_INF("leave %s!\n", (0 == result->state)? "done" : "failed");
			os_sem_give(&wifi_mgr->wifi_state_sem);
			break;
		default:
			SYS_LOG_DBG("Don't card event:%d\n", evt_id);
			break;
	}
}

bool wifi_mgr_init(wifi_event_callback callback)
{
	if(wifi_mgr == NULL)
	{
		wifi_mgr = mem_malloc(sizeof(struct wifi_manager_info));
	}

	if(wifi_mgr == NULL)
	{
		return false;
	}

	os_sem_init(&wifi_mgr->wifi_state_sem, 0, 1);

#ifdef CONFIG_TCP_HELPER
	tcp_client_init();
#endif

	net_mgmt_init_event_callback(&wifi_mgr->mgmt_cb, mgmt_event_handler,
				     (NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL |
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN));

	net_mgmt_add_event_callback(&wifi_mgr->mgmt_cb);

	wifi_mgr->wifi_state = WIFI_STATUS_UNKNOW;
	wifi_mgr->callback = callback;
	return true;
}

int wifi_mgr_connect_to(char * ssid, char * password)
{
	int try_time = MAX_RETRY_TIMES;
	int rc = 0;
	char *argv[3];

	register_wifi_event_cb(__wifi_event_cb);

RETRY:

	argv[0] = "join";
	argv[1] = ssid;
	argv[2] = password;

	os_sem_reset(&wifi_mgr->wifi_state_sem);

	wifi_mgr->wifi_state = WIFI_CONNECTING;

	net_notify_dhcp_ssid(ssid);

	run_wifi_cmd_arg(3, argv);

	if(os_sem_take(&wifi_mgr->wifi_state_sem, OS_MSEC(20000)))
	{
		SYS_LOG_INF("wifi connect timeout !!!\n");
	}

	if(wifi_mgr->wifi_state == WIFI_CONNECT_SUCCEEDED)
	{
		SYS_LOG_INF("wifi connect success !!!\n");
		rc = 0;
		goto END;
	}
	else
	{
		SYS_LOG_INF("wifi connect failed "
					"ssid %s psw %s  !!!\n",
					ssid, password);
		rc = -ETIMEDOUT;
		try_time --;

		if(try_time > 0 
			&& ((wifi_mgr->wifi_state == WIFI_CONNECTING)
			   || (wifi_mgr->wifi_state == WIFI_CONNECT_FAILED))){
			goto RETRY;
		}
	}

END:
	register_wifi_event_cb(NULL);
	return rc;
}

int wifi_mgr_connect_cancel(void)
{
	if(wifi_mgr->wifi_state == WIFI_CONNECTING)
	{
		wifi_mgr->wifi_state = WIFI_STATUS_UNKNOW;
		os_sem_give(&wifi_mgr->wifi_state_sem);
	}
	return 0;
}

int wifi_mgr_disconnect(void)
{
	SYS_LOG_INF("wifi disconnect begin wifi_mgr->wifi_state %d !!!\n",wifi_mgr->wifi_state);
	register_wifi_event_cb(NULL);
	if(wifi_mgr->wifi_state == WIFI_CONNECT_SUCCEEDED
		|| wifi_mgr->wifi_state == WIFI_IP_OK)
	{
		register_wifi_event_cb(__wifi_event_cb);
		os_sem_reset(&wifi_mgr->wifi_state_sem);
		wifi_mgr->wifi_state = WIFI_DISCONNECTING;
		run_wifi_cmd("leave");

		if(os_sem_take(&wifi_mgr->wifi_state_sem, OS_MSEC(20000)))
		{
			SYS_LOG_ERR("wifi disconnect timeout !!!\n");
			register_wifi_event_cb(NULL);
			return -1;
		}
		register_wifi_event_cb(NULL);
	}
	SYS_LOG_INF("wifi disconnect finised !!!\n");
	return 0;
}

int wifi_mgr_get_mac_addr(char *mac)
{
	struct net_if *iface = net_if_get_default();
	if(iface != NULL)
	{
		if(nvram_config_get(CFG_BT_MAC, mac, 6) <= 0) {
			memcpy(mac, iface->link_addr.addr, 6);
		}
		return 0;
	}
	return -1;
}

int wifi_mgr_get_ip_addr(char *ipaddr, int size)
{
	struct net_if *iface = net_if_get_default();
	struct net_if_addr *unicast = NULL;

	if(iface != NULL)
	{
		unicast = &iface->ipv4.unicast[0];
		snprintf(ipaddr, size,"%s",(unicast->is_used)? (char *)net_sprint_ipv4_addr(&unicast->address.in_addr) : "none");
		return 0;
	}
	return -1;
}

int wifi_mgr_get_connected_ssid(char *ssid)
{
	int rssi = 0;
	if(act_get_connect_ap_status(ssid,NULL,&rssi) != 0)
	{
		rssi = 0;
	}
	return !rssi;
}

int wifi_mgr_get_rssi(void)
{
	int rssi = 0;
	if(act_get_connect_ap_status(NULL,NULL,&rssi) != 0)
	{
		rssi = 0;
	}
	return rssi;
}