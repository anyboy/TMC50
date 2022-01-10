#ifndef _UTIL_H
#define _UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

int wext_get_ssid(const char *ifname, __u8 *ssid);
int wext_set_ssid(const char *ifname, const __u8 *ssid, __u16 ssid_len);
int wext_set_bssid(const char *ifname, const __u8 *bssid);
int wext_set_auth_param(const char *ifname, __u16 idx, __u32 value);
int wext_set_key_ext(const char *ifname, __u16 alg, const __u8 *addr, int key_idx, int set_tx, const __u8 *seq, __u16 seq_len, __u8 *key, __u16 key_len);
int wext_get_enc_ext(const char *ifname, __u16 *alg, __u8 *key_idx, __u8 *passphrase);
int wext_set_passphrase(const char *ifname, const __u8 *passphrase, __u16 passphrase_len);
int wext_get_passphrase(const char *ifname, __u8 *passphrase);
int wext_disconnect(const char *ifname);
int wext_set_mode(const char *ifname, int mode);
int wext_get_mode(const char *ifname, int *mode);
int wext_set_ap_ssid(const char *ifname, const __u8 *ssid, __u16 ssid_len);
int wext_set_country(const char *ifname, char *country_code);
int wext_get_rssi(const char *ifname, int *rssi);
int wext_set_channel(const char *ifname, __u8 ch);
int wext_get_channel(const char *ifname, __u8 *ch);
int wext_register_multicast_address(const char *ifname, rtw_mac_t *mac);
int wext_unregister_multicast_address(const char *ifname, rtw_mac_t *mac);
int wext_set_scan(const char *ifname, char *buf, __u16 buf_len, __u16 flags);
int wext_get_scan(const char *ifname, char *buf, __u16 buf_len);
int wext_set_mac_address(const char *ifname, char * mac);
int wext_get_mac_address(const char *ifname, char * mac);
int wext_enable_powersave(const char *ifname);
int wext_disable_powersave(const char *ifname);
int wext_get_txpower(const char *ifname, int *cckpoweridx, int *ofdmpoweridx);
int wext_set_txpower(const char *ifname, int bcck, int poweridx);
int wext_get_associated_client_list(const char *ifname, void * client_list_buffer, __u16 buffer_length);
int wext_get_ap_info(const char *ifname, rtw_bss_info_t * ap_info, rtw_security_t* security);
int wext_mp_command(const char *ifname, char *cmd, int show_msg);
int wext_private_command(const char *ifname, char *cmd, int show_msg);
void wext_wlan_indicate(unsigned int cmd, union iwreq_data *wrqu, char *extra);
int wext_set_pscan_channel(const char * ifname,__u8 * ch,__u8 length);
int wext_set_autoreconnect(const char *ifname, __u8 mode);
int wext_get_autoreconnect(const char *ifname, __u8 *mode);
#ifdef CONFIG_CUSTOM_IE
int wext_add_custom_ie(const char *ifname, void * cus_ie, int ie_num);
int wext_update_custom_ie(const char *ifname, void * cus_ie, int ie_index);
int wext_del_custom_ie(const char *ifname);
#endif
int wext_del_station(const char *ifname, unsigned char* hwaddr);
int wext_get_auto_chl(const char *ifname, int low_chl, int hi_chl);

#define wext_handshake_done rltk_wlan_handshake_done

#ifdef CONFIG_P2P_NEW
int wext_send_mgnt(const char *ifname, char *buf, __u16 buf_len, __u16 flags);
#endif
int wext_set_gen_ie(const char *ifname, char *buf, __u16 buf_len, __u16 flags);

#ifdef	__cplusplus
}
#endif

#endif /* _UTIL_H */
