#include "common.h"
#include "container/string.h"

#include <string.h>
#include <stdlib.h>
#include <cctype>

using namespace llt;

template <>
uint64_t hash::calc(uint64_t start, const char* str)
{
	uint64_t hash = start + 7521;
	for (int i = 0; str[i] != '\0'; i++) {
		hash = ((hash << 5) + hash) + str[i];
	}
	return hash;
}

template <>
uint64_t hash::calc(uint64_t start, const String* str)
{
	return calc(start, str->cstr());
}

void* mem::set(void* ptr, byte val, uint64_t size)
{
	return ::memset(ptr, val, size);
}

void* mem::copy(void* dst, const void* src, uint64_t size)
{
	return ::memcpy(dst, src, size);
}

void* mem::move(void* dst, const void* src, uint64_t size)
{
	return ::memmove(dst, src, size);
}

void* mem::chr(void* ptr, byte val, uint64_t size)
{
	return ::memchr(ptr, val, size);
}

int mem::compare(const void* p1, const void* p2, uint64_t size)
{
	return ::memcmp(p1, p2, size);
}

bool mem::vcompare(void* ptr, byte val, uint64_t size)
{
	return (
		(*((byte*)ptr)) == val &&
		::memcmp(ptr, ((byte*)ptr) + sizeof(byte), size - 1) == 0
	);
}

uint64_t cstr::length(const char* str)
{
	return ::strlen(str);
}

char* cstr::concat(char* dst, const char* src, uint64_t size)
{
	return ::strncat(dst, src, size);
}

char* cstr::copy(char* dst, const char* src, uint64_t size)
{
	return ::strncpy(dst, src, size);
}

int cstr::compare(const char* str1, const char* str2)
{
	return ::strcmp(str1, str2);
}

uint64_t cstr::span(const char* str, const char* match)
{
	return ::strspn(str, match);
}

uint64_t cstr::cspan(const char* str, const char* match)
{
	return ::strcspn(str, match);
}

char* cstr::token(char* str, const char* delimiter)
{
	return ::strtok(str, delimiter);
}

bool cstr::isSpace(char c)
{
	return ::isspace(c);
}

char cstr::toUpper(char c)
{
	return (char)::toupper(c);
}

char cstr::toLower(char c)
{
	return (char)::tolower(c);
}

void cstr::fromLonglong(char* buf, uint64_t size, int64_t value)
{
	::snprintf(buf, size, "%lld", value);
}

void cstr::fromUlonglong(char* buf, uint64_t size, uint64_t value)
{
	::snprintf(buf, size, "%llu", value);
}

void cstr::fromInt(char* buf, uint64_t size, int value)
{
	::snprintf(buf, size, "%d", value);
}

void cstr::fromFloat(char* buf, uint64_t size, float value)
{
	::snprintf(buf, size, "%f", value);
}

void cstr::fromDouble(char* buf, uint64_t size, double value)
{
	::snprintf(buf, size, "%f", value);
}

long long cstr::toLonglong(const char* str)
{
	return ::atoll(str);
}

int cstr::toInt(const char* str)
{
	return ::atoi(str);
}

float cstr::toFloat(const char* str)
{
	return ::atof(str);
}
