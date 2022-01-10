#include <uart.h>
#include <usp_protocol.h>
#include <misc/printk.h>
#include <string.h>
#include <logging/sys_log.h>
#include <cbuf.h>

#if 0
#define print_hex(str, buf, size) do {printk("%s\n", str); print_buffer(buf, 1, size, 16, 0);} while(0)
//#define print_hex(...) do {} while(0)
#define DEBUG_PRINT SYS_LOG_INF
#define ERROR_PRINT SYS_LOG_ERR
#else
#define print_hex(...) do {} while(0)
#define DEBUG_PRINT(...) do {} while(0)
#define ERROR_PRINT(...) do {} while(0)
#endif

static volatile bool data_transmitted;
static volatile bool data_arrived;
static u8_t data_buf[128];
static cbuf_t data_cbuf;

static void uart_interrupt_handler(struct device *dev)
{
	int read;
	int store;
	u8_t buf[64];

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (uart_irq_rx_ready(dev)) {
		data_arrived = true;
		read = uart_fifo_read(dev, buf, sizeof(buf));
		store = cbuf_write(&data_cbuf, buf, read);
		if (read != store) {
			ERROR_PRINT("cbuf write error, %d/%d/%d/%d",
					store,
					read,
					cbuf_get_free_space(&data_cbuf),
					cbuf_get_capacity(&data_cbuf)
					);
		}
	}
}

static int read_data(struct device *dev, u8_t* buf, int len, u32_t timeout_ms)
{
	int read;
#if 0
	u32_t start_ts;
#else
	int count=0;
#endif

	if (NULL == buf){
		SYS_LOG_DBG("Discard buffer data, len = %d", cbuf_get_used_space(&data_cbuf));
		cbuf_reset(&data_cbuf);
		return len;
	}

	if (timeout_ms > 0)
	{
#if 0
                start_ts = k_uptime_get_32();
#endif
		while (len > cbuf_get_used_space(&data_cbuf))
		{
			k_sleep(1);
#if 0
			if ((k_uptime_get_32() - start_ts) > timeout_ms){
				ERROR_PRINT("Timeout error, len=%d, buffered=%d, timeout=%d", len, cbuf_get_used_space(&data_cbuf), timeout_ms);
				read = cbuf_read(&data_cbuf, buf, cbuf_get_used_space(&data_cbuf));
				if(read>0){
					print_hex("read data: ", buf, read);
				}
				return 0;
			}
#else
			count++;
			if(count>timeout_ms){
				//Do not print error when do 1 byte tring read.
				if(len>1) {
					SYS_LOG_INF("Timeout error, len=%d, buffered=%d, timeout=%d", len, cbuf_get_used_space(&data_cbuf), timeout_ms);
					read = cbuf_read(&data_cbuf, buf, cbuf_get_used_space(&data_cbuf));
					if(read>0){
						print_hex("read data: ", buf, read);
					}
				}
				return 0;
			}
#endif
		}
	}

	read = cbuf_read(&data_cbuf, buf, len);
	if(read>0){
		print_hex("read data: ", buf, read);
	}

	return read;
}

static int write_data(struct device *dev, const u8_t* buf, int len)
{
	int written;

	//uart_irq_tx_enable(dev);

	//print_hex("write data: ", buf, len);
	while (len) {
		//data_transmitted = false;
		written = uart_fifo_fill(dev, buf, len);
#if 0
		while (data_transmitted == false) {
			k_sleep(1);
		}
#endif

		len -= written;
		buf += written;
	}

	//uart_irq_tx_disable(dev);

	return  len;
}

static int uart_isr_register(void)
{
	struct device *dev;

	dev = device_get_binding(CONFIG_UART_ACTS_PORT_1_NAME);
	if (!dev) {
		ERROR_PRINT("UART device not found.");
		return -1;
	}

	uart_irq_callback_set(dev, uart_interrupt_handler);
	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	return 0;
}

static int uart_isr_unregister(void)
{
	struct device *dev;

	dev = device_get_binding(CONFIG_UART_ACTS_PORT_1_NAME);
	if (!dev) {
		ERROR_PRINT("UART device not found.");
		return -1;
	}

	uart_irq_rx_disable(dev);
	uart_irq_callback_set(dev, NULL);

	return 0;
}

int usp_uart_read(u8_t * data, u32_t length, u32_t timeout_ms)
{
	struct device *uart1_dev;

	uart1_dev = device_get_binding(CONFIG_UART_ACTS_PORT_1_NAME);
	if (NULL != uart1_dev) {
		return read_data(uart1_dev, data, length, timeout_ms);
	}

	return -1;
}

int usp_uart_write(u8_t * data, u32_t length, u32_t timeout_ms)
{
	struct device *uart1_dev;

	uart1_dev = device_get_binding(CONFIG_UART_ACTS_PORT_1_NAME);
	if (NULL != uart1_dev) {
		return write_data(uart1_dev, data, length);
	}

	return -1;
}

int usp_uart_ioctl(u32_t mode, void *para1, void *para2)
{
	int ret = 0;

	struct device *uart1_dev;

	switch (mode) {
	case USP_IOCTL_SET_BAUDRATE:
		uart1_dev = device_get_binding(CONFIG_UART_ACTS_PORT_1_NAME);
		if (NULL == uart1_dev) {
			return -1;
		}
		DEBUG_PRINT("To set baudrate %d bps.", (u32_t)para1);
		ret = uart_baud_rate_set(uart1_dev, (u32_t)para1);
		break;

	case USP_IOCTL_CHECK_WRITE_FINISH:
#if 0
		if (!UART_TxDone()) {
			ret = -1;
		}
#endif
		ret = 1;
		break;

	case USP_IOCTL_ENTER_WRITE_CRITICAL:
#if 0
		if (TRUE == (bool)para1) {
			uart_print_enable(FALSE);
		} else {
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

int usp_phy_init(usp_phy_interface_t * phy)
{
	int ret;

	DEBUG_PRINT("To init cbuf.");
	cbuf_init(&data_cbuf, data_buf, sizeof(data_buf));

	ret = uart_isr_register();
	phy->read = usp_uart_read;
	phy->write = usp_uart_write;
	phy->ioctl = usp_uart_ioctl;

	return ret;
}

int usp_phy_deinit(void)
{
	int ret;

	ret = uart_isr_unregister();
	cbuf_deinit(&data_cbuf);
	return ret;
}

