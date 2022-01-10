#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <mem_manager.h>
#include "../app/wifi_conf.h"
#include "zephyr_service.h"


//#define DEBUG_ZEPHYR_SERVICE	1

#ifdef DEBUG_ZEPHYR_SERVICE
#define ZEPHYR_SERVICE_DEBUG(fmt, ...) printk("[%s,%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ZEPHYR_SERVICE_DEBUG(fmt, ...)
#endif

struct zephyr_wifi_timer {
	struct k_delayed_work delay_work;		/* Keep in first  position */
	u8_t submit;
	u8_t runing;
	u8_t autoReload;
	osdepTickType PeriodInTicks;
	void *user_data;
	void (*timer_fn)(void *);
};

/* extern function */
extern uint32_t _tick_get_32(void);

#define BTC_MEM_DEBUG		0

#if BTC_MEM_DEBUG
#define BTC_MEM_DEBUG_MAX	100
static uint32_t btc_max_malloc_cut = 0;
static uint32_t btc_cur_malloc_cut = 0;
static uint32_t btc_max_memory_used = 0;
static uint32_t btc_cur_memory_used = 0;

static uint32_t btc_mem_debug[BTC_MEM_DEBUG_MAX][2];

static void wifi_mem_debug_malloc(void *addr, uint32_t size)
{
	uint32_t i;

	btc_cur_malloc_cut++;
	if (btc_cur_malloc_cut > btc_max_malloc_cut) {
		btc_max_malloc_cut = btc_cur_malloc_cut;
	}

	btc_cur_memory_used += size;
	if (btc_cur_memory_used > btc_max_memory_used) {
		btc_max_memory_used = btc_cur_memory_used;
	}

	for (i=0; i<BTC_MEM_DEBUG_MAX; i++) {
		if (btc_mem_debug[i][0] == 0) {
			btc_mem_debug[i][0] = (uint32_t)addr;
			btc_mem_debug[i][1] = size;
			return;
		}
	}

	printk("%s not have enough index for debug, addr:%p\n", __func__, addr);
}

static void wifi_mem_debug_free(void *addr)
{
	uint32_t i;

	btc_cur_malloc_cut--;

	for (i=0; i<BTC_MEM_DEBUG_MAX; i++) {
		if (btc_mem_debug[i][0] == (uint32_t)addr) {
			btc_mem_debug[i][0] = 0;
			btc_cur_memory_used -= btc_mem_debug[i][1];
			return;
		}
	}

	printk("%s unknow addr:%p\n", __func__, addr);
}

void wifi_mem_debug_print(void)
{
	uint32_t i, flag = 0;

	printk("btc mem max mem_malloc cnt:%d, btc_cur_malloc_cut:%d\n", btc_max_malloc_cut, btc_cur_malloc_cut);
	printk("btc btc_max_memory_used:%d, btc_cur_memory_used:%d\n", btc_max_memory_used, btc_cur_memory_used);

	for (i=0; i<BTC_MEM_DEBUG_MAX; i++) {
		if (btc_mem_debug[i][0] != 0) {
			printk("Unfree mem addr:0x%x, size:%d\n", btc_mem_debug[i][0], btc_mem_debug[i][1]);
			flag = 1;
		}
	}

	if (!flag) {
		printk("All mem mem_free!\n");
	}
}
#else
#define wifi_mem_debug_malloc(addr, size)
#define wifi_mem_debug_free(addr)
#define wifi_mem_debug_print()
#endif

static char wifi_large_mem_0[3992] __aligned(4);
static char wifi_large_mem_1[5700] __aligned(4);
static char wifi_large_mem_2[2048] __aligned(4);
static u8_t large_mem_0_used = 0;
static u8_t large_mem_1_used = 0;
static u8_t large_mem_2_used = 0;

#define TRY_ALLOC_WIFI_LARGE_MEM(idx, size) \
	do { \
		if (size == sizeof(wifi_large_mem_##idx)) { \
			PR_DBG("alloc large_memory_" #idx ": %zu", size); \
			if (large_mem_##idx##_used) { \
				PR_ERR("multi used large_memory_" #idx); \
				return NULL; \
			} \
			large_mem_##idx##_used = true; \
			return wifi_large_mem_##idx; \
		} \
	} while (0)

#define TRY_FREE_WIFI_LARGE_MEM(idx, ptr) \
	do { \
		if (ptr == wifi_large_mem_##idx) { \
			PR_DBG("mem_free large_memory_" #idx); \
			large_mem_##idx##_used = false; \
			return; \
		} \
	} while (0)

void *wifi_mem_malloc(unsigned int size)
{
	TRY_ALLOC_WIFI_LARGE_MEM(0, size);
	TRY_ALLOC_WIFI_LARGE_MEM(1, size);
	TRY_ALLOC_WIFI_LARGE_MEM(2, size);

	/* Wifi receive data packet larger than 1640, not for me, just drop */
	if (size > 1536) {
		PR_WRN("too large %u > %d", size, 1536);
		return NULL;
	}

	void *ptr = mem_malloc(size);
	if (ptr) {
		wifi_mem_debug_malloc(ptr, size);
	} else {
		PR_ERR("failed (size=%zu)", size);
		//dump_stack();
	}

	return ptr;
}

void wifi_mem_free(void *ptr)
{
	if (!ptr)
		return;

	TRY_FREE_WIFI_LARGE_MEM(0, ptr);
	TRY_FREE_WIFI_LARGE_MEM(1, ptr);
	TRY_FREE_WIFI_LARGE_MEM(2, ptr);

	wifi_mem_debug_free(ptr);
	mem_free(ptr);
}

/* Enter critical */
void save_and_cli(void)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_lock();
}

/* Exit critical */
void restore_flags(void)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_unlock();
}

/*
* Call cli to lock irq and enter halt,
* so not find interfase to unlock irq
*/
void cli(void)
{
	printk("%s,%d, Enter halt!!!!!!!!!!\n", __func__, __LINE__);
	k_sched_lock();
}

/* Not needed on 64bit architectures */
static unsigned int __div64_32(u64 *n, unsigned int base)
{
	u64 rem = *n;
	u64 b = base;
	u64 res, d = 1;
	unsigned int high = rem >> 32;

	/* Reduce the thing a bit first */
	res = 0;
	if (high >= base) {
		high /= base;
		res = (u64) high << 32;
		rem -= (u64) (high * base) << 32;
	}

	while ((u64)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}

static u8* _zephyr_malloc(u32 sz)
{
	u8 *p = wifi_mem_malloc(sz);

	return p;
}

static u8* _zephyr_zmalloc(u32 sz)
{
	u8 *pbuf = _zephyr_malloc(sz);

	if (pbuf != NULL)
		memset(pbuf, 0, sz);

	return pbuf;	
}

static void _zephyr_mfree(u8 *pbuf, u32 sz)
{
	wifi_mem_free(pbuf);
}

static void _zephyr_memcpy(void* dst, void* src, u32 sz)
{
	memcpy(dst, src, sz);
}

static int _zephyr_memcmp(void *dst, void *src, u32 sz)
{
	if (!(memcmp(dst, src, sz)))
		return 1;

	return 0;
}

static void _zephyr_memset(void *pbuf, int c, u32 sz)
{
	memset(pbuf, c, sz);
}

static void _zephyr_init_sema(_sema *sema, int init_val)
{
	*sema = wifi_mem_malloc(sizeof(struct k_sem));

	if (*sema == NULL) {
		printk("%s, %d, Can't mem_malloc memory!\n", __func__, __LINE__);
	} else {
		k_sem_init((struct k_sem *)*sema, init_val, 0xffffffff);		//Set max count 0xffffffff
	}
}

static void _zephyr_free_sema(_sema *sema)
{
	if(*sema != NULL)
		wifi_mem_free(*sema);

	*sema = NULL;
}

static void _zephyr_up_sema(_sema *sema)
{
	k_sem_give((struct k_sem *)*sema);
}

static u32 _zephyr_down_sema(_sema *sema, u32 timeout)
{
	if (k_sem_take((struct k_sem *)*sema, timeout) != 0) {
		printk("%s, failed, timeout:%d\n", __func__, timeout);
		return 0;
	}

	return 1;
}

static void _zephyr_mutex_init(_mutex *pmutex)
{
	*pmutex = wifi_mem_malloc(sizeof(struct k_mutex));

	if (*pmutex == NULL) {
		printk("%s, %d, Can't mem_malloc memory!\n", __func__, __LINE__);
	} else {
		k_mutex_init((struct k_mutex *)*pmutex);
	}
}

static void _zephyr_mutex_free(_mutex *pmutex)
{
	if(*pmutex != NULL)
		wifi_mem_free(*pmutex);

	*pmutex = NULL;
}

static void _zephyr_mutex_get(_mutex *pmutex)
{
	k_mutex_lock((struct k_mutex *)*pmutex, K_FOREVER);
}

static void _zephyr_mutex_put(_mutex *pmutex)
{
	k_mutex_unlock((struct k_mutex *)*pmutex);
}

static void _zephyr_enter_critical(_lock *plock, _irqL *pirqL)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_lock();
}

static void _zephyr_exit_critical(_lock *plock, _irqL *pirqL)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_unlock();
}

static int _zephyr_enter_critical_mutex(_mutex *pmutex, _irqL *pirqL)
{
	/* Insure pmutex initialize from  _zephyr_mutex_init */
	k_mutex_lock((struct k_mutex *)*pmutex, K_FOREVER);
	return 0;
}

static void _zephyr_exit_critical_mutex(_mutex *pmutex, _irqL *pirqL)
{
	/* Insure pmutex initialize from  _zephyr_mutex_init */
	k_mutex_unlock((struct k_mutex *)*pmutex);
}

void _task_enter_critical(void)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_lock();
}

void _task_exit_critical(void)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_unlock();
}

/* spinlock need to use another way??? */
static void _zephyr_spinlock_init(_lock *plock)
{
	*plock = wifi_mem_malloc(sizeof(struct k_mutex));

	if (*plock == NULL) {
		printk("%s, %d, Can't mem_malloc memory!\n", __func__, __LINE__);
	} else {
		k_mutex_init((struct k_mutex *)*plock);
	}
}

static void _zephyr_spinlock_free(_lock *plock)
{
	if(*plock != NULL)
		wifi_mem_free(*plock);

	*plock = NULL;
}

static void _zephyr_spinlock(_lock *plock)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_lock();
}

static void _zephyr_spinunlock(_lock *plock)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_unlock();
}

static void _zephyr_spinlock_irqsave(_lock *plock, _irqL *irqL)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_lock();
}

static void _zephyr_spinunlock_irqsave(_lock *plock, _irqL *irqL)
{
	if (k_is_in_isr()) {
		return;
	}

	k_sched_unlock();
}

static int _zephyr_init_xqueue(_xqueue* queue, const char* name, u32 message_size, u32 number_of_messages)
{
	char *buff = wifi_mem_malloc(message_size*number_of_messages);

	if (buff == NULL) {
		printk("%s, %d, Can't mem_malloc memory!\n", __func__, __LINE__);
		return -1;
	}

	*queue = wifi_mem_malloc(sizeof(struct k_msgq));
	if (*queue == NULL) {
		wifi_mem_free(buff);
		printk("%s, %d, Can't mem_malloc memory!\n", __func__, __LINE__);
		return -1;
	} else {
		k_msgq_init((struct k_msgq *)*queue, buff, message_size, number_of_messages);
	}

	return 0;
}

static int _zephyr_push_to_xqueue(_xqueue* queue, void* message, u32 timeout_ms)
{
	if (0 != k_msgq_put((struct k_msgq *)*queue, message, timeout_ms)) {
		return -1;
	}

	return 0;
}

static int _zephyr_pop_from_xqueue(_xqueue* queue, void* message, u32 timeout_ms)
{
	if (0 != k_msgq_get((struct k_msgq *)*queue, message, timeout_ms)) {
		return -1;
	}

	return 0;
}

static int _zephyr_deinit_xqueue(_xqueue* queue)
{
	struct k_msgq *msgq;

	if (*queue == NULL) {
		return -1;
	}

	msgq = (struct k_msgq *)*queue;
	wifi_mem_free(msgq->buffer_start);
	wifi_mem_free(msgq);

	return 0;
}

static u32 _zephyr_get_current_time(void)
{
	return _tick_get_32();	//The count of ticks since vTaskStartScheduler was called.
}

static u32 _zephyr_systime_to_ms(u32 systime)
{
	return __ticks_to_ms(systime);
}

static u32 _zephyr_systime_to_sec(u32 systime)
{
	return __ticks_to_ms(systime)/1000;
}

static u32 _zephyr_ms_to_systime(u32 ms)
{
	return _ms_to_ticks(ms);
}

static u32 _zephyr_sec_to_systime(u32 sec)
{
	return _ms_to_ticks(sec*1000);
}

static void _zephyr_msleep_os(int ms)
{
	k_sleep(ms);
}

static void _zephyr_usleep_os(int us)
{
	if (us > 1000) {
		k_sleep(us/1000);
		k_busy_wait(us%1000);
	} else {
		k_busy_wait(us);
	}
}

static void _zephyr_mdelay_os(int ms)
{
	k_sleep(ms);
}

static void _zephyr_udelay_os(int us)
{
	if (us > 1000) {
		k_sleep(us/1000);
		k_busy_wait(us%1000);
	} else {
		k_busy_wait(us);
	}
}

static void _zephyr_yield_os(void)
{
	k_yield();
}

static void _zephyr_timer_wrapper(struct k_work *work)
{
	struct zephyr_wifi_timer *timer = CONTAINER_OF(work, struct zephyr_wifi_timer, delay_work);
	_timer *ptimer = (_timer *)timer->user_data;

	timer->runing = 1;

	ZEPHYR_SERVICE_DEBUG("timer:%p\n", ptimer->timer_hdl);
	if (ptimer->function) {
		ptimer->function((void *)ptimer->data);
	}

	timer->runing = 0;
	timer->submit = 0;
}

static void _zephyr_init_timer(_timer *ptimer, void *adapter, TIMER_FUN pfunc,void *cntx, const char *name)
{
	struct zephyr_wifi_timer *timer;

	timer = wifi_mem_malloc(sizeof(struct zephyr_wifi_timer));
	ptimer->timer_hdl  = timer;
	if (ptimer->timer_hdl == NULL) {
		printk("%s, %d, Can't mem_malloc memory!\n", __func__, __LINE__);
		return;
	}

	timer->user_data = ptimer;
	timer->autoReload = 0;
	timer->PeriodInTicks = 0;
	timer->runing = 0;
	timer->submit = 0;

	ptimer->function = pfunc;
	ptimer->data = (u32) cntx;
	k_delayed_work_init(&timer->delay_work, _zephyr_timer_wrapper);
	ZEPHYR_SERVICE_DEBUG("timer:%p\n", ptimer->timer_hdl);
}

static void _zephyr_set_timer(_timer *ptimer, u32 delay_time_ms)
{	
	struct zephyr_wifi_timer *timer;

	ZEPHYR_SERVICE_DEBUG("timer:%p, time:%d\n", ptimer->timer_hdl, delay_time_ms);
	if (ptimer->timer_hdl == NULL) {
		return;
	}

	timer = (struct zephyr_wifi_timer *)ptimer->timer_hdl;
	k_delayed_work_cancel(&timer->delay_work);
	k_delayed_work_submit(&timer->delay_work, delay_time_ms);
	timer->submit = 1;
}

static u8 _zephyr_cancel_timer_ex(_timer *ptimer)
{
	struct zephyr_wifi_timer *timer;

	ZEPHYR_SERVICE_DEBUG("timer:%p\n", ptimer->timer_hdl);
	if (ptimer->timer_hdl == NULL) {
		return -1;
	}

	timer = (struct zephyr_wifi_timer *)ptimer->timer_hdl;
	k_delayed_work_cancel(&timer->delay_work);
	timer->submit = 0;
	return 0;
}

static void _zephyr_del_timer(_timer *ptimer)
{
	int prio;
	struct zephyr_wifi_timer *timer;

	ZEPHYR_SERVICE_DEBUG("timer:%p\n", ptimer->timer_hdl);
	if (ptimer->timer_hdl == NULL) {
		return;
	}

	prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), -1);

	timer = (struct zephyr_wifi_timer *)ptimer->timer_hdl;
	while (timer->runing) {
		k_sleep(10);
	}

	k_delayed_work_cancel(&timer->delay_work);
	wifi_mem_free(ptimer->timer_hdl);
	ptimer->timer_hdl = NULL;

	k_thread_priority_set(k_current_get(), prio);
}

static void _zephyr_ATOMIC_SET(ATOMIC_T *v, int i)
{
	atomic_set(v,i);
}

static int _zephyr_ATOMIC_READ(ATOMIC_T *v)
{
	return atomic_get(v);
}

static void _zephyr_ATOMIC_ADD(ATOMIC_T *v, int i)
{
	atomic_add(v, i);
}

static void _zephyr_ATOMIC_SUB(ATOMIC_T *v, int i)
{
	atomic_sub(v, i);
}

static void _zephyr_ATOMIC_INC(ATOMIC_T *v)
{
	atomic_inc(v);
}

static void _zephyr_ATOMIC_DEC(ATOMIC_T *v)
{
	atomic_dec(v);
}

static int _zephyr_ATOMIC_ADD_RETURN(ATOMIC_T *v, int i)
{
	int tmp;

	save_and_cli();
	atomic_add(v, i);
	tmp = atomic_get(v);
	restore_flags();

	return tmp;
}

static int _zephyr_ATOMIC_SUB_RETURN(ATOMIC_T *v, int i)
{
	int tmp;

	save_and_cli();
	atomic_sub(v, i);
	tmp = atomic_get(v);
	restore_flags();

	return tmp;
}

static int _zephyr_ATOMIC_INC_RETURN(ATOMIC_T *v)
{
	return _zephyr_ATOMIC_ADD_RETURN(v, 1);
}

static int _zephyr_ATOMIC_DEC_RETURN(ATOMIC_T *v)
{
	return _zephyr_ATOMIC_SUB_RETURN(v, 1);
}

static u64 _zephyr_modular64(u64 n, u64 base)
{
	unsigned int __base = (base);
	unsigned int __rem;

	if (((n) >> 32) == 0) {
		__rem = (unsigned int)(n) % __base;
		(n) = (unsigned int)(n) / __base;
	}
	else
		__rem = __div64_32(&(n), __base);
	
	return __rem;
}

static int _zephyr_get_random_bytes(void *buf, u32 len)
{
	int i, count;
	uint8_t *p = (uint8_t *)buf;
	uint32_t *pl = (uint32_t *)buf;
	uint32_t rand_value;

	count = len/4;
	for (i=0; i<count; i++) {
		pl[i] = sys_rand32_get();
	}

	rand_value = sys_rand32_get();
	for (i=(count*4); i<len; i++) {
		p[i] = (rand_value >> (8*(i%4)))&0xFF;
	}

	return 0;
}

static u32 _zephyr_GetFreeHeapSize(void)
{
	/* wait todo */
	return (1024*30);
}

void _zephyr_thread_wrapper(void *parama1, void *parama2, void *parama3)
{
	thread_func_t task_func = parama1;
	void *task_ctx = parama2;

	if (task_func) {
		task_func(task_ctx);
	}
}

#define THREAD_2_STACK_SIZE		(1664 + 512)
#define THREAD_3_STACK_SIZE		1280
static struct k_thread wifi_thread_prio2;
static struct k_thread wifi_thread_prio3;
static K_THREAD_STACK_DEFINE(wifi_thread_prio2_stack, THREAD_2_STACK_SIZE);
static K_THREAD_STACK_DEFINE(wifi_thread_prio3_stack, THREAD_3_STACK_SIZE);

/* return ret: 1, success, 0: failed */
static int _zephyr_create_task(struct task_struct *ptask, const char *name,
	u32  stack_size, u32 priority, thread_func_t func, void *thctx)
{
	u32_t stk_size;
	k_thread_stack_t stack;
	struct k_thread *thread;

	if (priority == 2) {
		stk_size = THREAD_2_STACK_SIZE;
		thread = &wifi_thread_prio2;
		stack = wifi_thread_prio2_stack;
	} else if (priority == 3) {
		stk_size = THREAD_3_STACK_SIZE;
		thread = &wifi_thread_prio3;
		stack = wifi_thread_prio3_stack;
	} else {
		/* TODO: more thread, wait todo */
		printk("TODO: more thread, wait todo\n");
		return 0;
	}

	ptask->task_name = name;
	ptask->blocked = 0;
	ptask->callback_running = 0;

	_zephyr_init_sema(&ptask->wakeup_sema, 0);
	_zephyr_init_sema(&ptask->terminate_sema, 0);

	ptask->task = k_thread_create(thread, (k_thread_stack_t)stack, stk_size, _zephyr_thread_wrapper,
					func, thctx, NULL, priority, 0, K_NO_WAIT);
	if(ptask->task == NULL){
		printk("Create Task \"%s\" Failed!\n", ptask->task_name);
		_zephyr_free_sema(&ptask->wakeup_sema);
		_zephyr_free_sema(&ptask->terminate_sema);
		return 0;
	} else {
		printk("%s: %s, priority:%d, stack size(u32):%d, thread:%p\n", __func__,
			name, priority, stk_size, ptask->task);
		return 1;
	}
}

static void _zephyr_delete_task(struct task_struct *ptask)
{
	if (!ptask->task){
		printk("_freertos_delete_task(): ptask is NULL!\n");
		return;
	}

	ptask->blocked = 1;

	_zephyr_up_sema(&ptask->wakeup_sema);
	_zephyr_down_sema(&ptask->terminate_sema, TIMER_MAX_DELAY);

	_zephyr_free_sema(&ptask->wakeup_sema);
	_zephyr_free_sema(&ptask->terminate_sema);

	ptask->task = 0;

	printk("Delete Task \"%s\"\n", ptask->task_name);
	printk("\n++++++++Warning, _zephyr_delete_task, buf not release task stack!!!!!!!!\n\n");
}

void _zephyr_wakeup_task(struct task_struct *ptask)
{
	_zephyr_up_sema(&ptask->wakeup_sema);
}

static void _zephyr_thread_enter(char *name)
{
	printk("\n\rRTKTHREAD %s\n", name);
}

static void _zephyr_thread_exit(void)
{
	printk("\n\rRTKTHREAD exit %s\n", __func__);
}

void _task_thread_exit(void *task)
{
	printk("\n\rRTKTHREAD exit %s\n", __func__);
}

static void _zephyr_repeat_timer_wrapper(struct k_work *work)
{
	struct zephyr_wifi_timer *timer = CONTAINER_OF(work, struct zephyr_wifi_timer, delay_work);

	timer->runing = 1;
	timer->submit = 0;

	if (timer->autoReload) {
		k_delayed_work_submit(&timer->delay_work, timer->PeriodInTicks);
		timer->submit = 1;
	}

	if (timer->timer_fn) {
		timer->timer_fn(timer);
	}

	timer->runing = 0;
}

_timerHandle _zephyr_timerCreate(const signed char *pcTimerName,
							  osdepTickType xTimerPeriodInTicks,
							  u32 uxAutoReload,
							  void *pvTimerID,
							  TIMER_FUN pxCallbackFunction)
{
	struct zephyr_wifi_timer *timer;
	_timerHandle hdl;

	if(xTimerPeriodInTicks == TIMER_MAX_DELAY) {
		xTimerPeriodInTicks = portMAX_DELAY;
	}

	timer = wifi_mem_malloc(sizeof(struct zephyr_wifi_timer));
	hdl = (_timerHandle)timer;
	if (hdl == NULL) {
		printk("%s, %d, Can't mem_malloc memory!\n", __func__, __LINE__);
		return NULL;
	}

	timer->user_data = NULL;
	timer->autoReload = uxAutoReload;
	timer->PeriodInTicks = xTimerPeriodInTicks;
	timer->timer_fn = pxCallbackFunction;
	timer->runing = 0;

	k_delayed_work_init(&timer->delay_work, _zephyr_repeat_timer_wrapper);
	k_delayed_work_submit(&timer->delay_work, xTimerPeriodInTicks);
	timer->submit = 1;
	ZEPHYR_SERVICE_DEBUG("timer:%p\n", hdl);

	return hdl;
}

u32 _zephyr_timerDelete(_timerHandle xTimer, osdepTickType xBlockTime)
{
	int prio;
	struct zephyr_wifi_timer *timer;

	ZEPHYR_SERVICE_DEBUG("timer:%p\n", xTimer);
	if (xTimer == NULL) {
		return -1;
	}

	prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), -1);

	timer = (struct zephyr_wifi_timer *)xTimer;
	while (timer->runing) {
		k_sleep(10);
	}

	k_delayed_work_cancel(&timer->delay_work);
	k_thread_priority_set(k_current_get(), prio);

	wifi_mem_free(xTimer);
	return 0;
}

u32 _zephyr_timerIsTimerActive(_timerHandle xTimer)
{
	struct zephyr_wifi_timer *timer;

	ZEPHYR_SERVICE_DEBUG("timer:%p\n", xTimer);
	if (xTimer == NULL) {
		return 0;
	}

	timer = (struct zephyr_wifi_timer *)xTimer;
	return timer->submit;
}

u32  _zephyr_timerStop(_timerHandle xTimer,
							   osdepTickType xBlockTime)
{
	struct zephyr_wifi_timer *timer;

	ZEPHYR_SERVICE_DEBUG("timer:%p\n", xTimer);
	if (xTimer == NULL) {
		return -1;
	}

	timer = (struct zephyr_wifi_timer *)xTimer;
	k_delayed_work_cancel(&timer->delay_work);
	timer->submit = 0;

	return 0;
}

u32  _zephyr_timerChangePeriod(_timerHandle xTimer,
							   osdepTickType xNewPeriod,
							   osdepTickType xBlockTime)
{
	struct zephyr_wifi_timer *timer;

	ZEPHYR_SERVICE_DEBUG("timer:%p\n", xTimer);
	if (xTimer == NULL) {
		return -1;
	}

	timer = (struct zephyr_wifi_timer *)xTimer;

	if(xNewPeriod == 0)
		xNewPeriod += 1;

	k_delayed_work_cancel(&timer->delay_work);

	timer->PeriodInTicks = xNewPeriod;
	timer->autoReload = 1;
	timer->submit = 1;
	k_delayed_work_submit(&timer->delay_work, xNewPeriod);

	return 0;
}

const struct osdep_service_ops osdep_service = {
	_zephyr_malloc, //rtw_vmalloc
	_zephyr_zmalloc, //rtw_zvmalloc
	_zephyr_mfree, //rtw_vmfree
	_zephyr_malloc, //rtw_malloc
	_zephyr_zmalloc, //rtw_zmalloc
	_zephyr_mfree, //rtw_mfree

	_zephyr_memcpy, //rtw_memcpy
	_zephyr_memcmp, //rtw_memcmp
	_zephyr_memset, //rtw_memset

	_zephyr_init_sema, //rtw_init_sema
	_zephyr_free_sema, //rtw_free_sema
	_zephyr_up_sema, //rtw_up_sema
	_zephyr_down_sema, //rtw_down_sema

	_zephyr_mutex_init, //rtw_mutex_init
	_zephyr_mutex_free, //rtw_mutex_free
	_zephyr_mutex_get, //rtw_mutex_get
	_zephyr_mutex_put, //rtw_mutex_put

	_zephyr_enter_critical, //rtw_enter_critical
	_zephyr_exit_critical, //rtw_exit_critical
	NULL, //rtw_enter_critical_bh
	NULL, //rtw_exit_critical_bh

	_zephyr_enter_critical_mutex, //rtw_enter_critical_mutex
	_zephyr_exit_critical_mutex, //rtw_exit_critical_mutex

	_zephyr_spinlock_init, //rtw_spinlock_init
	_zephyr_spinlock_free, //rtw_spinlock_free
	_zephyr_spinlock, //rtw_spin_lock
	_zephyr_spinunlock, //rtw_spin_unlock
	_zephyr_spinlock_irqsave, //rtw_spinlock_irqsave
	_zephyr_spinunlock_irqsave, //rtw_spinunlock_irqsave

	_zephyr_init_xqueue, //rtw_init_xqueue
	_zephyr_push_to_xqueue, //rtw_push_to_xqueue
	_zephyr_pop_from_xqueue, //rtw_pop_from_xqueue
	_zephyr_deinit_xqueue, //rtw_deinit_xqueue

	_zephyr_get_current_time, //rtw_get_current_time
	_zephyr_systime_to_ms, //rtw_systime_to_ms
	_zephyr_systime_to_sec, //rtw_systime_to_sec
	_zephyr_ms_to_systime, //rtw_ms_to_systime
	_zephyr_sec_to_systime, //rtw_sec_to_systime

	_zephyr_msleep_os, //rtw_msleep_os
	_zephyr_usleep_os, //rtw_usleep_os
	_zephyr_mdelay_os, //rtw_mdelay_os
	_zephyr_udelay_os, //rtw_udelay_os
	_zephyr_yield_os, //rtw_yield_os

	_zephyr_init_timer, //rtw_init_timer
	_zephyr_set_timer, //rtw_set_timer
	_zephyr_cancel_timer_ex, //rtw_cancel_timer
	_zephyr_del_timer, //rtw_del_timer

	_zephyr_ATOMIC_SET, //ATOMIC_SET
	_zephyr_ATOMIC_READ, //ATOMIC_READ
	_zephyr_ATOMIC_ADD, //ATOMIC_ADD
	_zephyr_ATOMIC_SUB, //ATOMIC_SUB
	_zephyr_ATOMIC_INC, //ATOMIC_INC
	_zephyr_ATOMIC_DEC, //ATOMIC_DEC
	_zephyr_ATOMIC_ADD_RETURN, //ATOMIC_ADD_RETURN
	_zephyr_ATOMIC_SUB_RETURN, //ATOMIC_SUB_RETURN
	_zephyr_ATOMIC_INC_RETURN, //ATOMIC_INC_RETURN
	_zephyr_ATOMIC_DEC_RETURN, //ATOMIC_DEC_RETURN
	
	_zephyr_modular64, //rtw_modular64
	_zephyr_get_random_bytes, //rtw_get_random_bytes
	_zephyr_GetFreeHeapSize, //rtw_getFreeHeapSize
	
	_zephyr_create_task, //rtw_create_task
	_zephyr_delete_task, //rtw_delete_task
	_zephyr_wakeup_task, //rtw_wakeup_task

	_zephyr_thread_enter, //rtw_thread_enter
	_zephyr_thread_exit, //rtw_thread_exit

	_zephyr_timerCreate, //rtw_timerCreate,
	_zephyr_timerDelete, //rtw_timerDelete,
	_zephyr_timerIsTimerActive, //rtw_timerIsTimerActive,
	_zephyr_timerStop, //rtw_timerStop,
	_zephyr_timerChangePeriod //rtw_timerChangePeriod
};

