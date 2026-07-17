
static String8 String8Alloc(Arena *arena, u32 len)
{
	String8 str = {0};
	str.str = ArenaPushArray(arena, u8, len + 1);
	str.len = len;

	return str;
}

static String8 String8Clone(Arena *arena, String8 string)
{
	String8 clone = {0};
	clone.str = ArenaPushArray(arena, u8, string.len + 1);
	clone.len = string.len;

	MemCopy(clone.str, string.str, clone.len);

	return clone;
}

static String8 String8Append(Arena *arena, String8 a, String8 b)
{
	u32 len = a.len + b.len;

	String8 out = String8Alloc(arena, len);

	MemCopy(out.str,         a.str, a.len);
	MemCopy(out.str + a.len, b.str, b.len);

	return out;
}

static String8 String8Fmt(Arena *arena, const char *fmt, ...)
{
	char out[512] = {0};
	
	va_list args;
	va_start(args, fmt);
	vsnprintf(out, sizeof(out), fmt, args);
	va_end(args);

	return String8Clone(arena, String8FromCStr(out));
}

static String8 String8Skip(String8 string, u64 to)
{
	String8 skipped = {0};
	skipped.str = string.str + to;
	skipped.len = string.len - to;

	return skipped;
}

static b32 String8Match(String8 a, String8 b)
{
	if (a.len != b.len)
		return false;

	for (u32 i = 0; i < a.len; i++)
	{
		if (a.str[i] != b.str[i])
			return false;
	}

	return true;
}

static b32 String8StartsWith(String8 string, String8 prefix)
{
	if (prefix.len > string.len)
		return false;

	for (u32 i = 0; i < prefix.len; i++)
	{
		if (string.str[i] != prefix.str[i])
			return false;
	}

	return true;
}

static u64 String8Find(String8 string, String8 substr)
{
	for (u64 i = 0; i < string.len; i++)
	{
		String8 here = String8Init(string.str + i, substr.len);

		if (String8Match(here, substr))
			return i;
	}

	return CORE_STRING_INVALID_INDEX;
}

static u64 String8FindLast(String8 string, String8 substr)
{
	for (u64 i = string.len - substr.len - 1; i >= 0; i--)
	{
		String8 here = String8Init(string.str + i, substr.len);

		if (String8Match(here, substr))
			return i;
	}

	return CORE_STRING_INVALID_INDEX;
}

static u64 String8FindLastIncl(String8 string, String8 substr)
{
	for (u64 i = string.len - substr.len - 1; i >= 0; i--)
	{
		String8 here = String8Init(string.str + i, substr.len);

		if (String8Match(here, substr))
			return i + substr.len;
	}

	return CORE_STRING_INVALID_INDEX;
}

static b32 CharIsWhitespace(u8 c)
{
	return c <= 32;
}

static b32 CharIsLower(u8 c)
{
	return c >= 'a' && c <= 'z';
}

static b32 CharIsUpper(u8 c)
{
	return c >= 'A' && c <= 'Z';
}

static b32 CharIsAlpha(u8 c)
{
	return CharIsLower(c) || CharIsUpper(c);
}

static b32 CharIsDigit(u8 c)
{
	return c >= '1' && c <= '9';
}

static u8 CharToLower(u8 c)
{
	if (CharIsLower(c))
		c += 32;

	return c;
}

static u8 CharToUpper(u8 c)
{
	if (CharIsUpper(c))
		c -= 32;

	return c;
}
