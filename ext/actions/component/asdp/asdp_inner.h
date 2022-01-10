#ifndef __ASDP_INNER_H
#define __ASDP_INNER_H

#define MAX_ASDP_SERVICE_NUM 3

#include <logging/sys_log.h>
#include <os_common_api.h>


#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "ASDP"
#endif


#if 1
#define ASDP_DBG SYS_LOG_DBG
#define ASDP_INF SYS_LOG_INF
#define ASDP_WRN SYS_LOG_WRN
#define ASDP_ERR SYS_LOG_ERR
//#define ASDP_HEX(str, buf, size) do {printk("%s\n", str); print_buffer(buf, 1, size, 16, 0);} while(0)
#define ASDP_HEX(...) do{}while(0)
#else
#define ASDP_DBG(...) do{}while(0)
#define ASDP_INF(...) do{}while(0)
#define ASDP_WRN(...) do{}while(0)
#define ASDP_ERR(...) do{}while(0)
#endif

#define ASDP_SEND_BUFFER_SIZE 0x1000
#define ASDP_RECEIVE_PACKAGE_MAX_SIZE 0x100

#define TLV_MAX_DATA_LENGTH	0x3fff
#define TLV_TYPE_ERROR_CODE	0x7f
#define TLV_TYPE_MAIN		0x80


struct asdp_ctx {
	u8_t init;
	u8_t thread_terminal;
	u8_t thread_terminaled;
	char *send_buf;
	int send_buf_size;
	u8_t receive_buf[ASDP_RECEIVE_PACKAGE_MAX_SIZE];
	os_mutex mutex;
	os_tid_t port_thread;
	struct asdp_port_srv cmd_proc[MAX_ASDP_SERVICE_NUM];
	usp_handle_t usp;
};

extern struct asdp_ctx *asdp_get_context(void);

#endif
