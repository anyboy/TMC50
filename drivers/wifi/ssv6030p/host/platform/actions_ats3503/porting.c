/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include <rtos.h>
#include <porting.h>
#include <ssv_lib.h>
#include <log.h>
#include <uart.h>
#include <mem_manager.h>

#define APP_INCLUDE_WIFI_FW
#define HEAP_5_POOL_SIZE  32768

static struct device *uart_console_dev;

/*=============console==================*/
unsigned char hal_getchar(void)
{
    unsigned char data = 0;

	if (uart_console_dev == 0)
	{
		uart_console_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
		
		if (uart_console_dev)
		{
			uart_irq_rx_disable(uart_console_dev);
			uart_irq_callback_set(uart_console_dev,  NULL);
		}
	}
	
	if (uart_console_dev)
	{
		if (!uart_irq_rx_ready(uart_console_dev))
		{
			return 0;
		}
		
	    if (uart_fifo_read(uart_console_dev, &data, 1) != 1)
	    {
	        return 0;
	    }
	}
	else
	{
		LOG_PRINTF("uart_console_dev == NULL\r\n");
	}
 
    return data;
}

/*=============Memory==================*/

#ifdef OS_MEM_DEBUG
int g_os_malloc_size = 0;
int g_os_malloc_size_max = 0;
OS_APIs void *OS_MemAlloc1( unsigned int size, char *file, int line_num)
#else
OS_APIs void *OS_MemAlloc( unsigned int size )
#endif
{
    /**
        *  Platform dependent code. Please rewrite
        *  this piece of code for different system.
        */
    void * ptr = NULL;
	
#ifdef OS_MEM_DEBUG
    //ptr = k_malloc(size + 4);

#ifdef CONFIG_USE_WIFI_HEAP_MEM
    ptr = pvPortMalloc(size + 4);
#else
    ptr = mem_malloc(size + 4);
#endif
    //k_defrag();
    if (ptr)
    {
        g_os_malloc_size += size;
        if (g_os_malloc_size > g_os_malloc_size_max) g_os_malloc_size_max = g_os_malloc_size;
        *(int*)ptr = size;
        ptr = (char *)ptr + 4;
        //LOG_PRINTF("OS_MemAlloc ptr = %p(%d), file %s, line %d\r\n", ptr, size, file, line_num);
        //LOG_PRINTF("free memory = %d, max alloc = %d\r\n", (int)HEAP_5_POOL_SIZE - g_os_malloc_size, g_os_malloc_size_max);
    }
#else
#ifdef CONFIG_USE_WIFI_HEAP_MEM
    ptr = pvPortMalloc(size);
#else
    ptr = mem_malloc(size);
#endif
#endif
    if (ptr) OS_MemSET(ptr,0,size);
    else 
    {        
#ifdef OS_MEM_DEBUG
    //LOG_PRINTF("OS_MemAlloc %d failed file = %s line = %d !!!!!!!!!!!!!!\r\n", size, file ,line_num);
	//LOG_PRINTF("free memory = %d, max alloc = %d\r\n", (int)HEAP_5_POOL_SIZE - g_os_malloc_size, g_os_malloc_size_max);
#else
	LOG_PRINTF("OS_MemAlloc %d failed !!!!!!!!!!!!!!\r\n", size);
#endif
    }
    return ptr;
}

/**
 *  We do not recommend using OS_MemFree() API
 *  because we do not want to support memory
 *  management mechanism in embedded system.
 */
#ifdef OS_MEM_DEBUG 
OS_APIs void __OS_MemFree1( void *m, char *file, int line_num)
#else
OS_APIs void __OS_MemFree( void *m )
#endif
{
    /**
        *  Platform depedent code. Please rewrite
        *  this piece of code for different system.
        */
        #ifdef OS_MEM_DEBUG
        int size = 0;
        m = (char *)m - 4;
        size = *(int*)m;
        g_os_malloc_size -= size;
        //LOG_PRINTF("__OS_MemFree ptr = %p, file %s, line %d, \r\n", m, file, line_num);
        //LOG_PRINTF("free memory = %d, max alloc = %d\r\n", (int)HEAP_5_POOL_SIZE - g_os_malloc_size, g_os_malloc_size_max);
        #endif
        //k_free(m);

#ifdef CONFIG_USE_WIFI_HEAP_MEM
        vPortFree(m);
#else
        mem_free(m);
#endif
}

void OS_MemCPY(void *pdest, const void *psrc, unsigned int size)
{
    ssv6xxx_memcpy(pdest,psrc,size);
}

void OS_MemSET(void *pdest, unsigned char byte, unsigned int size)
{
    ssv6xxx_memset(pdest,byte,size);
}

static char * _getbase (char *, int *);
static int _atob (unsigned long  *, char *, int);

static char *
_getbase(char *p, int *basep)
{
	if (p[0] == '0') {
		switch (p[1]) {
		case 'x':
			*basep = 16;
			break;
		case 't': case 'n':
			*basep = 10;
			break;
		case 'o':
			*basep = 8;
			break;
		default:
			*basep = 10;
			return (p);
		}
		return (p + 2);
	}
	*basep = 10;
	return (p);
}


/*
 *  _atob(vp,p,base)
 */
static int
_atob (unsigned long *vp, char *p, int base)
{
	unsigned long value, v1, v2;
	char *q, tmp[20];
	int digit;

	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		base = 16;
		p += 2;
	}

	if (base == 16 && (q = strchr (p, '.')) != 0) {
		if (q - p > sizeof(tmp) - 1)
			return (0);

		strncpy (tmp, p, q - p);
		tmp[q - p] = '\0';
		if (!_atob (&v1, tmp, 16))
			return (0);

		q++;
		if (strchr (q, '.'))
			return (0);

		if (!_atob (&v2, q, 16))
			return (0);
		*vp = (v1 << 16) + v2;
		return (1);
	}

	value = *vp = 0;
	for (; *p; p++) {
		if (*p >= '0' && *p <= '9')
			digit = *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			digit = *p - 'a' + 10;
		else if (*p >= 'A' && *p <= 'F')
			digit = *p - 'A' + 10;
		else
			return (0);

		if (digit >= base)
			return (0);
		value *= base;
		value += digit;
	}
	*vp = value;
	return (1);
}

/*
 *  atob(vp,p,base) 
 *      converts p to binary result in vp, rtn 1 on success
 */
int
atob(unsigned long *vp, char *p, int base)
{
	unsigned long  v;

	if (base == 0)
		p = _getbase (p, &base);
	if (_atob (&v, p, base)) {
		*vp = v;
		return (1);
	}
	return (0);
}

/*
 *  gethex(vp,p,n) 
 *      convert n hex digits from p to binary, result in vp, 
 *      rtn 1 on success
 */
int
gethex(int *vp, char *p, int n)
{
	unsigned long v;
	int digit;

	for (v = 0; n > 0; n--) {
		if (*p == 0)
			return (0);
		if (*p >= '0' && *p <= '9')
			digit = *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			digit = *p - 'a' + 10;
		else if (*p >= 'A' && *p <= 'F')
			digit = *p - 'A' + 10;
		else
			return (0);

		v <<= 4;
		v |= digit;
		p++;
	}
	*vp = v;
	return (1);
}

int
strcspn (const char *p, const char *s)
{
	int i, j;

	for (i = 0; p[i]; i++) {
		for (j = 0; s[j]; j++) {
			if (s[j] == p[i])
				break;
		}
		if (s[j])
			break;
	}
	return (i);
}

/*
 * ** fscanf --\    sscanf --\
 * **          |                  |
 * **  scanf --+-- vfscanf ----- vsscanf
 * **
 * ** This not been very well tested.. it probably has bugs
 */
static int vfscanf (FILE *, const char *, va_list);
static int vsscanf (const char *, const char *, va_list);

#define ISSPACE " \t\n\r\f\v"
#define MAXLN    255
/*
 *  scanf(fmt,va_alist) 
 */
int 
scanf (const char *fmt, ...)
{
    int             count;
    va_list ap;
    
    va_start (ap, fmt);
    count = vfscanf (stdin, fmt, ap);
    va_end (ap);
    return (count);
}

/*
 *  fscanf(fp,fmt,va_alist)
 */
int 
fscanf (FILE *fp, const char *fmt, ...)
{
    int             count;
    va_list ap;

    va_start (ap, fmt);
    count = vfscanf (fp, fmt, ap);
    va_end (ap);
    return (count);
}

/*
 *  sscanf(buf,fmt,va_alist)
 */
int 
sscanf (const char *buf, const char *fmt, ...)
{
    int             count;
    va_list ap;
    
    va_start (ap, fmt);
    count = vsscanf (buf, fmt, ap);
    va_end (ap);
    return (count);
}

extern char *fgets(char *buf, int bufsize, FILE *stream);

/*
 *  vfscanf(fp,fmt,ap) 
 */
static int
vfscanf (FILE *fp, const char *fmt, va_list ap)
{
    int             count;
    char            buf[MAXLN + 1];

    if (fgets (buf, MAXLN, fp) == 0)
	return (-1);
    count = vsscanf (buf, fmt, ap);
    return (count);
}


/*
 *  vsscanf(buf,fmt,ap)
 */
static int
vsscanf (const char *buf, const char *s, va_list ap)
{
    int             count, noassign, width, base = 0, lflag;
    const char     *tc;
    char           *t, tmp[MAXLN];

    count = noassign = width = lflag = 0;
    while (*s && *buf) {
	while (isspace (*s))
	    s++;
	if (*s == '%') {
	    s++;
	    for (; *s; s++) {
		if (strchr ("dibouxcsefg%", *s))
		    break;
		if (*s == '*')
		    noassign = 1;
		else if (*s == 'l' || *s == 'L')
		    lflag = 1;
		else if (*s >= '1' && *s <= '9') {
		    for (tc = s; isdigit (*s); s++);
		    strncpy (tmp, tc, s - tc);
		    tmp[s - tc] = '\0';
		    atob ((unsigned long *)&width, tmp, 10);
		    s--;
		}
	    }
	    if (*s == 's') {
		while (isspace (*buf))
		    buf++;
		if (!width)
		    width = strcspn (buf, ISSPACE);
		if (!noassign) {
		    strncpy (t = va_arg (ap, char *), buf, width);
		    t[width] = '\0';
		}
		buf += width;
	    } else if (*s == 'c') {
		if (!width)
		    width = 1;
		if (!noassign) {
		    strncpy (t = va_arg (ap, char *), buf, width);
		    t[width] = '\0';
		}
		buf += width;
	    } else if (strchr ("dobxu", *s)) {
		while (isspace (*buf))
		    buf++;
		if (*s == 'd' || *s == 'u')
		    base = 10;
		else if (*s == 'x')
		    base = 16;
		else if (*s == 'o')
		    base = 8;
		else if (*s == 'b')
		    base = 2;
		if (!width) {
		    if (isspace (*(s + 1)) || *(s + 1) == 0)
			width = strcspn (buf, ISSPACE);
		    else
			width = strchr (buf, *(s + 1)) - buf;
		}
		strncpy (tmp, buf, width);
		tmp[width] = '\0';
		buf += width;
		if (!noassign)
		    atob (va_arg (ap, unsigned long *), tmp, base);
	    }
	    if (!noassign)
		count++;
	    width = noassign = lflag = 0;
	    s++;
	} else {
	    while (isspace (*buf))
		buf++;
	    if (*s != *buf)
		break;
	    else
		s++, buf++;
	}
    }
    return (count);
}


//=====================Platform LDO EN ping setting=======================
//#define SSV_LDO_EN_PIN              IO_E1

#ifdef SSV_LDO_EN_PIN
static struct device *wifi_gpio_port;
#endif

void platform_ldo_en_pin_init(void)
{
#ifdef SSV_LDO_EN_PIN
	wifi_gpio_port = device_get_binding(SSV_LDO_EN_PIN_NAME);
	if (!wifi_gpio_port) {
		printk("cannot found dev gpio\n");
		return;
	}

	gpio_pin_configure(wifi_gpio_port, SSV_LDO_EN_PIN, GPIO_DIR_OUT);
#endif
}

void platform_ldo_en(bool en)
{
#ifdef SSV_LDO_EN_PIN
	gpio_pin_write(wifi_gpio_port, SSV_LDO_EN_PIN, en);

	if (en)
		OS_MsDelay(100);
#endif
}
 
//=====================find fw to download=======================
#include <firmware/ssv6200_uart_bin.h>
unsigned char platform_download_firmware(void)
{
    //LOG_PRINTF("bin size =%d\r\n",sizeof(ssv6200_uart_bin));
    return ssv6xxx_download_fw((u8 *)ssv6200_uart_bin,sizeof(ssv6200_uart_bin));
}

void platform_read_firmware(void *d,void *s,unsigned int len)
{
    OS_MemCPY(d,s,len);
}
