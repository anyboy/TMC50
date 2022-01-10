/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief link memory address overlay helper
 */

#include <kernel.h>
#include <string.h>
#include <misc/printk.h>
#include <notify.h>

/**
 * notifier_chain_register	- Add notifier to a notifier chain
 * @list: Pointer to root list pointer
 * @n: New entry in notifier chain
 *
 * Adds a notifier to a notifier chain.
 *
 * Currently always returns zero.
 */
int notifier_chain_register(struct notifier_block **list, struct notifier_block *n)
{
	while(*list)
	{
		if(n->priority > (*list)->priority)
			break;
		list= &((*list)->next);
	}
	n->next = *list;
	*list=n;

	return 0;
}

/**
 * notifier_chain_unregister - Remove notifier from a notifier chain
 * @nl: Pointer to root list pointer
 * @n: New entry in notifier chain
 *
 * Removes a notifier from a notifier chain.
 *
 * Returns zero on success, or %-ENOENT on failure.
 */
 
int notifier_chain_unregister(struct notifier_block **nl, struct notifier_block *n)
{
	while((*nl)!=NULL)
	{
		if((*nl)==n)
		{
			*nl=n->next;
			return 0;
		}
		nl=&((*nl)->next);
	}

	return -ENOENT;
}

/**
 * notifier_call_chain - Call functions in a notifier chain
 * @n: Pointer to root pointer of notifier chain
 * @val: Value passed unmodified to notifier function
 * @v: Pointer passed unmodified to notifier function
 *
 * Calls each function in a notifier chain in turn.
 *
 * If the return value of the notifier can be and'd
 * with %NOTIFY_STOP_MASK, then notifier_call_chain
 * will return immediately, with the return value of
 * the notifier function which halted execution.
 * Otherwise, the return value is the return value
 * of the last notifier function called.
 */
 
int notifier_call_chain(struct notifier_block **n, unsigned long val, void *v)
{
	int ret=NOTIFY_DONE;
	struct notifier_block *nb = *n;

	while(nb)
	{
		ret=nb->notifier_call(nb,val,v);
		if(ret&NOTIFY_STOP_MASK)
		{
			return ret;
		}
		nb=nb->next;
	}
	return ret;
}
