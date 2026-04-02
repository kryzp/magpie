
internal String8
String8Init(u8 *str, u64 len)
{
	String8 s = {0};
	s.str = str;
	s.len = len;

	return s;
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

internal String8
String8ViewUpToLastSubstringIncl(String8 string, String8 sub)
{
	String8 result = {0};
	result.str = string.str;
	result.len = 0;

	for (u64 i = string.len - sub.len - 1; i >= 0; i--)
	{
		String8 here = String8Init(string.str + i, sub.len);

		if (String8Match(here, sub))
		{
			result.len = i + sub.len;
			break;
		}
	}

	return result;
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
