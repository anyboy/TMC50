/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_bat.c
*/

#include "att_pattern_header.h"
#include "adc.h"
#include "board.h"
#include "byteorder.h"
#include "power_supply.h"
#include <nvram_config.h>


#define ATTEST_SAMPLE_VOL_CNT           (16)
#define ATTEST_MULT1000                 (48*100 * 1000 / 1024)
#define ATTEST_BAT_MIN_VOL              (2400)
#define ATTEST_BAT_CAP_MEASURE_MODE     (2)

#if (ATTEST_BAT_CAP_MEASURE_MODE == 2)
#define BATTERY_COMPENSATION_HIGH_VOL   (110)
#define BATTERY_COMPENSATION_MID_VOL    (150)
#define BATTERY_COMPENSATION_LOW_VOL    (175)
#endif

struct att_capctr_result {
    u8_t capacity;
    u8_t result;
};

struct att_capctr_check {
    u8_t compare_flag;      /* 0-only read cap, 1-compare */
    u8_t ctr_disable_falg;  /* 0-keep doing, 1-disable */
    u8_t minval;
    u8_t maxval;
};

struct att_batcap_reference {
	u16_t cap;
	u16_t volt;
};

struct att_bat_data {
    struct adc_seq_table seq_table;
    struct adc_seq_entry seq_entry;
    u8_t bat_buf[2];
};

struct att_env_var *self;
static struct att_capctr_result capctr_result;
static struct att_capctr_check capctr_check;
static struct battery_capctr_info capctr_info;
return_result_t *return_data;
u16_t trans_bytes;
u8_t cap_ctr_val[2];

struct att_bat_data batdata;
struct device *bat_dev;

struct att_batcap_reference att_batcap_range[] = {
    BOARD_BATTERY_CAP_MAPPINGS
};

static u16_t att_adcval_to_voltage(u16_t adcval)
{
	return ATTEST_MULT1000 * adcval / 1000;
}

static int get_nvram_bat(struct battery_capctr_info *bat_info)
{
    int ret_val;
    ret_val = property_get(CFG_ATT_BAT_INFO, (char *)bat_info, sizeof(struct battery_capctr_info));

    return ret_val;
}

static int set_nvram_bat(struct battery_capctr_info *bat_info)
{
    int ret_val;
    ret_val = property_set(CFG_ATT_BAT_INFO, (char *)bat_info, sizeof(struct battery_capctr_info));
    if (ret_val < 0)
        return ret_val;

    property_flush(CFG_ATT_BAT_INFO);

    return 0;
}

#if (ATTEST_BAT_CAP_MEASURE_MODE == 1)
static void att_bat_stop_charging(void)
{
    /* do something to disable charging */
}
#endif

static u16_t att_bat_get_voltage(void)
{
    u8_t i, j;
    u16_t adc_bat, temp_vol, min_vol, max_vol;
    u16_t vol_mv;
    u32_t sum_temp_vol, sum_vol = 0;
    
#if (ATTEST_BAT_CAP_MEASURE_MODE == 1)
    att_bat_stop_charging();
#endif

    for(i = 0; i < 4; i++) {
        k_sleep(200);

        min_vol=0xffff;
        max_vol=0;
        sum_temp_vol = 0;

        for(j = 0; j < 8; j++) {
            k_sleep(20);
            adc_read(bat_dev, &batdata.seq_table);
            adc_bat = sys_get_le16(batdata.seq_entry.buffer);
            temp_vol = att_adcval_to_voltage(adc_bat);

            if(temp_vol < min_vol)
                min_vol = temp_vol;
            else if(max_vol < temp_vol)
                max_vol = temp_vol;
            
            sum_temp_vol += temp_vol;
        }
        sum_temp_vol -= (min_vol + max_vol);
        sum_vol += ((sum_temp_vol*10 / (j -2) + 5) / 10);
    }
    
    vol_mv = (sum_vol*10 / i + 5) / 10;

#if (ATTEST_BAT_CAP_MEASURE_MODE == 2)
    //needs compensation if measure during charging
    //(for reference only): different for charge IC and batteries and capacity, 
    if(vol_mv >= 4100)
        vol_mv -= BATTERY_COMPENSATION_HIGH_VOL;
    else if(vol_mv >= 3950)
        vol_mv -= BATTERY_COMPENSATION_MID_VOL;
    else
        vol_mv -= BATTERY_COMPENSATION_LOW_VOL;
#endif

    return vol_mv;
}

static u8_t att_bat_get_capacity(u16_t volval)
{
    u8_t i, map_cnt = ARRAY_SIZE(att_batcap_range);
    u16_t cap1, vol1, cap2, vol2, capacity = 0;
    
    if(volval >= att_batcap_range[map_cnt-1].volt)
        return 100;
    else if(volval <= att_batcap_range[0].volt)
        return 0;
    
    for(i = 0; i < map_cnt; i++) {
        if(volval <= att_batcap_range[i].volt) {
            cap1 = att_batcap_range[i-1].cap;
            vol1 = att_batcap_range[i-1].volt;
            cap2 = att_batcap_range[i].cap;
            vol2 = att_batcap_range[i].volt;
            capacity = cap1 + (((cap2-cap1) * (volval-vol1)*10 + 5) / (vol2-vol1)) / 10;
            break;
        }
    }
    
    return capacity;
}

static int att_bat_init(void)
{
    bat_dev = device_get_binding(CONFIG_BATTERY_ACTS_ADC_NAME);
    if (!bat_dev) {
        SYS_LOG_INF("cannot found bat_dev\n");
        return -1;
    }

    batdata.seq_table.entries = &batdata.seq_entry;
    batdata.seq_table.num_entries = 1;
    
    batdata.seq_entry.buffer = &batdata.bat_buf[0];
    batdata.seq_entry.buffer_length = sizeof(batdata.bat_buf);
    batdata.seq_entry.sampling_delay = 0;
    batdata.seq_entry.channel_id = CONFIG_BATTERY_ACTS_ADC_CHAN;

    return 0;
}

static bool att_bat_cap_ctr_start(void)
{
    bool ret_val = false;
    
    capctr_info.capctr_enable_flag = BATTERY_CAPCTR_ENABLE;
    capctr_info.capctr_minval = cap_ctr_val[0];
    capctr_info.capctr_maxval = cap_ctr_val[1];

    if(set_nvram_bat(&capctr_info) < 0) {
        SYS_LOG_INF("set nvram fail\n");
        return false;
    }
    if(get_nvram_bat(&capctr_info) >= 0)
        if((capctr_info.capctr_enable_flag == BATTERY_CAPCTR_ENABLE)
            && (capctr_info.capctr_minval == cap_ctr_val[0])
            && (capctr_info.capctr_maxval == cap_ctr_val[1]))
            ret_val = true;

    return ret_val;
}

static bool att_bat_cap_test(void)
{
    u16_t vol;
    bool ret_val = true;

    if (get_nvram_bat(&capctr_info) < 0)
        return false;

    capctr_check.minval = cap_ctr_val[0];
    capctr_check.maxval = cap_ctr_val[1];
    
    vol = att_bat_get_voltage();
    capctr_result.capacity = att_bat_get_capacity(vol);
    if(capctr_check.compare_flag) {
        if ((capctr_result.capacity <= capctr_check.maxval) && (capctr_result.capacity >= capctr_check.minval)) {
            capctr_result.result = BATTERY_CAPCTR_STA_PASSED;
            ret_val = true;
            SYS_LOG_INF("compare passed\n");
        }
        else {
            capctr_result.result = BATTERY_CAPCTR_STA_FAILED;
            ret_val = false;
            SYS_LOG_INF("compare failed\n");
        }
    }

    if(capctr_check.ctr_disable_falg) {
        memset(&capctr_info, 0, sizeof(capctr_info));
        if (set_nvram_bat(&capctr_info) < 0)
            ret_val = false;
    }

    return ret_val;
}

static bool act_test_bat_test_dispatch(void)
{
    bool ret_val = true;
    
    if(att_bat_init() != 0) {
        SYS_LOG_INF("init error\n");
        return false;
    }

    if(self->test_id == TESTID_CAP_CTR_START)
        ret_val = att_bat_cap_ctr_start();
    else if(self->test_id == TESTID_CAP_TEST)
        ret_val = att_bat_cap_test();
    
    return ret_val;
}

static int act_test_bat_arg(void)
{
    int ret_val = 0;
    u16_t *line_buffer, *start, *end;
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
        line_buffer = (u16_t *)(self->rw_temp_buffer + sizeof(stub_ext_cmd_t));

        if (self->test_id == TESTID_CAP_CTR_START) {
            arg_num = 1;
            act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    cap_ctr_val[0] = unicode_to_int(start, end, 10);
            
            act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    cap_ctr_val[1] = unicode_to_int(start, end, 10);
        }
        else if (self->test_id == TESTID_CAP_TEST) {
            arg_num = 1;
            act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
            capctr_check.compare_flag = unicode_to_int(start, end, 10);
            
            act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
            cap_ctr_val[0] = unicode_to_int(start, end, 10);
            
            act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
		    cap_ctr_val[1] = unicode_to_int(start, end, 10);

            act_test_parse_test_arg(line_buffer, arg_num++, &start, &end);
            capctr_check.ctr_disable_falg = unicode_to_int(start, end, 10);
        }

        if ((cap_ctr_val[1] <= cap_ctr_val[0]) || (cap_ctr_val[1] > 100) || (cap_ctr_val[0] < 0)) {
            SYS_LOG_INF("invalid args\n");
            ret_val = -1;
        }
    }
    return ret_val;
}

static bool act_test_report_bat_result(bool ret_val)
{
    return_data->test_id = self->test_id;
    return_data->test_result = ret_val;

    if (self->test_id == TESTID_CAP_TEST) {
        uint32_to_unicode(capctr_result.capacity, &(return_data->return_arg[trans_bytes]), &trans_bytes, 10);
    }
    
    //add terminator
    return_data->return_arg[trans_bytes++] = 0x0000;

    //four-byte alignment
    if ((trans_bytes % 2) != 0)
        return_data->return_arg[trans_bytes++] = 0x0000;
    
    return act_test_report_result(return_data, trans_bytes * 2 + 4);
}

bool pattern_main(struct att_env_var *para)
{
    bool ret_val = false;
    self = para;
    return_data = (return_result_t *) (self->return_data_buffer);
    trans_bytes = 0;

    if (self->arg_len != 0) {
		if (act_test_bat_arg() < 0) {
			SYS_LOG_INF("read bat_ctr arg fail\n");
			goto main_exit;
		}
	}

    ret_val = act_test_bat_test_dispatch();

main_exit:
    ret_val = act_test_report_bat_result(ret_val);

    SYS_LOG_INF("att test end: %d\n", ret_val);
    return ret_val;
}
