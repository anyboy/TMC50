/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_audio.c
*/

#include "ap_autotest_audio.h"

struct att_env_var *self;

return_result_t *return_data;
u16_t trans_bytes;

aux_mic_channel_test_arg_t g_linein_channel_test_arg;
aux_mic_channel_test_arg_t g_mic_channel_test_arg;
fm_rx_test_arg_t g_fm_rx_test_arg;
audio_channel_result_t g_audio_channel_result;

static int act_test_audio_arg(void)
{
    int ret_val = 0;
	u16_t *line_buffer;
	u16_t *start;
	u16_t *end;
	u8_t arg_num;

    ret_val = att_write_data(STUB_CMD_ATT_GET_TEST_ARG, 0, self->rw_temp_buffer);

    if (ret_val == 0) {
        ret_val = att_read_data(STUB_CMD_ATT_GET_TEST_ARG, self->arg_len, self->rw_temp_buffer);

        if (ret_val == 0) {
            u8_t temp_data[8];
            //do not use STUB_ATT_RW_TEMP_BUFFER, which will be used to parse parameters
            ret_val = att_write_data(STUB_CMD_ATT_ACK, 0, temp_data);
        }
    }

	if (ret_val == 0) {
        struct channel_arg *left_arg, *right_arg;
		line_buffer = (u16_t *)(self->rw_temp_buffer + sizeof(stub_ext_cmd_t));
		
		if (self->test_id == TESTID_LINEIN_CH_TEST) {
		    arg_num = 1;
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_linein_channel_test_arg.dac_play_flag = unicode_to_int(start, end, 10);
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_linein_channel_test_arg.dac_play_volume_db = unicode_to_int(start, end, 10);

			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_linein_channel_test_arg.input_channel = unicode_to_int(start, end, 10);

            left_arg = &g_linein_channel_test_arg.left;
            right_arg = &g_linein_channel_test_arg.right;

            SYS_LOG_INF("read linein channel test arg : %d, %d, %d\n", 
					g_linein_channel_test_arg.dac_play_flag,
					g_linein_channel_test_arg.dac_play_volume_db,
					g_linein_channel_test_arg.input_channel);
		}
        else if (self->test_id == TESTID_MIC_CH_TEST) {
		    arg_num = 1;
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_mic_channel_test_arg.dac_play_flag = unicode_to_int(start, end, 10);
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_mic_channel_test_arg.dac_play_volume_db = unicode_to_int(start, end, 10);
			
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_mic_channel_test_arg.input_channel = unicode_to_int(start, end, 10);

            left_arg = &g_mic_channel_test_arg.left;
            right_arg = &g_mic_channel_test_arg.right;

            SYS_LOG_INF("read mic channel test arg : %d, %d, %d\n", 
					g_mic_channel_test_arg.dac_play_flag,
					g_mic_channel_test_arg.dac_play_volume_db,
					g_mic_channel_test_arg.input_channel);
		}
        else /*TESTID_FM_CH_TEST*/ {
		    arg_num = 1;
			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_fm_rx_test_arg.fm_rx_channel = unicode_to_int(start, end, 10);

			act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    g_fm_rx_test_arg.fm_rx_aux_in_channel = unicode_to_int(start, end, 10);

            left_arg = &g_fm_rx_test_arg.left;
            right_arg = &g_fm_rx_test_arg.right;

            SYS_LOG_INF("read fm rx test arg : %d, %d\n",
					g_fm_rx_test_arg.fm_rx_channel, 
					g_fm_rx_test_arg.fm_rx_aux_in_channel);
		}
        
        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        left_arg->test_ch = unicode_to_int(start, end, 10);
        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        left_arg->ch_power_th = unicode_to_int(start, end, 10) * 10;
        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        left_arg->test_ch_spectrum = unicode_to_int(start, end, 10);
        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        left_arg->ch_spectrum_snr_th = unicode_to_int(start, end, 10);

        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        right_arg->test_ch = unicode_to_int(start, end, 10);
        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        right_arg->ch_power_th = unicode_to_int(start, end, 10) * 10;
        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        right_arg->test_ch_spectrum = unicode_to_int(start, end, 10);
        act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
        right_arg->ch_spectrum_snr_th = unicode_to_int(start, end, 10);

        SYS_LOG_INF("left channel arg : %d, %d, %d, %d\n", 
					left_arg->test_ch,
					left_arg->ch_power_th,
					left_arg->test_ch_spectrum,
					left_arg->ch_spectrum_snr_th);
        SYS_LOG_INF("right channel arg : %d, %d, %d, %d\n", 
					right_arg->test_ch,
					right_arg->ch_power_th,
					right_arg->test_ch_spectrum,
					right_arg->ch_spectrum_snr_th);
	}

    return ret_val;
}

static bool act_test_report_audio_result(bool ret_val)
{
    return_data->test_id = self->test_id;

    return_data->test_result = ret_val;

    int32_to_unicode(g_audio_channel_result.left_ch_result, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(g_audio_channel_result.left_ch_power, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(g_audio_channel_result.left_ch_spectrum_snr, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
    return_data->return_arg[trans_bytes++] = 0x002c;

    int32_to_unicode(g_audio_channel_result.right_ch_result, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(g_audio_channel_result.right_ch_power, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
    return_data->return_arg[trans_bytes++] = 0x002c;
    int32_to_unicode(g_audio_channel_result.right_ch_spectrum_snr, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);

    //add terminator
    return_data->return_arg[trans_bytes++] = 0x0000;

    //four-byte alignment
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;

    return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

#define AUDIO_ADC_BUFFER_LEN  0x1000
short *adc_in_buffer, *adc_in_data, *adc_in_data_left, *adc_in_data_right;
int adc_in_recv_count = 0;
u8_t adc_in_data_flag;
bool stereo_flag = true;

static int _audio_adc_callback(void *cb_data, u32_t reason)
{
	int i;
    
	if (stereo_flag == true) {
		if (reason == AIN_DMA_IRQ_HF){
			for (i = 0; i < (AUDIO_ADC_BUFFER_LEN/2)/(sizeof(short)*2); i++) {
				*(adc_in_data_left + i) = *(adc_in_buffer + 2*i);
				*(adc_in_data_right + i) = *(adc_in_buffer + 2*i + 1);
			}
		} else {
			for (i = 0; i < (AUDIO_ADC_BUFFER_LEN/2)/(sizeof(short)*2); i++) {
				*(adc_in_data_left + i) = *(adc_in_buffer + AUDIO_ADC_BUFFER_LEN/4 + 2*i);
				*(adc_in_data_right + i) = *(adc_in_buffer + AUDIO_ADC_BUFFER_LEN/4 + 2*i + 1);
			}
		}
	} else {
		if (reason == AIN_DMA_IRQ_HF) {
		    for (i = 0; i < (AUDIO_ADC_BUFFER_LEN/2)/(sizeof(short)); i++) {
				*(adc_in_data_left + i) = *(adc_in_buffer + i);
			}	
		} else {
			for (i = 0; i < (AUDIO_ADC_BUFFER_LEN/2)/(sizeof(short)); i++) {
				*(adc_in_data_left + i) = *(adc_in_buffer + AUDIO_ADC_BUFFER_LEN/4 + i);
			}
		}
	}
    
	adc_in_data_flag = 1;
	adc_in_recv_count++;
	return 0;
}

static bool audio_adc_play_start(u8_t self_play_flag, s8_t volume_db)
{    
	if (self_play_flag == 1)
		ap_autotest_audio_dac_start(volume_db);

	adc_in_buffer = app_mem_malloc(AUDIO_ADC_BUFFER_LEN);
	if (adc_in_buffer == NULL)
		return false;
    
	adc_in_data = app_mem_malloc(AUDIO_ADC_BUFFER_LEN/2); 
	if (adc_in_data == NULL) {
		app_mem_free(adc_in_buffer);
		return false;
	}
    
	adc_in_data_left = adc_in_data;
	if (stereo_flag == true) {
		adc_in_data_right = adc_in_data + AUDIO_ADC_BUFFER_LEN/4/sizeof(short);
	}
    
	ap_autotest_audio_adc_start(adc_in_buffer, AUDIO_ADC_BUFFER_LEN, _audio_adc_callback);

    return true;
}

static void audio_adc_play_stop(u8_t self_play_flag)
{
    ap_autotest_audio_adc_stop();
	app_mem_free(adc_in_buffer);
	app_mem_free(adc_in_data);

	if (self_play_flag == 1)
		ap_autotest_audio_dac_stop();
}

static int audio_get_fft_data(s32_t *fft_buf, int fft_offset, short *data_buf, u16_t data_len)
{
    int i;
    s16_t *p_short_value;
    
    for (i = 0; (i < data_len) && (i < FFT_POINTS-fft_offset); i++) {
        p_short_value = (s16_t *)(&fft_buf[fft_offset + i]);
        *(p_short_value + 0) = 0;
        *(p_short_value + 1) = data_buf[i];
    }
    
    fft_offset += i;
    return fft_offset;
}

static bool audio_test_proceeding(aux_mic_channel_test_arg_t *test_arg, audio_channel_result_t *test_result)
{
    bool ret_val = true;
    int fft_offset_left = 0, fft_offset_right = 0;
    static s32_t fft_data_left[FFT_POINTS], fft_data_right[FFT_POINTS];
	u32_t start_playback_time;
	short left_cur_power_max, left_cur_power_rms;
	short right_cur_power_max, right_cur_power_rms;
	s16_t left_cur_power_rms_db = -600, right_cur_power_rms_db = -600;
    FFT_RESULT fft_lch, fft_rch;

	start_playback_time = os_uptime_get_32();

	while (1) {
		if (os_uptime_get_32() - start_playback_time >= 20*1000)
			break;

        if (stereo_flag == true) {
            energy_calc(adc_in_data_left, AUDIO_ADC_BUFFER_LEN/4/sizeof(short), &left_cur_power_max, &left_cur_power_rms);
            left_cur_power_rms_db = math_calc_db(left_cur_power_rms);
            SYS_LOG_INF("left energy : %x,%x ; db %d\n", left_cur_power_max, left_cur_power_rms, left_cur_power_rms_db);
                        
            energy_calc(adc_in_data_right, AUDIO_ADC_BUFFER_LEN/4/sizeof(short), &right_cur_power_max, &right_cur_power_rms);
            right_cur_power_rms_db = math_calc_db(right_cur_power_rms);
            SYS_LOG_INF("right energy : %x,%x ; db %d\n", right_cur_power_max, right_cur_power_rms, right_cur_power_rms_db);
        } else {
            energy_calc(adc_in_data_left, AUDIO_ADC_BUFFER_LEN/2/sizeof(short), &left_cur_power_max, &left_cur_power_rms);
			left_cur_power_rms_db = math_calc_db(left_cur_power_rms);
			SYS_LOG_INF("left energy : %x,%x ; db %d\n", left_cur_power_max, left_cur_power_rms, left_cur_power_rms_db);
        }
        
		if (adc_in_data_flag == 1) {
			adc_in_data_flag = 0;
			if (adc_in_recv_count >= 60) {  //discarded: The first 60 ADC samples are unstable
                
				if (stereo_flag == true) {
					fft_offset_left = audio_get_fft_data(fft_data_left, fft_offset_left, adc_in_data_left, AUDIO_ADC_BUFFER_LEN/4/sizeof(short));
					fft_offset_right = audio_get_fft_data(fft_data_right, fft_offset_right, adc_in_data_right, AUDIO_ADC_BUFFER_LEN/4/sizeof(short));
				} else {	
					fft_offset_left = audio_get_fft_data(fft_data_left, fft_offset_left, adc_in_data_left, AUDIO_ADC_BUFFER_LEN/2/sizeof(short));
				}
                
                if (fft_offset_left >= FFT_POINTS)
					break;
			}
		}
		os_sleep(1);
	}

	test_result->left_ch_result = true;
	test_result->left_ch_power = left_cur_power_rms_db;
	if (test_arg->left.test_ch) {
		if (left_cur_power_rms_db < test_arg->left.ch_power_th) {
			printk("lch power rms %d < %d *0.1dB\n", left_cur_power_rms_db, test_arg->left.ch_power_th);
			test_result->left_ch_result = false;
			ret_val = false;
		}
        #if (MY_SAMPLE_RATE == 48)
        if (test_arg->left.test_ch_spectrum == 1) {
            fft_lch = fft_process(fft_data_left, 48000, 1000);
            printk("fft : %llu, %llu, %d, %d, %x, %x\n", fft_lch.signal_pow, fft_lch.noise_pow, fft_lch.snr,
                    fft_lch.freq_main, fft_lch.ampl_max, fft_lch.ampl_min);

            if (fft_lch.freq_main != 21) {  //1KHZ
                printk("lch not 1khz sin wave\n");
                test_result->left_ch_result = false;
                ret_val = false;
            }

		    test_result->left_ch_spectrum_snr = fft_lch.snr;
		    if (fft_lch.snr < test_arg->left.ch_spectrum_snr_th) {
			    printk("lch snr < %d dB\n", test_arg->left.ch_spectrum_snr_th);
			    test_result->left_ch_result = false;
			    ret_val = false;
		    }
	    }
        #endif
	}

	test_result->right_ch_result = true;
	test_result->right_ch_power = right_cur_power_rms_db;
	if (test_arg->right.test_ch) {
		if (right_cur_power_rms_db < test_arg->right.ch_power_th) {
			test_result->right_ch_result = false;
			ret_val = false;
		}

        u8_t freq_1khz;
        if (test_arg->dac_play_flag == 0)
            freq_1khz = 1;
        else
            freq_1khz = 2;

        #if (MY_SAMPLE_RATE == 48)
		if (test_arg->right.test_ch_spectrum == 1) {
            fft_rch = fft_process(fft_data_right, 48000, freq_1khz*1000);
            printk("fft_rch: %llu, %llu, %d, %d, %x, %x\n", fft_rch.signal_pow, fft_rch.noise_pow, fft_rch.snr,
                    fft_rch.freq_main, fft_rch.ampl_max, fft_rch.ampl_min);
            if (fft_rch.freq_main !=21*freq_1khz && fft_rch.freq_main != (21*freq_1khz + freq_1khz/2)) {   //1KHz
			    printk("rch not 2khz sin wave\n");
			    test_result->right_ch_result = false;
			    ret_val = false;
		    }

		    test_result->right_ch_spectrum_snr = fft_rch.snr;
		    if (fft_rch.snr < test_arg->right.ch_spectrum_snr_th) {
			    printk("rch snr < %d dB\n", test_arg->right.ch_spectrum_snr_th);
			    test_result->right_ch_result = false;
			    ret_val = false;
		    }
	    }
        #endif
    }
    
	return ret_val;
}

static bool act_test_audio_aux_in(aux_mic_channel_test_arg_t *linein_channel_test_arg)
{
    bool ret_val = true;

    stereo_flag = true;
    
    ret_val = audio_adc_play_start(linein_channel_test_arg->dac_play_flag, linein_channel_test_arg->dac_play_volume_db);
    if (!ret_val)
        return false;
    
    ret_val = audio_test_proceeding(linein_channel_test_arg, &g_audio_channel_result);
    
    audio_adc_play_stop(linein_channel_test_arg->dac_play_flag);

	return ret_val;
}

static bool act_test_audio_mic_in(aux_mic_channel_test_arg_t *mic_channel_test_arg)
{
    bool ret_val = true;

	if (g_mic_channel_test_arg.input_channel == 2) {
		stereo_flag = true;
	} else {
		stereo_flag = false;
        mic_channel_test_arg->right.test_ch = 0;
	}

    ret_val = audio_adc_play_start(mic_channel_test_arg->dac_play_flag, mic_channel_test_arg->dac_play_volume_db);
    if (!ret_val)
        return false;
    
    ret_val = audio_test_proceeding(mic_channel_test_arg, &g_audio_channel_result);
    
    audio_adc_play_stop(mic_channel_test_arg->dac_play_flag);
    
	return ret_val;
}

static bool act_test_audio_fm_in(fm_rx_test_arg_t *fm_rx_test_arg)
{
    bool ret_val = true;



	return ret_val;
}

bool pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	self = para;
	return_data = (return_result_t *) (self->return_data_buffer);
	trans_bytes = 0;

	if (self->arg_len != 0) {
		if (act_test_audio_arg() < 0) {
			SYS_LOG_INF("read aging playback arg fail\n");
			goto main_exit;
		}
	}

	if (self->test_id == TESTID_LINEIN_CH_TEST) {
		ret_val = act_test_audio_aux_in(&g_linein_channel_test_arg);
	} else if (self->test_id == TESTID_MIC_CH_TEST) {
		ret_val = act_test_audio_mic_in(&g_mic_channel_test_arg);
	}else /*TESTID_FM_CH_TEST*/ {
		ret_val = act_test_audio_fm_in(&g_fm_rx_test_arg);
	}

main_exit:

	SYS_LOG_INF("att report result : %d\n", ret_val);

	ret_val = act_test_report_audio_result(ret_val);

    SYS_LOG_INF("att test end : %d\n", ret_val);
    
    return ret_val;
}

