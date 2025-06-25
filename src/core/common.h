#pragma once

#include <inttypes.h>
#include <string>
#include <memory>

#ifdef MGP_DEBUG

#include <stdio.h>

// todo: use static_assert
#define mgp_ASSERT(_exp, _msg, ...) do{if(!(_exp)){::printf((_msg "\n"), ##__VA_ARGS__);*((volatile int *)0)=0;}}while(0)
#define mgp_ERROR(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);*((volatile int *)0)=0;}while(0)
#define mgp_LOG(_msg, ...) do{::printf((_msg "\n"), ##__VA_ARGS__);}while(0)

#else

#define mgp_ASSERT(_exp, _msg)
#define mgp_ERROR(_msg)
#define mgp_LOG(_msg, ...)

#endif

#define mgp_VK_CHECK(_func_call, _error_msg) do{if(VkResult _vk_check_result=_func_call;_vk_check_result!=VK_SUCCESS){mgp_ERROR(_error_msg ": %d",_vk_check_result);}}while(0);

#define mgp_BYTES    (_x) (_x)
#define mgp_KILOBYTES(_x) (_x * 1024LL)
#define mgp_MEGABYTES(_x) (_x * 1024LL * 1024LL)
#define mgp_GIGABYTES(_x) (_x * 1024LL * 1024LL * 1024LL)
#define mgp_TERABYTES(_x) (_x * 1024LL * 1024LL * 1024LL * 1024LL)

#define mgp_ARRAY_LENGTH(_arr) (sizeof((_arr)) / sizeof((*_arr)))
#define mgp_SWAP(_x, _y) (::__mgputils_swap((_x), (_y)))
#define mgp_SID(_str) (hash::calc(0, (_str)))

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

template <typename T> using Unique = std::unique_ptr<T>;
template <typename T, typename... Args>
constexpr Unique<T> createScope(Args &&...args) { return std::make_unique<T>(std::forward<Args>(args)...); }

template <typename T> using Ref = std::shared_ptr<T>;
template <typename T, typename... Args>
constexpr Ref<T> createRef(Args &&...args) { return std::make_shared<T>(std::forward<Args>(args)...); }

template <typename T> using Weak = std::weak_ptr<T>;

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
