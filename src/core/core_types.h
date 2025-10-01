#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <assert.h>

#define STRINGIFY(x) #x

#define debug_log(m, ...)				\
	do {						\
		printf((m "\n"), ##__VA_ARGS__);	\
	} while (0)

#define debug_log_crash(m, ...)			\
	do {					\
		debug_log(m, ##__VA_ARGS__);	\
		assert(0);			\
	} while (0)

#define square_root                sqrtf
#define memory_copy                memcpy
#define memory_set                 memset
#define memory_move                memmove
#define memory_zero_struct(s)      memset((s), 0, sizeof(*(s)))
#define cstr_copy                  strcpy
#define cstr_copy_n                strncpy
#define cstr_compare               strcmp
#define cstr_length                strlen
#define cstr_to_int(s)             ((int)atoi(s))
#define cstr_to_float(s)           ((float)atof(s))
#define min_value(a, b)            (((a) < (b)) ? (a) : (b))
#define max_value(a, b)            (((a) > (b)) ? (a) : (b))
#define clamp_value(v, lo, hi)     (((v) < (lo)) ? (lo) : (((v) > (hi)) ? (hi) : (v)))
#define array_size(a)              (sizeof(a) / sizeof(*(a)))

#define MEMORY_ALIGN_UP(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

#define BYTES(n) (n)
#define KILOBYTES(n) (BYTES(n)     * 1024ULL)
#define MEGABYTES(n) (KILOBYTES(n) * 1024ULL)
#define GIGABYTES(n) (MEGABYTES(n) * 1024ULL)

#define PI                                  3.1415926535897
#define PIf                                 3.1415926535897f
#define ONE_OVER_SQUARE_ROOT_OF_TWO_PI      0.3989422804
#define ONE_OVER_SQUARE_ROOT_OF_TWO_PIf     0.3989422804f
#define EULERS_NUMBER                       2.718281828459045
#define EULERS_NUMBERf                      2.718281828459045f
#define EPSILON DBL_EPSILON
#define EPSILONf FLT_EPSILON

#define bool _Bool
#define true 1
#define false 0

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint8_t  b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;

#endif // CORE_TYPES_H
