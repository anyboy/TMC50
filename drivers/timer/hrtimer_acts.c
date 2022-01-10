/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2019-3-29-1:27:29             1.0             build this file
 ********************************************************************************/
/*!
 * \file     hrtimer.c
 * \brief
 * \author
 * \version  1.0
 * \date  2019-3-29-1:27:29
 *******************************************************************************/
#include <kernel.h>
#include <soc.h>
#include <hrtimer.h>
#include <string.h>

struct hrtimer_ctx
{
    timer_register_t *timer_register;
    sys_dlist_t hrtimer_q;
    uint32_t timer_load_val;
    uint64_t current_jiffies;
    uint64_t adjust_jiffies;
};

static struct hrtimer_ctx g_hrtimer_ctx;

//#define HRTIMER_DEBUG

#ifdef HRTIMER_DEBUG
#include <misc/printk.h>
#define TT_DEBUG(fmt, ...) TRACE("[%d]" fmt, k_uptime_get_32(), ##__VA_ARGS__)
#else
#define TT_DEBUG(fmt, ...)
#endif

static inline struct hrtimer_ctx *_get_hrtimer_ctx(void)
{
    return &g_hrtimer_ctx;
}

static inline struct hrtimer *_find_first_hrtimer_to_unpend(void)
{
    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

	return (struct hrtimer *)sys_dlist_peek_head(&ctx->hrtimer_q);
}

static inline struct hrtimer *_find_next_hrtimer(struct hrtimer *ttimer)
{
    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

    return (struct hrtimer *)sys_dlist_peek_next(&ctx->hrtimer_q, (void *)ttimer);
}

static inline unsigned int _hrtimer_us_to_tick(uint32_t m)
{
    return (m * CONFIG_HOSC_CLK_MHZ);
}

static void _hrtimer_reload(timer_register_t *timer_register, uint32_t value)
{
    if (value > MAX_TIMER_COUNT) {
        value = MAX_TIMER_COUNT;
    }

    // timer1 is used to timer, not reload
    timer_register->ctl = 0;

    //clear pending
    timer_register->ctl |= (1 << T0_CTL_ZIPD);

	while (timer_register->ctl != 0);

    timer_register->val = value;

	while (timer_register->val != value);

    // start timer
    timer_register->ctl = 0x22;
}

static uint64_t _get_current_jiffies(void)
{
    int val;
    uint32_t flags;

    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

    flags = irq_lock();

    /*hw timer bug, we should check*/
    val = ctx->timer_load_val - timer_count_read(ctx->timer_register);

    val = val > 0 ? val : 0;
    if (ctx->adjust_jiffies < ctx->current_jiffies + val) {
        ctx->adjust_jiffies = ctx->current_jiffies + val;
    }

    irq_unlock(flags);

    return ctx->adjust_jiffies;
}

static void _update_expires_to_timer(uint64_t expires)
{
    int32_t val;

    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

    if (expires <= ctx->current_jiffies || expires > (ctx->current_jiffies + MAX_TIMER_COUNT)) return;

    val = ctx->timer_load_val - timer_count_read(ctx->timer_register);

    val = val > 0 ? val : 0;

    if (expires > ctx->current_jiffies + val) {
        val = (uint32_t) (expires - ctx->current_jiffies - val);
    } else {
        val = 1;
    }

    ctx->timer_load_val = expires - ctx->current_jiffies;

    _hrtimer_reload(ctx->timer_register, val);
}

static void _hrtimer_remove(struct hrtimer *timer)
{
    u32_t irq_flags;

	struct hrtimer *in_q, *next;

    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

    irq_flags = irq_lock();

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&ctx->hrtimer_q, in_q, next, node) {
		if (timer == in_q) {
			sys_dlist_remove(&in_q->node);
			irq_unlock(irq_flags);
			return;
		}
	}

	irq_unlock(irq_flags);
}

static void _hrtimer_insert(struct hrtimer *ttimer,
				 u32_t expiry_time)
{
    u32_t irq_flags;

    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

	sys_dlist_t *thread_timer_q = &ctx->hrtimer_q;
	struct hrtimer *in_q;

    irq_flags = irq_lock();

	SYS_DLIST_FOR_EACH_CONTAINER(thread_timer_q, in_q, node) {
		if (ttimer->expiry_time < in_q->expiry_time) {
			sys_dlist_insert_before(thread_timer_q, &in_q->node,
						&ttimer->node);
			goto check_queue;
		}
	}

	sys_dlist_append(thread_timer_q, &ttimer->node);

check_queue:
    if(ttimer == _find_first_hrtimer_to_unpend()){
        _update_expires_to_timer(ttimer->expiry_time);
    }

	irq_unlock(irq_flags);
}

void hrtimer_init(struct hrtimer *ttimer, hrtimer_expiry_t expiry_fn,
		       void *expiry_fn_arg)
{
	__ASSERT(ttimer != NULL, "");

	TT_DEBUG("timer %p: init func %p arg 0x%p\n", ttimer,
		ttimer->expiry_fn, ttimer->expiry_fn_arg);

	/* remove hrtimer if already submited */
	_hrtimer_remove(ttimer);

	memset(ttimer, 0, sizeof(struct hrtimer));
	ttimer->expiry_time = ULLONG_MAX;
	ttimer->expiry_fn = expiry_fn;
	ttimer->expiry_fn_arg = expiry_fn_arg;
	sys_dlist_init(&ttimer->node);
}


void hrtimer_start(struct hrtimer *ttimer, s32_t duration, s32_t period)
{
	__ASSERT((ttimer != NULL) && (ttimer->expiry_fn != NULL), "");

	_hrtimer_remove(ttimer);

	ttimer->expiry_time = _get_current_jiffies() + _hrtimer_us_to_tick(duration);
	ttimer->period = period;
	ttimer->duration = duration;

	TT_DEBUG("timer %p: start duration %d period %d\n",
		ttimer, duration, period);

	_hrtimer_insert(ttimer, ttimer->expiry_time);
}

void hrtimer_stop(struct hrtimer *ttimer)
{
    u32_t flags;

    struct hrtimer *next;

	__ASSERT(ttimer != NULL, "");

	TT_DEBUG("timer %p: stop\n", ttimer);

	flags = irq_lock();

	/* check if the current timer is the mininum one and then update the current timer  */
	if(ttimer == _find_first_hrtimer_to_unpend()){
		next = _find_next_hrtimer(ttimer);
		if(next){
            _update_expires_to_timer(next->expiry_time);
		}
	}

	_hrtimer_remove(ttimer);

	irq_unlock(flags);
}

bool hrtimer_is_running(struct hrtimer *ttimer)
{
	struct hrtimer *in_q;
	bool result = false;
	u32_t flags;

    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

	__ASSERT(ttimer != NULL, "");

	flags = irq_lock();

	SYS_DLIST_FOR_EACH_CONTAINER(&ctx->hrtimer_q, in_q, node) {
		if (ttimer == in_q) {
			result = true;
			break;
		}
	}
	irq_unlock(flags);
	return result;
}

sys_dlist_t call_and_del;
struct hrtimer *cur;

void hrtimer_handle_expired(void)
{
    struct hrtimer_ctx *ctx;
    struct hrtimer *tmpNode;
    //int key = 0;
#ifdef CONFIG_BUSY_WAIT_USES_ALTERNATE_CLOCK
    u32_t time_jiffies_32;
    u32_t timer_cnt;
#endif
    SYS_IRQ_FLAGS flags;

    ctx = _get_hrtimer_ctx();

	//key = irq_lock();
	sys_irq_lock(&flags);
    sys_dlist_init(&call_and_del);

#ifdef CONFIG_BUSY_WAIT_USES_ALTERNATE_CLOCK

	/*
	 * The time of executing reload T0 and other list operations etc. are not calculated by the timer and lead to the bias is big.
	 * One way to repair this is to use T1 to compensate hrtimer.
	 * Take bluetooth clock as standard, 360ms hrtimer bias will reduce the bias from 4ms to 0.4ms.
	 */

    timer_cnt = 0xfffffffful  - sys_read32(T1_CNT);

    time_jiffies_32 = ctx->current_jiffies;

    if(time_jiffies_32 > timer_cnt)
    {
        ctx->current_jiffies = ctx->current_jiffies + 0x100000000;
    }

    ctx->current_jiffies = (ctx->current_jiffies & (~0xffffffffull)) + timer_cnt;
#else
    ctx->current_jiffies += ctx->timer_load_val;
#endif

    SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&ctx->hrtimer_q, cur, tmpNode, node){
        if (cur->expiry_time > ctx->current_jiffies) {
            break;
        }

        sys_dlist_remove(&cur->node);
        sys_dlist_append(&call_and_del, &cur->node);
    }

	// amend timer load value to be the latest timer node value
    if (sys_dlist_is_empty(&(ctx->hrtimer_q))){
        ctx->timer_load_val = MAX_TIMER_COUNT;
    } else {
        if(cur && (cur->expiry_time <= (ctx->current_jiffies + MAX_TIMER_COUNT))) {
            ctx->timer_load_val = cur->expiry_time - ctx->current_jiffies;
        }else{
            ctx->timer_load_val = MAX_TIMER_COUNT;
        }
    }

    _hrtimer_reload(ctx->timer_register, ctx->timer_load_val);

    SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&call_and_del, cur, tmpNode, node){
		// timer is expired and start to excute timer function
        sys_dlist_remove(&cur->node);

		// check if the timer is period or not
        if (cur->period != 0) {
			// if is period timer, add the timer into the queue again
			hrtimer_start(cur, cur->period, cur->period);
        }

        TT_DEBUG("timer %p: call %p\n", cur, cur->expiry_fn);
        //irq_unlock(key);
        sys_irq_unlock(&flags);
        /* invoke thread timer expiry function */
        cur->expiry_fn(cur, cur->expiry_fn_arg);
        //key = irq_lock();
        sys_irq_lock(&flags);
    }
    //irq_unlock(key);
    sys_irq_unlock(&flags);
}

void hrtimer_runtime_init(void)
{
    struct hrtimer_ctx *ctx = _get_hrtimer_ctx();

    ctx->timer_register = (timer_register_t *)T0_CTL;
	ctx->timer_load_val = MAX_TIMER_COUNT;

	sys_dlist_init(&ctx->hrtimer_q);

	_hrtimer_reload(ctx->timer_register, ctx->timer_load_val);

    IRQ_CONNECT(IRQ_ID_TIMER0, CONFIG_IRQ_PRIO_NORMAL, hrtimer_handle_expired, 0, 0);

    irq_enable(IRQ_ID_TIMER0);
}



