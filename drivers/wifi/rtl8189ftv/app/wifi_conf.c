//----------------------------------------------------------------------------//
#include <zephyr.h>
#include <mem_manager.h>
#include "../osdep/net_intf.h"
#include "wifi_conf.h"


/******************************************************
 *                    Constants
 ******************************************************/
#define SCAN_USE_SEMAPHORE	0

#define RTW_JOIN_TIMEOUT 15000

#define JOIN_ASSOCIATED             (uint32_t)(1 << 0)
#define JOIN_AUTHENTICATED          (uint32_t)(1 << 1)
#define JOIN_LINK_READY             (uint32_t)(1 << 2)
#define JOIN_SECURITY_COMPLETE      (uint32_t)(1 << 3)
#define JOIN_COMPLETE               (uint32_t)(1 << 4)
#define JOIN_NO_NETWORKS            (uint32_t)(1 << 5)

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *               Variables Declarations
 ******************************************************/

#if CONFIG_LWIP_LAYER
extern struct netif xnetif[NET_IF_NUM];
#endif

/******************************************************
 *               Variables Definitions
 ******************************************************/
static internal_scan_handler_t scan_result_handler_ptr = {0, 0, 0, RTW_FALSE, 0, 0, 0, 0, 0};
static internal_join_result_t* join_user_data;
static rtw_mode_t wifi_mode;
uint32_t rtw_join_status;

/******************************************************
 *               Function Definitions
 ******************************************************/

#if CONFIG_WLAN
//----------------------------------------------------------------------------//
extern int promisc_set(rtw_rcr_level_t enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used);
extern unsigned char promisc_enabled(void);
static int wifi_connect_local(rtw_network_info_t *pWifi)
{
	int ret = 0;

	if(promisc_enabled())
		promisc_set(0, NULL, 0);

	if(!pWifi) return -1;
	switch(pWifi->security_type){
		case RTW_SECURITY_OPEN:
			ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_NONE, NULL, 0, 0, 0, 0, NULL, 0);
			break;
		case RTW_SECURITY_WEP_PSK:
		case RTW_SECURITY_WEP_SHARED:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_SHARED_KEY);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_WEP, NULL, pWifi->key_id, 1 /* set tx key */, 0, 0, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
		case RTW_SECURITY_WPA2_TKIP_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_TKIP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_AES_PSK:
		case RTW_SECURITY_WPA2_AES_PSK:
		case RTW_SECURITY_WPA2_MIXED_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		default:
			ret = -1;
			printf("\n\rWIFICONF: security type is not supported");
			break;
	}
	if(ret == 0)
		ret = wext_set_ssid(WLAN0_NAME, pWifi->ssid.val, pWifi->ssid.len);
	return ret;
}

static int wifi_connect_bssid_local(rtw_network_info_t *pWifi)
{
	int ret = 0;
	u8 bssid[12] = {0};

	if(promisc_enabled())
		promisc_set(0, NULL, 0);

	if(!pWifi) return -1;
	switch(pWifi->security_type){
		case RTW_SECURITY_OPEN:
			ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_NONE, NULL, 0, 0, 0, 0, NULL, 0);
			break;
		case RTW_SECURITY_WEP_PSK:
		case RTW_SECURITY_WEP_SHARED:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_SHARED_KEY);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_WEP, NULL, pWifi->key_id, 1 /* set tx key */, 0, 0, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
		case RTW_SECURITY_WPA2_TKIP_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_TKIP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_AES_PSK:
		case RTW_SECURITY_WPA2_AES_PSK:
		case RTW_SECURITY_WPA2_MIXED_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		default:
			ret = -1;
			printf("\n\rWIFICONF: security type is not supported");
			break;
	}
	if(ret == 0){
		memcpy(bssid, pWifi->bssid.octet, ETH_ALEN);
		if(pWifi->ssid.len){
			bssid[ETH_ALEN] = '#';
			bssid[ETH_ALEN + 1] = '@';
			memcpy(bssid + ETH_ALEN + 2, &pWifi, sizeof(pWifi));
		}
		ret = wext_set_bssid(WLAN0_NAME, bssid);
	}
	return ret;
}

static void wifi_connected_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	if((join_user_data!=NULL)&&((join_user_data->network_info.security_type == RTW_SECURITY_OPEN) ||
		(join_user_data->network_info.security_type == RTW_SECURITY_WEP_PSK))){
		rtw_join_status = JOIN_COMPLETE | JOIN_SECURITY_COMPLETE | JOIN_ASSOCIATED | JOIN_AUTHENTICATED | JOIN_LINK_READY;
		//printf("\n\r%s(): join status = %x\n", __func__, rtw_join_status);
		rtw_up_sema(&join_user_data->join_sema);
	}
}
static void wifi_handshake_done_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	rtw_join_status = JOIN_COMPLETE | JOIN_SECURITY_COMPLETE | JOIN_ASSOCIATED | JOIN_AUTHENTICATED | JOIN_LINK_READY;
	//printf("\n\r%s(): join status = %x\n", __func__, rtw_join_status);
	if(join_user_data != NULL)
		rtw_up_sema(&join_user_data->join_sema);
}

static void wifi_disconn_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	rtw_join_status = JOIN_NO_NETWORKS;

	if(join_user_data != NULL)
		rtw_up_sema(&join_user_data->join_sema);
}

//----------------------------------------------------------------------------//
#if 0  //#ifdef CONNECT_SCAN_WHILE
static WIFI_LINK_STATUS wifi_get_connect_status(rtw_network_info_t *pWifi)
{
	char essid[33];
	WIFI_LINK_STATUS status = WIFI_LINK_DISCONNECTED;
	if((pWifi->security_type == RTW_SECURITY_WPA2_AES_PSK) || (pWifi->security_type == RTW_SECURITY_WPA_AES_PSK)){
		if(wext_handshake_done() == 0) {
			printf("\n\rHandshake not done yet\n");
			return status;
		}
	}
	if(wext_get_ssid(WLAN0_NAME, (unsigned char*)essid) > 0){
		if(memcmp(essid, pWifi->ssid.val, pWifi->ssid.len) == 0){
			status = WIFI_LINK_CONNECTED;
		}
	}
	return status;
}

int rtk_wifi_connect(
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void				*semaphore)
{

	rtw_network_info_t wifi;
	WIFI_LINK_STATUS status;
	int timeout = 20;

	strcpy(wifi.ssid.val, ssid);
	wifi.security_type = security_type;
	wifi.password = password;
	wifi.ssid.len = ssid_len;
	wifi.password_len = password_len;
	wifi.key_id = key_id;

	printf("\n\rJoining BSS ...");

	if(wifi_connect_local(&wifi) < 0) {
		printf("\n\rERROR: Operation failed!");
		return RTW_ERROR;
	}

	while(1) {
		status = wifi_get_connect_status(&wifi);

		if(status == WIFI_LINK_CONNECTED) {
			printf("\n\r%s connected", wifi.ssid.val);
			break;
		}

		if(timeout == 0) {
			printf("\n\rERROR: Join BSS timeout!");
			break;
		}

		vTaskDelay(1 * configTICK_RATE_HZ);
		timeout --;
	}

	if(status == WIFI_LINK_CONNECTED)
		return RTW_SUCCESS;

	return RTW_ERROR;
}
#endif
int rtk_wifi_connect(
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore)
{
	xSemaphoreHandle join_semaphore;
	rtw_result_t result = RTW_SUCCESS;
	rtw_join_status = 0;//clear for last connect status

    if ( ( ( ( password_len >  RTW_MAX_PSK_LEN ) ||
             ( password_len <  RTW_MIN_PSK_LEN ) ) &&
           ( ( security_type == RTW_SECURITY_WPA_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_MIXED_PSK ) ) ) ) {
		return RTW_INVALID_KEY;
	}

	internal_join_result_t *join_result = (internal_join_result_t *)rtw_zmalloc(sizeof(internal_join_result_t));

	if(!join_result) {
		return RTW_NOMEM;
	}

	join_result->network_info.ssid.len = ssid_len > 32 ? 32 : ssid_len;
	rtw_memcpy(join_result->network_info.ssid.val, ssid, ssid_len);

	join_result->network_info.password_len = password_len;
	if(password_len) {
		join_result->network_info.password = rtw_zmalloc(password_len + 1);
		if(!join_result->network_info.password) {
			return RTW_NOMEM;
		}
		rtw_memcpy(join_result->network_info.password, password, password_len);
	}

	join_result->network_info.security_type = security_type;
	join_result->network_info.key_id = key_id;

	if(semaphore == NULL) {
		rtw_init_sema( &join_result->join_sema, 0 );
		if(!join_result->join_sema) return RTW_NORESOURCE;
		join_semaphore = join_result->join_sema;
	} else {
		join_result->join_sema = semaphore;
	}

	wifi_reg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, wifi_disconn_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl, NULL);

	wifi_connect_local(&join_result->network_info);

	join_user_data = join_result;

	if(semaphore == NULL) {
		if(rtw_down_timeout_sema( &join_result->join_sema, RTW_JOIN_TIMEOUT ) == RTW_FALSE) {
			printf("RTW API: Join bss timeout\r\n");
			if(password_len) {
				rtw_free(join_result->network_info.password);
			}
			rtw_free((u8*)join_result);
			rtw_free_sema((void **)&join_semaphore);
			result = RTW_TIMEOUT;
			goto error;
		} else {
			rtw_free_sema((void **)&join_semaphore );
			if(join_result->network_info.password_len) {
				rtw_free(join_result->network_info.password);
			}
			rtw_free((u8*)join_result);
			if(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) {
				result = RTW_ERROR;
				goto error;
			}
		}
	}
	result = RTW_SUCCESS;

error:
	join_user_data = NULL;
	wifi_unreg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl);
	return result;
}

int wifi_connect_bssid(
	unsigned char 		bssid[ETH_ALEN],
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				bssid_len,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore)
{
	xSemaphoreHandle join_semaphore;
	rtw_result_t result = RTW_SUCCESS;
	rtw_join_status = 0;//clear for last connect status

    if ( ( ( ( password_len >  RTW_MAX_PSK_LEN ) ||
             ( password_len <  RTW_MIN_PSK_LEN ) ) &&
           ( ( security_type == RTW_SECURITY_WPA_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_MIXED_PSK ) ) ) ) {
		return RTW_INVALID_KEY;
	}

	internal_join_result_t *join_result = (internal_join_result_t *)rtw_zmalloc(sizeof(internal_join_result_t));

	if(!join_result) {
		return RTW_NOMEM;
	}
	if(ssid_len && ssid){
		join_result->network_info.ssid.len = ssid_len > 32 ? 32 : ssid_len;
		rtw_memcpy(join_result->network_info.ssid.val, ssid, ssid_len);
	}
	rtw_memcpy(join_result->network_info.bssid.octet, bssid, bssid_len);

	join_result->network_info.password_len = password_len;
	if(password_len) {
		join_result->network_info.password = rtw_zmalloc(password_len);
		if(!join_result->network_info.password) {
			return RTW_NOMEM;
		}
		rtw_memcpy(join_result->network_info.password, password, password_len);
	}

	join_result->network_info.security_type = security_type;
	join_result->network_info.key_id = key_id;

	if(semaphore == NULL) {
		rtw_init_sema( &join_result->join_sema, 0 );
		if(!join_result->join_sema) return RTW_NORESOURCE;
		join_semaphore = join_result->join_sema;
	} else {
		join_result->join_sema = semaphore;
	}

	wifi_reg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, wifi_disconn_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl, NULL);

	wifi_connect_bssid_local(&join_result->network_info);

	join_user_data = join_result;

	if(semaphore == NULL) {
		if(rtw_down_timeout_sema( &join_result->join_sema, RTW_JOIN_TIMEOUT ) == RTW_FALSE) {
			printf("RTW API: Join bss timeout\r\n");
			if(password_len) {
				rtw_free(join_result->network_info.password);
			}
			rtw_free((u8*)join_result);
			rtw_free_sema((void **)&join_semaphore);
			result = RTW_TIMEOUT;
			goto error;
		} else {
			rtw_free_sema((void **)&join_semaphore);
			if(join_result->network_info.password_len) {
				rtw_free(join_result->network_info.password);
			}
			rtw_free((u8*)join_result);
			if(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) {
				result = RTW_ERROR;
				goto error;
			}
		}
	}
	result = RTW_SUCCESS;

error:
	join_user_data = NULL;
	wifi_unreg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl);
	return result;
}

int rtk_wifi_disconnect(void)
{
	return wext_disconnect(WLAN0_NAME);
}

//----------------------------------------------------------------------------//
int wifi_is_up(rtw_interface_t interface)
{
	if(interface == RTW_AP_INTERFACE) {
		if(wifi_mode == RTW_MODE_STA_AP) {
			return rltk_wlan_running(WLAN1_IDX);
		}
	}

	return rltk_wlan_running(WLAN0_IDX);
}

int wifi_is_ready_to_transceive(rtw_interface_t interface)
{
	switch ( interface )
	{
		case RTW_AP_INTERFACE:
			return ( wifi_is_up(interface) == RTW_TRUE ) ? RTW_SUCCESS : RTW_ERROR;

		case RTW_STA_INTERFACE:
			if( wifi_is_up(interface) == RTW_TRUE ){
				switch ( rtw_join_status )
				{
					case JOIN_NO_NETWORKS:
						return RTW_NOTFOUND;

					case JOIN_AUTHENTICATED | JOIN_ASSOCIATED | JOIN_LINK_READY | JOIN_SECURITY_COMPLETE | JOIN_COMPLETE:
						return RTW_SUCCESS;

					case 0:
					case JOIN_SECURITY_COMPLETE: /* For open/WEP */
						return RTW_NOT_AUTHENTICATED;

					case JOIN_AUTHENTICATED | JOIN_LINK_READY | JOIN_SECURITY_COMPLETE:
						return RTW_UNFINISHED;

					case JOIN_AUTHENTICATED | JOIN_LINK_READY:
					case JOIN_AUTHENTICATED | JOIN_LINK_READY | JOIN_COMPLETE:
						return RTW_NOT_KEYED;

					default:
						return RTW_ERROR;
				}
			} else {
				return RTW_ERROR;
			}
		default:
			return RTW_ERROR;
	}
}
//----------------------------------------------------------------------------//
#if 0
int wifi_set_mac_address(char * mac)
{
	u8 MAC[ETH_ALEN];
	sscanf(mac, MAC_FMT, MAC, MAC+1, MAC+2, MAC+3, MAC+4, MAC+5);
	return wext_set_mac_address(WLAN0_NAME, MAC);
}
#else
extern void rltk_set_mac(unsigned char * mac);
int wifi_set_mac_address(char * mac)
{
//	int ret = 0;
	int i;
	u32 MAC_S[ETH_ALEN];
	u8 MAC[ETH_ALEN];
	u8 *check_mac;
	check_mac = (u8*)mac;

	if(strlen(mac) != 17){
	printf("\r\n INVALID MAC ADDRESS!");
	return -1;
	}

	while(*check_mac !='\0'){
		if((*check_mac == 58)||
		(*check_mac > 64 &&*check_mac < 71)||
		(*check_mac > 96&&*check_mac < 103)||
		(*check_mac>47&&*check_mac<58)){
			check_mac++;
			continue;
		}
		printf("\r\n INVALID MAC ADDRESS!");
		return -1;
	}
	sscanf(mac, MAC_FMT, MAC_S, MAC_S+1, MAC_S+2, MAC_S+3, MAC_S+4, MAC_S+5);
	for(i = 0; i < ETH_ALEN; i ++)
		MAC[i] = (u8)MAC_S[i] & 0xFF;
	rltk_set_mac(MAC);
#if CONFIG_WRITE_MAC_TO_FLASH
	/*erase flash before write each time*/
	flash_EraseSector(FLASH_STORE_MAC_SEC);
	/*write mac to flash*/
	if(-1 == flash_Wrtie(FLASH_ADD_STORE_MAC, (char*)MAC, ETH_ALEN)){
		printf("write mac address to flash error!\r\n");
//		ret =  -2;
		}
#endif
	printf("\r\n write mac address success, please reset wifi !\r\n");
	return 0;

}
#endif
int wifi_get_mac_address(char * mac)
{
	return wext_get_mac_address(WLAN0_NAME, mac);
}

//----------------------------------------------------------------------------//
int wifi_enable_powersave(void)
{
	return wext_enable_powersave(WLAN0_NAME);
}

int wifi_disable_powersave(void)
{
	return wext_disable_powersave(WLAN0_NAME);
}

//----------------------------------------------------------------------------//
int wifi_get_txpower(int *cckpoweridx, int*ofdmpoweridx)
{
	return wext_get_txpower(WLAN0_NAME, cckpoweridx, ofdmpoweridx);
}

int wifi_set_txpower(int bcck, int poweridx)
{
	return wext_set_txpower(WLAN0_NAME, bcck, poweridx);
}

//----------------------------------------------------------------------------//
int wifi_get_associated_client_list(void * client_list_buffer, uint16_t buffer_length)
{
	const char * ifname = WLAN0_NAME;
	if(wifi_mode == RTW_MODE_STA_AP) {
		ifname = WLAN1_NAME;
	}
    return wext_get_associated_client_list(ifname, client_list_buffer, buffer_length );
}

//----------------------------------------------------------------------------//
int wifi_get_ap_info(rtw_bss_info_t * ap_info, rtw_security_t* security)
{
	const char * ifname = WLAN0_NAME;
	if(wifi_mode == RTW_MODE_STA_AP) {
		ifname = WLAN1_NAME;
	}
    return wext_get_ap_info(ifname, ap_info, security );
}

//----------------------------------------------------------------------------//
int wifi_set_country(rtw_country_code_t country_code)
{
	int ret;
	switch(country_code){
		case RTW_COUNTRY_US:	ret = wext_set_country(WLAN0_NAME, "US");	break;
		case RTW_COUNTRY_EU:	ret = wext_set_country(WLAN0_NAME, "EU");	break;
		case RTW_COUNTRY_JP:	ret = wext_set_country(WLAN0_NAME, "JP");	break;
		case RTW_COUNTRY_CN:	ret = wext_set_country(WLAN0_NAME, "CN");	break;
		default:	printf("\n\rUnknown country_code\n"); 	ret = -1;		break;
	}
	return ret;
}

//----------------------------------------------------------------------------//
int wifi_get_rssi(int *pRSSI)
{
	return wext_get_rssi(WLAN0_NAME, pRSSI);
}

//----------------------------------------------------------------------------//
int wifi_set_channel(int channel)
{
	return wext_set_channel(WLAN0_NAME, channel);
}


int wifi_get_channel(int *channel)
{
	return wext_get_channel(WLAN0_NAME, (u8*)channel);
}

//----------------------------------------------------------------------------//
int wifi_set_ext_channel(int num)
{
	return rltk_wlan_set_ext_channel(num);
}

//----------------------------------------------------------------------------//
int wifi_register_multicast_address(rtw_mac_t *mac)
{
	return wext_register_multicast_address(WLAN0_NAME, mac);
}

int wifi_unregister_multicast_address(rtw_mac_t *mac)
{
	return wext_unregister_multicast_address(WLAN0_NAME, mac);
}

//----------------------------------------------------------------------------//
int wifi_rf_on(void)
{
	int ret;
	ret = rltk_wlan_rf_on();
	return ret;
}

//----------------------------------------------------------------------------//
int wifi_rf_off(void)
{
	int ret;
	ret = rltk_wlan_rf_off();
	return ret;
}
int wifi_on(rtw_mode_t mode)
{
	int ret = 1;
	int timeout = 20;
	int idx;
	int devnum = 1;

	rtw_join_status = 0;
	wifi_mode = mode;

	init_event_callback_list();
	if(mode == RTW_MODE_STA_AP)
		devnum = 2;

	if(rltk_wlan_running(WLAN0_IDX)) {
		printf("\n\rWIFI is already running");
		return 1;
	}

	printf("\n\rInitializing WIFI ...");
	for(idx=0;idx<devnum;idx++){
		ret = rltk_wlan_init(idx, mode);
		if(ret <0)
			return ret;
	}
	for(idx=0;idx<devnum;idx++)
		rltk_wlan_start(idx);

	while(1) {
		if(rltk_wlan_running(devnum-1)) {
			printf("\n\rWIFI initialized");
			break;
		}

		if(timeout == 0) {
			printf("\n\rERROR: Init WIFI timeout!");
			break;
		}

		vTaskDelay(1 * configTICK_RATE_HZ);
		timeout--;
	}

	return ret;
}

int wifi_off(void)
{
	int ret = 0;
	int timeout = 20;

	if((rltk_wlan_running(WLAN0_IDX) == 0) &&
		(rltk_wlan_running(WLAN1_IDX) == 0)) {
		printf("\n\rWIFI is not running");
		return 0;
	}

#if CONFIG_LWIP_LAYER
	dhcps_deinit();
	LwIP_DHCP(0, DHCP_STOP);
#elif CONFIG_ZEPHYR_LAYER
	zephyr_dhcp_stop();
#endif

	printf("\n\rDeinitializing WIFI ...");
	rltk_wlan_deinit();

	while(1) {
		if((rltk_wlan_running(WLAN0_IDX) == 0) &&
			(rltk_wlan_running(WLAN1_IDX) == 0)) {
			printf("\n\rWIFI deinitialized");
			break;
		}

		if(timeout == 0) {
			printf("\n\rERROR: Deinit WIFI timeout!");
			break;
		}

		vTaskDelay(1 * configTICK_RATE_HZ);
		timeout --;
	}

	wifi_mode = RTW_MODE_NONE;

	return ret;
}

//----------------------------------------------------------------------------//
static void wifi_ap_sta_assoc_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	//USER TODO

}
static void wifi_ap_sta_disassoc_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	//USER TODO
}

#ifdef CONFIG_WPS //construct WPS IE
int wpas_wps_init(const char* ifname);
#endif

int wifi_autochl_select(int low_chl, int hi_chl)
{
	return wext_get_auto_chl(WLAN0_NAME, low_chl, hi_chl);
}

int wifi_start_ap(
	char 				*ssid,
	rtw_security_t		security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int					channel)
{
	const char *ifname = WLAN0_NAME;
	int ret = 0;

	if(wifi_mode == RTW_MODE_STA_AP) {
		ifname = WLAN1_NAME;
	}

	if(promisc_enabled())
		promisc_set(0, NULL, 0);

	wifi_reg_event_handler(WIFI_EVENT_STA_ASSOC, wifi_ap_sta_assoc_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_STA_DISASSOC, wifi_ap_sta_disassoc_hdl, NULL);

	ret = wext_set_mode(ifname, IW_MODE_MASTER);
	if(ret < 0) goto exit;
	ret = wext_set_channel(ifname, channel);	//Set channel before starting ap
	if(ret < 0) goto exit;

	switch(security_type) {
		case RTW_SECURITY_OPEN:
			break;
		case RTW_SECURITY_WPA2_AES_PSK:
			ret = wext_set_auth_param(ifname, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(ifname, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(ifname, (u8*)password, password_len);
			break;
		default:
			ret = -1;
			printf("\n\rWIFICONF: security type is not supported");
			break;
	}
	if(ret < 0) goto exit;
#if 0
//hide beacon SSID, add by serena_li
	u8 value = 1;   // 1: ssid = NUll, 2: ssid = ......, other: ssid = ssid
	ret = set_hidden_ssid(ifname, value);
	if(ret < 0) goto exit;
#endif
	ret = wext_set_ap_ssid(ifname, (u8*)ssid, ssid_len);
#ifdef CONFIG_WPS //construct WPS IE
	wpas_wps_init(ifname);
#endif
exit:
	return ret;
}

#if 0
static void wifi_scan_each_report_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	int i =0;
	int insert_pos = 0;
	rtw_scan_result_t** result_ptr = (rtw_scan_result_t**)buf;
	rtw_scan_result_t* temp = NULL;

	for(i=0; i<scan_result_handler_ptr.scan_cnt; i++){
		if(CMP_MAC(scan_result_handler_ptr.ap_details[i].BSSID.octet, (*result_ptr)->BSSID.octet)){
			memset(*result_ptr, 0, sizeof(rtw_scan_result_t));
			return;
		}
	}

	scan_result_handler_ptr.scan_cnt++;

	if(scan_result_handler_ptr.scan_cnt > scan_result_handler_ptr.max_ap_size){
		scan_result_handler_ptr.scan_cnt = scan_result_handler_ptr.max_ap_size;
		if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size-1]->signal_strength){
			rtw_memcpy(scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size-1], *result_ptr, sizeof(rtw_scan_result_t));
			temp = scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size -1];
		}else
			return;
	}else{
		rtw_memcpy(&scan_result_handler_ptr.ap_details[scan_result_handler_ptr.scan_cnt-1], *result_ptr, sizeof(rtw_scan_result_t));
	}

	for(i=0; i< scan_result_handler_ptr.scan_cnt-1; i++){
		if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[i]->signal_strength)
			break;
	}
	insert_pos = i;

	for(i = scan_result_handler_ptr.scan_cnt-1; i>insert_pos; i--)
		scan_result_handler_ptr.pap_details[i] = scan_result_handler_ptr.pap_details[i-1];

	if(temp != NULL)
		scan_result_handler_ptr.pap_details[insert_pos] = temp;
	else
		scan_result_handler_ptr.pap_details[insert_pos] = &scan_result_handler_ptr.ap_details[scan_result_handler_ptr.scan_cnt-1];
	rtw_memset(*result_ptr, 0, sizeof(rtw_scan_result_t));
}
#else
static void wifi_scan_each_report_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	int i =0;
	int j =0;
	int insert_pos = 0;
	rtw_scan_result_t** result_ptr = (rtw_scan_result_t**)buf;
	rtw_scan_result_t* temp = NULL;

	for(i=0; i<scan_result_handler_ptr.scan_cnt; i++){
		if(CMP_MAC(scan_result_handler_ptr.pap_details[i]->BSSID.octet, (*result_ptr)->BSSID.octet)){
			if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[i]->signal_strength){
				temp = scan_result_handler_ptr.pap_details[i];
				for(j = i-1; j >= 0; j--){
					if(scan_result_handler_ptr.pap_details[j]->signal_strength >= (*result_ptr)->signal_strength)
						break;
					else
						scan_result_handler_ptr.pap_details[j+1] = scan_result_handler_ptr.pap_details[j];
				}
				scan_result_handler_ptr.pap_details[j+1] = temp;
				scan_result_handler_ptr.pap_details[j+1]->signal_strength = (*result_ptr)->signal_strength;
			}
			memset(*result_ptr, 0, sizeof(rtw_scan_result_t));
			return;
		}
	}

	scan_result_handler_ptr.scan_cnt++;

	if(scan_result_handler_ptr.scan_cnt > scan_result_handler_ptr.max_ap_size){
		scan_result_handler_ptr.scan_cnt = scan_result_handler_ptr.max_ap_size;
		if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size-1]->signal_strength){
			rtw_memcpy(scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size-1], *result_ptr, sizeof(rtw_scan_result_t));
			temp = scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size -1];
		}else
			return;
	}else{
		rtw_memcpy(&scan_result_handler_ptr.ap_details[scan_result_handler_ptr.scan_cnt-1], *result_ptr, sizeof(rtw_scan_result_t));
	}

	for(i=0; i< scan_result_handler_ptr.scan_cnt-1; i++){
		if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[i]->signal_strength)
			break;
	}
	insert_pos = i;

	for(i = scan_result_handler_ptr.scan_cnt-1; i>insert_pos; i--)
		scan_result_handler_ptr.pap_details[i] = scan_result_handler_ptr.pap_details[i-1];

	if(temp != NULL)
		scan_result_handler_ptr.pap_details[insert_pos] = temp;
	else
		scan_result_handler_ptr.pap_details[insert_pos] = &scan_result_handler_ptr.ap_details[scan_result_handler_ptr.scan_cnt-1];
	rtw_memset(*result_ptr, 0, sizeof(rtw_scan_result_t));
}
#endif

static void wifi_scan_done_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	int i = 0;
	rtw_scan_handler_result_t scan_result_report;

	for(i=0; i<scan_result_handler_ptr.scan_cnt; i++){
		rtw_memcpy(&scan_result_report.ap_details, scan_result_handler_ptr.pap_details[i], sizeof(rtw_scan_result_t));
		scan_result_report.scan_complete = scan_result_handler_ptr.scan_complete;
		scan_result_report.user_data = scan_result_handler_ptr.user_data;
		(*scan_result_handler_ptr.gscan_result_handler)(&scan_result_report);
	}

	scan_result_handler_ptr.scan_complete = RTW_TRUE;
	scan_result_report.scan_complete = RTW_TRUE;
	(*scan_result_handler_ptr.gscan_result_handler)(&scan_result_report);

	rtw_free(scan_result_handler_ptr.ap_details);
	rtw_free(scan_result_handler_ptr.pap_details);
#if SCAN_USE_SEMAPHORE
	rtw_up_sema(&scan_result_handler_ptr.scan_semaphore);
#else
	scan_result_handler_ptr.scan_running = 0;
#endif
	wifi_unreg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, wifi_scan_each_report_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_SCAN_DONE, wifi_scan_done_hdl);
	return;
}

//int rtk_wifi_scan(char *buf, int buf_len, xSemaphoreHandle * semaphore)
int wifi_scan(rtw_scan_type_t                    scan_type,
				  rtw_bss_type_t                     bss_type,
				  void*                result_ptr)
{
	int ret;
	scan_buf_arg * pscan_buf;
	u16 flags = scan_type | (bss_type << 8);
	if(result_ptr != NULL){
		pscan_buf = (scan_buf_arg *)result_ptr;
		ret = wext_set_scan(WLAN0_NAME, (char*)pscan_buf->buf, pscan_buf->buf_len, flags);
	}else{
		wifi_reg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, wifi_scan_each_report_hdl, NULL);
		wifi_reg_event_handler(WIFI_EVENT_SCAN_DONE, wifi_scan_done_hdl, NULL);
		ret = wext_set_scan(WLAN0_NAME, NULL, 0, flags);
	}

	if(ret == 0) {
		if(result_ptr != NULL){
			ret = wext_get_scan(WLAN0_NAME, pscan_buf->buf, pscan_buf->buf_len);
		}
	}
	return ret;
}

extern void wifi_scan_ssid_cb(char *ssid, int ssid_len, u8_t security_mode, int rssi);

int wifi_scan_networks_with_ssid(rtw_scan_result_handler_t results_handler, void* user_data, char* in_ssid, int in_ssid_len){
	int scan_cnt = 0, add_cnt = 0;
	scan_buf_arg scan_buf;
	int ret;

	scan_buf.buf_len = *((int*)user_data);
	scan_buf.buf = (char*)pvPortMalloc(scan_buf.buf_len);
	if(!scan_buf.buf){
		printf("\n\rERROR: Can't mem_malloc memory");
		return RTW_NOMEM;
	}
	memset(scan_buf.buf, 0, scan_buf.buf_len);
	//set ssid
	if(in_ssid && in_ssid_len > 0 && in_ssid_len <= 32){
		memcpy(scan_buf.buf, &in_ssid_len, sizeof(int));
		memcpy(scan_buf.buf+sizeof(int), in_ssid, in_ssid_len);
	}
	//Scan channel
	if((scan_cnt = (wifi_scan(RTW_SCAN_TYPE_ACTIVE, RTW_BSS_TYPE_ANY, &scan_buf))) < 0){
		printf("\n\rERROR: wifi scan failed");
		ret = RTW_ERROR;
	}else{
		int plen = 0;
		while(plen < scan_buf.buf_len){
			int len, rssi, ssid_len; //i;
			u8 wps_password_id, security_mode;
			char *mac, *ssid;
			//u8 *security_mode;
			//printf("\n\r");
			// len
			len = (int)*(scan_buf.buf + plen);
			//printf("len = %d, ", len);
			// check end
			if(len == 0) break;
			// mac
			mac = scan_buf.buf + plen + 1;
			//printf("mac = ");
			//for(i=0; i<6; i++)
			//	printf("%02x,", (u8)*(mac+i));
			// rssi
			memcpy(&rssi, (scan_buf.buf + plen + 1 + 6), sizeof(int));
			//printf(" rssi = %d, ", rssi);
			// security_mode
			security_mode = *(scan_buf.buf + plen + 1 + 6 + 4);
			switch (security_mode) {
			case IW_ENCODE_ALG_NONE:
				//printf("sec = open    , ");
				break;
			case IW_ENCODE_ALG_WEP:
				//printf("sec = wep     , ");
				break;
			case IW_ENCODE_ALG_CCMP:
				//printf("sec = wpa/wpa2, ");
				break;
			}
			// password id
			wps_password_id = *(scan_buf.buf + plen + 1 + 6 + 4 + 1);
			//printf("wps password id = %d, ", wps_password_id);

			//printf("channel = %d ", *(scan_buf.buf + plen + 1 + 6 + 4 + 1 + 1));
			// ssid
			ssid_len = len - 1 - 6 - 4 - 1 - 1 - 1;
			ssid = scan_buf.buf + plen + 1 + 6 + 4 + 1 + 1 + 1;
			//printf("ssid = ");
			//for(i=0; i<ssid_len; i++)
			//	printf("%c", *(ssid+i));
			plen += len;
			add_cnt++;

			if ((in_ssid_len == ssid_len) && !strncmp(in_ssid, ssid, ssid_len)) {
				wifi_scan_ssid_cb(in_ssid, in_ssid_len, security_mode, rssi);
			}
		}

		//printf("\n\rwifi_scan: add count = %d, scan count = %d", add_cnt, scan_cnt);
		ret = RTW_SUCCESS;
	}

	if(scan_buf.buf)
	    vPortFree(scan_buf.buf);

	return ret;
}


int wifi_scan_networks(rtw_scan_result_handler_t results_handler, void* user_data)
{
	unsigned int max_ap_size = 24; //32;	/* mem_malloc mem(64*24) < 1536 */

#if SCAN_USE_SEMAPHORE
	rtw_bool_t result;
	if(NULL == scan_result_handler_ptr.scan_semaphore)
		rtw_init_sema(&scan_result_handler_ptr.scan_semaphore, 1);

	scan_result_handler_ptr.scan_start_time = rtw_get_current_time();
	/* Initialise the semaphore that will prevent simultaneous access - cannot be a mutex, since
	* we don't want to allow the same thread to start a new scan */
	result = (rtw_bool_t)rtw_down_timeout_sema(&scan_result_handler_ptr.scan_semaphore, SCAN_LONGEST_WAIT_TIME);
	if ( result != RTW_TRUE )
	{
		/* Return error result, but set the semaphore to work the next time */
		rtw_up_sema(&scan_result_handler_ptr.scan_semaphore);
		return RTW_TIMEOUT;
	}
#else
	if(scan_result_handler_ptr.scan_running){
		int count = 100;
		while(scan_result_handler_ptr.scan_running && count > 0)
		{
			rtw_msleep_os(20);
			count --;
		}
		if(count == 0){
			printf("\n\r[%d]WiFi: Scan is running. Wait 2s timeout.", rtw_get_current_time());
			return RTW_TIMEOUT;
		}
	}
	scan_result_handler_ptr.scan_start_time = rtw_get_current_time();
	scan_result_handler_ptr.scan_running = 1;
#endif

	scan_result_handler_ptr.gscan_result_handler = results_handler;

	scan_result_handler_ptr.max_ap_size = max_ap_size;
	scan_result_handler_ptr.ap_details = (rtw_scan_result_t*)rtw_zmalloc(max_ap_size*sizeof(rtw_scan_result_t));
	if(scan_result_handler_ptr.ap_details == NULL){
		goto error_with_result_ptr;
	}
	rtw_memset(scan_result_handler_ptr.ap_details, 0, max_ap_size*sizeof(rtw_scan_result_t));

	scan_result_handler_ptr.pap_details = (rtw_scan_result_t**)rtw_zmalloc(max_ap_size*sizeof(rtw_scan_result_t*));
	if(scan_result_handler_ptr.pap_details == NULL)
		return RTW_ERROR;
	rtw_memset(scan_result_handler_ptr.pap_details, 0, max_ap_size);

	scan_result_handler_ptr.scan_cnt = 0;

	scan_result_handler_ptr.scan_complete = RTW_FALSE;
	scan_result_handler_ptr.user_data = user_data;

	if (wifi_scan( RTW_SCAN_COMMAMD<<4 | RTW_SCAN_TYPE_ACTIVE, RTW_BSS_TYPE_ANY, NULL) != RTW_SUCCESS)
	{
		goto error_with_result_ptr;
	}

	return RTW_SUCCESS;

error_with_result_ptr:
	rtw_free((u8*)scan_result_handler_ptr.pap_details);
	scan_result_handler_ptr.pap_details = NULL;
	return RTW_ERROR;
}

//----------------------------------------------------------------------------//
int wifi_set_pscan_chan(__u8 * channel_list,__u8 length)
{
	if(channel_list)
	    return wext_set_pscan_channel(WLAN0_NAME, channel_list, length);
	else
	    return -1;
}

//----------------------------------------------------------------------------//
int wifi_get_setting(const char *ifname, rtw_wifi_setting_t *pSetting)
{
	int ret = 0;
	int mode = 0;
	unsigned short security = 0;

	memset(pSetting, 0, sizeof(rtw_wifi_setting_t));
	if(wext_get_mode(ifname, &mode) < 0)
		ret = -1;

	switch(mode) {
		case IW_MODE_MASTER:
			pSetting->mode = RTW_MODE_AP;
			break;
		case IW_MODE_INFRA:
		default:
			pSetting->mode = RTW_MODE_STA;
			break;
		//default:
			//printf("\r\n%s(): Unknown mode %d\n", __func__, mode);
			//break;
	}

	if(wext_get_ssid(ifname, pSetting->ssid) < 0)
		ret = -1;
	if(wext_get_channel(ifname, &pSetting->channel) < 0)
		ret = -1;
	if(wext_get_enc_ext(ifname, &security, &pSetting->key_idx, pSetting->password) < 0)
		ret = -1;

	switch(security){
		case IW_ENCODE_ALG_NONE:
			pSetting->security_type = RTW_SECURITY_OPEN;
			break;
		case IW_ENCODE_ALG_WEP:
			pSetting->security_type = RTW_SECURITY_WEP_PSK;
			break;
		case IW_ENCODE_ALG_TKIP:
			pSetting->security_type = RTW_SECURITY_WPA_TKIP_PSK;
			break;
		case IW_ENCODE_ALG_CCMP:
			pSetting->security_type = RTW_SECURITY_WPA2_AES_PSK;
			break;
		default:
			break;
	}

	if(security == IW_ENCODE_ALG_TKIP || security == IW_ENCODE_ALG_CCMP)
		if(wext_get_passphrase(ifname, pSetting->password) < 0)
			ret = -1;

	return ret;
}
//----------------------------------------------------------------------------//
int wifi_show_setting(const char *ifname, rtw_wifi_setting_t *pSetting)
{
	int ret = 0;

	printf("\n\r\nWIFI  %s Setting:",ifname);
	printf("\n\r==============================");

	switch(pSetting->mode) {
		case RTW_MODE_AP:
			printf("\n\r      MODE => AP");
			break;
		case RTW_MODE_STA:
			printf("\n\r      MODE => STATION");
			break;
		default:
			printf("\n\r      MODE => UNKNOWN");
	}

	printf("\n\r      SSID => %s", pSetting->ssid);
	printf("\n\r   CHANNEL => %d", pSetting->channel);

	switch(pSetting->security_type) {
		case RTW_SECURITY_OPEN:
			printf("\n\r  SECURITY => OPEN");
			break;
		case RTW_SECURITY_WEP_PSK:
			printf("\n\r  SECURITY => WEP");
			printf("\n\r KEY INDEX => %d", pSetting->key_idx);
			break;
		case RTW_SECURITY_WPA_AES_PSK:
			printf("\n\r  SECURITY => WPA_AES");
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
			printf("\n\r  SECURITY => WPA_TKIP");
			break;
		case RTW_SECURITY_WPA2_TKIP_PSK:
			printf("\n\r  SECURITY => WPA2_TKIP");
			break;
		case RTW_SECURITY_WPA2_AES_PSK:
			printf("\n\r  SECURITY => WPA2_AES");
			break;
		case RTW_SECURITY_WPA2_MIXED_PSK:
			printf("\n\r  SECURITY => WPA2_MIXED");
			break;
		default:
			printf("\n\r  SECURITY => UNKNOWN");
	}

	printf("\n\r  PASSWORD => %s", pSetting->password);
	printf("\n\r");

	return ret;
}

//----------------------------------------------------------------------------//
int wifi_set_network_mode(rtw_network_mode_t mode)
{
	if((mode == RTW_NETWORK_B) || (mode == RTW_NETWORK_BG) || (mode == RTW_NETWORK_BGN))
		return rltk_wlan_wireless_mode((unsigned char) mode);

	return -1;
}

int wifi_set_wps_phase(unsigned char is_trigger_wps)
{
	return rltk_wlan_set_wps_phase(is_trigger_wps);
}


//----------------------------------------------------------------------------//
int wifi_set_promisc(unsigned char enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used)
{
	return promisc_set(enabled, callback, len_used);
}

void wifi_enter_promisc_mode(){
	int mode = 0;
	unsigned char ssid[33];

	if(wifi_mode == RTW_MODE_STA_AP){
		wifi_off();
		vTaskDelay(20);
		wifi_on(RTW_MODE_PROMISC);
	}else{
		wext_get_mode(WLAN0_NAME, &mode);

		switch(mode) {
			case IW_MODE_MASTER:    //In AP mode
				rltk_wlan_deinit();
				vTaskDelay(20);
				rltk_wlan_init(0, RTW_MODE_PROMISC);
				rltk_wlan_start(0);
				break;
			case IW_MODE_INFRA:		//In STA mode
				if(wext_get_ssid(WLAN0_NAME, ssid) > 0)
					rtk_wifi_disconnect();
		}
	}
}

int wifi_restart_ap(
	unsigned char 		*ssid,
	rtw_security_t		security_type,
	unsigned char 		*password,
	int 				ssid_len,
	int 				password_len,
	int					channel)
{
	unsigned char idx = 0;
#if CONFIG_LWIP_LAYER
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct netif * pnetif = &xnetif[0];
#endif

#ifdef  CONFIG_CONCURRENT_MODE
	rtw_wifi_setting_t setting;
	int sta_linked = 0;
#endif

	if(rltk_wlan_running(WLAN1_IDX)){
		idx = 1;
	}

#if CONFIG_LWIP_LAYER
	// stop dhcp server
	dhcps_deinit();
#endif

#ifdef  CONFIG_CONCURRENT_MODE
	if(idx > 0){
		sta_linked = wifi_get_setting(WLAN0_NAME, &setting);
		wifi_off();
		vTaskDelay(20);
		wifi_on(RTW_MODE_STA_AP);
	}
	else
#endif
	{
#if CONFIG_LWIP_LAYER
		IP4_ADDR(&ipaddr, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		netif_set_addr(pnetif, &ipaddr, &netmask,&gw);
#endif
		wifi_off();
		vTaskDelay(20);
		wifi_on(RTW_MODE_AP);
	}
	// start ap
	if(wifi_start_ap((char*)ssid, security_type, (char*)password, ssid_len, password_len, channel) < 0) {
		printf("\n\rERROR: Operation failed!");
		return -1;
	}

#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
	printf("\r\nWebServer Thread: High Water Mark is %d\n", uxTaskGetStackHighWaterMark(NULL));
#endif

#ifdef  CONFIG_CONCURRENT_MODE
	// connect to ap if wlan0 was linked with ap
	if(idx > 0 && sta_linked == 0){
		int ret;
		printf("\r\nAP: ssid=%s", (char*)setting.ssid);
		printf("\r\nAP: security_type=%d", setting.security_type);
		printf("\r\nAP: password=%s", (char*)setting.password);
		printf("\r\nAP: key_idx =%d\n", setting.key_idx);
		ret = rtk_wifi_connect((char*)setting.ssid,
									setting.security_type,
									(char*)setting.password,
									strlen((char*)setting.ssid),
									strlen((char*)setting.password),
									setting.key_idx,
									NULL);
		if(ret == RTW_SUCCESS) {
#if CONFIG_DHCP_CLIENT
			/* Start DHCPClient */
			LwIP_DHCP(0, DHCP_START);
#elif CONFIG_ZEPHYR_LAYER
			zephyr_dhcp_start();
#endif
		}
	}
#endif
#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
	printf("\r\nWebServer Thread: High Water Mark is %d\n", uxTaskGetStackHighWaterMark(NULL));
#endif

#if CONFIG_LWIP_LAYER
	// start dhcp server
	dhcps_init(&xnetif[idx]);
#endif

	return 0;
}

#ifdef CONFIG_AUTO_RECONNECT
int wifi_set_autoreconnect(__u8 mode)
{
	return wext_set_autoreconnect(WLAN0_NAME, mode);
}

int wifi_get_autoreconnect(__u8 *mode)
{
	return wext_get_autoreconnect(WLAN0_NAME, mode);
}
#endif

#ifdef CONFIG_CUSTOM_IE
/*
 * Example for custom ie
 *
 * u8 test_1[] = {221, 2, 2, 2};
 * u8 test_2[] = {221, 2, 1, 1};
 * rtw_custom_ie_t buf[2] = {{test_1, PROBE_REQ},
 *		 {test_2, PROBE_RSP | BEACON}};
 * u8 buf_test2[] = {221, 2, 1, 3} ;
 * rtw_custom_ie_t buf_update = {buf_test2, PROBE_REQ};
 *
 * add ie list
 * static void cmd_add_ie(int argc, char **argv)
 * {
 *	 wifi_add_custom_ie((void *)buf, 2);
 * }
 *
 * update current ie
 * static void cmd_update_ie(int argc, char **argv)
 * {
 *	 wifi_update_custom_ie(&buf_update, 2);
 * }
 *
 * delete all ie
 * static void cmd_del_ie(int argc, char **argv)
 * {
 *	 wifi_del_custom_ie();
 * }
 */

int wifi_add_custom_ie(void *cus_ie, int ie_num)
{
	return wext_add_custom_ie(WLAN0_NAME, cus_ie, ie_num);
}


int wifi_update_custom_ie(void *cus_ie, int ie_index)
{
	return wext_update_custom_ie(WLAN0_NAME, cus_ie, ie_index);
}

int wifi_del_custom_ie()
{
	return wext_del_custom_ie(WLAN0_NAME);
}

#endif

#ifdef CONFIG_PROMISC
void promisc_init_packet_filter();
int promisc_add_packet_filter(u8 filter_id, rtw_packet_filter_pattern_t *patt, rtw_packet_filter_rule_t rule);
int promisc_enable_packet_filter(u8 filter_id);
int promisc_disable_packet_filter(u8 filter_id);
int promisc_remove_packet_filter(u8 filter_id);
void wifi_init_packet_filter()
{
	promisc_init_packet_filter();
}

int wifi_add_packet_filter(unsigned char filter_id, rtw_packet_filter_pattern_t *patt, rtw_packet_filter_rule_t rule)
{
	return promisc_add_packet_filter(filter_id, patt, rule);
}

int wifi_enable_packet_filter(unsigned char filter_id)
{
	return promisc_enable_packet_filter(filter_id);
}

int wifi_disable_packet_filter(unsigned char filter_id)
{
	return promisc_disable_packet_filter(filter_id);
}

int wifi_remove_packet_filter(unsigned char filter_id)
{
	return promisc_remove_packet_filter(filter_id);
}
#endif

#ifdef CONFIG_SUSPEND
void system_suspend_resume();
void wifi_suspend_resume()
{
	printf("\r\n%s!", __func__);
	if(wifi_mode != RTW_MODE_STA){
		wifi_off();
	}else{
		rtw_suspend(wifi_mode);
	}

	system_suspend_resume();

	if(wifi_mode != RTW_MODE_STA){
		wifi_on(RTW_MODE_AP);
	}else{
		rtw_resume(wifi_mode);
	}
}
#endif

extern int promisc_issue_probereq(char* ssid, u8 ssid_len);
int wifi_issue_probereq(char* ssid, u8 ssid_len)
{
	return promisc_issue_probereq(ssid, ssid_len);
}

#ifdef GENERALPLUS_REQUEST
extern int rltk_tp_counter(void);
int wifi_TP_counter(void)
{
	return rltk_tp_counter();
}
#endif
//----------------------------------------------------------------------------//
#endif	//#if CONFIG_WLAN
