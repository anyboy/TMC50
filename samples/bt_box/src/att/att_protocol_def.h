#ifndef __ATT_PROTOCOL_DEF_H
#define __ATT_PROTOCOL_DEF_H

#include <logging/sys_log.h>
#include <zephyr.h>

#include <drivers/stub/stub.h>

//ATT test item timeout value, unit:s
#define  ATT_ITEM_TEST_TIMEOUT       60

/* atf_dir_t max items is variable, such as 15,31,47,... , max value No more than 255 */

#define ATF_MAX_SUB_FILENAME_LENGTH  12
typedef struct
{
    u8_t  filename[ATF_MAX_SUB_FILENAME_LENGTH];
    u32_t checksum;
    u32_t offset;
    u32_t length;
    u32_t load_addr;
    u32_t run_addr;
} atf_dir_t;

typedef struct
{
    u8_t magic[8];
    u8_t sdk_version[4];
    u8_t file_total;
    u8_t reserved[7];
    u8_t build_time[12];
} atf_head_t;

typedef enum
{
    TEST_MODE_CARD = 0,
    TEST_MODE_USB,
    TEST_MODE_UART,
    TEST_MODE_EXTEND
} test_mode_e;

/*
exit_mode after att test done
*/
typedef enum
{
    ATT_EXIT_REGULAR = 1,
    ATT_EXIT_SHUTOFF_DIRECTLY,
    ATT_EXIT_SHUTOFF_PLUG_USB,
    ATT_EXIT_SYS_REBOOT_PLUG_USB,
} att_exit_mode_e;

typedef enum
{
    TEST_PASS = 0,
} test_result_e; /*FIXME*/

typedef enum
{
    TEST_STATE_INIT = 0x0,
    TEST_STATE_START = 0x1,
    TEST_STATE_STOP = 0x2,
    TEST_STATE_PAUSE = 0x3,
} test_state_e;

typedef enum
{
    //DUT connect directly with PC
    DUT_CONNECT_MODE_DIRECT = 0,
    //DUT connect by UDA with PC
    DUT_CONNECT_MODE_UDA,
} dut_connect_state_e;

typedef enum
{
    //DUT In normal operation mode, the test error is stopped immediately
    DUT_WORK_MODE_NORMAL = 0,
    //DUT will Continue with the next test after a test error
    DUT_WORK_MODE_SPECIAL = 1,
} dut_work_mode_e;

typedef enum
{
    TEST_RECORD_BTADDR_INCREASE = 0x0,
    TEST_RECORD_BTADDR_RANDOM,
    TEST_RECORD_BTADDR_FIX,
    TEST_RECORD_TESTCOUNT,
} test_record_info_e;

typedef struct
{
    stub_ext_cmd_t ext_cmd;
    u16_t offsetl;
    u16_t offseth;
    u16_t lengthl;
    u16_t lengthh;
} att_fread_arg_t;

typedef struct
{
    stub_ext_cmd_t ext_cmd;
    u8_t dut_connect_mode;
    u8_t dut_work_mode;
    u8_t timeout;
    u8_t reserved;
    u8_t bdaddr[6];
    u8_t reserved2[22];
} att_start_arg_t;

typedef struct
{
    stub_ext_cmd_t ext_cmd;
    u8_t timeout;
    u8_t reserved[31];
} att_hold_arg_t;

typedef struct
{
    stub_ext_cmd_t ext_cmd;
    u8_t reboot_timeout;
    u8_t reserved[31];
} att_swtich_fw_arg_t;

typedef struct
{
    u16_t last_test_id;
    u8_t reserved[30];
} pc_test_info_t;

typedef struct
{
    stub_ext_cmd_t ext_cmd;
    pc_test_info_t pc_test_info;
} att_pc_test_info_t;

typedef struct
{
    u8_t test_state;
    u8_t reserved;
    u16_t test_id;
    u16_t arg_len;
    u32_t run_addr;
    char last_pt_name[13]; //8+1+3+'\0'
} autotest_test_info_t;

typedef struct
{
    u8_t stub_head[6];
    /*Variable length array*/
    u8_t arg_data[0];
} att_pc_arg_data_t;

typedef struct
{
    u8_t stub_head[6];
    u16_t test_id;
    u8_t test_result;
    u8_t timeout;
    /*Variable length array*/
    u16_t return_arg[0];
} return_result_t;

typedef struct
{
    stub_ext_cmd_t cmd;
    u8_t log_data[0];
} print_log_t;

//atf file read data request packet
typedef struct
{
    stub_ext_cmd_t cmd;
    u32_t offset;
    u32_t readLen;
} __attribute__((packed)) atf_file_read_t;

//atf file read data reply packet
typedef struct
{
    stub_ext_cmd_t cmd;
    u8_t payload[0];
} __attribute__((packed)) atf_file_read_ack_t;

#endif /* __ATT_PROTOCOL_DEF_H */
