/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_tx_mp_manager.c
*/

#include "ap_autotest_rf_tx.h"

void mp_core_load(void)
{
	install_fcc_drv my_install_fcc_drv;
	u32_t fcc_entry_para;

	/* boot to fccdrv entry */
	my_install_fcc_drv = (install_fcc_drv) FCC_DRV_INSTALL_ENTRY_ADDR;
	fcc_entry_para = 0;
	fcc_entry_para |= (1 << FCC_WORD_MODE_SHIFT);
	my_install_fcc_drv(fcc_entry_para);
}

void mp_task_tx_start(unsigned int channel, unsigned int power, unsigned int packet_counter)
{
	act_test_bt_packet_send_process(channel, power, packet_counter);
}

