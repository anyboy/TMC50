#include <zephyr.h>
#include <mem_manager.h>
#include "wifi_conf.h"


int iw_ioctl(const char * ifname, unsigned long request, struct iwreq *	pwrq)
{
	memcpy(pwrq->ifr_name, ifname, 5);
	return rltk_wlan_control(request, (void *) pwrq);
}

int wext_get_ssid(const char *ifname, __u8 *ssid)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.essid.pointer = ssid;
	iwr.u.essid.length = 32;

	if (iw_ioctl(ifname, SIOCGIWESSID, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWESSID] ssid = NULL, not connected"); //do not use perror
		ret = -1;
	} else {
		ret = iwr.u.essid.length;
		if (ret > 32)
			ret = 32;
		/* Some drivers include nul termination in the SSID, so let's
		 * remove it here before further processing. WE-21 changes this
		 * to explicitly require the length _not_ to include nul
		 * termination. */
		if (ret > 0 && ssid[ret - 1] == '\0')
			ret--;
		ssid[ret] = '\0';
	}

	return ret;
}

int wext_set_ssid(const char *ifname, const __u8 *ssid, __u16 ssid_len)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.essid.pointer = (void *) ssid;
	iwr.u.essid.length = ssid_len;
	iwr.u.essid.flags = (ssid_len != 0);

	if (iw_ioctl(ifname, SIOCSIWESSID, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWESSID] error");
		ret = -1;
	}

	return ret;
}

int wext_set_bssid(const char *ifname, const __u8 *bssid)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.ap_addr.sa_family = ARPHRD_ETHER;
	memcpy(iwr.u.ap_addr.sa_data, bssid, ETH_ALEN);

	if(bssid[ETH_ALEN]=='#' && bssid[ETH_ALEN + 1]=='@'){
		memcpy(iwr.u.ap_addr.sa_data + ETH_ALEN, bssid + ETH_ALEN, 6);
	}

	if (iw_ioctl(ifname, SIOCSIWAP, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWAP] error");
		ret = -1;
	}

	return ret;
}

int is_broadcast_ether_addr(const unsigned char *addr)
{
	return (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]) == 0xff;
}

int wext_set_auth_param(const char *ifname, __u16 idx, __u32 value)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.param.flags = idx & IW_AUTH_INDEX;
	iwr.u.param.value = value;

	if (iw_ioctl(ifname, SIOCSIWAUTH, &iwr) < 0) {
		printf("\n\rWEXT: SIOCSIWAUTH(param %d value 0x%x) failed)", idx, value);
	}

	return ret;
}

int wext_set_key_ext(const char *ifname, __u16 alg, const __u8 *addr, int key_idx, int set_tx, const __u8 *seq, __u16 seq_len, __u8 *key, __u16 key_len)
{
	struct iwreq iwr;
	int ret = 0;
	struct iw_encode_ext *ext;

	ext = (struct iw_encode_ext *) pvPortMalloc(sizeof(struct iw_encode_ext) + key_len);
	if (ext == NULL)
		return -1;
	else
		memset(ext, 0, sizeof(struct iw_encode_ext) + key_len);

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.encoding.flags = key_idx + 1;
	iwr.u.encoding.flags |= IW_ENCODE_TEMP;
	iwr.u.encoding.pointer = ext;
	iwr.u.encoding.length = sizeof(struct iw_encode_ext) + key_len;

	if (alg == IW_ENCODE_DISABLED)
		iwr.u.encoding.flags |= IW_ENCODE_DISABLED;

	if (addr == NULL || is_broadcast_ether_addr(addr))
		ext->ext_flags |= IW_ENCODE_EXT_GROUP_KEY;

	if (set_tx)
		ext->ext_flags |= IW_ENCODE_EXT_SET_TX_KEY;

	ext->addr.sa_family = ARPHRD_ETHER;

	if (addr)
		memcpy(ext->addr.sa_data, addr, ETH_ALEN);
	else
		memset(ext->addr.sa_data, 0xff, ETH_ALEN);

	if (key && key_len) {
		memcpy(ext->key, key, key_len);
		ext->key_len = key_len;
	}

	ext->alg = alg;

	if (seq && seq_len) {
		ext->ext_flags |= IW_ENCODE_EXT_RX_SEQ_VALID;
		memcpy(ext->rx_seq, seq, seq_len);
	}

	if (iw_ioctl(ifname, SIOCSIWENCODEEXT, &iwr) < 0) {
		ret = -2;
		printf("\n\rioctl[SIOCSIWENCODEEXT] set key fail");
	}

	if(ext != NULL)
		vPortFree(ext);

	return ret;
}

int wext_get_enc_ext(const char *ifname, __u16 *alg, __u8 *key_idx, __u8 *passphrase)
{
	struct iwreq iwr;
	int ret = 0;
	struct iw_encode_ext *ext;

	ext = (struct iw_encode_ext *) pvPortMalloc(sizeof(struct iw_encode_ext) + 16);
	if (ext == NULL)
		return -1;
	else
		memset(ext, 0, sizeof(struct iw_encode_ext) + 16);

	iwr.u.encoding.pointer = ext;

	if (iw_ioctl(ifname, SIOCGIWENCODEEXT, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWENCODEEXT] error");
		ret = -1;
	}
	else
	{
		*alg = ext->alg;
		if(key_idx)
			*key_idx = (__u8)iwr.u.encoding.flags;
		if(passphrase)
			memcpy(passphrase, ext->key, ext->key_len);
	}

	if(ext != NULL)
		vPortFree(ext);

	return ret;
}

int wext_set_passphrase(const char *ifname, const __u8 *passphrase, __u16 passphrase_len)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.passphrase.pointer = (void *) passphrase;
	iwr.u.passphrase.length = passphrase_len;
	iwr.u.passphrase.flags = (passphrase_len != 0);

	if (iw_ioctl(ifname, SIOCSIWPRIVPASSPHRASE, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWESSID+0x1f] error");
		ret = -1;
	}

	return ret;
}

int wext_get_passphrase(const char *ifname, __u8 *passphrase)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.passphrase.pointer = (void *) passphrase;

	if (iw_ioctl(ifname, SIOCGIWPRIVPASSPHRASE, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWPRIVPASSPHRASE] error");
		ret = -1;
	}
	else {
		ret = iwr.u.passphrase.length;
		passphrase[ret] = '\0';
	}

	return ret;
}

int wext_disconnect(const char *ifname)
{
	int ret = 0;
	const __u8 null_bssid[ETH_ALEN] = {0, 0, 0, 0, 0, 1};	//set the last to 1 since driver will filter the mac with all 0x00 or 0xff

	if (wext_set_bssid(ifname, null_bssid) < 0){
		printf("\n\rWEXT: Failed to set bogus BSSID to disconnect");
		ret = -1;
	}
	return ret;
}


int wext_set_mac_address(const char *ifname, char * mac)
{
	int ret = 0;

	char *buf = (char *)rtw_zmalloc(13+17+1);
	if(buf == NULL) return -1;

	snprintf(buf, 13+17, "write_mac %s", mac);
	ret = wext_private_command(ifname, buf, 0);

	rtw_free(buf);
	return ret;
}

int wext_get_mac_address(const char *ifname, char * mac)
{
	int ret = 0;
	char *buf = (char *)rtw_zmalloc(32);
	if(buf == NULL) return -1;

	rtw_memcpy(buf, "read_mac", 8);
	ret = wext_private_command(ifname, buf, 1);

	strcpy(mac, buf);

	rtw_free(buf);
	return ret;
}

int wext_enable_powersave(const char *ifname)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;

	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("pm_set");
	para = pvPortMalloc(24);

	snprintf((char*)para, cmd_len, "pm_set");
	*(para+cmd_len) = 1; //ips=1;
	*(para+cmd_len+1) = 1;//lps=1

	iwr.u.essid.pointer = para;
	iwr.u.essid.length = cmd_len + 2;

	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWPRIVAPESSID] error");
		ret = -1;
	}

	vPortFree(para);
	return ret;
}

int wext_disable_powersave(const char *ifname)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;

	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("pm_set");
	para = pvPortMalloc(24);

	snprintf((char*)para, cmd_len, "pm_set");
	*(para+cmd_len) = 0; //ips=0;
	*(para+cmd_len+1) = 0;//lps=0

	iwr.u.essid.pointer = para;
	iwr.u.essid.length = cmd_len + 2;

	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWPRIVAPESSID] error");
		ret = -1;
	}

	vPortFree(para);
	return ret;

}

int wext_get_txpower(const char *ifname, int *cckpoweridx, int *ofdmpoweridx)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	if (iw_ioctl(ifname, SIOCGIWTXPOW, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWTXPOW] error");
		ret = -1;
	}
	else{
		*cckpoweridx = iwr.u.txpower.value & 0xFF;
		*ofdmpoweridx = (iwr.u.txpower.value>>8) & 0xFF;
	}

	return ret;
}

int wext_set_txpower(const char *ifname, int bcck, int poweridx)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.txpower.value = poweridx;
	iwr.u.txpower.flags = bcck;
	if (iw_ioctl(ifname, SIOCSIWTXPOW, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWTXPOW] error");
		ret = -1;
	}

	return ret;
}

int wext_get_associated_client_list(const char *ifname, void * client_list_buffer, uint16_t buffer_length)
{
	int ret = 0;
	void * buf;

	buf = (void *)rtw_zmalloc(25);
	if(buf == NULL) return -1;

	snprintf(buf, 25, "get_client_list %x", client_list_buffer);
	ret = wext_private_command(ifname, buf, 0);

	rtw_free(buf);

	return ret;
}

int wext_get_ap_info(const char *ifname, rtw_bss_info_t * ap_info, rtw_security_t* security)
{
	int ret = 0;
	void * buf;

	buf = (void *)rtw_zmalloc(24);
	if(buf == NULL) return -1;

	snprintf(buf, 24, "get_ap_info %x", ap_info);
	ret = wext_private_command(ifname, buf, 0);

	snprintf(buf, 24, "get_security");
	ret = wext_private_command(ifname, buf, 0);

	sscanf(buf, "%d", security);

	rtw_free(buf);

	return ret;
}

int wext_set_mode(const char *ifname, int mode)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.mode = mode;
	if (iw_ioctl(ifname, SIOCSIWMODE, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWMODE] error");
		ret = -1;
	}

	return ret;
}

int wext_get_mode(const char *ifname, int *mode)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));

	if (iw_ioctl(ifname, SIOCGIWMODE, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWMODE] error");
		ret = -1;
	}
	else
		*mode = iwr.u.mode;

	return ret;
}

int wext_set_ap_ssid(const char *ifname, const __u8 *ssid, __u16 ssid_len)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.essid.pointer = (void *) ssid;
	iwr.u.essid.length = ssid_len;
	iwr.u.essid.flags = (ssid_len != 0);

	if (iw_ioctl(ifname, SIOCSIWPRIVAPESSID, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWPRIVAPESSID] error");
		ret = -1;
	}

	return ret;
}

int wext_set_country(const char *ifname, char *country_code)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));

	iwr.u.data.pointer = country_code;
	if (iw_ioctl(ifname, SIOCSIWPRIVCOUNTRY, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWPRIVCOUNTRY] error");
		ret = -1;
	}
	return ret;
}

int wext_get_rssi(const char *ifname, int *rssi)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));

	if (iw_ioctl(ifname, SIOCGIWSENS, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWSENS] error");
		ret = -1;
	} else {
		*rssi = 0 - iwr.u.sens.value;
	}
	return ret;
}

int wext_set_pscan_channel(const char *ifname, __u8 *ch, __u8 length)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int i =0;

	memset(&iwr, 0, sizeof(iwr));
	//Format of para:function_name num_channel chan1 ...
	para = pvPortMalloc((length + 1) + 12);//size:length+num_chan+function_name
	//Cmd
	snprintf((char*)para, 12, "PartialScan");
	//length
	*(para+12) = length;
	for(i = 0; i < length; i++){
		*(para + 13 + i)= *(ch + i);
	}

	iwr.u.data.pointer = para;
	iwr.u.data.length = (length + 1) + 12;
	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_set_pscan_channel():ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}
	vPortFree(para);
	return ret;
}
int wext_set_channel(const char *ifname, __u8 ch)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.freq.m = 0;
	iwr.u.freq.e = 0;
	iwr.u.freq.i = ch;

	if (iw_ioctl(ifname, SIOCSIWFREQ, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWFREQ] error");
		ret = -1;
	}

	return ret;
}

int wext_get_channel(const char *ifname, __u8 *ch)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));

	if (iw_ioctl(ifname, SIOCGIWFREQ, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWFREQ] error");
		ret = -1;
	}
	else
		*ch = iwr.u.freq.i;

	return ret;
}

int wext_register_multicast_address(const char *ifname, rtw_mac_t *mac)
{
	int ret = 0;
	char * buf = (char *)rtw_zmalloc(32);
	if(buf == NULL) return -1;

	snprintf(buf, 32, "reg_multicast "MAC_FMT, MAC_ARG(mac->octet));
	ret = wext_private_command(ifname, buf, 0);

	rtw_free(buf);
	return ret;
}

int wext_unregister_multicast_address(const char *ifname, rtw_mac_t *mac)
{
	int ret = 0;
	char * buf = (char *)rtw_zmalloc(35);
	if(buf == NULL) return -1;

	snprintf(buf, 35, "reg_multicast -d "MAC_FMT, MAC_ARG(mac->octet));
	ret = wext_private_command(ifname, buf, 0);

	rtw_free(buf);
	return ret;
}

int wext_set_scan(const char *ifname, char *buf, __u16 buf_len, __u16 flags)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
#if 0 //for scan_with_ssid
	if(buf)
		memset(buf, 0, buf_len);
#endif
	iwr.u.data.pointer = buf;
	iwr.u.data.flags = flags;
	iwr.u.data.length = buf_len;
	if (iw_ioctl(ifname, SIOCSIWSCAN, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWSCAN] error");
		ret = -1;
	}
	return ret;
}

int wext_get_scan(const char *ifname, char *buf, __u16 buf_len)
{
	struct iwreq iwr;
	int ret = 0;

	iwr.u.data.pointer = buf;
	iwr.u.data.length = buf_len;
	if (iw_ioctl(ifname, SIOCGIWSCAN, &iwr) < 0) {
		printf("\n\rioctl[SIOCGIWSCAN] error");
		ret = -1;
	}else
		ret = iwr.u.data.flags;
	return ret;
}

int wext_private_command(const char *ifname, char *cmd, int show_msg)
{
	struct iwreq iwr;
	int ret = 0, buf_size;
	char *buf;
	u8 cmdname[17] = {0}; // IFNAMSIZ+1

	sscanf(cmd, "%16s", cmdname);
	if(strcmp((const char *)cmdname, "efuse_get") == 0)
		buf_size = 2600;//2600 for efuse_get rmap,0,512
	else
		buf_size = 1024;
	if(strlen(cmd) >= buf_size)
		buf_size = strlen(cmd) + 1;	// 1 : '\0'
	buf = (char*)pvPortMalloc(buf_size);
	if(!buf){
		printf("\n\rWEXT: Can't mem_malloc memory");
		return -1;
	}
	memset(buf, 0, buf_size);
	strcpy(buf, cmd);
	memset(&iwr, 0, sizeof(iwr));
	iwr.u.data.pointer = buf;
	iwr.u.data.length = buf_size;
	iwr.u.data.flags = 0;

	if ((ret = iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr)) < 0) {
		printf("\n\rioctl[SIOCDEVPRIVATE] error. ret=%d\n", ret);
	}
	if (show_msg && iwr.u.data.length) {
		printf("\n\rPrivate Message: %s", (char *) iwr.u.data.pointer);
	}
	vPortFree(buf);
	return ret;
}

void wext_wlan_indicate(unsigned int cmd, union iwreq_data *wrqu, char *extra)
{
	unsigned char null_mac[6] = {0};

	switch(cmd)
	{
		case SIOCGIWAP:
			if(wrqu->ap_addr.sa_family == ARPHRD_ETHER)
			{
				if(!memcmp(wrqu->ap_addr.sa_data, null_mac, sizeof(null_mac)))
					wifi_indication(WIFI_EVENT_DISCONNECT, NULL, 0, 0);
				else
					wifi_indication(WIFI_EVENT_CONNECT, wrqu->ap_addr.sa_data, sizeof(null_mac), 0);
			}
			break;

		case IWEVCUSTOM:
			if(extra)
			{
				if(!memcmp(IW_EXT_STR_FOURWAY_DONE, extra, strlen(IW_EXT_STR_FOURWAY_DONE)))
					wifi_indication(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, extra, strlen(IW_EXT_STR_FOURWAY_DONE), 0);
				else if(!memcmp(IW_EXT_STR_RECONNECTION_FAIL, extra, strlen(IW_EXT_STR_RECONNECTION_FAIL)))
					wifi_indication(WIFI_EVENT_RECONNECTION_FAIL, extra, strlen(IW_EXT_STR_RECONNECTION_FAIL), 0);
				else if(!memcmp(IW_EVT_STR_STA_ASSOC, extra, strlen(IW_EVT_STR_STA_ASSOC)))
					wifi_indication(WIFI_EVENT_STA_ASSOC, wrqu->data.pointer, wrqu->data.length, 0);
				else if(!memcmp(IW_EVT_STR_STA_DISASSOC, extra, strlen(IW_EVT_STR_STA_DISASSOC)))
					wifi_indication(WIFI_EVENT_STA_DISASSOC, wrqu->addr.sa_data, sizeof(null_mac), 0);
#ifdef CONFIG_P2P_NEW
				else if(!memcmp(IW_EVT_STR_SEND_ACTION_DONE, extra, strlen(IW_EVT_STR_SEND_ACTION_DONE)))
					wifi_indication(WIFI_EVENT_SEND_ACTION_DONE, NULL, 0, wrqu->data.flags);
#endif
			}
			break;
		case SIOCGIWSCAN:
			if(wrqu->data.pointer == NULL)
				wifi_indication(WIFI_EVENT_SCAN_DONE, wrqu->data.pointer, 0, 0);
			else
				wifi_indication(WIFI_EVENT_SCAN_RESULT_REPORT, wrqu->data.pointer, 0, 0);
			break;
#ifdef CONFIG_P2P_NEW
		case IWEVMGNTRECV:
			wifi_indication(WIFI_EVENT_RX_MGNT, wrqu->data.pointer, wrqu->data.length, wrqu->data.flags);
			break;
#endif
		default:
			break;

	}
}

#ifdef CONFIG_P2P_NEW
int wext_send_mgnt(const char *ifname, char *buf, __u16 buf_len, __u16 flags)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.data.pointer = buf;
	iwr.u.data.length = buf_len;
	iwr.u.data.flags = flags;
	if (iw_ioctl(ifname, SIOCSIWMGNTSEND, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWMGNTSEND] error");
		ret = -1;
	}
	return ret;
}
#endif

int wext_set_gen_ie(const char *ifname, char *buf, __u16 buf_len, __u16 flags)
{
	struct iwreq iwr;
	int ret = 0;

	memset(&iwr, 0, sizeof(iwr));
	iwr.u.data.pointer = buf;
	iwr.u.data.length = buf_len;
	iwr.u.data.flags = flags;
	if (iw_ioctl(ifname, SIOCSIWGENIE, &iwr) < 0) {
		printf("\n\rioctl[SIOCSIWGENIE] error");
		ret = -1;
	}
	return ret;
}

int wext_set_autoreconnect(const char *ifname, __u8 mode)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;

	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("SetAutoRecnt");
	para = pvPortMalloc((1) + cmd_len);//size:para_len+cmd_len
	//Cmd
	snprintf((char*)para, cmd_len, "SetAutoRecnt");
	//length
	*(para+cmd_len) = mode;	//para

	iwr.u.data.pointer = para;
	iwr.u.data.length = (1) + cmd_len;
	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_set_autoreconnect():ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}
	vPortFree(para);
	return ret;
}

int wext_get_autoreconnect(const char *ifname, __u8 *mode)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;

	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("GetAutoRecnt");
	//para = pvPortMalloc((1) + cmd_len);//size:para_len+cmd_len
	para = pvPortMalloc(cmd_len);//size:para_len+cmd_len
	//Cmd
	snprintf((char*)para, cmd_len, "GetAutoRecnt");
	//length
	//*(para+cmd_len) = *mode;	//para

	iwr.u.data.pointer = para;
	//iwr.u.data.length = (1) + cmd_len;
	iwr.u.data.length = cmd_len;
	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_set_autoreconnect():ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}
	*mode = *(__u8 *)(iwr.u.data.pointer);
	vPortFree(para);
	return ret;
}

#ifdef CONFIG_CUSTOM_IE
int wext_add_custom_ie(const char *ifname, void *cus_ie, int ie_num)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;
	if(ie_num <= 0 || !cus_ie){
		printf("\n\rwext_add_custom_ie():wrong parameter");
		ret = -1;
		return ret;
	}
	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("SetCusIE");
	para = pvPortMalloc((4)* 2 + cmd_len);//size:addr len+cmd_len
	//Cmd
	snprintf(para, cmd_len, "SetCusIE");
	//addr length
	*(__u32 *)(para + cmd_len) = (__u32)cus_ie; //ie addr
	//ie_num
	*(__u32 *)(para + cmd_len + 4) = ie_num; //num of ie

	iwr.u.data.pointer = para;
	iwr.u.data.length = (4)* 2 + cmd_len;// 2 input
	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_add_custom_ie():ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}
	vPortFree(para);

	return ret;
}

int wext_update_custom_ie(const char *ifname, void * cus_ie, int ie_index)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;
	if(ie_index <= 0 || !cus_ie){
		printf("\n\rwext_update_custom_ie():wrong parameter");
		ret = -1;
		return ret;
	}
	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("UpdateIE");
	para = pvPortMalloc((4)* 2 + cmd_len);//size:addr len+cmd_len
	//Cmd
	snprintf(para, cmd_len, "UpdateIE");
	//addr length
	*(__u32 *)(para + cmd_len) = (__u32)cus_ie; //ie addr
	//ie_index
	*(__u32 *)(para + cmd_len + 4) = ie_index; //num of ie

	iwr.u.data.pointer = para;
	iwr.u.data.length = (4)* 2 + cmd_len;// 2 input
	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_update_custom_ie():ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}
	vPortFree(para);

	return ret;

}

int wext_del_custom_ie(const char *ifname)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;

	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("DelIE");
	para = pvPortMalloc(cmd_len);//size:addr len+cmd_len
	//Cmd
	snprintf(para, cmd_len, "DelIE");

	iwr.u.data.pointer = para;
	iwr.u.data.length = cmd_len;
	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_del_custom_ie():ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}
	vPortFree(para);

	return ret;
}

#endif

#ifdef CONFIG_AP_MODE
int wext_enable_forwarding(const char *ifname)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;

	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("forwarding_set");
	para = pvPortMalloc(cmd_len + 1);
	if(para == NULL) return -1;

	// forwarding_set 1
	snprintf((char *) para, cmd_len, "forwarding_set");
	*(para + cmd_len) = '1';

	iwr.u.essid.pointer = para;
	iwr.u.essid.length = cmd_len + 1;

	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_enable_forwarding(): ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}

	vPortFree(para);
	return ret;
}

int wext_disable_forwarding(const char *ifname)
{
	struct iwreq iwr;
	int ret = 0;
	__u8 *para = NULL;
	int cmd_len = 0;

	memset(&iwr, 0, sizeof(iwr));
	cmd_len = sizeof("forwarding_set");
	para = pvPortMalloc(cmd_len + 1);
	if(para == NULL) return -1;

	// forwarding_set 0
	snprintf((char *) para, cmd_len, "forwarding_set");
	*(para + cmd_len) = '0';

	iwr.u.essid.pointer = para;
	iwr.u.essid.length = cmd_len + 1;

	if (iw_ioctl(ifname, SIOCDEVPRIVATE, &iwr) < 0) {
		printf("\n\rwext_disable_forwarding(): ioctl[SIOCDEVPRIVATE] error");
		ret = -1;
	}

	vPortFree(para);
	return ret;

}
#endif

int wext_del_station(const char *ifname, unsigned char* hwaddr)
{
	return rltk_del_station(ifname, hwaddr);
}

int wext_get_auto_chl(const char *ifname, int low_chl, int hi_chl)
{
	int ret = -1;
	int channel = 0;
	wext_disable_powersave(ifname);
	if((channel = rltk_get_auto_chl(ifname, low_chl, hi_chl)) != 0 )
		ret = channel ;
	wext_enable_powersave(ifname);
	return ret;
}

