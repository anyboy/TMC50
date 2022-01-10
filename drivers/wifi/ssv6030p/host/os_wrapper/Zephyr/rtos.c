/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include "rtos.h"
#include <log.h>


volatile u8 gOsFromISR = 0;
static u8 os_init_flag = 0;

#define STATIC_STK
#ifdef STATIC_STK
//(TOTAL_STACK_SIZE<<4) --> transfer unit to bytes
//((TOTAL_STACK_SIZE<<4)>>2) --> transfer unit to word

/*============Stack Size (unint: 16bytes)===================*/
#define WIFI_TASK_CNT			5
#define WIFI_RX_STACK_SIZE_EXT	6
#define WIFI_TX_STACK_SIZE_EXT	8
#define CMD_ENG_STACK_SIZE_EXT	6
#define NETMGR_STACK_SIZE_EXT	6

const u32 pri_stack_size[5][2] = {
	{TMR_TASK_PRIORITY, 	TMR_TASK_STACK_SIZE},
	{CMD_ENG_PRIORITY, 		CMD_ENG_STACK_SIZE + CMD_ENG_STACK_SIZE_EXT},
	{NETMGR_PRIORITY, 		NETMGR_STACK_SIZE + NETMGR_STACK_SIZE_EXT},
	{WIFI_RX_PRIORITY, 		(WIFI_RX_STACK_SIZE + WIFI_RX_STACK_SIZE_EXT)},
	{WIFI_TX_PRIORITY, 		WIFI_TX_STACK_SIZE + WIFI_TX_STACK_SIZE_EXT}
};

#define TOTAL_STACK_SIZE_ALL (TOTAL_STACK_SIZE+\
							   WIFI_RX_STACK_SIZE_EXT + \
							   WIFI_TX_STACK_SIZE_EXT + \
							   CMD_ENG_STACK_SIZE_EXT + \
							   NETMGR_STACK_SIZE_EXT)

u32 task_stack[((TOTAL_STACK_SIZE_ALL<<4)>>2)];
u32 stk_used=0;
OsMutex StkMutex;
#endif

u32 task_stack_size = 0;

OS_APIs s32  OS_Init( void )
{
    if (os_init_flag == 1)
    {
        return 0;
    }

    os_init_flag = 1;
    
    gOsFromISR = 0;
    //enable uc-os timer
    //timer_freq_setup(0, OS_TICKS_PER_SEC, 0, OSTimeTick);
#ifdef STATIC_STK
    OS_MutexInit(&StkMutex);
    task_stack_size = ((TOTAL_STACK_SIZE_ALL<<4)>>2) * 4;
    OS_MemSET(task_stack,0,task_stack_size);
    //OS_MemSET((void *)wifi_task_stack,0,sizeof(wifi_task_stack));
#endif
#ifdef STATIC_STK
    LOG_PRINTF("Total Stack size is 0x%x (bytes)\r\n",task_stack_size);
#endif
    return OS_SUCCESS;
}

OS_APIs unsigned long OS_Random(void)
{
	return  k_uptime_get_32()%65536+54*18;
}

OS_APIs void OS_Terminate( void )
{
}

OS_APIs u32 OS_EnterCritical(void)
{
	int key = irq_lock();
    return key;
}

OS_APIs void OS_ExitCritical(u32 val)
{
    irq_unlock(val);
}

/* Task: */

static void dummy_task(void *p1, void *p2, void* p3)
{
    OsTask p_task_entry = (OsTask)p1;
	void * p_task_param = (void *)p2;

    if (p_task_entry)
    {
       p_task_entry(p_task_param);
    }
    else
    {
        LOG_PRINTF("dummy_task: pThread Is NULL\r\n");
    }
}

OS_APIs s32 OS_TaskCreate( OsTask task, const char *name, u32 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle )
{
    u32* stk_ptr = NULL;
    k_tid_t tid = 0;
    u32 stk_size=(stackSize>>2); //transfer unit to word
    u32 thread_size, i;
    struct k_thread *thread;

    if (stk_size == 0)
    {
        LOG_ERROR("OS_TaskCreate %s stk_size is 0 !!\n", name);
        return OS_FAILED;
    }

#ifdef STATIC_STK
    OS_MutexLock(StkMutex);
	for (i=0; i<WIFI_TASK_CNT; i++) {
		if (pri == pri_stack_size[i][0]) {
			stackSize = pri_stack_size[i][1] * 16;
			stk_size=(stackSize>>2);
			break;
		}
	}

    if((stk_used+stk_size)>(task_stack_size>>2)){
        LOG_ERROR("Stack is not enough for new task %s\r\n",name);
        ASSERT(FALSE);
    }
    stk_ptr=&task_stack[stk_used];
    stk_used+=stk_size;
    LOG_PRINTF("Free Stack size is 0x%x (bytes)\r\n",(task_stack_size-(stk_used<<2)));
    OS_MutexUnLock(StkMutex);    
#else
    stk_ptr = (void*)OS_MemAlloc(stackSize);
    if(! stk_ptr){
        LOG_ERROR("alloc %s stack fail\n",name);
        return OS_FAILED;
    }
#endif

    thread = (struct k_thread *)stk_ptr;
    thread_size = (sizeof(struct k_thread) + 3)/4;
    stk_ptr = &stk_ptr[thread_size];
    stk_size -= thread_size;
    stackSize -= (thread_size*4);
    tid = k_thread_create(thread, (k_thread_stack_t)stk_ptr, stackSize, dummy_task, task, param, NULL, K_PRIO_PREEMPT(pri), 0, K_NO_WAIT);
    LOG_PRINTF("OS_TaskCreate = %s,tid=%p,pri=%d,stackSize=%d, stack top=0x%x, stack botton=0x%x\r\n", name, tid, pri,stackSize,(unsigned int)&stk_ptr[stk_size-1],(unsigned int)&(stk_ptr[0]));

	if(tid != NULL)
    {
    	if (taskHandle)
			*taskHandle = tid;
        return OS_SUCCESS;
    }
    else
        LOG_PRINTF("OSTaskCreate tid = %p error \r\n", tid);

    return OS_FAILED;
}


OS_APIs void OS_TaskDelete(OsTaskHandle taskHandle)
{
    int ret = 0;

    if (taskHandle)
    {
        ret  = k_thread_cancel(taskHandle);
        if (ret != 0)
        {
            LOG_PRINTF("OS_TaskDelete(%p) ret=%d, tid = %p\r\n", taskHandle, ret, k_current_get());
        }

        OS_MemFree(taskHandle);
    }
}

OS_APIs void OS_StartScheduler( void )
{
}

OS_APIs u32 OS_GetSysTick(void)
{
    return k_uptime_get_32()/TICK_RATE_MS;
}

OS_APIs void OS_TickDelay(u32 ticks)
{
    k_sleep (ticks * TICK_RATE_MS);
}

/* Mutex APIs: */
OS_APIs s32 OS_MutexInit( OsMutex *mutex )
{
    struct k_mutex *p_mutex = NULL;
    p_mutex = OS_MemAlloc(sizeof(struct k_mutex));
    if (NULL == p_mutex)
    {
        LOG_PRINTF("%s(%d):malloc fail, tid = %p\r\n",__FUNCTION__,__LINE__, k_current_get());
        return OS_FAILED;
    }

    k_mutex_init(p_mutex);
    *mutex = p_mutex;
    
    return OS_SUCCESS;
}

OS_APIs void OS_MutexLock( OsMutex mutex )
{
    int  ret = 0;
    if ( NULL == mutex ){
        LOG_PRINTF("OS_MutexLock mutex = NULL fail !!!\n");
        return;
    }
    ret = k_mutex_lock(mutex, K_FOREVER);
    if(ret!=0) 
    {
        LOG_PRINTF("OS_MutexLock ret = %d, tid = %p!!!\n", ret, k_current_get());
        assert(FALSE);
    }
}

OS_APIs void OS_MutexUnLock( OsMutex mutex )
{
    if ( NULL == mutex ){
        LOG_PRINTF("OS_MutexLock mutex = NULL fail !!!\n");
        return;
    }
    
    k_mutex_unlock(mutex);
    
    return;
}

OS_APIs void OS_MutexDelete( OsMutex mutex )
{
    if (NULL == mutex)
    {
        LOG_PRINTF("OsMutex Is NULL\r\n");
        return;
    }

    OS_MemFree(mutex);
}

OS_APIs void OS_MsDelay(u32 ms)
{
    k_sleep(ms);
}

OS_APIs s32 OS_SemInit( OsSemaphore* Sem, u16 maxcnt, u16 cnt)
{
    *Sem = (OsSemaphore)OS_MemAlloc(sizeof(struct k_sem));
    if(NULL==*Sem)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return OS_FAILED;
    }
    
    k_sem_init(*Sem, cnt, maxcnt);
    return OS_SUCCESS;
}

OS_APIs bool OS_SemWait( OsSemaphore Sem , u16 timeout)
{
    int ret = 0;
    
    if (NULL == Sem)
    {
        LOG_PRINTF("OsSemaphore Is NULL\r\n");
        return OS_FAILED;
    }
    
    ret = k_sem_take(Sem, (timeout == 0) ? K_FOREVER : (timeout * TICK_RATE_MS));
    if (ret != 0) 
    {
        if (ret != -EAGAIN)
        {
            LOG_PRINTF("k_sem_take failed ret = %d, tid = %p\r\n", ret, k_current_get());
        }
        
        return OS_FAILED;
    }
    else 
    {
        return OS_SUCCESS;
    }
}

OS_APIs u8 OS_SemSignal( OsSemaphore Sem)
{
    if (NULL == Sem)
    {
        LOG_PRINTF("OsSemaphore Is NULL\r\n");
        return OS_FAILED;
    }
    
    k_sem_give(Sem);

	return OS_SUCCESS;
}

OS_APIs u32 OS_SemCntQuery( OsSemaphore Sem)
{
    if (NULL == Sem)
    {
        LOG_PRINTF("OsSemaphore Is NULL\r\n");
        return OS_FAILED;
    }
    return k_sem_count_get(Sem);
}

OS_APIs u8 OS_SemSignal_FromISR( OsSemaphore Sem)
{
	return OS_SemSignal(Sem);
}

OS_APIs void OS_SemDelete(OsSemaphore Sem)
{
    if (NULL == Sem)
    {
        LOG_PRINTF("OsMutex Is NULL\r\n");
        return;
    }

    OS_MemFree(Sem);
}

/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen )
{
    OsMsgQ tmpq;
    int size = sizeof(void *) * QLen;
    void* qpool = OS_MemAlloc(size);

    if(qpool)
    {
        OS_MemSET((void *)qpool, 0, size);	/* clear out msg q structure */
        
        tmpq = OS_MemAlloc(sizeof(struct k_msgq));

        if (!tmpq)
        {
            LOG_PRINTF("%s, size = %d, tid = %p\r\n",__func__, (int)sizeof(struct k_msgq), k_current_get());
            OS_MemFree(qpool);
            *MsgQ = NULL;
            return OS_FAILED;
        }
        
        k_msgq_init(tmpq, qpool, sizeof(void *), QLen);
        
        *MsgQ = tmpq;
    }
    else
    {
        LOG_PRINTF("%s, size = %d, tid = %p\r\n", __func__, size, k_current_get());
    }

    return OS_SUCCESS;
}

OS_APIs s32 OS_MsgQDelete( OsMsgQ MsgQ)
{
    if (NULL == MsgQ)
    {
        LOG_PRINTF("OsMsgQ Is NULL\r\n");
        return OS_FAILED;
    }

    OS_MemFree((void *)(MsgQ->buffer_start));
    OS_MemFree(MsgQ);

    return OS_SUCCESS;
}

OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    int ret = 0;
    
    if (NULL == MsgQ)
    {
        LOG_PRINTF("OsMsgQ Is NULL\r\n");
        return OS_FAILED;
    }

	ret = k_msgq_put(MsgQ, &MsgItem->MsgData, K_FOREVER);
    if ( 0!=ret )
    {
        LOG_PRINTF("k_msgq_put err = %d, tid = %p\r\n",  ret, k_current_get());
    }
    
    return ( 0!=ret )? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQEnqueueTry( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    int ret = 0;
     
    if (NULL == MsgQ)
    {
        LOG_PRINTF("OsMsgQ Is NULL\r\n");
        return OS_FAILED;
    }

	ret = k_msgq_put(MsgQ, &MsgItem->MsgData, K_NO_WAIT);
    if ((0 != ret ) && (-ENOMSG != ret))
    {
        LOG_PRINTF("k_msgq_put err = %d, tid = %p\r\n", ret, k_current_get());
    }
    
    return ( 0!=ret )? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, u32 timeOut, bool fromISR )
{
    int ret = 0;
    
    if (NULL == MsgQ)
    {
        LOG_PRINTF("OsMsgQ Is NULL\r\n");
        return OS_FAILED;
    }

	ret = k_msgq_get(MsgQ, &MsgItem->MsgData, (timeOut == 0) ? K_FOREVER : (timeOut * TICK_RATE_MS));

    MsgItem->MsgCmd = 0;
    
    if ((ret != 0 )&&(ret != -EAGAIN))
    {
        LOG_PRINTF("k_msgq_get failed ret = %d, tid = %p\r\n", ret, k_current_get());
    }
    
    return ( 0 != ret ) ? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ )
{
    return MsgQ ? MsgQ->used_msgs : 0;
}

/* Timer: */
OS_APIs s32 OS_TimerCreate( OsTimer *timer, u32 ms, u8 autoReload, void *args, OsTimerHandler timHandler )
{
    return OS_SUCCESS;

}

OS_APIs s32 OS_TimerSet( OsTimer timer, u32 ms, u8 autoReload, void *args )
{
    return OS_SUCCESS;

}

OS_APIs s32 OS_TimerStart( OsTimer timer )
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_TimerStop( OsTimer timer )
{
    return OS_SUCCESS;
}

OS_APIs void *OS_TimerGetData( OsTimer timer )
{
   return NULL;
}

#ifdef __TEST_BANDWIDTH_RX
int g_test_bandwidth = -1;
static unsigned int tick1[24],tick2[24];
static unsigned int packets_len[24] = {0};
int test_bandwidth_rx(int num, char * file, int line_num, unsigned int len)
{
    if ((g_test_bandwidth == num) && (len > 1000))
    {
        if (packets_len[num] == 0)
        {
            tick1[num] = OS_GetSysTick();
        }
        packets_len[num] += len;
        tick2[num] = (OS_GetSysTick() - tick1[num]);
        if (tick2[num] >= (1000 / TICK_RATE_MS))
        {
            if (g_test_bandwidth == num)
            {
                LOG_PRINTF("[%d]%s, line %d::  Rx %d Mbps, packets_len %d, tick = %d\r\n", g_test_bandwidth, file, line_num, ((packets_len[num] * 8) / 1024) / (tick2[num] * TICK_RATE_MS),  packets_len[num], tick2[num]);
            }
            packets_len[num] = 0;
        }

        if (g_test_bandwidth == num)
            return 1;
    }
    return 0;
}

int test_tx_data(void *data, int len)
{
    //LOG_PRINTF("test_tx_thread\r\n");
    return TXRXTask_FrameEnqueue(data, 0) ? 0 : -1;
    #if 0
    g_data_len = len;
    g_data      = data;
    while (OS_SemSignal(tx_data_sphr) != 0)
    {
         OS_TickDelay(1);
    }
    #endif
}
#endif

