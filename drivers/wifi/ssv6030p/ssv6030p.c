/* ssv6030p.c - SSV6030P driver using uart_pipe. This is meant for
 * network connectivity between host and qemu. The host will
 * need to run tunssv6030p process.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(CONFIG_SSV6030P_DEBUG)
#define SYS_LOG_DOMAIN "ssv6030p"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>
#include <stdio.h>
#endif

#include <kernel.h>
#include <device.h>
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <misc/util.h>

#ifdef CONFIG_NETWORKING
#include <net/buf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include <net/net_mgmt.h>
#include "system_config.h"

#include "ssv6030p.h"

/* log_level only used by ssv6030p driver lib */
int log_level = CONFIG_SYS_LOG_DEFAULT_LEVEL;

struct ssv6030p_context {
	bool init_done;
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;
};

bool ssv_wifi_init_done;

#if defined(CONFIG_SSV6030P_DEBUG)
#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
#define COLOR_OFF     "\x1B[0m"
#define COLOR_YELLOW  "\x1B[0;33m"
#else
#define COLOR_OFF     ""
#define COLOR_YELLOW  ""
#endif

static void hexdump(const char *str, const uint8_t *packet,
		    size_t length, size_t ll_reserve)
{
	int n = 0;

	if (!length) {
		SYS_LOG_DBG("%s zero-length packet", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printf("%s %08X ", str, n);
		}

#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
		if (n < ll_reserve) {
			printf(COLOR_YELLOW);
		} else {
			printf(COLOR_OFF);
		}
#endif
		printf("%02X ", *packet++);

#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
		if (n < ll_reserve) {
			printf(COLOR_OFF);
		}
#endif
		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printf("\n");
			} else {
				printf(" ");
			}
		}
	}

	if (n % 16) {
		printf("\n");
	}
}
#else
#define hexdump(ssv6030p, str, packet, length, ll_reserve)
#endif

static struct ssv6030p_context ssv6030p_context_data;

static int ssv6030p_send(struct net_if *iface, struct net_pkt *pkt)
{
	uint16_t len;

	if (!ssv_wifi_init_done || !pkt->frags) {
		net_pkt_unref(pkt);
		/* If unref pkt, must return 0, or net_if_tx will unref again */
		return 0;
	}

	wifi_exit_ps(true);
	len = net_pkt_ll_reserve(pkt) + net_pkt_get_len(pkt);

	//SYS_LOG_DBG("tx:%d\n", len);
	netstack_output(iface, pkt, len);
	return 0;
}

#define WIFI_INPUT_ALLOC_TIMEOUT	1500	/* 1500ms */
#define ETH_HDR_LEN		(sizeof(struct net_eth_hdr))
/* Reserve one rx pkt for mqtt/tcp ack/udp */
static K_MUTEX_DEFINE(reserve_rx_pkt_mutex);
static u16_t mqtt_port;

static void ssv6030p_get_mqtt_port(void)
{
#ifdef CONFIG_NVRAM_CONFIG
	int port;
	char temp[8];
	memset(temp,0,sizeof(temp));

	int ret = nvram_config_get(CFG_MQTT_SERVER_PORT,temp, sizeof(temp));
	if (ret < 0)
	{
		port = 0;
	} else {
		port = atoi(temp);
	}

	mqtt_port = (u16_t)port;
#else
	mqtt_port = 0;
#endif
}

static struct net_pkt *ssv6030p_get_net_rx_pkt(u8_t *data, size_t len)
{
	static struct net_pkt *reserv_pkt = NULL;
	static s32_t time_out = WIFI_INPUT_ALLOC_TIMEOUT;
	struct net_eth_hdr *eth_hdr;
	struct net_pkt *pkt = NULL;
	struct net_ipv4_hdr *hdr;
	u16_t tcp_head_len, src_port;

	k_mutex_lock(&reserve_rx_pkt_mutex, K_FOREVER);

	if (!reserv_pkt) {
		reserv_pkt = net_pkt_get_reserve_rx(0, WIFI_INPUT_ALLOC_TIMEOUT);
	}

	if (!reserv_pkt) {
		goto get_exit;
	}

	eth_hdr = (struct net_eth_hdr *)data;
	if (ntohs(eth_hdr->type) != NET_ETH_PTYPE_IP) {
		pkt = reserv_pkt;
		reserv_pkt = NULL;
		goto get_exit;
	}

	hdr = (struct net_ipv4_hdr *)&data[ETH_HDR_LEN];
	if (hdr->proto != IPPROTO_TCP) {
		pkt = reserv_pkt;
		reserv_pkt = NULL;
		goto get_exit;
	}

	/* tcp packet, source port */
	src_port = (data[ETH_HDR_LEN + NET_IPV4H_LEN] << 8) + data[ETH_HDR_LEN + NET_IPV4H_LEN + 1];
	/* tcp head: 2 byte src port, 2 byte dst port, 4 byte seq, 4byte ack, offset */
	tcp_head_len = ((data[ETH_HDR_LEN + NET_IPV4H_LEN + 2 + 2 + 4 + 4] & 0xF0) >> 4)*4;

	if ((len == (ETH_HDR_LEN + NET_IPV4H_LEN + tcp_head_len)) ||
		(src_port == mqtt_port)) {
		/* Tcp packet without data or mqtt packet */
		pkt = reserv_pkt;
		reserv_pkt = NULL;
		goto get_exit;
	}

	pkt = net_pkt_get_reserve_rx(0, time_out);
	if (pkt) {
		time_out = WIFI_INPUT_ALLOC_TIMEOUT;
	} else if (time_out > 500) {
		time_out >>= 1;
	}

get_exit:
	k_mutex_unlock(&reserve_rx_pkt_mutex);
	return pkt;
}

int ssv6030p_recv_cb(void *data, size_t len, uint32_t hdr_offse)
{
	struct ssv6030p_context *ssv6030p = &ssv6030p_context_data;
	struct net_pkt *pkt;
	struct net_buf *frag;
	int res;

	if (!ssv6030p->init_done) {
		goto free_data;
	}

	/* For reset wifi power save timer counter */
	wifi_exit_ps(false);

	pkt = ssv6030p_get_net_rx_pkt(data, len);
	if (!pkt) {
		SYS_LOG_WRN("Can't net_pkt_get_reserve_rx\n");
		goto free_data;
	}

	frag = (struct net_buf *)((char *)data - hdr_offse - sizeof(struct net_buf));
	frag->data = data;
	frag->len = len;
	net_pkt_frag_add(pkt, frag);

	net_pktbuf_set_owner(pkt);
	res = net_recv_data(ssv6030p->iface, pkt);
	if (res < 0) {
		SYS_LOG_DBG("%s", (res == (-ENODATA))? "pkt without frag" : "net iface not ready");
		net_pkt_unref(pkt);		/* This unref pkt and frag, just return 0 */
	}

	return 0;

free_data:
	return -1;
}

static inline struct net_linkaddr *ssv6030p_get_mac(struct ssv6030p_context *ssv6030p)
{
    extern int netdev_getmacaddr(const char *ifname, unsigned char *macaddr);

	netdev_getmacaddr(0, ssv6030p->mac_addr);

	ssv6030p->ll_addr.addr = ssv6030p->mac_addr;
	ssv6030p->ll_addr.len = sizeof(ssv6030p->mac_addr);

	return &ssv6030p->ll_addr;
}


#define _SSV6030P_L2_LAYER ETHERNET_L2
#define _SSV6030P_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)
#define _SSV6030P_MTU 1500

static void ssv6030p_iface_init(struct net_if *iface)
{
	struct ssv6030p_context *ssv6030p = net_if_get_device(iface)->driver_data;
	struct net_linkaddr *ll_addr = NULL;

	if (!ssv_wifi_init_done) {
		return;
	}

	if (ssv6030p)
	{
		ll_addr = ssv6030p_get_mac(ssv6030p);
		ssv6030p->init_done = true;
		ssv6030p->iface = iface;

		net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len,
				     NET_LINK_ETHERNET);
	}
	else
	{
		printk("%s %d: ssv6030p_iface_init error\n", __func__, __LINE__);
	}
}

static struct net_if_api ssv6030p_if_api = {
	.init = ssv6030p_iface_init,
	.send = ssv6030p_send,
};

void ssv6030p_dhcpc_start(struct net_if *iface)
{
	ssv_optimize_rf_parameter();
	net_dhcpv4_start(iface);
}

void ssv6030p_dhcpc_stop(struct net_if *iface)
{
	net_dhcpv4_stop(iface);
}

void ssv6030p_iface_get_ipaddr(uint32_t *ipaddr, uint32_t *mask, uint32_t *gw, uint32_t *dns)
{
	struct net_if_addr *unicast;
    struct net_if *iface;
	struct ssv6030p_context *ssv6030p = &ssv6030p_context_data;
	if (ssv6030p && ssv6030p->iface)
	{
        iface = ssv6030p->iface;
        unicast = &(iface->ipv4.unicast[0]);
        if (unicast->is_used)
        {
            if (ipaddr)
            {
                *ipaddr = unicast->address.in_addr.in4_u.u4_addr32[0];
            }
        }

        if (mask)
        {
            *mask = iface->ipv4.netmask.in4_u.u4_addr32[0];
        }

        if (gw)
        {
            *gw = iface->ipv4.gw.in4_u.u4_addr32[0];
        }

        if (dns)
        {
            *dns = 0; // to be done
        }
	}
}

void ssv6030p_iface_link_up(void)
{
	struct ssv6030p_context *ssv6030p = &ssv6030p_context_data;
	if (ssv6030p)
	{
		printk("%s %d: ssv6030p_dhcpc_start\n", __func__, __LINE__);
        net_if_up(ssv6030p->iface);
		ssv6030p_dhcpc_start(ssv6030p->iface);
	}
}

void ssv6030p_iface_link_down(void)
{
	struct ssv6030p_context *ssv6030p = &ssv6030p_context_data;
	if (ssv6030p)
	{
		printk("%s %d: ssv6030p_dhcpc_stop\n", __func__, __LINE__);
        net_if_down(ssv6030p->iface);
		ssv6030p_dhcpc_stop(ssv6030p->iface);
	}
}

#endif

static int ssv6030p_init(struct device *dev)
{
	ssv6030p_get_mqtt_port();

	printk("%s %d: ssv6xxx_dev_init\n", __func__, __LINE__);
#ifdef CONFIG_USE_WIFI_HEAP_MEM
    vPortInit();
    printk("############# use heap_5 memory \r\n");
#else
    printk("############# use zerphy memory \r\n");
#endif

	ssv_wifi_init_done = false;

	if (0 != ssv6xxx_dev_init(0)) {
		printk("ssv6030p  no ready\r\n");
		return -1;
	}

	while (!netmgr_wifi_check_sta())
	{
		OS_TickDelay(1);
	}
	printk("ssv6030p ready \r\n");

	ssv_wifi_init_done = true;
	actions_cli_init();
	return 0;
}

#ifdef CONFIG_NETWORKING
NET_DEVICE_INIT(ssv6030p, "ssv6030p", ssv6030p_init, &ssv6030p_context_data,
		NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ssv6030p_if_api,
		_SSV6030P_L2_LAYER, _SSV6030P_L2_CTX_TYPE, _SSV6030P_MTU);


#else
DEVICE_INIT(ssv6030p, "ssv6030p", ssv6030p_init,
		NULL, NULL, POST_KERNEL, 0);
#endif

