#ifndef VECTOR_H_
#define VECTOR_H_

#include <initializer_list>
#include <new>
#include <utility>

#include "core/common.h"

#include "function.h"

namespace llt
{
	/**
	 * Dynamically sized array.
	 */
	template <typename T>
	class Vector
	{
    public:
		class Iterator
		{
		public:
			Iterator() : m_ptr(nullptr) { }
			Iterator(T *ptr) : m_ptr(ptr) { }
			~Iterator() = default;
			T &operator * () const { return *m_ptr; }
			T *operator -> () const { return m_ptr; }
			Iterator &operator ++ () { m_ptr++; return *this; }
			Iterator &operator -- () { m_ptr--; return *this; }
			Iterator &operator ++ (int) { m_ptr++; return *this; }
			Iterator &operator -- (int) { m_ptr--; return *this; }
			bool operator == (const Iterator &other) const { return this->m_ptr == other.m_ptr; }
			bool operator != (const Iterator &other) const { return this->m_ptr != other.m_ptr; }
		private:
			T *m_ptr;
		};

		class ConstIterator
		{
		public:
			ConstIterator() : m_ptr(nullptr) { }
			ConstIterator(const T *ptr) : m_ptr(ptr) { }
			~ConstIterator() = default;
			const T &operator * () const { return *m_ptr; }
			const T *operator -> () const { return m_ptr; }
			ConstIterator &operator ++ () { m_ptr++; return *this; }
			ConstIterator &operator -- () { m_ptr--; return *this; }
			ConstIterator &operator ++ (int) { m_ptr++; return *this; }
			ConstIterator &operator -- (int) { m_ptr--; return *this; }
			bool operator == (const ConstIterator &other) const { return this->m_ptr == other.m_ptr; }
			bool operator != (const ConstIterator &other) const { return this->m_ptr != other.m_ptr; }
		private:
			const T *m_ptr;
		};

		class ReverseIterator
		{
		public:
			ReverseIterator() : m_ptr(nullptr) { }
			ReverseIterator(T *ptr) : m_ptr(ptr) { }
			~ReverseIterator() = default;
			T &operator * () const { return *m_ptr; }
			T *operator -> () const { return m_ptr; }
			ReverseIterator &operator ++ () { m_ptr--; return *this; }
			ReverseIterator &operator -- () { m_ptr++; return *this; }
			ReverseIterator &operator ++ (int) { m_ptr--; return *this; }
			ReverseIterator &operator -- (int) { m_ptr++; return *this; }
			bool operator == (const ReverseIterator &other) const { return this->m_ptr == other.m_ptr; }
			bool operator != (const ReverseIterator &other) const { return this->m_ptr != other.m_ptr; }
		private:
			T *m_ptr;
		};

		class ReverseConstIterator
		{
		public:
			ReverseConstIterator() : m_ptr(nullptr) { }
			ReverseConstIterator(const T *ptr) : m_ptr(ptr) { }
			~ReverseConstIterator() = default;
			const T &operator * () const { return *m_ptr; }
			const T *operator -> () const { return m_ptr; }
			ReverseConstIterator &operator ++ () { m_ptr--; return *this; }
			ReverseConstIterator &operator -- () { m_ptr++; return *this; }
			ReverseConstIterator &operator ++ (int) { m_ptr--; return *this; }
			ReverseConstIterator &operator -- (int) { m_ptr++; return *this; }
			bool operator == (const ReverseConstIterator &other) const { return this->m_ptr == other.m_ptr; }
			bool operator != (const ReverseConstIterator &other) const { return this->m_ptr != other.m_ptr; }
		private:
			const T *m_ptr;
		};

        Vector();
        Vector(std::initializer_list<T> data);
        Vector(uint64_t initialCapacity);
        Vector(uint64_t initialCapacity, const T &initialElement);
		Vector(T *buf, uint64_t length);
		Vector(const Iterator &begin, const Iterator &end);
        Vector(const Vector &other);
        Vector(Vector &&other) noexcept;

        Vector &operator = (const Vector &other);
        Vector &operator = (Vector &&other) noexcept;

        ~Vector();

		/*
		 * Allocate a set amount of space.
		 */
        void allocate(uint64_t capacity);

		/*
		 * Resize (expand/erase) to a new size.
		 */
        void resize(uint64_t newSize);
        void expand(uint64_t amount = 1);

		/*
		 * Fill with a certain value.
		 */
		void fill(byte value);

		void erase(uint64_t index, uint64_t amount = 1);
		void erase(Iterator it, uint64_t amount = 1);

		Iterator find(const T &item);

		T *data();
		const T *data() const;

		Iterator insert(int index, const T &item);

		/*
		 * Add a new element to the front.
		 */
        Iterator pushFront(const T &item);

		/*
		 * Add a new element to the back.
		 */
        Iterator pushBack(const T &item);

		/*
		 * Remove the front element.
		 */
        T popFront();

		/*
		 * Remove the last element.
		 */
		T popBack();

		/*
		 * Emplace a new element to the front.
		 */
		template <typename... Args>
		Iterator emplaceFront(Args&&... args);

		/*
		 * Emplace a new element to the back.
		 */
		template <typename... Args>
		Iterator emplaceBack(Args&&... args);

        void clear();
        uint64_t size() const;

		/*
		 * Check if there are any elements in the vector.
		 */
		bool any() const;
		bool empty() const;

		T &front();
		const T &front() const;
		T &back();
		const T &back() const;

        Iterator begin();
        ConstIterator begin() const;
        Iterator end();
        ConstIterator end() const;

		ReverseIterator rbegin();
		ReverseConstIterator rbegin() const;
		ReverseIterator rend();
		ReverseConstIterator rend() const;

		ConstIterator cbegin() const;
		ConstIterator cend() const;
		ReverseConstIterator crbegin() const;
		ReverseConstIterator crend() const;

		T &at(uint64_t idx);
        const T &at(uint64_t idx) const;

        T &operator [] (uint64_t idx);
        const T &operator [] (uint64_t idx) const;

    private:
        T *m_buf;
        uint64_t m_size;
        uint64_t m_capacity;
	};

    template <typename T>
    Vector<T>::Vector()
		: m_buf(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
    }
    
    template <typename T>
    Vector<T>::Vector(std::initializer_list<T> data)
        : Vector()
    {
        allocate(data.size());
        m_size = data.size();

        for (uint64_t i = 0; i < m_size; i++) {
            new (m_buf + i) T(data.begin()[i]);
        }
    }

    template <typename T>
    Vector<T>::Vector(uint64_t initialCapacity)
        : Vector()
    {
        allocate(initialCapacity);
        m_size = initialCapacity;

        for (uint64_t i = 0; i < m_capacity; i++) {
            new (m_buf + i) T();
        }
    }

    template <typename T>
    Vector<T>::Vector(uint64_t initialCapacity, const T &initialElement)
        : Vector()
    {
        allocate(initialCapacity);
        m_size = initialCapacity;

        for (uint64_t i = 0; i < m_capacity; i++) {
            new (m_buf + i) T(initialElement);
        }
    }

	template <typename T>
	Vector<T>::Vector(T *buf, uint64_t length)
		: Vector()
	{
		allocate(length);
		m_size = length;

		for (uint64_t i = 0; i < length; i++) {
			new (m_buf + i) T(buf[i]);
        }
	}

	template <typename T>
	Vector<T>::Vector(const Iterator &begin, const Iterator &end)
		: Vector()
	{
		allocate(end.m_ptr - begin.m_ptr);
		m_size = end.m_ptr - begin.m_ptr;

		for (uint64_t i = 0; i < m_size; i++) {
			new (m_buf + i) T(begin.m_ptr[i]);
        }
	}

    template <typename T>
    Vector<T>::Vector(const Vector &other)
        : Vector()
    {
        if (other.m_capacity <= 0) {
			return;
		}

		allocate(other.m_capacity);
		clear();
		m_size = other.size();

		for (uint64_t i = 0; i < other.m_size; i++) {
			new (m_buf + i) T(other.m_buf[i]);
		}
    }

    template <typename T>
    Vector<T>::Vector(Vector &&other) noexcept
		: Vector()
    {
        this->m_capacity = std::move(other.m_capacity);
        this->m_size = std::move(other.m_size);
        this->m_buf = std::move(other.m_buf);

        other.m_capacity = 0;
        other.m_size = 0;
        other.m_buf = nullptr;
    }
    
    template <typename T>
    Vector<T>& Vector<T>::operator = (const Vector &other)
    {
		allocate(other.m_capacity);
		clear();

		m_size = other.m_size;

		for (uint64_t i = 0; i < other.m_size; i++) {
			new (m_buf + i) T(other.m_buf[i]);
		}

        return *this;
    }
    
    template <typename T>
    Vector<T>& Vector<T>::operator = (Vector &&other) noexcept
    {
		clear();

		if (m_buf) {
			::operator delete (m_buf, sizeof(T) * m_capacity);
		}

		this->m_capacity = std::move(other.m_capacity);
		this->m_size = std::move(other.m_size);
		this->m_buf = std::move(other.m_buf);

		other.m_capacity = 0;
		other.m_size = 0;
		other.m_buf = nullptr;

        return *this;
    }

    template <typename T>
    Vector<T>::~Vector()
    {
        clear();

		if (m_buf) {
			::operator delete (m_buf, sizeof(T) * m_capacity);
		}

        m_buf = nullptr;
        m_capacity = 0;
        m_size = 0;
    }

    template <typename T>
    void Vector<T>::clear()
    {
        for (int i = 0; i < m_size; i++) {
            m_buf[i].~T();
		}

		mem::set(m_buf, 0, m_size * sizeof(T));
        m_size = 0;
    }

    template <typename T>
    void Vector<T>::allocate(uint64_t capacity)
    {
		// check if we even need to allocate more
        if (capacity <= m_capacity) {
			return;
		}

		uint64_t newCapacity = m_capacity > 0
			? m_capacity
			: 8;

		// keep doubling our capacity until we are greater than our current capacity
		while (newCapacity < capacity) {
			newCapacity *= 2;
		}

		// allocate a new command buffer
		T *newBuffer = (T*)::operator new (sizeof(T) * newCapacity);
		mem::set(newBuffer, 0, sizeof(T) * newCapacity);

		// move all of our elements into the new buffer
		for (int i = 0; i < m_size; i++) {
			new (newBuffer + i) T(std::move(m_buf[i]));
		}

		// destroy our old buffer
		if (m_buf) {
			::operator delete (m_buf, sizeof(T) * m_capacity);
		}

		// update our old buffer and capacity to new buffer and capacity
		m_buf = newBuffer;
		m_capacity = newCapacity;
    }

    template <typename T>
    void Vector<T>::resize(uint64_t newSize)
    {
        if (newSize < m_size) {
            erase(newSize, m_size - newSize);
        } else if (newSize > m_size) {
            expand(newSize - m_size);
        }

		m_size = newSize;
    }

    template <typename T>
    void Vector<T>::expand(uint64_t amount)
    {
        LLT_ASSERT(amount > 0, "Expand amount must be higher than 0");

        allocate(m_size + amount);

        for (int i = 0; i < amount; i++) {
            new (m_buf + m_size + i) T();
        }
    }

    template <typename T>
    void Vector<T>::fill(byte value)
    {
        mem::set(m_buf, value, sizeof(T) * m_capacity);
    }

	template <typename T>
	void Vector<T>::erase(uint64_t index, uint64_t amount)
	{
		if (amount <= 0) {
			return;
		}

		for (uint64_t i = 0; i < amount; i++) {
			m_buf[i + index].~T();
		}

		// shuffle all of our elements back
		for (uint64_t i = 0; i < m_size - amount; i++) {
			m_buf[i + index] = std::move(m_buf[i + index + amount]);
        }

		m_size -= amount;
	}

	template <typename T>
	void Vector<T>::erase(Iterator it, uint64_t amount)
	{
		if (amount <= 0) {
			return;
		}

		// shuffle all of our elements back
		for (int i = 0; i < m_size - amount; i++) {
			*(it.m_ptr + i) = std::move(*(it.m_ptr + i + amount));
        }

		for (int i = m_size - amount; i < m_size; i++) {
			it->~T();
        }

		m_size -= amount;
	}

	template <typename T>
	typename Vector<T>::Iterator Vector<T>::find(const T &item)
	{
		// linear search
		for (uint64_t i = 0; i < m_size; i++) {
			if (m_buf[i] == item) {
				return Iterator(&m_buf[i]);
			}
		}

		return end();
	}

	template <typename T>
	Vector<T>::Iterator Vector<T>::insert(int index, const T &item)
	{
		resize(m_size + 1);
		mem::move(m_buf + index + 1, m_buf + index, sizeof(T) * (m_size - index));
		new (m_buf + index) T(std::move(item));
		return Iterator(m_buf + index);
	}

    template <typename T>
	Vector<T>::Iterator Vector<T>::pushFront(const T &item)
    {
        resize(m_size + 1);
        mem::move(m_buf + 1, m_buf, sizeof(T) * m_size);
        new (m_buf) T(std::move(item));
		return Iterator(m_buf);
    }

    template <typename T>
	Vector<T>::Iterator Vector<T>::pushBack(const T &item)
    {
        resize(m_size + 1);
        new (m_buf + m_size - 1) T(std::move(item));
		return Iterator(m_buf + m_size - 1);
    }

    template <typename T>
    T Vector<T>::popFront()
    {
        T item = std::move(m_buf[0]);
        m_buf[0].~T();
		mem::move(m_buf, m_buf + 1, m_size - 1);
        m_size--;
		return item;
    }

    template <typename T>
    T Vector<T>::popBack()
    {
        T item = std::move(m_buf[m_size - 1]);
        m_buf[m_size - 1].~T();
        m_size--;
		return item;
    }

	template <typename T>
	template <typename... Args>
	Vector<T>::Iterator Vector<T>::emplaceFront(Args&&... args)
	{
		resize(m_size + 1);
		mem::move(m_buf + 1, m_buf, sizeof(T) * (m_size - 1));
		new (m_buf) T(std::forward<Args>(args)...);
		return Iterator(m_buf);
	}

	template <typename T>
	template <typename... Args>
	Vector<T>::Iterator Vector<T>::emplaceBack(Args&&... args)
	{
		resize(m_size + 1);
		new (m_buf + m_size - 1) T(std::forward<Args>(args)...);
		return Iterator(m_buf + m_size - 1);
	}

	template <typename T>
	T *Vector<T>::data()
	{
		return m_buf;
	}

	template <typename T>
	const T *Vector<T>::data() const
	{
		return m_buf;
	}

	template <typename T>
	T &Vector<T>::front()
	{
		return m_buf[0];
	}

	template <typename T>
	const T &Vector<T>::front() const
	{
		return m_buf[0];
	}

	template <typename T>
	T &Vector<T>::back()
	{
		return m_buf[m_size - 1];
	}

	template <typename T>
	const T &Vector<T>::back() const
	{
		return m_buf[m_size - 1];
	}

    template <typename T>
    uint64_t Vector<T>::size() const
    {
        return m_size;
    }

	template <typename T>
	bool Vector<T>::any() const
	{
		return m_size != 0;
	}

	template <typename T>
	bool Vector<T>::empty() const
	{
		return m_size == 0;
	}

	template <typename T>
	typename Vector<T>::Iterator Vector<T>::begin()
	{
		return Iterator(m_buf);
	}

	template <typename T>
	typename Vector<T>::ConstIterator Vector<T>::begin() const
	{
		return ConstIterator(m_buf);
	}

	template <typename T>
	typename Vector<T>::Iterator Vector<T>::end()
	{
		return Iterator(m_buf + m_size);
	}

	template <typename T>
	typename Vector<T>::ConstIterator Vector<T>::end() const
	{
		return ConstIterator(m_buf + m_size);
	}

	template <typename T>
	typename Vector<T>::ReverseIterator Vector<T>::rbegin()
	{
		return ReverseIterator(m_buf + m_size - 1);
	}

	template <typename T>
	typename Vector<T>::ReverseConstIterator Vector<T>::rbegin() const
	{
		return ReverseConstIterator(m_buf + m_size - 1);
	}

	template <typename T>
	typename Vector<T>::ReverseIterator Vector<T>::rend()
	{
		return ReverseIterator(m_buf - 1);
	}

	template <typename T>
	typename Vector<T>::ReverseConstIterator Vector<T>::rend() const
	{
		return ReverseConstIterator(m_buf - 1);
	}

	template <typename T>
	typename Vector<T>::ConstIterator Vector<T>::cbegin() const
	{
		return ConstIterator(m_buf);
	}

	template <typename T>
	typename Vector<T>::ConstIterator Vector<T>::cend() const
	{
		return ConstIterator(m_buf + m_size);
	}

	template <typename T>
	typename Vector<T>::ReverseConstIterator Vector<T>::crbegin() const
	{
		return ReverseConstIterator(m_buf + m_size - 1);
	}

	template <typename T>
	typename Vector<T>::ReverseConstIterator Vector<T>::crend() const
	{
		return ReverseConstIterator(m_buf - 1);
	}

    template <typename T>
    T &Vector<T>::at(uint64_t idx)
    {
		LLT_ASSERT(idx >= 0 && idx < m_size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, m_size);
        return m_buf[idx];
    }

    template <typename T>
    const T &Vector<T>::at(uint64_t idx) const
    {
		LLT_ASSERT(idx >= 0 && idx < m_size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, m_size);
        return m_buf[idx];
    }

    template <typename T>
    T &Vector<T>::operator [] (uint64_t idx)
    {
		LLT_ASSERT(idx >= 0 && idx < m_size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, m_size);
        return m_buf[idx];
    }

    template <typename T>
    const T &Vector<T>::operator [] (uint64_t idx) const
    {
		LLT_ASSERT(idx >= 0 && idx < m_size, "Index must be within bounds: INDEX=%llu, SIZE=%llu", idx, m_size);
        return m_buf[idx];
    }
}

#endif // VECTOR_H_
