/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include <sys_event.h>
#include <btservice_api.h>
#include <dvfs.h>
#include <shell/shell.h>
#include <bluetooth/host_interface.h>
#include <property_manager.h>

#define PTS_TEST_SHELL_MODULE		"pts"

static int pts_connect_acl_a2dp_avrcp(int argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[3];

	memset(info, 0, sizeof(info));
	cnt = btif_br_get_auto_reconnect_info(info, 1);
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 1;
	info[0].hfp = 0;
	info[0].avrcp = 1;
	info[0].hfp_first = 0;
	btif_br_set_auto_reconnect_info(info, 3);
	property_flush(NULL);

	bt_manager_startup_reconnect();
	return 0;
}

static int pts_connect_acl_hfp(int argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[3];

	memset(info, 0, sizeof(info));
	cnt = btif_br_get_auto_reconnect_info(info, 1);
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 0;
	info[0].hfp = 1;
	info[0].avrcp = 0;
	info[0].hfp_first = 1;
	btif_br_set_auto_reconnect_info(info, 3);
	property_flush(NULL);

	bt_manager_startup_reconnect();
	return 0;
}

static int pts_hfp_cmd(int argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts hfp_cmd xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("hfp cmd:%s", cmd);

	/* AT cmd
	 * "ATA"			Answer call
	 * "AT+CHUP"		rejuet call
	 * "ATD1234567;"	Place a Call with the Phone Number Supplied by the HF.
	 * "ATD>1;"			Memory Dialing from the HF.
	 * "AT+BLDN"		Last Number Re-Dial from the HF.
	 * "AT+CHLD=0"	Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call.
	 * "AT+CHLD=x"	refer HFP_v1.7.2.pdf.
	 * "AT+NREC=x"	Noise Reduction and Echo Canceling.
	 * "AT+BVRA=x"	Bluetooth Voice Recognition Activation.
	 * "AT+VTS=x"		Transmit DTMF Codes.
	 * "AT+VGS=x"		Volume Level Synchronization.
	 * "AT+VGM=x"		Volume Level Synchronization.
	 * "AT+CLCC"		List of Current Calls in AG.
	 * "AT+BTRH"		Query Response and Hold Status/Put an Incoming Call on Hold from HF.
	 * "AT+CNUM"		HF query the AG subscriber number.
	 * "AT+BIA="		Bluetooth Indicators Activation.
	 * "AT+COPS?"		Query currently selected Network operator.
	 */

	if (btif_pts_send_hfp_cmd(cmd)) {
		SYS_LOG_WRN("Not ready\n");
	}
	return 0;
}

static int pts_hfp_connect_sco(int argc, char *argv[])
{
	if (btif_pts_hfp_active_connect_sco()) {
		SYS_LOG_WRN("Not ready\n");
	}

	return 0;
}

static int pts_disconnect(int argc, char *argv[])
{
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
	return 0;
}

static const struct shell_cmd pts_test_commands[] = {
	{ "connect_acl_a2dp_avrcp", pts_connect_acl_a2dp_avrcp, "pts active connect acl/a2dp/avrcp"},
	{ "connect_acl_hfp", pts_connect_acl_hfp, "pts active connect acl/hfp"},
	{ "hfp_cmd", pts_hfp_cmd, "pts hfp command"},
	{ "hfp_connect_sco", pts_hfp_connect_sco, "pts hfp active connect sco"},
	{ "disconnect", pts_disconnect, "pts do disconnect"},
	{ NULL, NULL, NULL}
};

SHELL_REGISTER(PTS_TEST_SHELL_MODULE, pts_test_commands);
