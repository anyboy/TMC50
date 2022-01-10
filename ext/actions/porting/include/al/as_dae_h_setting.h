/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: jpl<jianpingliao@actions-semi.com>
 *
 * Change log:
 *	2019/12/16: Created by jpl.
 */

#ifndef __AS_DAE_H_SETTING_H__
#define __AS_DAE_H_SETTING_H__

#define AS_DAE_H_EQ_NUM_BANDS  (14)
#define AS_DAE_H_PARA_BIN_SIZE (sizeof(((as_dae_h_para_info_t*)0)->as_dae_h_para))

//// detail of as_dae_h_para in struct as_dae_h_para_info_t
//
// #define EQ_NUM_BANDS 14

// struct eq_band_setting
// {
// 	/* center frequency, or high-pass filter's cut-off frequency in Hz */
// 	short cutoff;
// 	/* Q value in 0.1, suggested range 3~30 */
// 	short q;
// 	/* gain in 0.1 dB, suggested range -240~120 */
// 	short gain;
// 	/* filter type. 0: disabled; 1: peak; 2: high-pass; 3: low-pass; 4: low shelf; 5: high shelf */
// 	short type;
// };
//
// typedef struct
// {
// 	short peq_enable;
// 	short peq_version;
// 	short peq_precut;// in 0.1db
// 	short peq_reserve0;
//	/* 0: EQ disabled in this band; 1: speak EQ; 2: mic EQ*/
// 	struct eq_band_setting band_settings[EQ_NUM_BANDS];
// }peq_para_t;
//
// typedef struct
// {
// 	peq_para_t peq_para;
// 	short dae_h_para_reserve[22];
// }as_dae_h_bin_para_t;//total 192 bytes

/*for DAE AL*/
typedef struct
{
	short bypass;
	short reserve0;
	/* ASQT configurable parameters */
	char as_dae_h_para[192];
} as_dae_h_para_info_t;

#endif /* __AS_DAE_H_SETTING_H__ */
