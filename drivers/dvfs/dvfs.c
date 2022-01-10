/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dynamic voltage and frequency scaling interface
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <soc.h>
#include <dvfs.h>
#include <notify.h>

#define SYS_LOG_DOMAIN "DVFS"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <logging/sys_log.h>

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL

struct dvfs_manager {
	struct k_sem lock;
	u8_t cur_dvfs_idx;
	u8_t dvfs_level_cnt;
	u8_t asrc_limit_clk_mhz;
	u8_t spdif_limit_clk_mhz;
	struct dvfs_level *dvfs_level_tbl;

	struct notifier_block *notifier_list;
};

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)

struct dvfs_level_max {
	u16_t cpu_freq;
	u16_t dsp_freq;
	u16_t vdd_volt;
};

/* NOTE: DON'T modify max_soc_dvfs_table except actions ic designers */
#define CPU_FREQ_MAX  240
#define VDD_VOLT_MAX  1250
static const struct dvfs_level_max max_soc_dvfs_table[] = {
	/* cpu_freq,  dsp_freq, vodd_volt  */
	{162,          0,      1100},
	{192,          0,      1150},
	{216,          0,      1200},
	{CPU_FREQ_MAX, 0,      VDD_VOLT_MAX},
};

static u16_t dvfs_get_optimal_volt(u16_t cpu_freq, u16_t dsp_freq)
{
	u16_t volt;
	int i;

	volt = max_soc_dvfs_table[ARRAY_SIZE(max_soc_dvfs_table) - 1].vdd_volt;
	for (i = 0; i < ARRAY_SIZE(max_soc_dvfs_table); i++) {
		if (cpu_freq <= max_soc_dvfs_table[i].cpu_freq) {
			volt = max_soc_dvfs_table[i].vdd_volt;
			break;
		}
	}

	return volt;
}


static struct dvfs_level default_soc_dvfs_table[] = {
	/* level                       enable_cnt cpu_freq,  dsp_freq, vodd_volt  */
	{DVFS_LEVEL_S2,                0,         48,        0,      0},
	{DVFS_LEVEL_NORMAL_NO_EFX,     0,         60,        0,      0},
	{DVFS_LEVEL_NORMAL_NO_DRC,     0,         90,        0,      0},
	{DVFS_LEVEL_NORMAL,            0,        138,        0,      0},
	{DVFS_LEVEL_MIDDLE,   	       0,        162,        0,      0},
	{DVFS_LEVEL_PERFORMANCE,   	   0,        192,        0,      0},
	{DVFS_LEVEL_HIGH_PERFORMANCE,  0,        240,        0,      0},
};

#else

static struct dvfs_level default_soc_dvfs_table[] = {
	/* level                       enable_cnt cpu_freq,  dsp_freq, vodd_volt  */
	{DVFS_LEVEL_S2,                0,         60,        60,      1100},
	{DVFS_LEVEL_NORMAL,            0,         90,        90,      1300},
	{DVFS_LEVEL_MIDDLE,   	       0,        120,        120,     1300},
	{DVFS_LEVEL_PERFORMANCE,   	   0,        180,        180,     1300},
	{DVFS_LEVEL_HIGH_PERFORMANCE,  0,        240,        240,     1300},
};

#endif

struct dvfs_manager g_dvfs;

static int level_id_to_tbl_idx(int level_id)
{
	int i;

	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++) {
		if (g_dvfs.dvfs_level_tbl[i].level_id == level_id) {
			return i;
		}
	}

	return -1;
}

static int dvfs_get_max_idx(void)
{
	int i;

	for (i = (g_dvfs.dvfs_level_cnt - 1); i >= 0; i--) {
		if (g_dvfs.dvfs_level_tbl[i].enable_cnt > 0) {
			return i;
		}
	}

	return 0;
}

static void dvfs_dump_tbl(void)
{
	const struct dvfs_level *dvfs_level = &g_dvfs.dvfs_level_tbl[0];
	int i;

	SYS_LOG_INF("idx   level_id   dsp   cpu  vdd  enable_cnt");
	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++, dvfs_level++) {
		SYS_LOG_INF("%d: %d   %d   %d   %d   %d", i,
			dvfs_level->level_id,
			dvfs_level->dsp_freq,
			dvfs_level->cpu_freq,
			dvfs_level->vdd_volt,
			dvfs_level->enable_cnt);
	}
}

static void dvfs_corepll_changed_notify(int state, int old_corepll_freq, int new_corepll_freq)
{
	struct dvfs_freqs freqs;

	freqs.old = old_corepll_freq;
	freqs.new = new_corepll_freq;

	notifier_call_chain(&g_dvfs.notifier_list, state, &freqs);
}

int dvfs_register_notifier(struct notifier_block *nb)
{
	int ret;

	k_sem_take(&g_dvfs.lock, K_FOREVER);

	ret = notifier_chain_register(&g_dvfs.notifier_list, nb);
	if (ret < 0) {
		SYS_LOG_ERR("failed to register notify %p", nb);
	}

	k_sem_give(&g_dvfs.lock);

	return ret;
}

int dvfs_unregister_notifier(struct notifier_block *nb)
{
	int ret;

	k_sem_take(&g_dvfs.lock, K_FOREVER);

	ret = notifier_chain_unregister(&g_dvfs.notifier_list, nb);
	if (ret < 0) {
		SYS_LOG_ERR("failed to unregister notify %p", nb);
	}

	k_sem_give(&g_dvfs.lock);

	return ret;
}

static void dvfs_sync(void)
{
	struct dvfs_level *dvfs_level, *old_dvfs_level;
	unsigned int old_idx, new_idx, old_volt;
	unsigned old_dsp_freq;

	old_idx = g_dvfs.cur_dvfs_idx;

	/* get current max dvfs level */
	new_idx = dvfs_get_max_idx();
	if (new_idx == old_idx) {
		/* same level, no need sync */
		SYS_LOG_INF("max idx %d\n", new_idx);
		return;
	}

	dvfs_level = &g_dvfs.dvfs_level_tbl[new_idx];

	old_dvfs_level = &g_dvfs.dvfs_level_tbl[old_idx];
	old_volt = soc_pmu_get_vdd_voltage();
	old_dsp_freq = soc_freq_get_dsp_freq();

	SYS_LOG_INF("level_id [%d] -> [%d]", old_dvfs_level->level_id,
		dvfs_level->level_id);

	/* set vdd voltage before clock setting if new vdd is up */
	if (dvfs_level->vdd_volt > old_volt) {
		soc_pmu_set_vdd_voltage(dvfs_level->vdd_volt);
	}

	/* send notify before clock setting */
	dvfs_corepll_changed_notify(DVFS_EVENT_PRE_CHANGE,
		old_dsp_freq, dvfs_level->dsp_freq);

	SYS_LOG_INF("new dsp freq %d, cpu freq %d, vdd volt %d", dvfs_level->dsp_freq,
		dvfs_level->cpu_freq, dvfs_level->vdd_volt);

	/* adjust core/dsp/cpu clock */
	soc_freq_set_cpu_clk(dvfs_level->dsp_freq, dvfs_level->cpu_freq);

	/* send notify after clock setting */
	dvfs_corepll_changed_notify(DVFS_EVENT_POST_CHANGE,
		old_dsp_freq, dvfs_level->dsp_freq);

	/* set vdd voltage after clock setting if new vdd is down */
	if (dvfs_level->vdd_volt < old_volt) {
		soc_pmu_set_vdd_voltage(dvfs_level->vdd_volt);
	}

	g_dvfs.cur_dvfs_idx = new_idx;
}

static int dvfs_update_freq(int level_id, bool is_set, const char *user_info)
{
	struct dvfs_level *dvfs_level;
	int tbl_idx;

	SYS_LOG_INF("level %d, is_set %d %s", level_id, is_set, user_info);

	tbl_idx = level_id_to_tbl_idx(level_id);
	if (tbl_idx < 0) {
		SYS_LOG_ERR("%s: invalid level id %d\n", __func__, level_id);
		return -EINVAL;
	}

	dvfs_level = &g_dvfs.dvfs_level_tbl[tbl_idx];

	k_sem_take(&g_dvfs.lock, K_FOREVER);

	if (is_set) {
	    if(dvfs_level->enable_cnt < 255){
            dvfs_level->enable_cnt++;
	    }else{
            SYS_LOG_WRN("max dvfs level count");
	    }
	} else {
		if (dvfs_level->enable_cnt > 0) {
			dvfs_level->enable_cnt--;
		}else{
            SYS_LOG_WRN("min dvfs level count");
		}
	}

	dvfs_sync();

	k_sem_give(&g_dvfs.lock);

	return 0;
}

int dvfs_set_level(int level_id, const char *user_info)
{
	return dvfs_update_freq(level_id, 1, user_info);
}

int dvfs_unset_level(int level_id, const char *user_info)
{
	return dvfs_update_freq(level_id, 0, user_info);
}

int dvfs_get_current_level(void)
{
	int idx;

	if (!g_dvfs.dvfs_level_tbl)
		return -1;

	idx = g_dvfs.cur_dvfs_idx;
	if (idx < 0) {
		idx = 0;
	}

	return g_dvfs.dvfs_level_tbl[idx].level_id;
}

int dvfs_set_freq_table(struct dvfs_level *dvfs_level_tbl, int level_cnt)
{
	int i;

	if (!dvfs_level_tbl || level_cnt <= 0)
		return -EINVAL;

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
	for (i = 0; i < level_cnt; i++) {
		dvfs_level_tbl[i].vdd_volt = dvfs_get_optimal_volt(dvfs_level_tbl[i].cpu_freq, dvfs_level_tbl[i].dsp_freq);
	}
#endif

	g_dvfs.dvfs_level_cnt = level_cnt;
	g_dvfs.dvfs_level_tbl = dvfs_level_tbl;

	g_dvfs.cur_dvfs_idx = 0;

	dvfs_dump_tbl();

	return 0;
}

static inline u32_t soc_current_corepll_mhz(void)
{
	return (sys_read32(CMU_COREPLL_CTL) & 0x7F) * 6;
}

int soc_dvfs_set_asrc_rate(int clk_mhz)
{
    int level;
    struct dvfs_level *dvfs_cur;
	u32_t corepll_mhz = soc_current_corepll_mhz();

    g_dvfs.asrc_limit_clk_mhz = clk_mhz;

    level = g_dvfs.cur_dvfs_idx;
	if (level < 0) {
		level = 0;
	}

    dvfs_cur = &g_dvfs.dvfs_level_tbl[level];

    if(corepll_mhz >= g_dvfs.asrc_limit_clk_mhz){

        if(clk_mhz){
            soc_freq_set_asrc_freq(clk_mhz);
        }

        return 0;
    }else{
        SYS_LOG_ERR("\n\ncurrent dvfs freq too low");
        printk("corepll freq %d cpu freq %d\n", corepll_mhz, dvfs_cur->cpu_freq);
        printk("asrc request freq %d\n", \
            g_dvfs.asrc_limit_clk_mhz);

        return -EINVAL;
    }
}

int soc_dvfs_set_spdif_rate(int sample_rate)
{
    int level;
    struct dvfs_level *dvfs_cur;
	u32_t corepll_mhz = soc_current_corepll_mhz();

    g_dvfs.spdif_limit_clk_mhz = soc_freq_get_spdif_corepll_freq(sample_rate);

    level = g_dvfs.cur_dvfs_idx;
	if (level < 0) {
		level = 0;
	}

    dvfs_cur = &g_dvfs.dvfs_level_tbl[level];

    if(corepll_mhz >= g_dvfs.spdif_limit_clk_mhz){

        if(sample_rate){
            return soc_freq_set_spdif_freq(sample_rate);
        }
    }else{
        SYS_LOG_ERR("\n\ncurrent dvfs freq too low\n\n");
        printk("corepll freq %d cpu freq %d\n", corepll_mhz, dvfs_cur->cpu_freq);
        printk("spdif request freq %d\n", \
            g_dvfs.spdif_limit_clk_mhz);
    }

    return 0;
}
#endif /* CONFIG_DVFS_DYNAMIC_LEVEL */


static int dvfs_init(struct device *arg)
{
#ifdef CONFIG_SOC_VDD_VOLTAGE
	/* now k_busy_wait() is ready for adjust vdd voltage */
	soc_pmu_set_vdd_voltage(CONFIG_SOC_VDD_VOLTAGE);
#endif

	SYS_LOG_INF("default dsp freq %dMHz, cpu freq %dMHz, vdd %d mV",
		soc_freq_get_dsp_freq(), soc_freq_get_cpu_freq(),
		soc_pmu_get_vdd_voltage());

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_freq_table(default_soc_dvfs_table,
		ARRAY_SIZE(default_soc_dvfs_table));

	k_sem_init(&g_dvfs.lock, 1, 1);

	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "init");
#endif

	return 0;
}

SYS_INIT(dvfs_init, PRE_KERNEL_2, 2);
