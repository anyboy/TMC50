#include <zephyr/types.h>
#include <drivers/console/uart_pipe.h>
#include "system_app.h"
#include <msg_manager.h>
#include "btmusic.h"
#include "btcall.h"
#include <kernel.h>

#define UART_TEMP_BUF_SIZE 10
//fifo -> uart_temp_buf
static u8_t uart_temp_buf[UART_TEMP_BUF_SIZE];
//vol
static s8_t media_vol = 0xf0;

static os_delayed_work uart_run_delaywork;

/**
 * @brief 
 * 
 * @param vol 
 */
static void _do_post_vol_set_cmd(os_work *work)
{
	struct app_msg new_msg = { 0 };
	bool result;

	new_msg.type = MSG_INPUT_EVENT;

	new_msg.value = media_vol;

	if (!strcmp(APP_ID_BTCALL, app_manager_get_current_app())) {
		new_msg.cmd = MSG_BT_CALL_VOL_SET;
		result = send_async_msg(APP_ID_BTCALL, &new_msg);
	} else if (!strcmp(APP_ID_BTMUSIC, app_manager_get_current_app())) {
		new_msg.cmd = MSG_BT_PLAY_VOL_SET;
		result = send_async_msg(APP_ID_BTMUSIC, &new_msg);
	} else {

	}
}

/**
 * @brief post volume set message
 * 
 * @param vol new volume
 */
static void _post_vol_set_cmd(u8_t vol)
{
	vol -= 0x10;

	if (vol != media_vol) {
		media_vol = vol;

		//submit
		os_delayed_work_submit(&uart_run_delaywork, 0);
	}
}
/**
 * @brief 
 * 
 * @param buf 
 * @param off 
 * @return u8_t* 
 */
u8_t* system_app_uart_rx_cb(u8_t *buf, size_t *off)
{
	unsigned i, len;
	u8_t cmd;

	SYS_LOG_INF("buf %x, off %d", (unsigned)buf, (unsigned)*off);
	len = (unsigned)*off;

	for (i = 0; i < len; ++i)
	{
		cmd = buf[i];
		if ((cmd >= 0x10) && (cmd < 0x30))
		{
			//vol
			_post_vol_set_cmd(cmd);
		}
	}

	*off = 0;

	return buf;
}

/**
 * @brief init uart service
 * 
 */
void system_app_uart_init(void)
{
	uart_pipe_register(uart_temp_buf, UART_TEMP_BUF_SIZE, system_app_uart_rx_cb);

	os_delayed_work_init(&uart_run_delaywork, _do_post_vol_set_cmd);
}

/**
 * write to uart
 */
void system_app_uart_tx(u8_t *buf, size_t len)
{
	SYS_LOG_INF("dump first 16 bytes:\n");
	print_buffer(buf, 1, len, 16, 0);

	uart_pipe_send(buf, len);
}