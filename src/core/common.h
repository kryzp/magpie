#ifndef COMMON_H_
#define COMMON_H_

#include <inttypes.h>
#include <stdio.h>

#ifdef LLT_DEBUG

#include <stdio.h>

#define LLT_ASSERT(_exp, _msg, ...) do{if(!(_exp)){::printf((_msg "\n"), ##__VA_ARGS__);*((volatile int *)0)=0;}}while(0)
#define LLT_ERROR(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);*((volatile int *)0)=0;}while(0)
#define LLT_LOG(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);}while(0)

#else

#define LLT_ASSERT(_exp, _msg)
#define LLT_ERROR(_msg)
#define LLT_LOG(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);}while(0)

#endif

#define LLT_BYTES    (_x) (_x)
#define LLT_KILOBYTES(_x) (_x * 1024LL)
#define LLT_MEGABYTES(_x) (_x * 1024LL * 1024LL)
#define LLT_GIGABYTES(_x) (_x * 1024LL * 1024LL * 1024LL)
#define LLT_TERABYTES(_x) (_x * 1024LL * 1024LL * 1024LL * 1024LL)

#define LLT_ARRAY_LENGTH(_arr) (sizeof((_arr)) / sizeof((*_arr)))
#define LLT_SWAP(_x, _y) (::__lltutils_swap((_x), (_y)))
#define LLT_SID(_str) (hash::calc(0, (_str)))

template <typename T>
inline void __lltutils_swap(T &x, T &y)
{
	T tmp = x;
	x = y;
	y = tmp;
}

using sbyte = char;
using ubyte = unsigned char;
using byte  = unsigned char;

#define cauto const auto

namespace llt
{
	template <uint64_t Size> class Str;
	using String = Str<512>;

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
		template <> uint64_t calc(uint64_t start, const String *str);
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

	// magic constants
	namespace mgc
	{
		static constexpr uint64_t FRAMES_IN_FLIGHT = 3;
		static constexpr uint64_t RASTER_SHADER_COUNT = 2;
		static constexpr int MAX_SAMPLER_MIP_LEVELS = 4;
	}
}

#endif // COMMON_H_
