#ifndef _RTOS_DEF_H_
#define _RTOS_DEF_H_
#include <zephyr.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define  PRINT printf
#else /* !CONFIG_STDOUT_CONSOLE */
#include <misc/printk.h>
#define PRINT printk
#endif /* CONFIG_STDOUT_CONSOLE */

/*============OS parameter setting===================*/
typedef void (*OsTask)(void *);

typedef struct k_thread *   OsTaskHandle;
typedef struct k_mutex *	OsMutex;
typedef struct k_sem *		OsSemaphore;
typedef struct k_msgq *		OsMsgQ;
typedef int                OsTimer;
typedef int           OsTimerHandler;

#define OS_TASK_ENTER_CRITICAL()       int task_key; task_key = OS_EnterCritical()

#define OS_TASK_EXIT_CRITICAL()        OS_ExitCritical(task_key)

#define TICK_RATE_MS (1000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)


#endif
