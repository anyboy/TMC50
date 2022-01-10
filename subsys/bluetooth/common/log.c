/* log.c - logging helpers */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper for printk parameters to convert from binary to hex.
 * We declare multiple buffers so the helper can be used multiple times
 * in a single printk call.
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr.h>
#include <misc/util.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define LOG_DIRECT_OUTPUT	1

const char *bt_hex(const void *buf, size_t len)
{
#if LOG_DIRECT_OUTPUT
	static const char *retStr = "bt_hex";
	const u8_t *data = buf;
	size_t i;

	printk("%s: ", retStr);
	for (i=0; i<len; i++) {
		printk("%02x", data[i]);
	}
	printk("\n");

	return retStr;
#else
	static const char hex[] = "0123456789abcdef";
	static char hexbufs[4][129];
	static u8_t curbuf;
	const u8_t *b = buf;
	unsigned int mask;
	char *str;
	int i;

	mask = irq_lock();
	str = hexbufs[curbuf++];
	curbuf %= ARRAY_SIZE(hexbufs);
	irq_unlock(mask);

	len = min(len, (sizeof(hexbufs[0]) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2]     = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
#endif
}

const char *bt_addr_str(const bt_addr_t *addr)
{
#if LOG_DIRECT_OUTPUT
	static const char *retStr = "bt_addr";
	int i;

	printk("%s: ", retStr);
	for (i=0; i<6; i++) {
		printk("%02x%s", addr->val[5-i], (5 - i)? ":" : "");
	}
	printk("\n");

	return retStr;
#else
	static char bufs[2][18];
	static u8_t cur;
	char *str;

	str = bufs[cur++];
	cur %= ARRAY_SIZE(bufs);
	bt_addr_to_str(addr, str, sizeof(bufs[cur]));

	return str;
#endif
}

const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
#if LOG_DIRECT_OUTPUT
	static const char *retStr = "bt_addr_le";
	int i;

	printk("%s: ", retStr);

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		printk("public ");
		break;
	case BT_ADDR_LE_RANDOM:
		printk("random ");
		break;
	default:
		printk("type(0x%02x) ", addr->type);
		break;
	}

	for (i=0; i<6; i++) {
		printk("%02x%s", addr->a.val[5-i], (5 - i)? ":" : "");
	}
	printk("\n");

	return retStr;
#else
	static char bufs[2][27];
	static u8_t cur;
	char *str;

	str = bufs[cur++];
	cur %= ARRAY_SIZE(bufs);
	bt_addr_le_to_str(addr, str, sizeof(bufs[cur]));

	return str;
#endif
}

