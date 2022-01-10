/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2019-2-18-3:20:35             1.0             build this file
 ********************************************************************************/
/*!
 * \file     temperature_compensation.c
 * \brief
 * \author
 * \version  1.0
 * \date  2019-2-18-3:20:35
 *******************************************************************************/
#include <kernel.h>
#include <init.h>
#include <thread_timer.h>
#include <soc.h>
#include "temp_comp.h"
#include <ext_types.h>
#include <soc_atp.h>
#include <compensation.h>
#include <string.h>

#define SYS_LOG_DOMAIN "comp"

#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#endif
#include <logging/sys_log.h>

static cap_temp_comp_ctrl_t  cap_temp_comp_ctrl;

// <"temperature compensation", CFG_CATEGORY_BLUETOOTH>
static const CFG_Struct_Cap_Temp_Comp CFG_Cap_Temp_Comp_Tbl =
{
    // <"enable temperature compensation", CFG_TYPE_BOOL>
    .Enable_Cap_Temp_Comp = true,

    // <"temperature compensation", CFG_Type_Cap_Temp_Comp>
    .Table =
    {
        { CAP_TEMP_N_20, 0.0f * 10 },
        { CAP_TEMP_0,    0.0f * 10 },
        { CAP_TEMP_P_20, 0.0f * 10 },
        { CAP_TEMP_P_25, 0.0f * 10 },
        { CAP_TEMP_P_40, 0.0f * 10 },
        { CAP_TEMP_P_60, 0.0f * 10 },
        { CAP_TEMP_P_75, 0.0f * 10 },

        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
    },
};

/*temperature compensation process
 */
static void cap_temp_comp_proc(void)
{
    cap_temp_comp_ctrl_t*  p = &cap_temp_comp_ctrl;

    int  comp = 0;
    int  temp;

    temp = sys_pm_temp_adc_get_temperature();

    temp += cap_temp_comp_ctrl.delta;

    if (ABS(temp - p->last_temp) <= 2)
        return;

    p->last_temp = temp;

    if (temp <= (s8_t)p->configs.Table[0].Cap_Temp)
    {
        comp = (s8_t)p->configs.Table[0].Cap_Comp;
    }
    else if (temp >= (s8_t)p->configs.Table[p->table_size - 1].Cap_Temp)
    {
        comp = (s8_t)p->configs.Table[p->table_size - 1].Cap_Comp;
    }
    else
    {
        int  i;

        for (i = 0; i < p->table_size - 1; i++)
        {
            CFG_Type_Cap_Temp_Comp*  t1 = &p->configs.Table[i];
            CFG_Type_Cap_Temp_Comp*  t2 = &p->configs.Table[i + 1];

            if (temp >= (s8_t)t1->Cap_Temp &&
                temp <  (s8_t)t2->Cap_Temp)
            {
                comp = (temp - (s8_t)t1->Cap_Temp) *
                    ((s8_t)t2->Cap_Comp - (s8_t)t1->Cap_Comp) /
                    ((s8_t)t2->Cap_Temp - (s8_t)t1->Cap_Temp) +
                    (s8_t)t1->Cap_Comp;

                break;
            }
        }
    }

    SYS_LOG_INF("%d, %d.%d", temp, comp / 10, ABS(comp) % 10);

    soc_set_hosc_cap((int)p->normal_cap + comp);
}


//static int cap_temp_comp_cmp(CFG_Type_Cap_Temp_Comp* t1, CFG_Type_Cap_Temp_Comp* t2)
//{
//    return ((s8_t)t1->Cap_Temp - (s8_t)t2->Cap_Temp);
//}


static void cap_temp_comp_timer_handler(struct thread_timer *timer, void* pdata)
{
    cap_temp_comp_ctrl_t*  p = &cap_temp_comp_ctrl;

    if (pdata == NULL)
    {
        sys_pm_temp_adc_ctrl_enable(true);

        p->timer.expiry_fn_arg = (void*)1;

        p->timer.period = 20;

        thread_timer_restart(&p->timer);
    }
    else
    {
        cap_temp_comp_proc();

        sys_pm_temp_adc_ctrl_enable(false);

        p->timer.expiry_fn_arg = NULL;

        p->timer.period = CAP_TEMP_COMP_CHECK_SEC * 1000;

        thread_timer_restart(&p->timer);
    }
}

/*!
 * \brief temperature compensation
 */
static void sys_cap_temp_comp(CFG_Struct_Cap_Temp_Comp* configs)
{
    cap_temp_comp_ctrl_t*  p = &cap_temp_comp_ctrl;

    int  i;

    p->configs = *configs;

    //qsort(p->configs.Table, CFG_MAX_CAP_TEMP_COMP,
    //    sizeof(CFG_Type_Cap_Temp_Comp), (void*)cap_temp_comp_cmp);

    p->table_size = 0;

    for (i = 0; i < CFG_MAX_CAP_TEMP_COMP; i++)
    {
        if (p->configs.Table[i].Cap_Temp == CAP_TEMP_NA)
            break;

        p->table_size += 1;
    }

	/*
	 * setting the capacitor as the normal value?
	 * compensate the temperature for the capacitor normal value ?
	 */
    p->normal_cap = soc_get_hosc_cap();

    p->last_temp = CAP_TEMP_NA;

    if (p->table_size == 0)
        return;

    if (!p->configs.Enable_Cap_Temp_Comp)
        return;

    p->enabled = true;

    sys_pm_temp_adc_ctrl_enable(true);

    cap_temp_comp_start();
}


/* get the temprature compensation is enabled
 */
uint32_t cap_temp_comp_enabled(void)
{
    cap_temp_comp_ctrl_t*  p = &cap_temp_comp_ctrl;

    return p->enabled;
}

/* timer start for temperature compensation
 */
void cap_temp_comp_start(void)
{
    cap_temp_comp_ctrl_t*  p = &cap_temp_comp_ctrl;

    thread_timer_init(&p->timer, cap_temp_comp_timer_handler, NULL);

    thread_timer_start(&p->timer, 0, CAP_TEMP_COMP_CHECK_SEC * 1000);
}


/* timer stop for temperature compensation
 */
void cap_temp_comp_stop(void)
{
    cap_temp_comp_ctrl_t*  p = &cap_temp_comp_ctrl;

    sys_pm_temp_adc_ctrl_enable(false);

    thread_timer_stop(&p->timer);
}

static int cap_temp_comp_init(struct device *arg)
{
    int delta;

    int err;

    CFG_Struct_Cap_Temp_Comp  configs;

    /* temperature ajust bias
     */
    err = soc_atp_read_bits(&delta, 64, 4);

    if(err){
        delta = 0;
    }

    if (delta & 0x10)
        delta = (delta & 0x0f);
    else
        delta = -(delta & 0x0f);

    cap_temp_comp_ctrl.delta = delta;

    memcpy(&configs, &CFG_Cap_Temp_Comp_Tbl, sizeof(CFG_Struct_Cap_Temp_Comp));

    sys_cap_temp_comp(&configs);

    return 0;
}

SYS_INIT(cap_temp_comp_init, POST_KERNEL, CONFIG_TEMP_COMPENSATION_INIT_PRIORITY_DEVICE);

