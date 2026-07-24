#ifndef CORE_STRING_H
#define CORE_STRING_H

#define CORE_STRING_INVALID_INDEX ((u32)-1)

typedef struct String8 String8;
struct String8
{
	u8 *str;
	u64 len;
};

#define String8Init(str_, len_) ((String8) { .str = (u8 *)(str_), .len = (len_) })
#define String8Substr(str_, begin_, end_) String8Init((str_).str + (begin_), (end_) - (begin_))
#define String8FromCStr(str_) String8Init((str_), CStrLength(str_))
#define String8Lit(str_) String8Init((str_), sizeof(str_) - 1)

// PrintF("%.*s", String8VArg(string));
#define String8VArg(str_) (i32)((str_).len), (const char *)((str_).str)

internal String8 String8Alloc(Arena *arena, u32 len);
internal String8 String8Clone(Arena *arena, String8 string);
internal String8 String8Append(Arena *arena, String8 a, String8 b);
internal String8 String8Fmt(Arena *arena, const char *fmt, ...);

internal String8 String8Skip(String8 string, u64 to);

internal b32 String8Match(String8 a, String8 b);
internal b32 String8StartsWith(String8 string, String8 prefix);

internal u64 String8Find(String8 string, String8 substr);
internal u64 String8FindLast(String8 string, String8 substr);
internal u64 String8FindLastIncl(String8 string, String8 substr); // note: inclusive from the left->right !

internal b32 CharIsWhitespace(u8 c);
internal b32 CharIsLower(u8 c);
internal b32 CharIsUpper(u8 c);
internal b32 CharIsAlpha(u8 c);
internal b32 CharIsDigit(u8 c);

internal u8 CharToLower(u8 c);
internal u8 CharToUpper(u8 c);

#endif // CORE_STRING_H
