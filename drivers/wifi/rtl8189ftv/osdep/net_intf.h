#ifndef __NET_INTF_H__
#define __NET_INTF_H__

#define MAX_ETH_MSG	1540
#define CONFIG_TX_ZERO_COPY
//----- ------------------------------------------------------------------
// Wlan Interface Provided
//----- ------------------------------------------------------------------
unsigned char rltk_wlan_check_isup(int idx);
void rltk_wlan_tx_inc(int idx);
void rltk_wlan_tx_dec(int idx);
#ifdef CONFIG_TX_ZERO_COPY
//unsigned int rltk_wlan_sec(int idx);
#endif
struct sk_buff * rltk_wlan_get_recv_skb(int idx);
#ifdef CONFIG_TX_ZERO_COPY
struct sk_buff * rltk_wlan_alloc_skb_0copy();
struct sk_buff * rltk_wlan_alloc_skb(unsigned int total_len);
#else
struct sk_buff * rltk_wlan_alloc_skb(unsigned int total_len);
#endif
void rltk_wlan_set_netif_info(int idx_wlan, void * dev, unsigned char * dev_addr);
void rltk_wlan_send_skb(int idx, struct sk_buff *skb);	//struct sk_buff as defined above comment line
unsigned char rltk_wlan_running(unsigned char idx);		// interface is up. 0: interface is down

//----- ------------------------------------------------------------------
// Network Interface provided
//----- ------------------------------------------------------------------
int netif_is_valid_IP(int idx,unsigned char * ip_dest);
int netif_tx(int idx, uint8_t *data, uint32_t len);
void netif_rx(int idx, unsigned int len);
void netif_post_sleep_processing(void);
void netif_pre_sleep_processing(void);
void zephyr_dhcp_start(void);
void zephyr_dhcp_stop(void);

#endif //#ifndef __NET_INTF_H__