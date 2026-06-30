#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdalign.h>
#include <stdarg.h>

#define STRINGIFY(x) #x
#define MCONCAT(a, b) a##b

#define true   1
#define false  0

#define ArraySize(arr)      (sizeof(arr) / sizeof((arr)[0]))

#define MemCopy             memcpy
#define MemSet              memset
#define MemMove             memmove
#define MemZero(x, y)       MemSet((x), 0, (y))
#define MemZeroArray(a)     MemSet((a), 0, sizeof(a))
#define MemZeroStruct(s)    MemSet((s), 0, sizeof(*(s)))
#define MemAlignUp(v, a)    (((v) + (a) - 1) & ~((a) - 1))

#define CStrCopy            strcpy
#define CStrCopyN           strncpy
#define CStrCompare         strcmp
#define CStrLength          strlen
#define CStrToI32(s)        ((i32)atoi(s))
#define CStrToF32(s)        ((f32)atof(s))

#define Bytes(n)     (n)
#define Kilobytes(n) (1024ull * Bytes(n))
#define Megabytes(n) (1024ull * Kilobytes(n))
#define Gigabytes(n) (1024ull * Megabytes(n))

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

// booleans use a signed type for utility
// operations like subtracting booleans
// from each other when handling input :p
typedef int8_t      b8;
typedef int16_t     b16;
typedef int32_t     b32;
typedef int64_t     b64;

typedef float       f32;
typedef double      f64;

typedef size_t      usize;
typedef uintptr_t   uptr;

#endif // CORE_TYPES_H
