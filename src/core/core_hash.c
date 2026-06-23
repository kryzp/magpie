
#define HASH_FNV_PRIME          1099511628211ULL
#define HASH_FNV_OFFSET_BASIS   1469598103934665603ULL

static u64 HashBytesGeneric(const void *key, u64 length)
{
	const u8 *data = (const u8 *)key;
	
	u64 hash = HASH_FNV_OFFSET_BASIS;

	for (u64 i = 0; i < length; i++)
	{
		hash ^= (u64)data[i];
		hash *= HASH_FNV_PRIME;
	}

	return hash;
}

static u64 HashBytesGenericCombine(u64 start, const void *key, u64 length)
{
	const u8 *data = (const u8 *)key;
	
	u64 hash = start;

	for (u64 i = 0; i < length; i++)
	{
		hash ^= (u64)data[i];
		hash *= HASH_FNV_PRIME;
	}

	return hash;
}

static u64 HashStr8(String8 str)
{
	u64 hash = HASH_FNV_OFFSET_BASIS;

	for (u64 i = 0; i < str.len; i++)
	{
		hash ^= str.str[i];
		hash *= HASH_FNV_PRIME;
	}

	return hash;
}

static u64 HashCStr(const char *str)
{
	u64 hash = HASH_FNV_OFFSET_BASIS;

	while (*str != '\0')
	{
		hash ^= (u64)*str;
		hash *= HASH_FNV_PRIME;
		str++;
	}

	return hash;
}
