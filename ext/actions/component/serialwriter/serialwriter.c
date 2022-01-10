/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file serial writer interface
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <stdio.h>
#include <init.h>
#include <string.h>
#include <shell/shell.h>
#include <nvram_config.h>
#include <drivers/console/uart_pipe.h>
#ifdef CONFIG_MBEDTLS
#include <mbedtls/md5.h>
#endif

#include "system_config.h"

extern void __stdout_hook_install(int (*fn)(int));
extern void __printk_hook_install(int (*fn)(int));

#define ASW_READY        "ActionsSerialReady"
#define ASW_WRITE        "ActionsSerialWrite"
#define ASW_SN           " SN=\""
#define ASW_BTMAC        " BTMAC=\""
#define ASW_APP_ID       " APPID=\""
#define ASW_PUBKEY       " PUBKEY=\""
#define ASW_OK           "ActionsSerialWriteOK"
#define ASW_FAIL         "ActionsSerialWriteFail%x"
#define ASW_RETRY        "\x03\x0D"
#define ASW_MD5          "#md5#"

#define SERIAL_WRITER ASW_WRITE

#define MAX_ASW_BUFFER_SIZE 160

struct serial_writer_info {
	u8_t asw_buff[MAX_ASW_BUFFER_SIZE];
};

struct serial_writer_info *serial_writer;

static char *write_sn(char *matched_str)
{
    /*
     * match write sn command, for example:
     *   ActionsSerialWrite SN="ACTS3503008_16"
     */
	int ret = -1, len;
	char *sn, *sn_end, *btmac;
	char *end = NULL;
	char temp[32];

	/* extract sn */
	sn = matched_str + strlen(ASW_SN);
	end = strchr(sn, '\"');
	if (end) {
		end[0] = '\0';
	}

	matched_str = strstr(sn, "#");
	if (matched_str != NULL) {

		sn = matched_str + 1;
		sn_end = strchr(sn, '#');
		if (sn_end != NULL) {
			sn_end[0] = 0;
			len = strlen(sn);

			if (len < 1 || len > sizeof(temp))
				return NULL;

			/* write sn */
		#ifdef CONFIG_NVRAM_CONFIG
			ret = nvram_config_set_factory(CFG_SN_INFO, sn, len);
		#endif
			if (ret)
				goto exit;

			memset(temp, 0, sizeof(temp));
		#ifdef CONFIG_NVRAM_CONFIG
			ret = nvram_config_get(CFG_SN_INFO, temp, len);
		#endif
			if (ret > 0 && !memcmp(temp, sn, len)) {
				ret = 0;
			} else {
				ret = -1;
				goto exit;
			}
		}

		btmac = sn_end + 1;
		len = strlen(btmac);
		if (len < 1 || len > sizeof(temp))
			return NULL;

		/* write mac */
	#ifdef CONFIG_NVRAM_CONFIG
		ret = nvram_config_set_factory(CFG_BT_MAC, btmac, len);
	#endif
		if (ret)
			goto exit;

		memset(temp, 0, sizeof(temp));
	#ifdef CONFIG_NVRAM_CONFIG
		ret = nvram_config_get(CFG_BT_MAC, temp, len);
	#endif
		if (ret > 0 && !memcmp(temp, btmac, len)) {
			ret = 0;
		} else {
			ret = -1;
			goto exit;
		}
	} else {

		len = strlen(sn);
		if (len < 1)
			return NULL;

		/* write sn */
	#ifdef CONFIG_NVRAM_CONFIG
		ret = nvram_config_set_factory(CFG_SN_INFO, sn, len);
	#endif
		if (ret)
			goto exit;

		memset(temp, 0, sizeof(temp));
	#ifdef CONFIG_NVRAM_CONFIG
		ret = nvram_config_get(CFG_SN_INFO, temp, sizeof(temp));
	#endif
		if (ret > 0 && !memcmp(temp, sn, len)) {
			ret = 0;
		}
	}

exit:
	if (ret) {
		return NULL;
	} else {
		return end + 1;
	}
}

static char *write_config(char *matched_str, char *cmd, char *cfg)
{
	/*
	 * match write sn command, for example:
	 *   ActionsSerialWrite BTMAC="a1b2c3d4e501"
	 */
	int ret = -1, len;
	char *mac;
	char *end = NULL;
	char temp[32];

	/* extract sn */
	mac = matched_str + strlen(cmd);

	end = strchr(mac, '\"');
	end[0] = '\0';
	len = strlen(mac);

	if (len < 1 || end == NULL)
	return NULL;

	/* write mac addr */
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_set_factory(cfg, mac, len);
#endif
	if (!ret) {
		memset(temp, 0, sizeof(temp));
	#ifdef CONFIG_NVRAM_CONFIG
		ret = nvram_config_get_factory(cfg, temp, sizeof(temp));
	#endif
		if (ret > 0 && !memcmp(temp, mac, len)) {
			return end + 1;
		}
	}
	return NULL;
}
static bool check_serial_data_md5(u8_t *buf, u8_t *md5)
{
	u8_t md5_out[16];
	u8_t md5_text_out[32];

	mbedtls_md5(buf, strlen(buf), md5_out);
	HEXTOSTR(md5_out, 16, md5_text_out);
	if (!memcmp(md5_text_out, md5, sizeof(md5_text_out))) {
		return true;
	} else {
		return false;
	}

}
static u8_t *asw_recv_cb(u8_t *buf, size_t *off)
{
	u8_t *matched_str;
	u8_t *md5_buff = NULL;
	u8_t *bak_buf = buf;
	int ret = 0;
	u8_t result[32];

	if (!buf || !off)
		goto exit;

	/* command must be end with '\r' */
	if (buf[*off - 1] != '\r') {
		goto exit;
	}

	buf[*off - 1] = '\0';
	*off = 0;

	/* shake hands with pc */
	if (strcmp(buf, ASW_WRITE) == 0) {
		uart_pipe_send(ASW_READY, sizeof(ASW_READY));
		goto exit;
	}

    /* try again? */
	matched_str = strstr(buf, ASW_RETRY);
	if (matched_str != NULL) {
		goto exit;
	}

	matched_str = strstr(buf, ASW_WRITE);

	if (matched_str == NULL) {
		goto exit;
	}

	md5_buff = strstr(matched_str, ASW_MD5));
	if (md5_buff != NULL) {
		md5_buff[0] = 0;
		matched_str += strlen(ASW_WRITE) + 1;
		md5_buff += strlen(ASW_MD5);
		if (!check_serial_data_md5(matched_str, md5_buff)) {
			ret = 5;
			goto result;
		}
	}

	ret = -1;
	while (buf != NULL) {

		if ((matched_str = strstr(buf, ASW_SN)) != NULL) {
			buf = write_sn(matched_str);
			ret = 0;
		} else if ((matched_str = strstr(buf, ASW_BTMAC)) != NULL) {
			buf = write_config(matched_str, ASW_BTMAC, CFG_BT_MAC);
			ret = 0;
		} else if ((matched_str = strstr(buf, ASW_APP_ID)) != NULL) {
			buf = write_config(matched_str, ASW_APP_ID, CFG_SAIR_AGENT_ID);
			ret = 0;
		} else if ((matched_str = strstr(buf, ASW_PUBKEY)) != NULL) {
			buf = write_config(matched_str, ASW_PUBKEY, CFG_SAIR_TOKEN);
			ret = 0;
		} else {
			ret = 0;
			break;
		}
	}
result:
	if (ret) {
		/* fail */
		snprintf(result, 32, ASW_FAIL, ret);
		uart_pipe_send(result, strlen(result));
	} else {
	   /* success */
	   uart_pipe_send(ASW_OK, sizeof(ASW_OK));
	}
exit:
	return bak_buf;
}

static int __dummy_console_out(int c)
{
	return c;
}

bool serial_writer_init(void)
{
	if (serial_writer != NULL) {
		mem_free(serial_writer);
		serial_writer = NULL;
	}

	serial_writer = mem_malloc(sizeof(struct serial_writer_info));

	if (serial_writer == NULL) {
		return false;
	}

	/* uninstall default uart console hook */
	__stdout_hook_install(__dummy_console_out);
	__printk_hook_install(__dummy_console_out);

    /* register uart pipe */
	uart_pipe_register(serial_writer->asw_buff, MAX_ASW_BUFFER_SIZE, asw_recv_cb);

	uart_pipe_send(ASW_READY, sizeof(ASW_READY));

	return true;
}

static int serial_writer_moniter(int argc, char *argv[])
{
	if (strcmp(argv[0], ASW_WRITE) == 0) {
		printk("Enter  sn mode\r\n");
		serial_writer_init();
	}
	return 0;
}

int serial_writer_moniter_init(struct device *unused)
{
	ARG_UNUSED(unused);
	shell_register_app_cmd_handler(serial_writer_moniter);
	return 0;
}

SYS_INIT(serial_writer_moniter_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);


