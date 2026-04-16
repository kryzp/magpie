#ifndef CORE_STRING_H
#define CORE_STRING_H

typedef struct String8 String8;
struct String8
{
	u8 *str;
	u64 len;
};

internal String8 String8Init(u8 *str, u64 len);

#define Str8(s) String8Init((u8 *)(s), sizeof(s) - 1)

internal String8 String8Alloc(Arena *arena, u32 len);
internal String8 String8Clone(Arena *arena, String8 string);
internal String8 String8Append(Arena *arena, String8 a, String8 b);

internal b32 String8Match(String8 a, String8 b);
internal u32 String8UpToLastSubstringIncl(String8 string, String8 sub);

internal b32 CharIsWhitespace(u8 c);
internal b32 CharIsLower(u8 c);
internal b32 CharIsUpper(u8 c);
internal b32 CharIsAlpha(u8 c);
internal b32 CharIsDigit(u8 c);

internal u8 CharToLower(u8 c);
internal u8 CharToUpper(u8 c);

#endif // CORE_STRING_H
