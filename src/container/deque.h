#ifndef DEQUE_H_
#define DEQUE_H_

#include <new>
#include <memory>

#include "core/common.h"

namespace llt
{
	/**
	 * Dynamically-resizing double ended queue to which you can add
	 * new objects or pick the first one out of the queue.
	 */
	template <typename T, uint64_t ChunkSize = 64>
	class Deque
	{
		enum
		{
			EXPAND_FRONT,
			EXPAND_BACK
		};

	public:
		class Iterator
		{
			friend class Deque<T, ChunkSize>;

		public:
			Iterator() : m_cur(nullptr), m_first(nullptr), m_last(nullptr), m_chunk(nullptr) { }
			Iterator(T *curr, T *first, T *last, T **chunk) : m_cur(curr), m_first(first), m_last(last), m_chunk(chunk) { }
			Iterator(T **chunk) : m_cur(nullptr), m_first(nullptr), m_last(nullptr), m_chunk(chunk) { setChunk(chunk); }
			~Iterator() = default;

			T &operator * () const { return *m_cur; }
			T *operator -> () const { return m_cur; }

			T *get() { return m_cur; }
			const T *get() const { return m_cur; }

			void setChunk(T **chunk) {
				m_chunk = chunk;
				m_first = *chunk;
				m_last = m_first + ChunkSize;
			}

			Iterator &operator ++ () {
				m_cur++;
				if (m_cur == m_last) {
					setChunk(m_chunk + 1);
					m_cur = m_first;
				}
				return *this;
			}

			Iterator &operator -- () {
				if (m_cur == m_first) {
					setChunk(m_chunk - 1);
					m_cur = m_last;
				}
				m_cur--;
				return *this;
			}

			Iterator operator ++ (int) {
				Iterator t = *this;
				++(*this);
				return t;
			}

			Iterator operator -- (int) {
				Iterator t = *this;
				--(*this);
				return t;
			}

			Iterator &operator += (int64_t d) {
				int64_t offset = d + m_cur - m_first; // calculate our chunk offset
				if (offset >= 0 && offset < ChunkSize) {
					m_cur += d; // if we're not crossing over a chunk, just add it on
				} else {
					// calculate the number of chunks which we just crossed over
					int64_t chunkOffset = 0;
					if (offset > 0) {
						chunkOffset = offset / ChunkSize;
					} else {
						chunkOffset = ((offset + 1) / ChunkSize) - 1;
					}
					setChunk(m_chunk + chunkOffset); // update our chunk accordingly
					m_cur = m_first + (offset - (chunkOffset * ChunkSize)); // set our current location relative to the chunk
				}
				return *this;
			}

			Iterator &operator -= (int64_t d) {
				return (*this) += -d;
			}

			Iterator operator + (int64_t d) const {
				Iterator t = *this;
				return t += d;
			}

			Iterator operator - (int64_t d) const {
				Iterator t = *this;
				return t -= d;
			}

			T &operator [] (int64_t d) const {
				return *((*this) + d);
			}

			bool operator == (const Iterator &other) const { return this->m_cur == other.m_cur && this->m_first == other.m_first && this->m_last == other.m_last && this->m_chunk == other.m_chunk; }
			bool operator != (const Iterator &other) const { return this->m_cur != other.m_cur || this->m_first != other.m_first || this->m_last != other.m_last || this->m_chunk != other.m_chunk; }

		private:
			T *m_cur;
			T *m_first;
			T *m_last;
			T **m_chunk;
		};

		Deque(int initialCapacity = 8);

		Deque(const Deque &other);
		Deque(Deque &&other) noexcept;

		Deque &operator = (const Deque &other);
		Deque &operator = (Deque &&other) noexcept;

		~Deque();

		void clear();
		bool empty() const;
		uint64_t size() const;
		void swap(Deque &other);

		T &pushBack(const T &item);
		T &pushFront(const T &item);

		T popBack();
		T popFront();

		template <typename... Args>
		T &emplaceFront(Args&&... args);

		template <typename... Args>
		T &emplaceBack(Args&&... args);

		T &front();
		const T &front() const;
		T &back();
		const T &back() const;

		Iterator begin();
		Iterator end();

		T &at(uint64_t idx);
		const T &at(uint64_t idx) const;

		T &operator [] (uint64_t idx);
		const T &operator [] (uint64_t idx) const;

	private:
		void increaseSize(int frontOrBack);
		void expandFront();
		void expandBack();

		constexpr int chunks() const;

		Iterator m_begin;
		Iterator m_end;

		T **m_map;
		uint64_t m_size;
		uint64_t m_capacity;
	};

	template <typename T, uint64_t ChunkSize>
	Deque<T, ChunkSize>::Deque(int initialCapacity)
		: m_begin()
		, m_end()
		, m_map(nullptr)
		, m_size(0)
		, m_capacity(0)
	{
		T **newMap = (T **)::operator new (sizeof(T *) * initialCapacity);
		mem::set(newMap, 0, sizeof(T *) * initialCapacity);

		T **base = newMap + (initialCapacity / 2);

		// allocate the initial capacity of chunks
		for (int j = 0; j < initialCapacity; j++) {
			if (!newMap[j]) {
				newMap[j] = (T *)::operator new (sizeof(T) * ChunkSize);
			}
		}

		m_map = newMap;
		m_capacity = initialCapacity * ChunkSize;

		m_begin.setChunk(base);
		m_begin.m_cur = m_begin.m_first;

		m_end.setChunk(base);
		m_end.m_cur = m_end.m_first + 1;
	}

	template <typename T, uint64_t ChunkSize>
	Deque<T, ChunkSize>::Deque(const Deque &other)
		: Deque()
	{
		clear();

		// add all the chunks from the other double ended queue
		for (T *cur = other.m_begin.m_cur + 1; cur < other.m_end.m_cur; cur++) {
			pushBack(*cur);
		}
	}

	template <typename T, uint64_t ChunkSize>
	Deque<T, ChunkSize>::Deque(Deque &&other) noexcept
		: Deque()
	{
		clear();
		swap(other);
	}

	template <typename T, uint64_t ChunkSize>
	Deque<T, ChunkSize>& Deque<T, ChunkSize>::operator = (const Deque &other)
	{
		clear();

		// add all the chunks from the other double ended queue
		for (T *cur = other.m_begin.m_cur + 1; cur < other.m_end.m_cur; cur++) {
			pushBack(*cur);
		}

		return *this;
	}

	template <typename T, uint64_t ChunkSize>
	Deque<T, ChunkSize>& Deque<T, ChunkSize>::operator = (Deque &&other) noexcept
	{
		clear();
		swap(other);

		return *this;
	}

	template <typename T, uint64_t ChunkSize>
	Deque<T, ChunkSize>::~Deque()
	{
		clear();

		for (int i = 0; i < chunks(); i++) {
			::operator delete (m_map[i], sizeof(T) * ChunkSize);
		}

		::operator delete (m_map, sizeof(T *) * chunks());

		m_map = nullptr;
		m_capacity = 0;
		m_size = 0;
	}

	template <typename T, uint64_t ChunkSize>
	void Deque<T, ChunkSize>::clear()
	{
		for (Iterator ptr = m_begin + 1; ptr.m_cur <= m_end.m_cur; ptr++) {
			ptr.get()->~T();
		}

		for (int i = 0; i < chunks(); i++) {
			mem::set(m_map[i], 0, sizeof(T) * ChunkSize);
		}

		m_size = 0;

		T **base  = m_map + (chunks() / 2);

		m_begin.setChunk(base);
		m_begin.m_cur = m_begin.m_first;

		m_end.setChunk(base);
		m_end.m_cur = m_end.m_first + 1;
	}

	template <typename T, uint64_t ChunkSize>
	void Deque<T, ChunkSize>::increaseSize(int frontOrBack)
	{
		// double our number of chunks
		uint64_t numChunks = chunks() * 2;

		// allocate our map!
		T **newMap = (T**)::operator new (sizeof(T *) * numChunks);
		mem::set(newMap, 0, sizeof(T *) * numChunks);

		int begin_offset = m_begin	.m_cur   - m_begin	.m_first;
		int end_offset   = m_end  	.m_cur   - m_end	.m_first;
		int begin_chunk  = m_begin	.m_chunk - m_map			;
		int end_chunk    = m_end	.m_chunk - m_begin	.m_chunk;

		// copy our data map to specific regions depending on what direction we are allocating in
		if (frontOrBack == EXPAND_FRONT) {
			mem::copy(newMap + chunks(), m_map, sizeof(T *) * chunks());
		} else if (frontOrBack == EXPAND_BACK) {
			mem::copy(newMap, m_map, sizeof(T *) * chunks());
		}

		// allocate the new size
		for (int j = 0; j < numChunks; j++) {
			if (!newMap[j]) {
				newMap[j] = (T *)::operator new (sizeof(T) * ChunkSize);
			}
		}

		// set our begin and end iterator chunks to the new
		// expended-to locations depending on the direction
		// in which we are allocating.
		if (frontOrBack == EXPAND_FRONT) {
			m_begin.setChunk(newMap + chunks());
			m_end  .setChunk(newMap + chunks() + end_chunk);
		} else if (frontOrBack == EXPAND_BACK) {
			m_begin.setChunk(newMap + begin_chunk + 1);
			m_end  .setChunk(newMap + chunks() - 1);
		}

		m_begin.m_cur = m_begin.m_first + begin_offset;
		m_end  .m_cur = m_end  .m_first + end_offset;

		// destroy our old map
		::operator delete (m_map, sizeof(T *) * m_capacity);

		m_map = newMap;
		m_capacity *= 2;
	}

	template <typename T, uint64_t ChunkSize>
	void Deque<T, ChunkSize>::expandFront()
	{
		if ((m_begin - 1).m_chunk <= m_map) {
			increaseSize(EXPAND_FRONT);
		}
		m_size++;
	}

	template <typename T, uint64_t ChunkSize>
	void Deque<T, ChunkSize>::expandBack()
	{
		if ((m_end + 1).m_chunk >= (m_map + chunks())) {
			increaseSize(EXPAND_BACK);
		}
		m_size++;
	}

	template <typename T, uint64_t ChunkSize>
	constexpr int Deque<T, ChunkSize>::chunks() const
	{
		return m_capacity / ChunkSize;
	}

	template <typename T, uint64_t ChunkSize>
	bool Deque<T, ChunkSize>::empty() const
	{
		return m_size == 0;
	}

	template <typename T, uint64_t ChunkSize>
	uint64_t Deque<T, ChunkSize>::size() const
	{
		return m_size;
	}

	template <typename T, uint64_t ChunkSize>
	void Deque<T, ChunkSize>::swap(Deque &other)
	{
		Iterator t_begin = this->m_begin;
		Iterator t_end = this->m_end;
		T **t_map = this->m_map;
		uint64_t t_size = this->m_size;
		uint64_t t_capacity = this->m_capacity;

		this->m_begin = other.m_begin;
		this->m_end = other.m_end;
		this->m_map = other.m_map;
		this->m_size = other.m_size;
		this->m_capacity = other.m_capacity;

		other.m_begin = t_begin;
		other.m_end = t_end;
		other.m_map = t_map;
		other.m_size = t_size;
		other.m_capacity = t_capacity;
	}

	template <typename T, uint64_t ChunkSize>
	T &Deque<T, ChunkSize>::pushFront(const T &item)
	{
		expandFront();
		(*m_begin) = std::move(item);
		m_begin--;
		return *(m_begin + 1);
	}

	template <typename T, uint64_t ChunkSize>
	T &Deque<T, ChunkSize>::pushBack(const T &item)
	{
		expandBack();
		(*m_end) = std::move(item);
		m_end++;
		return *(m_end - 1);
	}

	template <typename T, uint64_t ChunkSize>
	T Deque<T, ChunkSize>::popFront()
	{
		LLT_ASSERT(m_size > 0, "Deque must not be empty!");
		m_size--;
		m_begin++;
		return *m_begin;
	}

	template <typename T, uint64_t ChunkSize>
	T Deque<T, ChunkSize>::popBack()
	{
		LLT_ASSERT(m_size > 0, "Deque must not be empty!");
		m_size--;
		m_end--;
		return *m_end;
	}

	template <typename T, uint64_t ChunkSize>
	template <typename... Args>
	T &Deque<T, ChunkSize>::emplaceFront(Args&&... args)
	{
		expandFront();
		new (m_begin.get()) T(std::forward<Args>(args)...);
		m_begin--;
		return *(m_begin + 1);
	}

	template <typename T, uint64_t ChunkSize>
	template <typename... Args>
	T &Deque<T, ChunkSize>::emplaceBack(Args&&... args)
	{
		expandBack();
		new (m_end.get()) T(std::forward<Args>(args)...);
		m_end++;
		return *(m_end - 1);
	}

	template <typename T, uint64_t ChunkSize>
	T &Deque<T, ChunkSize>::front()
	{
		return *(m_begin + 1);
	}

	template <typename T, uint64_t ChunkSize>
	const T &Deque<T, ChunkSize>::front() const
	{
		return *(m_begin + 1);
	}

	template <typename T, uint64_t ChunkSize>
	T &Deque<T, ChunkSize>::back()
	{
		return *(m_end - 1);
	}

	template <typename T, uint64_t ChunkSize>
	const T &Deque<T, ChunkSize>::back() const
	{
		return *(m_end - 1);
	}

	template <typename T, uint64_t ChunkSize>
	typename Deque<T, ChunkSize>::Iterator Deque<T, ChunkSize>::begin()
	{
		return m_begin;
	}

	template <typename T, uint64_t ChunkSize>
	typename Deque<T, ChunkSize>::Iterator Deque<T, ChunkSize>::end()
	{
		return m_end;
	}

	template <typename T, uint64_t ChunkSize>
	T &Deque<T, ChunkSize>::at(uint64_t idx)
	{
		return m_begin[idx];
	}

	template <typename T, uint64_t ChunkSize>
	const T &Deque<T, ChunkSize>::at(uint64_t idx) const
	{
		return m_begin[idx];
	}

	template <typename T, uint64_t ChunkSize>
	T &Deque<T, ChunkSize>::operator [] (uint64_t idx)
	{
		return m_begin[idx];
	}

	template <typename T, uint64_t ChunkSize>
	const T &Deque<T, ChunkSize>::operator [] (uint64_t idx) const
	{
		return m_begin[idx];
	}
}

#endif // DEQUE_H_
