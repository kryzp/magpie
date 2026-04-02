#ifndef CORE_HASH_H
#define CORE_HASH_H

internal u64 HashBytesGeneric(const void *key, u64 length);
internal u64 HashBytesGenericCombine(u64 start, const void *key, u64 length);

internal u64 HashStr8(String8 str);
internal u64 HashCStr(const char *str);

#endif // CORE_HASH_H
