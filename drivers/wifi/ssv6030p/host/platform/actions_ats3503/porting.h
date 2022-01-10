/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_

#include <zephyr.h>
#include <ssv_types.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h> 
#define  PRINT printf
#else /* !CONFIG_STDOUT_CONSOLE */
#include <misc/printk.h>
#define PRINT printk
#endif /* CONFIG_STDOUT_CONSOLE */

#include <board.h>

#ifdef BOARD_WIFI_POWER_EN_GPIO
#include <gpio.h>
#define SSV_LDO_EN_PIN         BOARD_WIFI_POWER_EN_GPIO

#ifndef BOARD_WIFI_POWER_EN_GPIO_NAME
#define SSV_LDO_EN_PIN_NAME    "GPIO_0"
#else
#define SSV_LDO_EN_PIN_NAME    BOARD_WIFI_POWER_EN_GPIO_NAME
#endif
#define SSV_LDO_EN_PIN       BOARD_WIFI_POWER_EN_GPIO
#endif

#ifdef BOARD_WIFI_SDIO_BUS_ID
#define SSV_WIFI_SDIO_BUS_ID BOARD_WIFI_SDIO_BUS_ID
#else
#define SSV_WIFI_SDIO_BUS_ID 0
#endif

#undef LWIP_PROVIDE_ERRNO
#define HOST_PLATFORM_SEL 

#ifndef in_range
#define in_range(c, lo, up)  ((u8)c >= lo && (u8)c <= up)
#endif

#ifndef isprint
#define isprint(c)           in_range(c, 0x20, 0x7f)
#endif

#ifndef  isdigit 
#define isdigit(c)           in_range(c, '0', '9')
#endif

#ifndef isxdigit
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#endif

#ifndef islower
#define islower(c)           in_range(c, 'a', 'z')
#endif

#ifndef isspace
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#endif


//#define __TEST_BANDWIDTH_RX

//#define OS_MEM_DEBUG

/*============Task Priority===================*/
#define RX_ISR_PRIORITY    0 //No use
#define MLME_TASK_PRIORITY 1

#define TMR_TASK_PRIORITY  2
#define CMD_ENG_PRIORITY   3
#define WIFI_RX_PRIORITY   4
#define WIFI_TX_PRIORITY   5
#define NETMGR_PRIORITY    6

#define TCPIP_PRIORITY     1
#define DHCPD_PRIORITY     2


#define NETAPP_PRIORITY    1
#define NETAPP_PRIORITY_1  1
#define NETAPP_PRIORITY_2  1
#define TASK_END_PRIO      2

/*============Console setting===================*/
#define hal_print PRINT
#define FFLUSH(x) //fflush(x)
#define hal_putchar(ch) PRINT("%c",ch) 
extern u8 hal_getchar();
extern int sscanf (const char *buf, const char *fmt, ...);

#define abort() 

#ifdef CONFIG_WIFI_EXT_RX_INT
#define EXT_RX_INT
#endif

#define SDIO_NO_NEED_ALIGN

/* sdio rx int, use card int trigger, pulldown/pullup */
//#define SDIO_CARD_INT_TRIGGER

//#define CONFIG_BUS_LOOPBACK_TEST 1

/*============Compiler setting===================*/
#define ARM_ADS

//SSV PACK Definition
#define SSV_PACKED_STRUCT_BEGIN   //__attribute__ ((packed))
#define SSV_PACKED_STRUCT_END      
#define SSV_PACKED_STRUCT_STRUCT  __attribute__ ((packed))     
#define SSV_PACKED_STRUCT_FIELD(x)   x

//LWIP PACK Definition
#define PACK_STRUCT_BEGIN    
#define PACK_STRUCT_END      
#define PACK_STRUCT_STRUCT     __attribute__ ((packed))
#define PACK_STRUCT_FIELD(x)    x

#define ALIGN_ARRAY(a) __attribute__ ((aligned(a)))

#ifndef RAMFUNC
#define RAMFUNC __attribute__((section(".ramfunc")))
#endif

//#define inline __inline

/*============SSV-DRV setting===================*/
#define	CONFIG_RX_POLL      0
#define INTERFACE "sdio"
#define SDRV_INCLUDE_SDIO   1

/*============Stack Size (unint: 16bytes)===================*/
#define TMR_TASK_STACK_SIZE   40      //36
#define MLME_TASK_STACK_SIZE  0
#define CMD_ENG_STACK_SIZE    96      //128
#define TCPIP_STACK_SIZE      0
#define DHCPD_STACK_SIZE      0       
#define NETAPP1_STACK_SIZE    0   
#define NETAPP2_STACK_SIZE    0       
#define NETAPP3_STACK_SIZE    0     
#define NETMGR_STACK_SIZE     96      //128  
#define CLI_TASK_STACK_SIZE   0      
#define RX_ISR_STACK_SIZE     0     
#define WIFI_RX_STACK_SIZE    68      //96  
#define WIFI_TX_STACK_SIZE    68      //96
#define DEV_INIT_STACK_SIZE    0    
#define PING_THREAD_STACK_SIZE 0

#if (USE_ICOMM_LWIP == 0)
#undef TCPIP_STACK_SIZE
#define TCPIP_STACK_SIZE 0

#undef NETAPP1_STACK_SIZE
#define NETAPP1_STACK_SIZE 0

#undef NETAPP2_STACK_SIZE
#define NETAPP2_STACK_SIZE    0

#undef NETAPP3_STACK_SIZE
#define NETAPP3_STACK_SIZE    0

#undef CLI_TASK_STACK_SIZE  
#define CLI_TASK_STACK_SIZE 0
#endif

#define TOTAL_STACK_SIZE (TMR_TASK_STACK_SIZE+ \
                          MLME_TASK_STACK_SIZE+ \
                          CMD_ENG_STACK_SIZE+ \
                          TCPIP_STACK_SIZE+ \
                          DHCPD_STACK_SIZE+ \
                          NETAPP1_STACK_SIZE+ \
						  NETAPP2_STACK_SIZE+ \
						  NETAPP3_STACK_SIZE+ \
                          NETMGR_STACK_SIZE+ \
                          CLI_TASK_STACK_SIZE+ \
						  RX_ISR_STACK_SIZE+ \
						  WIFI_RX_STACK_SIZE+ \
                          WIFI_TX_STACK_SIZE+ \
                          DEV_INIT_STACK_SIZE+\
                          PING_THREAD_STACK_SIZE)
                          
/*============Memory========================*/
#ifdef OS_MEM_DEBUG
#define OS_MemAlloc(size) OS_MemAlloc1(size, __func__, __LINE__) 
#define __OS_MemFree(m) __OS_MemFree1(m, __func__, __LINE__) 
#else
OS_APIs void *OS_MemAlloc( u32 size );
OS_APIs void __OS_MemFree( void *m );
#endif

OS_APIs void OS_MemSET(void *pdest, u8 byte, u32 size);
OS_APIs void OS_MemCPY(void *pdest, const void *psrc, u32 size);

/*=========================================*/
void platform_ldo_en_pin_init(void);
void platform_ldo_en(bool en);

#endif
