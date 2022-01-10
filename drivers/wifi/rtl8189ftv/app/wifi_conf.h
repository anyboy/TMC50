//----------------------------------------------------------------------------//
#ifndef __WIFI_CONF_H
#define __WIFI_CONF_H

#include "../wlan/wifi_constants.h"
#include "../wlan/wifi_structures.h"
#include "../wlan/wireless.h"
#include "../wlan/wlan_intf.h"
#include "../wlan/osdep_service.h"
#include "../osdep/clib_dep.h"
#include "wifi_ind.h"
#include "wifi_util.h"

#ifdef __cplusplus
  extern "C" {
#endif

#define CONFIG_WLAN 			1
#define NET_IF_NUM				1
#define WLAN0_NAME				"wlan0"
#define WLAN1_NAME				"wlan1"
#define CONFIG_PROMISC			1
#define CONFIG_ENABLE_AP		0

#define CONFIG_ZEPHYR_LAYER		1


/******************************************************
 *                    Macros
 ******************************************************/

#define RTW_ENABLE_API_INFO

#ifdef RTW_ENABLE_API_INFO
    #define RTW_API_INFO(args) do {printf args;} while(0)
#else
    #define RTW_API_INFO(args)
#endif

#define MAC_ARG(x) ((u8*)(x))[0],((u8*)(x))[1],((u8*)(x))[2],((u8*)(x))[3],((u8*)(x))[4],((u8*)(x))[5]
#define CMP_MAC( a, b )  (((a[0])==(b[0]))&& \
                          ((a[1])==(b[1]))&& \
                          ((a[2])==(b[2]))&& \
                          ((a[3])==(b[3]))&& \
                          ((a[4])==(b[4]))&& \
                          ((a[5])==(b[5])))

/******************************************************
 *                    Constants
 ******************************************************/
#define SCAN_LONGEST_WAIT_TIME  (4500)


#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"

/******************************************************
 *                 Type Definitions
 ******************************************************/

/** Scan result callback function pointer type
 *
 * @param result_ptr  : A pointer to the pointer that indicates where to put the next scan result
 * @param user_data   : User provided data
 */
typedef void (*rtw_scan_result_callback_t)( rtw_scan_result_t** result_ptr, void* user_data );
typedef rtw_result_t (*rtw_scan_result_handler_t)( rtw_scan_handler_result_t* malloced_scan_result );

/******************************************************
 *                    Structures
 ******************************************************/
typedef enum _WIFI_LINK_STATUS{
	WIFI_LINK_DISCONNECTED = 0,
	WIFI_LINK_CONNECTED
}WIFI_LINK_STATUS;

typedef struct {
	char *buf;
	int buf_len;
} scan_buf_arg;

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct internal_scan_handler{
	rtw_scan_result_t** pap_details;
	rtw_scan_result_t * ap_details;
	int	scan_cnt;
	rtw_bool_t	scan_complete;
	unsigned char	max_ap_size;
	rtw_scan_result_handler_t gscan_result_handler;
#if SCAN_USE_SEMAPHORE
	void *scan_semaphore;
#else
	int 	scan_running;
#endif
	void*	user_data;
	unsigned int	scan_start_time;
} internal_scan_handler_t;

typedef struct {
    rtw_network_info_t	network_info;
    void *join_sema;
} internal_join_result_t;

/******************************************************
 *               Function Declarations
 ******************************************************/
/**
 * Initialises Realtek WiFi API System
 *
 * - Initialises the required parts of the software platform
 *   i.e. worker, event registering, semaphore, etc.
 *
 * - Initialises the RTW API thread which handles the asynchronous event
 *
 * @return RTW_SUCCESS if initialization is successful, RTW_ERROR otherwise
 */
int wifi_manager_init(void);

/** Joins a Wi-Fi network
 *
 * Scans for, associates and authenticates with a Wi-Fi network.
 * On successful return, the system is ready to send data packets.
 *
 * @param[in] ssid       : A null terminated string containing the SSID name of the network to join
 * @param[in] security_type  : Authentication type:
 *                         - RTW_SECURITY_OPEN           - Open Security
 *                         - RTW_SECURITY_WEP_PSK        - WEP Security with open authentication
 *                         - RTW_SECURITY_WEP_SHARED     - WEP Security with shared authentication
 *                         - RTW_SECURITY_WPA_TKIP_PSK   - WPA Security
 *                         - RTW_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher
 *                         - RTW_SECURITY_WPA2_TKIP_PSK  - WPA2 Security using TKIP cipher
 *                         - RTW_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers
 * @param[in] password : A byte array containing either the
 *  						 cleartext security key for WPA/WPA2
 *  						 secured networks, or a pointer to
 *  						 an array of rtw_wep_key_t
 *  						 structures for WEP secured networks
 * @param[in] ssid_len  : The length of the SSID in
 *  	 bytes.
 * @param[in] password_len  : The length of the security_key in
 *  	 bytes.
 * @param[in] key_id  : The index of the wep key.
 * @param[in] semaphore   : A user provided semaphore that is flagged when the join is complete
 *
 * @return    RTW_SUCCESS : when the system is joined and ready
 *  		  to send data packets
 *   	   	 RTW_ERROR   : if an error occurred
 */
int rtk_wifi_connect(
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore);

int wifi_connect_bssid(
	unsigned char 		bssid[ETH_ALEN],
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				bssid_len,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore);
/** Disassociates from a Wi-Fi network.
 *
 * @return    RTW_SUCCESS : On successful disassociation from
 *  		  the AP
 *           RTW_ERROR   : If an error occurred
*/
int rtk_wifi_disconnect(void);

/** Check if the interface specified is up.
 *
 * @return    RTW_TRUE   : If it's up
 *           RTW_FALSE   : If it's not
*/
int wifi_is_up(rtw_interface_t interface);

/** Determines if a particular interface is ready to transceive ethernet packets
 *
 * @param     Radio interface to check, options are
 *  				RTW_STA_INTERFACE, RTW_AP_INTERFACE
 * @return    RTW_SUCCESS  : if the interface is ready to
 *  		  transceive ethernet packets
 * @return    RTW_NOTFOUND : no AP with a matching SSID was
 *  		  found
 * @return    RTW_NOT_AUTHENTICATED: a matching AP was found but
 *  								   it won't let you
 *  								   authenticate. This can
 *  								   occur if this device is
 *  								   in the block list on the
 *  								   AP.
 * @return    RTW_NOT_KEYED: the device has authenticated and
 *  						   associated but has not completed
 *  						   the key exchange. This can occur
 *  						   if the passphrase is incorrect.
 * @return    RTW_ERROR    : if the interface is not ready to
 *  		  transceive ethernet packets
 */
int wifi_is_ready_to_transceive(rtw_interface_t interface);

/** ----------------------------------------------------------------------
 *  WARNING : This function is for internal use only!
 *  ----------------------------------------------------------------------
 *  This function sets the current Media Access Control (MAC) address of the
 *  802.11 device.
 *
 * @param[in] mac Wi-Fi MAC address
 * @return    0 for success or -1 for invalid format  or -2 for write flash error
 */
int wifi_set_mac_address(char * mac);

/** Retrieves the current Media Access Control (MAC) address
 *  (or Ethernet hardware address) of the 802.11 device
 *
 * @param mac Pointer to a variable that the current MAC address will be written to
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_get_mac_address(char * mac);

/** Enables powersave mode
 *
 * @return @ref rtw_result_t
 */
int wifi_enable_powersave(void);

/** Disables 802.11 power save mode
 *
 * @return  RTW_SUCCESS : if power save mode was successfully
 *  		disabled
 *         RTW_ERROR   : if power save mode was not successfully
 *          disabled
 */
int wifi_disable_powersave(void);

/** Gets the tx power in index units
 *
 * @param cckpoweridx/ofdmpoweridx : The variable to receive the tx power in index.
 *
 * @return  RTW_SUCCESS : if successful
 *          RTW_ERROR   : if not successful
 */
int wifi_get_txpower(int *cckpoweridx, int*ofdmpoweridx);

/** Sets the tx power in index units
 *
 * @param poweridx : The desired tx power in index.
 * @param bcck : 1 - cck set ; 0 - ofdm set.
 *
 * @return  RTW_SUCCESS : if tx power was successfully set
 *          RTW_ERROR   : if tx power was not successfully set
 */
int wifi_set_txpower(int bcck, int poweridx);

/** Get the associated clients with SoftAP
 *
 * @param client_list_buffer : the location where the client
 *  			  list will be stored
 * @param buffer_length : the buffer length.
 *
 * @return  RTW_SUCCESS : if result was successfully get
 *          RTW_ERROR   : if result was not successfully get
 */
int wifi_get_associated_client_list(void * client_list_buffer, unsigned short buffer_length);

/** Get the SoftAP information
 *
 * @param ap_info : the location where the AP info will be
 *  			  stored
 * @param security : the security type.
 *
 * @return  RTW_SUCCESS : if result was successfully get
 *          RTW_ERROR   : if result was not successfully get
 */
int wifi_get_ap_info(rtw_bss_info_t * ap_info, rtw_security_t* security);

/** Set the country code to driver to determine the channel set
 *
 * @param country_code : the country code.
 *
 * @return  RTW_SUCCESS : if result was successfully set
 *          RTW_ERROR   : if result was not successfully set
 */
int wifi_set_country(rtw_country_code_t country_code);

/** Retrieve the latest RSSI value
 *
 * @param rssi: The location where the RSSI value will be stored
 *
 * @return  RTW_SUCCESS : if the RSSI was succesfully retrieved
 *          RTW_ERROR   : if the RSSI was not retrieved
 */
int wifi_get_rssi(int *pRSSI);

/** Set the current channel on STA interface
 *
 * @param channel   : The desired channel
 *
 * @return  RTW_SUCCESS : if the channel was successfully set
 *  		RTW_ERROR   : if the channel was not successfully
 *  		set
 */
int wifi_set_channel(int channel);

/** Get the current channel on STA interface
 *
 * @param channel   : A pointer to the variable where the
 *  				channel value will be written
 *
 * @return  RTW_SUCCESS : if the channel was successfully read
 *  		RTW_ERROR   : if the channel was not successfully
 *  		read
 */
int wifi_set_ext_channel(int num);

/**set current bandwidth 40M
 *
 *@param num :
 *			0 20M bandwidth
 *			1 ext channel use upper 20M
 * 			2 ext channel use lower 20M
 *
 *@return :
 *			-1:error
 * 			0:success
 *
 */
int wifi_get_channel(int *channel);

/** Registers interest in a multicast address
 * Once a multicast address has been registered, all packets detected on the
 * medium destined for that address are forwarded to the host.
 * Otherwise they are ignored.
 *
 * @param mac: Ethernet MAC address
 *
 * @return  RTW_SUCCESS : if the address was registered
 *  		successfully
 *         RTW_ERROR   : if the address was not registered
 */
int wifi_register_multicast_address(rtw_mac_t *mac);

/** Unregisters interest in a multicast address
 * Once a multicast address has been unregistered, all packets detected on the
 * medium destined for that address are ignored.
 *
 * @param mac: Ethernet MAC address
 *
 * @return  RTW_SUCCESS : if the address was unregistered
 *  		successfully
 *         RTW_ERROR   : if the address was not unregistered
 */
int wifi_unregister_multicast_address(rtw_mac_t *mac);

int wifi_rf_on(void);
int wifi_rf_off(void);

/** Turn on the Wi-Fi device
 *
 * - Bring the Wireless interface "Up"
 * - Initialises the driver thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @param mode: wifi work mode
 *
 * @return  RTW_SUCCESS : if the WiFi chip was initialised
 *  		successfully
 *         RTW_ERROR   : if the WiFi chip was not initialised
 *  		successfully
 */
int wifi_on(rtw_mode_t mode);

/**
 * Turn off the Wi-Fi device
 *
 * - Bring the Wireless interface "Down"
 * - De-Initialises the driver thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return RTW_SUCCESS if deinitialization is successful,
 *  	   RTW_ERROR otherwise
 */
int wifi_off(void);

int wifi_autochl_select(int low_chl, int hi_chl);

/** Starts an infrastructure WiFi network
 *
 * @warning If a STA interface is active when this function is called, the softAP will\n
 *          start on the same channel as the STA. It will NOT use the channel provided!
 *
 * @param[in] ssid       : A null terminated string containing
 *  	 the SSID name of the network to join
 * @param[in] security_type  : Authentication type: \n
 *                         - RTW_SECURITY_OPEN           - Open Security \n
 *                         - RTW_SECURITY_WPA_TKIP_PSK   - WPA Security \n
 *                         - RTW_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher \n
 *                         - RTW_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers \n
 *                         - WEP security is NOT IMPLEMENTED. It is NOT SECURE! \n
 * @param[in] password : A byte array containing the cleartext
 *  	 security key for the network
 * @param[in] ssid_len  : The length of the SSID in
 *  	 bytes.
 * @param[in] password_len   : The length of the security_key in
 *  	 bytes.
 * @param[in] channel      : 802.11 channel number
 *
 * @return    RTW_SUCCESS : if successfully creates an AP
 *            RTW_ERROR   : if an error occurred
 */
int wifi_start_ap(
	char 				*ssid,
	rtw_security_t		security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int					channel);

/** Initiates a scan to search for 802.11 networks.
 *
 *  The scan progressively accumulates results over time, and
 *  may take between 1 and 3 seconds to complete. The results of
 *  the scan will be individually provided to the callback
 *  function. Note: The callback function will be executed in
 *  the context of the RTW thread.
 *
 * @param[in]  scan_type     : Specifies whether the scan should
 *  	 be Active, Passive or scan Prohibited channels
 * @param[in]  bss_type      : Specifies whether the scan should
 *  						   search for Infrastructure
 *  						   networks (those using an Access
 *  						   Point), Ad-hoc networks, or both
 *  						   types.
 * @param callback[in]   : the callback function which will
 *  			  receive and process the result data.
 * @param result_ptr[in] : a pointer to a pointer to a result
 *  				storage structure.
 * @param user_data[in]  : user specific data that will be
 *  			   passed directly to the callback function
 *
 * @note : When scanning specific channels, devices with a
 *  	 strong signal strength on nearby channels may be
 *  	 detected
 * @note : Callback must not use blocking functions, since it is
 *  	   called from the context of the RTW thread.
 * @note : The callback, result_ptr and user_data variables will
 *  	   be referenced after the function returns. Those
 *  	   variables must remain valid until the scan is
 *  	   complete.
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_scan(rtw_scan_type_t                    scan_type,
				  rtw_bss_type_t                     bss_type,
				  void*                result_ptr);

/** Initiates a scan to search for 802.11 networks, a higher
 *  level API based on wifi_scan to simplify the scan
 *  operation.
 *
 *  The scan results will be list by the order of RSSI.
 *  It may demand hundreds bytes memory during scan
 *  processing according to the quantity of AP nearby.
 *
 * @param results_handler[in]   : the callback function which
 *  			  will receive and process the result data.
 * @param user_data[in]  : user specific data that will be
 *  			   passed directly to the callback function
 *
 * @note : Callback must not use blocking functions, since it is
 *  	   called from the context of the RTW thread.
 * @note : The callback, user_data variables will
 *  	   be referenced after the function returns. Those
 *  	   variables must remain valid until the scan is
 *  	   complete.
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_scan_networks(rtw_scan_result_handler_t results_handler, void* user_data);
int wifi_scan_networks_with_ssid(rtw_scan_result_handler_t results_handler, void* user_data, char* in_ssid, int in_ssid_len);

/** Set the partical scan
 *
 * @param channel_list[in]   : the channel set the scan will
 *  				  stay on
 * @param length[in]  : the channel list length
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_set_pscan_chan(__u8 * channel_list,__u8 length);

/** Get the network information
 *
 * @param ifname[in]   : the name of the interface we are care
 * @param pSetting[in]  : the location where the network
 *  			  information will be stored
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_get_setting(const char *ifname,rtw_wifi_setting_t *pSetting);

/** Show the network information
 *
 * @param ifname[in]   : the name of the interface we are care
 * @param pSetting[in]  : the location where the network
 *  			  information was stored
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_show_setting(const char *ifname,rtw_wifi_setting_t *pSetting);

/** Set the network mode according to the data rate it's
 *  supported
 *  
 * @param mode[in]   : the network mode
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_set_network_mode(rtw_network_mode_t mode);

/** Set the chip to worke in the promisc mode
 *  
 * @param enabled[in]   : enabled can be set 0, 1 and 2. if rcr_level is zero, disable the promisc, else enable the promisc.
 *                    0 means disable the promisc
 *                    1 means enable the promisc
 *                    2 means enable the promisc special for length is used
 * @param callback[in]   : the callback function which will 
 *  			  receive and process the netowork data.
 * @param len_used[in]   : specify if the the promisc length is 
 *  			  used.
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_set_promisc(unsigned char enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used);

/** Set the wps phase
 *  
 * @param is_trigger_wps[in]   : to trigger wps function or not
 *
 * @return    RTW_SUCCESS or RTW_ERROR
 */
int wifi_set_wps_phase(unsigned char is_trigger_wps);

/** Restarts an infrastructure WiFi network
 *
 * @warning If a STA interface is active when this function is called, the softAP will\n
 *          start on the same channel as the STA. It will NOT use the channel provided!
 *
 * @param[in] ssid       : A null terminated string containing 
 *  	 the SSID name of the network to join
 * @param[in] security_type  : Authentication type: \n
 *                         - RTW_SECURITY_OPEN           - Open Security \n
 *                         - RTW_SECURITY_WPA_TKIP_PSK   - WPA Security \n
 *                         - RTW_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher \n
 *                         - RTW_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers \n
 *                         - WEP security is NOT IMPLEMENTED. It is NOT SECURE! \n
 * @param[in] password : A byte array containing the cleartext 
 *  	 security key for the network
 * @param[in] ssid_len  : The length of the SSID in 
 *  	 bytes.
 * @param[in] password_len   : The length of the security_key in 
 *  	 bytes.
 * @param[in] channel      : 802.11 channel number
 *
 * @return    RTW_SUCCESS : if successfully creates an AP
 *            RTW_ERROR   : if an error occurred
 */
int wifi_restart_ap(
	unsigned char 		*ssid,
	rtw_security_t		security_type,
	unsigned char 		*password,
	int 				ssid_len,
	int 				password_len,
	int					channel);


int wifi_set_autoreconnect(__u8 mode);
int wifi_get_autoreconnect(__u8 *mode);

#ifdef CONFIG_CUSTOM_IE
#define BIT(x)	((__u32)1 << (x))

#ifndef _CUSTOM_IE_TYPE_
#define _CUSTOM_IE_TYPE_
typedef enum CUSTOM_IE_TYPE{
	PROBE_REQ = BIT(0),
	PROBE_RSP = BIT(1),
	BEACON	  = BIT(2),
}rtw_custom_ie_type_t;
#endif /* _CUSTOM_IE_TYPE_ */

/* ie format
 * +-----------+--------+-----------------------+
 * |element ID | length | content in length byte|
 * +-----------+--------+-----------------------+
 *
 * type: refer to CUSTOM_IE_TYPE
 */
#ifndef _CUS_IE_
#define _CUS_IE_
typedef struct _cus_ie{
	__u8 *ie;
	__u8 type;
}rtw_custom_ie_t, *p_rtw_custom_ie_t;
#endif /* _CUS_IE_ */

int wifi_add_custom_ie(void *cus_ie, int ie_num);

int wifi_update_custom_ie(void *cus_ie, int ie_index);

int wifi_del_custom_ie(void);
#endif

#ifdef CONFIG_PROMISC
void wifi_init_packet_filter(void);
int wifi_add_packet_filter(unsigned char filter_id, rtw_packet_filter_pattern_t *patt, rtw_packet_filter_rule_t rule);
int wifi_enable_packet_filter(unsigned char filter_id);
int wifi_disable_packet_filter(unsigned char filter_id);
int wifi_remove_packet_filter(unsigned char filter_id);
#endif

#ifdef CONFIG_SUSPEND
void wifi_suspend_resume(void);
#endif

int wifi_issue_probereq(char* ssid, unsigned char ssid_len);
#ifdef GENERALPLUS_REQUEST
int wifi_TP_counter(void);
#endif

extern int wifi_interactive_cmd(int argc, char **argv);

#ifdef __cplusplus
  }
#endif

#endif // __WIFI_CONF_H

//----------------------------------------------------------------------------//
