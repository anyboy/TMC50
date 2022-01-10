/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifndef _SSV6030P_H_
#define _SSV6030P_H_

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern int log_level;

extern int netstack_output(void* net_interface, void *data, u32 len);
extern int ssv6xxx_dev_init(int hmode);
extern bool netmgr_wifi_check_sta();
extern void OS_TickDelay(u32 ticks);
extern void wifi_exit_ps(bool exit);
extern void actions_cli_init(void);
extern void ssv_optimize_rf_parameter(void);

#endif //#ifndef _SSV6030P_H_