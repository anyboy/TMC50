/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DNS_CLIENT_H_
#define _DNS_CLIENT_H_

void del_dns_cached(char *ip);
int net_dns_resolve(char *name, struct in_addr * retaddr);

#endif
