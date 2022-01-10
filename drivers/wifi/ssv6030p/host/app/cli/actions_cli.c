/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include <rtos.h>
#include <log.h>
#include <porting.h>
#include <net/buf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include "../netmgr/net_mgr.h"
#include "cli.h"
#ifdef CONFIG_SWITCH_APPFAMILY
#include <soc_reboot.h>
#endif
#if (CLI_ENABLE==0)

#define WIFI_CONNECT_JOIN_ASYNC		1		/* Join a AP that is already in the AP list */

#define WIFI_SUPPORT_POWER_SAVE		1

#define ACT_CLI_ARG_SIZE	5

typedef enum{
	WIFI_EVT_SCAN_RESULT,
	WIFI_EVT_JOIN_RESULT,
	WIFI_EVT_SCAN_DONE,
	WIFI_EVT_SCONFIG_SCAN_DONE,
	WIFI_EVT_LEAVE_RESULT,
	WIFI_EVT_STATUS
}wifi_evt_type;

enum {
	WIFI_POWER_SAVING,
	WIFI_POWER_SAVE,
	WIFI_POWER_WAKING,
	WIFI_POWER_WAKE,
};

typedef struct
{
	u8 rssi;
	u8 ssid_len;
	u8 *ssid_buf;
	u8 *pwd;
	u8 *addr;
	s32 state;
}wifi_result;

typedef void (*wifi_evtcb_fn)(unsigned int nEvtId, void *data, unsigned int len);

extern void cmd_rf_test(s32 argc, char *argv[]);
void act_cmd_wifi_list(s32 argc, char *argv[]);

static K_MUTEX_DEFINE(cb_handle_mutex);

extern u16 g_SconfigChannelMask;
extern u32 g_sconfig_solution;
static wifi_evtcb_fn app_cb_handle = NULL;
u16_t airkiss_random = 0xFFFF;

#if WIFI_SUPPORT_POWER_SAVE
#define WIFI_POWER_SAVE_CHECK_INTERVAL	(1*MSEC_PER_SEC)
#define WIFI_POWER_SAVE_CHEDK_CNT		3

struct k_delayed_work wifi_power_save;
static K_MUTEX_DEFINE(wifi_ps_mutex);
static u8	wifi_PS_cnt;
static u8	wifi_power_state;

extern bool enter_ps_reserv_buf(void);
extern bool net_pkt_enough_buf(void);

static void wifi_enter_ps(void)
{
    struct cfg_ps_request wowreq;
    u32 ipaddr, gw, netmask;

    netdev_getipv4info(WLAN_IFNAME, &ipaddr, &gw, &netmask);
    MEMSET((void*)&wowreq,0,sizeof(wowreq));
    wowreq.ipv4addr = ipaddr;
    wowreq.dtim_multiple = 3;
    wowreq.host_ps_st = HOST_PS_SETUP;
    ssv6xxx_wifi_pwr_saving(&wowreq,TRUE);
}

void wifi_exit_ps(bool exit)
{
	int i = 0;

	wifi_PS_cnt = 0;

	if (!exit || (wifi_power_state == WIFI_POWER_WAKE)) {
		return;
	}

	/* Wifi power save used 600ms, wait timeout 2000ms */
	while ((i++ < 40) && (wifi_power_state == WIFI_POWER_SAVING)) {
		k_sleep(50);
	}

	k_mutex_lock(&wifi_ps_mutex, K_FOREVER);

	if (wifi_power_state == WIFI_POWER_SAVE) {
		//printk("ww enter\n");
		wifi_power_state = WIFI_POWER_WAKING;
		ssv6xxx_wifi_wakeup();
		//printk("ww exit\n");
	} else if (wifi_power_state == WIFI_POWER_SAVING) {
		printk("wifi still in WIFI_POWER_SAVING state\n");
	}

	k_mutex_unlock(&wifi_ps_mutex);
}

static void wifi_power_save_timeout(struct k_work *work)
{
	int is_joining, re_times;
	Ap_sta_status info;

	if ((wifi_power_state == WIFI_POWER_WAKE) && (app_cb_handle == NULL) &&
		net_pkt_enough_buf()) {
		MEMSET(&info , 0 , sizeof(Ap_sta_status));
		ssv6xxx_wifi_status(&info);
		netmgr_get_ap_join_info(&is_joining, &re_times);
		if ((info.status == 0) || (info.operate != SSV6XXX_HWM_STA) ||
			(is_joining == 1) || (!enter_ps_reserv_buf())) {
			wifi_PS_cnt = 0;
			goto exit;
		}

		k_mutex_lock(&wifi_ps_mutex, K_FOREVER);

		if ((wifi_power_state == WIFI_POWER_WAKE) &&
			(wifi_PS_cnt++ >= WIFI_POWER_SAVE_CHEDK_CNT)) {
			wifi_PS_cnt = 0;
			//printk("wp enter\n");
			wifi_power_state = WIFI_POWER_SAVING;
			wifi_enter_ps();
			//printk("wp exit\n");
		}

		k_mutex_unlock(&wifi_ps_mutex);
	}

exit:
	k_delayed_work_submit(&wifi_power_save, WIFI_POWER_SAVE_CHECK_INTERVAL);
}

void init_power_save_delay_work(void)
{
	static bool init_once = false;

	wifi_PS_cnt = 0;
	wifi_power_state = WIFI_POWER_WAKE;

	if (!init_once) {
		init_once = true;
		k_delayed_work_init(&wifi_power_save, wifi_power_save_timeout);
		k_delayed_work_submit(&wifi_power_save, WIFI_POWER_SAVE_CHECK_INTERVAL);
	}
}
#else
void wifi_exit_ps(bool exit){}
#endif

void act_cmd_scan(s32 argc, char *argv[])
{
    if (argc > 1)
    {
        int num_ssids = argc - 2;
        u16 channel = (u16)strtol(argv[1],NULL,16);

        if (num_ssids > 5)
        {
            num_ssids = 5;
        }

        if (num_ssids > 0)
        {
            char *ssids = NULL;
            int i = 0;

            ssids = MALLOC(num_ssids * MAX_SSID_LEN);
            if (ssids == NULL)
            {
                return;
            }

            for (i = 0; i < num_ssids; i++)
            {
                if (strlen(argv[2 + i]) < MAX_SSID_LEN)
                {
                    MEMCPY((char *)(ssids + i * MAX_SSID_LEN), argv[2 + i], strlen(argv[2 + i]) + 1);
                }
                else
                {
                    MEMCPY((char *)(ssids + i * MAX_SSID_LEN), argv[2 + i], MAX_SSID_LEN);
                }
            }

            netmgr_wifi_scan_async(channel, ssids, num_ssids);
        }
        else
        {
            netmgr_wifi_scan_async(channel, NULL, 0);
        }
    }
    else
    {
	    netmgr_wifi_scan_async(0, NULL, 0);
    }
}

static void join_ap_scan(s32 argc, char *argv[], bool scan)
{
	int ret;
	wifi_mode mode;
	wifi_sta_join_cfg join_cfg;

	memset((void * )&join_cfg, 0, sizeof(join_cfg));
	mode = SSV6XXX_HWM_STA;
	memcpy((void *)join_cfg.ssid.ssid, argv[1], strlen(argv[1]));
	join_cfg.ssid.ssid_len = strlen(argv[1]);
	if ((argc > 2) && (strlen(argv[2]) > 0))
	{
		strcpy((void *)join_cfg.password, argv[2]);
	}

	if (netmgr_wifi_check_sta())
	{
		if (scan) {
			ret = netmgr_wifi_join_other_async(&join_cfg);
		} else {
			ret = netmgr_wifi_join_async(&join_cfg);
		}
	}
	else
	{
		ret = netmgr_wifi_switch_async(mode, NULL, &join_cfg);
	}

	if (ret != 0)
	{
		LOG_PRINTF("STA mode error\r\n");
		return;
	}

	LOG_PRINTF("STA mode success\r\n");
}

#if WIFI_CONNECT_JOIN_ASYNC
#define WIFI_SCAN_CHANNEL_1611			0x842
#define WIFI_SCAN_CHANNEL_ALL			0x0
#define WIFI_SCAN_1611_TIMEOUT			(150*3 + 1000)		/* 150ms one channel */
#define WIFI_SCAN_ALL_TIMEOUT			(150*13 + 1000)		/* 150ms one channel */

static const struct {
	u16_t channel;
	s32_t timeout;
} const channel_timeout[] = {
	{WIFI_SCAN_CHANNEL_1611, WIFI_SCAN_1611_TIMEOUT},
	{WIFI_SCAN_CHANNEL_ALL, WIFI_SCAN_ALL_TIMEOUT},
	{WIFI_SCAN_CHANNEL_ALL, WIFI_SCAN_ALL_TIMEOUT}
};

static K_MUTEX_DEFINE(wifi_connect_mutex);
static struct k_sem scan_done_sem;
static char *join_ssid = NULL;
static char join_ap_flag = 0;
static char scan_ap_cnt = 0;

static void join_scan_result_cb(char *ssid, u8_t len, u8_t rssi)
{
	if (join_ap_flag) {
		if ((join_ssid) && (strlen(join_ssid) == len) &&
			(!memcmp(join_ssid, ssid, len))) {
			scan_ap_cnt++;
		}
	}
}

static void join_scan_done_cb(void)
{
	if (join_ap_flag) {
		k_sem_give(&scan_done_sem);
	}
}

void act_cmd_join(s32 argc, char *argv[])
{
	int i;
	char *ssids = NULL;
	wifi_result cb_result;

	k_mutex_lock(&wifi_connect_mutex, K_FOREVER);
	join_ap_flag = 1;
	scan_ap_cnt = 0;
	join_ssid = argv[1];

	for (i=0; i<ARRAY_SIZE(channel_timeout); i++) {
		ssids = MALLOC(MAX_SSID_LEN + 1);		/* netmgr_wifi_scan_async will free ssids  */
		if (ssids == NULL) {
			continue;
		}

		if (strlen(argv[1]) < MAX_SSID_LEN) {
			MEMCPY(ssids, argv[1], (strlen(argv[1]) + 1));
		} else {
			MEMCPY(ssids, argv[1], MAX_SSID_LEN);
		}
		k_sem_reset(&scan_done_sem);
		netmgr_wifi_scan_async(channel_timeout[i].channel, ssids, 1);
		k_sem_take(&scan_done_sem, channel_timeout[i].timeout);

		if (scan_ap_cnt) {
			break;
		}
	}

	if (scan_ap_cnt == 0) {
		cb_result.state = 1;
		k_mutex_lock(&cb_handle_mutex, K_FOREVER);
		if (app_cb_handle) {
			app_cb_handle(WIFI_EVT_JOIN_RESULT, &cb_result, sizeof(cb_result));
		}
		k_mutex_unlock(&cb_handle_mutex);
		printk("Connect can't scan ap: %s\n", argv[1]);
	} else {
		/* join ap without scan */
		join_ap_scan(argc, argv, false);
	}

	join_ap_flag = 0;
	k_mutex_unlock(&wifi_connect_mutex);
}
#else
void act_cmd_join(s32 argc, char *argv[])
{
	/* join ap with scan */
	join_ap_scan(argc, argv, true);
}
#endif

void printf_data(void *data, int len)
{
    int i = 0;
    LOG_PRINTF("len: %d\r\n", len);
    for (i = 0 ; i < len; i++)
    {
        if (i % 16 == 0) LOG_PRINTF("\r\n");
        LOG_PRINTF("0x%02X, ", *((char *)data + i) & 0xff);
    }
    LOG_PRINTF("\r\n");
}

void act_cmd_join_fast(s32 argc, char *argv[])
{
    struct cfg_join_request *joincfg = MALLOC(sizeof(struct cfg_join_request));
    int len = 0;
    LOG_PRINTF("start joinfast tick=%d\r\n", OS_GetSysTick());
    if (ssv6xxx_wifi_get_joincfg(joincfg, &len) == 0)
    {
        printf_data(joincfg, sizeof(struct cfg_join_request));
        netmgr_wifi_join_fast_async(joincfg);
    }
    else
    {
        LOG_PRINTF("joinfast fail\r\n");
    }

    FREE(joincfg);

    LOG_PRINTF("end joinfast tick=%d\r\n", OS_GetSysTick());
}

void act_cmd_airkiss(s32 argc, char *argv[])
{
	wifi_mode mode;
	Sta_setting sta;
	int ret = 0;
	int times = 0;

	mode = SSV6XXX_HWM_SCONFIG;
	MEMSET(&sta, 0 , sizeof(Sta_setting));

	sta.status = TRUE;
	ret = netmgr_wifi_control_async(mode, NULL, &sta);
	if (ret != 0)
	{
		LOG_PRINTF("SCONFIG mode error\r\n");
	}

	while(!netmgr_wifi_check_sconfig())
	{
		if (times > 100)
		{
			break;
		}
		times++;
		OS_MsDelay(10);
	}

	g_SconfigChannelMask=0x3FFE; //This variable is only for RD use, customers don't need to modify it.
	g_sconfig_solution=SMART_CONFIG_SOLUTION;
	ret = netmgr_wifi_sconfig_async(g_SconfigChannelMask);
	if (ret != 0)
	{
		LOG_PRINTF("SCONFIG async error\r\n");
		return;
	}

	LOG_PRINTF("SCONFIG mode success\r\n");
}

void act_cmd_iw(s32 argc, char *argv[])
{
	if (!strcmp(argv[1], "scan")) {
		act_cmd_scan((argc - 1), &argv[1]);
	} else if (!strcmp(argv[1], "join")) {
		act_cmd_join((argc - 1), &argv[1]);
	} else if (!strcmp(argv[1], "list")) {
		act_cmd_wifi_list((argc - 1), &argv[1]);
	} else {
		printk("%s, unknow cmd:%s\n", __func__, argv[1]);
	}
}

void act_cmd_ctl(s32 argc, char *argv[])
{
	if (!strcmp(argv[1], "sconfig")) {
		act_cmd_airkiss(argc, argv);
	} else {
		printk("%s, unknow cmd:%s\n", __func__, argv[1]);
	}
}

void act_cmd_leave(s32 argc, char *argv[])
{
	int ret = -1;

    ret = netmgr_wifi_leave_async();
    if (ret != 0)
    {
	    printk("netmgr_wifi_leave failed !!\r\n");
    }
}

void act_cmd_stamode(s32 argc, char *argv[])
{
	wifi_mode mode;
	Sta_setting sta;
	int ret = 0;
	int times = 0;

	mode = SSV6XXX_HWM_STA;
	MEMSET(&sta, 0 , sizeof(Sta_setting));

	sta.status = TRUE;
	ret = netmgr_wifi_control_async(mode, NULL, &sta);
	if (ret != 0)
	{
		LOG_PRINTF("SCONFIG mode error\r\n");
	}

	while(!netmgr_wifi_check_sta())
	{
		if (times > 100)
		{
			break;
		}
		times++;
		OS_MsDelay(10);
	}
}

void act_cmd_rf(s32 argc, char *argv[])
{
    cmd_rf_test(argc, argv);
}

void act_cmd_status(s32 argc, char *argv[])
{
	Ap_sta_status info;
	wifi_result cb_result;

	MEMSET(&info , 0 , sizeof(Ap_sta_status));
	MEMSET(&cb_result, 0, sizeof(cb_result));

	ssv6xxx_wifi_status(&info);

	if ((SSV6XXX_HWM_STA==info.operate)||(SSV6XXX_HWM_SCONFIG==info.operate)) {
		cb_result.state = info.operate;
		cb_result.ssid_buf = info.u.station.ssid.ssid;
		cb_result.ssid_len = info.u.station.ssid.ssid_len;
	} else {
		cb_result.state = -1;
	}

	k_mutex_lock(&cb_handle_mutex, K_FOREVER);
	if (app_cb_handle) {
		app_cb_handle(WIFI_EVT_STATUS, &cb_result, sizeof(cb_result));
	}
	k_mutex_unlock(&cb_handle_mutex);
}

void act_cmd_status_2(s32 argc, char *argv[])
{
    bool errormsg = FALSE;
    u8 ssid_buf[32+1]={0};
    Ap_sta_status info;

    MEMSET(&info , 0 , sizeof(Ap_sta_status));
    errormsg = FALSE;
    ssv6xxx_wifi_status(&info);
    if(info.status)
        LOG_PRINTF("status:ON\r\n");
    else
        LOG_PRINTF("status:OFF\r\n");
    if((SSV6XXX_HWM_STA==info.operate)||(SSV6XXX_HWM_SCONFIG==info.operate))
    {
        LOG_PRINTF("Mode:%s, %s\r\n",(SSV6XXX_HWM_STA==info.operate)?"Station":"Sconfig",(info.u.station.apinfo.status == CONNECT) ? "connected" :"disconnected");
        LOG_PRINTF("self Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            info.u.station.selfmac[0],
            info.u.station.selfmac[1],
            info.u.station.selfmac[2],
            info.u.station.selfmac[3],
            info.u.station.selfmac[4],
            info.u.station.selfmac[5]);
        MEMCPY((void*)ssid_buf,(void*)info.u.station.ssid.ssid,info.u.station.ssid.ssid_len);
        LOG_PRINTF("SSID:%s\r\n",ssid_buf);
        LOG_PRINTF("channel:%d\r\n",info.u.station.channel);
        LOG_PRINTF("AP Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            info.u.station.apinfo.Mac[0],
            info.u.station.apinfo.Mac[1],
            info.u.station.apinfo.Mac[2],
            info.u.station.apinfo.Mac[3],
            info.u.station.apinfo.Mac[4],
            info.u.station.apinfo.Mac[5]);
        if(is_valid_ether_addr(info.u.station.apinfo.Mac)){
            LOG_PRINTF("RSSI = -%d (dBm)\r\n",ssv6xxx_get_rssi_by_mac(info.u.station.apinfo.Mac));
        }
        else{
            LOG_PRINTF("RSSI = 0 (dBm)\r\n");
        }
    }
}

int act_get_connect_ap_status(char *ssid, char *mac, int *rssi)
{
	Ap_sta_status info;

	wifi_exit_ps(true);
	k_sleep(20);

	MEMSET(&info , 0 , sizeof(Ap_sta_status));
	ssv6xxx_wifi_status(&info);

	if (info.status && (SSV6XXX_HWM_STA==info.operate) &&
		(info.u.station.apinfo.status == CONNECT)) {
		if (ssid) {
			MEMCPY((void*)ssid, (void*)info.u.station.ssid.ssid, info.u.station.ssid.ssid_len);
		}

		if (mac) {
			MEMCPY((void*)mac, info.u.station.apinfo.Mac, ETHER_ADDR_LEN);
		}
		if(is_valid_ether_addr(info.u.station.apinfo.Mac)) {
			*rssi = -ssv6xxx_get_rssi_by_mac(info.u.station.apinfo.Mac);
		} else {
			*rssi = 0;
		}

		return 0;
	}

	return -1;
}

#define WPA_AUTH_ALG_OPEN BIT(0)
#define WPA_AUTH_ALG_SHARED BIT(1)
#define WPA_AUTH_ALG_LEAP BIT(2)
#define WPA_AUTH_ALG_FT BIT(3)

//#define SEC_USE_NONE
//#define SEC_USE_WEP40_PSK
//#define SEC_USE_WEP40_OPEN
//#define SEC_USE_WEP104_PSK
//#define SEC_USE_WEP104_OPEN
//#define SEC_USE_WPA_TKIP
#define SEC_USE_WPA2_CCMP

void act_cmd_wifi_list(s32 argc, char *argv[])
{
    u32 i=0,AP_cnt;
    s32     pairwise_cipher_index=0,group_cipher_index=0;
    u8      sec_str[][7]={"OPEN","WEP40","WEP104","TKIP","CCMP"};
    u8  ssid_buf[MAX_SSID_LEN+1]={0};
    Ap_sta_status connected_info;

    struct ssv6xxx_ieee80211_bss *ap_list = NULL;
    AP_cnt = ssv6xxx_get_aplist_info((void *)&ap_list);


    MEMSET(&connected_info , 0 , sizeof(Ap_sta_status));
    ssv6xxx_wifi_status(&connected_info);

    if((ap_list==NULL) || (AP_cnt==0))
    {
        LOG_PRINTF("AP list empty!\r\n");
        return;
    }
    for (i=0; i<AP_cnt; i++)
    {

        if(ap_list[i].channel_id!= 0)
		{
		    LOG_PRINTF("BSSID: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            ap_list[i].bssid.addr[0],  ap_list[i].bssid.addr[1], ap_list[i].bssid.addr[2],  ap_list[i].bssid.addr[3],  ap_list[i].bssid.addr[4],  ap_list[i].bssid.addr[5]);
            MEMSET((void*)ssid_buf,0,sizeof(ssid_buf));
            MEMCPY((void*)ssid_buf,(void*)ap_list[i].ssid.ssid,ap_list[i].ssid.ssid_len);
            LOG_PRINTF("SSID: %s\t", ssid_buf);
			LOG_PRINTF("@Channel Idx: %d\r\n", ap_list[i].channel_id);
            if(ap_list[i].capab_info&BIT(4)){
                LOG_PRINTF("Secure Type=[%s]\r\n",
                ap_list[i].proto&WPA_PROTO_WPA?"WPA":
                ap_list[i].proto&WPA_PROTO_RSN?"WPA2":"WEP");

                if(ap_list[i].pairwise_cipher[0]){
                    pairwise_cipher_index=0;
                    LOG_PRINTF("Pairwise cipher=");
                    for(pairwise_cipher_index=0;pairwise_cipher_index<8;pairwise_cipher_index++){
                        if(ap_list[i].pairwise_cipher[0]&BIT(pairwise_cipher_index)){
                            LOG_PRINTF("[%s] ",sec_str[pairwise_cipher_index]);
                        }
                    }
                    LOG_PRINTF("\r\n");
                }
                if(ap_list[i].group_cipher){
                    group_cipher_index=0;
                    LOG_PRINTF("Group cipher=");
                    for(group_cipher_index=0;group_cipher_index<8;group_cipher_index++){
                        if(ap_list[i].group_cipher&BIT(group_cipher_index)){
                            LOG_PRINTF("[%s] ",sec_str[group_cipher_index]);
                        }
                    }
                    LOG_PRINTF("\r\n");
                }
            }else{
                LOG_PRINTF("Secure Type=[NONE]\r\n");
            }

            if(!memcmp((void *)ap_list[i].bssid.addr,(void *)connected_info.u.station.apinfo.Mac,ETHER_ADDR_LEN)){
                LOG_PRINTF("RSSI=-%d (dBm)\r\n",ssv6xxx_get_rssi_by_mac((u8 *)ap_list[i].bssid.addr));
            }
            else{
                LOG_PRINTF("RSSI=-%d (dBm)\r\n",ap_list[i].rxphypad.rpci);
            }
            LOG_PRINTF("\r\n");
		}

    }
    FREE((void *)ap_list);
}

CLICmds ActCmdTable[] = {
	{ "iw",			act_cmd_iw,			"iw scan/join"},
	{ "ctl",		act_cmd_ctl,		"ctl sconfig on"},
	{ "scan",		act_cmd_scan,		"scan"},
	{ "join",		act_cmd_join,		"join"},
	{ "joinfast",   act_cmd_join_fast,	"joinfast"},
	{ "airkiss",	act_cmd_airkiss,	"airkiss"},
	{ "leave",		act_cmd_leave,		"leave"},
	{ "status",		act_cmd_status,		"status"},
	{ "status2",	act_cmd_status_2,	"status2"},
	{ "list",	    act_cmd_wifi_list,	"list"},
	{ "stamode",	act_cmd_stamode,	"stamode"},
	{ "rf",	        act_cmd_rf,	        "rftest"},
	{ NULL, 		NULL, 				NULL }
};

extern bool ssv_wifi_init_done;

s32 Cli_RunCmd(char *CmdBuffer)
{
	CLICmds *CmdPtr;
	u8 ch;
	char *pch;
	char	*sgArgV[ACT_CLI_ARG_SIZE];
	u8 	sgArgC;

	if (!ssv_wifi_init_done) {
		return -1;
	}

	for (sgArgC=0,ch=0, pch=CmdBuffer; (*pch!=0x00)&&(sgArgC<ACT_CLI_ARG_SIZE); pch++) {
		if ((ch==0) && (*pch!=' ') && (*pch!=0x0a) && (*pch!=0x0d)) {
			ch = 1;
			sgArgV[sgArgC] = pch;
		}

		if ((ch==1) && ((*pch==' ')||(*pch=='\t')||(*pch==0x0a)||(*pch==0x0d))) {
			*pch = 0x00;
			ch = 0;
			sgArgC ++;
		}
	}

	if (sgArgC == ACT_CLI_ARG_SIZE) {
		LOG_PRINTF("Total nummber of arg are over %d \r\n", ACT_CLI_ARG_SIZE);
		return 0;
	}

	if ( ch == 1) {
		sgArgC ++;
	}

	if (sgArgC <= 0)
		return 0;

	for( CmdPtr=ActCmdTable; CmdPtr->Cmd; CmdPtr ++ ) {
		if (!strcmp(sgArgV[0], CmdPtr->Cmd)) {
			wifi_exit_ps(true);
			CmdPtr->CmdHandler(sgArgC, sgArgV);
			break;
		}
	}

	if (NULL == CmdPtr->Cmd)
		return -1;

	return 0;
}

s32 Cli_RunCmd_arg(s32 argc, char *argv[])
{
	CLICmds *CmdPtr;

	if (!ssv_wifi_init_done) {
		return -1;
	}

	for( CmdPtr=ActCmdTable; CmdPtr->Cmd; CmdPtr++) {
		if (!strcmp(argv[0], CmdPtr->Cmd)) {
			wifi_exit_ps(true);
			CmdPtr->CmdHandler(argc, argv);
			break;
		}
	}

	if (NULL == CmdPtr->Cmd)
		return -1;

	return 0;
}

static struct net_pkt *prepare_udp(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport)
{
	struct net_if *iface;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_ipv4_hdr *ipv4;
	struct net_udp_hdr hdr, *udp;
	struct in_addr tmpaddr;
	uint16_t tmplen;

	iface = net_if_get_default();

	pkt = net_pkt_get_reserve_tx(0, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	frag = net_pkt_get_reserve_tx_data(net_if_get_ll_reserve(iface, NULL),
					 K_FOREVER);
	if (!frag) {
		net_pkt_unref(pkt);
		return NULL;
	}

	net_pkt_set_ll_reserve(pkt, net_buf_headroom(frag));
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	net_pkt_frag_add(pkt, frag);

	/* Leave room for IPv4 + UDP headers */
	net_buf_add(pkt->frags, NET_IPV4UDPH_LEN);

	memcpy((frag->data + NET_IPV4UDPH_LEN), data, len);
	net_buf_add(frag, len);

	ipv4 = NET_IPV4_HDR(pkt);
	udp = (struct net_udp_hdr *)net_udp_get_hdr(pkt, &hdr);

	tmplen = net_buf_frags_len(pkt->frags);

	/* Setup IPv4 header */
	memset(ipv4, 0, sizeof(struct net_ipv4_hdr));

	ipv4->vhl = 0x45;
	ipv4->ttl = 0xFF;
	ipv4->proto = IPPROTO_UDP;
	ipv4->len[0] = tmplen >> 8;
	ipv4->len[1] = (uint8_t)tmplen;
	ipv4->chksum = ~net_calc_chksum_ipv4(pkt);

	tmpaddr.s_addr = srcip;
	net_ipaddr_copy(&ipv4->src, &tmpaddr);
	tmpaddr.s_addr = dstip;
	net_ipaddr_copy(&ipv4->dst, &tmpaddr);

	tmplen -= NET_IPV4H_LEN;
	/* Setup UDP header */
	udp->src_port = htons(srcport);
	udp->dst_port = htons(dstport);
	udp->len = htons(tmplen);
	udp->chksum = 0;
	udp->chksum = ~net_calc_chksum(pkt, IPPROTO_UDP);

	return pkt;
}

int airkiss_udp_send(void)
{
	/* Wait todo, call to send airkiss config result */
	struct net_pkt *pkt;
	int i;
	u8_t ramdom;
#ifdef CONFIG_SWITCH_APPFAMILY
	if(sys_get_reboot_cmd() != REBOOT_CMD_NONE) {
		airkiss_random = sys_get_reboot_paramater();
		sys_set_reboot_paramater(0);
	}
#endif

	if (airkiss_random == 0xFFFF) {
		return 0;
	}

	ramdom = (u8_t)airkiss_random;
	for (i=0; i <20; i++) {
		pkt = prepare_udp(&ramdom, 1, 0, 9999, 0xFFFFFFFF, 10000);
		if (!pkt) {
			continue;
		}

		if (net_send_data(pkt) < 0) {
			net_pkt_unref(pkt);
		}
	}

	airkiss_random = 0xFFFF;
	return 0;
}

void netdev_status_change_cb(void *netif)
{
	airkiss_udp_send();
}

static void wifi_event_cb_handler(u32 evt_id, void *data, s32 len)
{
	struct resp_evt_result *evt_result;
	ap_info_state *scan_res;
	wifi_result cb_result;
	u32 reEvt = 0xFFFF;

	MEMSET(&cb_result, 0, sizeof(cb_result));
	switch (evt_id) {
		case SOC_EVT_SCAN_RESULT:
			scan_res = (ap_info_state*)data;
			cb_result.rssi = scan_res->apInfo[scan_res->index].rxphypad.rpci;
			cb_result.ssid_buf = scan_res->apInfo[scan_res->index].ssid.ssid;
			cb_result.ssid_len = scan_res->apInfo[scan_res->index].ssid.ssid_len;
			cb_result.addr = scan_res->apInfo[scan_res->index].bssid.addr;
			reEvt = WIFI_EVT_SCAN_RESULT;
#if WIFI_CONNECT_JOIN_ASYNC
			join_scan_result_cb(cb_result.ssid_buf, cb_result.ssid_len, cb_result.rssi);
#endif
			break;
		case SOC_EVT_JOIN_RESULT:
			evt_result = (struct resp_evt_result *)data;
			cb_result.state = evt_result->u.join.status_code;
			if (evt_result->u.join.status_code == 1)
			{
				printk("Join failure!!\r\n");
			}
			if (evt_result->u.join.status_code == 2)
			{
				printk("Join failure because of wrong password !!\r\n");
			}
			reEvt = WIFI_EVT_JOIN_RESULT;
			break;
		case SOC_EVT_SCAN_DONE:
			evt_result = (struct resp_evt_result *)data;
			cb_result.state = evt_result->u.scan_done.result_code;
			reEvt = WIFI_EVT_SCAN_DONE;
			printk("Scan down\n");
#if WIFI_CONNECT_JOIN_ASYNC
			join_scan_done_cb();
#endif
			break;
		case SOC_EVT_SCONFIG_SCAN_DONE:
			evt_result = (struct resp_evt_result *)data;
			cb_result.state = evt_result->u.sconfig_done.result_code;
			if (0 == cb_result.state) {
				cb_result.ssid_buf = evt_result->u.sconfig_done.ssid;
				cb_result.ssid_len = evt_result->u.sconfig_done.ssid_len;
				cb_result.pwd = evt_result->u.sconfig_done.pwd;
			}
			reEvt = WIFI_EVT_SCONFIG_SCAN_DONE;
			airkiss_random = evt_result->u.sconfig_done.rand;
		#ifdef CONFIG_SWITCH_APPFAMILY
			sys_set_reboot_paramater(airkiss_random);
		#endif
			break;
		case SOC_EVT_LEAVE_RESULT:
			evt_result = (struct resp_evt_result *)data;
			cb_result.state = evt_result->u.leave.reason_code;
			reEvt = WIFI_EVT_LEAVE_RESULT;
			printk("%s SOC_EVT_LEAVE_RESULT reason_code:%d\n", __func__, evt_result->u.leave.reason_code);
			break;
		case SOC_EVT_PS_SETUP_OK:
#if WIFI_SUPPORT_POWER_SAVE
			wifi_PS_cnt = 0;
			wifi_power_state = WIFI_POWER_SAVE;
			//printk("wp cb\n");
#endif
			break;
		case SOC_EVT_PS_WAKENED:
#if WIFI_SUPPORT_POWER_SAVE
			wifi_PS_cnt = 0;
			wifi_power_state = WIFI_POWER_WAKE;
			//printk("ww cb\n");
#endif
			break;
		default:
			/* don't care other event */
			return;
	}

	k_mutex_lock(&cb_handle_mutex, K_FOREVER);
	if (app_cb_handle) {
		app_cb_handle(reEvt, &cb_result, sizeof(cb_result));
	}
	k_mutex_unlock(&cb_handle_mutex);
}

/* register/unregister wifi event callback,
*  cbhandle != NULL, register, wifi will not enter power save
*  cbhandle == NULL, unregister, wifi will enter power save
*/
void register_wifi_event_cb(wifi_evtcb_fn cbhandle)
{
	k_mutex_lock(&cb_handle_mutex, K_FOREVER);
	app_cb_handle = cbhandle;
	k_mutex_unlock(&cb_handle_mutex);
}

int run_wifi_cmd_arg(int argc, char *argv[])
{
	Cli_RunCmd_arg(argc, argv);
}

int run_wifi_cmd(char *CmdBuffer)
{
	Cli_RunCmd(CmdBuffer);
}

void actions_cli_init(void)
{
	ssv6xxx_wifi_reg_evt_cb(wifi_event_cb_handler);
#if WIFI_SUPPORT_POWER_SAVE
	init_power_save_delay_work();
#endif

#if WIFI_CONNECT_JOIN_ASYNC
	k_sem_init(&scan_done_sem, 0, 1);
#endif
}

#else

void register_wifi_event_cb(wifi_evtcb_fn evtcb)
{
}

#endif

