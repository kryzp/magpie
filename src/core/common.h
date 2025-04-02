#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <string>

#ifdef MGP_DEBUG

#include <stdio.h>

// todo: use static_assert
#define MGP_ASSERT(_exp, _msg, ...) do{if(!(_exp)){::printf((_msg "\n"), ##__VA_ARGS__);*((volatile int *)0)=0;}}while(0)
#define MGP_ERROR(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);*((volatile int *)0)=0;}while(0)
#define MGP_LOG(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);}while(0)

#else

#define MGP_ASSERT(_exp, _msg)
#define MGP_ERROR(_msg)
#define MGP_LOG(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);}while(0)

#endif

#define MGP_VK_CHECK(_func_call, _error_msg) do{if(VkResult _VK_CHECK_RESULT_ABCDEFGH=_func_call;_VK_CHECK_RESULT_ABCDEFGH!=VK_SUCCESS){MGP_ERROR(_error_msg ": %d",_VK_CHECK_RESULT_ABCDEFGH);}}while(0);

#define MGP_BYTES    (_x) (_x)
#define MGP_KILOBYTES(_x) (_x * 1024LL)
#define MGP_MEGABYTES(_x) (_x * 1024LL * 1024LL)
#define MGP_GIGABYTES(_x) (_x * 1024LL * 1024LL * 1024LL)
#define MGP_TERABYTES(_x) (_x * 1024LL * 1024LL * 1024LL * 1024LL)

#define MGP_ARRAY_LENGTH(_arr) (sizeof((_arr)) / sizeof((*_arr)))
#define MGP_SWAP(_x, _y) (::__mgputils_swap((_x), (_y)))
#define MGP_SID(_str) (hash::calc(0, (_str)))

template <typename T>
inline void __mgputils_swap(T &x, T &y)
{
	T tmp = x;
	x = y;
	y = tmp;
}

using sbyte = char;
using ubyte = unsigned char;
using byte  = unsigned char;

#define cauto const auto

namespace mgp
{
	// hashing implementation
	namespace hash
	{
		// fowler noll vo hash function
		template <typename T>
		uint64_t calc(uint64_t start, const T *data)
		{
			const uint64_t prime = 0x01000193;
			const uint64_t offset = 0x811C9DC5;
			const byte *input = (const byte *)data;

			uint64_t output = start;
			for (uint64_t i = 0; i < sizeof(T); i++)
			{
				output ^= input[i];
				output *= prime;
			}

			return output ^ offset;
		}

		template <typename T>
		void combine(uint64_t *inout, const T *data)
		{
			(*inout) = calc(*inout, data);
		}

		template <typename T>
		uint64_t calc(const T *data)
		{
			return calc(0, data);
		}

		template <> uint64_t calc(uint64_t start, const char *str);
		template <> uint64_t calc(uint64_t start, const std::string *str);
	}

	// wrapper around C memory functions to make code more legible
	namespace mem
	{
		void *set(void *ptr, byte val, uint64_t size);
		void *copy(void *dst, const void *src, uint64_t size);
		void *move(void *dst, const void *src, uint64_t size);
		void *chr(void *ptr, byte val, uint64_t size);
		int compare(const void *p1, const void *p2, uint64_t size);
		bool vcompare(void *ptr, byte val, uint64_t size);
	}

	// wrapper around C string functions to make code more legible
	namespace cstr
	{
		uint64_t length(const char *str);
		char *concat(char *dst, const char *src, uint64_t size);
		char *copy(char *dst, const char *src, uint64_t size);
		int compare(const char *str1, const char *str2);
		uint64_t span(const char *str, const char *match);
		uint64_t cspan(const char *str, const char *match);
		char *token(char *str, const char *sep);
		bool isSpace(char c);
		char toUpper(char c);
		char toLower(char c);
		void fromLonglong(char *buf, uint64_t size, int64_t value);
		void fromUlonglong(char *buf, uint64_t size, uint64_t value);
		void fromInt(char *buf, uint64_t size, int value);
		void fromFloat(char *buf, uint64_t size, float value);
		void fromDouble(char *buf, uint64_t size, double value);
		long long toLonglong(const char *str);
		int toInt(const char *str);
		float toFloat(const char *str);
	}
}
