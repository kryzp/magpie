#pragma once

#include "types.h"

// FNV-1a 64-bit hash.
namespace hash
{
	constexpr static u64 FNV_PRIME = 1099511628211ULL;
	constexpr static u64 FNV_OFFSET_BASIS = 1469598103934665603ULL;
	
	constexpr u64 generic(const void *key, u64 length)
	{
		const u8 *data = (const u8 *)key;
		u64 hash = FNV_OFFSET_BASIS;

		for (u64 i = 0; i < length; i++) {
			hash ^= (u64)data[i];
			hash *= FNV_PRIME;
		}

		return hash;
	}
	
	constexpr u64 generic_combine(u64 start, const void *key, u64 length)
	{
		const u8 *data = (const u8 *)key;
		u64 hash = start;

		for (u64 i = 0; i < length; i++) {
			hash ^= (u64)data[i];
			hash *= FNV_PRIME;
		}

		return hash;
	}

	constexpr u64 c_str(const char *string)
	{
		u64 hash = FNV_OFFSET_BASIS;

		while (*string != '\0') {
			hash ^= (u64)*string;
			hash *= FNV_PRIME;
			string++;
		}

		return hash;
	}
}
