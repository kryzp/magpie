#ifndef CORE_HASH_H
#define CORE_HASH_H

#include "core/types.h"
#include "container/string.h"

namespace hash {

constexpr u64 FNV_PRIME = 1099511628211ULL;
constexpr u64 FNV_OFFSET_BASIS = 1469598103934665603ULL;

u64 Bytes(const void *key, u64 length);
u64 BytesCombine(u64 start, const void *key, u64 length);

u64 Str(const String &str);
u64 CStr(const char *str);

u32 Ints(i32 a, i32 b);

u32 Crc32(const void *data, u64 length);

template <typename T>
inline u64 Struct(const T &value)
{
	return Bytes(&value, sizeof(T));
}

}

#endif // CORE_HASH_H
