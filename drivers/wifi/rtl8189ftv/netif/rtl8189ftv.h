/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __RTL8189RTV_H__
#define __RTL8189RTV_H__

int rtl8189ftv_recv_cb(struct net_pkt *pkt);
void rtl8189ftv_set_netif_info(int idx_wlan, void *dev, unsigned char *dev_addr);
int rtl8189ftv_is_valid_IP(int idx, unsigned char *ip_dest);
unsigned char *rtl8189ftv_get_ip(int idx);
void rtl8189ftv_dhcpc_start(void);
void rtl8189ftv_dhcpc_stop(void);

#endif
