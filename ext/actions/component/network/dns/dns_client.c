/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_DNS_RESOLVER
#include <os_common_api.h>
#include <mem_manager.h>
#include <net/dns_resolve.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <misc/printk.h>
#include <string.h>
#include <errno.h>

#define CLIENT_DNS_TIMEOUT  500 /* ms */
#define RETRY_DNS_CNT		5
#define DNS_CACHED_NUM		6
#define DNS_TTL_CHECK_INTERVAL	(1*MSEC_PER_SEC)

typedef struct {
	char *name;
	struct in_addr addr;
	u32_t ttl;
}t_dns_cached;

typedef struct {
	struct in_addr addr;
	u32_t ttl;
}dns_cb_result;

static bool dns_cached_work_start = false;
static struct k_delayed_work dns_cached_work;
static K_MUTEX_DEFINE(dns_cached_lock);
static t_dns_cached dns_cached[DNS_CACHED_NUM];
static K_MUTEX_DEFINE(dns_resolve_lock);
static K_SEM_DEFINE(dns_wait_sem, 0, 1);

static int find_dns_cached(char *name, struct in_addr *retaddr)
{
	u8_t i;

	k_mutex_lock(&dns_cached_lock, K_FOREVER);

	for (i=0; i<DNS_CACHED_NUM; i++) {
		if (strlen(dns_cached[i].name) == strlen(name)) {
			if (0 == memcmp(dns_cached[i].name, name,  strlen(name))) {
				retaddr->s_addr = dns_cached[i].addr.s_addr;
				k_mutex_unlock(&dns_cached_lock);
				return 0;
			}
		}
	}

	k_mutex_unlock(&dns_cached_lock);
	return -1;
}

static void dns_check_ttl_timeout(struct k_work *work)
{
	u8_t i;

	k_mutex_lock(&dns_cached_lock, K_FOREVER);

	for (i=0; i<DNS_CACHED_NUM; i++) {
		if (dns_cached[i].ttl > 0) {
			dns_cached[i].ttl--;
		}

		if (dns_cached[i].name && (0 == dns_cached[i].ttl)) {
			mem_free(dns_cached[i].name);
			dns_cached[i].name = NULL;
			dns_cached[i].addr.s_addr = 0;
		}
	}

	k_mutex_unlock(&dns_cached_lock);

	k_delayed_work_submit(&dns_cached_work, DNS_TTL_CHECK_INTERVAL);
}

static void add_dns_cached(char *name, struct in_addr *retaddr, u32_t ttl)
{
	u8_t i, len;
	char *pChar;

	if (ttl == 0) {
		/* Not need to cache dns */
		return;
	}

	len = strlen(name);
	pChar = (char *)mem_malloc(len + 1);
	if (NULL == pChar) {
		return;
	}

	memset(pChar, 0, (len + 1));
	memcpy(pChar, name, len);

	if (!dns_cached_work_start) {
		dns_cached_work_start = true;
		k_delayed_work_init(&dns_cached_work, dns_check_ttl_timeout);
		k_delayed_work_submit(&dns_cached_work, DNS_TTL_CHECK_INTERVAL);
	}

	k_mutex_lock(&dns_cached_lock, K_FOREVER);

	for (i=0; i<DNS_CACHED_NUM; i++) {
		if (NULL == dns_cached[i].name) {
			dns_cached[i].name = pChar;
			dns_cached[i].addr.s_addr = retaddr->s_addr;
			dns_cached[i].ttl = ttl;
			goto add_finish;
		}
	}

	if (NULL != dns_cached[DNS_CACHED_NUM - 1].name) {
		mem_free(dns_cached[DNS_CACHED_NUM - 1].name);
	}

	for (i=(DNS_CACHED_NUM - 1); i>0; i--) {
		dns_cached[i].name = dns_cached[i - 1].name;
		dns_cached[i].addr.s_addr = dns_cached[i - 1].addr.s_addr;
		dns_cached[i].ttl = dns_cached[i - 1].ttl;
	}

	dns_cached[0].name = pChar;
	dns_cached[0].addr.s_addr = retaddr->s_addr;
	dns_cached[0].ttl = ttl;

add_finish:
	k_mutex_unlock(&dns_cached_lock);
}

void del_dns_cached(char *ip)
{
	struct in_addr  in_addr_t = { { { 0 } } };
	int ret, i;

	ret = net_addr_pton(AF_INET, ip, &in_addr_t);
	if (ret < 0) {
		SYS_LOG_ERR("invalid ip:%s", ip);
		return;
	}

	k_mutex_lock(&dns_cached_lock, K_FOREVER);

	for (i=0; i<DNS_CACHED_NUM; i++) {
		if (dns_cached[i].addr.s_addr == in_addr_t.s_addr) {
			if (NULL != dns_cached[i].name) {
				mem_free(dns_cached[i].name);
				dns_cached[i].name = NULL;
			}
			dns_cached[i].addr.s_addr = 0;
			dns_cached[i].ttl = 0;
			break;
		}
	}

	k_mutex_unlock(&dns_cached_lock);
}

static void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	char *hr_family;
	void *addr;
	dns_cb_result *dns_result = user_data;

	switch (status) {
	case DNS_EAI_CANCELED:
		NET_INFO("DNS query was canceled");
		k_sem_give(&dns_wait_sem);
		return;
	case DNS_EAI_FAIL:
		NET_INFO("DNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		NET_INFO("Cannot resolve address");
		return;
	case DNS_EAI_ALLDONE:
		NET_INFO("DNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		NET_INFO("DNS resolving error (%d)", status);
		return;
	}

	if (!info) {
		return;
	}

	if (info->ai_family == AF_INET) {
		hr_family = "IPv4";
		addr = &net_sin(&info->ai_addr)->sin_addr;
		dns_result->addr.s_addr = net_sin(&info->ai_addr)->sin_addr.s_addr;
		dns_result->ttl = info->ai_ttl;
		k_sem_give(&dns_wait_sem);
	} else if (info->ai_family == AF_INET6) {
		hr_family = "IPv6";
		addr = &net_sin6(&info->ai_addr)->sin6_addr;
	} else {
		NET_ERR("Invalid IP address family %d", info->ai_family);
		return;
	}
}


static int do_dns_work(char *name, dns_cb_result *dns_result)
{
	u16_t dns_id, rand_id;
	int ret = -EIO, i = 0;

	rand_id = sys_rand32_get();
	dns_result->addr.s_addr = 0;
	dns_result->ttl = 0;
	while (i++ < RETRY_DNS_CNT) {
		k_sem_reset(&dns_wait_sem);

		dns_id = rand_id;
		ret = dns_get_addr_info(name,
					DNS_QUERY_TYPE_A,
					&dns_id,
					dns_result_cb,
					dns_result,
					CLIENT_DNS_TIMEOUT);
		if (ret < 0) {
			dns_cancel_addr_info(dns_id);
			k_sleep(CLIENT_DNS_TIMEOUT/2);
			continue;
		}

		k_sem_take(&dns_wait_sem, CLIENT_DNS_TIMEOUT);
		if (dns_result->addr.s_addr != 0) {
			ret = 0;
			break;
		} else {
			dns_cancel_addr_info(dns_id);
			ret = -EIO;
		}
	}

	return ret;
}

int net_dns_resolve(char *name, struct in_addr *retaddr)
{
	int ret, prio;
	dns_cb_result dns_result;

	if (0 == find_dns_cached(name, retaddr)) {
		return 0;
	}

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	k_mutex_lock(&dns_resolve_lock, K_FOREVER);
	ret = do_dns_work(name, &dns_result);
	if (ret < 0) {
		dns_exchange_server_addr();
		ret = do_dns_work(name, &dns_result);
		dns_exchange_server_addr();
	}
	k_mutex_unlock(&dns_resolve_lock);

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	if (ret < 0) {
		SYS_LOG_ERR("Can't resolve %s rc: %d", name, ret);
		return ret;
	}

	retaddr->s_addr = dns_result.addr.s_addr;
	add_dns_cached(name, &dns_result.addr, dns_result.ttl);
	return 0;
}
#else
void del_dns_cached(char *ip)
{
	SYS_LOG_WRN("this function not support");
}
int net_dns_resolve(char *name, struct in_addr * retaddr)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
#endif
