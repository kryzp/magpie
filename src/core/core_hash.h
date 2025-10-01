#ifndef CORE_HASH_H
#define CORE_HASH_H

#include "core_types.h"

u64 hash_bytes_generic(const void *key, u64 length);
u64 hash_bytes_generic_combine(u64 start, const void *key, u64 length);
u64 hash_cstr(const char *string);

#endif // CORE_HASH_H
