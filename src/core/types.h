#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>

#include <memory>

#define STRINGIFY(x) #x
#define MCONCAT(a, b) a##b

// We need this because of expansion order in macros.
// e.g: MCONCAT_EXP(x, __LINE__) != MCONCAT(x, __LINE__)
#define MCONCAT_EXP(a, b) MCONCAT(a, b)

#define debug_log(m, ...)					\
	do {									\
		printf((m "\n"), ##__VA_ARGS__);	\
	} while (0)

#define debug_log_crash(m, ...)			\
	do {								\
		debug_log(m, ##__VA_ARGS__);	\
		assert(0);						\
	} while (0)

#define memory_copy                              memcpy
#define memory_set                               memset
#define memory_move                              memmove
#define memory_zero_struct(s)                    memset((s), 0, sizeof(*(s)))
#define cstr_copy                                strcpy
#define cstr_copy_n                              strncpy
#define cstr_compare                             strcmp
#define cstr_length                              strlen
#define cstr_to_int(s)                           ((int)atoi(s))
#define cstr_to_float(s)                         ((float)atof(s))
#define array_size(a)                            (sizeof(a) / sizeof(*(a)))
#define memory_align_up(value, alignment)        (((value) + (alignment) - 1) & ~((alignment) - 1))

#define BYTES(n) (n)
#define KILOBYTES(n) (BYTES(n)     * 1024ULL)
#define MEGABYTES(n) (KILOBYTES(n) * 1024ULL)
#define GIGABYTES(n) (MEGABYTES(n) * 1024ULL)

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

typedef unsigned long ulong;
typedef uintptr_t uptr;

template <typename T> using Unique = std::unique_ptr<T>;

template <typename T, typename... TArgs>
constexpr Unique<T> create_unique(TArgs&& ... args)
{
	return std::make_unique<T>(std::forward<TArgs>(args)...);
}

template <typename T> using Ref = std::shared_ptr<T>;

template <typename T, typename... TArgs>
constexpr Ref<T> create_ref(TArgs&& ... args)
{
	return std::make_shared<T>(std::forward<TArgs>(args)...);
}

template <typename T> using Weak = std::weak_ptr<T>;
