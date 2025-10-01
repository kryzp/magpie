#ifndef CORE_STRING_H
#define CORE_STRING_H

#include "core_types.h"

struct string8 {
	u8 *str;
	u64 len;
};

struct string8 string8_init(u8 *str, u64 len);

#define str8(s) string8_init((u8 *)(s), sizeof(s) - 1)

bool string8_match(struct string8 a, struct string8 b);
struct string8 string8_before_first_substring_from_back_inclusive(struct string8 string, struct string8 substring);

bool char_is_whitespace(char c);
bool char_is_alpha(char c);
bool char_is_lower(char c);
bool char_is_upper(char c);
bool char_is_digit(char c);
int char_to_lower(u8 c);
int char_to_upper(u8 c);

#endif // CORE_STRING_H
