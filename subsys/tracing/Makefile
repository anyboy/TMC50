ZEPHYRINCLUDE += -I$(srctree)/subsys/tracing/include

obj-$(CONFIG_TRACING_CPU_STATS) += cpu_stats.o

obj-$(CONFIG_TRACING_CORE) += tracing_buffer.o tracing_core.o tracing_format_common.o
obj-$(CONFIG_TRACING_SYNC) += tracing_format_sync.o
obj-$(CONFIG_TRACING_ASYNC) += tracing_format_async.o

obj-$(CONFIG_TRACING_BACKEND_USB) += tracing_backend_usb.o
obj-$(CONFIG_TRACING_BACKEND_UART) += tracing_backend_uart.o
obj-$(CONFIG_TRACING_BACKEND_POSIX) += tracing_backend_posix.o
obj-$(CONFIG_TRACING_BACKEND_HEXDUMP) += tracing_backend_hexdump.o
obj-$(CONFIG_TRACING_BACKEND_SPI) += tracing_backend_spi.o

obj-$(CONFIG_TRACING_CTF) += ctf/
obj-$(CONFIG_SEGGER_SYSTEMVIEW) += sysview/
obj-$(CONFIG_TRACING_TEST) += tracing_string_format_test.o
obj-$(CONFIG_TRACING_ATRACE) += atrace/
