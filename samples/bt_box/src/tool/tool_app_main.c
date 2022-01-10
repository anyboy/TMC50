/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <adc.h>
#include "tool_app_inner.h"
#include "tts_manager.h"

act_tool_data_t g_tool_data;

#ifdef CONFIG_ATT_SUPPORT_MULTIPLE_DEVICES
/* ascending sequence, and the bigest value should be 880(0x370) or lower */
static const u16_t tool_adc2serial[] = {0, 90, 180, 270, 360, 450, 540, 630, 720, 880};
#endif

static int pc_connect_probe(void)
{
	int pc_type = 0;

	if (stub_get_data(g_tool_data.dev_stub, STUB_CMD_OPEN, &pc_type, 4))
		return 0;

	SYS_LOG_INF("stub type = %d", pc_type);
	return pc_type;
}

#ifdef CONFIG_ATT_SUPPORT_MULTIPLE_DEVICES
static u16_t tool_read_adc(void)
{
/** this routine hopes to read a adc value that had existed, 
 *  and then return immediately.
 */
	u8_t tool_adc_buf[4];
	struct adc_seq_entry tool_adc_entry;
	struct adc_seq_table tool_adc_table;
	struct device *tool_adc_dev = device_get_binding(CONFIG_ADC_0_NAME);

	tool_adc_table.entries = &tool_adc_entry;
	tool_adc_table.num_entries = 1;

	tool_adc_entry.buffer = tool_adc_buf;
	tool_adc_entry.buffer_length = sizeof(tool_adc_buf);
	tool_adc_entry.sampling_delay = 0;
	tool_adc_entry.channel_id = CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_CHAN;

	adc_read(tool_adc_dev, &tool_adc_table);
	return sys_get_le16(tool_adc_entry.buffer);
}

static void get_serial_number(u8_t *num)
{
	u16_t tool_adc_val = tool_read_adc();
	u8_t i;

	for (i = 0; i < ARRAY_SIZE(tool_adc2serial); i++) {
		if (tool_adc_val < tool_adc2serial[i]) {
			*num = i;
			break;
		}
	}

	/* adc_val is too big */
	if (i >= ARRAY_SIZE(tool_adc2serial))
		*num = 0;
}
#endif

static void tool_process_deal(void *p1, void *p2, void *p3)
{
	int timeout_cnt = 0;
	u8_t serial_num = 0;

	SYS_LOG_INF("Enter");

#ifdef CONFIG_ATT_SUPPORT_MULTIPLE_DEVICES
	get_serial_number(&serial_num);
#endif

	if (g_tool_data.dev_stub && g_tool_data.type == STUB_PC_TOOL_ATT_MODE) {
		/* active connection */
	} else {
		g_tool_data.dev_stub = act_stub_open(CONFIG_STUB_DEV_USB_NAME, serial_num);
		if (!g_tool_data.dev_stub) {
			SYS_LOG_ERR("act_stub_open failed");
			goto out_exit;
		}
	}

	while (!g_tool_data.quit) {
		if (g_tool_data.dev_stub && g_tool_data.type == STUB_PC_TOOL_ATT_MODE) {
			/* active connection */
		} else {
			int pc_type = pc_connect_probe();
			g_tool_data.type = pc_type;
			g_tool_data.dev_type = TOOL_DEV_TYPE_USB;
			SYS_LOG_INF("pc_type %d \n", pc_type);
		}

		switch (g_tool_data.type) {
#ifdef CONFIG_TOOL_ASQT
		case STUB_PC_TOOL_ASQT_MODE:
			SYS_LOG_INF("ASQT");
			os_sem_give(&g_tool_data.init_sem);
			tool_asqt_loop();
			break;
#endif

#ifdef CONFIG_TOOL_ASET
		case STUB_PC_TOOL_ASET_EQ_MODE:
			SYS_LOG_INF("ASET");
			os_sem_give(&g_tool_data.init_sem);
			tool_aset_loop();
			break;
#endif

#ifdef CONFIG_TOOL_ECTT
		case STUB_PC_TOOL_ECTT_MODE:
			SYS_LOG_INF("ECTT");
			os_sem_give(&g_tool_data.init_sem);
			tool_ectt_loop();
			break;
#endif

#ifdef CONFIG_ACTIONS_ATT
		case STUB_PC_TOOL_ATT_MODE:
			SYS_LOG_INF("ATT");
		#ifdef CONFIG_PLAYTTS
			/* default ignore tts */
			tts_manager_lock();
		#endif
			os_sem_give(&g_tool_data.init_sem);
			tool_att_loop();
			break;
#endif

		case 0:
			/* sleep 100 ms, then check again */
			os_sleep(100);
			if (timeout_cnt ++ > 10) {
				os_sem_give(&g_tool_data.init_sem);
				g_tool_data.quit = 1;
			}
			break;

		default:
			SYS_LOG_WRN("stub_pc_type = %d", g_tool_data.type);
			break;
		}
	}

	act_stub_close(g_tool_data.dev_stub);
	g_tool_data.dev_stub = NULL;
out_exit:
	os_thread_priority_set(k_current_get(), -1);
	g_tool_data.quited = 1;
	SYS_LOG_INF("Exit");
}

int tool_get_dev_type(void)
{
	return g_tool_data.dev_type;
}

struct device * tool_get_dev_device(void)
{
	return g_tool_data.dev_stub->stub_dev;
}

int tool_init(void)
{
	if (g_tool_data.stack)
		return -EALREADY;

	g_tool_data.stack = app_mem_malloc(CONFIG_TOOL_STACK_SIZE);
	if (!g_tool_data.stack) {
		SYS_LOG_ERR("tool stack mem_malloc failed");
		return -ENOMEM;
	}

	os_sem_init(&g_tool_data.init_sem, 0, 1);

	g_tool_data.quit = 0;
	g_tool_data.quited = 0;

	if (g_tool_data.dev_stub && g_tool_data.type == STUB_PC_TOOL_ATT_MODE) {
		/* active connection */
	} else {
		g_tool_data.type = 0;
	}

	os_thread_create(g_tool_data.stack, CONFIG_TOOL_STACK_SIZE,
			tool_process_deal, NULL, NULL, NULL, 8, 0, OS_NO_WAIT);

	os_sem_take(&g_tool_data.init_sem, OS_FOREVER);

	SYS_LOG_INF("begin trying to connect pc tool");
	return g_tool_data.type;
}

void tool_deinit(void)
{
	if (!g_tool_data.stack)
		return;

	SYS_LOG_WRN("will disconnect pc tool, then exit");

	g_tool_data.quit = 1;
	while (!g_tool_data.quited)
		os_sleep(1);

	app_mem_free(g_tool_data.stack);
	g_tool_data.stack = NULL;

	SYS_LOG_INF("exit now");
}
