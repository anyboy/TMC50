/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file trace os interface
 */

#include <init.h>
#include <kernel.h>
#include <cbuf.h>
#include <trace.h>
#include <trace_impl.h>
#include <kernel_structs.h>

#ifdef CONFIG_TRACE_EVENT

#define ROUND_POINT(sz) (((sz) + (4 - 1)) & ~(4 - 1))

#define TRACE_PACKET_LENGTH 48
#define MAX_MODULE_NAME 40
#define TRACE_TYPE  0XFFFFFF00
#define TRACE_EVENT 0X000000FF


/****************************************************************************/
/* Trace packet header                                                      */
/****************************************************************************/
/* 0:  SYNC                                                                 */
/* 1:  Length                                                               */
/* 2:  SequenceNumber                                                       */
/* 3:  Checksum                                                             */
/* 4:  Timestamp                                                            */
/* 8:  event                                                                */
/****************************************************************************/

#define TRACE_SYNC_CODE             0x7E

#define TRACE_CHECKSUM_LEN              (sizeof(uint8_t) +      /* Sync   */ \
                                         sizeof(uint8_t) +      /* Length */ \
                                         sizeof(uint8_t))       /* SequenceNumber */

#define TRACE_PAYLOAD_START         (3)

typedef enum
{
    TRACE_GROUP_TASK        = 1,
    TRACE_GROUP_SEM         = 2,
    TRACE_GROUP_MUTEX       = 3,
    TRACE_GROUP_WORK        = 4,
    TRACE_GROUP_TIMEOUT     = 5,
    TRACE_GROUP_MALLOC      = 6,
    TRACE_GROUP_BTSTREAM    = 7,
}trace_group_e;

typedef enum
{
    TASK_TRACE_SWITCH       = 0x101,
    TASK_TRACE_CREATE       = 0x103,
    TASK_TRACE_SLEEP        = 0x104,
    TASK_TRACE_PRI_CHANGE   = 0x105,
    TASK_TRACE_SUSPEND      = 0x105,
    TASK_TRACE_RESUME       = 0x106,
    TASK_TRACE_DEL          = 0x107,
    TASK_TRACE_ABORT        = 0x108,
}trace_task_type_e;

typedef enum
{
    SEM_TRACE_CREATE        = 0x201,
    SEM_TRACE_DEL           = 0x202,
    SEM_TRACE_TAKE          = 0x203,
    SEM_TRACE_GIVE          = 0x204,
}trace_sem_type_e;

typedef enum
{
    MEM_TRACE_MALLOC        = 0x601,
    MEM_TRACE_FREE          = 0x602,
}trace_malloc_type_e;

typedef struct
{
    uint8_t sync;
    uint8_t length;
    uint8_t seq;
    uint8_t checksum;
    uint32_t timestamp;
    uint32_t event;
}trace_header_t;

static uint32_t event_mask;
static struct k_thread *hit_thread;
static struct k_thread *filter_thread;
static uint8_t g_seq_num;
static struct k_thread *last_thread;

int32_t set_filter_task(int8_t prio, uint32_t is_filter)
{
	struct k_thread *thread_list = NULL;

	thread_list   = ((struct k_thread *)(_kernel.threads));
	while (thread_list != NULL) {
        if(os_thread_priority_get(thread_list) == prio){
            if(is_filter){
                filter_thread = thread_list;
            }else{
                hit_thread = thread_list;
            }
            return 0;
        }
		thread_list = (struct k_thread *)SYS_THREAD_MONITOR_NEXT(thread_list);
	}

	printk("thread prio %d not exist\n", prio);

    return -EINVAL;
}

int set_event_mask(const uint32_t mask)
{
    uint32_t event = event_mask;
    event_mask = mask;
    return event;
}

/* task trace function */
void trace_binary_init(cbuf_t *cbuf, char *buffer, int size)
{
    if(!cbuf_is_init(cbuf)) {
        cbuf_init(cbuf, buffer, size);
        event_mask = 0;
    }
}

static uint32_t _trace_is_active(struct k_thread  *thread, uint32_t event)
{
    //default not enable
    if (!event_mask){
        return 0;
    }  

    if (hit_thread != NULL && hit_thread != thread) {
        return 0;
    }

    if (filter_thread != NULL && filter_thread == thread) {
        return 0;
    }

    /*when event_mask represents an event, filter exact event*/
    if ((event_mask & TRACE_EVENT) != 0 && event_mask != event) {
        return 0;
    }

    /*when event_mask represents an event type, filter match type*/
    if (((event_mask & TRACE_TYPE) != 0) && (event_mask & TRACE_TYPE) != (event & TRACE_TYPE)) {
        return 0;
    }  
}


static void _trace_binary_write(struct k_thread  *thread, const void *buf, uint32_t len, uint32_t event)
{
    uint32_t i;

    uint8_t *pdata = (uint8_t *)buf;

    trace_header_t *trace_head = (trace_header_t *)buf;
  
    trace_ctx_t *trace_ctx = get_trace_ctx();
   
    trace_head->sync = TRACE_SYNC_CODE;
    trace_head->length = len + sizeof(trace_header_t);
    trace_head->seq = g_seq_num++;

    __ASSERT_NO_MSG(trace_head->length <= TRACE_PACKET_LENGTH);

    trace_head->timestamp = _timer_cycle_get_32();

    trace_head->event = event;

    trace_head->checksum = *pdata++;

    for (i = 1; i < trace_head->length; i++)
    {
        if(i != 3){
            trace_head->checksum += *pdata;
        }

        pdata++;
    }    

    cbuf_write(&trace_ctx->cbuf_binary, buf, trace_head->length);

    trigger_trace_task(TRACE_EVENT_TX);    
}

static int _copy_task_name(struct k_thread  *thread, char *buf)
{
    uint32_t  str_len = 0;

    strcpy(buf, task_evt2str(thread->base.prio));
    str_len = strlen(task_evt2str(thread->base.prio)) + 1;

    return str_len;
}

void _trace_task_switch(struct k_thread  *from, struct k_thread  *to)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(from, TASK_TRACE_SWITCH)){
        return;
    }

    if (to->base.prio == CONFIG_CONSOLE_SHELL_PRIORITY){
        last_thread = from; 
        return;
    }
    
    if (from->base.prio == CONFIG_CONSOLE_SHELL_PRIORITY){
        if(to == last_thread){
            return;
        }else{
            from = last_thread;
        }
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)from->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)to->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];

    copy_len = _copy_task_name(from, addr_second);

    addr_second += copy_len;
    str_len += copy_len;

    copy_len = _copy_task_name(to, addr_second);

    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);
   
    _trace_binary_write(from, buf, 8 + str_len, TASK_TRACE_SWITCH);
}


void _trace_task_create(struct k_thread  *from)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(from, TASK_TRACE_CREATE)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)from->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 1];
    copy_len = _copy_task_name(from, addr_second);

    addr_second += copy_len;

    str_len += copy_len;
    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 4 + str_len;

    _trace_binary_write(from, buf, addr_second - addr_first, TASK_TRACE_CREATE);
}

void _trace_task_sleep(struct k_thread *thread, int32_t ticks)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(thread, TASK_TRACE_SLEEP)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)thread->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 1];
    copy_len = _copy_task_name(thread, addr_second);
    addr_second += copy_len;

    str_len += addr_second;
    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 4 + str_len;

    *(uint32_t *)addr_second = (uint32_t)ticks;
    addr_second += 4;

    _trace_binary_write(thread, buf, addr_second - addr_first, TASK_TRACE_SLEEP);
}

void _trace_task_pri_change(struct k_thread *current, struct k_thread *changed, int8_t pri)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, TASK_TRACE_PRI_CHANGE)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)changed->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    copy_len = _copy_task_name(changed, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    *(uint32_t *)addr_second = (uint32_t)pri;
    addr_second += 4;

    _trace_binary_write(current, buf, addr_second - addr_first, TASK_TRACE_PRI_CHANGE);
}

void _trace_task_suspend(struct k_thread *current, struct k_thread *changed)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, TASK_TRACE_SUSPEND)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)changed->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    copy_len = _copy_task_name(changed, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, TASK_TRACE_SUSPEND);
}

void _trace_task_resume(struct k_thread *current, struct k_thread *changed)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, TASK_TRACE_RESUME)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)changed->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    copy_len = _copy_task_name(changed, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, TASK_TRACE_RESUME);
}

void _trace_task_del(struct k_thread *current, struct k_thread *changed)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, TASK_TRACE_DEL)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)changed->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    copy_len = _copy_task_name(changed, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, TASK_TRACE_DEL);
}

void _trace_task_abort(struct k_thread *current, struct k_thread *changed)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, TASK_TRACE_ABORT)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)changed->base.thread_state;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    copy_len = _copy_task_name(changed, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, TASK_TRACE_ABORT);
}

/* semaphore trace function */
void _trace_sem_create(struct k_thread *current, struct k_sem *sem)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, SEM_TRACE_CREATE)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)sem;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, SEM_TRACE_CREATE);
}

/* semaphore trace function */
void _trace_sem_destory(struct k_thread *current, struct k_sem *sem)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, SEM_TRACE_DEL)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)sem;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, SEM_TRACE_DEL);
}


void _trace_sem_take(struct k_thread *current, struct k_sem *sem)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, SEM_TRACE_TAKE)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)sem;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, SEM_TRACE_TAKE);
}

void _trace_sem_give(struct k_thread *current, struct k_sem *sem)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_first = (char *)(buf + TRACE_PAYLOAD_START);
    char     *addr_second;
    uint32_t  str_len = 0;
    uint32_t  copy_len;

    if(!_trace_is_active(current, SEM_TRACE_GIVE)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)current->base.thread_state;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)sem;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    copy_len = _copy_task_name(current, addr_second);
    addr_second += copy_len;
    str_len += copy_len;

    str_len = ROUND_POINT(str_len);

    addr_second = addr_first + 8 + str_len;

    _trace_binary_write(current, buf, addr_second - addr_first, SEM_TRACE_GIVE);
}

void _trace_malloc(uint32_t address, uint32_t malloc_size, uint32_t caller)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_second;
    uint32_t  str_len = 0;
    struct k_thread *current = k_current_get();

    if(!_trace_is_active(current, MEM_TRACE_MALLOC)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)address;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)malloc_size;
    buf[TRACE_PAYLOAD_START + 2] = (uint32_t)caller;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 3];
    str_len = _copy_task_name(current, addr_second);
   
    str_len = ROUND_POINT(str_len);

    _trace_binary_write(current, buf, 12 + str_len, MEM_TRACE_MALLOC);    
}

void _trace_free(uint32_t address, uint32_t caller)
{
    uint32_t  buf[TRACE_PACKET_LENGTH / 4];
    char     *addr_second;
    uint32_t  str_len = 0;
    struct k_thread *current = k_current_get();

    if(!_trace_is_active(current, MEM_TRACE_FREE)){
        return;
    }

    buf[TRACE_PAYLOAD_START] = (uint32_t)address;
    buf[TRACE_PAYLOAD_START + 1] = (uint32_t)caller;

    addr_second = (char *)&buf[TRACE_PAYLOAD_START + 2];
    str_len = _copy_task_name(current, addr_second);
   
    str_len = ROUND_POINT(str_len);

    _trace_binary_write(current, buf, 8 + str_len, MEM_TRACE_FREE);        
}

#endif




