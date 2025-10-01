#include "core_string.h"

struct string8 string8_init(u8 *str, u64 len)
{
	struct string8 s = {0};
	s.str = str;
	s.len = len;

	return s;
}

bool string8_match(struct string8 a, struct string8 b)
{
	if (a.len != b.len)
		return false;

	for (s32 i = 0; i < a.len; i++) {
		if (a.str[i] != b.str[i])
			return false;
	}

	return true;
}

struct string8 string8_before_first_substring_from_back_inclusive(struct string8 string, struct string8 substring)
{
	struct string8 result = {0};
	result.str = string.str;
	result.len = 0;

	for (u64 i = string.len - substring.len - 1; i >= 0; i--) {
		struct string8 here = string8_init(string.str + i, substring.len);

		if (string8_match(here, substring)) {
			result.len = i + substring.len;
			break;
		}
	}

	return result;
}

bool char_is_whitespace(char c)
{
	return c <= 32;
}

bool char_is_alpha(char c)
{
	return ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z'));
}

bool char_is_lower(char c)
{
	return c >= 'a' && c <= 'z';
}

bool char_is_upper(char c)
{
	return c >= 'A' && c <= 'Z';
}

bool char_is_digit(char c)
{
	return c >= '0' && c <= '9';
}

int char_to_lower(u8 c)
{
	if (char_is_upper(c))
		c += 32;

	return c;
}

int char_to_upper(u8 c)
{
	if (char_is_lower(c))
		c -= 32;

	return c;
}
