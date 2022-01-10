
#ifndef _FFT_H_
#define _FFT_H_

#include "att_pattern_header.h"

#define 	STAGE_BFLY 					10
#define 	FFT_POINTS 					(1 << STAGE_BFLY)

//fft result
typedef struct
{
	u64_t signal_pow;
	u64_t noise_pow;
	u16_t snr; /*return 0 means < 10dB; return 10~80 means >10~80dB, but <20~90dB, return 90 means >90dB*/
	u16_t freq_main;
	s32_t  ampl_max;
	s32_t  ampl_min;
} FFT_RESULT;

extern FFT_RESULT fft_process(s32_t *pBuf, u32_t sr, s32_t input_freq);

#endif
