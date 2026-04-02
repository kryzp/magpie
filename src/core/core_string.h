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

internal b32 String8Match(String8 a, String8 b);
internal String8 String8ViewUpToLastSubstringIncl(String8 string, String8 sub);

internal b32 CharIsWhitespace(u8 c);
internal b32 CharIsLower(u8 c);
internal b32 CharIsUpper(u8 c);
internal b32 CharIsAlpha(u8 c);
internal b32 CharIsDigit(u8 c);

internal u8 CharToLower(u8 c);
internal u8 CharToUpper(u8 c);

#endif // CORE_STRING_H
