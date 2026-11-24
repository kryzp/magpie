#include "core/hash.h"

u64 hash::Bytes(const void *key, u64 length)
{
	return BytesCombine(FNV_OFFSET_BASIS, key, length);
}

u64 hash::BytesCombine(u64 start, const void *key, u64 length)
{
	const u8 *data = (const u8 *)key;

	u64 hash = start;

	for (u64 i = 0; i < length; i++) {
		hash ^= (u64)data[i];
		hash *= FNV_PRIME;
	}

	return hash;
}

u64 hash::Str(const String &str)
{
	u64 hash = FNV_OFFSET_BASIS;

	for (u64 i = 0; i < str.size(); i++) {
		hash ^= (u8)str[i];
		hash *= FNV_PRIME;
	}

	return hash;
}

u64 hash::CStr(const char *str)
{
	u64 hash = FNV_OFFSET_BASIS;

	while (*str != '\0') {
		hash ^= (u64)*str;
		hash *= FNV_PRIME;
		str++;
	}

	return hash;
}

u32 hash::Ints(i32 a, i32 b)
{
	u32 h = (u32)a * 374761393u + (u32)b * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	h ^= h >> 16;

	return h;
}

struct Crc32Table {
	u32 entries[256];

	Crc32Table()
	{
		for (u32 i = 0; i < 256; i++) {
			u32 c = i;

			for (u32 k = 0; k < 8; k++) {
				c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
			}

			entries[i] = c;
		}
	}
};

u32 hash::Crc32(const void *data, u64 length)
{
	static const Crc32Table table;

	const u8 *bytes = (const u8 *)data;
	u32 crc = 0xFFFFFFFFu;

	for (u64 i = 0; i < length; i++) {
		crc = table.entries[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ 0xFFFFFFFFu;
}
