/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dynamic voltage and frequency scaling interface
 */

#ifndef	_DVFS_H_
#define	_DVFS_H_

#ifndef _ASMLANGUAGE

/*
 * dvfs levels
 */
#define DVFS_LEVEL_S2                   (10)
#define DVFS_LEVEL_NORMAL_NO_EFX	    (20)
#define DVFS_LEVEL_NORMAL_NO_DRC	    (30)
#define DVFS_LEVEL_NORMAL			    (40)
#define DVFS_LEVEL_MIDDLE			    (50)
#define DVFS_LEVEL_PERFORMANCE		    (60)
#define DVFS_LEVEL_HIGH_PERFORMANCE	    (70)

#define DVFS_EVENT_PRE_CHANGE		(1)
#define DVFS_EVENT_POST_CHANGE		(2)

struct dvfs_freqs {
	unsigned int old;
	unsigned int new;
};


enum dvfs_dev_clk_type {
    DVFS_DEV_CLK_ASRC,
    DVFS_DEV_CLK_SPDIF,
};

struct dvfs_level {
	u8_t level_id;
	u8_t enable_cnt;
	u16_t cpu_freq;
	u16_t dsp_freq;
	u16_t vdd_volt;
};

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL

int dvfs_set_freq_table(struct dvfs_level *dvfs_level_tbl, int level_cnt);
int dvfs_get_current_level(void);
int dvfs_set_level(int level_id, const char *user_info);
int dvfs_unset_level(int level_id, const char *user_info);
unsigned int soc_freq_set_asrc_freq(u32_t asrc_freq);
unsigned int soc_freq_get_spdif_corepll_freq(u32_t sample_rate);
unsigned int soc_freq_set_spdif_freq(u32_t sample_rate);
int soc_dvfs_set_asrc_rate(int clk_mhz);
int soc_dvfs_set_spdif_rate(int sample_rate);
#else

#define dvfs_set_freq_table(dvfs_level_tbl, level_cnt)		({ 0; })
#define dvfs_get_current_level()				({ 0; })
#define dvfs_set_level(level_id, user_info)			({ 0; })
#define dvfs_unset_level(level_id, user_info)			({ 0; })
#define soc_freq_set_asrc_freq(asrc_freq)	({ 0; })
#define soc_dvfs_set_asrc_rate(clk_mhz) ({ 0; })
#define soc_freq_get_spdif_corepll_freq(sample_rate)	({ 0; })
#define soc_dvfs_set_spdif_rate()				    do {} while (0)
#endif	/* CONFIG_DVFS_DYNAMIC_LEVEL */


#endif /* _ASMLANGUAGE */

#endif /* _DVFS_H_	*/
