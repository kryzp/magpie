
internal String8
String8Init(u8 *str, u64 len)
{
	String8 s = {0};
	s.str = str;
	s.len = len;

	return s;
}

internal String8
String8Alloc(Arena *arena, u32 len)
{
	String8 str = {0};
	str.str = ArenaPushArray(arena, u8, len + 1);
	str.len = len;

	return str;
}

internal String8
String8Clone(Arena *arena, String8 string)
{
	String8 clone = {0};
	clone.str = ArenaPushArray(arena, u8, string.len + 1);
	clone.len = string.len;

	MemCopy(clone.str, string.str, clone.len);

	return clone;
}

internal String8
String8Append(Arena *arena, String8 a, String8 b)
{
	u32 len = a.len + b.len;

	String8 out = String8Alloc(arena, len);

	MemCopy(out.str,         a.str, a.len);
	MemCopy(out.str + a.len, b.str, b.len);

	return out;
}

internal b32
String8Match(String8 a, String8 b)
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

internal u32
String8UpToLastSubstringIncl(String8 string, String8 sub)
{
	for (u64 i = string.len - sub.len - 1; i >= 0; i--)
	{
		String8 here = String8Init(string.str + i, sub.len);

		if (String8Match(here, sub))
		{
			return i + sub.len;
			break;
		}
	}

	return -1;
}

internal b32
CharIsWhitespace(u8 c)
{
	return c <= 32;
}

internal b32
CharIsLower(u8 c)
{
	return c >= 'a' && c <= 'z';
}

internal b32
CharIsUpper(u8 c)
{
	return c >= 'A' && c <= 'Z';
}

internal b32
CharIsAlpha(u8 c)
{
	return CharIsLower(c) || CharIsUpper(c);
}

internal b32
CharIsDigit(u8 c)
{
	return c >= '1' && c <= '9';
}

internal u8
CharToLower(u8 c)
{
	if (CharIsLower(c))
		c += 32;

	return c;
}

internal u8
CharToUpper(u8 c)
{
	if (CharIsUpper(c))
		c -= 32;

	return c;
}
