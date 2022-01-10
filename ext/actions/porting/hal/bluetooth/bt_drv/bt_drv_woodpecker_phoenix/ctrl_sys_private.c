/*
 * Copyright(c) 2017-2027 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * Johnny           2017-9-5        create initial version
 */
/*******************************
 * include files
 *******************************/
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <soc.h>
#include <assert.h>
#include <irq.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <mem_manager.h>
#include <sys_wakelock.h>
#include <linker/section_tags.h>
#include <shell/shell.h>
#include <bluetooth/btdrv_api.h>
#include "ctrl_interface.h"
#include <hex_str.h>
#include <property_manager.h>

#define SYS_LOG_DOMAIN                  "BTC"

#include <logging/sys_log.h>

/*******************************
 * macro definitions
 *******************************/

#define CONFIG_ZEPHYR_STACK

#ifndef __TIME_CRITICAL_ROM
#define __TIME_CRITICAL_ROM __ramfunc
#endif

#ifndef CONFIG_KERNEL_SHELL
void dump_irqstat(void)
{
}
#endif

#ifdef CONFIG_BT_CONTROLER_DEBUG_PRINT
#define CONFIG_BT_HCI_TX_PRINT 1
#define CONFIG_BT_HCI_RX_PRINT 1
#endif

#define MAX_PRINT_TX_COUNT              (50)
#define MAX_PRINT_RX_COUNT              (50)

#define CONFIG_2WIRE_DEBUG_DISABLE      0x20000000
#define CONFIG_DUMP_RX_ADC_DATA         0x40000000
#define CONFIG_GPIO_DEBUG_DISABLE       0x80000000

#define BB_TXRAM_ADDR                   0xC0176740
#define BB_RXRAM_ADDR                   0xC0176B80

#define HCI_PACKET_TYPE_COMMAND         0x01
#define HCI_PACKET_TYPE_ACL             0x02
#define HCI_PACKET_TYPE_SCO             0x03
#define HCI_PACKET_TYPE_EVENT           0x04

#define HCI_NUM_COMPELET_EVENT          0x13

/* Connection Handle 12bits, PB Flag 2bits, BC Flag 2bits, Data Total Length 2bytes */
#define HCI_ACL_HEADER_LEN              4

/* Connection Handle 12bits, Packet_Status_Flag 2bits, Data Total Length 1byte */
#define HCI_SCO_HEADER_LEN              3

/* Event Code 1byte, Parameter Total Length 1byte */
#define HCI_EVENT_HEADER_LEN            2

#define CFG_BT_MAC                      "BT_MAC"

#define BT_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
    static char __in_section_unique(BTCON_MEMPOOL) __aligned(slab_align) \
        _k_mem_slab_buf_##name[(slab_num_blocks) * (slab_block_size)]; \
    static struct k_mem_slab name \
        __in_section(_k_mem_slab, static, name) = \
        _K_MEM_SLAB_INITIALIZER(name, _k_mem_slab_buf_##name, \
                      slab_block_size, slab_num_blocks)


#define  UART0_REGISTER_BASE  0xC01a0000
#define  UART1_REGISTER_BASE  0xC01b0000
enum hci_pkt_type {
    HCI_CMD_PKT = 0x01,
    HCI_ACL_PKT,
    HCI_SYNC_PKT,
    HCI_EVT_PKT
};
#define BT_HCI_ACL_HDR_SIZE             4
#define BT_HCI_SYNC_HDR_SIZE            3
#define BT_HCI_CMD_HDR_SIZE             3
#define BT_HCI_EVT_HDR_SIZE				2


#define UART_CTL    0
#define UART_RXDAT  0x4
#define UART_TXDAT  0x8
#define UART_STA    0xc
#define UART_BR  	0x10

#define MRCR0 0xc0000000
#define MRCR0_UART1RESET 19
#define MRCR0_UART0RESET 18
#define CMU_DEVCLKEN0_UART1CLKEN  19
#define CMU_DEVCLKEN0_UART0CLKEN  18
/*******************************
 * variable definitions
 *******************************/
/* Memory pool for BT controller pages, 2KB * 7 = 14KB */
BT_MEM_SLAB_DEFINE(btdrv_con_pool, 2048, 7, 4);
static int btcon_mem_page_cnt;

static K_SEM_DEFINE(ctrl_event_sem, 0, 0xffffffff);

typedef void(*irq_handler_t)(void *);
static irq_handler_t g_tws_irq_handler = NULL;

static char __in_section_unique(BTCON_MEMPOOL) __aligned(STACK_ALIGN) btcon_thread_stack[CONFIG_CONTROLLER_HIGH_STACKSIZE];
static __in_section_unique(btcon.noinit.stack) struct k_thread btcon_thread;
/*******************************
 * function declaration
 *******************************/
extern void __attribute__((long_call)) bb_use_dma_access_txrx_ram(void);
extern void __attribute__((long_call)) bb_use_mcu_access_txrx_ram(void);
extern void __attribute__((long_call)) irq_bb_intr_handle(void);
extern void __attribute__((long_call)) irq_tws_intr_handle(void);
extern void __attribute__((long_call)) irq_disable_tws_sync_intr(void);
extern void __attribute__((long_call)) modem_reset_rx_adc_fifo(void);
extern void sys_set_bt_ctrl_sleep_pending(int pending, uint32_t sleep_ms);
extern int nvram_config_get(const char *name, void *data, int max_len);
//extern unsigned int ctrl_switch0;
extern void sys_set_bt_host_wake_up_pending(uint8_t pending);
#ifdef CONFIG_BT_SNOOP
extern int btsnoop_init(void);
extern int btsnoop_write_packet(u8_t type, const u8_t *packet, bool is_received);
#endif
extern int btsrv_sco_data_out_cb(struct rt_sco_data *rsd);
extern int btsrv_sco_data_in_cb(struct rt_sco_data *rsd);

#ifdef CONFIG_BT_HCI_TX_PRINT
static const char *tx_type_string[] = {
    "NULL:",
    "TX:01 ",
    "TX:02 ",
    "TX:03 ",
    "TX:04 "
};
#endif

#ifdef CONFIG_BT_HCI_RX_PRINT
static const char *rx_type_string[] = {
    "NULL:",
    "RX:01 ",
    "RX:02 ",
    "RX:03 ",
    "RX:04 "
};
#endif

static btdrv_rx_cb_t btdrv_recieve_data_cbk = NULL;
struct uart_bqb {
	uint32_t uart_base;
	uint8_t *buf;
};

struct uart_bqb uart_bqb_inst;
/*******************************
 * function definitions
 *******************************/
__TIME_CRITICAL_ROM unsigned int bt_local_irq_save(void)
{
    return irq_lock();
}

__TIME_CRITICAL_ROM void bt_local_irq_restore(unsigned int flags)
{
    irq_unlock(flags);
}

__TIME_CRITICAL_ROM void bt_os_sched_lock(void)
{
    k_sched_lock();
}

__TIME_CRITICAL_ROM void bt_os_sched_unlock(void)
{
    k_sched_unlock();
}

__TIME_CRITICAL_ROM void event_post(void)
{
    k_sem_give(&ctrl_event_sem);
}

__TIME_CRITICAL_ROM void event_pend(void)
{
    k_sem_take(&ctrl_event_sem, K_FOREVER);
}

__TIME_CRITICAL_ROM unsigned int get_current_time(void)
{
    return k_uptime_get_32();
}

unsigned int get_sys_c0_count(void)
{
    return k_cycle_get_32();
}

__TIME_CRITICAL_ROM void bt_gpio_output(unsigned int gpio_pin, unsigned int level)
{
//    if (ctrl_switch0 & CONFIG_GPIO_DEBUG_DISABLE) {
//        return;
//    }

    if(level)
        /* output high level */
        sys_write32(GPIO_BIT(gpio_pin), GPIO_REG_BSR(GPIO_REG_BASE, gpio_pin));
    else
        /* output low level */
        sys_write32(GPIO_BIT(gpio_pin), GPIO_REG_BRR(GPIO_REG_BASE, gpio_pin));
}

void bt_gpio_enable_output(unsigned int gpio_pin, unsigned int default_level)
{
    uint32_t gpio_ctrl;

//    if (ctrl_switch0 & CONFIG_GPIO_DEBUG_DISABLE) {
//        return;
//    }

    bt_gpio_output(gpio_pin, default_level);

    gpio_ctrl = GPIO_REG_CTL(GPIO_REG_BASE, gpio_pin);
    sys_write32((sys_read32(gpio_ctrl) & ~(GPIO_CTL_MFP_MASK | GPIO_CTL_GPIO_INEN))
        | GPIO_CTL_MFP_GPIO | GPIO_CTL_GPIO_OUTEN, gpio_ctrl);
}

__TIME_CRITICAL_ROM void *add_one_mem_block(void)
{
    void *block;

    if (k_mem_slab_alloc(&btdrv_con_pool, &block, K_NO_WAIT) != 0) {
        SYS_LOG_ERR("Failed to allocate page for bt controller");
        return NULL;
    }

    btcon_mem_page_cnt++;
    SYS_LOG_DBG("malloc page: %p, total %d", block, btcon_mem_page_cnt);
    return block;
}

__TIME_CRITICAL_ROM void subtract_one_block(void *block)
{
    btcon_mem_page_cnt--;
    SYS_LOG_DBG("free page: %p, total %d", block, btcon_mem_page_cnt);

    k_mem_slab_free(&btdrv_con_pool, &block);
}

#ifdef CONFIG_CONSOLE_SHELL
static int shell_cmd_btcon_mem_info(int argc, char *argv[])
{
    int page_cnt;

    page_cnt = ctrl_mem_dump_info();
    SYS_LOG_INF("page cnt %d, %d\n", page_cnt, btcon_mem_page_cnt);

    return 0;
}

static int shell_cmd_btcon_log_level(int argc, char *argv[])
{
    unsigned int log_level;

    if (argc < 2) {
        SYS_LOG_INF("btcon: current log level %d", ctrl_get_log_level());
	return 0;
    }

    log_level = atoi(argv[1]);
    ctrl_set_log_level(log_level);

    SYS_LOG_INF("btcon: set log level %d", log_level);

    return 0;
}

static int shell_cmd_btcon_log_module_mask(int argc, char *argv[])
{
    unsigned int log_module_mask;

    if (argc < 2) {
        SYS_LOG_INF("btcon: current log module mask 0x%x", ctrl_get_log_module_mask());
	return 0;
    }

    log_module_mask = strtoul(argv[1], NULL, 16);
    ctrl_set_log_module_mask(log_module_mask);

    SYS_LOG_INF("btcon: set log module mask 0x%x", log_module_mask);

    return 0;
}

static int shell_cmd_btcon_trace_module_mask(int argc, char *argv[])
{
    unsigned int trace_module_mask;

    if (argc < 2) {
        SYS_LOG_INF("btcon: current trace module mask 0x%x", ctrl_get_trace_module_mask());
	return 0;
    }

    trace_module_mask = strtoul(argv[1], NULL, 16);
    ctrl_set_trace_module_mask(trace_module_mask);

    SYS_LOG_INF("btcon: set trace module mask 0x%x", trace_module_mask);

    return 0;
}

static const struct shell_cmd btcon_commands[] = {
    { "mem_info", shell_cmd_btcon_mem_info, "show btcon memory info" },
    { "log_level", shell_cmd_btcon_log_level, "btcon log level" },
    { "log_module_mask", shell_cmd_btcon_log_module_mask, "btcon log module mask" },
    { "trace_module_mask", shell_cmd_btcon_trace_module_mask, "btcon trace module mask" },
    { NULL, NULL, NULL }
};
SHELL_REGISTER("btcon", btcon_commands);
#endif

__TIME_CRITICAL_ROM void btcon_irq_handle(void *arg)
{
    irq_bb_intr_handle();
}

signed int request_bb_irq(void)
{
    IRQ_CONNECT(IRQ_ID_BT_BASEBAND, CONFIG_IRQ_PRIO_HIGHEST, btcon_irq_handle, NULL, 0);
    irq_enable(IRQ_ID_BT_BASEBAND);
    return 0;
}

signed int free_bb_irq(void)
{
    irq_disable(IRQ_ID_BT_BASEBAND);
    return 0;
}

void enter_sleep_notification(void)
{
    uint32_t sleep_ms;

    /* get sleep time from from AUX_TIMER (* 2.5 ms) */
    sleep_ms = (sys_read32(0xc01760a0) << 1) + (sys_read32(0xc01760a0) >> 1);

    sys_set_bt_ctrl_sleep_pending(1, sleep_ms);
}

void exit_sleep_notification(void)
{
    sys_set_bt_ctrl_sleep_pending(0, 0);
}

/* not used by now */
unsigned char ctrl_min_work_cpufreq_req(unsigned char min_clk_mhz)
{
    return 24;
}

void ctrl_set_high_cpufreq(unsigned char enable)
{
#if 0
    if (enable)
        soc_dvfs_set_level(SOC_DVFS_LEVEL_MCU_PERFORMANCE);
    else
        soc_dvfs_unset_level(SOC_DVFS_LEVEL_MCU_PERFORMANCE);
#endif
}

signed int request_bt_sys_resource(void)
{
    /* reset rf modem and baseband */
    sys_write32(sys_read32(RMU_MRCR1) & ~0x7, RMU_MRCR1);

    /* enable 64MHz and 32MHz clock */
    sys_write32(sys_read32(SPLL_CTL) | (1 << 5)| (1 << 4) | (1 << 0), SPLL_CTL);

    /* enable rf modem baseband relative clock */
    sys_write32(sys_read32(CMU_DEVCLKEN1) | 0xbf, CMU_DEVCLKEN1);

//    if (ctrl_switch0 & CONFIG_DUMP_RX_ADC_DATA) {
//        sys_write32(sys_read32(CMU_DEVCLKEN1) | (1 << 6), CMU_DEVCLKEN1);
//    }
    /* rf modem and baseband normal */
    sys_write32(sys_read32(RMU_MRCR1) | 0x7, RMU_MRCR1);

    /* calibration interval: 100ms */
    sys_write32(0xa009ca, CMU_HCL3K2CTL);
    k_busy_wait(100);
    sys_write32(0xa009cb, CMU_HCL3K2CTL);

    return 0;
}

signed int free_bt_sys_resource(void)
{
    /* disable clken */
    sys_write32(sys_read32(CMU_DEVCLKEN1) & ~0xff, CMU_DEVCLKEN1);

    /* disable spll */
    //CMU_ANALOG_REGISTER.SPLL_CTL &= ~((1<<SPLL_CTL_SPLL_EN) | (1<<SPLL_CTL_CK32M_EN));

    /* reset rf modem and baseband */
    sys_write32(sys_read32(RMU_MRCR1) & ~0x7, RMU_MRCR1);

    return 0;
}

typedef struct
{
    volatile uint32_t DMA_CTL;
    volatile uint32_t DMA_START;
    volatile uint32_t DMA_SADDR0;
    volatile uint32_t DMA_SADDR1;
    volatile uint32_t DMA_DADDR0;
    volatile uint32_t DMA_DADDR1;
    volatile uint32_t DMA_BC;
    volatile uint32_t DMA_RC;
} dmacontroller_1_t;

static inline int get_mem_phy_addr(unsigned int addr)
{
    return addr;
}

__TIME_CRITICAL_ROM void hw_memcpy32(void *dst, const void *src, unsigned int size, unsigned char blocking)
{
    dmacontroller_1_t * dma;
    uint32_t dat, start_ts, comm2;
    int abort_fsm = 0;

    /* use dma0 */
    dma = (dmacontroller_1_t *)0xc0010100;

    if (dst == (void *)BB_TXRAM_ADDR) {

#if 1
        /* ensure TXRAM is writable */
        dat = ~(*((volatile uint32_t *)dst));
        *((volatile uint32_t *)dst) = dat;

        start_ts = k_cycle_get_32();
        while (dat != (*((volatile uint32_t *)dst))) {
            /* 15us timeout */
            if ((k_cycle_get_32() - start_ts) >= 24 * 15) {
                if (abort_fsm) {
                    SYS_LOG_ERR("txram timeout");
                    break;
                } else {
                    /* forcely abort rx fsm */
                    *(volatile uint32_t *)0xc0176048 |= 0x3;
                    k_busy_wait(3);
                    *(volatile uint32_t *)0xc0176048 &= ~0x3;

                    /* clear no pkd pending */
                   comm2 = *(volatile uint32_t *)0xc0176028;
                   if (!(comm2 & (1 << 13))) {
                       k_busy_wait(15);
                       *(volatile uint32_t *)0xc0176028 = (comm2 & 0xffff) | (1 << 29);
                   }

                    abort_fsm = 1;
                }
            }

            *((volatile uint32_t *)dst) = dat;
        }
#endif

        dma->DMA_CTL = 0x0100;
        if ((signed int)src & 3) {
            SYS_LOG_ERR("src addr:%x", (uint32_t)src);
        }

        dma->DMA_SADDR0 = get_mem_phy_addr((uint32_t)src);
        dma->DMA_DADDR0 = (unsigned int)dst;
    } else if (src == (void *)BB_RXRAM_ADDR) {
        dma->DMA_CTL = 0x0001;
        if ((signed int)dst & 3) {
            SYS_LOG_ERR("dst addr:%x", (uint32_t)dst);
        }
        dma->DMA_SADDR0 = (unsigned int)src;
        dma->DMA_DADDR0 = get_mem_phy_addr((uint32_t)dst);

    } else {
        SYS_LOG_ERR("cannot use hw_memcpy32");
        return;
    }

    dma->DMA_BC = size;

    // size = (size + 3)/4 * 4;
    if (size <= 4) {
        *((uint32_t *)dst) = *((uint32_t *)src);
        return;
    }

    bb_use_dma_access_txrx_ram();
    dma->DMA_START = 1;

    if (blocking) {
        while (dma->DMA_START);
        bb_use_mcu_access_txrx_ram();
    }
}

__TIME_CRITICAL_ROM void wait_hw_memcpy32_finish(void)
{
    dmacontroller_1_t * dma;

//    if (ctrl_switch0 & CONFIG_DUMP_RX_ADC_DATA) {
//        dma = (dmacontroller_1_t *) 0xc0040100;
//    } else {
        dma = (dmacontroller_1_t *) 0xc0010100;
//    }

    while (dma->DMA_START);
    bb_use_mcu_access_txrx_ram();
}

void dump_rx_adc_dma_init(uint32_t dst_buf, uint32_t len)
{
}

void dump_rx_adc_data_reset(void)
{
}

__TIME_CRITICAL_ROM signed int dummy_printf(const char* format, ...)
{
    return 0;
}

__TIME_CRITICAL_ROM signed int dummy_print_hex(const char* prefix, const void* data, int size)
{
    return 0;
}

extern void trace_hexdump(const char* prefix, const uint8_t* buf, uint32_t len);
__TIME_CRITICAL_ROM signed int print_hex(const char* prefix, const uint8_t* data, int size)
{
    if (!size) {
        printk("%s zero-length signal packet\n", prefix);
        return 0;
    }

	trace_hexdump(prefix, data, size);

    return 0;
}

signed int ctrl_private_init(void)
{
    k_sem_reset(&ctrl_event_sem);
    return 0;
}

signed int ctrl_private_deinit(void)
{
    return 0;
}

u8_t hci_set_acl_print_enable(u8_t enable)
{
	unsigned int mask;
	u8_t orig_enable;

	mask = ctrl_get_log_module_mask();
	orig_enable = (mask & BT_LOG_MODULE_HCI_TX) ? 1 : 0;

	if (enable)
		mask |= BT_LOG_MODULE_HCI_TX;
	else
		mask &= ~BT_LOG_MODULE_HCI_TX;

	ctrl_set_log_module_mask(mask);

	return orig_enable;
}

#ifdef CONFIG_BT_HCI_TX_PRINT
static int hci_is_enable_dump_tx(void)
{
	return (ctrl_get_log_module_mask() & BT_LOG_MODULE_HCI_TX);
}
#endif

#ifdef CONFIG_BT_HCI_RX_PRINT
static int hci_is_enable_dump_rx(void)
{
	return (ctrl_get_log_module_mask() & BT_LOG_MODULE_HCI_RX);
}
#endif

int btdrv_send(void *buf, int len, uint8_t type)
{
    if (NULL == btdrv_recieve_data_cbk) {
        return 0;
    }
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(WAKELOCK_BT_EVENT);
#endif

#ifdef CONFIG_BT_SNOOP
    if (type != HCI_PACKET_TYPE_SCO)
        btsnoop_write_packet(type, (const uint8_t *)buf, FALSE);
#endif

#ifdef CONFIG_BT_HCI_TX_PRINT
    if (hci_is_enable_dump_tx()) {
        extern bool bt_a2dp_is_media_tx_channel(u16_t handle, u16_t cid);
        u16_t handle, cid;
        u8_t *data = buf;
        u8_t tws_protocol;

        if (type == HCI_PACKET_TYPE_COMMAND) {
            print_hex(tx_type_string[type], (const uint8_t *)buf, min(len, MAX_PRINT_TX_COUNT));
        } else if ((type == HCI_PACKET_TYPE_ACL)) {
            handle = data[0] | ((data[1]&0x0F) << 8);
            cid = data[6] | (data[7] << 8);
            tws_protocol = (data[8] == 0xEE)? 1 : 0;
		#ifdef CONFIG_BT_A2DP
            if (!(bt_a2dp_is_media_tx_channel(handle, cid) && !tws_protocol)) {
                print_hex(tx_type_string[type], (const uint8_t *)buf, min(len, MAX_PRINT_TX_COUNT));
            }
		#endif
        }
    }

#endif

    ctrl_deliver_data_from_h2c(type, buf);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(WAKELOCK_BT_EVENT);
#endif

    return len;
}

static int btdrv_recive_data(uint8_t type, const uint8_t *buf)
{
    u16_t len = 0;

    sys_set_bt_host_wake_up_pending(1);
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(WAKELOCK_BT_EVENT);
#endif

    if (type == HCI_PACKET_TYPE_ACL) {
        len = ((u16_t) (*(buf + 3)) << 8) | (*(buf + 2));
        len += HCI_ACL_HEADER_LEN;
    } else if(type == HCI_PACKET_TYPE_SCO) {
        len = (*(buf + 2));
        len += HCI_SCO_HEADER_LEN;
    } else if(type == HCI_PACKET_TYPE_EVENT) {
        len = (*(buf + 1));
        len += HCI_EVENT_HEADER_LEN;
    } else {
#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(WAKELOCK_BT_EVENT);
#endif
        return 0;
    }

#ifdef CONFIG_BT_HCI_RX_PRINT
    if (hci_is_enable_dump_rx()) {
        extern bool bt_a2dp_is_media_rx_channel(u16_t handle, u16_t cid);
        u16_t handle, cid;
        u8_t continue_frag;
        u8_t tws_protocol;

        if (type == HCI_PACKET_TYPE_EVENT && (buf[0] != HCI_NUM_COMPELET_EVENT)) {
            print_hex(rx_type_string[type], (const uint8_t *)buf, min(len, MAX_PRINT_RX_COUNT));
        } else if ((type == HCI_PACKET_TYPE_ACL)) {
            handle = buf[0] | ((buf[1]&0x0F) << 8);
            cid = buf[6] | (buf[7] << 8);
            continue_frag = ((buf[1]&0x30) == 0x10)? 1 : 0;
            tws_protocol = (buf[8] == 0xEE)? 1 : 0;
		#ifdef CONFIG_BT_A2DP
            if ((!(bt_a2dp_is_media_rx_channel(handle, cid) && !tws_protocol)) && !continue_frag) {
                print_hex(rx_type_string[type], (const uint8_t *)buf, min(len, MAX_PRINT_RX_COUNT));
            }
		#endif
        }
    }
#endif

#ifdef CONFIG_BT_SNOOP
    if (type != HCI_PACKET_TYPE_SCO)
        btsnoop_write_packet(type, (const uint8_t *)buf, TRUE);
#endif

    if (btdrv_recieve_data_cbk != NULL) {
        btdrv_recieve_data_cbk((uint8_t *)buf, len, type);
    }

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(WAKELOCK_BT_EVENT);
#endif

    return 1;
}

__TIME_CRITICAL_ROM uint32_t bt_set_interrupt_time(uint16_t acl_handle, uint32_t clock)
{
    t_devicelink_clock bt_clock;

    bt_clock.bt_clk = clock & 0xFFFFFFF;
    bt_clock.bt_intraslot = 0;

    return ctrl_set_bt_clk(acl_handle, bt_clock);
}

__TIME_CRITICAL_ROM void bt_adjust_link_time(u16_t acl_handle, s16_t adjust_val)
{
      ctrl_adjust_link_time(acl_handle, adjust_val);
}

__TIME_CRITICAL_ROM uint32_t bt_read_current_btclock(uint16_t acl_handle, uint8_t *buf)
{
      return ctrl_get_bt_clk(acl_handle, (t_devicelink_clock *)buf);
}

void bt_enable_tws_irq(bool enable)
{
    SYS_LOG_INF("tws irq enable %d", enable);

    if (enable) {
#if 0
        if (_arch_irq_get_pending(IRQ_ID_BT_TWS)) {
            /* Run to here, some problem happened */
            printk("IRQ_ID_BT_TWS have pending irq\n");
        }
#endif
        irq_enable(IRQ_ID_BT_TWS);
    } else {
        bt_set_interrupt_time(0, 0);
        irq_disable(IRQ_ID_BT_TWS);
    }
}

__TIME_CRITICAL_ROM static void bt_tws_irq_handler(void *arg)
{
    if (g_tws_irq_handler)
        g_tws_irq_handler(NULL);
}

void bt_request_tws_irq(void *int_handle)
{
    SYS_LOG_INF("int_handle %p", int_handle);
    g_tws_irq_handler = int_handle;
    IRQ_CONNECT(IRQ_ID_BT_TWS, CONFIG_IRQ_PRIO_HIGHEST, bt_tws_irq_handler, NULL, 0);
}

#ifdef CONFIG_BT_CONTROLER_BQB
void BT_DutTest(int bqb_mode)
{
    u8_t eut_test_cmd[8];

    //bt_controller_test_close_watchdog();

    eut_test_cmd[0] = 0x01;
    eut_test_cmd[1] = 0x03;
    eut_test_cmd[2] = 0x0c;
    eut_test_cmd[3] = 0x00;
    btdrv_send(&eut_test_cmd[1], sizeof(eut_test_cmd) - 1, eut_test_cmd[0]);
    k_busy_wait(2000 * 1000); /* HCI RESET */

    eut_test_cmd[0] = 0x01;
    eut_test_cmd[1] = 0x1a;
    eut_test_cmd[2] = 0x0c;
    eut_test_cmd[3] = 0x01;
    eut_test_cmd[4] = 0x03; /*write scan */
    btdrv_send(&eut_test_cmd[1], sizeof(eut_test_cmd) - 1, eut_test_cmd[0]);

    k_busy_wait(20 * 1000);

    eut_test_cmd[0] = 0x01;
    eut_test_cmd[1] = 0x05;
    eut_test_cmd[2] = 0x0c;
    eut_test_cmd[3] = 0x03;
    eut_test_cmd[4] = 0x02;
    eut_test_cmd[5] = 0x00;
    eut_test_cmd[6] = 0x02;
    btdrv_send(&eut_test_cmd[1], sizeof(eut_test_cmd) - 1, eut_test_cmd[0]);
    k_busy_wait(20 * 1000); /* setup eventfilter */

    eut_test_cmd[0] = 0x01;
    eut_test_cmd[1] = 0x03;
    eut_test_cmd[2] = 0x18;
    eut_test_cmd[3] = 0x00;
    btdrv_send(&eut_test_cmd[1], sizeof(eut_test_cmd) - 1, eut_test_cmd[0]);
    k_busy_wait(20 * 1000); /* dut mode */

#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	if (bqb_mode == 1)
		SYS_LOG_INF("BT test mode Entered Classic BQB ...");
	else
		SYS_LOG_INF("BT test mode Entered BLE BQB ...");
#endif

    while (1)
    {
		;//qac
    }
}
#endif

#ifdef CONFIG_BT_CONTROLER_BLE_BQB
static int uart_bqb_rx_handler(uint8_t *hdr, int len)
{
	int rx_len = 0;

	while(!(sys_read32(uart_bqb_inst.uart_base + UART_STA) & 0x20)) {
		hdr[rx_len] = sys_read8(uart_bqb_inst.uart_base + UART_RXDAT);
		rx_len++;

		if(rx_len == len)
			return 0;
	}

	return -1;
}

static int uart_bqb_rx_fsm(uint8_t first_data)
{
	uint8_t type = first_data;
	uint16_t len, opcode, handle;
	uint8_t *hdr = uart_bqb_inst.buf;
	int status;

	printk("h4 type %x", type);

	switch (type) {
	case HCI_CMD_PKT:
		status = uart_bqb_rx_handler(hdr, BT_HCI_CMD_HDR_SIZE);
		if (status < 0)
			return -1;
		opcode = hdr[0] | (hdr[1]<<8);
		len = hdr[2];
		if (!len)
			break;
		status = uart_bqb_rx_handler(uart_bqb_inst.buf + BT_HCI_CMD_HDR_SIZE, len);
		print_hex("\nCMD:", uart_bqb_inst.buf, len+3);
		if (status < 0)
			return -1;
		break;

	case HCI_ACL_PKT:
		status = uart_bqb_rx_handler(hdr, BT_HCI_ACL_HDR_SIZE);
		if (status < 0)
			return -1;
		handle = hdr[0] | (hdr[1] <<8);
		len =  hdr[2] | (hdr[3]<<8);
		if (!len)
			break;
		status = uart_bqb_rx_handler(uart_bqb_inst.buf+BT_HCI_ACL_HDR_SIZE, len);
		print_hex("\nACL:", uart_bqb_inst.buf, len+4);
		if (status < 0)
			return -1;
	break;

	case HCI_SYNC_PKT:
		status = uart_bqb_rx_handler(hdr, BT_HCI_SYNC_HDR_SIZE);
		if (status < 0)
			return -1;
		handle = hdr[0] | (hdr[1]<<8);
		len = hdr[2];
		if (!len)
			break;
		status = uart_bqb_rx_handler(uart_bqb_inst.buf+BT_HCI_SYNC_HDR_SIZE, len);
		print_hex("\nSYNC:", uart_bqb_inst.buf, len+3);
		if (status < 0)
			return -1;
		break;

	default:
		return false;
	}

	ctrl_deliver_data_from_h2c(type, uart_bqb_inst.buf);

	return 0;
}

static void uart_rx_irq_handler(void *inst)
{
	uint32_t uart_base = uart_bqb_inst.uart_base;
	uint8_t type;

	type = sys_read8(uart_base + UART_RXDAT);
	uart_bqb_rx_fsm(type);

	sys_write32(0x0000001, uart_base + UART_STA);
}

static void uart_chimera_16550_init(uint8_t idx, uint32_t baud_rate)
{
	uint32_t div;
	uint32_t uart_base;

	if (idx > 1) {
		printk("unsupport uart_idx %d\n", idx);
		return;
	}

	uart_base = (idx == 0) ? UART0_REGISTER_BASE : UART1_REGISTER_BASE;

	if(idx == 0) {
		acts_clock_peripheral_enable(CLOCK_ID_UART0);
		acts_reset_peripheral(RESET_ID_UART0);
	} else if (idx == 1) {
		acts_clock_peripheral_enable(CLOCK_ID_UART1);
		acts_reset_peripheral(RESET_ID_UART1);
	}

	div = 24000000 / baud_rate;
	sys_write32((div << 16) | div, uart_base + UART_BR);

	sys_write32(0xf0068a03, uart_base + UART_CTL);
	sys_write32(sys_read32(uart_base + UART_STA) | 0x00c0001f, uart_base + UART_STA);

#ifdef CONFIG_BT_CONTROLER_BLE_BQB
#if (CONFIG_BT_BQB_UART_PORT == 0)
	IRQ_CONNECT(IRQ_ID_UART0, CONFIG_IRQ_PRIO_HIGH, uart_rx_irq_handler, NULL, 0);
	irq_enable(IRQ_ID_UART0);
#elif (CONFIG_BT_BQB_UART_PORT == 1)
	IRQ_CONNECT(IRQ_ID_UART1, CONFIG_IRQ_PRIO_HIGH, uart_rx_irq_handler, NULL, 0);
	irq_enable(IRQ_ID_UART1);
#else
#error "invalid bqb uart port"
#endif
#endif
	uart_bqb_inst.uart_base = uart_base;
}


static void uart_bqb_write_data(const uint8_t *buf, uint32_t cnt)
{
	int i;

	for (i = 0; i < cnt; i++) {
		while (sys_read32(uart_bqb_inst.uart_base + UART_STA) & (1 << 21))
			;

		sys_write8(buf[i], uart_bqb_inst.uart_base + UART_TXDAT);
	}
}

static int uart_bqb_tx_done()
{
	return (sys_read32(uart_bqb_inst.uart_base + UART_STA) & (1<<21));
}

int32_t uart_bqb_tx(uint8_t type, const uint8_t *buf)
{
	uint16_t len;

	switch (type) {
	case HCI_ACL_PKT:
		len =  (buf[2] | (buf[3]<<8)) + BT_HCI_ACL_HDR_SIZE;
		break;

	case HCI_SYNC_PKT:
		len = buf[2] + BT_HCI_SYNC_HDR_SIZE;
		break;

	case HCI_EVT_PKT:
		len = buf[1] + BT_HCI_EVT_HDR_SIZE;
		print_hex("\nEVT:", buf, len);
		break;

	default:
		return 0;
	}

	while(uart_bqb_tx_done())
		;

	/* sending H4 type */
	uart_bqb_write_data((const uint8_t*)&type, 1);
	/* sending payload data */
	uart_bqb_write_data(buf, len);

	/* wait TX transmit finish */
	while(uart_bqb_tx_done())
		;

	return 1;
}

void uart_bqb_init(void)
{
	printk("uart_bqb: port %d\n", CONFIG_BT_BQB_UART_PORT);

	uart_chimera_16550_init(CONFIG_BT_BQB_UART_PORT, 115200);
	uart_bqb_inst.buf = mem_malloc(1024);
}
#else
int32_t uart_bqb_tx(uint8_t type, const uint8_t *buf)
{
	return 0;
}

void uart_bqb_init(void)
{
}
#endif

static void btcon_thread_loop(void *param,void *param2,void *param3)
{
    SYS_LOG_INF("started\n");

    while (1) {
        ctrl_loop();
    }

    SYS_LOG_INF("stopped\n");
    return ;
}

static void btcon_thread_init(void)
{
    memset(&btcon_thread, 0, sizeof(struct k_thread));
    k_thread_create(&btcon_thread, (k_thread_stack_t)btcon_thread_stack,
            sizeof(btcon_thread_stack), btcon_thread_loop,
            NULL, NULL, NULL, CONFIG_CONTROLLER_HIGH_PRIORITY, 0, K_NO_WAIT);
}

#ifdef CONFIG_ZEPHYR_STACK
static void get_mac_addr(uint8_t *mac)
{
	const u8_t *default_mac = "F44EFC112233";
	u8_t mac_str[13], i, value;
	int ret;

	ret = property_get(CFG_BT_MAC, mac_str, 12);
	if(ret < 12) {
		SYS_LOG_WRN("property_get CFG_BT_MAC ret: %d", ret);
		memcpy(mac_str, default_mac, 12);
	}

	str_to_hex(mac, mac_str, 6);

	/* Set to controller in reverse order */
	for (i=0; i<3; i++) {
		value = mac[i];
		mac[i] = mac[5 -i];
		mac[5 -i] = value;
	}

	mac_str[12] = 0;
	SYS_LOG_INF("Set bt mac: %s", mac_str);
}
#endif

#ifdef CONFIG_BT_CONTROLER_BQB
static int btdrv_get_bqb_mode(void)
{
	int bqb_mode;

#ifdef CONFIG_BT_CONTROLER_BLE_BQB
	bqb_mode = 2;
#else
	bqb_mode = property_get_int(CFG_BT_TEST_MODE, -1);
#endif

	return bqb_mode;
}

static int btdrv_init_try_enter_bqb(void)
{
	int bqb_mode;

	bqb_mode = btdrv_get_bqb_mode();

	if (bqb_mode > 0) {
		SYS_LOG_INF("btc: enter bqb %d", bqb_mode);
		k_thread_priority_set(k_current_get(), 0);

		/* disable watchdog */
		soc_watchdog_stop();

		BT_DutTest(bqb_mode);
	}

	return 0;
}
#endif

int btdrv_init(btdrv_rx_cb_t rx_cb)
{
    ctrl_cfg_t cfg;
    int err;

    SYS_LOG_INF("init");

#ifdef CONFIG_BT_SNOOP
	btsnoop_init();
#endif

    if (rx_cb == NULL)
        return -EINVAL;

    memset(&cfg, 0x0, sizeof(ctrl_cfg_t));

    get_mac_addr(cfg.bd_addr);
    btdrv_recieve_data_cbk = rx_cb;
    cfg.deliver_data_from_c2h = btdrv_recive_data;

    cfg.log_level = BT_LOG_LEVEL_DEBUG;
    cfg.log_module_mask = BT_LOG_MODULE_ALL;
    cfg.trace_module_mask = 0;

    cfg.tx_pwr_lvl = CONFIG_BT_CONTROLER_MAX_TXPOWER;
    cfg.def_pwr_lvl = 0; // 0db

#ifdef CONFIG_BT_CONTROLER_DEBUG_PRINT
    cfg.param_buf |= CTRL_PARAM_RSV_PRINT_DEBUG;
#endif

#ifdef CONFIG_BT_CONTROLER_DEBUG_GPIO
    cfg.param_buf |= CTRL_PARAM_RSV_GPIO_DEBUG;
#endif

#ifdef CONFIG_BT_CONTROLER_BQB
    if (btdrv_get_bqb_mode() > 0) {
        cfg.param_buf |= CTRL_PARAM_RSV_BQB_MODE;
    }
#endif

#ifdef CONFIG_BT_CONTROLER_BLE_BQB
    cfg.param_buf &= ~CTRL_PARAM_RSV_GPIO_DEBUG;
    cfg.param_buf |= CTRL_PARAM_RSV_UART_TRANS;
#endif

    cfg.rt_sco_out_cbk = btsrv_sco_data_out_cb;
    cfg.rt_sco_in_cbk = btsrv_sco_data_in_cb;

    err = ctrl_init(&cfg);
    if (err) {
        return err;
    }

    btcon_thread_init();

#ifdef CONFIG_BT_CONTROLER_BQB
    btdrv_init_try_enter_bqb();
#endif

    return 0;
}

void btdrv_exit(void)
{
    SYS_LOG_INF("exit");

    ctrl_deinit();
    btdrv_recieve_data_cbk = NULL;
}
