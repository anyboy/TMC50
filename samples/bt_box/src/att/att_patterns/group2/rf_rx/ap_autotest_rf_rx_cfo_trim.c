/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_cfo_trim.c
*/

#include "ap_autotest_rf_rx.h"

//#define CONFIG_APPROACH_OLD_EFUSE

#ifdef CONFIG_APPROACH_OLD_EFUSE
static u32_t my_abs(s32_t value)
{
    if (value > 0)
        return value;
    else
        return (0 - value);
}
#endif

u8_t att_read_trim_cap(u32_t mode)
{
    u32_t trim_cap;
    int ret_val;

    ret_val = freq_compensation_read(&trim_cap, mode);

    if (ret_val == TRIM_CAP_READ_NO_ERROR) {
        return trim_cap;
    } else {
        extern int soc_get_hosc_cap(void);
        return soc_get_hosc_cap();
    }
}

int32_t freq_compensation_write(uint32_t *trim_cap, uint32_t mode);

u8_t att_write_trim_cap(u8_t index, u32_t mode)
{
    u32_t trim_cap = index;
    u32_t value_bak;
    int ret_val;

#ifdef CONFIG_APPROACH_OLD_EFUSE
	//Read the old written value and make an approximation comparison with the new value.
	//If the old value is a subset of the new value, you can avoid writing once more
	ret_val = freq_compensation_read(&value_bak, mode);
	if (ret_val == TRIM_CAP_READ_NO_ERROR) { /*FIXME : return must differentiate between efuse and nvram*/
		if (my_abs(index - value_bak) <= 2)
			trim_cap = value_bak;
		printk("old val %d set val %d new val %d", value_bak, index, trim_cap);
	}
#endif

    ret_val = freq_compensation_write(&trim_cap, mode);

    if (ret_val == TRIM_CAP_WRITE_NO_ERROR) {
        //Read the frequency offset again to see if they are equal
        ret_val = freq_compensation_read(&value_bak, mode);
        if (ret_val == TRIM_CAP_READ_NO_ERROR) {
            if (value_bak == trim_cap)
                return trim_cap;
        }
        return 0;
    } else {
        if (ret_val == TRIM_CAP_WRITE_ERR_NO_RESOURSE)
            printk("efuse write over!\n");
        else if (ret_val == TRIM_CAP_WRITE_ERR_HW)
            printk("efuse write HW err!\n");

        return 0;
    }
}

extern int soc_atp_get_hosc_calib(int id, unsigned char *calib_val);

//3*8=24bit
#define MAX_EFUSE_CAP_RECORD  (CONFIG_COMPENSATION_FREQ_INDEX_NUM)

static uint32_t read_efuse_freq_value(uint32_t *read_cap_value, int *index)
{
    int i;
    uint8_t cap_value[MAX_EFUSE_CAP_RECORD];

    for (i = 0; i < MAX_EFUSE_CAP_RECORD; i++) { 
        soc_atp_get_hosc_calib(i, &cap_value[i]);
    }

    //return the last modified value, it is the newest value
    for (i = 0; i < MAX_EFUSE_CAP_RECORD; i++) {
        if (cap_value[i] == 0) {
            break;
        }
    }

    //Not written efuse
    if (i == 0) {
        *read_cap_value = 0xff;
        *index = 0xff;
    } else {
        *read_cap_value = cap_value[i - 1];
        *index = i - 1;
    }

    return 0;
}

static int32_t write_efuse_new_value(int new_value, int old_index)
{
    int new_index;
    int ret_val;

    if (old_index != 0xff) {
        new_index = old_index + 1;
    } else {
        new_index = 0;
    }

    if (new_index < MAX_EFUSE_CAP_RECORD) {
        ret_val = soc_atp_set_hosc_calib(new_index, new_value);
    } else {
        return -2;
    }

    return ret_val;
}

static int32_t write_efuse_freq_compensation(uint32_t *cap_value)
{
    uint32_t trim_cap_value;
    int index;
    uint32_t old_cap_value;

    //write low 8bit only
    trim_cap_value = (*cap_value) & 0xff;

    read_efuse_freq_value(&old_cap_value, &index);

    if (index != 0xff) {
        if (old_cap_value == trim_cap_value) {
            return 0;
        } else {
            //Add a new frequency offset value
            //efuse already written fully
            if (index == (MAX_EFUSE_CAP_RECORD - 1)) {
                return -2;
            } else {
                return write_efuse_new_value(trim_cap_value, index);
            }
        }
    } else {
        //Not written efuse
        return write_efuse_new_value(trim_cap_value, index);
    }
}

static int32_t spi_nor_freq_compensation_param_write(uint32_t *trim_cap)
{
    int32_t ret_val;

    ret_val = property_set(CFG_BT_CFO_VAL, (void *) trim_cap, sizeof(u8_t));
    if (ret_val < 0)
        return ret_val;

    property_flush(CFG_BT_CFO_VAL);

    return ret_val;
}

int32_t freq_compensation_write(uint32_t *trim_cap, uint32_t mode)
{
    int ret_val;

    //Two bytes in total, two bytes of symmetric data
    *trim_cap &= 0xffff;

#ifdef CONFIG_COMPENSATION_HOSC_CAP_NVRAM_PRIORITY
    ret_val = spi_nor_freq_compensation_param_write(trim_cap);
    if (ret_val == 0) {
        ret_val = TRIM_CAP_WRITE_NO_ERROR;
    } else {
        ret_val = TRIM_CAP_WRITE_ERR_HW;
    }
#else
    ret_val = write_efuse_freq_compensation(trim_cap);

    if (mode == RW_TRIM_CAP_SNOR) {
        //efuse already written fully
        if (ret_val == -2) {
            //write norflash nvram
            ret_val = spi_nor_freq_compensation_param_write(trim_cap);

            if (ret_val == 0) {
                ret_val = TRIM_CAP_WRITE_NO_ERROR;
            } else {
                ret_val = TRIM_CAP_WRITE_ERR_HW;
            }
        } else if (ret_val == -1) {
            ret_val = TRIM_CAP_WRITE_ERR_HW;
        } else {
            ret_val = TRIM_CAP_WRITE_NO_ERROR;
        }
    } else {
        //efuse already written fully
        if (ret_val == -2) {
            ret_val = TRIM_CAP_WRITE_ERR_NO_RESOURSE;
        } else if (ret_val == -1) {
            ret_val = TRIM_CAP_WRITE_ERR_HW;
        } else {
            ret_val = TRIM_CAP_WRITE_NO_ERROR;
        }
    }
#endif

    return ret_val;
}

