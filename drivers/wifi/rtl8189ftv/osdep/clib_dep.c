#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include "clib_dep.h"

extern char *fgets(char *buf, int bufsize, FILE *stream);

#ifndef in_range
#define in_range(c, lo, up)  ((uint8_t)c >= lo && (uint8_t)c <= up)
#endif

#ifndef isprint
#define isprint(c)           in_range(c, 0x20, 0x7f)
#endif

#ifndef  isdigit
#define isdigit(c)           in_range(c, '0', '9')
#endif

#ifndef isxdigit
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#endif

#ifndef islower
#define islower(c)           in_range(c, 'a', 'z')
#endif

#ifndef isspace
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#endif

#define ISSPACE " \t\n\r\f\v"
#define MAXLN    255

/*
 *  vsscanf(buf,fmt,ap)
 */
static int vsscanf(const char *buf, const char *s, va_list ap)
{
    int             count, noassign, width, base, lflag;
    const char     *tc;
    char           *t, tmp[MAXLN];

	base = 0;
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
		    atob ((long unsigned int *)&width, tmp, 10);
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

/*
 *  vfscanf(fp,fmt,ap)
 */
static int vfscanf(FILE *fp, const char *fmt, va_list ap)
{
    int             count;
    char            buf[MAXLN + 1];

    if (fgets (buf, MAXLN, fp) == 0)
	return (-1);
    count = vsscanf (buf, fmt, ap);
    return (count);
}

/*
 *  scanf(fmt,va_alist)
 */
int scanf(const char *fmt, ...)
{
    int             count;
    va_list ap;

    va_start (ap, fmt);
    count = vfscanf(stdin, fmt, ap);
    va_end (ap);
    return (count);
}

/*
 *  fscanf(fp,fmt,va_alist)
 */
int fscanf(FILE *fp, const char *fmt, ...)
{
    int             count;
    va_list ap;

    va_start (ap, fmt);
    count = vfscanf(fp, fmt, ap);
    va_end (ap);
    return (count);
}

/*
 *  sscanf(buf,fmt,va_alist)
 */
int sscanf(const char *buf, const char *fmt, ...)
{
    int             count;
    va_list ap;

    va_start (ap, fmt);
    count = vsscanf(buf, fmt, ap);
    va_end (ap);
    return (count);
}

static char *_getbase(char *p, int *basep)
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

static int _atob (unsigned long *vp, char *p, int base)
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
int atob(long unsigned int *vp, char *p, int base)
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

static bool char_in_set(char chr, const char *set)
{
        const char *ptr;

        for (ptr = set; *ptr; ptr++) {
                if (*ptr == chr) {
                        return true;
                }
        }

        return false;
}

char *strsep(char *strp, const char *delims)
{
        const char *delim;
        char *ptr;

        if (!strp) {
                return NULL;
        }

        for (delim = delims; *delim; delim++) {
                ptr = strchr(strp, *delim);
                if (ptr) {
                        *ptr = '\0';

                        for (ptr++; *ptr; ptr++) {
                                if (!char_in_set(*ptr, delims)) {
                                        break;
                                }
                        }

                        return ptr;
                }
        }

        return NULL;
}

int strcspn (const char *p, const char *s)
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

int abs(int value)
{
	return (value>= 0 ? value : -value);
}

