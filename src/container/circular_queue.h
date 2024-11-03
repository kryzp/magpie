#ifndef CIRCULAR_QUEUE_H_
#define CIRCULAR_QUEUE_H_

#include "../common.h"

namespace rvn
{
	/**
	 * Circular queue to which you can add
	 * new objects or pick the first one out of the queue,
	 * which then wraps back around after max capacity is reached,
	 * overwriting older items.
	 */
	template <typename T, uint64_t Capacity>
	class CircularQueue
	{
	public:
		CircularQueue();

		CircularQueue(const CircularQueue& other);
		CircularQueue(CircularQueue&& other) noexcept;

		CircularQueue& operator = (const CircularQueue& other);
		CircularQueue& operator = (CircularQueue&& other) noexcept;

		~CircularQueue();

		T& front();
		const T& front() const;
		T& back();
		const T& back() const;

		void clear();
		bool empty() const;
		bool full() const;

		uint64_t size() const;
		constexpr uint64_t getCapacity() const;

		T& push(const T& item);
		T pop();

		template <typename... Args>
		T& emplace(Args&&... args);

	private:
		int64_t m_front;
		int64_t m_rear;
		uint64_t m_size;
		T* m_buf;
	};

	template <typename T, uint64_t Capacity>
	CircularQueue<T, Capacity>::CircularQueue()
		: m_front(0)
		, m_rear(-1)
		, m_size(0)
		, m_buf(nullptr)
	{
		m_buf = new T[Capacity];
	}

	template <typename T, uint64_t Capacity>
	CircularQueue<T, Capacity>::CircularQueue(const CircularQueue<T, Capacity>& other)
	{
	}

	template <typename T, uint64_t Capacity>
	CircularQueue<T, Capacity>::CircularQueue(CircularQueue<T, Capacity>&& other) noexcept
	{
	}

	template <typename T, uint64_t Capacity>
	CircularQueue<T, Capacity>& CircularQueue<T, Capacity>::operator = (const CircularQueue<T, Capacity>& other)
	{
		return *this;
	}

	template <typename T, uint64_t Capacity>
	CircularQueue<T, Capacity>& CircularQueue<T, Capacity>::operator = (CircularQueue<T, Capacity>&& other) noexcept
	{
		return *this;
	}

	template <typename T, uint64_t Capacity>
	CircularQueue<T, Capacity>::~CircularQueue()
	{
		clear();

		if (m_buf) {
			::operator delete (m_buf, sizeof(T) * Capacity);
		}

		m_buf = nullptr;
		m_size = 0;
		m_front = 0;
		m_rear = -1;
	}

	template <typename T, uint64_t Capacity>
	T& CircularQueue<T, Capacity>::front()
	{
		return m_buf[m_front];
	}

	template <typename T, uint64_t Capacity>
	const T& CircularQueue<T, Capacity>::front() const
	{
		return m_buf[m_front];
	}

	template <typename T, uint64_t Capacity>
	T& CircularQueue<T, Capacity>::back()
	{
		return m_buf[m_rear];
	}

	template <typename T, uint64_t Capacity>
	const T& CircularQueue<T, Capacity>::back() const
	{
		return m_buf[m_rear];
	}

	template <typename T, uint64_t Capacity>
	void CircularQueue<T, Capacity>::clear()
	{
		for (int i = 0; i < m_size; i++) {
			m_buf[i].~T();
		}

		mem::set(m_buf, 0, m_size * sizeof(T));
		m_size = 0;

		m_front = 0;
		m_rear = -1;
	}

	template <typename T, uint64_t Capacity>
	T CircularQueue<T, Capacity>::pop()
	{
		LLT_ASSERT(m_size > 0, "[CIRQUE|DEBUG] must contain at least one element!");
		int64_t tmp = m_front;
		m_front = (m_front + 1) % Capacity;
		m_size--;
		return m_buf[tmp];
	}

	template <typename T, uint64_t Capacity>
	T& CircularQueue<T, Capacity>::push(const T& item)
	{
		m_rear = (m_rear + 1) % Capacity;
		m_buf[m_rear] = item;
		m_size++;
		return m_buf[m_rear];
	}

	template <typename T, uint64_t Capacity>
	template <typename... Args>
	T& CircularQueue<T, Capacity>::emplace(Args&&... args)
	{
		m_rear = (m_rear + 1) % Capacity;
		new (m_buf + m_rear) T(std::forward<Args>(args)...);
		m_size++;
		return m_buf[m_rear];
	}

	template <typename T, uint64_t Capacity>
	bool CircularQueue<T, Capacity>::empty() const
	{
		return m_size == 0;
	}

	template <typename T, uint64_t Capacity>
	bool CircularQueue<T, Capacity>::full() const
	{
		return m_size == Capacity;
	}

	template <typename T, uint64_t Capacity>
	uint64_t CircularQueue<T, Capacity>::size() const
	{
		return m_size;
	}

	template <typename T, uint64_t Capacity>
	constexpr uint64_t CircularQueue<T, Capacity>::getCapacity() const
	{
		return Capacity;
	}
}

#endif // CIRCULAR_QUEUE_H_
