#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <cstdint>
#include <memory>
#include <cstddef>
#include <cstring>

#define STRINGIFY(x) #x
#define MCONCAT(a, b) a##b

#define ArraySize(arr)        (sizeof(arr) / sizeof((arr)[0]))

#define MemAlignUp(v, a)      (((v) + (a) - 1) & ~((a) - 1))
#define MemAlignDown(v, a)    ((v) & ~((u64)(a) - 1))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uintptr_t uptr;

constexpr u64 Kilobytes(u64 n)
{
	return 1024ull * n;
}

constexpr u64 Megabytes(u64 n)
{
	return 1024ull * Kilobytes(n);
}

constexpr u64 Gigabytes(u64 n)
{
	return 1024ull * Megabytes(n);
}

template <typename T> using Unique = std::unique_ptr<T>;
template <typename T> using SharedRef = std::shared_ptr<T>;
template <typename T> using Weak = std::weak_ptr<T>;

template <typename T, typename... TArgs>
constexpr Unique<T> CreateScope(TArgs &&...args)
{
	return std::make_unique<T>(std::forward<TArgs>(args)...);
}

template <typename T, typename... TArgs>
constexpr SharedRef<T> CreateRef(TArgs &&...args)
{
	return std::make_shared<T>(std::forward<TArgs>(args)...);
}

#endif // CORE_TYPES_H
