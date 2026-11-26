/*
 * LupoUsbDriver - OS layer for AmigaOS 3.x (Exec memory, timer.device clock,
 * logging to the serial port with KPutStr or through a replaceable hook).
 *
 * Usable both from programs (libnix startup) and from devices/libraries
 * built without a C library: it only needs SysBase and TimerBase.
 *
 * SPDX-License-Identifier: MIT
 */
#include <exec/memory.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include "lupousb/os.h"

struct Device *TimerBase;               /* set by the owner after OpenDevice */
int os_log_level = LU_LOG_INFO;
void (*os_log_hook)(const char *line);  /* NULL: serial debug output */

extern void KPutStr(const char *);      /* debug.lib */

void *os_alloc(u32 size)
{
    return AllocVec(size ? size : 1, MEMF_PUBLIC | MEMF_CLEAR);
}

void os_free(void *p, u32 size)
{
    (void)size;
    if (p)
        FreeVec(p);
}

u32 os_time_ms(void)
{
    struct timeval tv;
    if (!TimerBase)
        return 0;
    GetSysTime(&tv);
    return (u32)tv.tv_secs * 1000u + (u32)tv.tv_micro / 1000u;
}

void os_log(int level, const char *fmt, ...)
{
    static const char *const tag[] = { "ERR ", "WARN", "INFO", "DBG " };
    static const char prefix[] = "[lupousb ";
    char buf[200];
    u32 n = sizeof(prefix) - 1;
    __builtin_va_list ap;
    if (level > os_log_level)
        return;
    lu_memcpy(buf, prefix, n);
    lu_memcpy(buf + n, tag[level & 3], 4);
    buf[n + 4] = ']';
    buf[n + 5] = ' ';
    __builtin_va_start(ap, fmt);
    lu_vsnprintf(buf + n + 6, sizeof(buf) - n - 6, fmt, ap);
    __builtin_va_end(ap);
    if (os_log_hook) {
        os_log_hook(buf);
    } else {
        KPutStr(buf);
        KPutStr("\n");
    }
}
