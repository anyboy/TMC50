/** @file
 * @brief DHCPv4 client related functions
 */

/*
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_DHCPV4)
#define SYS_LOG_DOMAIN "net/dhcpv4"
#define NET_LOG_ENABLED 1
#endif

#include "dhcpv4.h"

#include <errno.h>
#include <inttypes.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include "net_private.h"

#include <net/udp.h>
#include "udp_internal.h"
#include <net/dhcpv4.h>

#ifdef NET_DHCPV4_CACHED_PRE_IP
#include <nvram_config.h>
#endif

struct dhcp_msg {
	u8_t op;		/* Message type, 1:BOOTREQUEST, 2:BOOTREPLY */
	u8_t htype;		/* Hardware Address Type */
	u8_t hlen;		/* Hardware Address length */
	u8_t hops;		/* used by relay agents when booting via relay
				 * agent, client sets zero
				 */
	u32_t xid;		/* Transaction ID, random number */
	u16_t secs;		/* Seconds elapsed since client began address
				 * acquisition or renewal process
				 */
	u16_t flags;		/* Broadcast or Unicast */
	u8_t ciaddr[4];	/* Client IP Address */
	u8_t yiaddr[4];	/* your (client) IP address */
	u8_t siaddr[4];	/* IP address of next server to use in bootstrap
				 * returned in DHCPOFFER, DHCPACK by server
				 */
	u8_t giaddr[4];	/* Relat agent IP address */
	u8_t chaddr[16];	/* Client hardware address */
} __packed;

#define SIZE_OF_SNAME	64
#define SIZE_OF_FILE	128

#define DHCPV4_MSG_BROADCAST	0x8000
#define DHCPV4_MSG_UNICAST	0x0000

#define DHCPV4_MSG_BOOT_REQUEST	1
#define DHCPV4_MSG_BOOT_REPLY	2

#define HARDWARE_ETHERNET_TYPE 1
#define HARDWARE_ETHERNET_LEN  6

#define DHCPV4_SERVER_PORT  67
#define DHCPV4_CLIENT_PORT  68

/* These enumerations represent RFC2131 defined msy type codes, hence
 * they should not be renumbered.
 */
enum dhcpv4_msg_type {
	DHCPV4_MSG_TYPE_DISCOVER	= 1,
	DHCPV4_MSG_TYPE_OFFER		= 2,
	DHCPV4_MSG_TYPE_REQUEST		= 3,
	DHCPV4_MSG_TYPE_DECLINE		= 4,
	DHCPV4_MSG_TYPE_ACK		= 5,
	DHCPV4_MSG_TYPE_NAK		= 6,
	DHCPV4_MSG_TYPE_RELEASE		= 7,
	DHCPV4_MSG_TYPE_INFORM		= 8,
};

#define DHCPV4_OPTIONS_PAD			0
#define DHCPV4_OPTIONS_SUBNET_MASK	1
#define DHCPV4_OPTIONS_ROUTER		3
#define DHCPV4_OPTIONS_DNS_SERVER	6
#define DHCPV4_OPTIONS_REQ_IPADDR	50
#define DHCPV4_OPTIONS_LEASE_TIME	51
#define DHCPV4_OPTIONS_MSG_TYPE		53
#define DHCPV4_OPTIONS_SERVER_ID	54
#define DHCPV4_OPTIONS_REQ_LIST		55
#define DHCPV4_OPTION_MAX_MSG_SIZE	57
#define DHCPV4_OPTIONS_RENEWAL		58
#define DHCPV4_OPTIONS_REBINDING	59
#define DHCPV4_OPTIONS_END		255

/* TODO:
 * 1) Support for UNICAST flag (some dhcpv4 servers will not reply if
 *    DISCOVER message contains BROADCAST FLAG).
 * 2) Support T2(Rebind) timer.
 */

/* Maximum number of REQUEST or RENEWAL retransmits before reverting
 * to DISCOVER.
 */
#define DHCPV4_MAX_NUMBER_OF_ATTEMPTS	6	//  3

/* For some ap, dhcpv4 reply xid not equal requet xid */
#define DHCPV4_MAX_DIFF_XID_THRESHOLD	(DHCPV4_MAX_NUMBER_OF_ATTEMPTS/2)
//#define DHCPV4_USED_RELEASE				1

/* Initial message retry timeout (s).  This timeout increases
 * exponentially on each retransmit.
 * RFC2131 4.1
 */
#define DHCPV4_INITIAL_RETRY_TIMEOUT 1 // 4

/* Initial minimum and maximum delay in INIT state before sending the
 * initial DISCOVER message.
 * RFC2131 4.1.1
 */
#define DHCPV4_INITIAL_DELAY_MIN 1
#define DHCPV4_INITIAL_DELAY_MAX 3

/* RFC 1497 [17] */
static const u8_t magic_cookie[4] = { 0x63, 0x82, 0x53, 0x63 };

/* For some ap, dhcpv4 reply xid not equal requet xid */
static u8_t dhcpv4_xid_diff_cnt;

#ifdef NET_DHCPV4_CACHED_PRE_IP
#define DHCP_SSID_LEN 32

#define DHCP_CACHED_SSID_NAME	"DHCP_SSID"
#define DHCP_CACHED_IP_NAME		"DHCP_CACHED"

#define DHCPV4_REQUEST_PREIP_ATTEMPTS	2
#define DHCPV4_REQUEST_PREIP_TIMEOUT	1

struct dhcp_cached_ip {
	struct in_addr server_id;
	struct in_addr requested_ip;
	struct in_addr gw;
	struct in_addr netmask;
};

static int send_request_preip(struct net_if *iface);
#endif

extern void dns_update_server_addr(struct in_addr *dns_server);
static void dhcpv4_timeout(struct k_work *work);

static const char *
net_dhcpv4_msg_type_name(enum dhcpv4_msg_type msg_type) __attribute__((unused));

const char *net_dhcpv4_state_name(enum net_dhcpv4_state state)
{
	static const char * const name[] = {
		"disabled",
		"init",
		"selecting",
		"requesting",
		"renewing",
		"rebinding",
		"bound",
#ifdef NET_DHCPV4_CACHED_PRE_IP
		"reqpreip",
#endif
	};

	__ASSERT_NO_MSG(state >= 0 && state < sizeof(name));
	return name[state];
}

static const char *
net_dhcpv4_msg_type_name(enum dhcpv4_msg_type msg_type)
{
	static const char * const name[] = {
		"discover",
		"offer",
		"request",
		"decline",
		"ack",
		"nak",
		"release",
		"inform"
	};

	__ASSERT_NO_MSG(msg_type >= 1 && msg_type <= sizeof(name));
	return name[msg_type - 1];
}

/* Add magic cookie to DCHPv4 messages */
static inline bool add_cookie(struct net_pkt *pkt)
{
	return net_pkt_append_all(pkt, sizeof(magic_cookie),
			      magic_cookie, K_FOREVER);
}

/* Add a an option with the form OPTION LENGTH VALUE.  */
static bool add_option_length_value(struct net_pkt *pkt, u8_t option,
				    u8_t size, const u8_t *value)
{
	if (!net_pkt_append_u8(pkt, option)) {
		return false;
	}

	if (!net_pkt_append_u8(pkt, size)) {
		return false;
	}

	if (!net_pkt_append_all(pkt, size, value, K_FOREVER)) {
		return false;
	}

	return true;
}

/* Add DHCPv4 message type */
static bool add_msg_type(struct net_pkt *pkt, u8_t type)
{
	return add_option_length_value(pkt, DHCPV4_OPTIONS_MSG_TYPE, 1, &type);
}

/* Add DHCPv4 minimum required options for server to reply.
 * Can be added more if needed.
 */
static bool add_req_options(struct net_pkt *pkt)
{
	static const u8_t data[5] = { DHCPV4_OPTIONS_REQ_LIST,
					 3, /* Length */
					 DHCPV4_OPTIONS_SUBNET_MASK,
					 DHCPV4_OPTIONS_ROUTER,
					 DHCPV4_OPTIONS_DNS_SERVER };

	return net_pkt_append_all(pkt, sizeof(data), data, K_FOREVER);
}

static bool add_max_msg_size(struct net_pkt *pkt)
{
	static const u8_t data[4] = { DHCPV4_OPTION_MAX_MSG_SIZE,
					2,          /* Length */
					0x05,       /* max msg size 0x05b4(1460) */
					0xB4};

	return net_pkt_append_all(pkt, sizeof(data), data, K_FOREVER);
}

static bool add_server_id(struct net_pkt *pkt, const struct in_addr *addr)
{
	return add_option_length_value(pkt, DHCPV4_OPTIONS_SERVER_ID, 4,
				       addr->s4_addr);
}

static bool add_req_ipaddr(struct net_pkt *pkt, const struct in_addr *addr)
{
	return add_option_length_value(pkt, DHCPV4_OPTIONS_REQ_IPADDR, 4,
				       addr->s4_addr);
}

/* Add DHCPv4 Options end, rest of the message can be padded wit zeros */
static inline bool add_end(struct net_pkt *pkt)
{
	return net_pkt_append_u8(pkt, DHCPV4_OPTIONS_END);
}

/* File is empty ATM */
static inline bool add_file(struct net_pkt *pkt)
{
	u8_t len = SIZE_OF_FILE;

	while (len-- > 0) {
		if (!net_pkt_append_u8(pkt, 0)) {
			return false;
		}
	}

	return true;
}

/* SNAME is empty ATM */
static inline bool add_sname(struct net_pkt *pkt)
{
	u8_t len = SIZE_OF_SNAME;

	while (len-- > 0) {
		if (!net_pkt_append_u8(pkt, 0)) {
			return false;
		}
	}

	return true;
}

/* Setup IPv4 + UDP header */
static void setup_header(struct net_pkt *pkt, const struct in_addr *server_addr)
{
	struct net_ipv4_hdr *ipv4;
	struct net_udp_hdr hdr, *udp;
	u16_t len;

	ipv4 = NET_IPV4_HDR(pkt);

	udp = net_udp_get_hdr(pkt, &hdr);
	NET_ASSERT(udp && udp != &hdr);

	len = net_pkt_get_len(pkt);

	/* Setup IPv4 header */
	memset(ipv4, 0, sizeof(struct net_ipv4_hdr));

	ipv4->vhl = 0x45;
	ipv4->ttl = 0xFF;
	ipv4->proto = IPPROTO_UDP;
	ipv4->len[0] = len >> 8;
	ipv4->len[1] = (u8_t)len;
	ipv4->chksum = ~net_calc_chksum_ipv4(pkt);

	net_ipaddr_copy(&ipv4->dst, server_addr);

	len -= NET_IPV4H_LEN;
	/* Setup UDP header */
	udp->src_port = htons(DHCPV4_CLIENT_PORT);
	udp->dst_port = htons(DHCPV4_SERVER_PORT);
	udp->len = htons(len);
	udp->chksum = 0;
	udp->chksum = ~net_calc_chksum_udp(pkt);

	net_udp_set_hdr(pkt, udp);
}

/* Prepare initial DHCPv4 message and add options as per message type */
static struct net_pkt *prepare_message(struct net_if *iface, u8_t type,
				       const struct in_addr *ciaddr)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct dhcp_msg *msg;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				     K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		goto fail;
	}

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	net_pkt_frag_add(pkt, frag);

	/* Leave room for IPv4 + UDP headers */
	net_buf_add(pkt->frags, NET_IPV4UDPH_LEN);

	if (net_buf_tailroom(frag) < sizeof(struct dhcp_msg)) {
		goto fail;
	}

	msg = (struct dhcp_msg *)(frag->data + NET_IPV4UDPH_LEN);
	memset(msg, 0, sizeof(struct dhcp_msg));

	msg->op = DHCPV4_MSG_BOOT_REQUEST;

	msg->htype = HARDWARE_ETHERNET_TYPE;
	msg->hlen = HARDWARE_ETHERNET_LEN;

	msg->xid = htonl(iface->dhcpv4.xid);
	msg->flags = htons(DHCPV4_MSG_UNICAST);

	if (ciaddr) {
		/* The ciaddr field was zero'd out above, if we are
		 * asked to send a ciaddr then fill it in now
		 * otherwise leave it as all zeros.
		 */
		memcpy(msg->ciaddr, ciaddr, 4);
	}

	memcpy(msg->chaddr, iface->link_addr.addr, iface->link_addr.len);

	net_buf_add(frag, sizeof(struct dhcp_msg));

	if (!add_sname(pkt) ||
	    !add_file(pkt) ||
	    !add_cookie(pkt) ||
	    !add_msg_type(pkt, type)) {
		goto fail;
	}

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

/* Prepare DHCPv4 Message request and send it to peer */
static void send_request(struct net_if *iface)
{
	struct net_pkt *pkt;
	u32_t timeout;
	const struct in_addr *server_addr = net_ipv4_broadcast_address();
	const struct in_addr *ciaddr = NULL;
	bool with_server_id = false;
	bool with_requested_ip = false;

	iface->dhcpv4.xid++;

	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_BOUND:
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
#endif
		/* Not possible */
		NET_ASSERT_INFO(0, "Invalid state %s",
				net_dhcpv4_state_name(iface->dhcpv4.state));
		break;
	case NET_DHCPV4_REQUESTING:
		with_server_id = true;
		with_requested_ip = true;
		break;
	case NET_DHCPV4_RENEWING:
		/* Since we have an address populate the ciaddr field.
		 */
		ciaddr = &iface->dhcpv4.requested_ip;

		/* UNICAST the DHCPREQUEST */
		server_addr = &iface->dhcpv4.server_id;

		/* RFC2131 4.4.5 Client MUST NOT include server
		 * identifier in the DHCPREQUEST.
		 */
		break;
	case NET_DHCPV4_REBINDING:
		/* Since we have an address populate the ciaddr field.
		 */
		ciaddr = &iface->dhcpv4.requested_ip;

		server_addr = net_ipv4_broadcast_address();
		break;
	}

	pkt = prepare_message(iface, DHCPV4_MSG_TYPE_REQUEST, ciaddr);
	if (!pkt) {
		goto fail;
	}

	if (with_server_id && !add_server_id(pkt, &iface->dhcpv4.server_id)) {
		goto fail;
	}

	if (with_requested_ip
	    && !add_req_ipaddr(pkt, &iface->dhcpv4.requested_ip)) {
		goto fail;
	}

	if (!add_max_msg_size(pkt) ||
		!add_req_options(pkt) ||
		!add_end(pkt)) {
		goto fail;
	}

	setup_header(pkt, server_addr);

	if (net_send_data(pkt) < 0) {
		goto fail;
	}

	timeout = (iface->dhcpv4.attempts > 5)? 60 : (DHCPV4_INITIAL_RETRY_TIMEOUT << iface->dhcpv4.attempts);
	//timeout = DHCPV4_INITIAL_RETRY_TIMEOUT;
	k_delayed_work_submit(&iface->dhcpv4.timer, timeout * MSEC_PER_SEC);
	iface->dhcpv4.attempts++;

	const char *ciaddr_buf = 0;
	static char pbuf[NET_IPV4_ADDR_LEN];

	if (ciaddr) {
		ciaddr_buf = net_addr_ntop(AF_INET, ciaddr, pbuf, sizeof(pbuf));
	} else {
		ciaddr_buf = "0.0.0.0";
	}

	NET_DBG("send request dst=%s xid=0x%"PRIx32" ciaddr=%s"
		"%s%s timeout=%"PRIu32"s",
		net_sprint_ipv4_addr(server_addr),
		iface->dhcpv4.xid,
		ciaddr_buf,
		with_server_id ? " +server-id" : "",
		with_requested_ip ? " +requested-ip" : "",
		timeout);

	return;

fail:
	NET_DBG("Message preparation failed");

	if (pkt) {
		net_pkt_unref(pkt);
	}

	/* Need to submit delay work when fail to send packet */
	timeout = DHCPV4_INITIAL_RETRY_TIMEOUT;
	k_delayed_work_submit(&iface->dhcpv4.timer, timeout * MSEC_PER_SEC);
	iface->dhcpv4.attempts++;
}

#ifdef DHCPV4_USED_RELEASE
static void send_release(struct net_if *iface)
{
	struct net_pkt *pkt;

	if ((iface->dhcpv4.server_id.s_addr == 0) ||
		(iface->dhcpv4.requested_ip.s_addr == 0)) {
		return;
	}

	iface->dhcpv4.xid++;
	pkt = prepare_message(iface, DHCPV4_MSG_TYPE_RELEASE, &iface->dhcpv4.requested_ip);
	if (!pkt) {
		goto fail;
	}

	if (!add_server_id(pkt, &iface->dhcpv4.server_id)) {
		goto fail;
	}

	if (!add_end(pkt)) {
		goto fail;
	}

	setup_header(pkt, net_ipv4_broadcast_address());

	if (net_send_data(pkt) < 0) {
		goto fail;
	}

	return;

fail:
	NET_DBG("Message preparation failed");

	if (pkt) {
		net_pkt_unref(pkt);
	}
}
#else
#define send_release(iface)
#endif

/* Prepare DHCPv4 Discover message and broadcast it */
static void send_discover(struct net_if *iface)
{
	struct net_pkt *pkt;
	u32_t timeout;

	iface->dhcpv4.xid++;

	pkt = prepare_message(iface, DHCPV4_MSG_TYPE_DISCOVER, NULL);
	if (!pkt) {
		goto fail;
	}

	if (!add_max_msg_size(pkt) ||
		!add_req_options(pkt) ||
		!add_end(pkt)) {
		goto fail;
	}

	setup_header(pkt, net_ipv4_broadcast_address());

	if (net_send_data(pkt) < 0) {
		goto fail;
	}

	timeout = (iface->dhcpv4.attempts > 5)? 60 : (DHCPV4_INITIAL_RETRY_TIMEOUT << iface->dhcpv4.attempts);
	//timeout = DHCPV4_INITIAL_RETRY_TIMEOUT;
	k_delayed_work_submit(&iface->dhcpv4.timer, timeout * MSEC_PER_SEC);
	iface->dhcpv4.attempts++;

	NET_DBG("send discover xid=0x%"PRIx32" timeout=%"PRIu32"s",
		iface->dhcpv4.xid, timeout);

	return;

fail:
	NET_DBG("Message preparation failed");

	if (pkt) {
		net_pkt_unref(pkt);
	}

	/* Need to submit next timeout handle when failed to send packet */
	timeout = DHCPV4_INITIAL_RETRY_TIMEOUT;
	k_delayed_work_submit(&iface->dhcpv4.timer, timeout * MSEC_PER_SEC);
	iface->dhcpv4.attempts++;
}

static void enter_selecting(struct net_if *iface)
{
	iface->dhcpv4.attempts = 0;

	iface->dhcpv4.lease_time = 0;
	iface->dhcpv4.renewal_time = 0;
	iface->dhcpv4.rebinding_time = 0;

	iface->dhcpv4.state = NET_DHCPV4_SELECTING;
	NET_DBG("enter state=%s",
		net_dhcpv4_state_name(iface->dhcpv4.state));

	send_release(iface);
	send_discover(iface);
}

static void enter_requesting(struct net_if *iface)
{
	iface->dhcpv4.attempts = 0;
	iface->dhcpv4.state = NET_DHCPV4_REQUESTING;
	NET_DBG("enter state=%s",
		net_dhcpv4_state_name(iface->dhcpv4.state));

	send_request(iface);
}

static void dhcpv4_t1_timeout(struct k_work *work)
{
	struct net_if *iface = CONTAINER_OF(work, struct net_if,
					    dhcpv4.t1_timer);

	NET_DBG("");

	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
#endif
		/* This path cannot happen. */
		NET_ASSERT_INFO(0, "Invalid state %s",
				net_dhcpv4_state_name(iface->dhcpv4.state));
		break;
	case NET_DHCPV4_BOUND:
		iface->dhcpv4.state = NET_DHCPV4_RENEWING;
		NET_DBG("enter state=%s",
			net_dhcpv4_state_name(iface->dhcpv4.state));
		iface->dhcpv4.attempts = 0;
		send_request(iface);
		break;
	}
}

static void dhcpv4_t2_timeout(struct k_work *work)
{
	struct net_if *iface = CONTAINER_OF(work, struct net_if,
					    dhcpv4.t2_timer);

	NET_DBG("");

	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_REBINDING:
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
#endif
		NET_ASSERT_INFO(0, "Invalid state %s",
				net_dhcpv4_state_name(iface->dhcpv4.state));
		break;
	case NET_DHCPV4_BOUND:
		/* If renewal time and rebinding time are
		 * misconfigured we may end up with T2 firing before
		 * T1.  Deal with it as though we had transitioned
		 * through RENEWAL already.
		 */
	case NET_DHCPV4_RENEWING:
		iface->dhcpv4.state = NET_DHCPV4_REBINDING;
		NET_DBG("enter state=%s",
			net_dhcpv4_state_name(iface->dhcpv4.state));
		iface->dhcpv4.attempts = 0;
		send_request(iface);
		break;
	}
}

static void enter_bound(struct net_if *iface)
{
	u32_t renewal_time;
	u32_t rebinding_time;

	k_delayed_work_cancel(&iface->dhcpv4.timer);

	renewal_time = iface->dhcpv4.renewal_time;
	if (!renewal_time) {
		/* The default renewal time rfc2131 4.4.5 */
		renewal_time = iface->dhcpv4.lease_time / 2;
	}

	rebinding_time = iface->dhcpv4.rebinding_time;
	if (!rebinding_time) {
		/* The default rebinding time rfc2131 4.4.5 */
		rebinding_time = iface->dhcpv4.lease_time * 875 / 1000;
	}

	iface->dhcpv4.state = NET_DHCPV4_BOUND;
	NET_DBG("enter state=%s renewal=%"PRIu32"s "
		"rebinding=%"PRIu32"s",
		net_dhcpv4_state_name(iface->dhcpv4.state),
		renewal_time, rebinding_time);

	/* Start renewal time */
	k_delayed_work_submit(&iface->dhcpv4.t1_timer,
			      renewal_time * MSEC_PER_SEC);

	/* Start rebinding time */
	k_delayed_work_submit(&iface->dhcpv4.t2_timer,
			      rebinding_time * MSEC_PER_SEC);
}

static void dhcpv4_timeout(struct k_work *work)
{
	struct net_if *iface = CONTAINER_OF(work, struct net_if,
					    dhcpv4.timer);

	NET_DBG("state=%s", net_dhcpv4_state_name(iface->dhcpv4.state));

	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
		break;
	case NET_DHCPV4_INIT:
		enter_selecting(iface);
		break;
	case NET_DHCPV4_SELECTING:
		/* Failed to get OFFER message, send DISCOVER again */
		send_release(iface);
		send_discover(iface);
		break;
	case NET_DHCPV4_REQUESTING:
		/* Maximum number of renewal attempts failed, so start
		 * from the beginning.
		 */
		if (iface->dhcpv4.attempts >= DHCPV4_MAX_NUMBER_OF_ATTEMPTS) {
			NET_DBG("too many attempts, restart");
			enter_selecting(iface);
		} else {
			send_request(iface);
		}

		break;
	case NET_DHCPV4_BOUND:
		break;
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
		if (iface->dhcpv4.attempts >= DHCPV4_MAX_NUMBER_OF_ATTEMPTS) {
			NET_DBG("too many attempts, restart");
			if (!net_if_ipv4_addr_rm(iface,
						 &iface->dhcpv4.requested_ip)) {
				NET_DBG("Failed to remove addr from iface");
			} else {
				iface->dhcpv4.add_flag = false;
			}
			/* Maximum number of renewal attempts failed, so start
			 * from the beginning.
			 */
			enter_selecting(iface);

		} else {
			send_request(iface);
		}
		break;
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
		if (iface->dhcpv4.attempts >= DHCPV4_REQUEST_PREIP_ATTEMPTS) {
			NET_DBG("too many attempts, restart");
			enter_selecting(iface);
		} else {
			if (0 != send_request_preip(iface)) {
				enter_selecting(iface);
			}
		}
		break;
#endif
	}
}

/* Parse DHCPv4 options and retrieve relavant information
 * as per RFC 2132.
 */
static enum net_verdict parse_options(struct net_if *iface,
				      struct net_buf *frag,
				      u16_t offset,
				      enum dhcpv4_msg_type *msg_type)
{
	u8_t cookie[4];
	u8_t length;
	u8_t type;
	u16_t pos;

	frag = net_frag_read(frag, offset, &pos, sizeof(magic_cookie),
			     (u8_t *)cookie);
	if (!frag || memcmp(magic_cookie, cookie, sizeof(magic_cookie))) {

		NET_DBG("Incorrect magic cookie");
		return NET_DROP;
	}

	while (frag) {
		frag = net_frag_read_u8(frag, pos, &pos, &type);

		if (type == DHCPV4_OPTIONS_PAD) {
			/* The pad option can be used to cause subsequent fields to align on
			 * word boundaries. The code for the pad option is 0, and its length is 1 octet.
			 */
			continue;
		}

		if (type == DHCPV4_OPTIONS_END) {
			NET_DBG("options_end");
			return NET_OK;
		}

		frag = net_frag_read_u8(frag, pos, &pos, &length);
		if (!frag) {
			NET_ERR("option parsing, bad length");
			return NET_DROP;
		}

		switch (type) {
		case DHCPV4_OPTIONS_SUBNET_MASK: {
			struct in_addr netmask;

			if (length != 4) {
				NET_ERR("options_subnet_mask, bad length");
				return NET_DROP;
			}

			frag = net_frag_read(frag, pos, &pos, length,
					     netmask.s4_addr);
			if (!frag && pos) {
				NET_ERR("options_subnet_mask, short packet");
				return NET_DROP;
			}
			net_if_ipv4_set_netmask(iface, &netmask);
			NET_DBG("options_subnet_mask %s",
				net_sprint_ipv4_addr(&netmask));
			break;
		}
		case DHCPV4_OPTIONS_ROUTER: {
			struct in_addr router;

			/* Router option may present 1 or more
			 * addresses for routers on the clients
			 * subnet.  Routers should be listed in order
			 * of preference.  Hence we choose the first
			 * and skip the rest.
			 */
			if (length % 4 != 0 || length < 4) {
				NET_ERR("options_router, bad length");
				return NET_DROP;
			}

			frag = net_frag_read(frag, pos, &pos, 4, router.s4_addr);
			frag = net_frag_skip(frag, pos, &pos, length - 4);
			if (!frag && pos) {
				NET_ERR("options_router, short packet");
				return NET_DROP;
			}

			NET_DBG("options_router: %s",
				net_sprint_ipv4_addr(&router));
			net_if_ipv4_set_gw(iface, &router);
			break;
		}
		case DHCPV4_OPTIONS_LEASE_TIME:
			if (length != 4) {
				NET_ERR("options_lease_time, bad length");
				return NET_DROP;
			}

			frag = net_frag_read_be32(frag, pos, &pos,
						  &iface->dhcpv4.lease_time);
			NET_DBG("options_lease_time: %u",
				iface->dhcpv4.lease_time);
			if (!iface->dhcpv4.lease_time) {
				return NET_DROP;
			}

			break;
		case DHCPV4_OPTIONS_RENEWAL:
			if (length != 4) {
				NET_DBG("options_renewal, bad length");
				return NET_DROP;
			}

			frag = net_frag_read_be32(frag, pos, &pos,
						  &iface->dhcpv4.renewal_time);
			NET_DBG("options_renewal: %u",
				iface->dhcpv4.renewal_time);
			if (!iface->dhcpv4.renewal_time) {
				return NET_DROP;
			}

			break;
		case DHCPV4_OPTIONS_REBINDING:
			if (length != 4) {
				NET_DBG("options_rebinding, bad length");
				return NET_DROP;
			}

			frag = net_frag_read_be32(frag, pos, &pos,
					&iface->dhcpv4.rebinding_time);
			NET_DBG("options_rebinding: %u",
				iface->dhcpv4.rebinding_time);
			if (!iface->dhcpv4.rebinding_time) {
				return NET_DROP;
			}

			break;
		case DHCPV4_OPTIONS_SERVER_ID:
			if (length != 4) {
				NET_DBG("options_server_id, bad length");
				return NET_DROP;
			}

			frag = net_frag_read(frag, pos, &pos, length,
					     iface->dhcpv4.server_id.s4_addr);
			NET_DBG("options_server_id: %s",
				net_sprint_ipv4_addr(&iface->dhcpv4.server_id));
			break;
		case DHCPV4_OPTIONS_MSG_TYPE: {
			u8_t v;

			if (length != 1) {
				NET_DBG("options_msg_type, bad length");
				return NET_DROP;
			}

			frag = net_frag_read_u8(frag, pos, &pos, &v);
			*msg_type = v;
			break;
		}
		case DHCPV4_OPTIONS_DNS_SERVER:
			if (length == 0 || length%4 != 0) {
				NET_DBG("options_dns_server, bad length:%d\n", length);
				frag = net_frag_skip(frag, pos, &pos, length);
				break;
			}

			frag = net_frag_read(frag, pos, &pos, 4,
					     iface->dhcpv4.dns_server.s4_addr);
			NET_DBG("options_dns_server: %s\n",
				net_sprint_ipv4_addr(&iface->dhcpv4.dns_server));
			if (length > 4) {
				frag = net_frag_skip(frag, pos, &pos, (length - 4));
			}
			break;
		default:
			NET_DBG("option unknown: %d", type);
			frag = net_frag_skip(frag, pos, &pos, length);
			break;
		}

		if (!frag && pos) {
			return NET_DROP;
		}
	}

	/* Invalid case: Options without DHCPV4_OPTIONS_END. */
	return NET_DROP;
}

static inline void handle_offer(struct net_if *iface)
{
	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
	case NET_DHCPV4_BOUND:
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
#endif
		break;
	case NET_DHCPV4_SELECTING:
		k_delayed_work_cancel(&iface->dhcpv4.timer);
		enter_requesting(iface);
		break;
	}
}

static void handle_ack(struct net_if *iface)
{
#ifdef NET_DHCPV4_CACHED_PRE_IP
	int ret;
	struct dhcp_cached_ip dhcp_ip;
#endif

	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_BOUND:
		break;
	case NET_DHCPV4_REQUESTING:
		NET_INFO("Received: %s",
			 net_sprint_ipv4_addr(&iface->dhcpv4.requested_ip));
		if (!net_if_ipv4_addr_add(iface,
					  &iface->dhcpv4.requested_ip,
					  NET_ADDR_DHCP,
					  iface->dhcpv4.lease_time)) {
			NET_DBG("Failed to add IPv4 addr to iface %p", iface);
			return;
		} else {
			iface->dhcpv4.add_flag = true;
		}

		dns_update_server_addr(&iface->dhcpv4.dns_server);

#ifdef NET_DHCPV4_CACHED_PRE_IP
		dhcp_ip.server_id.s_addr = iface->dhcpv4.server_id.s_addr;
		dhcp_ip.requested_ip.s_addr = iface->dhcpv4.requested_ip.s_addr;
		dhcp_ip.gw.s_addr = iface->ipv4.gw.s_addr;
		dhcp_ip.netmask.s_addr = iface->ipv4.netmask.s_addr;
	#ifdef CONFIG_NVRAM_CONFIG
		ret = nvram_config_set(DHCP_CACHED_IP_NAME, &dhcp_ip, sizeof(struct dhcp_cached_ip));
		if (ret) {
			SYS_LOG_ERR("Write %s failed", DHCP_CACHED_IP_NAME);
		}
	#endif
#endif
		enter_bound(iface);
		break;

	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
		/* TODO: If the renewal is success, update only
		 * vlifetime on iface.
		 */
		enter_bound(iface);
		break;
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
		if (!net_if_ipv4_addr_add(iface,
			&iface->dhcpv4.requested_ip, NET_ADDR_DHCP,
			iface->dhcpv4.lease_time)) {
			/* If run to here, will not request ip again, need to fix? */
			NET_DBG("Failed to add IPv4 addr to iface %p", iface);
			return;
		} else {
			iface->dhcpv4.add_flag = true;
		}

		dns_update_server_addr(&iface->dhcpv4.dns_server);

		if ((iface->dhcpv4.server_id.s_addr == 0) ||
			(iface->ipv4.gw.s_addr == 0) ||
			(iface->ipv4.netmask.s_addr)) {
		#ifdef CONFIG_NVRAM_CONFIG
			ret = nvram_config_get(DHCP_CACHED_IP_NAME, &dhcp_ip, sizeof(struct dhcp_cached_ip));
			if (ret < 0) {
				SYS_LOG_ERR("Read %s failed!", DHCP_CACHED_IP_NAME);
				send_release(iface);
				send_discover(iface);
				return;
			}
		#endif

			if (iface->dhcpv4.server_id.s_addr == 0) {
				iface->dhcpv4.server_id.s_addr = dhcp_ip.server_id.s_addr;
			}

			if (iface->ipv4.gw.s_addr == 0) {
				iface->ipv4.gw.s_addr = dhcp_ip.gw.s_addr;
			}

			if (iface->ipv4.netmask.s_addr == 0) {
				iface->ipv4.netmask.s_addr = dhcp_ip.netmask.s_addr;
			}
		}

		enter_bound(iface);
		break;
#endif
	}
}

static void handle_nak(struct net_if *iface)
{
	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_BOUND:
		break;
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_REBINDING:
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
#endif
		/* Restart the configuration process. */

		k_delayed_work_cancel(&iface->dhcpv4.timer);
		k_delayed_work_cancel(&iface->dhcpv4.t1_timer);
		k_delayed_work_cancel(&iface->dhcpv4.t2_timer);

		enter_selecting(iface);
		break;
	}
}

static void handle_dhcpv4_reply(struct net_if *iface,
				enum dhcpv4_msg_type msg_type)
{
	NET_DBG("state=%s msg=%s",
		net_dhcpv4_state_name(iface->dhcpv4.state),
		net_dhcpv4_msg_type_name(msg_type));

	switch (msg_type) {
	case DHCPV4_MSG_TYPE_OFFER:
		handle_offer(iface);
		break;
	case DHCPV4_MSG_TYPE_ACK:
		handle_ack(iface);
		break;
	case DHCPV4_MSG_TYPE_NAK:
		handle_nak(iface);
		break;
	default:
		NET_DBG("ignore message");
		break;
	}
}

static enum net_verdict net_dhcpv4_input(struct net_conn *conn,
					 struct net_pkt *pkt,
					 void *user_data)
{
	struct dhcp_msg *msg;
	struct net_buf *frag;
	struct net_if *iface;
	enum dhcpv4_msg_type msg_type = 0;
	u8_t min;
	u16_t pos;

	if (!conn) {
		NET_DBG("Invalid connection");
		return NET_DROP;
	}

	if (!pkt || !pkt->frags) {
		NET_DBG("Invalid packet, no fragments");
		return NET_DROP;
	}

	iface = net_pkt_iface(pkt);
	if (!iface) {
		NET_DBG("no iface");
		return NET_DROP;
	}

	frag = pkt->frags;
	min = NET_IPV4UDPH_LEN + sizeof(struct dhcp_msg);

	/* If the message is not DHCP then continue passing to
	 * related handlers.
	 */
	if (net_pkt_get_len(pkt) < min) {
		NET_DBG("Input msg is not related to DHCPv4");
		return NET_CONTINUE;

	}

	msg = (struct dhcp_msg *)(frag->data + NET_IPV4UDPH_LEN);

	NET_DBG("Received dhcp msg [op=0x%x htype=0x%x hlen=%u xid=0x%x "
		"secs=%u flags=0x%x ciaddr=%d.%d.%d.%d yiaddr=%d.%d.%d.%d "
		"siaddr=%d.%d.%d.%d giaddr=%d.%d.%d.%d chaddr=%s]",
		msg->op, msg->htype, msg->hlen, ntohl(msg->xid),
		msg->secs, msg->flags,
		msg->ciaddr[0], msg->ciaddr[1], msg->ciaddr[2], msg->ciaddr[3],
		msg->yiaddr[0], msg->yiaddr[1], msg->yiaddr[2], msg->yiaddr[3],
		msg->siaddr[0], msg->siaddr[1], msg->siaddr[2], msg->siaddr[3],
		msg->giaddr[0], msg->giaddr[1], msg->giaddr[2], msg->giaddr[3],
		net_sprint_ll_addr(msg->chaddr, 6));

	if (!(msg->op == DHCPV4_MSG_BOOT_REPLY &&
	      /* iface->dhcpv4.xid == ntohl(msg->xid) && */
	      !memcmp(msg->chaddr, iface->link_addr.addr,
		      iface->link_addr.len))) {

		NET_DBG("Unexpected op (%d), xid (%x vs %x) or chaddr",
			msg->op, iface->dhcpv4.xid, ntohl(msg->xid));
		goto drop;
	}

	/* For some ap, dhcpv4 reply xid not equal requet xid */
	if (iface->dhcpv4.xid == ntohl(msg->xid)) {
		dhcpv4_xid_diff_cnt = 0;
	} else {
		dhcpv4_xid_diff_cnt++;
		if (dhcpv4_xid_diff_cnt >= DHCPV4_MAX_NUMBER_OF_ATTEMPTS) {
			dhcpv4_xid_diff_cnt = 0;
			if ((iface->dhcpv4.state == NET_DHCPV4_SELECTING) &&
				(iface->dhcpv4.attempts >= DHCPV4_MAX_NUMBER_OF_ATTEMPTS)) {
				iface->dhcpv4.attempts = 0;
			}
		} else if (dhcpv4_xid_diff_cnt >= DHCPV4_MAX_DIFF_XID_THRESHOLD) {
			/* Let packet do dhcp parse */
		} else {
			goto drop;
		}
	}

	if (!iface->dhcpv4.add_flag) {
		memcpy(iface->dhcpv4.requested_ip.s4_addr, msg->yiaddr,
				sizeof(msg->yiaddr));
	}

	/* SNAME, FILE are not used at the moment, skip it */
	frag = net_frag_skip(frag, min, &pos, SIZE_OF_SNAME + SIZE_OF_FILE);
	if (!frag && pos) {
		NET_DBG("short packet while skipping sname");
		goto drop;
	}

	if (parse_options(iface, frag, pos, &msg_type) == NET_DROP) {
		NET_DBG("Invalid Options");
		goto drop;
	}

	net_pkt_unref(pkt);

	handle_dhcpv4_reply(iface, msg_type);

	return NET_OK;

drop:
	return NET_DROP;
}

void net_dhcpv4_start(struct net_if *iface)
{
	u32_t timeout;
	u32_t entropy;

	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
		dhcpv4_xid_diff_cnt = 0;
		iface->dhcpv4.state = NET_DHCPV4_INIT;
		NET_DBG("state=%s", net_dhcpv4_state_name(iface->dhcpv4.state));

		iface->dhcpv4.add_flag = false;
		iface->dhcpv4.attempts = 0;
		iface->dhcpv4.lease_time = 0;
		iface->dhcpv4.renewal_time = 0;

		iface->dhcpv4.server_id.s_addr = 0;
		iface->dhcpv4.requested_ip.s_addr = 0;
		iface->dhcpv4.dns_server.s_addr = 0;

		k_delayed_work_init(&iface->dhcpv4.timer, dhcpv4_timeout);
		k_delayed_work_init(&iface->dhcpv4.t1_timer, dhcpv4_t1_timeout);
		k_delayed_work_init(&iface->dhcpv4.t2_timer, dhcpv4_t2_timeout);

		/* We need entropy for both an XID and a random delay
		 * before sending the initial discover message.
		 */
		entropy = sys_rand32_get();

		/* A DHCP client MUST choose xid's in such a way as to
		 * minimize the change of using and xid identical to
		 * one used by another client.  Choose a random xid st
		 * startup and increment it on each new request.
		 */
		iface->dhcpv4.xid = entropy;


		/* RFC2131 4.1.1 requires we wait a random period
		 * between 1 and 10 seconds before sending the initial
		 * discover.
		 */
		timeout = entropy %
			(DHCPV4_INITIAL_DELAY_MAX - DHCPV4_INITIAL_DELAY_MIN) +
			DHCPV4_INITIAL_DELAY_MIN;

		NET_DBG("wait timeout=%"PRIu32"s", timeout);

#ifdef NET_DHCPV4_CACHED_PRE_IP
		iface->dhcpv4.state = NET_DHCPV4_REQ_PREIP;
#endif
		k_delayed_work_submit(&iface->dhcpv4.timer,
				      10 /* timeout * MSEC_PER_SEC */);	/* start dhcp after 10ms */
		break;
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
	case NET_DHCPV4_BOUND:
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
#endif
		break;
	}
}

void net_dhcpv4_stop(struct net_if *iface)
{
	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
		break;

	case NET_DHCPV4_BOUND:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
		if (!net_if_ipv4_addr_rm(iface,
					 &iface->dhcpv4.requested_ip)) {
			NET_DBG("Failed to remove addr from iface");
		} else {
			iface->dhcpv4.add_flag = false;
		}
		/* Fall through */

	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_REQUESTING:
#ifdef NET_DHCPV4_CACHED_PRE_IP
	case NET_DHCPV4_REQ_PREIP:
#endif
		iface->dhcpv4.state = NET_DHCPV4_DISABLED;
		NET_DBG("state=%s", net_dhcpv4_state_name(iface->dhcpv4.state));

		k_delayed_work_cancel(&iface->dhcpv4.timer);
		k_delayed_work_cancel(&iface->dhcpv4.t1_timer);
		k_delayed_work_cancel(&iface->dhcpv4.t2_timer);
		break;
	}
}

int dhcpv4_init(void)
{
	struct sockaddr local_addr;
	int ret;

	NET_DBG("");

	net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
			net_ipv4_unspecified_address());
	local_addr.sa_family = AF_INET;

	/* Register UDP input callback on
	 * DHCPV4_SERVER_PORT(67) and DHCPV4_CLIENT_PORT(68) for
	 * all dhcpv4 related incoming packets.
	 */
	ret = net_udp_register(NULL, &local_addr,
			       DHCPV4_SERVER_PORT,
			       DHCPV4_CLIENT_PORT,
			       net_dhcpv4_input, NULL, NULL);
	if (ret < 0) {
		NET_DBG("UDP callback registration failed");
		return ret;
	}

	return 0;
}

bool net_dhcpv4_receive_pkt(struct net_pkt *pkt)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		return false;
	}

	if ((ntohs(udp_hdr->src_port) != DHCPV4_SERVER_PORT) ||
		(ntohs(udp_hdr->dst_port) != DHCPV4_CLIENT_PORT)) {
		return false;
	}

	return true;
}

#ifdef NET_DHCPV4_CACHED_PRE_IP
static inline bool add_req_preipaddr(struct net_pkt *pkt, u8_t *preip)
{
	u8_t data;

	data = DHCPV4_OPTIONS_REQ_IPADDR;
	if (!net_pkt_append(pkt, 1, &data, K_FOREVER)) {
		return false;
	}

	data = 4;
	if (!net_pkt_append(pkt, 1, &data, K_FOREVER)) {
		return false;
	}

	if (!net_pkt_append(pkt, 4, preip, K_FOREVER)) {
		return false;
	}

	return true;
}

static int send_request_preip(struct net_if *iface)
{
	struct net_pkt *pkt;
	u32_t timeout;
	int data_len = 0;
	struct dhcp_cached_ip dhcp_ip;
#ifdef CONFIG_NVRAM_CONFIG
	data_len = nvram_config_get(DHCP_CACHED_IP_NAME, &dhcp_ip, sizeof(struct dhcp_cached_ip));
#endif
	if ((data_len < 0) || (dhcp_ip.server_id.s_addr == 0) ||
		(dhcp_ip.server_id.s_addr == 0xFFFFFFFF) || (dhcp_ip.requested_ip.s_addr == 0) ||
		(dhcp_ip.requested_ip.s_addr == 0xFFFFFFFF)) {
		SYS_LOG_INF("Read %s error or invalid ip", DHCP_CACHED_IP_NAME);
		return -1;
	}

	iface->dhcpv4.xid++;

	pkt = prepare_message(iface, DHCPV4_MSG_TYPE_REQUEST, NULL);
	if (!pkt) {
		goto fail;
	}

	if (!add_req_preipaddr(pkt, dhcp_ip.requested_ip.s4_addr) ||
		!add_max_msg_size(pkt) ||
		!add_req_options(pkt) ||
		!add_end(pkt)) {
		goto fail;
	}

	setup_header(pkt, net_ipv4_broadcast_address());

	if (net_send_data(pkt) < 0) {
		goto fail;
	}

	timeout = DHCPV4_REQUEST_PREIP_TIMEOUT;

	NET_DBG("enter state=%s xid=0x%"PRIx32" timeout=%"PRIu32"s",
		net_dhcpv4_state_name(iface->dhcpv4.state),
		iface->dhcpv4.xid, timeout);

	k_delayed_work_submit(&iface->dhcpv4.timer, timeout * MSEC_PER_SEC);

	iface->dhcpv4.attempts++;
	return 0;

fail:
	NET_DBG("Message preparation failed");

	if (!pkt) {
		net_pkt_unref(pkt);
	}

	return -1;
}

void net_notify_dhcp_ssid(char *ssid)
{
	int ret;
	bool clear_flag = false;
	char buf[DHCP_SSID_LEN];
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_get(DHCP_CACHED_SSID_NAME, buf, DHCP_SSID_LEN);
#endif
	if (ret < 0) {
		clear_flag = true;
	}

	if ((strlen(ssid) != ret) ||
		(memcmp(ssid, buf, strlen(ssid)) != 0)) {
		clear_flag = true;
	}

	if (clear_flag) {
	#ifdef CONFIG_NVRAM_CONFIG
		ret = nvram_config_set(DHCP_CACHED_SSID_NAME, ssid, strlen(ssid));
		if (ret < 0) {
			SYS_LOG_ERR("Write %s failed", DHCP_CACHED_SSID_NAME);
		}

		/* delete the item if data length is 0 */
		ret = nvram_config_set(DHCP_CACHED_IP_NAME, buf, 0);
		if (ret < 0) {
			SYS_LOG_ERR("delete %s failed", DHCP_CACHED_IP_NAME);
		}
	#endif
	}
}
#else
void net_notify_dhcp_ssid(char *ssid){}
#endif
