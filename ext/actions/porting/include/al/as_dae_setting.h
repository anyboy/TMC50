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

#ifndef __AS_DAE_SETTING_H__
#define __AS_DAE_SETTING_H__

#define AS_DAE_EQ_NUM_BANDS  (20)
#define AS_DAE_PARA_BIN_SIZE (sizeof(((as_dae_para_info_t*)0)->aset_bin))



typedef struct{
	int sample_rate;
	//0: blocksize*4, 1: blocksize*4, 2: blocksize *2
    int dae_buf_addr[3];     //֡����buf��������������32bitλ�����ȷֱ��ǲ���������4����������
    short dae_enable;        //musicͨ��֡�����ܿ���
    short channels;
    short block_size;
    short bypass;
    short pre_soft_volume;
    short soft_volume;
    short fade_in_time_ms;
    short fade_out_time_ms;
    short fade_flag;        //1��ʾ���룬2��ʾ����
    short output_channel_config;//1:������������ 2: ���˶���Ӧ��(L=R=ԭʼ��(L/2 + R/2)), 0 ��������
                                //3:ֻ�����������4��ֻ���������
    short bit_width;
    short reserve_0;
}music_common_para_t;

typedef struct
{
    short freq_display_enable;
    short freq_point_num;
    short freq_point[10];
} freq_display_para_t;


typedef struct
{
    int sample_rate;
    int dae_buf_addr[2];     //֡����buf��������������32bitλ�����ȷֱ��ǲ���������4����������
   short dae_enable;        //micͨ��֡�����ܿ���
    short channels;
    short bypass;
    short pre_soft_volume;
    short soft_volume;
    short fade_in_time_ms;
    short fade_out_time_ms;
    short fade_flag;        //1��ʾ���룬2��ʾ����
    short bit_width;
    short output_channel_config;//1:������������ 2: ���˶���Ӧ��(L=R=ԭʼ��(L/2 + R/2)), 0 ��������
                                //3:ֻ�����������4��ֻ���������
}mic_common_para_t;

typedef struct
{
    short enable;//�Ƿ���޷�����0��ʾ���򿪣�1��ʾ��
	short limiter_version;
	short limiter_reserve0;//Ԥ��
	short limiter_reserve1;//Ԥ��
	short limiter_reserve2;//Ԥ��
    short Limiter_threshold;//[�޷�����ֵ����λΪ0.1DB����Χ -60DB ~ 0DB; 
    short Limiter_attack_time;// �޷�������ʱ�䣬��λΪ0.01ms��ȡֵ��ΧΪ 0.02 ~ 10 ms; 
    short Limiter_release_time;// �޷����ͷ�ʱ�䣬��λΪ1ms��ȡֵ��ΧΪ 1 ~ 5000 ms]
}mic_limiter_para_t;


typedef struct
{
    short echo_enable;
    short echo_delay;
    short echo_decay;
    short echo_lowpass_fc1;
    short echo_lowpass_fc2;
    short decay_times; //for word aligned
}echo_para_t;

typedef struct
{
    short reverb_enable;
    short mRoomSize;                //�����С(%):0~100                 Ĭ�ϣ�75
    short mPreDelay;                //Ԥ�ӳ�(ms):0~100ms                Ĭ�ϣ�10
    short mReverberance;            //�����(%):0~100                   Ĭ�ϣ�50
    short mHfDamping;               //��Ƶ˥��(%):0~100                 Ĭ�ϣ�50
    short mToneLow;                 //����Ƶ�ʣ�32~523                  Ĭ�ϣ�32
    short mToneHigh;                //����Ƶ�ʣ�523~8372Hz              Ĭ�ϣ�8372
    short mWetGain;                 //ʪ��(������)��������ڣ�-60~10dB��Ĭ��-1dB
    short mDryGain;                 //����(ֱ����)��������ڣ�-60~10dB��Ĭ��-1dB
    short mStereoWidth;             //�������˲��������壬ֻ֧�ֵ���������0����
    short mWetOnly;                 //1��ʾֻ��ʪ����Ĭ��Ϊ0
    short reserve_0;
}reverb_para_t;

//acoutic feedback cancellation(������������Х������)
typedef struct
{
    /**********************************/
    //��Ƶ+����Ӧ�ݲ���ȥХ��
    short freq_shift_enable;
    short nhs_enable;
    short fft_th1;                          //Х�м������1��Ĭ��16dB������1dB������Ϊ14dB
    short fft_th2;                          //Х�м������2��Ĭ��33dB������1dB������Ϊ30dB
    short attack_time;                      //DRC����ʱ�䣬��Χ����ͬlimiter����������Ϊ0.1ms����Ϊ10
    short release_time;                     //DRC�ͷ�ʱ�䣬��Χ����ͬlimiter����������Ϊ500ms
    short DRC_dB;                               //DRCѹ���ķ��ȣ���λ1dB����Ϊ36dB
    short reserve_0;
}afc_para_t;

//Ƶ����ʾ�㷨

typedef struct
{
    short pitch_shift_enable;
    short pitch_level;              //-12~12,0��ʾ�����������ʾ����������ʾ����
}pitch_shift_para_t;

typedef struct
{
    short voice_removal_enable;     //������������
    short resever_0;
}voice_removal_para_t;



typedef struct
{
    short Ducking_enable;
    short side_chain_threshold;
    unsigned short rms_detection_time;
    unsigned short attack_time;
    unsigned short release_time;
    short main_chain_target_gain;
} ducking_effect_para_out_t;



/*for DAE AL*/
typedef struct
{
    music_common_para_t music_common_para;
    freq_display_para_t freq_display_para;

	char aset_bin[512];

#if 0
	char reserve[256];//for release header
#else	
	//karaok function here 180 bytes
    mic_common_para_t mic_common_para;
    mic_limiter_para_t mic_limiter_para;
    echo_para_t echo_para;
    reverb_para_t reverb_para;
    afc_para_t afc_para;

    pitch_shift_para_t pitch_shift_para;
    voice_removal_para_t voice_removal_para;

    
    ducking_effect_para_out_t ducking_effect_para;
    
    void *virtualbass_buf_pt;//need at least 812 bytes
    /****************** OK MIC BUFFER *****************/
    unsigned int echo_buf[6]; //ÿ��Buffer 4K�����24KB
    unsigned int freq_shift_buf; //0x590
    unsigned int pitch_shift_buf; //0xf00
    unsigned int reverb_buf1;
    unsigned int reverb_buf2;
    unsigned int reverb_buf3;
     unsigned int nhs_buf; //0x40f0
    /***********************************/
    short mic_mix_config_l; //������ͨ���������2��channels����������������mix�������������ķ�ʽ��0��ʾ��mix��1��ʾmix��2��ʾ�滻
    short mic_mix_config_r; //������ͨ���������2��channels����������������mix�������������ķ�ʽ��0��ʾ��mix��1��ʾmix��2��ʾ�滻
	short mic_peq_enable;
    short mic_peq_band_num; //micͨ��PEQ������������������棻����ǰ����������ͨ��
#endif


}as_dae_para_info_t;

#endif
