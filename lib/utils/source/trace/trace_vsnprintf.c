#include <stddef.h>
#include <stdarg.h>
#include <linker/sections.h>

#if defined(CONFIG_SOC_SERIES_WOODPECKER) || defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
#define __TRACE_RAMFUNC
#else
#define __TRACE_RAMFUNC __ramfunc
#endif

#define LINESEP_FORMAT_WINDOWS  0
#define LINESEP_FORMAT_UNIX     1
enum pad_type {
	PAD_NONE,
	PAD_ZERO_BEFORE,
	PAD_SPACE_BEFORE,
	PAD_SPACE_AFTER,
};

#define _putc(_str, _end, _ch)  \
do                              \
{                               \
    if (_str < _end)            \
    {				\
        *_str = _ch;            \
        _str++;                 \
    }				\
}                               \
while (0)


/**
 * @brief Output an unsigned long in hex format
 *
 * Output an unsigned long on output installed by platform at init time. Should
 * be able to handle an unsigned long of any size, 32 or 64 bit.
 * @param num Number to output
 *
 * @return N/A
 */
__TRACE_RAMFUNC static char* _printk_hex_ulong(char* str, char* end,
			      const unsigned long num, enum pad_type padding,
			      int min_width)
{
	int size = sizeof(num) * 2;
	int found_largest_digit = 0;
	int remaining = 8; /* 8 digits max */
	int digits = 0;

	for (; size; size--) {
		char nibble = (num >> ((size - 1) << 2) & 0xf);

		if (nibble || found_largest_digit || size == 1) {
			found_largest_digit = 1;
			nibble += nibble > 9 ? 87 : 48;
			_putc(str, end, (int)nibble);
			digits++;
			continue;
		}

		if (remaining-- <= min_width) {
			if (padding == PAD_ZERO_BEFORE) {
				_putc(str, end, '0');
			} else if (padding == PAD_SPACE_BEFORE) {
				_putc(str, end, ' ');
			}
		}
	}

	if (padding == PAD_SPACE_AFTER) {
		remaining = min_width * 2 - digits;
		while (remaining-- > 0) {
			_putc(str, end, ' ');
		}
	}
	return str;
}


/**
 * @brief Output an unsigned long (32-bit) in decimal format
 *
 * Output an unsigned long on output installed by platform at init time. Only
 * works with 32-bit values.
 * @param num Number to output
 *
 * @return N/A
 */
__TRACE_RAMFUNC static char* _printk_dec_ulong(char* str, char* end,
			      const unsigned long num, enum pad_type padding,
			      int min_width)
{
	unsigned long pos = 999999999;
	unsigned long remainder = num;
	int found_largest_digit = 0;
	int remaining = 10; /* 10 digits max */
	int digits = 1;

	/* make sure we don't skip if value is zero */
	if (min_width <= 0) {
		min_width = 1;
	}

	while (pos >= 9) {
		if (found_largest_digit || remainder > pos) {
			found_largest_digit = 1;
			_putc(str, end, (int)((remainder / (pos + 1)) + 48));
			digits++;
		} else if (remaining <= min_width
				&& padding < PAD_SPACE_AFTER) {
			_putc(str, end, (int)(padding == PAD_ZERO_BEFORE ? '0' : ' '));
			digits++;
		}
		remaining--;
		remainder %= (pos + 1);
		pos /= 10;
	}

	_putc(str, end, (int)(remainder + 48));

	if (padding == PAD_SPACE_AFTER) {
		remaining = min_width - digits;
		while (remaining-- > 0) {
			_putc(str, end, ' ');
		}
	}
	return str;
}

__TRACE_RAMFUNC int trace_vsnprintf(char* buf, int size, unsigned int linesep, const char* fmt, va_list ap)
{
	int might_format = 0; /* 1 if encountered a '%' */
	enum pad_type padding = PAD_NONE;
	int min_width = -1;
	int long_ctr = 0;
	char*  str = buf;
	char*  end = buf + size - 1;

	/* fmt has already been adjusted if needed */

	while (*fmt && *fmt != '\0') {
		if (!might_format) {
			if (*fmt != '%') {
				if (linesep == LINESEP_FORMAT_WINDOWS) {
					if(*fmt == '\n')
					_putc(str, end, '\r');
				}
				_putc(str, end, (int)*fmt);
			} else {
				might_format = 1;
				min_width = -1;
				padding = PAD_NONE;
				long_ctr = 0;
			}
		} else {
			switch (*fmt) {
			case '-':
				padding = PAD_SPACE_AFTER;
				goto still_might_format;
			case '0':
				if (min_width < 0 && padding == PAD_NONE) {
					padding = PAD_ZERO_BEFORE;
					goto still_might_format;
				}
				/* Fall through */
			case '1' ... '9':
				if (min_width < 0) {
					min_width = *fmt - '0';
				} else {
					min_width = 10 * min_width + *fmt - '0';
				}

				if (padding == PAD_NONE) {
					padding = PAD_SPACE_BEFORE;
				}
				goto still_might_format;
			case 'l':
				long_ctr++;
				/* Fall through */
			case 'z':
			case 'h':
				/* FIXME: do nothing for these modifiers */
				goto still_might_format;
			case 'd':
			case 'i': {
				long d;
				if (long_ctr < 2) {
					d = va_arg(ap, long);
				} else {
					d = (long)va_arg(ap, long long);
				}

				if (d < 0) {
					 _putc(str, end, (int)'-');
					d = -d;
					min_width--;
				}
				str = _printk_dec_ulong(str, end, d, padding,
						  min_width);
				break;
			}
			case 'u': {
				unsigned long u;

				if (long_ctr < 2) {
					u = va_arg(ap, unsigned long);
				} else {
					u = (unsigned long)va_arg(ap,
							unsigned long long);
				}

				str = _printk_dec_ulong(str, end, u, padding,
						  min_width);
				break;
			}
			case 'p':
				  _putc(str, end, '0');
				  _putc(str, end, 'x');
				  /* left-pad pointers with zeros */
				  padding = PAD_ZERO_BEFORE;
				  min_width = 8;
				  /* Fall through */
			case 'x':
			case 'X': {
				unsigned long x;

				if (long_ctr < 2) {
					x = va_arg(ap, unsigned long);
				} else {
					x = (unsigned long)va_arg(ap,
							unsigned long long);
				}

				str = _printk_hex_ulong(str, end, x, padding,
						  min_width);

				break;
			}
			case 's': {
				char *s = va_arg(ap, char *);

				while (*s) {
					_putc(str, end, (int)(*s));
					s++;
				}
				break;
			}
			case 'c': {
				int c = va_arg(ap, int);

				_putc(str, end, c);
				break;
			}
			case '%': {
				_putc(str, end, (int)'%');
				break;
			}
			default:
				_putc(str, end, (int)'%');
				_putc(str, end, (int)*fmt);
				break;
			}
			might_format = 0;
		}
still_might_format:
		++fmt;
	}
    if (str <= end) {
        *str = '\0';
	} else if (size > 0) {
        *end = '\0';
	}
	return (str - buf);
}
