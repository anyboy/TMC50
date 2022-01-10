/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ca manager interface for Actions
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <oem_interface.h>
#include <system_config.h>
#include <net_stream.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_IC_TYPE            "229r2"
#define DEFAULT_MAC                "54c9df71d937"
#define DEFAULT_CHIPID             "99-99"

#define UUID_POST_PARAM    "{\"customerID\":\"%s\","\
						   "\"token\":\"%s\","\
						   "\"icType\":\"%s\","\
						   "\"mac\":\"%s\","\
						   "\"projectName\":\"%s\","\
						   "\"chipid\":\"%s\","\
						   "\"deviceID\":\"%s\","\
						   "\"uuid\":\"%s\"}"
#define LICENSE_POST_PARAM "{\"customerID\":\"%s\","\
						   "\"token\":\"%s\","\
						   "\"uuid\":\"%s\","\
						   "\"serviceType\":\"%s\"}"

#define DEVICE_REGISTE_POST_PARAM    "{\"agentId\":\"%s\","\
										"\"ownMac\":\"%s\","\
										"\"chipId\":\"%s\""\
									  "}"

const char roboo_get_uuid_req[] =
	"POST /device/register?sign=%s HTTP/1.1\r\n"
	"cache-control: no-cache\r\n"
	"Content-Type: text/plain\r\n"
	"Content-Length: %d\r\n"
	"Accept: */*\r\n"
	"Host: %s:%s\r\n"
	"Connection: close\r\n\r\n"
	 UUID_POST_PARAM;

const char roboo_get_license_req[] =
	"POST /device/getServiceType?sign=%s HTTP/1.1\r\n"
	"cache-control: no-cache\r\n"
	"Content-Type: text/plain\r\n"
	"Content-Length: %d\r\n"
	"Accept: */*\r\n"
	"Host: %s:%s\r\n"
	"Connection: close\r\n\r\n"
	 LICENSE_POST_PARAM;

const char get_device_req[] =
	"POST /deviceWifi/getSnMac.do?sign=%s HTTP/1.1\r\n"
	"cache-control: no-cache\r\n"
	"Content-Type: text/plain\r\n"
	"Content-Length: %d\r\n"
	"Accept: */*\r\n"
	"Host: %s:%d\r\n"
	"Connection: close\r\n\r\n"
	 DEVICE_REGISTE_POST_PARAM;

const char CODE_TAG[]    = "\"code\":";// int
const char UUID_TAG[]    = "\"uuid\":\""; //string
const char LICENSE_TAG[]    = "\"deviceid\":\""; //string
const char LICENSE_END_TAG[]    = "\"},\"status\""; //string
const char DEVICE_ID_TAG[]    = "\"deviceId\":\""; //string
const char BT_MAC_TAG[]    = "\"mac\":\""; //string
const char APPID_TAG[]    =  "\"appId\":\""; //string
const char DEVICE_TYPE_TAG[]    =  "\"deviceType\":\""; //string

#define UUID_SPEC          "http://%s:%s/device/register?sign=%s"
#define LICENSE_SPEC       "http://%s:%s/device/getServiceType?sign=%s"
#define DEVICE_REGISTE_SPEC          "http://%s:%d/deviceWifi/getSnMac.do"

extern void AH(unsigned char *input, short length, unsigned char *output);

#define DATA_BUFF_LEN 		800
#define URL_BUFF_LEN  		128
#define SINGNATURE_LEN 		20
#define CHIP_ID_LEN 		8
#define MAC_ADDR_LEN 		13

struct register_info
{
	char mac_addr[MAC_ADDR_LEN];
	char chip_id[CHIP_ID_LEN];
	char device_sn[MAX_SN_INFO_LEN];
	char url_buff[URL_BUFF_LEN];
	char data_buff[DATA_BUFF_LEN];
	char signature[SINGNATURE_LEN];
	char bt_mac_addr[MAC_ADDR_LEN];
	char auth_server[URL_BUFF_LEN];
	char auth_agent_id[32];
	int  auth_server_port;
	os_sem response_sem;
	char * result;
};

static struct register_info * reg_info = NULL;
#ifdef CONFIG_NET_STREAM
static int get_item_by_tag(char* source, char*dest, const char* tag, int istr)
{
	int ret = 0;
	char * tmp = NULL;

	source = strstr(source,tag);

	if(source != NULL)
	{
		source = strchr(source,':');
		if(istr)
		{
			source += 2;
			tmp = strchr(source,'"');
		}
		else
		{
			source += 1;
			tmp = strchr(source,',');
		}
		memcpy(dest, source, tmp-source);
		dest[tmp-source] = '\0';
		SYS_LOG_DBG("dest : %s\n",dest);
	}
	else
	{
		SYS_LOG_ERR("get item %s failed ",tag);
		ret = -1;
	}

	return ret;

}

static int register_customer_device_response(char * buffer , int len)
{
	char* tmp = NULL;

	SYS_LOG_DBG("register_customer_device_response: %s ",buffer);

	if(reg_info == NULL)
	{
		goto exit;
	}

	tmp = app_mem_malloc(MAX_ID_LEN);
	if(tmp == NULL)
	{
		goto exit;
	}

	if(!get_item_by_tag(buffer, tmp, CODE_TAG, 0))
	{
		if(atoi(tmp)!=0)
		{
			SYS_LOG_WRN("register error");
		}
	}

	if(!get_item_by_tag(buffer, tmp, UUID_TAG, 1))
	{
		if(memcmp(reg_info->device_sn, tmp, strlen(reg_info->device_sn)))
		{
			SYS_LOG_ERR("register error return-uuid = %s,local-uuid = %s",tmp,reg_info->device_sn);
		}
		else
		{
			k_sem_give(&reg_info->response_sem);
		}
	}

exit:
	if(tmp != NULL)
	{
		app_mem_free(tmp);
	}
	return 0;
}
#endif

int register_customer_device(const struct customer_info *customer)
{
	io_stream_t  netstream = NULL;
#ifdef CONFIG_NET_STREAM
	struct request_info request;
#endif
	int chip_id;
	int ret = 0;
	u16_t len = 0;
	u8_t mac[6] = {0};
	u8_t sign[20] = {0};
	u8_t sign_hex[41] = {0};

	if(customer == NULL)
	{
		SYS_LOG_ERR("customer info is null");
		ret = -1;
		goto exit;
	}

	reg_info = app_mem_malloc(sizeof(struct register_info));
	if(reg_info == NULL)
	{
		//SYS_LOG_ERR("reg_info mem_malloc failed , need %ld bytes mem",sizeof(struct register_info));
		ret = -1;
		goto exit;
	}

	os_sem_init(&reg_info->response_sem, 0, 1);

	if(get_sair_client_id(reg_info->device_sn))
	{
		ret = -1;
		goto exit;
	}

	if(get_wifi_mac_addr(mac,MAC_ADDR_LEN))
	{
		snprintf(reg_info->mac_addr, MAC_ADDR_LEN, DEFAULT_MAC);
	}
	else
	{
		HEXTOSTR(mac, sizeof(mac),reg_info->mac_addr);
	}

	if(get_XY_coordinate(&chip_id))
	{
		snprintf(reg_info->chip_id, CHIP_ID_LEN, DEFAULT_CHIPID);
	}
	else
	{
		snprintf(reg_info->chip_id, CHIP_ID_LEN, "%d-%d", ((chip_id>>16)&0x00ff), ((chip_id)&0x00ff));
	}

	len = snprintf(reg_info->data_buff, DATA_BUFF_LEN, UUID_POST_PARAM,
				customer->customer_id,
				customer->token,
				customer->ic_type,
				reg_info->mac_addr,
				customer->project_name,
				reg_info->chip_id,
				customer->device_id,
				reg_info->device_sn);

	AH((unsigned char *) reg_info->data_buff, len, sign);

	HEXTOSTR(sign, 20, (char*) sign_hex);

	snprintf(reg_info->url_buff, URL_BUFF_LEN, UUID_SPEC,
						customer->server_addr,
						customer->server_port,
						sign_hex);

	snprintf(reg_info->data_buff, DATA_BUFF_LEN, roboo_get_uuid_req, sign_hex, len,
				customer->server_addr,
				customer->server_port,
				customer->customer_id,
				customer->token,
				customer->ic_type,
				reg_info->mac_addr,
				customer->project_name,
				reg_info->chip_id,
				customer->device_id,
				reg_info->device_sn);

	SYS_LOG_DBG("http_head =%s",reg_info->data_buff);

#ifdef CONFIG_NET_STREAM
	memset(&request, 0, sizeof(struct request_info));
	request.url = reg_info->url_buff;
	request.http_head = (char *)reg_info->data_buff;
	request.receive_cb = register_customer_device_response;
	request.send_post_immediately = true;

	netstream = stream_create(TYPE_NET_STREAM, &request);
#endif

	if (netstream == NULL) {
		SYS_LOG_ERR("stream create failed \n");
		ret = -1;
		goto exit;
	}

	ret = stream_open(netstream, MODE_OUT);

	if (ret) {
		SYS_LOG_ERR("stream open failed \n");
		stream_destroy(netstream);
		ret = -1;
		goto exit;
	}

	if(k_sem_take(&reg_info->response_sem, K_MSEC(10000)))
	{
		SYS_LOG_ERR("register uuid timeout");
		ret = -1;
	}

	stream_close(netstream);

	stream_destroy(netstream);

exit:
	if(reg_info != NULL)
	{
		app_mem_free(reg_info);
		reg_info = NULL;
	}
	return ret ;
}
#ifdef CONFIG_NET_STREAM
static int device_authentication_response(char * buffer , int len)
{
	char* tmp = NULL;

	SYS_LOG_DBG("register_customer_device_response: %s ",buffer);

	if(reg_info == NULL)
	{
		goto exit;
	}

	tmp = app_mem_malloc(MAX_ID_LEN);
	if(tmp == NULL)
	{
		goto exit;
	}

	if(!get_item_by_tag(buffer, tmp, CODE_TAG, 0))
	{
		if(atoi(tmp) != 200)
		{
			SYS_LOG_WRN("register error");
		}
	}

	if(!get_item_by_tag(buffer, tmp, DEVICE_ID_TAG, 1))
	{
		memcpy(reg_info->device_sn,tmp,strlen(tmp));
		nvram_config_set("SN_NUM", reg_info->device_sn, MAX_SN_INFO_LEN);
	}

	if(!get_item_by_tag(buffer, tmp, BT_MAC_TAG, 1))
	{
		memcpy(reg_info->bt_mac_addr, tmp, strlen(tmp));
		nvram_config_set("BT_MAC", reg_info->bt_mac_addr, MAC_ADDR_LEN);
	}

	if(!get_item_by_tag(buffer, tmp, DEVICE_TYPE_TAG, 1))
	{
		nvram_config_set("DEV_TYPE", tmp, strlen(tmp));
	}

	if(!get_item_by_tag(buffer, tmp, APPID_TAG, 1))
	{
		memcpy(reg_info->bt_mac_addr, tmp, strlen(tmp));
		nvram_config_set("APP_ID", tmp, strlen(tmp));
	}

	k_sem_give(&reg_info->response_sem);

exit:
	if(tmp != NULL)
	{
		app_mem_free(tmp);
	}
	return 0;
}
#endif
int device_authentication(void)
{
	io_stream_t  netstream = NULL;
#ifdef CONFIG_NET_STREAM
	struct request_info request;
#endif
	int chip_id;
	int ret = 0;
	u16_t len = 0;
	u8_t temp[6] = {0};
	u8_t sign[20] = {0};
	u8_t sign_hex[41] = {0};

	reg_info = app_mem_malloc(sizeof(struct register_info));
	if(reg_info == NULL)
	{
		//SYS_LOG_ERR("reg_info mem_malloc failed , need %ld bytes mem",sizeof(struct register_info));
		ret = -1;
		goto exit;
	}

	if(nvram_config_get("SN_NUM", reg_info->device_sn, MAX_SN_INFO_LEN) > 0)
	{
		ret = 0;
		goto exit;
	}

	os_sem_init(&reg_info->response_sem, 0, 1);

	if(get_wifi_mac_addr(temp,MAC_ADDR_LEN))
	{
		snprintf(reg_info->mac_addr, MAC_ADDR_LEN, DEFAULT_MAC);
	}
	else
	{
		HEXTOSTR(temp, sizeof(temp),reg_info->mac_addr);
	}

	if(get_XY_coordinate(&chip_id))
	{
		snprintf(reg_info->chip_id, CHIP_ID_LEN, DEFAULT_CHIPID);
	}
	else
	{
		snprintf(reg_info->chip_id, CHIP_ID_LEN, "%d-%d", ((chip_id>>16)&0x00ff), ((chip_id)&0x00ff));
	}

	if(nvram_config_get("AUTH_SERVER", reg_info->auth_server, URL_BUFF_LEN) <= 0) {
		snprintf(reg_info->auth_server, URL_BUFF_LEN, "%s" , "kiosk.athenamuses.cn");
	}

	if(nvram_config_get("AUTH_AGENT_ID", reg_info->auth_agent_id, 32) <= 0) {
		snprintf(reg_info->auth_agent_id, DATA_BUFF_LEN, "%s" , "wxde6d2a6f1f4d15f3");
	}

	memset(temp, 0, sizeof(temp));
	if(nvram_config_get("AUTH_PORT", temp, sizeof(temp)) <= 0) {
		reg_info->auth_server_port = 18899;
	} else {
		reg_info->auth_server_port = atoi(temp);
	}

	len = snprintf(reg_info->data_buff, DATA_BUFF_LEN, DEVICE_REGISTE_POST_PARAM,
					reg_info->auth_agent_id, 
					reg_info->mac_addr,
					reg_info->chip_id);

	AH((unsigned char *) reg_info->data_buff, len, sign);

	HEXTOSTR(sign, 20, (char*) sign_hex);

	snprintf(reg_info->url_buff, URL_BUFF_LEN, DEVICE_REGISTE_SPEC,
						reg_info->auth_server,
						reg_info->auth_server_port);

	snprintf(reg_info->data_buff, DATA_BUFF_LEN, get_device_req, sign_hex, len, 
				reg_info->auth_server,
				reg_info->auth_server_port,
				reg_info->auth_agent_id, 
				reg_info->mac_addr,
				reg_info->chip_id);

	SYS_LOG_DBG("http_head =%s",reg_info->data_buff);

#ifdef CONFIG_NET_STREAM
	memset(&request, 0, sizeof(struct request_info));
	request.url = reg_info->url_buff;
	request.http_head = (char *)reg_info->data_buff;
	request.receive_cb = device_authentication_response;
	request.send_post_immediately = true;

	netstream = stream_create(TYPE_NET_STREAM, &request);
#endif

	if (netstream == NULL) {
		SYS_LOG_ERR("stream create failed \n");
		ret = -1;
		goto exit;
	}

	ret = stream_open(netstream, MODE_OUT);

	if (ret) {
		SYS_LOG_ERR("stream open failed \n");
		stream_destroy(netstream);
		ret = -1;
		goto exit;
	}

	if(k_sem_take(&reg_info->response_sem, K_MSEC(10000)))
	{
		SYS_LOG_ERR("register uuid timeout");
		ret = -1;
	}

	stream_close(netstream);

	stream_destroy(netstream);

exit:
	if(reg_info != NULL)
	{
		app_mem_free(reg_info);
		reg_info = NULL;
	}
	return ret ;
}
#ifdef CONFIG_NET_STREAM
static unsigned int memcpy_ext_without_trans(void * d, const void * s, size_t n)
{
	/* attempt word-sized copying only if buffers have identical alignment */
	/*extend*/
	unsigned int filter_size = 0;
	size_t need_cpy_size = n;
	/*end*/

	unsigned char *d_byte = (unsigned char *)d;
	const unsigned char *s_byte = (const unsigned char *)s;

	if ((((unsigned int)d ^ (unsigned int)s_byte) & 0x3) == 0) {

		/* do byte-sized copying until word-aligned or finished */

		while (((unsigned int)d_byte) & 0x3) {
			if (n == 0) {
				return (unsigned int)d;
			}

			/*extend*/
			if(((*s_byte) == '\\') && ((*(s_byte+1)) != '\\'))
			{
				s_byte++;
				n--;
				filter_size++;
				continue;
			}
			/*end*/

			*(d_byte++) = *(s_byte++);
			n--;
		};
	}

	/* do byte-sized copying until finished */

	while (n > 0) {

		/*extend*/
		if(((*s_byte) == '\\') && ((*(s_byte+1)) != '\\'))
		{
			s_byte++;
			n--;
			filter_size++;
			continue;
		}
		/*end*/

		*(d_byte++) = *(s_byte++);
		n--;
	}

	//return d;
	return need_cpy_size - filter_size;
}

static int get_service_lisence_response(char * buffer , int len)
{
	char* begin_flag = NULL;
	char* end_flag = NULL;
	if(reg_info == NULL)
	{
		return -1;
	}
	begin_flag = strstr(buffer, LICENSE_TAG);
	if(begin_flag == NULL)
	{
		goto exit;
	}
	begin_flag += strlen(LICENSE_TAG);
	end_flag = strstr(begin_flag, LICENSE_END_TAG);
	if(end_flag == NULL)
	{
		goto exit;
	}
	len = (u32_t)end_flag - (u32_t)begin_flag;

	memcpy_ext_without_trans(reg_info->result, begin_flag, len);
	k_sem_give(&reg_info->response_sem);
	return 0;

exit:
	k_sem_give(&reg_info->response_sem);
	reg_info->result = NULL;
	return -1;
}
#endif

int get_service_lisence(const struct customer_info * customer, char * service_type, char * lisence)
{
	io_stream_t  netstream = NULL;
#ifdef CONFIG_NET_STREAM
	struct request_info request;
#endif
	int ret = 0;
	u32_t len = 0;
	u8_t sign[20] = {0};
	u8_t sign_hex[41] = {0};

	if(customer == NULL)
	{
		SYS_LOG_ERR("customer info is null");
		ret = -1;
		goto exit;
	}

	reg_info = app_mem_malloc(sizeof(struct register_info));
	if(reg_info == NULL)
	{
		//SYS_LOG_ERR("reg_info mem_malloc failed , need %ld bytes mem",sizeof(struct register_info));
		ret = -1;
		goto exit;
	}

	os_sem_init(&reg_info->response_sem, 0, 1);

	if(get_sair_client_id(reg_info->device_sn))
	{
		ret = -1;
		goto exit;
	}

	len = snprintf(reg_info->data_buff, DATA_BUFF_LEN, LICENSE_POST_PARAM,
				customer->customer_id,
				customer->token,
				reg_info->device_sn,
				service_type);

	AH((unsigned char *) reg_info->data_buff, len, sign);

	HEXTOSTR(sign, 20, (char*) sign_hex);

	snprintf(reg_info->url_buff, URL_BUFF_LEN, LICENSE_SPEC,
						customer->server_addr,
						customer->server_port,
						sign_hex);

	snprintf(reg_info->data_buff, DATA_BUFF_LEN, roboo_get_license_req, sign_hex, len,
				customer->server_addr,
				customer->server_port,
				customer->customer_id,
				customer->token,
				reg_info->device_sn,
				service_type);

	SYS_LOG_DBG("http_head=%s",reg_info->data_buff);

#ifdef CONFIG_NET_STREAM
	memset(&request, 0, sizeof(struct request_info));
	request.url = reg_info->url_buff;
	request.http_head = (char *)reg_info->data_buff;
	request.receive_cb = get_service_lisence_response;
	request.send_post_immediately = true;

	netstream = stream_create(TYPE_NET_STREAM, &request);
#endif

	if (netstream == NULL) {
		SYS_LOG_ERR("stream create failed \n");
		ret = -1;
		goto exit;
	}

	reg_info->result = lisence;

	ret = stream_open(netstream, MODE_OUT);

	if (ret) {
		SYS_LOG_ERR("stream open failed \n");
		stream_destroy(netstream);
		ret = -1;
		goto exit;
	}

	if(k_sem_take(&reg_info->response_sem, K_MSEC(10000)))
	{
		SYS_LOG_ERR("register uuid timeout");
		ret = -1;
	}
	else
	{
		if(reg_info->result == NULL)
		{
			ret = -1;
		}
	}

	stream_close(netstream);

	stream_destroy(netstream);

exit:
	if(reg_info != NULL)
	{
		app_mem_free(reg_info);
		reg_info = NULL;
	}
	return ret ;
}