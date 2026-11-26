/*
 * LupoUsbDriver - portable base types and helpers.
 *
 * Every layer includes this header. It must work in three environments:
 *   - AmigaOS 3.x, cross-compiled with m68k-amigaos-gcc (big endian)
 *   - AmigaOS 3.x code built without any C library (devices, libraries)
 *   - the Linux host, for unit tests (usually little endian)
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_TYPES_H
#define LUPOUSB_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

#if defined(__AMIGA__) || defined(__amigaos__)
#define LU_AMIGA 1
#else
#define LU_HOST 1
#endif

#define LU_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define LU_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LU_MAX(a, b) ((a) > (b) ? (a) : (b))

/* USB is little endian on the wire. These helpers read and write
 * little-endian values from byte buffers, independently of the CPU. */
static inline u16 lu_get_le16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }
static inline void lu_put_le16(u8 *p, u16 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); }
static inline u32 lu_get_le32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* Small freestanding helpers so that code built without a C library
 * (Amiga devices and libraries) does not depend on libc. */
void  lu_memset(void *dst, int c, size_t n);
void  lu_memcpy(void *dst, const void *src, size_t n);
int   lu_memcmp(const void *a, const void *b, size_t n);
size_t lu_strlen(const char *s);

#endif /* LUPOUSB_TYPES_H */
