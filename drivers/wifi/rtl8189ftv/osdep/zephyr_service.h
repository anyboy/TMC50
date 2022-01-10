#ifndef _ZEPHYR_SERVICE_H_
#define _ZEPHYR_SERVICE_H_

#include <atomic.h>
#include <string.h>
#include <logging/sys_log.h>
#include "wlan/skbuff.h"

#define configTICK_RATE_HZ	MSEC_PER_SEC

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef signed char			s8;
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long 	s64;
typedef unsigned long long	u64;
typedef unsigned int		uint;
typedef signed int			sint;

#define pdFALSE			(0)
#define pdTRUE			(1)
#define pdPASS			( pdTRUE )
#define pdFAIL			( pdFALSE )

#define IN
#define OUT
#define VOID void
#define NDIS_OID uint
#define NDIS_STATUS uint
#ifndef	PVOID
typedef void *		PVOID;
#endif

typedef unsigned int		__kernel_size_t;
typedef int			__kernel_ssize_t;
typedef	__kernel_size_t		SIZE_T;	
typedef	__kernel_ssize_t	SSIZE_T;

#define FIELD_OFFSET(s,field)	((SSIZE_T)&((s*)(0))->field)

// os types
typedef char			osdepCHAR;
typedef float			osdepFLOAT;
typedef double			osdepDOUBLE;
typedef long			osdepLONG;
typedef short			osdepSHORT;
typedef unsigned long	osdepSTACK_TYPE;
typedef long			osdepBASE_TYPE;
typedef unsigned long	osdepTickType;

typedef void*	_timerHandle;
typedef void*	_sema;
typedef void*	_mutex;
typedef void*	_lock;
typedef void*	_queueHandle;
typedef void*	_xqueue;

typedef struct timer_list	_timer;
typedef	struct sk_buff		_pkt;
typedef unsigned char		_buffer;

struct list_head {
	struct list_head *next, *prev;
};

struct	__queue	{
	struct	list_head	queue;
	_lock			lock;
};

typedef struct	__queue		_queue;
typedef struct	list_head	_list;
typedef unsigned long		_irqL;

typedef void*			_thread_hdl_;
typedef void			thread_return;
typedef void*			thread_context;

#define ATOMIC_T atomic_t

typedef struct k_sem *xSemaphoreHandle;
#define vTaskDelay	k_sleep
#define pvPortMalloc	mem_malloc	 
#define vPortFree	mem_free
#define xTaskGetTickCount		k_uptime_get

static inline _list *get_next(_list	*list)
{
	return list->next;
}	

static inline _list	*get_list_head(_queue	*queue)
{
	return (&(queue->queue));
}

#define LIST_CONTAINOR(ptr, type, member) \
	((type *)((char *)(ptr)-(SIZE_T)((char *)&((type *)ptr)->member - (char *)ptr)))
#define container_of(p,t,n) (t*)((p)-&(((t*)0)->n))

#define TASK_PRORITY_LOW  				1
#define TASK_PRORITY_MIDDLE   			2
#define TASK_PRORITY_HIGH    			3
#define TASK_PRORITY_SUPER    			4

#define TIMER_MAX_DELAY    				0xFFFFFFFF
#define portMAX_DELAY 					0xffff

struct rtw_netdev_priv_indicator {
	void *priv;
	u32 sizeof_priv;
};

#define rtw_netdev_priv(netdev) ( ((struct rtw_netdev_priv_indicator *)netdev_priv(netdev))->priv )

#define FUNC_NDEV_FMT "%s"
#define FUNC_NDEV_ARG(ndev) __func__

void save_and_cli(void);
void restore_flags(void);
void cli(void);

//----- ------------------------------------------------------------------
// Common Definition
//----- ------------------------------------------------------------------

#define __init
#define __exit
#define __devinit
#define __devexit

#define KERN_ERR
#define KERN_INFO
#define KERN_NOTICE

#define GFP_KERNEL			1
#define GFP_ATOMIC			1

#define SET_MODULE_OWNER(some_struct)	do { } while (0)
#define SET_NETDEV_DEV(dev, obj)	do { } while (0)
#define register_netdev(dev)		(0)
#define unregister_netdev(dev)		do { } while (0)
#define netif_queue_stopped(dev)	(0)
#define netif_wake_queue(dev)		do { } while (0)
#define printk				printf

#define DBG_ERR(fmt, args...)		printf("\n\r[%s] " fmt, __FUNCTION__, ## args)
#if WLAN_INTF_DBG
#define DBG_TRACE(fmt, args...)		printf("\n\r[%s] " fmt, __FUNCTION__, ## args)
#define DBG_INFO(fmt, args...)		printf("\n\r[%s] " fmt, __FUNCTION__, ## args)
#else
#define DBG_TRACE(fmt, args...)
#define DBG_INFO(fmt, args...)
#endif
#define HALT()				do { cli(); for(;;);} while(0)
#define ASSERT(x)			do { \
						if((x) == 0) \
							printf("\n\rAssert(" #x ") failed on line %d in file %s", __LINE__, __FILE__); \
						HALT(); \
					} while(0)

#undef DBG_ASSERT
#define DBG_ASSERT(x, msg)		do { \
						if((x) == 0) \
							printf("\n\r%s, Assert(" #x ") failed on line %d in file %s", msg, __LINE__, __FILE__); \
					} while(0)

#ifdef CONFIG_SYS_LOG
#define PR_DBG(fmt, ...) SYS_LOG_DBG(fmt, ##__VA_ARGS__)
#define PR_INF(fmt, ...) SYS_LOG_INF(fmt, ##__VA_ARGS__)
#define PR_WRN(fmt, ...) SYS_LOG_WRN(fmt, ##__VA_ARGS__)
#define PR_ERR(fmt, ...) SYS_LOG_ERR(fmt, ##__VA_ARGS__)
#else
#define PR_DBG(fmt, ...) printk("[wifi] [DBG] %s(%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define PR_INF(fmt, ...) printk("[wifi] [INF] %s(%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define PR_WRN(fmt, ...) printk("[wifi] [WRN] %s(%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define PR_ERR(fmt, ...) printk("[wifi] [ERR] %s(%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#endif

 /*
  *      These inlines deal with timer wrapping correctly. You are 
  *      strongly encouraged to use them
  *      1. Because people otherwise forget
  *      2. Because if the timer wrap changes in future you wont have to
  *         alter your driver code.
  *
  * time_after(a,b) returns true if the time a is after time b.
  *
  * Do this with "<0" and ">=0" to only test the sign of the result. A
  * good compiler would generate better code (and a really good compiler
  * wouldn't care). Gcc is currently neither.
  */
 #define time_after(a,b)	((long)(b) - (long)(a) < 0)
 #define time_before(a,b)	time_after(b,a)
  
 #define time_after_eq(a,b)	((long)(a) - (long)(b) >= 0)
 #define time_before_eq(a,b)	time_after_eq(b,a)

extern void	rtw_init_listhead(_list *list);
extern u32	rtw_is_list_empty(_list *phead);
extern void	rtw_list_insert_head(_list *plist, _list *phead);
extern void	rtw_list_insert_tail(_list *plist, _list *phead);
extern void	rtw_list_delete(_list *plist);

void _task_enter_critical(void);
#define taskENTER_CRITICAL _task_enter_critical

void _task_exit_critical(void);
#define taskEXIT_CRITICAL _task_exit_critical

void _task_thread_exit(void *task);
#define vTaskDelete _task_thread_exit

#endif /* _ZEPHYR_SERVICE_H_ */
