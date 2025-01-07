#ifndef ARRAY_H_
#define ARRAY_H_

#include <initializer_list>
#include "../common.h"

namespace llt
{
	/**
	 * Wrapper around a c-style array that provides some
	 * utility functions and makes them easier to work with.
	 */
	template <typename T, uint64_t Size>
	class Array
	{
	public:
		Array();
		Array(std::initializer_list<T> data);
		Array(const Array& other);
		Array& operator = (const Array& other);
		~Array();

		void fill(const T& value);
		void clear();

		constexpr uint64_t memorySize() const;
		constexpr uint64_t size() const;

		T* data();
		const T* data() const;

		T* begin();
		const T* begin() const;
		T* end();
		const T* end() const;

		const T* cbegin() const;
		const T* cend() const;

		T& at(uint64_t idx);
		const T& at(uint64_t idx) const;

		T& operator [] (uint64_t idx);
		const T& operator [] (uint64_t idx) const;

	private:
		T m_buf[Size];
	};

	template <typename T, uint64_t Size>
	Array<T, Size>::Array()
		: m_buf{}
	{
	}
	
	template <typename T, uint64_t Size>
	Array<T, Size>::Array(std::initializer_list<T> data)
	{
		for (uint64_t i = 0; i < Size; i++) {
			m_buf[i] = data.begin()[i];
		}
	}
	
	template <typename T, uint64_t Size>
	Array<T, Size>::Array(const Array& other)
	{
		for (uint64_t i = 0; i < Size; i++) {
			m_buf[i] = other.m_buf[i];
		}
	}
	
	template <typename T, uint64_t Size>
	Array<T, Size>& Array<T, Size>::operator = (const Array& other)
	{
		for (uint64_t i = 0; i < Size; i++) {
			m_buf[i] = other.m_buf[i];
		}

		return *this;
	}
	
	template <typename T, uint64_t Size>
	Array<T, Size>::~Array()
	{
	}

	template <typename T, uint64_t Size>
	void Array<T, Size>::fill(const T& value)
	{
		for (int i = 0; i < Size; i++) {
			m_buf[i] = value;
		}
	}

	template <typename T, uint64_t Size>
	void Array<T, Size>::clear()
	{
		mem::set(m_buf, 0, memorySize());
	}

	template <typename T, uint64_t Size>
	constexpr uint64_t Array<T, Size>::memorySize() const
	{
		return sizeof(T) * Size;
	}

	template <typename T, uint64_t Size>
	constexpr uint64_t Array<T, Size>::size() const
	{
		return Size;
	}

	template <typename T, uint64_t Size>
	T* Array<T, Size>::data()
	{
		return m_buf;
	}

	template <typename T, uint64_t Size>
	const T* Array<T, Size>::data() const
	{
		return m_buf;
	}

	template <typename T, uint64_t Size>
	T* Array<T, Size>::begin()
	{
		return m_buf;
	}

	template <typename T, uint64_t Size>
	const T* Array<T, Size>::begin() const
	{
		return m_buf;
	}

	template <typename T, uint64_t Size>
	T* Array<T, Size>::end()
	{
		return m_buf + Size;
	}

	template <typename T, uint64_t Size>
	const T* Array<T, Size>::end() const
	{
		return m_buf + Size;
	}

	template <typename T, uint64_t Size>
	const T* Array<T, Size>::cbegin() const
	{
		return m_buf;
	}

	template <typename T, uint64_t Size>
	const T* Array<T, Size>::cend() const
	{
		return m_buf + Size;
	}

	template <typename T, uint64_t Size>
	T& Array<T, Size>::at(uint64_t idx)
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, Size);
		return m_buf[idx];
	}

	template <typename T, uint64_t Size>
	const T& Array<T, Size>::at(uint64_t idx) const
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, Size);
		return m_buf[idx];
	}
	
	template <typename T, uint64_t Size>
	T& Array<T, Size>::operator [] (uint64_t idx)
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, Size);
		return m_buf[idx];
	}
	
	template <typename T, uint64_t Size>
	const T& Array<T, Size>::operator [] (uint64_t idx) const
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, Size);
		return m_buf[idx];
	}
}

#endif // ARRAY_H_
