/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef	_ACTIONS_SOC_ATP_H_
#define	_ACTIONS_SOC_ATP_H_

#ifndef _ASMLANGUAGE

extern unsigned int soc_get_system_info(unsigned int module);
extern int soc_atp_get_hosc_calib(int id, unsigned char *calib_val);
extern int soc_atp_get_rf_calib(int id, unsigned int *calib_val);
extern int soc_atp_get_pmu_calib(int id, int *calib_val);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_ATP_H_	*/
