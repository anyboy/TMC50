#ifndef __AP_AUTOTEST_H
#define __AP_AUTOTEST_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "att"
#include <logging/sys_log.h>
#endif

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <msg_manager.h>
#include <bt_manager.h>
#include <hotplug_manager.h>
#include <property_manager.h>
#include <thread_timer.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hex_str.h>
#include <global_mem.h>
#include <dvfs.h>
#include <thread_timer.h>
#include "app_defines.h"
#include "app_ui.h"
#include "app_switch.h"

#include <drivers/stub/stub.h>

#include "att_stub_cmd_def.h"
#include "att_testid_def.h"
#include "att_protocol_def.h"
#include "att_env_var.h"
#include "ap_autotest_mem_define.h"

/* msg define */
#define MSG_ATT_BT_ENGINE_READY  1

extern struct device * stub_dev;

extern autotest_test_info_t g_test_info;

extern atf_head_t g_atf_head;

extern struct att_env_var g_att_env_var;

extern void bytes_to_unicode(u8_t *byte_buffer, u8_t byte_index, u8_t byte_len, u16_t *unicode_buffer, u16_t *unicode_len);
extern void uint32_to_unicode(u32_t value, u16_t *unicode_buffer, u16_t *unicode_len, u32_t base);
extern void int32_to_unicode(s32_t value, u16_t *unicode_buffer, u16_t *unicode_len, u32_t base);
extern s32_t utf8str_to_unicode(u8_t *utf8, u32_t utf8Len, u16_t *unicode, u16_t *unicode_len);

extern s32_t att_write_data(u16_t cmd, u32_t payload_len, u8_t *data_addr);
extern s32_t att_read_data(u16_t cmd, u32_t payload_len, u8_t *data_addr);
extern s32_t att_log_to_pc(u16_t log_cmd, const u8_t *msg, u8_t *cmd_buf);

extern bool act_test_report_result(return_result_t *write_data, u16_t payload_len);
extern bool att_cmd_send(u16_t stub_cmd, u8_t try_count);

extern int test_dispatch(void);

extern int read_atf_file(u8_t *dst_addr, u32_t dst_buffer_len, u32_t offset, s32_t read_len);
extern int read_atf_sub_file(u8_t *dst_addr, u32_t dst_buffer_len, const u8_t *sub_file_name, s32_t offset, s32_t read_len);

/* pattern private interface */
extern u8_t unicode_to_uint8(u16_t widechar);
extern s32_t unicode_to_int(u16_t *start, u16_t *end, u32_t base);
extern s32_t unicode_encode_utf8(u8_t *s, u16_t widechar);
extern s32_t unicode_to_utf8_str(u16_t *start, u16_t *end, u8_t *utf8_buffer, u32_t utf8_buffer_len);
extern s32_t unicode_to_uint8_bytes(u16_t *start, u16_t *end, u8_t *byte_buffer, u8_t byte_index, u8_t byte_len);
extern u8_t *act_test_parse_test_arg(u16_t *line_buffer, u8_t arg_number, u16_t **start, u16_t **end);


#endif /* __AP_AUTOTEST_H */
