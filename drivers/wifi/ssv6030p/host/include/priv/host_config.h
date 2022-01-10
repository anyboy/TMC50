/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _SIM_CONFIG_H_
#define _SIM_CONFIG_H_

#include <porting.h>
#include <ssv_types.h>
#include <ssv_ex_lib.h>
// ------------------------- debug ---------------------------

#define CONFIG_STATUS_CHECK         0 //1
#define AP_MODE_BEACON_VIRT_BMAP_0XFF         1		// 1: FIX VIRT BMAP 0XFF, 0:DYNAMIC VIRT BMAP

//-----------------------------------------------------------
#define Sleep                       OS_MsDelay
#define AUTO_INIT_STATION           1


#define CONFIG_CHECKSUM_DCHECK      0
#define TARGET						prj1_config

// ------------------------- log -------------------------------
#define SSV_LOG_DEBUG           1


/** lower two bits indicate debug level
 * - 0 all
 * - 1 warning
 * - 2 serious
 * - 3 severe
 */
#define LOG_LEVEL_ALL     0x00
#define LOG_LEVEL_WARNING 0x01 	/* bad checksums, dropped packets, ... */
#define LOG_LEVEL_SERIOUS 0x02 	/* memory allocation failures, ... */
#define LOG_LEVEL_SEVERE  0x03
#define LOG_LEVEL_MASK_LEVEL 0x03


#define LOG_NONE	       			   (0)
#define LOG_MEM	       			       (1<<16)
#define LOG_L3_SOCKET				   (1<<17)
#define LOG_L3_API				   	   (1<<18)
#define LOG_L3_TCP					   (1<<19)
#define LOG_L3_UDP					   (1<<20)
#define LOG_L3_IP					   (1<<21)
#define LOG_L3_OTHER_PROTO			   (1<<22)
#define LOG_L2_DATA			           (1<<23)
#define LOG_L2_AP			           (1<<24)
#define LOG_L2_STA			           (1<<25)
#define LOG_CMDENG			           (1<<26)
#define LOG_TXRX			           (1<<27)
#define LOG_SCONFIG			           (1<<28)

#define LOG_L4_HTTPD                   (1<<29)
#define LOG_L4_NETMGR   	           (1<<30)
#define LOG_L4_DHCPD			       (1<<31)
#define LOG_L4_IPERF                   (1<<32)

#define LOG_ALL_MODULES                (0xffffffff)

//***********************************************//
//Modify default log config
#define CONFIG_LOG_MODULE				(LOG_ALL_MODULES)
#define CONFIG_LOG_LEVEL				(LOG_LEVEL_SERIOUS)

// ------------------------- chip -------------------------------
#define SSV6051Q    0
#define SSV6051Z    1
#define SSV6030P    2
#define SSV6052Q    3
#define SSV6006     4
#define SSV8031B    5

#define CONFIG_CHIP_ID     SSV6030P

// ------------------------PHY mode ----------------------------
#define PHY_GB_MODE    0
#define PHY_NB_MODE    1

#define PHY_SUPPORT_HT PHY_NB_MODE
// ------------------------- rtos -------------------------------
#define OS_MUTEX_SIZE				10              // Mutex
#define OS_COND_SIZE				5               // Conditional variable
#define OS_TIMER_SIZE				5               // OS Timer
#define OS_MSGQ_SIZE				5               // Message Queue

// ------------------------- cli -------------------------------
#define CLI_ENABLE					0               // Enable/Disable CLI
#if (CLI_ENABLE==1)
#define CLI_TASK_ENABLE             1

#if (CLI_TASK_ENABLE==1)
#define CLI_HISTORY_ENABLE			1               // Enable/Disable CLI history log. only work in SIM platofrm for now.
#define CLI_HISTORY_NUM				10

#define CLI_BUFFER_SIZE				80              // CLI Buffer size
#define CLI_ARG_SIZE				20             // The number of arguments
#define CLI_PROMPT					"wifi-host> "
#else
#define CLI_ARG_SIZE				10             // The number of arguments
#endif

#endif
#define CONFIG_BUS_LOOPBACK_TEST 0 // 1
// ------------------------- misc ------------------------------
#define _INTERNAL_MCU_SUPPLICANT_SUPPORT_
#define CONFIG_HOST_PLATFORM         1
/*test for sdio/spi drv*/
#define BUS_TEST_MODE 0
#define SSV_TMR_MAX   24

//STA mode change the RX sensitive dynamically
#define ENABLE_DYNAMIC_RX_SENSITIVE 1

//TX power mode , only workable for 6030P
#define TX_POWER_NORMAL          0
#define TX_POWER_B_HIGH_ONLY     1
#define TX_POWER_ENHANCE_ALL     2

#define CONFIG_TX_PWR_MODE     TX_POWER_B_HIGH_ONLY

//Voltage setting: LDO or DCDC
#define VOLT_LDO_REGULATOR  0
#define VOLT_DCDC_CONVERT   1

#ifdef CONFIG_WIFI_DCDC_MODE
#define SSV_VOLT_REGULATOR  VOLT_DCDC_CONVERT
#else
#define SSV_VOLT_REGULATOR  VOLT_LDO_REGULATOR
#endif

//Temperature compensate
#define TMPT_HIGH       100
#define TMPT_LOW        0
// ------------------------- mlme ------------------------------
/*Regular iw scan or regulare ap list update*/
#define CONFIG_AUTO_SCAN            0               // 1=auto scan, 0=auto flush ap_list table
#define MLME_TASK                   0               // MLME function 0=close,1=open. PS:If MLME_TASK=1, please re-set the stack size to 64 in porting.h

#if (MLME_TASK == 0)
#define TIMEOUT_TASK                MBOX_CMD_ENGINE               // MLME function 0=close,1=open
#else
#define TIMEOUT_TASK                MBOX_MLME_TASK               // MLME function 0=close,1=open
#endif

#define NUM_AP_INFO                 10
// ------------------------- txrx ------------------------------
#define RXFLT_ENABLE                0 //1               // Rx filter for data frames
#define ONE_TR_THREAD               0               // one txrx thread. PS: If ONE_TR_THREAD=1, please re-set the value of WIFI_RX_STACK_SIZE to 0 in porting.h
#define TX_TASK_SLEEP               1               // Tx task go to sleep 1 tick when get no tx resource 200 time
#define TX_TASK_SLEEP_TICK          1               // how long does the tx task sleep? The default time is 1 tick
#define TX_TASK_RETRY_CNT           200             // Tx task retry count when tx resource of wifi-chip is not enough

#define TX_RESOURCE_PAGE            60
#define RX_RESOURCE_PAGE            61

#define TXDUTY_AT_HOST 0
#define TXDUTY_AT_FW   1

#define TXDUTY_MODE    TXDUTY_AT_FW
// ------------------------- network ---------------------------
/* TCP/IP Configuration: */
#ifndef USE_ICOMM_LWIP
#define USE_ICOMM_LWIP          1
#endif
#define CONFIG_MAX_NETIF        1
#define AP_MODE_ENABLE          0
#define WLAN_MAX_STA            2
#define CONFIG_PLATFORM_CHECK   1

// ------------------------- mac address  ------------------------------
#define CONFIG_EFUSE_MAC            1
#define	CONFIG_RANDOM_MAC           1

// ------------------------- Deafult channel in STA mode ---------------------------
#define STA_DEFAULT_CHANNEL 6

// ------------------------- STA channel mask  ------------------------------
//This macro is for STA mode, evey bit corrsebonds to a channel, for example: bit[0] -> channel 0, bit[1] ->cahnnel 1.
//If user assign 0 to channel mask to netmgr_wifi_scan or netmgr_wifi_sconfig, we use the default value.
//If youe set ch0 and ch15, we will filter it automatically
#define DEFAULT_STA_CHANNEL_MASK 0x7FFE //from 1~14

// ------------------------- auto channel selection ---------------------------
//The ACS_CHANNEL_MASK is for auto channel selection in AP  mode, evey bit corrsebonds to a channel, for example: bit[0] -> channel 0, bit[1] ->cahnnel 1.
//Now, we set 0xFFE, this value means we do auto channel selection from channel 1 to channel 11.
//If you just want to choose a channel from 1, 6,11, you must set ACS_CHANNEL_MASK  to 0x842
//If youe set ch0, ch12, ch13, ch14, or ch15, we will filter it automatically
#define ACS_CHANNEL_MASK 0xFFE

//The g_acs_channel_scanning_interval is for auto channel secltion in AP mode, if you set 10, it means AP will stay in one channel for 10x10ms,
//and then change to the next channel.
#define ACS_SCANNING_INTERVAL 10 //The unit is 10ms

//This macro is used to set the number of times that you want to do the channel scanning.
//If this macro is 1, we do channel scanning one times, if this macro is 2, we do channel scanning two times, and then we choose a channel by all datas
#define ACS_SCANNING_LOOP 2

// ------------------------- Smart Config ---------------------------
#define ENABLE_SMART_CONFIG 1
#define SMART_CONFIG_AUTO_JOIN 0 //Auto join AP after getting the result of smart config
//The SmartConfig's solutions
#define WECHAT_AIRKISS_IN_FW 0
#define WECHAT_AIRKISS_ON_HOST 1
#define ICOMM_SMART_LINK 2
#define CUSTOMER_SOLUTION  3

//Define the solution
#define SMART_CONFIG_SOLUTION WECHAT_AIRKISS_IN_FW //ICOMM_SMART_LINK

#ifndef __SSV_UNIX_SIM__
    #if(SMART_CONFIG_SOLUTION==WECHAT_AIRKISS_ON_HOST)
    //# ERROR Configuration ...
    #endif
#endif

//This macro is for SmartConfig mode, evey bit corrsebonds to a channel, for example: bit[0] -> channel 0, bit[1] ->cahnnel 1.
//If user assign 0 to channel mask to netmgr_wifi_scan or netmgr_wifi_sconfig, we use the default value.
//If youe set ch0 and ch15, we will filter it automatically
#define DEFAULT_SCONFIG_CHANNEL_MASK 0x7FFE //from 1~14

// ------------------------- ---------------------------------------
#if (AP_MODE_ENABLE == 1)

	//#define __TEST__
	//#define __TEST_DATA__  //Test data flow
	//#define __TEST_BEACON__

#else
	#define __STA__

	//#define __TCPIP_TEST__
#endif


//#define __AP_DEBUG__

// ------------------------- AMPDU ---------------------------
#define AMPDU_TX_ENABLE      0
#define AMPDU_RX_ENABLE      1
#define AMPDU_RX_BUF_SIZE    64

#if AMPDU_TX_ENABLE
#define AMPDU_TX_MAINTRY     1 // base rate retry times
#define AMPDU_TX_MAXRETRY    3 // ampdu max retry times

#define AMPDU_TX_LASTRETRYB  1 // 1-retryb enable, 0-retryb disable
#define AMPDU_TX_LASTBRATE   2 // 0-"1M", 1-"2M", 2-"5.5M", 3-"11M";
#define AMPDU_TX_LASTBONCE   0

#define IW_SRC_TX            4
#else
#define AMPDU_TX_MAINTRY     3 // base rate retry times
#define AMPDU_TX_MAXRETRY    7 // ampdu max retry times

#define AMPDU_TX_LASTRETRYB  1 // 1-retryb enable, 0-retryb disable
#define AMPDU_TX_LASTBRATE   2 // 0-"1M", 1-"2M", 2-"5.5M", 3-"11M";
#define AMPDU_TX_LASTBONCE   1

#define IW_SRC_TX            7
#endif

// ------------------------- rate control ---------------------------
#if AMPDU_TX_ENABLE
#define RC_DEFAULT_RATE_MSK 0x01F0
#define RC_DEFAULT_RESENT_REPORT 1  // feature, 0 or 1
#define RC_DEFAULT_UP_PF 20         // percentage, smaller than RC_DEFAULT_DOWN_PF
#define RC_DEFAULT_DOWN_PF 40       // percentage, bigger than RC_DEFAULT_UP_PF
#define RC_DEFAULT_PER_FASTESTB 7   // percentage
#define RC_DEFAULT_PREPRBFRM 0      // feature, 0 or 1
#define RC_DIRECT_DOWN TRUE
#else
#define RC_DEFAULT_RATE_MSK 0x0FFF
#define RC_DEFAULT_RESENT_REPORT 1  // feature, 0 or 1
#define RC_DEFAULT_UP_PF 10         // percentage, smaller than RC_DEFAULT_DOWN_PF
#define RC_DEFAULT_DOWN_PF 27       // percentage, bigger than RC_DEFAULT_UP_PF
#define RC_DEFAULT_PER_FASTESTB 7   // percentage
#define RC_DEFAULT_PREPRBFRM 0      // feature, 0 or 1
#define RC_DIRECT_DOWN FALSE
#endif

#define RC_DEFAULT_DRATE_ENDIAN 0 //1: The default data rate is from the lowest rate of rate mask. 0:from highest index of rate mask

//==============EDCA===============
//#define EDCA_PATTERN_TEST
#ifdef EDCA_PATTERN_TEST
#define EDCA_DBG						1		//Enable to test edca function
#define MACTXRX_CONTROL_ENABLE			1		//To simulate MAC TX operation. It's also enable ASIC queue empty interrupt.
#define MACTXRX_CONTROL_DURATION_SIM	1		//TX control use softmain edca handler to test MAC TX EDCA function
//#define __EDCA_INT_TEST__						//
//#define __EDCA_NOTIFY_HOST__					//When TX done send an event to nofity host to know
#define BEACON_DBG						1
#else
#define EDCA_DBG						0		//Enable to test edca function
#define MACTXRX_CONTROL_ENABLE			0		//To simulate MAC TX operation. It's also enable ASIC queue empty interrupt.
#define MACTXRX_CONTROL_DURATION_SIM	0		//TX control use softmain edca handler to test MAC TX EDCA function
//#define __EDCA_INT_TEST__						//
//#define __EDCA_NOTIFY_HOST__					//When TX done send an event to nofity host to know
#define BEACON_DBG						0
#endif


//=================================

//#define PACKED

/* default ip */
#define DEFAULT_IPADDR   "192.168.25.1"
#define DEFAULT_SUBNET   "255.255.255.0"
#define DEFAULT_GATEWAY  "192.168.25.1"
#define DEFAULT_DNS      "192.168.25.1"

/* default dhcp server info */
#define DEFAULT_DHCP_START_IP    "192.168.25.101"
#define DEFAULT_DHCP_END_IP      "192.168.25.110"
#define DEFAULT_DHCP_MAX_LEASES  10

#define DEFAULT_DHCP_AUTO_TIME       (7200)
#define DEFAULT_DHCP_DECLINE_TIME    (3600)
#define DEFAULT_DHCP_CONFLICT_TIME   (3600)
#define DEFAULT_DHCP_OFFER_TIME      (60)
#define DEFAULT_DHCP_MIN_LEASE_SEC   (60)
#define DEFAULT_DHCP_MAX_LEASE_SEC   (60*60*24 * 10)


/* watchdog */
#if(CONFIG_CHIP_ID==SSV6006)
#define RECOVER_ENABLE                  0
#else
#define RECOVER_ENABLE                  0
#endif
#ifndef __SSV_UNIX_SIM__
#define RECOVER_MECHANISM               1 //0:Used GPIO interrupt, 1:Used Timer interrupt
#else
#define RECOVER_MECHANISM               0 //0:Used GPIO interrupt, 1:Used Timer interrupt
#endif
#define IPC_CHECK_TIMER                 1000 //For IPC interrupt

// ------------------------- Power Saving ---------------------------
#define BUS_IDLE_TIME 0 //5000 //mswpw

/* netmgr auto retry default times and delay value */
//#define NET_MGR_AUTO_RETRY_TIMES 3	//0xFFFF
//#define NET_MGR_AUTO_RETRY_DELAY 5	// unit s
#define NET_MGR_AUTO_RETRY_TIMES -1	//0xFFFF  -1: disable wifi driver auto retry connect
#define NET_MGR_AUTO_RETRY_DELAY 1	// unit s

//------------------------wpa2--------------------
//#define CONFIG_NO_WPA2

//------------------------AP mode--------------------
#if(CONFIG_CHIP_ID==SSV6006)
#define AUTO_BEACON 1
#else
#define AUTO_BEACON 0
#endif
#define AP_BEACON_INT	(50)
#define AP_ERP                   0
#define AP_B_SHORT_PREAMBLE      0
#define AP_RX_SHORT_GI           0
#define AP_RX_SUPPORT_BASIC_RATE     0xFFF
#define AP_RX_SUPPORT_MCS_RATE       0xFF

// ------------------------- Update features depends on TCPIP stack ---------------------------
/* TCP/IP Configuration: */
#ifndef USE_ICOMM_LWIP
#define USE_ICOMM_LWIP          1
#endif
/*
    1: LWIP ignore the pcb->cwnd when LWIP output tcp packet
    0: LWIP refer the pcb->cwnd when LWIP output tcp packet
*/
#define LWIP_TCP_IGNORE_CWND     0
/* 0 for minimal resources, 1 for double resource of setting 0 with higher performance, 2 for default, 3 for maximal resources */
/* RAM Usage:
   0: 46KB
   1: 77KB
   2: 99KB
   3: 177KB
*/
#define LWIP_PARAM_SET                  0
#if USE_ICOMM_LWIP
#define HTTPD_SUPPORT 0
#define DHCPD_SUPPORT 0
#define CONFIG_USE_LWIP_PBUF    1
#define CONFIG_MEMP_DEBUG       0
#define PING_SUPPORT 0
#define IPERF3_ENABLE  1
#define IPERF3_UDP_TEST 0
#else //#if USE_ICOMM_LWIP
#define PING_SUPPORT 0
#undef IPERF3_ENABLE
#define IPERF3_ENABLE  0
#define IPERF3_UDP_TEST 0
#undef  CONFIG_USE_LWIP_PBUF
#define CONFIG_USE_LWIP_PBUF    0
#undef	CONFIG_USE_ZEPHRY_NBUF
#define CONFIG_USE_ZEPHRY_NBUF	1
#undef  CONFIG_MEMP_DEBUG
#define CONFIG_MEMP_DEBUG       0
#define HTTPD_SUPPORT           0
#define DHCPD_SUPPORT           0

#endif //#if USE_ICOMM_LWIP

#if CONFIG_USE_LWIP_PBUF
#define POOL_SIZE PBUF_POOL_SIZE
#define POOL_SEC_SIZE PBUF_POOL_SEC_SIZE
#define RECV_BUF_SIZE MAX_RECV_BUF
#define TRX_HDR_LEN DRV_TRX_HDR_LEN

#else
#define RECV_BUF_SIZE           1516
#ifndef CONFIG_USE_ZEPHRY_NBUF
#define POOL_SIZE               16 //10
#define POOL_SEC_SIZE           2
#else
#define POOL_SIZE               16 //10
#define POOL_SEC_SIZE           2
#endif
#define TRX_HDR_LEN             40
#endif

//------------------------Net APP--------------------
//Disable NETAPP, and we need to manually set the netapp stack size to zero in porting.h
#define NETAPP_SUPPORT 0

//------------------------8023 <-> 80211--------------------
#define SW_8023TO80211 0

#if(CONFIG_CHIP_ID!=SSV6006)
#undef SW_8023TO80211
#define SW_8023TO80211 0
#endif

//Set a timeout periord for STA auto disconnect when AP power off or change channel
//The unit is 500ms, the default value is 40, it means STA will disconnect after AP power off for 20s
#define STA_NO_BCN_TIMEOUT 40


/*
In AP mode, DUT kick the STA by traffic, it is the original design.
However, DUT can only accept two STAs, it causes the target STA connect to DUT difficutly.
So, there is a new flow here, L2 give APP layer has the power to make a decision which STA could be ticked
"1": Kick STA BY APP
"0": Kick STA only by traffic
*/

#define APP_LEAD_KICK_STA 0

/*
After APP_KICK_TIME_OUT, the third STA can jameout the old STA if APP_LEAD_KICK_STA=1
*/
#define APP_KICK_TIME_OUT 10 //unit is s

//External GPIO
#define RXINTGPIO      8 

#endif /* _SIM_CONFIG_H_ */
