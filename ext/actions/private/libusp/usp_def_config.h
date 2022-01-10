#ifndef CONFIG_USP_DEF_CONFIG_H
#define CONFIG_USP_DEF_CONFIG_H

#define SYS_LOG_DOMAIN "USP"
#include <logging/sys_log.h>

#include <kernel.h>
#include "string.h"
#include "crc.h"
#include "usp_protocol.h"
#include <misc/byteorder.h>
//#include "asm-mips/mach/design_specification_struct.h"

//#define USP_DEBUG_ENABLE
#define USP_TRACE_LEVEL (1)

//Allow slave to report protocol info to master
//#define REPORT_USP_PROTOCOL_INFO

#define USP_WAIT_ACK_TIMEOUT_MSEC    (50)

//等待对方发送数据 timeout时间, ms unit
#define USP_PROTOCOL_RX_TIMEOUT         (100)

//USP 协议重传次数
#define USP_PROTOCOL_MAX_RETRY_COUNT    (5)

#endif
