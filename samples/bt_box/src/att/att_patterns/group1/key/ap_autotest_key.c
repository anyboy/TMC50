/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_key.c
*/

#include "att_pattern_header.h"
#include "adc.h"
#include "device.h"
#include "byteorder.h"
#include "board.h"
#include "input_dev.h"

#define ATTEST_KEY_WAIT_TIME        (5000)
#define ATTEST_ONOFF_KEYNAME        ("ONOFF")
#define PMU_ONOFF_KEY_ONOFF_PRESSED (1 << 0)

#define ATTEST_LINEIN           (0)
#define ATTEST_ADCKEY_NUM       (7)
#define ATTEST_ERROR_PERC       (6)

#define ATT_NO_ADCKEY_VALUE     (0x300)
#define ATT_LINEIN_RESIS        (7500)      /* unit 100 Ohm */
#define ATT_POLL_RESIS          (1000)
#define ATT_REF_VOL             (36)        /* unit 100 mV */
#define ATT_SVCC_VOL            (31)


struct att_keyvalue_reference {
    u16_t keycode;
    u16_t minvalue;
    u16_t maxvalue;
    char *name;
};

struct att_key_data {
    struct adc_seq_table seq_table;
    struct adc_seq_entry seq_entry;
    u8_t key_buf[4];
};

struct att_key {
    u16_t keycode;
    u16_t keyvalue;
    char *keyname;
    bool isvalid;
};

u16_t trans_bytes;
return_result_t *return_data;
struct att_env_var *self;
struct att_key_data keydata;
struct device *adc_dev;

struct att_key onoffkey;
struct att_key adckey[ATTEST_ADCKEY_NUM];
struct att_keyvalue_reference att_key_value_range[ATTEST_ADCKEY_NUM];

u16_t key_code[] = {KEY_ADFU, KEY_MENU, KEY_TBD, KEY_PREVIOUSSONG, KEY_NEXTSONG, KEY_VOLUMEDOWN, KEY_VOLUMEUP};
char *key_name[] = {"ADFU",   "MENU",   "TBD",   "PREV",           "NEXT",       "VOL-",         "VOL+"};
u16_t resistance[]={0,        36,       120,     390,              680,          1200,           2700};    /* unit 100 Ohm */

static u16_t att_key_get_ref_value(u16_t resis, s8_t err_perc)
{
    u32_t poll_resis, key_resis;
    u16_t multiple, ref_value;
#if ATTEST_LINEIN
    u32_t linein_resis;
#endif

    for(multiple = 100; multiple >= 10; multiple /= 10)
        if ((resis % multiple == 0) && (ATT_LINEIN_RESIS % multiple == 0) && (ATT_POLL_RESIS % multiple == 0))
            break;
    
    key_resis = resis / multiple;
    poll_resis = ATT_POLL_RESIS / multiple;
    
    key_resis  *= (100 + err_perc);
    poll_resis *= (100 - err_perc);
    
#if ATTEST_LINEIN
    linein_resis = ATT_LINEIN_RESIS / multiple;
    linein_resis *= (100 + err_perc);

    if(multiple <= 10) {
        poll_resis /= 10;
        key_resis /= 10;
        linein_resis /= 10;
    }
    key_resis = ((linein_resis * key_resis) * 10 / (linein_resis + key_resis) + 5) / 10;
    
#endif

    ref_value = (key_resis * 1000 / (key_resis + poll_resis) * ATT_SVCC_VOL / ATT_REF_VOL * 0x3FF + 5) / 1000;
	return ref_value;
}

static u32_t value_abs(int value)
{
    return (value > 0 ? value : -value);
}

static inline u16_t read_adckey_value(void)
{
    adc_read(adc_dev, &keydata.seq_table);
    return sys_get_le16(keydata.seq_entry.buffer);
}

static void att_key_value_range_init(void)
{
	int i;
	for(i = 0; i < ATTEST_ADCKEY_NUM; i++) {
        att_key_value_range[i].keycode = key_code[i];
		att_key_value_range[i].minvalue = att_key_get_ref_value(resistance[i], -ATTEST_ERROR_PERC);
		att_key_value_range[i].maxvalue = att_key_get_ref_value(resistance[i], ATTEST_ERROR_PERC);
        att_key_value_range[i].name = key_name[i];

        //for key ADFU
        if(att_key_value_range[i].minvalue == att_key_value_range[i].maxvalue)
            att_key_value_range[i].maxvalue += 7;
	}
}

static int att_adcvalue_isvalid(u16_t value)
{
    int i;
    
    for(i = 0; i < ATTEST_ADCKEY_NUM; i++)
        if((value >= att_key_value_range[i].minvalue)
            && (value <= att_key_value_range[i].maxvalue))
            return i;
    
    return -1;
}

static void att_adcvalue_verify(struct att_key *key, u16_t keyval)
{
    int i;
    
    key->keyvalue = keyval;
    i = att_adcvalue_isvalid(key->keyvalue);
    if(i < 0) {
        key->isvalid = false;
        key->keycode = KEY_RESERVED;
        key->keyname = "NULL";
        printk("invalid_keyvalue: 0x%x\n", key->keyvalue);
        return;
    }

    key->isvalid = true;
    key->keycode = att_key_value_range[i].keycode;
    key->keyname = att_key_value_range[i].name;
    printk("attkey_name: %s\n", key->keyname);
}

static void onoff_key_test_proceeding(u16_t no_press_value)
{
    u16_t value;
    value = !!(sys_read32(PMU_ONOFF_KEY) & PMU_ONOFF_KEY_ONOFF_PRESSED);
    if(value != no_press_value) {
        onoffkey.keyname = ATTEST_ONOFF_KEYNAME;
        onoffkey.keyvalue = value;
        onoffkey.isvalid = true;
        printk("attkey_name:%s\n", onoffkey.keyname);
    }
}

static u8_t adckey_test_proceeding(void)
{
    u32_t start_time;
    u16_t no_onoff_value, press_adckey_value;
    u8_t i, key_count = 0;
    bool repeat_flag, first_press = true;

    // the value without key pressed
    no_onoff_value = !!(sys_read32(PMU_ONOFF_KEY) & PMU_ONOFF_KEY_ONOFF_PRESSED);
    printk("please press key\n");
    
    start_time = k_uptime_get_32();
    while(1) {
        k_sleep(40);

        if(!onoffkey.isvalid) {
            onoff_key_test_proceeding(no_onoff_value);
			if (onoffkey.isvalid)
				start_time = k_uptime_get_32();
        }
        
        if (key_count < ATTEST_ADCKEY_NUM) {
            press_adckey_value = read_adckey_value();
            if (press_adckey_value < ATT_NO_ADCKEY_VALUE) {
                //debouncing by discarding the first value
                if(first_press) {
                    first_press = false;
                    continue;
                }

                //repeated key or wait for release
                repeat_flag = false;
                for (i = 0; i < key_count; i++) {
                    if (value_abs(press_adckey_value - adckey[i].keyvalue) < 10) {
                        repeat_flag = true;
                        first_press = true;
                        break;
                    }
                }
                
                if(!repeat_flag) {
                    att_adcvalue_verify(&adckey[key_count], press_adckey_value);
                
                    //clear flags for next key press
                    first_press = true;
                    key_count++;
    				start_time = k_uptime_get_32();
                }
            }
        } else if (onoffkey.isvalid)
            break;
        
        if (k_uptime_get_32() - start_time > ATTEST_KEY_WAIT_TIME) {
            SYS_LOG_INF("warning: key test timeout!\n");
            break;
        }
    }
    return key_count;
}

static int act_test_key_init(void)
{
    adc_dev = device_get_binding(CONFIG_ADC_0_NAME);
    if (!adc_dev) {
        SYS_LOG_INF("cannot found adc_dev\n");
        return -1;
    }
    
    keydata.seq_table.entries = &keydata.seq_entry;
    keydata.seq_table.num_entries = 1;
    
    keydata.seq_entry.buffer = &keydata.key_buf[0];
    keydata.seq_entry.buffer_length = sizeof(keydata.key_buf);
    keydata.seq_entry.sampling_delay = 0;
    keydata.seq_entry.channel_id = CONFIG_INPUT_DEV_ACTS_ADCKEY_ADC_CHAN;

    att_key_value_range_init();

    return 0;
}

static bool act_test_key_test(void)
{
    bool ret_val = true;
    u8_t i, j, tested_num, valid_num = 0;

    if(act_test_key_init() != 0) {
        SYS_LOG_INF("init error!\n");
        return false;
    }

    tested_num = adckey_test_proceeding();
    printk("\nadckeys are:");
    for (i = 0; i < tested_num; i++) {
        if (adckey[i].isvalid) {
            valid_num++;
            printk(" %s", adckey[i].keyname);

            /* key combination: keyvalue change greatly */
            for (j = 0; j < i; j++) {
                if (strcmp(adckey[i].keyname, adckey[j].keyname) == 0) {
                    ret_val = false;
                    break;
                }
            }
        } else {
            ret_val = false;
            printk(" 0x%x",adckey[i].keyvalue);
        }
    }

    if (valid_num != ATTEST_ADCKEY_NUM)
        ret_val = false;

    printk("\n%d in %d is valid\n", valid_num, ATTEST_ADCKEY_NUM);

    if(onoffkey.isvalid) {
        printk("ONOFF key is valid\n\n");
    } else {
        ret_val = false;
		printk("ONOFF key is invalid\n\n");
	}
    
    return ret_val;
}

static bool act_test_report_key_result(bool ret_val)
{
    u8_t i, name_len;
    char * key_names = app_mem_malloc(0x80);
    
    return_data->test_id = self->test_id;
    return_data->test_result = ret_val;

    if(key_name == NULL)
        return false;
    memset(key_names, 0, 0x80);

    strcat(key_names, onoffkey.keyname);
    uint32_to_unicode((u8_t)onoffkey.isvalid, &return_data->return_arg[trans_bytes], &trans_bytes, 10);
    return_data->return_arg[trans_bytes++] = 0x002c;
    
    for(i = 0; i < ATTEST_ADCKEY_NUM; i++) {
        strcat(key_names, ",");
        strcat(key_names, adckey[i].keyname);
        
        uint32_to_unicode(adckey[i].keyvalue, &return_data->return_arg[trans_bytes], &trans_bytes, 16);
        return_data->return_arg[trans_bytes++] = 0x002c;
    }
    
    name_len = strlen((const char *)key_names);
    utf8str_to_unicode((u8_t *)key_names, name_len, &return_data->return_arg[trans_bytes], &trans_bytes);

    app_mem_free(key_names);
    
    //add endl
    return_data->return_arg[trans_bytes++] = 0x0000;

    //If the parameter is not four byte aligned, four byte alignment is required
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
    
    ret_val = act_test_key_test();
    ret_val = act_test_report_key_result(ret_val);

    SYS_LOG_INF("att test end: %d\n", ret_val);
    return ret_val;
}
