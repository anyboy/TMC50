/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for USP driver
 *
 * Sample app for USP driver. The received data is echoed back
 * to the serial port.
 */
#include <stdio.h>
#include <string.h>
#include <device.h>
#include <uart.h>
#include <zephyr.h>
#include <stdio.h>
#include <uart.h>
#include <usp_protocol.h>
#include <mem_manager.h>
#include <usp_manager.h>
#include <cbuf.h>

#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#endif
#include <logging/sys_log.h>

#define PARSER_STACK_SIZE 0x800
#define RECEIVE_STACK_SIZE 0x800

static struct k_fifo  received_info_fifo;
static struct k_fifo  parser_info_fifo;
static usp_handle_t usp_handle;

static struct k_thread recv_thread;
static k_thread_stack_t recv_stack;

static struct k_thread parser_thread;
static k_thread_stack_t parser_stack;

#define USP_MAX_HANDLER (USP_MAX_PROTOCOL_HANDLER - 1)

struct usp_manager{
	u8_t protocol_type[USP_MAX_HANDLER];
	usp_protocol_handler_t protocol_handler[USP_MAX_HANDLER];
};

static cbuf_t usp_cbuf;

static char usp_recv_buf[3 * 1024];

static char usp_temp_buffer[1024];



#define USP_RX_DMA_CHAN 0x2

#define DIRECT_ISR

#ifndef DIRECT_ISR
static int recv_dma;
static struct device *dma_dev;
#endif

static struct usp_manager usp_mgr;


K_MEM_SLAB_DEFINE(usp_sem_pool, sizeof(struct k_sem), 3, 4);

static void *usp_sem_alloc(void)
{
	void *sem;
	k_mem_slab_alloc(&usp_sem_pool, &sem, K_NO_WAIT);
	return sem;
}

static void usp_sem_destory(void *sem)
{
	k_mem_slab_free(&usp_sem_pool, &sem);
}

static int usp_sem_init(void *sem, unsigned int value)
{
	k_sem_init((struct k_sem *)sem, value, 1);
	return 0;
}

static int usp_sem_post(void *sem)
{
	k_sem_give(sem);
	return 0;
}

static int usp_sem_wait(void *sem)
{
	k_sem_take(sem, K_FOREVER);
	return 0;
}

static int usp_sem_waitdelay(void *sem, u32_t timeout)
{
	k_sem_take(sem, K_MSEC(timeout));
	return 0;
}

static int usp_msleep(u32_t sleepms)
{
	k_sleep(K_MSEC(sleepms));
	return 0;
}

static const usp_os_interface_t usp_os_api =
{
	.sem_alloc = usp_sem_alloc,
	.sem_destroy = usp_sem_destory,
	.sem_init = usp_sem_init,
	.sem_post = usp_sem_post,
	.sem_wait = usp_sem_wait,
	.sem_timedwait = usp_sem_waitdelay,
	.msleep   = usp_msleep,
};

static int usp_uart_read(u8_t *data, u32_t length, u32_t timeout_ms)
{
	struct device *uart1_dev;

	uart1_dev = device_get_binding(CONFIG_UART_ACTS_PORT_1_NAME);
	if (NULL != uart1_dev) {
		return uart_read_data(uart1_dev, data, length, timeout_ms);
	}

	return -1;
}

static int usp_uart_write(u8_t *data, u32_t length, u32_t timeout_ms)
{
	struct device *uart1_dev;

	uart1_dev = device_get_binding(CONFIG_UART_ACTS_PORT_1_NAME);
	if (NULL != uart1_dev) {
		return uart_write_data(uart1_dev, data, length);
	}

	return -1;
}

static int usp_uart_ioctl(u32_t mode, void *para1, void *para2)
{
	int ret = 0;

	switch (mode)
	{
		case USP_IOCTL_SET_BAUDRATE:
#if 0
			ret = UART_Configure((uint32)para1);
#endif
			break;

		case USP_IOCTL_CHECK_WRITE_FINISH:
#if 0
			if (!UART_TxDone())
			{
				ret = -1;
			}
#endif
			ret = 1;
			break;

		case USP_IOCTL_ENTER_WRITE_CRITICAL:
#if 0
			if (TRUE == (bool)para1)
			{
				uart_print_enable(FALSE);
			}
			else
			{
				uart_print_enable(TRUE);
			}
#endif
			break;

		case USP_IOCTL_REBOOT:
			break;

		default:
			break;
	}

	return ret;
}

#ifndef DIRECT_ISR
void usp_rx_dma_isr(struct device *dev, u32_t priv_data, int reson)
{
	if(reson == DMA_IRQ_TC){
		UspRecvIsrDmaRoutinue(&usp_handle);
	}
}

void usp_rx_dma_frame_config(void *buf, u32_t len)
{
	dma_reload(dma_dev, recv_dma, 0, (u32_t)buf, len);
	dma_start(dma_dev, recv_dma);
}

static void usp_rx_dma_init(void)
{
    struct dma_config config_data;
    struct dma_block_config head_block;

    dma_dev = device_get_binding(CONFIG_DMA_0_NAME);

	recv_dma = dma_request(dma_dev, USP_RX_DMA_CHAN);

    memset(&config_data, 0, sizeof(config_data));
    memset(&head_block, 0, sizeof(head_block));
    config_data.channel_direction = PERIPHERAL_TO_MEMORY;
    config_data.dma_slot = 0x4;
    config_data.source_data_size = 1;
	config_data.source_burst_length = 1;

    config_data.dma_callback = usp_rx_dma_isr;
    config_data.callback_data = NULL;
    config_data.complete_callback_en = 1;
	config_data.half_complete_callback_en = 0;

    head_block.source_address = (unsigned int)NULL;
    head_block.dest_address = (unsigned int)NULL;
    head_block.block_size = 0;
    head_block.source_reload_en = 0;

    config_data.head_block = &head_block;

	dma_config(dma_dev, recv_dma, &config_data);

}
#else

#include <soc.h>

void usp_rx_dma_isr(void)
{
	sys_write32((0x101) << USP_RX_DMA_CHAN, DMAIP);

	UspRecvIsrDmaRoutinue(&usp_handle);
}

extern dma_reg_t *dma_num_to_reg(int dma_no);
void usp_rx_dma_frame_config(void *buf, u32_t len)
{
	struct dma_regs *reg = dma_num_to_reg(2);

    /* reload dma */
    reg->daddr0 = (u32_t)buf;
    reg->framelen = len;

	//clear uart rx fifo error
	sys_write32((1 << 2), 0xc01a000c);

    /* start dma */
    reg->start = 0x1;
}

static void usp_rx_dma_init(void)
{
	struct dma_regs *reg = dma_num_to_reg(USP_RX_DMA_CHAN);

	reg->ctl = 0x26014;

	sys_write32(sys_read32(DMAIE) | (1 << USP_RX_DMA_CHAN), DMAIE);
	sys_write32((0x101) << USP_RX_DMA_CHAN, DMAIP);

    IRQ_CONNECT(IRQ_ID_DMA0 + USP_RX_DMA_CHAN, CONFIG_IRQ_PRIO_HIGH, usp_rx_dma_isr, IRQ_ID_DMA0 + USP_RX_DMA_CHAN, 0);
	irq_enable(IRQ_ID_DMA0 + USP_RX_DMA_CHAN);

}

#endif


static void receiver_thread_frame_notify(usp_data_frame_t *data)
{
	struct usp_recv_frame_data *copy_data = mem_malloc(sizeof(struct usp_recv_frame_data) + data->payload_len);

	if(copy_data){
		memcpy(&copy_data->frame, data, sizeof(usp_data_t));

		memcpy(copy_data->frame.payload, data->payload, data->payload_len);

		k_fifo_put(&parser_info_fifo, copy_data);
	}
}

/* 接收线程 */
static void receiver_thread_dispatch(void *p1, void *p2, void *p3) {
	usp_packet_head_t usp_head;
	while(1){
		if(cbuf_get_used_space(&usp_cbuf) >= sizeof(usp_head)){
			cbuf_read(&usp_cbuf, usp_temp_buffer, sizeof(usp_head));
			memcpy(&usp_head, usp_temp_buffer, sizeof(usp_head));
			if(usp_head.payload_len){
				cbuf_read(&usp_cbuf, &usp_temp_buffer[sizeof(usp_head)], usp_head.payload_len + 2);
			}

			UspCheckFrameProcess(&usp_handle, usp_temp_buffer, sizeof(usp_data_pakcet_t) + usp_head.payload_len);
		}

		os_sleep(1);
	}
}

static void USPParseBasicCommand(usp_handle_t *usp_handle, usp_data_t *data)
{
	USPBasicCommandProcess(usp_handle, data->predefine_command, data->payload, data->payload_len);
}

/* 解析线程，上层应用层 */
static void parser_thread_dispatch(void *p1, void *p2, void *p3)
{
	usp_handle_t *handle = &usp_handle;

	struct usp_recv_frame_data *data;

	int i;

	while(1){
		data = k_fifo_get(&parser_info_fifo, K_FOREVER);
		if(data){
			if(data->frame.type != USP_PACKET_TYPE_DATA){
				panic("data err");
			}

			if(data->frame.protocol_type == 0){
				USPParseBasicCommand(handle, &data->frame);
			}else{
				for(i = 0; i < USP_MAX_HANDLER; i++){
					if(usp_mgr.protocol_type[i] == data->frame.protocol_type){
						usp_mgr.protocol_handler[i](&data->frame);
						break;
					}
				}

				if(i == USP_MAX_HANDLER){
					printk("invalid data %p %d\n", data, data->frame.protocol_type);
				}
			}
			mem_free(data);
		}
	}
}

static int usp_data_recv_handler(void *recv_data, u32_t len)
{
	int ret = 0;

	if(recv_data){
		//memcpy(data->data, recv_data, len);
		//data->len = len;
		//k_fifo_put(&received_info_fifo, data);
		//print_buffer(recv_data, 1, len, 16, -1);
		//if(len == sizeof(usp_data_pakcet_t) + sizeof(perf_data_t)){
		//	perf_data_t *data = (perf_data_t *)((u8_t *)recv_data + sizeof(usp_data_pakcet_t));
		//	printk("recv seq %d\n", data->seq);
		//}

		usp_packet_head_t *usp_head = (usp_packet_head_t *)recv_data;

		if(usp_head->type != USP_PACKET_TYPE_DATA && usp_head->type != USP_PACKET_TYPE_RSP){
			panic("data0 err");
		}

		ret = cbuf_write(&usp_cbuf, recv_data, len);

		if(ret == 0){
			return -ENOMEM;
		}else{
			ret = 0;
		}
	}

	return ret;
}


void usp_manager_init(void)
{
	usp_init_param_t param;

	usp_phy_interface_t phy_api;

	phy_api.read = usp_uart_read;
	phy_api.write = usp_uart_write;
	phy_api.ioctl = usp_uart_ioctl;

	usp_rx_dma_init();

	param.api = &phy_api;
	param.os_api = &usp_os_api;
	param.handler = receiver_thread_frame_notify;
	param.recv_data_handler = usp_data_recv_handler;
	param.recv_trigger_handler = usp_rx_dma_frame_config;

	InitUSPProtocolFD(&usp_handle, &param);

    recv_stack = mem_malloc(RECEIVE_STACK_SIZE);
    if(!recv_stack){
        return;
    }

    parser_stack = mem_malloc(PARSER_STACK_SIZE);
    if(!parser_stack){
        return;
    }

	cbuf_init(&usp_cbuf, usp_recv_buf, sizeof(usp_recv_buf));

	k_fifo_init(&received_info_fifo);
	k_fifo_init(&parser_info_fifo);

	k_thread_create(&recv_thread, recv_stack, RECEIVE_STACK_SIZE, receiver_thread_dispatch, \
	 NULL, NULL, NULL, 1, 0, 0);

	k_thread_create(&parser_thread, parser_stack, PARSER_STACK_SIZE, parser_thread_dispatch, \
	 NULL, NULL, NULL, 2, 0, 0);
}

int usp_manager_open(void)
{
	while(1){
		SYS_LOG_INF("wait connect...");

		ConnectUSPFD(&usp_handle);

		if(UspIsHardwareConnected(&usp_handle)){
			break;
		}

		os_sleep(5000);
	}

	return 0;
}

int usp_manager_open_protocol(u8_t protocol)
{
	return OpenUSPFD(&usp_handle, protocol);
}

int usp_manager_register_handler(u8_t protocol, usp_protocol_handler_t handler)
{
	int i;

	for(i = 0; i < USP_MAX_HANDLER; i++){
		if(usp_mgr.protocol_handler[i] == NULL && usp_mgr.protocol_type[i] == 0){
			usp_mgr.protocol_handler[i] = handler;
			usp_mgr.protocol_type[i] = protocol;
			break;
		}
	}

	return 0;
}

int usp_manager_send_data(u8_t *data, u32_t len)
{
	return WriteUSPDataFD(&usp_handle, data, len);
}


int usp_manager_close_protocol(u8_t protocol_type)
{
	return 0;
}

int usp_manager_get_lost_frame(void)
{
	return UspGetDropFrameCnt(&usp_handle);
}
