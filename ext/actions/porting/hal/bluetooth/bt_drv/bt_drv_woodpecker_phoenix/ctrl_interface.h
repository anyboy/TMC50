/*
 * Copyright(c) 2017-2027 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * Johnny           2017-5-4        create initial version
 */

#ifndef __CTRL_INTERFACE_H
#define __CTRL_INTERFACE_H

struct rt_sco_data {
	uint16_t handle;
	uint8_t len;
	uint8_t flag;
	uint8_t *buf;
	uint32_t anchor_clk;
};
/*******************************
 * type definitions
 *******************************/
typedef struct {
    /* local bluetooth address */
    unsigned char bd_addr[6];
    /* callback for controller sending data to host */
    signed int (*deliver_data_from_c2h)(unsigned char type, const unsigned char *buf);
    /* RF maxinum tx power level */
    signed short tx_pwr_lvl;
    /* Page\Inquire\Scan\LE default tx power level */
    signed short def_pwr_lvl;
    /* controller parameters buffer address */
    unsigned int param_buf;

    unsigned char log_level;
    unsigned char reserved[3];
    unsigned int log_module_mask;
    unsigned int trace_module_mask;

    signed int (*rt_sco_out_cbk)(struct rt_sco_data *rsd);
    signed int (*rt_sco_in_cbk)(struct rt_sco_data *rsd);

    /* reserve fot future used */
    void *rsv;
} ctrl_cfg_t;

#define CTRL_PARAM_RSV_MASK                 0xBFCE03E0
#define CTRL_PARAM_RSV_GPIO_DEBUG           0x00000010
#define CTRL_PARAM_RSV_BQB_MODE             0x00000020
#define CTRL_PARAM_RSV_PRINT_DEBUG          0x00000100
#define CTRL_PARAM_RSV_UART_TRANS           0x00001000
#define CTRL_PARAM_RSV_EDR3M_DISABLE        0x00010000
#define CTRL_PARAM_RSV_FIX_MAX_PWR          0x00100000

typedef struct bt_clock
{
    unsigned int bt_clk;  /* by 312.5us. the bit0 and bit1 are 0.  range is 0 ~ 0x0FFFFFFF.  */
    unsigned short bt_intraslot;  /* by 1us.  range is 0 ~ 1249.  */
} t_devicelink_clock;
/*******************************
 * function declaration
 *******************************/
signed int ctrl_init(ctrl_cfg_t *p_param);
signed int ctrl_deinit(void);
signed int ctrl_deliver_data_from_h2c(unsigned char type, const unsigned char *buf);
/* controller thread loop, return 0 means idle and 1 means busy */
signed int ctrl_loop(void);

/* get current bt clock, accuracy is 1us.  */
unsigned int __attribute__((long_call)) ctrl_get_bt_clk(unsigned short acl_handle, t_devicelink_clock *bt_clock);
/* set bt clock to be informed.  */
unsigned int __attribute__((long_call)) ctrl_set_bt_clk(unsigned short acl_handle, t_devicelink_clock bt_clock);
/* adjust link time.  adjust_val---range -36 ~ 96,  by 2.  */
void ctrl_adjust_link_time(unsigned short acl_handle, signed short adjust_val);
/* get null package count by controller for sync link after last call */
unsigned short ctrl_read_add_null_cnt(unsigned short sco_handle);
/* set flow-control switcher for acl link 2cap */
void ctrl_set_user_data_rx_flow(unsigned short acl_handle, unsigned char stop);

/* print log level */
#define BT_LOG_LEVEL_NONE		(0)
#define BT_LOG_LEVEL_ERROR		(1)
#define BT_LOG_LEVEL_WARNING		(2)
#define BT_LOG_LEVEL_INFO		(3)
#define BT_LOG_LEVEL_DEBUG		(4)

unsigned int ctrl_get_log_level(void);
void ctrl_set_log_level(unsigned int log_level);

/* log module for dump data */
#define BT_LOG_MODULE_LMP_TX		(1u << 0)
#define BT_LOG_MODULE_LMP_RX		(1u << 1)
#define BT_LOG_MODULE_LL_TX		(1u << 2)
#define BT_LOG_MODULE_LL_RX		(1u << 3)
#define BT_LOG_MODULE_HCI_TX		(1u << 4)
#define BT_LOG_MODULE_HCI_RX		(1u << 5)
#define BT_LOG_MODULE_ALL		(0xffffffff)

unsigned int ctrl_get_log_module_mask(void);
void ctrl_set_log_module_mask(unsigned int log_module_mask);

unsigned int ctrl_get_trace_module_mask(void);
void ctrl_set_trace_module_mask(unsigned int trace_module_mask);

unsigned int ctrl_mem_dump_info(void);

#endif

