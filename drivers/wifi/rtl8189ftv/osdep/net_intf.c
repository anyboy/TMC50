/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *
 * This program is mem_free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#include <zephyr.h>
#include <net/buf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include "../app/wifi_conf.h"
#include "rtl8189ftv.h"
#include "net_intf.h"

#define NET_RX_WAIT_TIMEOUT			2000	/* 2s */

unsigned int g_ap_sta_num = 0; //from 1-6
int netif_tx(int idx, uint8_t *data, uint32_t len)
{
	int ret = -EIO;

	struct sk_buff *skb = NULL;
	if(idx == -1){
		SYS_LOG_ERR("netif is DOWN");
		return ret;
	}

	save_and_cli();
	if(rltk_wlan_check_isup(idx)) {
		rltk_wlan_tx_inc(idx);
	} else {
		SYS_LOG_ERR("netif is DOWN");
		restore_flags();
		return ret;
	}
	restore_flags();

#ifdef CONFIG_TX_ZERO_COPY
	/* rltk_wlan_sec(), get encryption mode,  only open and AES ncryption mode use zero copy */
	//if(rltk_wlan_sec(idx) == 0x0 || rltk_wlan_sec(idx) == 0x4){
	if (true) {
		skb = rltk_wlan_alloc_skb_0copy();
		if (skb == NULL) {
			SYS_LOG_ERR("rltk_wlan_alloc_skb()failed!");
			goto exit;
		}
		skb->data = data;
		skb->head = data - ETHERNET_WIFI_RESERVE;
		skb->end = data + 1640 - 80;
		skb->tail = data + len;
		skb->len = len;
		ret = 0;
	} else {
		skb = rltk_wlan_alloc_skb(len);
		if (skb == NULL) {
			SYS_LOG_ERR("rltk_wlan_alloc_skb() for data len=%d failed!", len);
			goto exit;
		}

		//printk("netif_tx, len:%d\n", len);
		rtw_memcpy(skb->tail, (void *)data, len);
		skb_put(skb,  len);
		ret = 0;
	}
#else
	skb = rltk_wlan_alloc_skb(len);
	if (skb == NULL) {
		SYS_LOG_ERR("rltk_wlan_alloc_skb() for data len=%d failed!", len);
		goto exit;
	}

	//printk("netif_tx, len:%d\n", len);
	rtw_memcpy(skb->tail, (void *)data, len);
	skb_put(skb,  len);
	ret = 0;
#endif
	rltk_wlan_send_skb(idx, skb);

exit:
	save_and_cli();
	rltk_wlan_tx_dec(idx);
	restore_flags();

	return ret;
}

void netif_rx(int idx, unsigned int len)
{
	struct net_pkt *pkt = NULL;
	struct net_buf *frag;
	uint8_t *data_ptr;
	struct sk_buff *skb;
	s32_t wait_time = NET_RX_WAIT_TIMEOUT;
	u32_t pre_time;

	if(!rltk_wlan_running(idx)) {
		SYS_LOG_ERR("Wifi not running\n");
		return;
	}

	if ((len > MAX_ETH_MSG) || (len < 0))
		len = MAX_ETH_MSG;

	pre_time = k_uptime_get_32();
	frag = net_pkt_get_reserve_rx_data(0, wait_time);
	if (!frag) {
		SYS_LOG_ERR("Could not allocate data buffer");
		return;
	}

	wait_time = NET_RX_WAIT_TIMEOUT - (k_uptime_get_32() - pre_time);
	wait_time = (wait_time > 0)? wait_time : 10;
	pkt = net_pkt_get_reserve_rx(0, wait_time);
	if (!pkt) {
		SYS_LOG_ERR("Cannot allocate pbuf to receive packet");
		net_pkt_frag_unref(frag);
		return;
	}

	frag->data += ETHERNET_WIFI_RESERVE;
	data_ptr = frag->data;
	frag->len = len;
	net_pkt_frag_add(pkt, frag);

	skb = rltk_wlan_get_recv_skb(idx);
	rtw_memcpy((void *)data_ptr, skb->data, len);
	skb_pull(skb, len);

	//printk("netif_rx, len:%d\n", len);
	rtl8189ftv_recv_cb(pkt);
}

void rltk_wlan_set_netif_info(int idx_wlan, void *dev, unsigned char *dev_addr)
{
	rtl8189ftv_set_netif_info(idx_wlan, dev, dev_addr);
}

int netif_is_valid_IP(int idx, unsigned char *ip_dest)
{
	return rtl8189ftv_is_valid_IP(idx, ip_dest);
}

void zephyr_dhcp_start(void)
{
	rtl8189ftv_dhcpc_start();
}

void zephyr_dhcp_stop(void)
{
	rtl8189ftv_dhcpc_stop();
}

/* do nothing */
void netif_post_sleep_processing(void)
{
}

/* do nothing */
void netif_pre_sleep_processing(void)
{
}

unsigned char *rltk_wlan_get_ip(int idx)
{
	return rtl8189ftv_get_ip(idx);
}

/* Do nothing */
void Set_WLAN_Power_On(void)
{
}

/* Do nothing */
void Set_WLAN_Power_Off(void)
{
}

