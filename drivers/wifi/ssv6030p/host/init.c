/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <os_wrapper.h>
#include <log.h>
#include <host_apis.h>
#include <cli/cmds/cli_cmd_wifi.h>
#if HTTPD_SUPPORT
#include <httpserver_raw/httpd.h>
#endif
#if(ENABLE_SMART_CONFIG==1)
#include <SmartConfig/SmartConfig.h>
#endif
#include <net_mgr.h>
#include <netstack.h>
#include <dev.h>
#include <ssv_drv.h>

#if SSV_LOG_DEBUG
u32 g_log_module;
u32 g_log_min_level;
#endif
// ----------------------------------------------------
//Configurations
// ----------------------------------------------------
//Mac address
u8 config_mac[] ={0x60,0x11,0x22,0x33,0x44,0x55};

//Max number of AP list
u8 g_max_num_of_ap_list=NUM_AP_INFO;

//Auto channel selection in AP mode
u16 g_acs_channel_mask=ACS_CHANNEL_MASK;
u16 g_acs_channel_scanning_interval=ACS_SCANNING_INTERVAL;
u8  g_acs_channel_scanning_loop=ACS_SCANNING_LOOP;

//Default channel mask in sta and smart config mode
u16 g_sta_channel_mask = DEFAULT_STA_CHANNEL_MASK;

// ------------------------- rate control ---------------------------
struct Host_cfg g_host_cfg;
// ------------- User mode SmartConfig ...............................................
#if(ENABLE_SMART_CONFIG==1)
SSV6XXX_USER_SCONFIG_OP ssv6xxx_user_sconfig_op;
u32 g_sconfig_solution=SMART_CONFIG_SOLUTION;
u8 g_sconfig_auto_join=SMART_CONFIG_AUTO_JOIN;
#endif
// ----------------------------------------------------
u8 g_lwip_tcp_ignore_cwnd=LWIP_TCP_IGNORE_CWND;

#ifdef NET_MGR_AUTO_RETRY
int  g_auto_retry_times_delay = NET_MGR_AUTO_RETRY_DELAY;
int  g_auto_retry_times_max = NET_MGR_AUTO_RETRY_TIMES;
#endif

#if DHCPD_SUPPORT
extern int udhcpd_init(void);
#endif

#if 0
struct ap_info_st sg_ap_info[20];
#endif // 0

#if (AMPDU_RX_ENABLE == 1)
unsigned int record_buf[RX_AGG_RX_BA_MAX_STATION][RX_AGG_RX_BA_MAX_SESSIONS][AMPDU_RX_BUF_SIZE];
unsigned long reorder_time[RX_AGG_RX_BA_MAX_STATION][RX_AGG_RX_BA_MAX_SESSIONS][AMPDU_RX_BUF_SIZE];
#endif

char *g_record_buf = NULL;
char *g_reorder_time = NULL;

void stop_and_halt (void)
{
    //while (1) {}
    /*lint -restore */
} // end of - stop_and_halt -

int ssv6xxx_add_interface(ssv6xxx_hw_mode hw_mode)
{
    //0:false 1:true
    return 1;

}

//=====================Task parameter setting========================
extern struct task_info_st g_txrx_task_info[];
extern struct task_info_st g_host_task_info[];
extern struct task_info_st g_timer_task_info[];
#if !NET_MGR_NO_SYS
extern struct task_info_st st_netmgr_task[];
#endif
#if DHCPD_SUPPORT
extern struct task_info_st st_dhcpd_task[];
#endif
#if (MLME_TASK==1)
extern struct task_info_st g_mlme_task_info[];
#endif

void ssv6xxx_init_task_para(void)
{
    g_txrx_task_info[0].prio = OS_TX_TASK_PRIO;
    g_txrx_task_info[1].prio = OS_RX_TASK_PRIO;
    g_host_task_info[0].prio = OS_CMD_ENG_PRIO;
    g_timer_task_info[0].prio = OS_TMR_TASK_PRIO;
#if !NET_MGR_NO_SYS
    st_netmgr_task[0].prio = OS_NETMGR_TASK_PRIO;
#endif
#if DHCPD_SUPPORT    
    st_dhcpd_task[0].prio = OS_DHCPD_TASK_PRIO;
#endif
#if (MLME_TASK==1)
    g_mlme_task_info[0].prio = OS_MLME_TASK_PRIO;
#endif
}
//=============================================================

int ssv6xxx_start()
{
    u32 wifi_mode;
    ssv6xxx_drv_start();

    /* Reset MAC & Re-Init */

    /* Initialize ssv6200 mac */
    if(-1==ssv6xxx_init_mac())
    {
    	return SSV6XXX_FAILED;
    }

    //Set ap or station register
    wifi_mode = ssv6xxx_wifi_get_mode();
    if ((SSV6XXX_HWM_STA == wifi_mode)||(SSV6XXX_HWM_SCONFIG == wifi_mode))
    {
        if(-1==ssv6xxx_init_sta_mac(wifi_mode))
        {
            return SSV6XXX_FAILED;
        }
    }
    else
    {
        if(-1==ssv6xxx_init_ap_mac())
        {
            return SSV6XXX_FAILED;
        }
    }

    return SSV6XXX_SUCCESS;
}

#define SSV_START_DEMO

#ifdef SSV_START_DEMO
#define JOIN_DEFAULT_SSID    "Default_ap" //"china"
#define JOIN_DEFAULT_PSK     "12345678" //"12345678"

#define AP_DEFAULT_SSID    "ssv-6030-AP"
#define AP_DEFAULT_PSK     "12345678"


extern ssv6xxx_result STAmode_default(bool bJoin);

int STAmode_join_fast(void *data, int len)
{
    int ret = 0;
    u32 start_tick = 0;
    bool timeout = false;
    struct cfg_join_request *join_req = data;

    if (!data || (len != sizeof(struct cfg_join_request)))
    {
        LOG_PRINTF("STAmode_join_fast len err\r\n");
        return -1;
    }

    LOG_PRINTF("STAmode_join_fast issta(%d)\r\n", netmgr_wifi_check_sta());
    if (!netmgr_wifi_check_sta())
    {
        ret = STAmode_default(false);
    }

    if (ret != 0)
    {
        LOG_PRINTF("STAmode_default error %d\r\n", ret);
        return SSV6XXX_FAILED;
    }

    ret = netmgr_wifi_join_fast_async(join_req);
    if (ret != 0)
    {
        LOG_PRINTF("STAmode_join_fast error %d\r\n", ret);
        return SSV6XXX_FAILED;
    }

    start_tick = OS_GetSysTick();
    timeout = false;
    while (!netmgr_wifi_check_sta_connected())
    {
        // wait 3 second timeout
        if (((OS_GetSysTick() - start_tick) * OS_TICK_RATE_MS) >= 3000)
        {
            timeout = true;
            break;
        }
        OS_TickDelay(1);
    }

    if (timeout)
    {
        LOG_PRINTF("STAmode_join_fast timeout\r\n");
        return SSV6XXX_FAILED;
    }

    LOG_PRINTF("STAmode_join_fast success\r\n");
    return SSV6XXX_SUCCESS;
}

ssv6xxx_result STAmode_default(bool bJoin)
{
    int ret = SSV6XXX_SUCCESS;
    wifi_mode mode;
    wifi_sta_join_cfg join_cfg;
    u32 start_tick = 0;
    bool timeout = false;

    MEMSET((void * )&join_cfg, 0, sizeof(join_cfg));
    mode = SSV6XXX_HWM_STA;
    if(bJoin)
    {
        MEMCPY((void *)join_cfg.ssid.ssid,JOIN_DEFAULT_SSID,STRLEN(JOIN_DEFAULT_SSID));
        join_cfg.ssid.ssid_len=STRLEN(JOIN_DEFAULT_SSID);
        STRCPY((void *)join_cfg.password, JOIN_DEFAULT_PSK);
        if (netmgr_wifi_check_sta())
        {
            ret = netmgr_wifi_join_other_async(&join_cfg);
        }
        else
        {
            ret = netmgr_wifi_switch_async(mode, NULL, &join_cfg);
        }
    }
    else
    {
        Sta_setting sta;
        MEMSET(&sta, 0 , sizeof(Sta_setting));

        sta.status = TRUE;
        ret = netmgr_wifi_control_async(mode, NULL, &sta);
    }
    if (ret != 0)
    {
        LOG_PRINTF("STA mode error (%d)\r\n", bJoin);
        return SSV6XXX_FAILED;
    }
    
    start_tick = OS_GetSysTick();
    timeout = false;
    while (!netmgr_wifi_check_sta())
    {
        // wait 3 second timeout
        if (((OS_GetSysTick() - start_tick) * OS_TICK_RATE_MS) >= 3000)
        {
            timeout = true;
            break;
        }
        OS_TickDelay(1);
    }
    
    if (timeout)
    {
        LOG_PRINTF("STA mode timeout\r\n");
        return SSV6XXX_FAILED;
    }

    LOG_PRINTF("STA mode success\r\n");
    return SSV6XXX_SUCCESS;
}

#if (AP_MODE_ENABLE == 1)
ssv6xxx_result APmode_default()
{
    wifi_mode mode;
    ssv_wifi_ap_cfg ap_cfg;
    int ret = 0;
    u32 start_tick = 0;
    bool timeout = false;
    
    MEMSET((void * )&ap_cfg, 0, sizeof(ap_cfg));
    mode = SSV6XXX_HWM_AP;
    ap_cfg.status = TRUE;
    MEMCPY((void *)ap_cfg.ssid.ssid,AP_DEFAULT_SSID,STRLEN(AP_DEFAULT_SSID));
    ap_cfg.ssid.ssid_len = STRLEN(AP_DEFAULT_SSID);
    STRCPY((void *)(ap_cfg.password),AP_DEFAULT_PSK);
    ap_cfg.security =   SSV6XXX_SEC_WPA2_PSK;
    ap_cfg.proto = WPA_PROTO_RSN;
    ap_cfg.key_mgmt = WPA_KEY_MGMT_PSK ;
    ap_cfg.group_cipher=WPA_CIPHER_CCMP;
    ap_cfg.pairwise_cipher = WPA_CIPHER_CCMP;
    ap_cfg.channel = EN_CHANNEL_AUTO_SELECT;

    ret = netmgr_wifi_control_async(mode , &ap_cfg, NULL);
    if (ret != 0)
    {
        LOG_PRINTF("AP mode error\r\n");
        return SSV6XXX_FAILED;
    }
    
    start_tick = OS_GetSysTick();
    timeout = false;
    while (!netmgr_wifi_check_ap())
    {
        // wait 3 second timeout
        if (((OS_GetSysTick() - start_tick) * OS_TICK_RATE_MS) >= 3000)
        {
            timeout = true;
            break;
        }
        OS_TickDelay(1);
    }
    
    if (timeout)
    {
        LOG_PRINTF("AP mode timeout\r\n");
        return SSV6XXX_FAILED;
    }
    
    LOG_PRINTF("AP mode success\r\n");
    return SSV6XXX_SUCCESS;
}
#endif

#if(ENABLE_SMART_CONFIG==1)

extern u16 g_SconfigChannelMask;

ssv6xxx_result SCONFIGmode_default()
{
    wifi_mode mode;
    Sta_setting sta;
    int ret = 0;
    u32 start_tick = 0;
    bool timeout = false;
    
    mode = SSV6XXX_HWM_SCONFIG;
    MEMSET(&sta, 0 , sizeof(Sta_setting));
    
    sta.status = TRUE;
    ret = netmgr_wifi_control_async(mode, NULL, &sta);
    if (ret != 0)
    {
        LOG_PRINTF("SCONFIG mode error\r\n");
    }

    start_tick = OS_GetSysTick();
    timeout = false;
    while (!netmgr_wifi_check_sconfig())
    {
        // wait 3 second timeout
        if (((OS_GetSysTick() - start_tick) * OS_TICK_RATE_MS) >= 3000)
        {
            timeout = true;
            break;
        }
        OS_TickDelay(1);
    }
    
    if (timeout)
    {
        LOG_PRINTF("SCONFIG mode timeout\r\n");
        return SSV6XXX_FAILED;
    }
    
    g_SconfigChannelMask=0x3FFE; //This variable is only for RD use, customers don't need to modify it.
    g_sconfig_solution=SMART_CONFIG_SOLUTION;
    ret = netmgr_wifi_sconfig_async(g_SconfigChannelMask);
    if (ret != 0)
    {
        LOG_PRINTF("SCONFIG async error\r\n");
        return SSV6XXX_FAILED;
    }
    
    LOG_PRINTF("SCONFIG mode success\r\n");
    return SSV6XXX_SUCCESS;
}
#endif

ssv6xxx_result wifi_start(wifi_mode mode, bool static_ip, bool dhcp_server)
{

    ssv6xxx_result res = SSV6XXX_SUCCESS;
    /* station mode */
    if (mode == SSV6XXX_HWM_STA)
    {
        #if USE_ICOMM_LWIP
        if (static_ip)
        {
            netmgr_dhcpc_set(false);
            {
                ipinfo info;

                info.ipv4 = ipaddr_addr("192.168.100.100");
                info.netmask = ipaddr_addr("255.255.255.0");
                info.gateway = ipaddr_addr("192.168.100.1");
                info.dns = ipaddr_addr("192.168.100.1");

                netmgr_ipinfo_set(WLAN_IFNAME, &info, false);
            }
        }
        else
        {
            netmgr_dhcpc_set(true);
        }
        #endif

        res = STAmode_default(false); //false: don't auto join AP_DEFAULT_SSDI
    }

    #if (AP_MODE_ENABLE == 1)
    /* ap mode */
    if (mode == SSV6XXX_HWM_AP)
    {
        #if USE_ICOMM_LWIP
        if (static_ip)
        {
            ipinfo info;

            info.ipv4 = ipaddr_addr("172.16.10.1");
            info.netmask = ipaddr_addr("255.255.255.0");
            info.gateway = ipaddr_addr("172.16.10.1");
            info.dns = ipaddr_addr("172.16.10.1");

            netmgr_ipinfo_set(WLAN_IFNAME,&info, true);
        }

        if (dhcp_server)
        {
            netmgr_dhcpd_set(true);
        }
        else
        {
            netmgr_dhcpd_set(false);
        }
        #endif

        res = APmode_default();
    }
    #endif
    #if(ENABLE_SMART_CONFIG==1)
    /* smart link mode */
    if (mode == SSV6XXX_HWM_SCONFIG)
    {
        res = SCONFIGmode_default();
    }
    #endif

    LOG_PRINTF("wifi start \r\n");
    return res;
}
#endif

/**
 *  Entry point of the firmware code. After system booting, this is the
 *  first function to be called from boot code. This function need to
 *  initialize the chip register, software protoctol, RTOS and create
 *  tasks. Note that, all memory resources needed for each software
 *  modulle shall be pre-allocated in initialization time.
 */
/* return int to avoid from compiler warning */

#if (AP_MODE_ENABLE == 1)
extern s32 AP_Init(u32 max_sta_num);
#endif
extern void ssv_netmgr_init_netdev(bool default_cfg);
int ssv6xxx_dev_init(ssv6xxx_hw_mode hmode)
{
    ssv6xxx_result res;

#ifndef __SSV_UNIX_SIM__
    platform_ldo_en_pin_init();
    //ldo_en 0 -> 1 equal to HW reset.
    platform_ldo_en(0);
    OS_MsDelay(10);
    platform_ldo_en(1);
#endif

#if SSV_LOG_DEBUG
	g_log_module = CONFIG_LOG_MODULE;
	g_log_min_level = CONFIG_LOG_LEVEL;
#endif    


#if(ENABLE_SMART_CONFIG==1)
    //This function must be assigned before netmgr_init
    #if(SMART_CONFIG_SOLUTION==CUSTOMER_SOLUTION)
    // Register customer operation function
    // ssv6xxx_user_sconfig_op.UserSconfigInit= xxx ;
    // ssv6xxx_user_sconfig_op.UserSconfigPaserData= xxx ;
    // ssv6xxx_user_sconfig_op.UserSconfigSM= xxx;
    // ssv6xxx_user_sconfig_op.UserSconfigConnect= xxx;
    // ssv6xxx_user_sconfig_op.UserSconfigDeinit= xxx;
    #else
    ssv6xxx_user_sconfig_op.UserSconfigInit=SmartConfigInit;
    ssv6xxx_user_sconfig_op.UserSconfigPaserData=SmartConfigPaserData;
    ssv6xxx_user_sconfig_op.UserSconfigSM=SmartConfigSM;
    ssv6xxx_user_sconfig_op.UserSconfigConnect=SmartConfigConnect;
    ssv6xxx_user_sconfig_op.UserSconfigDeinit=SmartConfigDeinit;
    #endif
#endif    
    //host config default value
    OS_MemSET((void*)&g_host_cfg,0,sizeof(g_host_cfg));
    g_host_cfg.rate_mask= RC_DEFAULT_RATE_MSK;
    g_host_cfg.resent_fail_report= RC_DEFAULT_RESENT_REPORT;
    g_host_cfg.upgrade_per= RC_DEFAULT_UP_PF;
    g_host_cfg.downgrade_per= RC_DEFAULT_DOWN_PF;
    g_host_cfg.pre_alloc_prb_frm= RC_DEFAULT_PREPRBFRM;
    g_host_cfg.upper_fastestb= RC_DEFAULT_PER_FASTESTB;
    g_host_cfg.direct_rc_down= RC_DIRECT_DOWN;
    g_host_cfg.rc_drate_endian=RC_DEFAULT_DRATE_ENDIAN;
    g_host_cfg.tx_power_mode = CONFIG_TX_PWR_MODE;
    g_host_cfg.pool_size = POOL_SIZE;
    g_host_cfg.pool_sec_size = POOL_SEC_SIZE;
    g_host_cfg.recv_buf_size = RECV_BUF_SIZE;
    g_host_cfg.bcn_interval = AP_BEACON_INT;
    g_host_cfg.trx_hdr_len = TRX_HDR_LEN;
    g_host_cfg.erp = AP_ERP;
    g_host_cfg.b_short_preamble= AP_B_SHORT_PREAMBLE;
    g_host_cfg.tx_sleep = TX_TASK_SLEEP;
    g_host_cfg.tx_sleep_tick = TX_TASK_SLEEP_TICK;    
    g_host_cfg.tx_retry_cnt= TX_TASK_RETRY_CNT;    
    g_host_cfg.tx_res_page = TX_RESOURCE_PAGE;
    g_host_cfg.rx_res_page = RX_RESOURCE_PAGE;
    g_host_cfg.ap_rx_short_GI = AP_RX_SHORT_GI;
    g_host_cfg.ap_rx_support_legacy_rate_msk = AP_RX_SUPPORT_BASIC_RATE;
    g_host_cfg.ap_rx_support_mcs_rate_msk = AP_RX_SUPPORT_MCS_RATE;
    g_host_cfg.txduty.mode = TXDUTY_MODE;
    g_host_cfg.volt_mode = SSV_VOLT_REGULATOR;
    g_host_cfg.ampdu_rx_buf_size = AMPDU_RX_BUF_SIZE;
    g_host_cfg.support_ht = PHY_SUPPORT_HT;
    if(g_host_cfg.support_ht)
    {
        g_host_cfg.ampdu_rx_enable = AMPDU_RX_ENABLE;
        g_host_cfg.ampdu_tx_enable = AMPDU_TX_ENABLE;
    }
    else
    {
        g_host_cfg.ampdu_rx_enable= 0;
        g_host_cfg.ampdu_tx_enable= 0;
    }

    g_host_cfg.ampdu_tx_maintry    = AMPDU_TX_MAINTRY;
    g_host_cfg.ampdu_tx_maxretry   = AMPDU_TX_MAXRETRY;
    g_host_cfg.ampdu_tx_lastretryb = AMPDU_TX_LASTRETRYB;
    g_host_cfg.ampdu_tx_lastbrate  = AMPDU_TX_LASTBRATE;
    g_host_cfg.ampdu_tx_lastbonce  = AMPDU_TX_LASTBONCE;
    
    g_host_cfg.iw_src_tx_qidx1 =  IW_SRC_TX;
    g_host_cfg.iw_src_tx_qidx2 =  IW_SRC_TX;
    
    g_host_cfg.sta_no_bcn_timeout = STA_NO_BCN_TIMEOUT;
    g_host_cfg.app_lead_kick_sta=APP_LEAD_KICK_STA;
    g_host_cfg.app_kick_timeout=APP_KICK_TIME_OUT;

    g_host_cfg.rxIntGPIO   = RXINTGPIO;
    if(strcmp(INTERFACE, "spi") == 0)
    {
        g_host_cfg.extRxInt = 1;
    }
    else
    {
#ifdef EXT_RX_INT
        g_host_cfg.extRxInt = 1;
#else
        g_host_cfg.extRxInt = 0;
#endif
    }    
    if (g_host_cfg.ampdu_rx_enable)
    {
#if (AMPDU_RX_ENABLE == 1)
        g_record_buf = (unsigned char *)&(record_buf[0][0][0]);
        g_reorder_time = (unsigned char *)&(reorder_time[0][0][0]);
#endif
    }
    else
    {
        g_record_buf = NULL;
        g_reorder_time = NULL;
    }

	printk("Wifi %s interrupt, volt %s mode\n", (g_host_cfg.extRxInt == 1)? "External rx" : "Sdio inner", \
			(g_host_cfg.volt_mode == VOLT_LDO_REGULATOR)? "ldo" : "dcdc");
    LOG_PRINTF("g_host_cfg.ampdu_tx_enable = %d, g_host_cfg.ampdu_rx_enable = %d\r\n", g_host_cfg.ampdu_tx_enable, g_host_cfg.ampdu_rx_enable);
    
	/**
        * On simulation/emulation platform, initialize RTOS (simulation OS)
        * first. We use this simulation RTOS to create the whole simulation
        * and emulation platform.
        */
    ASSERT( OS_Init() == OS_SUCCESS );
    host_global_init();
    ssv6xxx_init_task_para();

	LOG_init(true, true, LOG_LEVEL_ON, LOG_MODULE_MASK(LOG_MODULE_EMPTY), false);
#ifdef __SSV_UNIX_SIM__
	LOG_out_dst_open(LOG_OUT_HOST_TERM, NULL);
	LOG_out_dst_turn_on(LOG_OUT_HOST_TERM);
#endif

    ASSERT( msg_evt_init(g_host_cfg.pool_size+g_host_cfg.pool_sec_size+SSV_TMR_MAX) == OS_SUCCESS );

#if (CONFIG_USE_LWIP_PBUF==0)
    ASSERT( PBUF_Init(POOL_SIZE) == OS_SUCCESS );
#endif//#if CONFIG_USE_LWIP_PBUF

#if (BUS_TEST_MODE == 0)
    netstack_init(NULL);
#endif
    /**
        * Initialize Host simulation platform. The Host initialization sequence
        * shall be the same as the sequence on the real host platform.
        *   @ Initialize host device drivers (SDIO/SIM/UART/SPI ...)
        */
    {

        ASSERT(ssv6xxx_drv_module_init() == true);
        LOG_PRINTF("Try to connecting CABRIO via %s...\n\r",INTERFACE);
        if (ssv6xxx_drv_select(INTERFACE) == false)
        {

            {
            LOG_PRINTF("==============================\n\r");
			LOG_PRINTF("Please Insert %s wifi device\n",INTERFACE);
			LOG_PRINTF("==============================\n\r");
        	}
			return -1;
        }
		#if (BUS_TEST_MODE == 0)
        ASSERT(ssv_hal_init() == true);

        if(ssv6xxx_wifi_init()!=SSV6XXX_SUCCESS)
			return -1;

        {
            os_timer_init();
        }
		#endif
    }
    ssv6xxx_platform_check();

    
    if(1){
        u8 sar_code=0;
        s8 temperature=TMPT_HIGH;
        ssv_hal_get_temperature(&sar_code,&temperature);
        g_host_cfg.high_tmpt = sar_code;
        temperature=TMPT_LOW;
        ssv_hal_get_temperature(&sar_code,&temperature);
        g_host_cfg.low_tmpt = sar_code;
    }
#if (BUS_TEST_MODE == 0)

    
#if (AP_MODE_ENABLE == 1)
    AP_Init(WLAN_MAX_STA);
#endif
    /**
    * Initialize TCP/IP Protocol Stack. If tcpip is init, the net_app_ask()
    * also need to be init.
    */

    
#if DHCPD_SUPPORT
    if(udhcpd_init() != 0)
    {
        LOG_PRINTF("udhcpd_init fail\r\n");
        return SSV6XXX_FAILED;
    }
#endif

#if HTTPD_SUPPORT
    httpd_init();
#endif

#if NETAPP_SUPPORT
    if(net_app_init()== 1)
    {
        LOG_PRINTF("net_app_init fail\n\r");
        return SSV6XXX_FAILED;
    }
#endif
    /* netmgr int */
    ssv_netmgr_init(true);

    switch (hmode)
    {
        case SSV6XXX_HWM_STA: //AUTO_INIT_STATION	    
        {
            res = wifi_start(SSV6XXX_HWM_STA,false,false);
            break;
        }
        case SSV6XXX_HWM_AP:
        {
#if (AP_MODE_ENABLE == 1)
           res = wifi_start(SSV6XXX_HWM_AP,false,true);
#else
           res = SSV6XXX_FAILED;
#endif
           break;
        }
        case SSV6XXX_HWM_SCONFIG:
        {

#if(ENABLE_SMART_CONFIG==1)
            res = wifi_start(SSV6XXX_HWM_SCONFIG,false,true);
#else
             res = SSV6XXX_FAILED;
#endif

            break;
        }
        default:
        {
            res = wifi_start(SSV6XXX_HWM_STA,false,false);
        }
    }
#else
    res = SSV6XXX_SUCCESS;
#endif
    if (1)
    {        
#if 0
        /* we can set default ip address and default dhcpd server config before netmgr netdev init */
        netmgr_cfg cfg;
        cfg.ipaddr = ipaddr_addr("192.168.100.1");
        cfg.netmask = ipaddr_addr("255.255.255.0");
        cfg.gw = ipaddr_addr("192.168.100.1");
        cfg.dns = ipaddr_addr("0.0.0.0");
        
        cfg.dhcps.start_ip = ipaddr_addr("192.168.100.101");
        cfg.dhcps.end_ip = ipaddr_addr("192.168.100.110");
        cfg.dhcps.max_leases = 10;
        cfg.dhcps.subnet = ipaddr_addr("255.255.255.0");
        cfg.dhcps.gw = ipaddr_addr("192.168.100.1");       
        cfg.dhcps.dns = ipaddr_addr("192.168.100.1");
        cfg.dhcps.auto_time = DEFAULT_DHCP_AUTO_TIME;
        cfg.dhcps.decline_time = DEFAULT_DHCP_DECLINE_TIME;
        cfg.dhcps.conflict_time = DEFAULT_DHCP_CONFLICT_TIME;
        cfg.dhcps.offer_time = DEFAULT_DHCP_OFFER_TIME;
        cfg.dhcps.max_lease_sec = DEFAULT_DHCP_MAX_LEASE_SEC;
        cfg.dhcps.min_lease_sec = DEFAULT_DHCP_MIN_LEASE_SEC;      
        netmgr_cfg_set(&cfg);
        ssv_netmgr_init_netdev(false);
#else
        /* we can set default ip address and default dhcpd server config */
        ssv_netmgr_init_netdev(true);
#endif
    }
#if (CLI_ENABLE==1)
#if (CLI_TASK_ENABLE==1)
    Cli_Init(0, NULL);
#endif
#if (BUS_TEST_MODE == 0)
    ssv6xxx_wifi_cfg();
#endif
#endif

#if (BUS_TEST_MODE == 0)
#if(MLME_TASK ==1)
    mlme_init();    //MLME task initial
#endif
#endif
    return res;
}


//#define SSV_IGMP_DEMO // multicast udp demo

#ifdef SSV_IGMP_DEMO

struct udp_pcb * g_upcb = NULL;
struct ip_addr ipMultiCast;
#define LISTEN_PORT       52000
#define LISTEN_ADDR       0xef0101ff   // 239.1.1.255
#define DST_PORT          52001

void ssv_test_print(const char *title, const u8 *buf,
                             size_t len)
{
    size_t i;
    LOG_PRINTF("%s - hexdump(len=%d):\r\n    ", title, len);
    if (buf == NULL) {
        LOG_PRINTF(" [NULL]");
    }else{
        for (i = 0; i < 16; i++)
            LOG_PRINTF("%02X ", i);

        LOG_PRINTF("\r\n---\r\n00|");
       for (i = 0; i < len; i++){
            LOG_PRINTF(" %02x", buf[i]);
            if((i+1)%16 ==0)
                LOG_PRINTF("\r\n%02x|", (i+1));
        }
    }
    LOG_PRINTF("\r\n-----------------------------\r\n");
}

int ssv_test_udp_tx(u8_t *data, u16_t len, u16_t port)
{
    int ret = -2;

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_POOL);

    memcpy(p->payload, data, len);

    ret = udp_sendto(g_upcb, p,(struct ip_addr *) (&ipMultiCast), port);

    pbuf_free(p);

    return ret;
}

void ssv_test_udp_rx(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
    unsigned char g_tData[256];
    int ret = 0;

    //LOG_PRINTF("addr:0x%x port:%d len:%d\r\n", addr->addr, port, p->tot_len);

    //ssv_test_print("ssv_test_udp_rx", OS_FRAME_GET_DATA(p), OS_FRAME_GET_DATA_LEN(p));

    ret = udp_connect(upcb, addr, port);            /* connect to the remote host */
    if (ret != 0)
    {
        LOG_PRINTF("udp_connect: ret = %d\r\n", ret);
    }

    if (p->len >= 256) p->len = 128;

    memcpy(g_tData, p->payload, p->len);
    g_tData[p->len] = 0;
    LOG_PRINTF("rxdata: %s\r\n", g_tData);

    STRCPY((char *)g_tData, "recv it, ok!");
    ssv_test_udp_tx(g_tData, STRLEN((char *)g_tData), port);

    pbuf_free(p);   /* don't leak the pbuf! */
}


int ssv_test_udp_init(void)
{
    int ret = 1;

#if LWIP_IGMP
    ret =  netmgr_igmp_enable(true);
    if (ret != 0)
    {
        LOG_PRINTF("netmgr_igmp_enable: ret = %d\r\n", ret);
        return ret;
    }
#endif /* LWIP_IGMP */

    g_upcb = udp_new();
    if (g_upcb == NULL)
    {
        LOG_PRINTF("udp_new fail\r\n");
        return -1;
    }

    ipMultiCast.addr = lwip_htonl(LISTEN_ADDR);   // 239.1.1.255

#if LWIP_IGMP
    ret = igmp_joingroup(IP_ADDR_ANY,(struct ip_addr *) (&ipMultiCast));
    if (ret != 0)
    {
        LOG_PRINTF("igmp_joingroup: ret = %d\r\n", ret);
        return ret;
    }
#endif

    ret = udp_bind(g_upcb, IP_ADDR_ANY, LISTEN_PORT);
    if (ret != 0)
    {
        LOG_PRINTF("udp_bind: ret = %d\r\n", ret);
        return ret;
    }

    udp_recv(g_upcb, ssv_test_udp_rx, (void *)0);

    LOG_PRINTF("ssv_test_udp_init ok\r\n");

    return ret;
}
#endif

/* Optimize wifi tx/rx */
void ssv_optimize_rf_parameter(void)
{
	printk("SSV rf parameter V1.1\n");
	ssv6xxx_drv_write_reg(0xce0071bc, 0x79806C72);   /* increase tx power, increase 1.5db */

	/* Optimize rx */
	ssv6xxx_drv_write_reg(0xce004010, 0x3f304905);
	ssv6xxx_drv_write_reg(0xce000004, 0x217f);
	ssv6xxx_drv_write_reg(0xce0043fc, 0x104e5);
	ssv6xxx_drv_write_reg(0xce0040c0, 0x7f000280);
}
