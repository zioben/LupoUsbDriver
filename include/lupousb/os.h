/*
 * LupoUsbDriver - minimal OS abstraction used by the portable layers.
 *
 * Implementations: src/common/os_host.c (unit tests, simulated clock)
 *                  src/amiga/os_amiga.c (Exec memory, timer.device, kprintf)
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_OS_H
#define LUPOUSB_OS_H

#include "lupousb/types.h"

/* Zero-filled allocation; returns NULL on failure. */
void *os_alloc(u32 size);
void  os_free(void *p, u32 size);

/* Monotonic millisecond clock. */
u32   os_time_ms(void);

enum { LU_LOG_ERROR = 0, LU_LOG_WARN, LU_LOG_INFO, LU_LOG_DEBUG };

/* printf-like logging with a minimal formatter: %d %u %x %02x %04x %s %c %p. */
void  os_log(int level, const char *fmt, ...);
extern int os_log_level;

#define LU_ERR(...)   os_log(LU_LOG_ERROR, __VA_ARGS__)
#define LU_WARN(...)  os_log(LU_LOG_WARN, __VA_ARGS__)
#define LU_INFO(...)  os_log(LU_LOG_INFO, __VA_ARGS__)
#ifdef NDEBUG
#define LU_DBG(...)   do { } while (0)
#else
#define LU_DBG(...)   os_log(LU_LOG_DEBUG, __VA_ARGS__)
#endif

/* Formats into buf (always NUL terminated). Used by both OS backends. */
int lu_vsnprintf(char *buf, u32 size, const char *fmt, __builtin_va_list ap);

#endif /* LUPOUSB_OS_H */
