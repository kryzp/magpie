#include "hash.h"

// FNV-1a 64-bit hash.
u64 hash::generic(const void *key, u64 length)
{
	const u8 *data = (const u8 *)key;
	u64 hash = 1469598103934665603ULL;

	for (u64 i = 0; i < length; i++) {
		hash ^= (u64)data[i];
		hash *= 1099511628211ULL;
	}

	return hash;
}

// FNV-1a 64-bit hash.
u64 hash::generic_combine(u64 start, const void *key, u64 length)
{
	const u8 *data = (const u8 *)key;
	u64 hash = start;

	for (u64 i = 0; i < length; i++) {
		hash ^= (u64)data[i];
		hash *= 1099511628211ULL;
	}

	return hash;
}

u64 hash::cstr(const char *string)
{
	u64 length = 0;
	while (string[length] != '\0') length++;
	return hash::generic(string, length);
}
