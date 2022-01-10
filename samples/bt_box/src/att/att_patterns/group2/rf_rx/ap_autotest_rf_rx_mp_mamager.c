/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_mp_manager.c
*/

#include "ap_autotest_rf_rx.h"

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

/*packet_counter:4 ~ 16*/
void mp_task_rx_start(u8_t channel, u32_t packet_counter)
{
	struct mp_test_stru *mp_test = &g_mp_test;
	mp_rx_report_t rx_report;
	u32_t cur_total_bits;
	int i;

    mp_test->ber_val = 0;
    mp_test->total_bits = 0;

	for (i = 0; i < CFO_RESULT_NUM; i++) {
		if (act_test_bt_packet_receive_process(channel, packet_counter) != 0) {
			printk("packet_receive fail!\n");
			mp_test->cfo_val[i] = INVALID_CFO_VAL;
			mp_test->rssi_val[i] = INVALID_RSSI_VAL;
			continue;
		}
		act_test_bt_rx_report(&rx_report);
		cur_total_bits = rx_report.rx_packet_cnts * rx_report.packet_len * 8;
		printk("rx report:cfo %d, rssi %d, total bits %d, err bits %d\n", 
			rx_report.cfo_index, rx_report.rx_rssi, cur_total_bits, rx_report.total_error_bits);
		mp_test->cfo_val[i] = 0 - rx_report.cfo_index; /*sign reverse*/
		mp_test->rssi_val[i] = rx_report.rx_rssi;
		mp_test->total_bits += cur_total_bits;
     	mp_test->ber_val += rx_report.total_error_bits;
	}
}

