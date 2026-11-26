/*
 * LupoUsbDriver - OS layer for the Linux host (unit tests, simulator).
 *
 * The clock is simulated: tests advance it explicitly with host_advance(),
 * which makes every run deterministic and independent of the machine speed.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/os.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int os_log_level = LU_LOG_WARN;
static u32 host_now = 1000;
static long host_alloc_count;

void *os_alloc(u32 size)
{
    void *p = calloc(1, size ? size : 1);
    if (p)
        host_alloc_count++;
    return p;
}

void os_free(void *p, u32 size)
{
    (void)size;
    if (p) {
        free(p);
        host_alloc_count--;
    }
}

long host_allocations(void) { return host_alloc_count; }

u32 os_time_ms(void) { return host_now; }
void host_set_time(u32 ms) { host_now = ms; }

void os_log(int level, const char *fmt, ...)
{
    static const char *const tag[] = { "ERR ", "WARN", "INFO", "DBG " };
    char buf[256];
    va_list ap;
    if (level > os_log_level)
        return;
    va_start(ap, fmt);
    lu_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "[%7u %s] %s\n", (unsigned)host_now, tag[level & 3], buf);
}
