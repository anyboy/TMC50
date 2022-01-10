/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_loop.c
*/

#include "ap_autotest.h"

#include "soc_boot.h"
#include "soc_pm.h"
#include "sys_manager.h"
#include "fw_version.h"

//when receive test id is 0xffff, DUT must stop send any packet to ATT Tool
static s32_t act_test_read_testid(void)
{
    int ret_val = 0;
    u8_t *cmd_data;

    {
        cmd_data = STUB_ATT_RW_TEMP_BUFFER;

        //The timeout is in seconds, depending on the time used by the longest test item
        cmd_data[6] = 20;  // Test item normal work timeout
        cmd_data[7] = 0;
        cmd_data[8] = 0;
        cmd_data[9] = 0;

        ret_val = att_write_data(STUB_CMD_ATT_GET_TEST_ID, 4, STUB_ATT_RW_TEMP_BUFFER);

        memset(STUB_ATT_RW_TEMP_BUFFER, 0x00, 20);
        g_test_info.test_id = 0;
        g_test_info.arg_len = 0;

        if (ret_val == 0)
        {
            ret_val = att_read_data(STUB_CMD_ATT_GET_TEST_ID, 4, STUB_ATT_RW_TEMP_BUFFER);

            if (ret_val == 0)
            {
                cmd_data = STUB_ATT_RW_TEMP_BUFFER;

                g_test_info.test_id = (cmd_data[6] | (cmd_data[7] << 8));

                g_test_info.arg_len = (cmd_data[8] | (cmd_data[9] << 8));
                //TESTID is TESTID_ID_QUIT, need't reply ACK
                if (g_test_info.test_id != TESTID_ID_QUIT)
                {
                    //reply ACK
                    ret_val = att_write_data(STUB_CMD_ATT_ACK, 0, STUB_ATT_RW_TEMP_BUFFER);
                }
            }
        }

        if (g_test_info.test_id == 0)
        {
            return -1;
        }
    }

    return ret_val;
}

static const struct att_code_info att_patterns_list[] =
{
	{ TESTID_MODIFY_BTNAME,    0, 0, "p01_bta.bin" },
	{ TESTID_MODIFY_BLENAME,   0, 0, "p01_bta.bin" },
	{ TESTID_MODIFY_BTADDR,    0, 0, "p01_bta.bin" },
	{ TESTID_MODIFY_BTADDR2,   0, 0, "p01_bta.bin" },

	{ TESTID_READ_BTNAME,      0, 0, "p02_bta.bin" },
	{ TESTID_READ_BTADDR,      0, 0, "p02_bta.bin" },

	{ TESTID_BQBMODE,          0, 0, "p03_bqb.bin" },
	{ TESTID_FCCMODE,          0, 1, "p04_fcc.bin" },

	{ TESTID_MP_TEST,          0, 1, "p05_rx.bin" },
	{ TESTID_MP_READ_TEST,     0, 1, "p05_rx.bin" },

	{ TESTID_PWR_TEST,         0, 1, "p06_tx.bin" },

	{ TESTID_BT_TEST,          0, 0, "p07_btl.bin" },
	{ TESTID_LED_TEST,         0, 0, "p10_led.bin" },
	{ TESTID_KEY_TEST,         0, 0, "p11_key.bin" },
	{ TESTID_CAP_CTR_START,    0, 0, "p12_bat.bin" },
	{ TESTID_CAP_TEST,         0, 0, "p12_bat.bin" },
	{ TESTID_EXIT_MODE,        0, 0, "p13_exit.bin" },

	{ TESTID_GPIO_TEST,        0, 0, "p18_gpio.bin" },
	{ TESTID_AGING_PB_START,   0, 0, "p19_age.bin" },
	{ TESTID_AGING_PB_CHECK,   0, 0, "p19_age.bin" },
	{ TESTID_LINEIN_CH_TEST,   0, 0, "p20_adc.bin" },
	{ TESTID_MIC_CH_TEST,      0, 0, "p20_adc.bin" },
	{ TESTID_FM_CH_TEST,       0, 0, "p20_adc.bin" },

	{ TESTID_USER_RESERVED0,   0, 0, "p40_user.bin" },
	{ TESTID_USER_RESERVED1,   0, 0, "p41_user.bin" },
	{ TESTID_USER_RESERVED2,   0, 0, "p42_user.bin" },
	{ TESTID_USER_RESERVED3,   0, 0, "p43_user.bin" },
	{ TESTID_USER_RESERVED4,   0, 0, "p44_user.bin" },
	{ TESTID_USER_RESERVED5,   0, 0, "p45_user.bin" },
	{ TESTID_USER_RESERVED6,   0, 0, "p46_user.bin" },
	{ TESTID_USER_RESERVED7,   0, 0, "p47_user.bin" },
};

static const struct att_drv_code_info att_patterns_drvs_list[] =
{
	{ "p04_fcc.bin", "fccdrv.bin" },
	{ "p05_rx.bin", "mpdrv.bin" },
	{ "p06_tx.bin", "mpdrv.bin" },
};

static const struct att_code_info* match_testid(int testid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(att_patterns_list); i++) {
		if (att_patterns_list[i].testid == testid)
			return &att_patterns_list[i];
	}

	return NULL;
}

static int match_driver(const char *pt_name, char *driver_name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(att_patterns_drvs_list); i++) {
		if (strncmp(att_patterns_drvs_list[i].pt_name, pt_name, ATF_MAX_SUB_FILENAME_LENGTH) == 0) {
			memcpy(driver_name, att_patterns_drvs_list[i].drv_name, sizeof(att_patterns_drvs_list[i].drv_name));
			return 0;
		}
	}

	return -1;
}

static const u8_t error_msg_ansi[] = "atf build time NOT equal fw build time";

static s32_t act_test_check_fw_buildtime(void)
{
	int ret_val = 0;
	struct fw_version *p_fw_ver;
	char *p_ver_name, *p_buildtime;
	int len, i;

	p_fw_ver = (struct fw_version *)soc_boot_get_fw_ver_addr();
	if (!p_fw_ver) {
		SYS_LOG_WRN("get_fw_ver_addr fail\n");
		ret_val = -1;
		goto check_err_ret;
	} else {
		p_ver_name = p_fw_ver->version_name;
		p_buildtime = NULL;
		len = strlen(p_ver_name);
		for (i = 0; i < len; i++) {
			if (p_ver_name[i] == '_') {
				p_buildtime = &p_ver_name[i+1]; /*point to build time*/
				break;
			}
		}
		if (!p_buildtime) {
			SYS_LOG_WRN("find _ charactor before build tiem fail, fw version format invalid\n");
			ret_val = -1;
			goto check_err_ret;
		}
	}

	ret_val = read_atf_file((u8_t *)&g_atf_head, sizeof(g_atf_head), 0, sizeof(g_atf_head));
	if (ret_val < (int) sizeof(g_atf_head)) {
		SYS_LOG_WRN("read atf_head fail\n");
		ret_val = -1;
		goto check_err_ret;
	} else {
		ret_val = 0;
	}

	SYS_LOG_INF("ver cmp : atf %s, fw %s\n", g_atf_head.build_time, p_buildtime);
	if (strcmp(g_atf_head.build_time, p_buildtime) != 0) {
		SYS_LOG_WRN("atf build time NOT equal fw build time, fail\n");
		att_log_to_pc(STUB_CMD_ATT_PRINT_ERROR, error_msg_ansi, STUB_ATT_RW_TEMP_BUFFER);

		ret_val = -1;
		goto check_err_ret;
	}

	SYS_LOG_INF("atf sub files : %d\n", g_atf_head.file_total);

check_err_ret:

	return ret_val;
}

static s32_t act_test_start_test(void)
{
	int ret_val = 0;
	//u8_t sdk_version[4];
	att_start_arg_t *att_start_arg;
	int try_count = 0;

att_start:

	att_start_arg = (att_start_arg_t *) STUB_ATT_RW_TEMP_BUFFER;

	memset(att_start_arg, 0x00, sizeof(att_start_arg_t));

	att_start_arg->dut_connect_mode = DUT_CONNECT_MODE_UDA;
	att_start_arg->dut_work_mode = DUT_WORK_MODE_NORMAL;
	att_start_arg->timeout = 5; /* get next test id timeout */
	att_start_arg->reserved = 0;

	ret_val = att_write_data(STUB_CMD_ATT_START, 32, STUB_ATT_RW_TEMP_BUFFER);
	if (ret_val == 0)
	{
		ret_val = att_read_data(STUB_CMD_ATT_ACK, 0, STUB_ATT_RW_TEMP_BUFFER);
		/* USB STUB Connecting here will return an error, Need't quit here */
		ret_val = 0;
	}
	else
	{
		try_count++;
		if (try_count <= 10) {
			k_sleep(10);
			goto att_start;
		} else {
			SYS_LOG_INF("att start fail\n");
			return -1;
		}
	}

	SYS_LOG_INF("att start ok\n");

	return ret_val;
}

static bool act_test_report_prod_card_result(bool ret_val)
{
	return_result_t *return_data = (return_result_t *) STUB_ATT_RETURN_DATA_BUFFER;
	u16_t trans_bytes = 0;
	
	return_data->test_id = TESTID_CARD_PRODUCT_TEST;
	return_data->test_result = ret_val;

	//add terminator
	return_data->return_arg[trans_bytes++] = 0x0000;

	//four-byte alignment
	if ((trans_bytes % 2) != 0)
		return_data->return_arg[trans_bytes++] = 0x0000;

	return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

static s32_t act_test_hold_test(void)
{
	int ret_val = 0;
	att_hold_arg_t *att_hold_arg;

	att_hold_arg = (att_hold_arg_t *) STUB_ATT_RW_TEMP_BUFFER;

	memset(att_hold_arg, 0x00, sizeof(att_hold_arg_t));

	att_hold_arg->timeout = 20; /* product card timeout */

	ret_val = att_write_data(STUB_CMD_ATT_HOLD, 32, STUB_ATT_RW_TEMP_BUFFER);
	if (ret_val == 0) {
		ret_val = att_read_data(STUB_CMD_ATT_ACK, 0, STUB_ATT_RW_TEMP_BUFFER);
		/* USB STUB Connecting here will return an error, Need't quit here */
		ret_val = 0;
	}

	return ret_val;
}

int test_dispatch(void)
{
	int ret_val = 0;
	const struct att_code_info *cur_att_code_info;
	int (*att_code_entry)(struct att_env_var *para);
	bool is_first_pattern = true;
	const boot_info_t *p_boot_info;

	if (act_test_start_test() < 0) {
		return -1;
	}

	while(1) {
		ret_val = act_test_read_testid();
		if (ret_val != 0) {
			os_sleep(100);
			continue;
		}

		if (g_test_info.test_id == TESTID_ID_WAIT) {
			os_sleep(500);
			continue;
		}

		if (g_test_info.test_id == TESTID_ID_QUIT) {
			break;
		}

		SYS_LOG_INF("att test_dispatch : %d\n", g_test_info.test_id);
		
		g_att_env_var.test_id = g_test_info.test_id;
		g_att_env_var.arg_len = g_test_info.arg_len;
		
		if (g_test_info.test_id == TESTID_CARD_PRODUCT_TEST) {
			if (act_test_report_prod_card_result(true) == false) {
				SYS_LOG_INF("att card product fail, quit!\n");
				break;
			}

			/* It is unexpected, but accept for compatibility */
			if (soc_boot_user_get_card_product_att_reboot() == true) {
				SYS_LOG_INF("card product att reboot\n");
				continue;
			}

			/* First card product, avoid secondly card product */
			p_boot_info = soc_boot_get_info();
			if (p_boot_info->prod_card_reboot == 1) {
				SYS_LOG_INF("card product reboot\n");
				continue;
			}

			if (act_test_hold_test() < 0) {
				SYS_LOG_INF("att hold fail, quit!\n");
				break;
			}

			//switch to card product
			os_sleep(10);
			sys_pm_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_GOTO_PROD_CARD_ATT);
		} else {
			if (is_first_pattern == true) {
				ret_val = act_test_check_fw_buildtime();
				if (ret_val < 0) {
					SYS_LOG_INF("check fw buildtime fail, quit!\n");
					break;
				}
			}
		}

		is_first_pattern = false;

		/* load pattern */
		cur_att_code_info = match_testid(g_test_info.test_id);
		if (cur_att_code_info != NULL) {
			if (strncmp(g_test_info.last_pt_name, cur_att_code_info->pt_name, ATF_MAX_SUB_FILENAME_LENGTH) != 0) {
				if (cur_att_code_info->need_drv == 1) {
					char driver_name[13];
					ret_val = match_driver(cur_att_code_info->pt_name, driver_name);
					if (ret_val < 0) {
						SYS_LOG_INF("match_driver fail, quit!\n");
						break;
					}
					ret_val = read_atf_sub_file(NULL, 0x10000, driver_name, 0, -1);
					if (ret_val <= 0) {
						SYS_LOG_INF("read_atf_sub_file driver fail, quit!\n");
						break;
					}
				}

				ret_val = read_atf_sub_file(NULL, 0x10000, cur_att_code_info->pt_name, 0, -1);
				if (ret_val <= 0) {
					SYS_LOG_INF("read_atf_sub_file fail, quit!\n");
					break;
				}

				memcpy(g_test_info.last_pt_name, cur_att_code_info->pt_name, sizeof(g_test_info.last_pt_name));
			}

			att_code_entry = (int (*)(struct att_env_var *))((void *)(g_test_info.run_addr));
			if (att_code_entry != NULL) {
				SYS_LOG_INF("Enter pattern %s in address 0x%x\n", cur_att_code_info->pt_name, g_test_info.run_addr);
				if (att_code_entry(&g_att_env_var) == false) {
					SYS_LOG_INF("att_code_entry return false, quit!\n");
					break;
				}
			}
		} else {
			SYS_LOG_INF("match_testid fail, quit!\n");
			break;
		}
	}

	SYS_LOG_INF("att test end\n");

	return 0;
}

