/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <rtos.h>
#include <log.h>
#include <ssv_ether.h>

#if defined(CONFIG_NVRAM_CONFIG)
#include <nvram_config.h>

#define WIFI_MAC_NVRAM	"WIFI_MAC"
#define WIFI_MAC_LEN	6
#endif

typedef enum t_MAC_METHOD_TYP{
    EFUSE_MAC,
    ETH0_MAC,
    RANDOM_MAC,
    OTHER_MAC,
}MAC_METHOD_TYP;
MAC_METHOD_TYP g_mac_mth=OTHER_MAC;

extern u8 config_mac[];

/*
 * output : u8_t *buf:  mac address(6 byte mac address)
 * return 0: get mac success, other: failed
*/
int wifi_get_mac(u8_t *buf)
{
#if defined(CONFIG_NVRAM_CONFIG)
	int ret;

	ret = nvram_config_get(WIFI_MAC_NVRAM, buf, WIFI_MAC_LEN);
	if (ret < 0) {
		return -1;
	} else {
		return 0;
	}
#else
	return -1;
#endif
}

/*
 * input : u8_t *buf:  mac address(6 byte mac address)
 * return 0: store mac success, other: failed
*/
int wifi_store_mac(u8_t *buf)
{
#if defined(CONFIG_NVRAM_CONFIG)
	int ret;

	ret = nvram_config_set(WIFI_MAC_NVRAM, buf, WIFI_MAC_LEN);
	if (ret < 0) {
		return -1;
	} else {
		return 0;
	}
#else
	return -1;
#endif
}

int ssv6xxx_get_cust_mac(u8 *mac)
{
    char *mac_method = "default";
#if(CONFIG_EFUSE_MAC ==1)
    read_efuse_macaddr(mac);
    if (is_valid_ether_addr(mac))
    {
        mac_method = "efuse";
        g_mac_mth = EFUSE_MAC;
        goto done;
    }
    else
    {
        ssv6xxx_memcpy((void*)mac,(void*)config_mac,6); //Recovery as default
    }
#endif//CONFIG_EFUSE_MAC ==1
#ifdef __SSV_UNIX_SIM__
    if(get_eth0_as_mac(mac)==0)
    {
        mac_method = "eth0";
        g_mac_mth = ETH0_MAC;
        goto done;
    }
#endif//__SSV_UNIX_SIM__
#if(CONFIG_RANDOM_MAC ==1)
	if (wifi_get_mac(mac) == 0) {
		mac_method = "nvram";
		g_mac_mth = OTHER_MAC;
		OS_MemCPY((void*)&config_mac[0],(void*)&mac[0],6);
		goto done;
	}

    if(g_mac_mth == RANDOM_MAC)
    {
        OS_MemCPY((void*)&mac[0],(void*)&config_mac[0],6);
        mac_method = "random-ori";
    }
    else
    {
        mac[0] = 0x60;mac[1] = 0x11;mac[2] = 0x33;mac[3] = 0x33;mac[4] = 0x33;mac[5] = 0x33;
        ssv_hal_gen_rand((mac+2),4);
        /*check the mac address is valid*/
        if (is_valid_ether_addr(mac))
        {
            g_mac_mth = RANDOM_MAC;
            mac_method = "random";
            OS_MemCPY((void*)&config_mac[0],(void*)&mac[0],6);
            wifi_store_mac(mac);
        }
    }
#endif//CONFIG_RANDOM_MAC ==1
done:

    LOG_PRINTF("[Info ] use \"%s\" to get mac address \r\n",mac_method);
	LOG_PRINTF("MAC= %x:%x:%x:%x:%x:%x\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return 0;
}

