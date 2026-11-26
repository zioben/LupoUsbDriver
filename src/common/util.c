/*
 * LupoUsbDriver - freestanding helpers (memory, strings, formatting).
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/types.h"
#include "lupousb/os.h"

void lu_memset(void *dst, int c, size_t n)
{
    u8 *d = dst;
    while (n--)
        *d++ = (u8)c;
}

void lu_memcpy(void *dst, const void *src, size_t n)
{
    u8 *d = dst;
    const u8 *s = src;
    while (n--)
        *d++ = *s++;
}

int lu_memcmp(const void *a, const void *b, size_t n)
{
    const u8 *x = a, *y = b;
    for (; n; n--, x++, y++)
        if (*x != *y)
            return *x - *y;
    return 0;
}

size_t lu_strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

/* Minimal vsnprintf: %d %u %x %X %s %c %p %%, optional '0' flag and width,
 * 'l' length modifier accepted (all integers are handled as 32 bit). */
int lu_vsnprintf(char *buf, u32 size, const char *fmt, __builtin_va_list ap)
{
    u32 pos = 0;
#define PUT(ch) do { if (pos + 1 < size) buf[pos] = (char)(ch); pos++; } while (0)
    while (*fmt) {
        char tmp[12];
        int width = 0, zero = 0, n = 0, neg = 0;
        u32 v;
        const char *s;
        if (*fmt != '%') {
            PUT(*fmt++);
            continue;
        }
        fmt++;
        if (*fmt == '0') { zero = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');
        while (*fmt == 'l')
            fmt++;
        switch (*fmt) {
        case 'd': {
            s32 sv = __builtin_va_arg(ap, s32);
            if (sv < 0) { neg = 1; v = (u32)(-sv); } else v = (u32)sv;
            goto dec;
        }
        case 'u':
            v = __builtin_va_arg(ap, u32);
        dec:
            do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
            if (neg) tmp[n++] = '-';
            goto out;
        case 'p':
            PUT('0'); PUT('x');
            v = (u32)(size_t)__builtin_va_arg(ap, void *);
            width = 8; zero = 1;
            goto hex;
        case 'x':
        case 'X':
            v = __builtin_va_arg(ap, u32);
        hex:
            do { tmp[n++] = "0123456789abcdef"[v & 15]; v >>= 4; } while (v);
        out:
            while (n < width) tmp[n++] = zero ? '0' : ' ';
            while (n) PUT(tmp[--n]);
            break;
        case 's':
            s = __builtin_va_arg(ap, const char *);
            if (!s) s = "(null)";
            n = (int)lu_strlen(s);
            while (n++ < width) PUT(' ');
            while (*s) PUT(*s++);
            break;
        case 'c':
            PUT((char)__builtin_va_arg(ap, int));
            break;
        case '%':
            PUT('%');
            break;
        default:
            PUT('%');
            if (*fmt) PUT(*fmt);
            break;
        }
        if (*fmt)
            fmt++;
    }
    if (size)
        buf[pos < size ? pos : size - 1] = 0;
#undef PUT
    return (int)pos;
}
