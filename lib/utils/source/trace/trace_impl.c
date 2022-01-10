/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file trace impl interface
 */


#include <init.h>
#include <kernel.h>
#include <cbuf.h>
#include <trace.h>
#include <trace_impl.h>
#include <ext_types.h>
#ifdef CONFIG_TRACE_UART
#include <dma.h>
#include <drivers/console/uart_console.h>
#endif
#ifdef CONFIG_TRACE_FILE
#include <trace_file.h>
#endif
#include <soc.h>
#include <esd_manager.h>
#include <string.h>

#include <os_common_api.h>

#if defined(CONFIG_USB_UART_CONSOLE)
#include <drivers/console/uart_usb.h>
#endif

extern void uart_puts(char *s, int len);

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#else
#define __stdout_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#else
#define __printk_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

#define LINESEP_FORMAT_WINDOWS  0

#define LINESEP_FORMAT_UNIX     1

trace_ctx_t g_trace_ctx;

static unsigned char trace_print_buffer[TRACE_PRINT_BUF_SIZE];

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
static unsigned char temp_buf[1]; /* trace out no used */
#else
static unsigned char temp_buf[TRACE_TEMP_BUF_SIZE];
#endif

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
/* u8_t __attribute__((section (".bss.rom_dependent_debug_buf"))) rom_printhex_debug_buf[0x80]; */
#else
static unsigned char hex_buf[TRACE_HEX_BUF_SIZE];
#endif

static int temp_index;

extern char *bin2hex(char *dst, const void *src, unsigned long count);

#ifdef CONFIG_TRACE_EVENT
static unsigned char binary_buffer[CONFIG_TRACE_EVENT_BUFFERSIZE];
#endif

extern int _arch_exception_vector_read(void);

extern unsigned int get_PSR(void);

#ifdef CONFIG_TRACE_USE_ROM_CODE
int rom_vsnprintf(char* buf, size_t size, unsigned int type, const char* fmt, va_list args);
#else
int trace_vsnprintf(char* buf, size_t size, unsigned int type, const char* fmt, va_list args);
#endif

void trace_sync(void);

int hal_trace_check_busy(trace_ctx_t *trace_ctx)
{
    int trace_busy = 0;

#ifdef CONFIG_TRACE_UART
    if(trace_transport_support(trace_ctx, TRACE_TRANSPORT_UART)){
        trace_busy |= uart_console_is_busy();
    }
#endif

    return trace_busy;
}

static void trace_putc(int c)
{
    if(temp_index < sizeof(temp_buf)){
	    temp_buf[temp_index] = (char)c;
	    temp_index++;
	}
}

static int trace_out(int c)
{
	if ('\n' == c) {
		trace_putc('\r');
	}
	trace_putc(c);

	return 0;
}


void hal_trace_output(char *buf, int len)
{
#ifdef CONFIG_TRACE_UART
    uart_console_output(buf, len);
#endif

#ifdef CONFIG_TRACE_FILE
    hal_trace_file_output(buf, len);
#endif
}

int hal_trace_need_sync(trace_ctx_t *trace_ctx)
{
#ifndef CONFIG_MIPS
    int vec;
#endif
    int need_sync = 0;

    if(!trace_ctx->init || trace_ctx->panic){
        need_sync = 1;
    }else{
#ifndef CONFIG_MIPS
        vec = _arch_exception_vector_read();

        if(vec != 0 && vec != 22 && vec < 32){
            //used sync mode when in exception
            need_sync = 1;
        }
#endif
    }

    return need_sync;
}

static int trace_start_tx(trace_ctx_t *trace_ctx)
{
	int32_t data_size;

	if(!trace_ctx->sending){
        data_size = cbuf_get_used_space(&trace_ctx->cbuf);

        if(data_size > 0){
            cbuf_dma_read(&trace_ctx->cbuf, &trace_ctx->dma_setting, (uint32_t)data_size);
            if(trace_ctx->dma_setting.read_len){
                trace_ctx->sending = TRUE;
                hal_trace_output(trace_ctx->dma_setting.start_addr, trace_ctx->dma_setting.read_len);
                return 1;
            }
        }

#ifdef CONFIG_TRACE_EVENT
        data_size = cbuf_get_used_space(&trace_ctx->cbuf_binary);

        if(data_size > 0){
            if(trace_ctx->dma_setting.read_len){
                cbuf_dma_read(&trace_ctx->cbuf_binary, &trace_ctx->dma_setting, (uint32_t)data_size);
                trace_ctx->sending = TRUE;
                hal_trace_output(trace_ctx->dma_setting.start_addr, trace_ctx->dma_setting.read_len);

                return 1;
            }
        }
#endif
        return 0;
    }

	return 1;
}


static void trace_stop_tx(trace_ctx_t *trace_ctx)
{
    if (trace_ctx->dma_setting.read_len != 0)
    {
        if(cbuf_is_ptr_valid(&trace_ctx->cbuf, (uint32_t)trace_ctx->dma_setting.start_addr)){
            cbuf_dma_update_read_ptr(&trace_ctx->cbuf, trace_ctx->dma_setting.read_len);
        }
#ifdef CONFIG_TRACE_EVENT
        else{
            cbuf_dma_update_read_ptr(&trace_ctx->cbuf_binary, trace_ctx->dma_setting.read_len);
        }
#endif
        trace_ctx->dma_setting.read_len = 0;
        trace_ctx->sending = FALSE;
    }
}

void trace_tx_dispatch(int msg_id)
{
    trace_ctx_t *trace_ctx = get_trace_ctx();

    switch(msg_id){
        case TRACE_EVENT_TX:
        trace_start_tx(trace_ctx);
        break;

        case TRACE_EVENT_TX_COMPLETED:
        trace_stop_tx(trace_ctx);
        trace_start_tx(trace_ctx);
        break;
    }
}

void trigger_trace_task(uint8_t msg_id)
{
    trace_ctx_t *trace_ctx = get_trace_ctx();

    if(msg_id == TRACE_EVENT_TX){
        if(trace_ctx->sending || hal_trace_need_sync(trace_ctx)){
            return;
        }
    }

#ifdef TRACE_TIME_FREFIX
    uart_console_output_trigger(msg_id);
#else
    trace_tx_dispatch(msg_id);
#endif
}

int trace_send_prepare(trace_ctx_t *trace_ctx)
{
    uint32_t cur_len;

    if(trace_ctx->sending){
        return EBUSY;
    }

    cur_len = cbuf_get_used_space(&(trace_ctx->cbuf));
    if(cur_len == 0){
        return ENOMEM;
    }

    //show full , means same print info drop 
    if ((trace_ctx->drop_bytes > 0) && (cbuf_get_free_space(&trace_ctx->cbuf) >= 8)){
        char tmp_buf[8];

        tmp_buf[0] = '\n';
        tmp_buf[1] = '@';

        trace_ctx->drop_bytes = trace_ctx->drop_bytes % 10000;
        tmp_buf[2] = '0' + trace_ctx->drop_bytes/1000;
        trace_ctx->drop_bytes = trace_ctx->drop_bytes % 1000;
        tmp_buf[3] = '0' + trace_ctx->drop_bytes/100;
        trace_ctx->drop_bytes = trace_ctx->drop_bytes % 100;
        tmp_buf[4] = '0' + trace_ctx->drop_bytes/10;
        trace_ctx->drop_bytes = trace_ctx->drop_bytes % 10;
        tmp_buf[5] = '0' + trace_ctx->drop_bytes;
        tmp_buf[6] = '@';
        tmp_buf[7] = '\n';

        cbuf_write(&trace_ctx->cbuf, tmp_buf, 8);

        trace_ctx->drop_bytes = 0;
    }

    return 0;
}


void trace_send_isr(struct device *dev, u32_t priv_data, int reson)
{
#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
    if(reson == 0){
        BROM_TRACE_DMA_TC_ISR();
    }
#else
    if(reson == 0){
        trigger_trace_task(TRACE_EVENT_TX_COMPLETED);
    }
#endif
}


void trace_output(const unsigned char *buf, unsigned int len, trace_ctx_t *trace_ctx)
{
    if(trace_ctx->trace_mode == TRACE_MODE_DMA){
#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
#if defined(CONFIG_USB_UART_CONSOLE)
		uart_usb_send((u8_t *)buf, len);
#else
		int irq_flag = irq_lock();
		BROM_TRACE_OUTPUT((u8_t *)buf, len);
		irq_unlock(irq_flag);
#endif
#else
        //avoid triggle exception cause nested call 
        if(!trace_ctx->in_trace){
    		int free_space;
            trace_ctx->in_trace = TRUE;

            free_space = cbuf_get_free_space(&trace_ctx->cbuf);

            if(len > free_space){
                trace_ctx->drop_bytes += len - free_space;
                len = free_space;
            }

            irq_flag = irq_lock();

            cbuf_write(&trace_ctx->cbuf, (void *)buf, len);

            if(trace_send_prepare(trace_ctx) == 0){
                trigger_trace_task(TRACE_EVENT_TX);
            }

            irq_unlock(irq_flag);

            trace_ctx->in_trace = FALSE;
        }else{
			//nested call
            trace_ctx->nested = TRUE;
        }
#endif
    }else{
        hal_trace_output((char *)buf, len);
    }
}

void trace_hexdump(const char* prefix, const uint8_t* buf, uint32_t len)
{
#ifdef CONFIG_PRINTK

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)

	BROM_PRINTHEX((char *)prefix, (void *)buf, len);

#else

    uint32_t n, cnt, total_cnt;
    uint32_t flags;
    uint8_t *hex_ptr;
    uint32_t free_space;
    trace_ctx_t *trace_ctx = get_trace_ctx();

#ifdef CONFIG_TRACE_PERF_ENABLE
    uint32_t run_time = _timer_cycle_get_32();
#endif  

	ARG_UNUSED(prefix);

    flags = irq_lock();

    hex_ptr = hex_buf;
    cnt = 0;
    total_cnt = 0;
    n = 0;
   
    free_space = cbuf_get_free_space(&trace_ctx->cbuf);
    
    while(len != 0) {
        hex_ptr = bin2hex(hex_ptr, (const void *)buf, 1);
        buf++;
        *hex_ptr = ' ';
        hex_ptr++;
        n++;
        cnt += 3;
        len--;
        
        if (n % 16 == 0){
            *hex_ptr = '\n';
            hex_ptr++;
            cnt++;
        }

        total_cnt += cnt;

        if(total_cnt > free_space){
            break;
        }        

        if(cnt > TRACE_TEMP_BUF_SIZE){
            trace_output(hex_buf, cnt, trace_ctx);
            free_space = cbuf_get_free_space(&trace_ctx->cbuf);
            hex_ptr = hex_buf;
            cnt = 0;           
        }
    }

    *hex_ptr = '\n';
    cnt++;    

    trace_output(hex_buf, cnt, trace_ctx);

#ifdef CONFIG_TRACE_PERF_ENABLE  
    run_time = _timer_cycle_get_32() - run_time;

    if(trace_ctx->hex_time < run_time){
        trace_ctx->hex_time  = run_time;
    }
#endif    

    irq_unlock(flags);

#endif

#else //undef CONFIG_PRINTK
	ARG_UNUSED(prefix);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
#endif
}

void trace_transport_onoff(uint8_t transport_type, uint8_t is_on)
{
    trace_ctx_t *trace_ctx = get_trace_ctx();

    if(is_on){
        trace_ctx->transport |= transport_type;
    }else{
        trace_ctx->transport &= (~transport_type);
    }
}

int trace_mode_set(unsigned int trace_mode)
{
    int old_trace_mode;
#ifdef CONFIG_SYS_IRQ_LOCK
	SYS_IRQ_FLAGS irq_flag;
	sys_irq_lock(&irq_flag);
#endif

    trace_ctx_t *trace_ctx = get_trace_ctx();

    old_trace_mode = trace_ctx->trace_mode;

    if(trace_mode != old_trace_mode && old_trace_mode == TRACE_MODE_DMA){
        trace_sync();
    }

    trace_ctx->trace_mode = trace_mode;

    if (trace_mode == TRACE_MODE_DMA) {
#ifdef CONFIG_UART_CONSOLE
#ifdef CONFIG_TRACE_UART
        uart_console_dma_switch(1, trace_send_isr, trace_ctx);

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
        BROM_TRACE_SET_MODE(TRACE_MODE_DMA, CONFIG_UART_CONSOLE_DMA_CHANNEL, &(trace_ctx->cbuf));
#endif

#endif
#endif
        __printk_hook_install(trace_out);
        __stdout_hook_install(trace_out);

    } else if (trace_mode == TRACE_MODE_CPU) {
#ifdef CONFIG_UART_CONSOLE
#ifdef CONFIG_TRACE_UART
        uart_console_dma_switch(0, 0, 0);

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
        BROM_TRACE_SET_MODE(TRACE_MODE_CPU, 0, NULL);
#endif

#endif
#endif
    }

#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_unlock(&irq_flag);
#endif

    return old_trace_mode;
}

int trace_irq_print_set(unsigned int irq_enable)
{
    int old_irq_mode;

    trace_ctx_t *trace_ctx = get_trace_ctx();

    old_irq_mode = trace_ctx->irq_print_disable;

    trace_ctx->irq_print_disable = irq_enable;

    return old_irq_mode;
}

int trace_dma_print_set(unsigned int dma_enable)
{
	int old_dma_enable;

	trace_ctx_t *trace_ctx = get_trace_ctx();

	old_dma_enable = !trace_ctx->dma_print_disable;

	if (dma_enable == false)
		trace_ctx->dma_print_disable = 1;
	else
		trace_ctx->dma_print_disable = 0;

	return old_dma_enable;
}

void trace_sync(void)
{
    int lock;

    trace_ctx_t *trace_ctx = get_trace_ctx();

	// lock irq avoid print nested call in isr context, maybe cause lock irq too long. 
    lock = irq_lock();

	// output all cache data 
    while(1){
	// wait transmit finished 
        if(hal_trace_check_busy(trace_ctx) == 0){
           trace_stop_tx(trace_ctx);
        }else{
           continue;
        }

        if(!trace_start_tx(trace_ctx)){
            break;
        }
    }

    irq_unlock(lock);
}

void trace_monitor(trace_ctx_t *trace_ctx)
{
#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
	/* BROM Trace Need't support nested */
#else

    int out_len;
    
    if(trace_ctx->nested){
		out_len = snprintk(temp_buf, sizeof(temp_buf), "trace nested\n");  

		trace_output(temp_buf, out_len, trace_ctx);
        trace_ctx->nested = 0;
        trace_ctx->in_trace = 0;
    }

    if(trace_ctx->caller){
		out_len = snprintk(temp_buf, sizeof(temp_buf), "trace when irq disable %x\n", \
		    trace_ctx->caller);     
        trace_output(temp_buf, out_len, trace_ctx);
        trace_ctx->caller = 0;
    }

#endif
}

#ifdef CONFIG_TRACE_TIME_FREFIX

static uint8_t digits[] = "0123456789abcdef";

static void get_time_prefix(char *num_str)
{
    uint32_t number;
	uint8_t num_len, ch;

	number = k_cycle_get_32() / 24;

	num_str[13] = ' ';
	num_str[12] = ']';
	num_len = 11;
	while(number != 0)
	{
		ch = digits[number % 10];
		num_str[num_len--] = ch;
		number /= 10;
	}
	ch = num_len;
	while(ch > 0)
		num_str[ch--] = ' ';
	num_str[0] = '[';
	while(++num_len <= 8)
		num_str[num_len - 1] = num_str[num_len];
	num_str[8] = '.';
}
#endif

#ifdef CONFIG_PRINTK

extern int __vprintk(const char *fmt, va_list ap);

int vprintk(const char *fmt, va_list args)
{
	int ret;

	char *p_buf;
	char trans_buffer[TRACE_TEMP_BUF_SIZE];

    p_buf = trans_buffer;

    trace_ctx_t *trace_ctx = get_trace_ctx();

    if(!trace_ctx->init){
#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
        ret = BROM_TRACE_VSNPRINTF(trans_buffer, TRACE_TEMP_BUF_SIZE, LINESEP_FORMAT_WINDOWS, fmt, args);
#if defined(CONFIG_USB_UART_CONSOLE)
		uart_usb_send(trans_buffer, ret);
#else
        int lock = irq_lock();
        BROM_TRACE_OUTPUT(trans_buffer, ret);
        irq_unlock(lock);
#endif
#else
        ret = __vprintk(fmt, args);
#endif
        return ret;
    }

    trace_monitor(trace_ctx);

    if(trace_ctx->irq_print_disable){
        if(k_is_in_isr()){
            return 0;
        }
    }

    if (trace_ctx->dma_print_disable == 0) {
        if (hal_trace_need_sync(trace_ctx)) {
            trace_mode_set(TRACE_MODE_CPU);
        } else {
            if (trace_ctx->trace_mode != TRACE_MODE_DMA)
                trace_mode_set(TRACE_MODE_DMA);
        }
    }

#ifdef CONFIG_TRACE_PERF_ENABLE
    uint32_t run_time = _timer_cycle_get_32();

    uint32_t flags = irq_lock();
#endif 

#ifdef CONFIG_TRACE_TIME_FREFIX
    get_time_prefix(trans_buffer);
    p_buf += 14;

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
    ret = BROM_TRACE_VSNPRINTF(p_buf, TRACE_TEMP_BUF_SIZE - 14, LINESEP_FORMAT_WINDOWS, fmt, args);
#else
#ifdef CONFIG_TRACE_USE_ROM_CODE
    ret = rom_vsnprintf(p_buf, TRACE_TEMP_BUF_SIZE - 14, LINESEP_FORMAT_WINDOWS, fmt, args);
#else
    ret = trace_vsnprintf(p_buf, TRACE_TEMP_BUF_SIZE - 14, LINESEP_FORMAT_WINDOWS, fmt, args);
#endif
#endif

    ret += 14;
#else    

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
    ret = BROM_TRACE_VSNPRINTF(p_buf, TRACE_TEMP_BUF_SIZE, LINESEP_FORMAT_WINDOWS, fmt, args);
#else
#ifdef CONFIG_TRACE_USE_ROM_CODE
    ret = vsnprintf(p_buf, TRACE_TEMP_BUF_SIZE, LINESEP_FORMAT_WINDOWS, fmt, args);
#else
    ret = trace_vsnprintf(p_buf, TRACE_TEMP_BUF_SIZE - 14, LINESEP_FORMAT_WINDOWS, fmt, args);
#endif
#endif

#endif

#ifdef CONFIG_TRACE_PERF_ENABLE  
    if(trace_ctx->convert_time < (_timer_cycle_get_32() - run_time)){
        trace_ctx->convert_time  = _timer_cycle_get_32() - run_time;
    }
#endif

    trace_output(trans_buffer, ret, trace_ctx);

#ifdef CONFIG_TRACE_PERF_ENABLE  
    run_time = _timer_cycle_get_32() - run_time;

    if(trace_ctx->print_time < run_time){
        trace_ctx->print_time  = run_time;
    }

    irq_unlock(flags);
#endif

	return ret;
}

#endif

void trace_set_panic(void)
{
    trace_ctx_t *trace_ctx = get_trace_ctx();

    trace_ctx->panic = 1;

    return;
}

void trace_clear_panic(void)
{
    trace_ctx_t *trace_ctx = get_trace_ctx();

    trace_ctx->panic = 0;

    return;
}

void trace_assert(ASSERT_PARAM)
{
	va_list ap;

    trace_set_panic();
#if defined(CONFIG_ASSERT_SHOW_FILE_FUNC)
    printk("ASSERTION FAIL [%s] @ %s:%s:%d:\n\t", test, file, func, line);
#elif  defined(CONFIG_ASSERT_SHOW_FILE) || defined(CONFIG_ASSERT_SHOW_FUNC)
    printk("ASSERTION FAIL [%s] @ %s:%d:\n\t", test, str, line);
#else
    printk("ASSERTION FAIL [%s] @ %d:\n\t", test, line);
#endif

    va_start(ap, fmt);
    vprintk(fmt, ap);
    va_end(ap);

    panic(0);
}


#ifdef CONFIG_ESD_SUPPORT

#if (0) /* if want to support esd print buf, must store cbuf entity at esd data */

static const char esd_tips[] = "\n\n--------ESD-------\n\n";

//If a restart is caused by ESD or unknown reasons, the ringbuffer retains data that may not have been printed before 
//You can print out the information before the crash at startup
 
static void trace_puts_esd_print_buf(void)
{
    int flags;

    int esd_data_valid = FALSE;

	flags = irq_lock();

    trace_ctx_t *trace_ctx = get_trace_ctx();

#ifdef CONFIG_TRACE_EVENT
    if(((trace_ctx->cbuf.length != 0 && trace_ctx->cbuf.length <= TRACE_PRINT_BUF_SIZE))
        || ((trace_ctx->cbuf_binary.length != 0 && trace_ctx->cbuf_binary.length <= CONFIG_TRACE_EVENT_BUFFERSIZE))){
#else
    if((trace_ctx->cbuf.length != 0 && trace_ctx->cbuf.length <= TRACE_PRINT_BUF_SIZE)){
#endif
        esd_data_valid = TRUE;

        uart_puts((char *)esd_tips, sizeof(esd_tips));
    }

    while (trace_ctx->cbuf.length != 0 && trace_ctx->cbuf.length <= TRACE_PRINT_BUF_SIZE) {
        if (cbuf_dma_read(&(trace_ctx->cbuf), &trace_ctx->dma_setting, trace_ctx->cbuf.length) == 0)
            break;

        uart_puts(trace_ctx->dma_setting.start_addr, trace_ctx->dma_setting.read_len);

        cbuf_dma_update_read_ptr(&(trace_ctx->cbuf), trace_ctx->dma_setting.read_len);
    }

#ifdef CONFIG_TRACE_EVENT
    while (trace_ctx->cbuf_binary.length != 0 && trace_ctx->cbuf_binary.length <= CONFIG_TRACE_EVENT_BUFFERSIZE) {
        if (cbuf_dma_read(&(trace_ctx->cbuf_binary), &trace_ctx->dma_setting, trace_ctx->cbuf_binary.length) == 0)
            break;

        uart_puts(trace_ctx->dma_setting.start_addr, trace_ctx->dma_setting.read_len);

        cbuf_dma_update_read_ptr(&(trace_ctx->cbuf_binary), trace_ctx->dma_setting.read_len);
    }
#endif

    if(esd_data_valid){
        uart_puts((char *)esd_tips, sizeof(esd_tips));
    }

	irq_unlock(flags);
}

#endif

#endif

void trace_init(void)
{
    trace_ctx_t *trace_ctx = get_trace_ctx();

    if(trace_ctx->init){
        return;
    }


#ifdef CONFIG_ESD_SUPPORT
#if (0)
    if (esd_manager_check_esd())
        trace_puts_esd_print_buf();
#endif
#endif

    cbuf_init(&trace_ctx->cbuf, trace_print_buffer, TRACE_PRINT_BUF_SIZE);

#ifdef CONFIG_TRACE_EVENT
    trace_binary_init(&trace_ctx->cbuf_binary, binary_buffer, CONFIG_TRACE_EVENT_BUFFERSIZE);
#endif

    trace_transport_onoff(TRACE_TRANSPORT_UART, 1);

    if (trace_ctx->dma_print_disable == 0) {
        trace_mode_set(TRACE_MODE_DMA);
    } else {
        trace_mode_set(TRACE_MODE_CPU);
    }

    __printk_hook_install(trace_out);

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
    BROM_TRACE_SET_MODE(TRACE_MODE_DMA, CONFIG_UART_CONSOLE_DMA_CHANNEL, &(trace_ctx->cbuf));
#endif

    trace_ctx->init = TRUE;
}

#ifdef CONFIG_TRACE_PERF_ENABLE
void trace_profile_collect(void)
{
    trace_ctx_t *trace_ctx = get_trace_ctx();

    printk("max print time %d(us)\n", trace_ctx->print_time / 24);
    printk("max convert time %d(us)\n", trace_ctx->convert_time / 24);
    printk("max hexdump time %d(us)\n", trace_ctx->hex_time / 24);

    trace_ctx->print_time = 0;
    trace_ctx->hex_time = 0;
    trace_ctx->convert_time = 0;
}
#endif

//SYS_INIT(trace_init, APPLICATION, CONFIG_UART_CONSOLE_INIT_PRIORITY);

