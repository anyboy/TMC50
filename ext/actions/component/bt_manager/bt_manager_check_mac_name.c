/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager genarate bt mac and name.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hex_str.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include <sys_event.h>
#include <property_manager.h>

//#define CONFIG_AUDIO_RANDOM 1

#ifdef CONFIG_AUDIO_RANDOM
#ifdef CONFIG_AUDIO_IN
#include <audio_policy.h>
#include <audio_system.h>
#include <audio_hal.h>
#endif
#else
#include <random.h>
#endif

#define MAC_STR_LEN		(12+1)	/* 12(len) + 1(NULL) */
#define BT_NAME_LEN		(32+1)	/* 32(len) + 1(NULL) */
#define BT_PRE_NAME_LEN	(20+1)	/* 10(len) + 1(NULL) */


#ifdef CONFIG_PROPERTY
#ifdef CONFIG_AUDIO_IN
#define AUDIO_BUF_SIZE			512
#define MIC_SAMPLE_TIMES		20

#ifdef CONFIG_AUDIO_RANDOM
typedef struct {
	u32_t samples;
	u32_t rand;
	u8_t *buffer;
	u16_t buffer_size;
} mic_randomizer_t;

static int mic_in_stream_handle(void *callback_data, u32_t reason)
{
	int  i;
	mic_randomizer_t *p = (mic_randomizer_t *)callback_data;

	for (i = 0; i < p->buffer_size / 2; i++) {
		p->rand = (p->rand*131) + p->buffer[i];
	}

	if (p->samples > 0) {
		p->samples--;
	}
	return 0;
}

static u32_t mic_gen_rand(void)
{
	mic_randomizer_t  mic_randomizer;
	audio_in_init_param_t init_param;
	void *ain_handle;
	void *sample_buf;

	memset(&init_param, 0, sizeof(audio_in_init_param_t));

	sample_buf = mem_malloc(AUDIO_BUF_SIZE);
	__ASSERT_NO_MSG(sample_buf != NULL);

	init_param.channel_type = audio_policy_get_record_channel_type(AUDIO_STREAM_VOICE);
	init_param.sample_rate = 16;
	init_param.reload_addr = sample_buf;
	init_param.reload_len = AUDIO_BUF_SIZE;
	init_param.adc_gain = 0;
	init_param.data_mode = 0;
	init_param.input_gain = 2;
	init_param.dma_reload = 1;
	init_param.left_input = audio_policy_get_record_left_channel_id(AUDIO_STREAM_VOICE);
	init_param.right_input = audio_policy_get_record_left_channel_id(AUDIO_STREAM_VOICE);

	init_param.callback = mic_in_stream_handle;
	init_param.callback_data = (void *)&mic_randomizer;

	mic_randomizer.samples = MIC_SAMPLE_TIMES;
	mic_randomizer.buffer = sample_buf;
	mic_randomizer.buffer_size = AUDIO_BUF_SIZE;

	ain_handle = hal_ain_channel_open(&init_param);
	if (!ain_handle) {
		SYS_LOG_ERR("ain open faild");
		return k_uptime_get_32();
	}

	hal_ain_channel_start(ain_handle);

	while (mic_randomizer.samples != 0) {
		os_sleep(1);
	}

	hal_ain_channel_stop(ain_handle);
	hal_ain_channel_close(ain_handle);

	mem_free(sample_buf);

	return mic_randomizer.rand;
}
#endif
#endif
#endif

static void bt_manager_bt_name(u8_t *mac_str)
{
#ifdef CONFIG_PROPERTY
	u8_t bt_name[BT_NAME_LEN];
	u8_t bt_pre_name[BT_PRE_NAME_LEN];
	u8_t len = 0;
	u8_t *cfg_name = CFG_BT_NAME;
	int ret;

try_ble_name:
	memset(bt_name, 0, BT_NAME_LEN);
	ret = property_get(cfg_name, bt_name, (BT_NAME_LEN-1));
	if (ret < 0) {
		SYS_LOG_WRN("ret: %d", ret);

		memset(bt_pre_name, 0, BT_PRE_NAME_LEN);
		ret = property_get(CFG_BT_PRE_NAME, bt_pre_name, (BT_PRE_NAME_LEN-1));
		if (ret > 0) {
			len = strlen(bt_pre_name);
			memcpy(bt_name, bt_pre_name, len);
		}
		memcpy(&bt_name[len], mac_str, strlen(mac_str));
		len += strlen(mac_str);

		property_set_factory(cfg_name, bt_name, len);
	}

	SYS_LOG_INF("%s: %s", cfg_name, bt_name);

	if (cfg_name == (u8_t *)CFG_BT_NAME) {
		cfg_name = CFG_BLE_NAME;
		goto try_ble_name;
	}
#endif
}


static void bt_manager_bt_mac(u8_t *mac_str)
{
#ifdef CONFIG_PROPERTY
	u8_t mac[6], i;
	int ret;
	u32_t rand_val;

	ret = property_get(CFG_BT_MAC, mac_str, (MAC_STR_LEN-1));
	if (ret < (MAC_STR_LEN-1)) {
		SYS_LOG_WRN("ret: %d", ret);
#ifdef CONFIG_AUDIO_RANDOM
		rand_val = mic_gen_rand();
#else
		rand_val = sys_rand32_get();
#endif
		bt_manager_config_set_pre_bt_mac(mac);
		memcpy(&mac[3], &rand_val, 3);

		for (i = 3; i < 6; i++) {
			if (mac[i] == 0)
				mac[i] = 0x01;
		}

		hex_to_str(mac_str, mac, 6);

		property_set_factory(CFG_BT_MAC, mac_str, (MAC_STR_LEN-1));
	} else {
		str_to_hex(mac, mac_str, 6);
		bt_manager_updata_pre_bt_mac(mac);
	}
#endif
	SYS_LOG_INF("BT MAC: %s", mac_str);
}

void bt_manager_check_mac_name(void)
{
	u8_t mac_str[MAC_STR_LEN];

	memset(mac_str, 0, MAC_STR_LEN);
	bt_manager_bt_mac(mac_str);
	bt_manager_bt_name(mac_str);
}

