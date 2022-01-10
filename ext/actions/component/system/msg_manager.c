/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file message manager interface
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "msg_manager"
#include <logging/sys_log.h>

#include <os_common_api.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <sys_wakelock.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>

extern int os_get_pending_msg_cnt(void);
/*global data mailbox for all app thread*/
OS_MUTEX_DEFINE(msg_manager_mutex);

static sys_slist_t	global_receiver_list;

static struct msg_listener *msg_manager_find_by_name(char *name)
{
	int key;
	sys_snode_t *node, *tmp;
	struct msg_listener *listener = NULL;

	key = irq_lock();

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_receiver_list, node, tmp) {
		listener = LISTENER_INFO(node);
		if (!strcmp(listener->name, name)) {
			goto exit;
		}
	}
	listener = NULL;
exit:
	irq_unlock(key);
	return listener;
}


static struct msg_listener *msg_manager_find_by_tid(os_tid_t tid)
{
	int key;
	sys_snode_t *node, *tmp;
	struct msg_listener *listener = NULL;

	key = irq_lock();

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_receiver_list, node, tmp) {
		listener = LISTENER_INFO(node);
		if (listener->tid == tid) {
			goto exit;
		}
	}
	listener = NULL;
exit:
	irq_unlock(key);
	return listener;
}

char *msg_manager_get_current(void)
{
	struct msg_listener *listener = msg_manager_find_by_tid(os_current_get());

	if (listener != NULL) {
		return listener->name;
	}
	return NULL;
}

char *msg_manager_get_name_by_tid(int tid)
{
	struct msg_listener *listener = msg_manager_find_by_tid((os_tid_t)tid);

	if (listener != NULL) {
		return listener->name;
	}
	return NULL;
}

bool msg_manager_add_listener(char *name, os_tid_t tid)
{

	struct msg_listener *listener = NULL;

	int key = irq_lock();

	listener = mem_malloc(sizeof(struct msg_listener));
	if (!listener) {
		goto exit;
	}

	listener->name = name;

	listener->tid = tid;

	sys_slist_append(&global_receiver_list, (sys_snode_t *)listener);

exit:
	irq_unlock(key);
	return true;
}

bool msg_manager_remove_listener(char *name)
{
	struct msg_listener *listener = msg_manager_find_by_name(name);
	bool result = false;

	int key = irq_lock();

	if (listener != NULL) {
		sys_slist_find_and_remove(&global_receiver_list, (sys_snode_t *)listener);
		mem_free(listener);
		result = true;
		goto exit;
	}
exit:
	irq_unlock(key);
	return result;
}

os_tid_t  msg_manager_listener_tid(char *name)
{
	struct msg_listener *listener = msg_manager_find_by_name(name);

	if (listener != NULL) {
		return listener->tid;
	}

	return NULL;
}
/*init manager*/
bool msg_manager_init(void)
{
	os_msg_init();
	return true;
}

/*@brief Provide send async mesg interface
 *Note:
 *
 *@param receiver the id for who will receive this msg
 *@param msg which msg you will send
 */

bool msg_manager_send_async_msg(char *receiver, struct app_msg *msg)
{
	bool result = false;
	os_tid_t target_thread_tid = OS_ANY;
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(WAKELOCK_MESSAGE);
#endif
	if (strcmp(receiver, ALL_RECEIVER_NAME)) {
		target_thread_tid = msg_manager_listener_tid(receiver);
		if (target_thread_tid == NULL) {
			SYS_LOG_ERR("app %s not ready\n", receiver);
			result = false;
			goto exit;
		}
	}

	if (!os_send_async_msg(target_thread_tid, msg, sizeof(struct app_msg))) {
		result = true;
	} else {
		result = false;
	}
exit:
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(WAKELOCK_MESSAGE);
#endif
	return result;
}

bool msg_manager_send_sync_msg(char *receiver, struct app_msg *msg)
{
	int prio;
	bool result = false;
	os_tid_t target_thread_tid = OS_ANY;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(WAKELOCK_MESSAGE);
#endif

	prio = os_thread_priority_get(os_current_get());
	os_thread_priority_set(os_current_get(), -1);

	if (strcmp(receiver, ALL_RECEIVER_NAME)) {
		target_thread_tid = msg_manager_listener_tid(receiver);
		if (target_thread_tid == NULL) {
			SYS_LOG_ERR("app %s not ready\n", receiver);
			result = false;
			goto exit;
		}
	}

	if (!os_send_sync_msg(target_thread_tid, msg, sizeof(struct app_msg))) {
		result = true;
	} else {
		result = false;
	}
exit:
	os_thread_priority_set(os_current_get(), prio);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(WAKELOCK_MESSAGE);
#endif

	return result;
}

bool msg_manager_receive_msg(struct app_msg *msg, int timeout)
{
	bool result = false;

	if (!os_receive_msg(msg, sizeof(struct app_msg), timeout))
		result  = true;

	return result;
}

int msg_manager_get_free_msg_num(void)
{
	return msg_pool_get_free_msg_num();
}

void msg_manager_drop_all_msg(void)
{
	os_msg_clean();
}

int msg_manager_get_pending_msg_cnt(void)
{
	return os_get_pending_msg_cnt();
}

