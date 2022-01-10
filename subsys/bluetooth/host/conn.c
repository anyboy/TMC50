/* conn.c - Bluetooth connection handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/stack.h>
#include <misc/__assert.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci_driver.h>
#include <bluetooth/att.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CONN)
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "hfp_internal.h"
#include "keys.h"
#include "smp.h"
#include "att_internal.h"
#include "bt_internal_variable.h"

extern struct bt_conn_tx conn_tx_link[];
extern struct bt_conn conns[];
extern struct bt_conn sco_conns[];
extern struct net_buf_pool acl_tx_pool;

#define DEFAULT_PIN_CODE		"0000"
#define SNIFF_MAX_INTERVAL		240		/* 240*0.625ms = 150ms */
#define SNIFF_MIN_INTERVAL		240		/* 240*0.625ms = 150ms */
#define SNIFF_ATTEMPT			4
#define SNIFF_TIMEOUT			1
#define SNIFF_WORK_INTERVAL		1000	/* 1000ms */
#define SNIFF_ENTER_IDLE_CNT	5
#define CONN_TX_PKT_RESERVE		2

/* How long until we cancel HCI_LE_Create_Connection */
#define CONN_TIMEOUT	K_SECONDS(3)

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
const struct bt_conn_auth_cb *bt_auth;
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

static u8_t bt_conn_enable_sniff_check __in_section_unique(bthost_bss);
static struct k_delayed_work sniff_mode_work __in_section_unique(bthost_bss);
static struct bt_conn_cb *callback_list __in_section_unique(bthost_bss);

struct conn_tx_cb {
	bt_conn_tx_cb_t cb;
};

#define conn_tx(buf) ((struct conn_tx_cb *)net_buf_user_data(buf))
static sys_slist_t free_tx = SYS_SLIST_STATIC_INIT(&free_tx);

#if defined(CONFIG_BT_BREDR)
enum pairing_method {
	LEGACY,			/* Legacy (pre-SSP) pairing */
	JUST_WORKS,		/* JustWorks pairing */
	PASSKEY_INPUT,		/* Passkey Entry input */
	PASSKEY_DISPLAY,	/* Passkey Entry display */
	PASSKEY_CONFIRM,	/* Passkey confirm */
};

/* based on table 5.7, Core Spec 4.2, Vol.3 Part C, 5.2.2.6 */
static const u8_t ssp_method[4 /* remote */][4 /* local */] = {
	      { JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS },
	      { JUST_WORKS, PASSKEY_CONFIRM, PASSKEY_INPUT, JUST_WORKS },
	      { PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS },
	      { JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS },
};

static int bt_conn_exit_sniff(struct bt_conn *conn);
#endif /* CONFIG_BT_BREDR */

struct k_sem *bt_conn_get_pkts(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		return &bt_dev.br.pkts;
	}
#endif /* CONFIG_BT_BREDR */

	return &bt_dev.le.pkts;
}

static u16_t bt_conn_br_tx_pending_cnt(struct bt_conn *conn)
{
	u16_t cnt = 0;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&conn->tx_pending, node) {
		cnt++;
	}

	return cnt;
}

static bool bt_conn_check_pkts(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		if (bt_dev.br.pkts.count > CONN_TX_PKT_RESERVE) {
		} else {
			if (bt_conn_br_tx_pending_cnt(conn) > (bt_dev.br.pkts_num - CONN_TX_PKT_RESERVE)) {
				return false;
			} else if (bt_dev.br.pkts.count) {
				return true;
			} else {
				return false;
			}
		}
		return true;
	}
#endif /* CONFIG_BT_BREDR */

	if (bt_dev.le.pkts.count) {
		return true;
	} else {
		return false;
	}
}

static void bt_conn_set_wait_pkts(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		/* atomic_set_bit(bt_dev.flags, BT_DEV_WAIT_BR_PKTS); */
		atomic_set_bit(conn->br.flags, BT_DEV_WAIT_BR_PKTS);
		return;
	}
#endif /* CONFIG_BT_BREDR */

	atomic_set_bit(bt_dev.flags, BT_DEV_WAIT_LE_PKTS);
}

static bool bt_conn_check_wait_pkts(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		/* if (atomic_test_bit(bt_dev.flags, BT_DEV_WAIT_BR_PKTS)) { */
		if (atomic_test_bit(conn->br.flags, BT_DEV_WAIT_BR_PKTS)) {
			return true;
		} else {
			return false;
		}
	}
#endif /* CONFIG_BT_BREDR */

	if (atomic_test_bit(bt_dev.flags, BT_DEV_WAIT_LE_PKTS)) {
		return true;
	} else {
		return false;
	}
}

static int bt_conn_wait_signal_event(struct bt_conn *conn,
			struct k_poll_event events[], u8_t *br_signal, u8_t *le_signal)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		if ((*br_signal) == 1) {
		} else {
			bt_dev.brpkts_signal.signaled = 0;
			k_poll_event_init(&events[0], K_POLL_TYPE_SIGNAL,
					K_POLL_MODE_NOTIFY_ONLY, &bt_dev.brpkts_signal);
			*br_signal = 1;
			return 1;
		}
		return 0;
	}
#endif /* CONFIG_BT_BREDR */

	if ((*le_signal) == 1) {
	} else {
		bt_dev.lepkts_signal.signaled = 0;
		k_poll_event_init(&events[0], K_POLL_TYPE_SIGNAL,
				K_POLL_MODE_NOTIFY_ONLY, &bt_dev.lepkts_signal);
		*le_signal = 1;
		return 1;
	}
	return 0;
}

void bt_conn_set_pkts_signal(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		/* if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_WAIT_BR_PKTS)) { */
		if (atomic_test_and_clear_bit(conn->br.flags, BT_DEV_WAIT_BR_PKTS)) {
			k_poll_signal(&bt_dev.brpkts_signal, 0);
		}
		return;
	}
#endif /* CONFIG_BT_BREDR */

	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_WAIT_LE_PKTS)) {
		k_poll_signal(&bt_dev.lepkts_signal, 0);
	}
}

static inline const char *state2str(bt_conn_state_t state)
{
	switch (state) {
	case BT_CONN_DISCONNECTED:
		return "disconnected";
	case BT_CONN_CONNECT_SCAN:
		return "connect-scan";
	case BT_CONN_CONNECT:
		return "connect";
	case BT_CONN_CONNECTED:
		return "connected";
	case BT_CONN_DISCONNECT:
		return "disconnect";
	default:
		return "(unknown)";
	}
}

static void notify_connected(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->connected) {
			cb->connected(conn, conn->err);
		}
	}

	if (conn->type == BT_CONN_TYPE_BR) {
		conn->br.in_sniff_mode = 0;
		conn->br.sniff_entering = 0;
		conn->br.sniff_exiting = 0;
		conn->br.idle_cnt = 0;
		conn->br.conn_rxtx_cnt = 0;
		conn->br.pre_conn_rxtx_cnt = 0;
		atomic_set(conn->br.flags, 0);
	}
}

static void notify_disconnected(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->disconnected) {
			cb->disconnected(conn, conn->err);
		}
	}
}

void notify_sco_data(struct bt_conn *conn, u8_t *buf, u8_t len, u8_t pkg_flag)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->rx_sco_data) {
			cb->rx_sco_data(conn, buf, len, pkg_flag);
		}
	}
}

void notify_connectionless_data(struct bt_conn *conn, u8_t *data, u16_t len)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->rx_connectionless_data) {
			cb->rx_connectionless_data(conn, data, len);
		}
	}
}

void notify_le_param_updated(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->le_param_updated) {
			cb->le_param_updated(conn, conn->le.interval,
					     conn->le.latency,
					     conn->le.timeout);
		}
	}
}

bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct bt_conn_cb *cb;

	if (!bt_le_conn_params_valid(param)) {
		return false;
	}

	for (cb = callback_list; cb; cb = cb->_next) {
		if (!cb->le_param_req) {
			continue;
		}

		if (!cb->le_param_req(conn, param)) {
			return false;
		}

		/* The callback may modify the parameters so we need to
		 * double-check that it returned valid parameters.
		 */
		if (!bt_le_conn_params_valid(param)) {
			return false;
		}
	}

	/* Default to accepting if there's no app callback */
	return true;
}

static void le_conn_update(struct k_work *work)
{
	struct bt_conn_le *le = CONTAINER_OF(work, struct bt_conn_le,
					     update_work);
	struct bt_conn *conn = CONTAINER_OF(le, struct bt_conn, le);
	const struct bt_le_conn_param *param;

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->state == BT_CONN_CONNECT) {
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	param = BT_LE_CONN_PARAM(conn->le.interval_min,
				 conn->le.interval_max,
				 conn->le.latency,
				 conn->le.timeout);

	bt_conn_le_param_update(conn, param);
}

static struct bt_conn *conn_new(void)
{
	struct bt_conn *conn = NULL;
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		if (!atomic_get(&conns[i].ref)) {
			conn = &conns[i];
			break;
		}
	}

	if (!conn) {
		return NULL;
	}

	memset(conn, 0, sizeof(*conn));

	atomic_set(&conn->ref, 1);

	return conn;
}

#if defined(CONFIG_BT_BREDR)
void bt_sco_cleanup(struct bt_conn *sco_conn)
{
	bt_conn_unref(sco_conn->sco.acl);
	sco_conn->sco.acl = NULL;
	bt_conn_unref(sco_conn);
}

static struct bt_conn *sco_conn_new(void)
{
	struct bt_conn *sco_conn = NULL;
	int i;

	for (i = 0; i < bt_inner_value.br_max_conn; i++) {
		if (!atomic_get(&sco_conns[i].ref)) {
			sco_conn = &sco_conns[i];
			break;
		}
	}

	if (!sco_conn) {
		return NULL;
	}

	memset(sco_conn, 0, sizeof(*sco_conn));

	atomic_set(&sco_conn->ref, 1);

	return sco_conn;
}

struct bt_conn *bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param)
{
	struct bt_hci_cp_connect *cp;
	struct bt_conn *conn;
	struct net_buf *buf;

	conn = bt_conn_lookup_addr_br(peer);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_CONNECT:
		case BT_CONN_CONNECTED:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}

	conn = bt_conn_add_br(peer);
	if (!conn) {
		return NULL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_CONNECT, sizeof(*cp));
	if (!buf) {
		bt_conn_unref(conn);
		return NULL;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	memset(cp, 0, sizeof(*cp));

	memcpy(&cp->bdaddr, peer, sizeof(cp->bdaddr));
	cp->packet_type = sys_cpu_to_le16(0xcc18); /* DM1 DH1 DM3 DH5 DM5 DH5 */
	cp->pscan_rep_mode = 0x02; /* R2 */
	cp->allow_role_switch = param->allow_role_switch ? 0x01 : 0x00;
	cp->clock_offset = 0x0000; /* TODO used cached clock offset */

	if (bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT, buf, NULL) < 0) {
		bt_conn_unref(conn);
		return NULL;
	}

	bt_conn_set_state(conn, BT_CONN_CONNECT);
	conn->role = BT_CONN_ROLE_MASTER;

	return conn;
}

struct bt_conn *bt_conn_br_acl_connecting(const bt_addr_t *addr)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_br(addr);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_CONNECT:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}

	return NULL;
}

int bt_conn_create_sco(struct bt_conn *br_conn)
{
	struct bt_hci_cp_setup_sync_conn *cp;
	struct bt_conn *sco_conn;
	struct net_buf *buf;
	int link_type;
	u8_t remote_esco_capable;

	sco_conn = bt_conn_lookup_addr_sco(&br_conn->br.dst);
	if (sco_conn) {
		switch (sco_conn->state) {
		case BT_CONN_CONNECT:
		case BT_CONN_CONNECTED:
			bt_conn_unref(sco_conn);
			return 0;
		default:
			bt_conn_unref(sco_conn);
			return -EBUSY;
		}
	}

	remote_esco_capable = BT_FEAT_LMP_ESCO_CAPABLE(br_conn->br.features);
	if (BT_FEAT_LMP_ESCO_CAPABLE(bt_dev.features) && remote_esco_capable) {
		link_type = BT_HCI_ESCO;
	} else {
		link_type = BT_HCI_SCO;
	}

	sco_conn = bt_conn_add_sco(&br_conn->br.dst, link_type);
	if (!sco_conn) {
		return -ENOMEM;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_SETUP_SYNC_CONN, sizeof(*cp));
	if (!buf) {
		bt_sco_cleanup(sco_conn);
		return -EIO;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	memset(cp, 0, sizeof(*cp));

	BT_DBG("handle : %x", sco_conn->sco.acl->handle);

	cp->handle = sco_conn->sco.acl->handle;
	cp->pkt_type = sco_conn->sco.pkt_type;
	cp->tx_bandwidth = 0x00001f40;
	cp->rx_bandwidth = 0x00001f40;
	cp->max_latency = 0x0007;
	/* Adapt controller, remote only support sco, set retrans_effort 0xFF */
	cp->retrans_effort = (remote_esco_capable) ? 0x01 : 0xFF;
	cp->content_format = BT_VOICE_CVSD_16BIT;

	if (bt_hci_cmd_send_sync(BT_HCI_OP_SETUP_SYNC_CONN, buf,
				 NULL) < 0) {
		bt_sco_cleanup(sco_conn);
		return -EIO;
	}

	bt_conn_set_state(sco_conn, BT_CONN_CONNECT);

	bt_conn_unref(sco_conn);
	return 0;
}

/* Just for pts test
 * for BT_HCI_OP_SETUP_SYNC_CONN command
 * controler will create esco when ag/hf both support
 */
int bt_pts_conn_creat_add_sco_cmd(struct bt_conn *br_conn)
{
	struct bt_hci_cp_add_sco_conn *cp;
	struct bt_conn *sco_conn;
	struct net_buf *buf;

	sco_conn = bt_conn_lookup_addr_sco(&br_conn->br.dst);
	if (sco_conn) {
		switch (sco_conn->state) {
		case BT_CONN_CONNECT:
		case BT_CONN_CONNECTED:
			bt_conn_unref(sco_conn);
			return 0;
		default:
			bt_conn_unref(sco_conn);
			return -EBUSY;
		}
	}

	sco_conn = bt_conn_add_sco(&br_conn->br.dst, BT_HCI_SCO);
	if (!sco_conn) {
		return -ENOMEM;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_ADD_SCO_CONN, sizeof(*cp));
	if (!buf) {
		bt_sco_cleanup(sco_conn);
		return -EIO;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memset(cp, 0, sizeof(*cp));

	BT_DBG("handle : %x", sco_conn->sco.acl->handle);
	cp->handle = sco_conn->sco.acl->handle;
	cp->pkt_type = (sco_conn->sco.pkt_type << 5);

	if (bt_hci_cmd_send_sync(BT_HCI_OP_ADD_SCO_CONN, buf,
				 NULL) < 0) {
		bt_sco_cleanup(sco_conn);
		return -EIO;
	}

	bt_conn_set_state(sco_conn, BT_CONN_CONNECT);
	bt_conn_unref(sco_conn);

	return 0;
}

struct bt_conn *bt_conn_lookup_addr_sco(const bt_addr_t *peer)
{
	int i;

	for (i = 0; i < bt_inner_value.br_max_conn; i++) {
		if (!atomic_get(&sco_conns[i].ref)) {
			continue;
		}

		if (sco_conns[i].type != BT_CONN_TYPE_SCO) {
			continue;
		}

		if (!bt_addr_cmp(peer, &sco_conns[i].sco.acl->br.dst)) {
			return bt_conn_ref(&sco_conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].type != BT_CONN_TYPE_BR) {
			continue;
		}

		if (!bt_addr_cmp(peer, &conns[i].br.dst)) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_add_sco(const bt_addr_t *peer, int link_type)
{
	struct bt_conn *sco_conn = sco_conn_new();

	if (!sco_conn) {
		return NULL;
	}

	sco_conn->sco.acl = bt_conn_lookup_addr_br(peer);
	sco_conn->type = BT_CONN_TYPE_SCO;

	if (link_type == BT_HCI_SCO) {
		sco_conn->sco.pkt_type = (bt_dev.br.esco_pkt_type &
								sco_conn->sco.acl->br.esco_pkt_type);
		sco_conn->sco.pkt_type &= ESCO_PKT_MASK;
#if 0
		if (BT_FEAT_LMP_ESCO_CAPABLE(bt_dev.features)) {
			sco_conn->sco.pkt_type = (bt_dev.br.esco_pkt_type &
						  ESCO_PKT_MASK);
		} else {
			/* SCO_PKT_MASK for ADD SCO CONNECTION COMMAND.
			 * BLUETOOTH SPECIFICATION Version 4.2 [Vol 2, Part E].
			 * A Host should not use these commands.
			 */
			sco_conn->sco.pkt_type = (bt_dev.br.esco_pkt_type &
						  SCO_PKT_MASK);
		}
#endif
	} else if (link_type == BT_HCI_ESCO) {
		sco_conn->sco.pkt_type = (bt_dev.br.esco_pkt_type &
								sco_conn->sco.acl->br.esco_pkt_type);
		sco_conn->sco.pkt_type &= (~EDR_ESCO_PKT_MASK);
	}

	return sco_conn;
}

struct bt_conn *bt_conn_add_br(const bt_addr_t *peer)
{
	struct bt_conn *conn = conn_new();

	if (!conn) {
		return NULL;
	}

	bt_addr_copy(&conn->br.dst, peer);
	conn->type = BT_CONN_TYPE_BR;

	return conn;
}

static int pin_code_neg_reply(const bt_addr_t *bdaddr)
{
	struct bt_hci_cp_pin_code_neg_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_PIN_CODE_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);

	return bt_hci_cmd_send_sync(BT_HCI_OP_PIN_CODE_NEG_REPLY, buf, NULL);
}

static int pin_code_reply(struct bt_conn *conn, const char *pin, u8_t len)
{
	struct bt_hci_cp_pin_code_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_PIN_CODE_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	bt_addr_copy(&cp->bdaddr, &conn->br.dst);
	cp->pin_len = len;
	strncpy((char *)cp->pin_code, pin, sizeof(cp->pin_code));

	return bt_hci_cmd_send_sync(BT_HCI_OP_PIN_CODE_REPLY, buf, NULL);
}

int bt_conn_auth_pincode_entry(struct bt_conn *conn, const char *pin)
{
	size_t len;

	if (!bt_auth) {
		return -EINVAL;
	}

	if (conn->type != BT_CONN_TYPE_BR) {
		return -EINVAL;
	}

	len = strlen(pin);
	if (len > 16) {
		return -EINVAL;
	}

	if (conn->required_sec_level == BT_SECURITY_HIGH && len < 16) {
		BT_WARN("PIN code for %s is not 16 bytes wide",
			bt_addr_str(&conn->br.dst));
		return -EPERM;
	}

	/* Allow user send entered PIN to remote, then reset user state. */
	if (!atomic_test_and_clear_bit(conn->flags, BT_CONN_USER)) {
		return -EPERM;
	}

	if (len == 16) {
		atomic_set_bit(conn->flags, BT_CONN_BR_LEGACY_SECURE);
	}

	return pin_code_reply(conn, pin, len);
}

void bt_conn_pin_code_req(struct bt_conn *conn)
{
	if (bt_auth && bt_auth->pincode_entry) {
		bool secure = false;

		if (conn->required_sec_level == BT_SECURITY_HIGH) {
			secure = true;
		}

		atomic_set_bit(conn->flags, BT_CONN_USER);
		atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
		bt_auth->pincode_entry(conn, secure);
	} else {
#ifdef DEFAULT_PIN_CODE
		pin_code_reply(conn, DEFAULT_PIN_CODE, strlen(DEFAULT_PIN_CODE));
#else
		pin_code_neg_reply(&conn->br.dst);
#endif
	}
}

u8_t bt_conn_get_io_capa(void)
{
	if (!bt_auth) {
		return BT_IO_NO_INPUT_OUTPUT;
	}

	if (bt_auth->passkey_confirm && bt_auth->passkey_display) {
		return BT_IO_DISPLAY_YESNO;
	}

	if (bt_auth->passkey_entry) {
		return BT_IO_KEYBOARD_ONLY;
	}

	if (bt_auth->passkey_display) {
		return BT_IO_DISPLAY_ONLY;
	}

	return BT_IO_NO_INPUT_OUTPUT;
}

static u8_t ssp_pair_method(const struct bt_conn *conn)
{
	return ssp_method[conn->br.remote_io_capa][bt_conn_get_io_capa()];
}

u8_t bt_conn_ssp_get_auth(const struct bt_conn *conn)
{
	/* Validate no bond auth request, and if valid use it. */
	if ((conn->br.remote_auth == BT_HCI_NO_BONDING) ||
	    ((conn->br.remote_auth == BT_HCI_NO_BONDING_MITM) &&
	     (ssp_pair_method(conn) > JUST_WORKS))) {
		return conn->br.remote_auth;
	}

	/* Local & remote have enough IO capabilities to get MITM protection. */
	if (ssp_pair_method(conn) > JUST_WORKS) {
		return conn->br.remote_auth | BT_MITM;
	}

	/* No MITM protection possible so ignore remote MITM requirement. */
	return (conn->br.remote_auth & ~BT_MITM);
}

static int ssp_confirm_reply(struct bt_conn *conn)
{
	struct bt_hci_cp_user_confirm_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_USER_CONFIRM_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, &conn->br.dst);

	return bt_hci_cmd_send_sync(BT_HCI_OP_USER_CONFIRM_REPLY, buf, NULL);
}

static int ssp_confirm_neg_reply(struct bt_conn *conn)
{
	struct bt_hci_cp_user_confirm_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_USER_CONFIRM_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, &conn->br.dst);

	return bt_hci_cmd_send_sync(BT_HCI_OP_USER_CONFIRM_NEG_REPLY, buf,
				    NULL);
}

void bt_conn_ssp_auth(struct bt_conn *conn, u32_t passkey)
{
	conn->br.pairing_method = ssp_pair_method(conn);

	/*
	 * If local required security is HIGH then MITM is mandatory.
	 * MITM protection is no achievable when SSP 'justworks' is applied.
	 */
	if (conn->required_sec_level > BT_SECURITY_MEDIUM &&
	    conn->br.pairing_method == JUST_WORKS) {
		BT_DBG("MITM protection infeasible for required security");
		ssp_confirm_neg_reply(conn);
		return;
	}

	switch (conn->br.pairing_method) {
	case PASSKEY_CONFIRM:
		atomic_set_bit(conn->flags, BT_CONN_USER);
		bt_auth->passkey_confirm(conn, passkey);
		break;
	case PASSKEY_DISPLAY:
		atomic_set_bit(conn->flags, BT_CONN_USER);
		bt_auth->passkey_display(conn, passkey);
		break;
	case PASSKEY_INPUT:
		atomic_set_bit(conn->flags, BT_CONN_USER);
		bt_auth->passkey_entry(conn);
		break;
	case JUST_WORKS:
		/*
		 * When local host works as pairing acceptor and 'justworks'
		 * model is applied then notify user about such pairing request.
		 * [BT Core 4.2 table 5.7, Vol 3, Part C, 5.2.2.6]
		 */
		if (bt_auth && bt_auth->pairing_confirm &&
		    !atomic_test_bit(conn->flags,
				     BT_CONN_BR_PAIRING_INITIATOR)) {
			atomic_set_bit(conn->flags, BT_CONN_USER);
			bt_auth->pairing_confirm(conn);
			break;
		}
		ssp_confirm_reply(conn);
		break;
	default:
		break;
	}
}

static int ssp_passkey_reply(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_hci_cp_user_passkey_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_USER_PASSKEY_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, &conn->br.dst);
	cp->passkey = sys_cpu_to_le32(passkey);

	return bt_hci_cmd_send_sync(BT_HCI_OP_USER_PASSKEY_REPLY, buf, NULL);
}

static int ssp_passkey_neg_reply(struct bt_conn *conn)
{
	struct bt_hci_cp_user_passkey_neg_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_USER_PASSKEY_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, &conn->br.dst);

	return bt_hci_cmd_send_sync(BT_HCI_OP_USER_PASSKEY_NEG_REPLY, buf,
				    NULL);
}

static int bt_hci_connect_br_cancel(struct bt_conn *conn)
{
	struct bt_hci_cp_connect_cancel *cp;
	struct bt_hci_rp_connect_cancel *rp;
	struct net_buf *buf, *rsp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_CONNECT_CANCEL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memcpy(&cp->bdaddr, &conn->br.dst, sizeof(cp->bdaddr));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT_CANCEL, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;

	err = rp->status ? -EIO : 0;

	net_buf_unref(rsp);

	return err;
}

static int conn_auth(struct bt_conn *conn)
{
	struct bt_hci_cp_auth_requested *auth;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_AUTH_REQUESTED, sizeof(*auth));
	if (!buf) {
		return -ENOBUFS;
	}

	auth = net_buf_add(buf, sizeof(*auth));
	auth->handle = sys_cpu_to_le16(conn->handle);

	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING_INITIATOR);

	return bt_hci_cmd_send_sync(BT_HCI_OP_AUTH_REQUESTED, buf, NULL);
}
#endif /* CONFIG_BT_BREDR */

#if defined(CONFIG_BT_SMP)
void bt_conn_identity_resolved(struct bt_conn *conn)
{
	const bt_addr_le_t *rpa;
	struct bt_conn_cb *cb;

	if (conn->role == BT_HCI_ROLE_MASTER) {
		rpa = &conn->le.resp_addr;
	} else {
		rpa = &conn->le.init_addr;
	}

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->identity_resolved) {
			cb->identity_resolved(conn, rpa, &conn->le.dst);
		}
	}
}

int bt_conn_le_start_encryption(struct bt_conn *conn, u64_t rand,
				u16_t ediv, const u8_t *ltk, size_t len)
{
	struct bt_hci_cp_le_start_encryption *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_START_ENCRYPTION, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->rand = rand;
	cp->ediv = ediv;

	memcpy(cp->ltk, ltk, len);
	if (len < sizeof(cp->ltk)) {
		memset(cp->ltk + len, 0, sizeof(cp->ltk) - len);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_START_ENCRYPTION, buf, NULL);
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
u8_t bt_conn_enc_key_size(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		struct bt_hci_cp_read_encryption_key_size *cp;
		struct bt_hci_rp_read_encryption_key_size *rp;
		struct net_buf *buf;
		struct net_buf *rsp;
		u8_t key_size;

		buf = bt_hci_cmd_create(BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
					sizeof(*cp));
		if (!buf) {
			return 0;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->handle = sys_cpu_to_le16(conn->handle);

		if (bt_hci_cmd_send_sync(BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
					buf, &rsp)) {
			return 0;
		}

		rp = (void *)rsp->data;

		key_size = rp->status ? 0 : rp->key_size;

		net_buf_unref(rsp);

		return key_size;
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		return conn->le.keys ? conn->le.keys->enc_size : 0;
	}

	return 0;
}

void bt_conn_security_changed(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->security_changed) {
			cb->security_changed(conn, conn->sec_level);
		}
	}
}

void bt_conn_notify_role_change(struct bt_conn *conn, u8_t role)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->role_change) {
			cb->role_change(conn, role);
		}
	}
}

bool bt_conn_notify_connect_req(bt_addr_t *peer)
{
	bool accept_req = true;
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->connect_req) {
			if (!cb->connect_req(peer)) {
				accept_req = false;
			}
		}
	}

	return accept_req;
}

static int start_security(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		if (atomic_test_bit(conn->flags, BT_CONN_BR_PAIRING)) {
			return -EBUSY;
		}

		if (conn->required_sec_level > BT_SECURITY_HIGH) {
			return -ENOTSUP;
		}

		if (bt_conn_get_io_capa() == BT_IO_NO_INPUT_OUTPUT &&
		    conn->required_sec_level > BT_SECURITY_MEDIUM) {
			return -EINVAL;
		}

		return conn_auth(conn);
	}
#endif /* CONFIG_BT_BREDR */

	switch (conn->role) {
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_SMP)
	case BT_HCI_ROLE_MASTER:
	{
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256,
						     &conn->le.dst);
			if (!conn->le.keys) {
				conn->le.keys = bt_keys_find(BT_KEYS_LTK,
							     &conn->le.dst);
			}
		}

		if (!conn->le.keys ||
		    !(conn->le.keys->keys & (BT_KEYS_LTK | BT_KEYS_LTK_P256))) {
			return bt_smp_send_pairing_req(conn);
		}

		if (conn->required_sec_level > BT_SECURITY_MEDIUM &&
		    !atomic_test_bit(conn->le.keys->flags,
				     BT_KEYS_AUTHENTICATED)) {
			return bt_smp_send_pairing_req(conn);
		}

		if (conn->required_sec_level > BT_SECURITY_HIGH &&
		    !atomic_test_bit(conn->le.keys->flags,
				     BT_KEYS_AUTHENTICATED) &&
		    !(conn->le.keys->keys & BT_KEYS_LTK_P256)) {
			return bt_smp_send_pairing_req(conn);
		}

		/* LE SC LTK and legacy master LTK are stored in same place */
		return bt_conn_le_start_encryption(conn,
						   conn->le.keys->ltk.rand,
						   conn->le.keys->ltk.ediv,
						   conn->le.keys->ltk.val,
						   conn->le.keys->enc_size);
	}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_SMP */
#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_SMP)
	case BT_HCI_ROLE_SLAVE:
		return bt_smp_send_security_req(conn);
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_SMP */
	default:
		return -EINVAL;
	}
}

int bt_conn_security(struct bt_conn *conn, bt_security_t sec)
{
	int err;

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_SC_ONLY) &&
	    sec < BT_SECURITY_FIPS) {
		return -EOPNOTSUPP;
	}

	/* nothing to do */
	if (conn->sec_level >= sec || conn->required_sec_level >= sec) {
		return 0;
	}

	conn->required_sec_level = sec;

	err = start_security(conn);

	/* reset required security level in case of error */
	if (err) {
		conn->required_sec_level = conn->sec_level;
	}

	return err;
}
#endif /* CONFIG_BT_SMP */

void bt_conn_cb_register(struct bt_conn_cb *cb)
{
	cb->_next = callback_list;
	callback_list = cb;
}

static void bt_conn_reset_rx_state(struct bt_conn *conn)
{
	if (!conn->rx_len) {
		return;
	}

	net_buf_unref(conn->rx);
	conn->rx = NULL;
	conn->rx_len = 0;
}

void bt_conn_recv(struct bt_conn *conn, struct net_buf *buf, u8_t flags)
{
	struct bt_l2cap_hdr *hdr;
	u16_t len;

	BT_DBG("handle %u len %u flags %02x", conn->handle, buf->len, flags);

	/* Check packet boundary flags */
	switch (flags) {
	case BT_ACL_START:
		hdr = (void *)buf->data;
		len = sys_le16_to_cpu(hdr->len);

		BT_DBG("First, len %u final %u", buf->len, len);

		if (conn->rx_len) {
			BT_ERR("Unexpected first L2CAP frame");
			bt_conn_reset_rx_state(conn);
		}

		conn->rx_len = (sizeof(*hdr) + len) - buf->len;
		BT_DBG("rx_len %u", conn->rx_len);
		if (conn->rx_len) {
			conn->rx = buf;
			return;
		}

		break;
	case BT_ACL_CONT:
		if (!conn->rx_len) {
			BT_ERR("Unexpected L2CAP continuation");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		if (buf->len > conn->rx_len) {
			BT_ERR("L2CAP data overflow");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		BT_DBG("Cont, len %u rx_len %u", buf->len, conn->rx_len);

		if (buf->len > net_buf_tailroom(conn->rx)) {
			BT_ERR("Not enough buffer space for L2CAP data");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		net_buf_add_mem(conn->rx, buf->data, buf->len);
		conn->rx_len -= buf->len;
		net_buf_unref(buf);

		if (conn->rx_len) {
			return;
		}

		buf = conn->rx;
		conn->rx = NULL;
		conn->rx_len = 0;

		break;
	default:
		BT_ERR("Unexpected ACL flags (0x%02x)", flags);
		bt_conn_reset_rx_state(conn);
		net_buf_unref(buf);
		return;
	}

	hdr = (void *)buf->data;
	len = sys_le16_to_cpu(hdr->len);

	if (sizeof(*hdr) + len != buf->len) {
		BT_ERR("ACL len mismatch (%u != %u)", len, buf->len);
		net_buf_unref(buf);
		return;
	}

	BT_DBG("Successfully parsed %u byte L2CAP packet", buf->len);

	bt_l2cap_recv(conn, buf);
}

int bt_conn_send_cb(struct bt_conn *conn, struct net_buf *buf,
		    bt_conn_tx_cb_t cb)
{
	struct net_buf_pool *pool;

	BT_DBG("conn handle %u buf len %u cb %p", conn->handle, buf->len, cb);

	pool = net_buf_pool_get(buf->pool_id);
	if (pool->user_data_size < BT_BUF_USER_DATA_MIN) {
		BT_ERR("Too small user data size");
		net_buf_unref(buf);
		return -EINVAL;
	}

	if (conn->state != BT_CONN_CONNECTED) {
		BT_ERR("not connected!");
		net_buf_unref(buf);
		return -ENOTCONN;
	}

	if (conn->type == BT_CONN_TYPE_BR) {
		if (conn->br.in_sniff_mode && (!conn->br.sniff_exiting)) {
			conn->br.sniff_exiting = 1;
			bt_conn_exit_sniff(conn);
		}
	}

	conn_tx(buf)->cb = cb;
	net_buf_put(&conn->tx_queue, buf);

	/* Let buf send out quickly */
	if (k_thread_priority_get(k_current_get()) <
		K_PRIO_COOP(CONFIG_BT_HCI_TX_PRIO)) {
		BT_WARN("Priority higher than tx thread");
	}
	k_yield();
	return 0;
}

static void tx_free(struct bt_conn_tx *tx)
{
	tx->cb = NULL;
	sys_slist_prepend(&free_tx, &tx->node);
}

void bt_conn_notify_tx(struct bt_conn *conn)
{
	struct bt_conn_tx *tx;

	BT_DBG("conn %p", conn);

	while ((tx = k_fifo_get(&conn->tx_notify, K_NO_WAIT))) {
		if (tx->cb) {
			tx->cb(conn);
		}

		tx_free(tx);
	}
}

static void notify_tx(void)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].state == BT_CONN_CONNECTED ||
		    conns[i].state == BT_CONN_DISCONNECT) {
			bt_conn_notify_tx(&conns[i]);
		}
	}
}

static sys_snode_t *add_pending_tx(struct bt_conn *conn, bt_conn_tx_cb_t cb)
{
	sys_snode_t *node;
	unsigned int key;

	BT_DBG("conn %p cb %p", conn, cb);

	__ASSERT(!sys_slist_is_empty(&free_tx), "No free conn TX contexts");

	node = sys_slist_get_not_empty(&free_tx);
	CONTAINER_OF(node, struct bt_conn_tx, node)->cb = cb;

	key = irq_lock();
	sys_slist_append(&conn->tx_pending, node);
	irq_unlock(key);

	return node;
}

static void remove_pending_tx(struct bt_conn *conn, sys_snode_t *node)
{
	unsigned int key;

	key = irq_lock();
	sys_slist_find_and_remove(&conn->tx_pending, node);
	irq_unlock(key);

	tx_free(CONTAINER_OF(node, struct bt_conn_tx, node));
}

static bool send_frag(struct bt_conn *conn, struct net_buf *buf, u8_t flags,
		      bool always_consume)
{
	struct bt_hci_acl_hdr *hdr;
	bt_conn_tx_cb_t cb;
	sys_snode_t *node;
	int err;

	BT_DBG("conn %p buf %p len %u flags 0x%02x", conn, buf, buf->len,
	       flags);

	/* Wait until the controller can accept ACL packets */
	if (k_sem_take(bt_conn_get_pkts(conn), K_NO_WAIT)) {
		BT_WARN("Conn(type:%d) need wait pkts", conn->type);
		k_sem_take(bt_conn_get_pkts(conn), K_FOREVER);
	}

	/* Make sure we notify and free up any pending tx contexts */
	notify_tx();

	/* Check for disconnection while waiting for pkts_sem */
	if (conn->state != BT_CONN_CONNECTED) {
		goto fail;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(conn->handle, flags));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	cb = conn_tx(buf)->cb;
	bt_buf_set_type(buf, BT_BUF_ACL_OUT);

	node = add_pending_tx(conn, cb);

	err = bt_send(buf);
	if (err) {
		BT_ERR("Unable to send to driver (err %d)", err);
		remove_pending_tx(conn, node);
		goto fail;
	}

	return true;

fail:
	k_sem_give(bt_conn_get_pkts(conn));
	bt_conn_set_pkts_signal(conn);
	if (always_consume) {
		net_buf_unref(buf);
	}
	return false;
}

static inline u16_t conn_mtu(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		return bt_dev.br.mtu;
	}
#endif /* CONFIG_BT_BREDR */

	return bt_dev.le.mtu;
}

static struct net_buf *create_frag(struct bt_conn *conn, struct net_buf *buf)
{
	struct net_buf *frag;
	u16_t frag_len;

	frag = bt_conn_create_pdu(NULL, 0);

	if (conn->state != BT_CONN_CONNECTED) {
		net_buf_unref(frag);
		return NULL;
	}

	/* Fragments never have a TX completion callback */
	conn_tx(frag)->cb = NULL;

	frag_len = min(conn_mtu(conn), net_buf_tailroom(frag));

	net_buf_add_mem(frag, buf->data, frag_len);
	net_buf_pull(buf, frag_len);

	return frag;
}

static bool send_buf(struct bt_conn *conn, struct net_buf *buf)
{
	struct net_buf *frag;

	BT_DBG("conn %p buf %p len %u", conn, buf, buf->len);

	/* Send directly if the packet fits the ACL MTU */
	if (buf->len <= conn_mtu(conn)) {
		return send_frag(conn, buf, BT_ACL_START_NO_FLUSH, false);
	}

	BT_WARN("buf->len(%d) > mtu(%d), pool id:%d", buf->len, conn_mtu(conn), buf->pool_id);

	/* Create & enqueue first fragment */
	frag = create_frag(conn, buf);
	if (!frag) {
		return false;
	}

	if (!send_frag(conn, frag, BT_ACL_START_NO_FLUSH, true)) {
		return false;
	}

	/*
	 * Send the fragments. For the last one simply use the original
	 * buffer (which works since we've used net_buf_pull on it.
	 */
	while (buf->len > conn_mtu(conn)) {
		frag = create_frag(conn, buf);
		if (!frag) {
			return false;
		}

		if (!send_frag(conn, frag, BT_ACL_CONT, true)) {
			return false;
		}
	}

	return send_frag(conn, buf, BT_ACL_CONT, false);
}

static struct k_poll_signal conn_change =
		K_POLL_SIGNAL_INITIALIZER(conn_change);

static void conn_cleanup(struct bt_conn *conn)
{
	struct net_buf *buf;
	struct bt_conn_tx *tx;

	/* Give back any allocated buffers */
	while ((buf = net_buf_get(&conn->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	__ASSERT(sys_slist_is_empty(&conn->tx_pending), "Pending TX packets");
	while ((tx = k_fifo_get(&conn->tx_notify, K_NO_WAIT))) {
		tx_free(tx);
	}

	bt_conn_reset_rx_state(conn);

	/* Release the reference we took for the very first
	 * state transition.
	 */
	bt_conn_unref(conn);
}

int bt_conn_prepare_events(struct k_poll_event events[])
{
	int i, ev_count = 0;
	u8_t br_signal = 0, le_signal = 0;

	BT_DBG("");

	conn_change.signaled = 0;
	k_poll_event_init(&events[ev_count++], K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &conn_change);

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		struct bt_conn *conn = &conns[i];

		if (!atomic_get(&conn->ref)) {
			continue;
		}

		if (conn->state == BT_CONN_DISCONNECTED &&
		    atomic_test_and_clear_bit(conn->flags, BT_CONN_CLEANUP)) {
			conn_cleanup(conn);
			continue;
		}

		if (conn->state != BT_CONN_CONNECTED) {
			continue;
		}

		BT_DBG("Adding conn %p to poll list", conn);

		k_poll_event_init(&events[ev_count],
				  K_POLL_TYPE_FIFO_DATA_AVAILABLE,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &conn->tx_notify);
		events[ev_count++].tag = BT_EVENT_CONN_TX_NOTIFY;

		if (bt_conn_check_wait_pkts(conn)) {
			ev_count += bt_conn_wait_signal_event(conn, &events[ev_count], &br_signal, &le_signal);
		} else {
			k_poll_event_init(&events[ev_count],
					  K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					  K_POLL_MODE_NOTIFY_ONLY,
					  &conn->tx_queue);
			events[ev_count++].tag = BT_EVENT_CONN_TX_QUEUE;
		}
	}

	return ev_count;
}

void bt_conn_process_tx(struct bt_conn *conn)
{
	struct net_buf *buf;
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	u32_t timestamp;
#endif

	BT_DBG("conn %p", conn);

	if (conn->state == BT_CONN_DISCONNECTED &&
	    atomic_test_and_clear_bit(conn->flags, BT_CONN_CLEANUP)) {
		BT_DBG("handle %u disconnected - cleaning up", conn->handle);
		conn_cleanup(conn);
		return;
	}

	if (!bt_conn_check_pkts(conn)) {
		bt_conn_set_wait_pkts(conn);
		return;
	}

	/* Get next ACL packet for connection */
	buf = net_buf_get(&conn->tx_queue, K_NO_WAIT);
	BT_ASSERT(buf);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	timestamp = k_uptime_get_32();
	if ((timestamp - buf->timestamp) > NET_BUF_TIMESTAMP_CHECK_TIME) {
		BT_WARN("Tx queue time:%d ms", (timestamp - buf->timestamp));
	}
#endif

	if (conn->type == BT_CONN_TYPE_BR) {
		conn->br.conn_rxtx_cnt++;
	}

	if (!send_buf(conn, buf)) {
		net_buf_unref(buf);
	}
}

int bt_br_conn_ready_send_data(struct bt_conn *conn)
{
	if ((conn->state != BT_CONN_CONNECTED) ||
		sys_slist_is_empty(&free_tx) ||
		(bt_dev.br.pkts.count <= bt_inner_value.br_reserve_pkts)) {
		/* BT_WARN("Conn(%p):%d, free_tx empty:%d, pkts:%d", conn, conn->state,
		 * sys_slist_is_empty(&free_tx), bt_dev.br.pkts.count);
		 */
		return 0;
	} else {
		return 1;
	}
}

int bt_br_conn_pending_pkt(struct bt_conn *conn, u8_t *host_pending, u8_t *controler_pending)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (&conns[i] == conn) {
			*host_pending = (u8_t)k_fifo_cnt_sum(&conn->tx_queue);
			*controler_pending = bt_conn_br_tx_pending_cnt(conn);
			return 0;
		}
	}

	*host_pending = 0;
	*controler_pending = 0;
	return -EIO;
}

int bt_le_conn_ready_send_data(struct bt_conn *conn)
{
	if ((conn->state != BT_CONN_CONNECTED) ||
		sys_slist_is_empty(&free_tx) ||
		(bt_dev.le.pkts.count <= bt_inner_value.le_reserve_pkts)) {
		BT_WARN("Conn(%p):%d, free_tx empty:%d, pkts:%d", conn, conn->state,
			sys_slist_is_empty(&free_tx), bt_dev.le.pkts.count);
		return 0;
	} else {
		return 1;
	}
}

struct bt_conn *bt_conn_add_le(const bt_addr_le_t *peer)
{
	struct bt_conn *conn = conn_new();

	if (!conn) {
		return NULL;
	}

	bt_addr_le_copy(&conn->le.dst, peer);
#if defined(CONFIG_BT_SMP)
	conn->sec_level = BT_SECURITY_LOW;
	conn->required_sec_level = BT_SECURITY_LOW;
#endif /* CONFIG_BT_SMP */
	conn->type = BT_CONN_TYPE_LE;
	conn->le.interval_min = BT_GAP_INIT_CONN_INT_MIN;
	conn->le.interval_max = BT_GAP_INIT_CONN_INT_MAX;
	k_delayed_work_init(&conn->le.update_work, le_conn_update);

	return conn;
}

static void process_unack_tx(struct bt_conn *conn)
{
	/* Return any unacknowledged packets */
	while (1) {
		sys_snode_t *node;
		unsigned int key;

		key = irq_lock();
		node = sys_slist_get(&conn->tx_pending);
		irq_unlock(key);

		if (!node) {
			break;
		}

		tx_free(CONTAINER_OF(node, struct bt_conn_tx, node));

		k_sem_give(bt_conn_get_pkts(conn));
		bt_conn_set_pkts_signal(conn);
	}
}

void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state)
{
	bt_conn_state_t old_state;

	BT_DBG("%s -> %s", state2str(conn->state), state2str(state));

	if (conn->state == state) {
		BT_WARN("no transition");
		return;
	}

	old_state = conn->state;
	conn->state = state;

	/* Actions needed for exiting the old state */
	switch (old_state) {
	case BT_CONN_DISCONNECTED:
		/* Take a reference for the first state transition after
		 * bt_conn_add_le() and keep it until reaching DISCONNECTED
		 * again.
		 */
		bt_conn_ref(conn);
		break;
	case BT_CONN_CONNECT:
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    conn->type == BT_CONN_TYPE_LE) {
			k_delayed_work_cancel(&conn->le.update_work);
		}
		break;
	default:
		break;
	}

	/* Actions needed for entering the new state */
	switch (conn->state) {
	case BT_CONN_CONNECTED:
		if (conn->type == BT_CONN_TYPE_SCO) {
			notify_connected(conn);
			break;
		}
		k_fifo_init(&conn->tx_queue);
		k_fifo_init(&conn->tx_notify);
		k_poll_signal(&conn_change, 0);

		sys_slist_init(&conn->channels);

		bt_l2cap_connected(conn);
		notify_connected(conn);
		break;
	case BT_CONN_DISCONNECTED:
		if (conn->type == BT_CONN_TYPE_SCO) {
			if (old_state == BT_CONN_CONNECTED ||
				old_state == BT_CONN_DISCONNECT) {
				notify_disconnected(conn);
			}

			bt_conn_unref(conn);
			break;
		}
		/* Notify disconnection and queue a dummy buffer to wake
		 * up and stop the tx thread for states where it was
		 * running.
		 */
		if (old_state == BT_CONN_CONNECTED ||
		    old_state == BT_CONN_DISCONNECT) {
			bt_l2cap_disconnected(conn);
			notify_disconnected(conn);
			process_unack_tx(conn);

			/* Cancel Connection Update if it is pending */
			if (conn->type == BT_CONN_TYPE_LE) {
				k_delayed_work_cancel(&conn->le.update_work);
			}

			atomic_set_bit(conn->flags, BT_CONN_CLEANUP);
			k_poll_signal(&conn_change, 0);
			/* The last ref will be dropped by the tx_thread */
		} else if (old_state == BT_CONN_CONNECT) {
			/* conn->err will be set in this case */
			notify_connected(conn);
			bt_conn_unref(conn);
		} else if (old_state == BT_CONN_CONNECT_SCAN) {
			/* this indicate LE Create Connection failed */
			if (conn->err) {
				notify_connected(conn);
			}

			bt_conn_unref(conn);
		}

		break;
	case BT_CONN_CONNECT_SCAN:
		break;
	case BT_CONN_CONNECT:
		if (conn->type == BT_CONN_TYPE_SCO) {
			break;
		}
		/*
		 * Timer is needed only for LE. For other link types controller
		 * will handle connection timeout.
		 */
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    conn->type == BT_CONN_TYPE_LE) {
			k_delayed_work_submit(&conn->le.update_work,
					      CONN_TIMEOUT);
		}

		break;
	case BT_CONN_DISCONNECT:
		break;
	default:
		BT_WARN("no valid (%u) state was set", state);

		break;
	}
}

struct bt_conn *bt_conn_lookup_handle(u16_t handle)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		/* We only care about connections with a valid handle */
		if (conns[i].state != BT_CONN_CONNECTED &&
		    conns[i].state != BT_CONN_DISCONNECT) {
			continue;
		}

		if (conns[i].handle == handle) {
			return bt_conn_ref(&conns[i]);
		}
	}

#if defined(CONFIG_BT_BREDR)
	for (i = 0; i < bt_inner_value.br_max_conn; i++) {
		if (!atomic_get(&sco_conns[i].ref)) {
			continue;
		}

		/* We only care about connections with a valid handle */
		if (sco_conns[i].state != BT_CONN_CONNECTED &&
		    sco_conns[i].state != BT_CONN_DISCONNECT) {
			continue;
		}

		if (sco_conns[i].handle == handle) {
			return bt_conn_ref(&sco_conns[i]);
		}
	}
#endif

	return NULL;
}

int bt_conn_addr_le_cmp(const struct bt_conn *conn, const bt_addr_le_t *peer)
{
	/* Check against conn dst address as it may be the identity address */
	if (!bt_addr_le_cmp(peer, &conn->le.dst)) {
		return 0;
	}

	/* Check against initial connection address */
	if (conn->role == BT_HCI_ROLE_MASTER) {
		return bt_addr_le_cmp(peer, &conn->le.resp_addr);
	}

	return bt_addr_le_cmp(peer, &conn->le.init_addr);
}

struct bt_conn *bt_conn_lookup_addr_le(const bt_addr_le_t *peer)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].type != BT_CONN_TYPE_LE) {
			continue;
		}

		if (!bt_conn_addr_le_cmp(&conns[i], peer)) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_state_le(const bt_addr_le_t *peer,
					const bt_conn_state_t state)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].type != BT_CONN_TYPE_LE) {
			continue;
		}

		if (peer && bt_conn_addr_le_cmp(&conns[i], peer)) {
			continue;
		}

		if (conns[i].state == state) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

void bt_conn_disconnect_all(void)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		struct bt_conn *conn = &conns[i];

		if (!atomic_get(&conn->ref)) {
			continue;
		}

		if (conn->state == BT_CONN_CONNECTED) {
			bt_conn_disconnect(conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_inc(&conn->ref);

	BT_DBG("handle %u ref %u", conn->handle, atomic_get(&conn->ref));

	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	BT_ASSERT((atomic_get(&conn->ref) > 0));
	atomic_dec(&conn->ref);

	BT_DBG("handle %u ref %u", conn->handle, atomic_get(&conn->ref));
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	return &conn->le.dst;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	info->type = conn->type;
	info->role = conn->role;

	switch (conn->type) {
	case BT_CONN_TYPE_LE:
		if (conn->role == BT_HCI_ROLE_MASTER) {
			info->le.src = &conn->le.init_addr;
			info->le.dst = &conn->le.resp_addr;
		} else {
			info->le.src = &conn->le.resp_addr;
			info->le.dst = &conn->le.init_addr;
		}
		info->le.interval = conn->le.interval;
		info->le.latency = conn->le.latency;
		info->le.timeout = conn->le.timeout;
		return 0;
#if defined(CONFIG_BT_BREDR)
	case BT_CONN_TYPE_BR:
		info->br.dst = &conn->br.dst;
		return 0;
	case BT_CONN_TYPE_SCO:
		info->br.dst = &conn->sco.acl->br.dst;
		return 0;
#endif
	}

	return -EINVAL;
}

u8_t bt_conn_get_type(const struct bt_conn *conn)
{
	return conn->type;
}

u16_t bt_conn_get_acl_handle(const struct bt_conn *conn)
{
	return conn->handle;
}

struct bt_conn *bt_conn_get_acl_conn_by_sco(const struct bt_conn *sco_conn)
{
	return sco_conn->sco.acl;
}

static int bt_hci_disconnect(struct bt_conn *conn, u8_t reason)
{
	struct net_buf *buf;
	struct bt_hci_cp_disconnect *disconn;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_DISCONNECT, sizeof(*disconn));
	if (!buf) {
		return -ENOBUFS;
	}

	disconn = net_buf_add(buf, sizeof(*disconn));
	disconn->handle = sys_cpu_to_le16(conn->handle);
	disconn->reason = reason;

	err = bt_hci_cmd_send(BT_HCI_OP_DISCONNECT, buf);
	if (err) {
		return err;
	}

	bt_conn_set_state(conn, BT_CONN_DISCONNECT);

	return 0;
}

int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
	BT_DBG("conn %p features 0x%02x params (%d-%d %d %d)", conn,
	       conn->le.features[0], param->interval_min,
	       param->interval_max, param->latency, param->timeout);

	/* Check if there's a need to update conn params */
	if (conn->le.interval >= param->interval_min &&
	    conn->le.interval <= param->interval_max &&
	    conn->le.latency == param->latency &&
	    conn->le.timeout == param->timeout) {
		return -EALREADY;
	}

	/* Cancel any pending update */
	k_delayed_work_cancel(&conn->le.update_work);

	/* Use LE connection parameter request if both local and remote support
	 * it; or if local role is master then use LE connection update.
	 */
	if ((BT_FEAT_LE_CONN_PARAM_REQ_PROC(bt_dev.le.features) &&
	     BT_FEAT_LE_CONN_PARAM_REQ_PROC(conn->le.features)) ||
	    (conn->role == BT_HCI_ROLE_MASTER)) {
		return bt_conn_le_conn_update(conn, param);
	}

	/* If remote master does not support LL Connection Parameters Request
	 * Procedure
	 */
	return bt_l2cap_update_conn_param(conn, param);
}

int bt_conn_disconnect(struct bt_conn *conn, u8_t reason)
{
	/* Disconnection is initiated by us, so auto connection shall
	 * be disabled. Otherwise the passive scan would be enabled
	 * and we could send LE Create Connection as soon as the remote
	 * starts advertising.
	 */
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->type == BT_CONN_TYPE_LE) {
		bt_le_set_auto_conn(&conn->le.dst, NULL);
	}

	switch (conn->state) {
	case BT_CONN_CONNECT_SCAN:
		conn->err = reason;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_le_scan_update(false);
		return 0;
	case BT_CONN_CONNECT:
#if defined(CONFIG_BT_BREDR)
		if (conn->type == BT_CONN_TYPE_BR) {
			return bt_hci_connect_br_cancel(conn);
		}
#endif /* CONFIG_BT_BREDR */

		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			k_delayed_work_cancel(&conn->le.update_work);
			return bt_hci_cmd_send(BT_HCI_OP_LE_CREATE_CONN_CANCEL,
					       NULL);
		}

		return 0;
	case BT_CONN_CONNECTED:
		return bt_hci_disconnect(conn, reason);
	case BT_CONN_DISCONNECT:
		return 0;
	case BT_CONN_DISCONNECTED:
	default:
		return -ENOTCONN;
	}
}

#if defined(CONFIG_BT_CENTRAL)
static void bt_conn_set_param_le(struct bt_conn *conn,
				 const struct bt_le_conn_param *param)
{
	conn->le.interval_max = param->interval_max;
	conn->le.latency = param->latency;
	conn->le.timeout = param->timeout;
}

struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
				  const struct bt_le_conn_param *param)
{
	struct bt_conn *conn;

	if (!bt_le_conn_params_valid(param)) {
		return NULL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return NULL;
	}

	conn = bt_conn_lookup_addr_le(peer);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_CONNECT_SCAN:
			bt_conn_set_param_le(conn, param);
			return conn;
		case BT_CONN_CONNECT:
		case BT_CONN_CONNECTED:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}

	conn = bt_conn_add_le(peer);
	if (!conn) {
		return NULL;
	}

	/* Set initial address - will be updated later if necessary. */
	bt_addr_le_copy(&conn->le.resp_addr, peer);

	bt_conn_set_param_le(conn, param);

	bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);

	bt_le_scan_update(true);

	return conn;
}

int bt_le_set_auto_conn(bt_addr_le_t *addr,
			const struct bt_le_conn_param *param)
{
	struct bt_conn *conn;

	if (param && !bt_le_conn_params_valid(param)) {
		return -EINVAL;
	}

	conn = bt_conn_lookup_addr_le(addr);
	if (!conn) {
		conn = bt_conn_add_le(addr);
		if (!conn) {
			return -ENOMEM;
		}
	}

	if (param) {
		bt_conn_set_param_le(conn, param);

		if (!atomic_test_and_set_bit(conn->flags,
					     BT_CONN_AUTO_CONNECT)) {
			bt_conn_ref(conn);
		}
	} else {
		if (atomic_test_and_clear_bit(conn->flags,
					      BT_CONN_AUTO_CONNECT)) {
			bt_conn_unref(conn);
			if (conn->state == BT_CONN_CONNECT_SCAN) {
				bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
			}
		}
	}

	if (conn->state == BT_CONN_DISCONNECTED &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		if (param) {
			bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
		}
		bt_le_scan_update(false);
	}

	bt_conn_unref(conn);

	return 0;
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
struct bt_conn *bt_conn_create_slave_le(const bt_addr_le_t *peer,
					const struct bt_le_adv_param *param)
{
	return NULL;
}
#endif /* CONFIG_BT_PERIPHERAL */

int bt_conn_le_conn_update(struct bt_conn *conn,
			   const struct bt_le_conn_param *param)
{
	struct hci_cp_le_conn_update *conn_update;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_UPDATE,
				sizeof(*conn_update));
	if (!buf) {
		return -ENOBUFS;
	}

	conn_update = net_buf_add(buf, sizeof(*conn_update));
	memset(conn_update, 0, sizeof(*conn_update));
	conn_update->handle = sys_cpu_to_le16(conn->handle);
	conn_update->conn_interval_min = sys_cpu_to_le16(param->interval_min);
	conn_update->conn_interval_max = sys_cpu_to_le16(param->interval_max);
	conn_update->conn_latency = sys_cpu_to_le16(param->latency);
	conn_update->supervision_timeout = sys_cpu_to_le16(param->timeout);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_UPDATE, buf);
}

struct net_buf *bt_conn_create_pdu(struct net_buf_pool *pool, size_t reserve)
{
	struct net_buf *buf;

	if (!pool) {
		pool = &acl_tx_pool;
	}

	buf = net_buf_alloc(pool, K_FOREVER);
	__ASSERT_NO_MSG(buf);

	reserve += sizeof(struct bt_hci_acl_hdr) + CONFIG_BT_HCI_RESERVE;
	net_buf_reserve(buf, reserve);

	return buf;
}

struct net_buf *bt_conn_create_pdu_len(struct net_buf_pool *pool, size_t reserve, u16_t len)
{
	struct net_buf *buf;
	u16_t malloc_len;

	if (!pool) {
		pool = &acl_tx_pool;
	}

	malloc_len = len + sizeof(struct bt_hci_acl_hdr) + CONFIG_BT_HCI_RESERVE;
	buf = net_buf_alloc_len(pool, malloc_len, K_FOREVER);
	__ASSERT_NO_MSG(buf);

	reserve += sizeof(struct bt_hci_acl_hdr) + CONFIG_BT_HCI_RESERVE;
	net_buf_reserve(buf, reserve);

	return buf;
}

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
int bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
	if (!cb) {
		bt_auth = NULL;
		return 0;
	}

	/* cancel callback should always be provided */
	if (!cb->cancel) {
		return -EINVAL;
	}

	if (bt_auth) {
		return -EALREADY;
	}

	bt_auth = cb;
	return 0;
}

int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	if (!bt_auth) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		bt_smp_auth_passkey_entry(conn, passkey);
		return 0;
	}

#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		/* User entered passkey, reset user state. */
		if (!atomic_test_and_clear_bit(conn->flags, BT_CONN_USER)) {
			return -EPERM;
		}

		if (conn->br.pairing_method == PASSKEY_INPUT) {
			return ssp_passkey_reply(conn, passkey);
		}
	}
#endif /* CONFIG_BT_BREDR */

	return -EINVAL;
}

int bt_conn_auth_passkey_confirm(struct bt_conn *conn)
{
	if (!bt_auth) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_SMP) &&
	    conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_passkey_confirm(conn);
	}

#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		/* Allow user confirm passkey value, then reset user state. */
		if (!atomic_test_and_clear_bit(conn->flags, BT_CONN_USER)) {
			return -EPERM;
		}

		return ssp_confirm_reply(conn);
	}
#endif /* CONFIG_BT_BREDR */

	return -EINVAL;
}

int bt_conn_auth_cancel(struct bt_conn *conn)
{
	if (!bt_auth) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_cancel(conn);
	}

#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		/* Allow user cancel authentication, then reset user state. */
		if (!atomic_test_and_clear_bit(conn->flags, BT_CONN_USER)) {
			return -EPERM;
		}

		switch (conn->br.pairing_method) {
		case JUST_WORKS:
		case PASSKEY_CONFIRM:
			return ssp_confirm_neg_reply(conn);
		case PASSKEY_INPUT:
			return ssp_passkey_neg_reply(conn);
		case PASSKEY_DISPLAY:
			return bt_conn_disconnect(conn,
						  BT_HCI_ERR_AUTHENTICATION_FAIL);
		case LEGACY:
			return pin_code_neg_reply(&conn->br.dst);
		default:
			break;
		}
	}
#endif /* CONFIG_BT_BREDR */

	return -EINVAL;
}

int bt_conn_auth_pairing_confirm(struct bt_conn *conn)
{
	if (!bt_auth) {
		return -EINVAL;
	}

	switch (conn->type) {
#if defined(CONFIG_BT_SMP)
	case BT_CONN_TYPE_LE:
		return bt_smp_auth_pairing_confirm(conn);
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_BREDR)
	case BT_CONN_TYPE_BR:
		return ssp_confirm_reply(conn);
#endif /* CONFIG_BT_BREDR */
	default:
		return -EINVAL;
	}
}
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

#if defined(CONFIG_BT_BREDR)
int bt_conn_role_discovery(struct bt_conn *conn, u8_t *role)
{
	int err;
	struct bt_hci_cp_role_discovery *rp;
	struct net_buf *buf;
	struct net_buf *rsp;

	if (conn->type != BT_CONN_TYPE_BR ||
		conn->state != BT_CONN_CONNECTED) {
		return -EIO;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_ROLE_DISCOVERY, sizeof(u16_t));
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_le16(buf, conn->handle);
	err = bt_hci_cmd_send_sync(BT_HCI_OP_ROLE_DISCOVERY, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	*role = rp->role;
	net_buf_unref(rsp);

	return 0;
}

int bt_conn_switch_role(struct bt_conn *conn, u8_t role)
{
	struct bt_hci_cp_switch_role *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_SWITCH_ROLE, sizeof(*cp));
	if (!buf) {
		return -ENOMEM;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	memset(cp, 0, sizeof(*cp));
	memcpy(&cp->bdaddr, &conn->br.dst, sizeof(cp->bdaddr));
	cp->role = role;

	return bt_hci_cmd_send(BT_HCI_OP_SWITCH_ROLE, buf);
}

int bt_conn_set_supervision_timeout(struct bt_conn *conn, u16_t timeout)
{
	struct bt_hci_cp_write_link_supervision_timeout *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_LINK_SUPERVISION_TIMEOUT, sizeof(*cp));
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_le16(buf, conn->handle);
	net_buf_add_le16(buf, timeout);

	return bt_hci_cmd_send(BT_HCI_OP_WRITE_LINK_SUPERVISION_TIMEOUT, buf);
}

static int bt_conn_enter_sniff(struct bt_conn *conn)
{
	struct bt_hci_cp_sniff_mode *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_SNIFF_MODE, sizeof(*cp));
	if (!buf) {
		return -ENOMEM;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->max_interval = sys_cpu_to_le16(SNIFF_MAX_INTERVAL);
	cp->min_interval = sys_cpu_to_le16(SNIFF_MIN_INTERVAL);
	cp->attempt = sys_cpu_to_le16(SNIFF_ATTEMPT);
	cp->timeout = sys_cpu_to_le16(SNIFF_TIMEOUT);

	return bt_hci_cmd_send(BT_HCI_OP_SNIFF_MODE, buf);
}

static int bt_conn_exit_sniff(struct bt_conn *conn)
{
	struct bt_hci_cp_exit_sniff_mode *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_EXIT_SNIFF_MODE, sizeof(*cp));
	if (!buf) {
		return -ENOMEM;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	return bt_hci_cmd_send(BT_HCI_OP_EXIT_SNIFF_MODE, buf);
}

void bt_conn_force_exit_sniff(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		struct bt_conn *s_conn = &conns[i];

		if (!bt_conn_enable_sniff_check) {
			break;
		}

		if (!atomic_get(&s_conn->ref) || (s_conn != conn)) {
			continue;
		}

		if (conn->type == BT_CONN_TYPE_BR) {
			/* Clear idle count, need exit  if in sniff entering state */
			conn->br.idle_cnt = 0;
			if ((conn->br.in_sniff_mode && (!conn->br.sniff_exiting)) ||
				conn->br.sniff_entering) {
				conn->br.sniff_exiting = 1;
				bt_conn_exit_sniff(conn);
			}
		}
	}
}

bool bt_conn_is_in_sniff(struct bt_conn *conn)
{
	return (conn->br.in_sniff_mode) ? true : false;
}

int bt_conn_send_connectionless_data(struct bt_conn *conn, u8_t *data, u16_t len)
{
	struct net_buf *buf;

	if (conn->type != BT_CONN_TYPE_BR ||
		conn->state != BT_CONN_CONNECTED) {
		return -EIO;
	}

	if (len > CONFIG_BT_L2CAP_TX_MTU) {
		return -EIO;
	}

	buf = bt_l2cap_create_pdu(NULL, 0);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, data, len);
	bt_l2cap_br_connectionless_send(conn, buf);

	return 0;
}
#endif

int bt_conn_send_sco_data(struct bt_conn *conn, u8_t *data, u8_t len)
{
	int err;
	struct net_buf *buf;
	struct bt_hci_sco_hdr *hdr;

	if (conn->type != BT_CONN_TYPE_SCO ||
		conn->state != BT_CONN_CONNECTED) {
		return -EIO;
	}

	if (len > CONFIG_BT_L2CAP_TX_MTU) {
		return -EIO;
	}

	buf = net_buf_alloc(&acl_tx_pool, K_NO_WAIT);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_reserve(buf, sizeof(*hdr));
	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(conn->handle, 0x0));
	hdr->len = len;
	net_buf_add_mem(buf, data, len);

	bt_buf_set_type(buf, BT_BUF_SCO_OUT);
	err = bt_send(buf);
	if (err) {
		BT_ERR("Unable to send to driver (err %d)", err);
		net_buf_unref(buf);
	}

	return err;
}

static void bt_conn_env_init(void)
{
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	bt_auth = NULL;
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

	memset(conns, 0, (sizeof(struct bt_conn)*bt_inner_value.max_conn));
	callback_list = NULL;
	memset(conn_tx_link, 0, (sizeof(struct bt_conn_tx)*bt_inner_value.acl_tx_max));
	sys_slist_init(&free_tx);

#if defined(CONFIG_BT_BREDR)
	memset(sco_conns, 0, sizeof(struct bt_conn)*bt_inner_value.br_max_conn);
#endif /* CONFIG_BT_BREDR */

	k_poll_signal_init(&conn_change);
}

static void bt_sniff_check_work(struct k_work *work)
{
	int i;

	for (i = 0; i < bt_inner_value.max_conn; i++) {
		struct bt_conn *conn = &conns[i];

		if (!bt_conn_enable_sniff_check) {
			break;
		}

		if (!atomic_get(&conn->ref)) {
			continue;
		}

		if ((conn->type == BT_CONN_TYPE_BR) &&
			(conn->state == BT_CONN_CONNECTED) &&
			(conn->role == BT_HCI_ROLE_MASTER) &&
			(!conn->br.in_sniff_mode) && (!conn->br.sniff_entering)) {
			if (conn->br.pre_conn_rxtx_cnt == conn->br.conn_rxtx_cnt) {
				conn->br.idle_cnt++;
				if (conn->br.idle_cnt >= SNIFF_ENTER_IDLE_CNT) {
					conn->br.sniff_entering = 1;
					bt_conn_enter_sniff(conn);
				}
			} else {
				conn->br.pre_conn_rxtx_cnt = conn->br.conn_rxtx_cnt;
				conn->br.idle_cnt = 0;
			}
		}
	}

	k_delayed_work_submit(&sniff_mode_work, SNIFF_WORK_INTERVAL);
}

void bt_conn_set_sniff_check_enable(bool enable)
{
	bt_conn_enable_sniff_check = enable ? 1 : 0;
}

bool bt_conn_is_br_acl_send_block(struct bt_conn *conn)
{
	bool ret = true;

	if (conn->type != BT_CONN_TYPE_BR ||
		conn->state != BT_CONN_CONNECTED) {
		goto exit_check;
	}

	if (k_fifo_cnt_sum(&conn->tx_queue) < (bt_dev.br.pkts_num - CONN_TX_PKT_RESERVE)) {
		ret = false;
	}

exit_check:
	return ret;
}

int bt_conn_init(void)
{
	int err, i;

	bt_conn_env_init();

	for (i = 0; i < bt_inner_value.acl_tx_max; i++) {
		sys_slist_prepend(&free_tx, &conn_tx_link[i].node);
	}

	/* bt_att_init will register channel to l2cap,
	 * so initialize bt_l2cap_init before bt_att_init
	 */
	bt_l2cap_init();

#ifdef CONFIG_BT_LE_ATT
	bt_att_init();
#endif

	err = bt_smp_init();
	if (err) {
		return err;
	}

	/* Initialize background scan */
	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		for (i = 0; i < bt_inner_value.max_conn; i++) {
			struct bt_conn *conn = &conns[i];

			if (!atomic_get(&conn->ref)) {
				continue;
			}

			if (atomic_test_bit(conn->flags,
					    BT_CONN_AUTO_CONNECT)) {
				bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
			}
		}
	}

	k_delayed_work_init(&sniff_mode_work, bt_sniff_check_work);
	k_delayed_work_submit(&sniff_mode_work, SNIFF_WORK_INTERVAL);
	bt_conn_enable_sniff_check = 1;

	return 0;
}
