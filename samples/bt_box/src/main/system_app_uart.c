#include <zephyr/types.h>
#include <drivers/console/uart_pipe.h>
#include "system_app.h"

#define UART_TEMP_BUF_SIZE 10
static u8_t uart_temp_buf[UART_TEMP_BUF_SIZE];

u8_t* system_app_uart_rx_cb(u8_t *buf, size_t *off)
{
	return buf;
}

void system_app_uart_init(void)
{
	uart_pipe_register(uart_temp_buf, UART_TEMP_BUF_SIZE, system_app_uart_rx_cb);
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
