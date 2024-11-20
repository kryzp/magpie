#ifndef HASH_MAP_H_
#define HASH_MAP_H_

#include "../common.h"
#include "pair.h"

namespace llt
{
	/**
	 * Dictionary structure that uses a hash function
	 * to index the different elements inside it.
	 */
	template <typename TKey, typename TValue>
	class HashMap
	{
	public:
		constexpr static unsigned MIN_CAPACITY = 16;

		using KeyValuePair = Pair<TKey, TValue>;

		struct Element
		{
			Element() : data(), next(nullptr), prev(nullptr) { }
			Element(const KeyValuePair& p) : data(p), next(nullptr), prev(nullptr) { }
			Element(Element&& other) noexcept : data(std::move(other.data)), next(std::move(other.next)), prev(std::move(other.prev)) { };
			KeyValuePair data;
			Element* next;
			Element* prev;
		};

		struct Iterator
		{
			Iterator() : m_elem(nullptr) { }
			Iterator(Element* init) : m_elem(init) { }
			~Iterator() = default;
			KeyValuePair& operator * () const { return m_elem->data; }
			KeyValuePair* operator -> () const { return &m_elem->data; }
			Iterator& operator ++ () { if (m_elem) { m_elem = m_elem->next; } return *this; }
			Iterator& operator -- () { if (m_elem) { m_elem = m_elem->prev; } return *this; }
			Iterator& operator ++ (int) { if (m_elem) { m_elem = m_elem->next; } return *this; }
			Iterator& operator -- (int) { if (m_elem) { m_elem = m_elem->prev; } return *this; }
			bool operator == (const Iterator& other) const { return this->m_elem == other.m_elem; }
			bool operator != (const Iterator& other) const { return this->m_elem != other.m_elem; }
		private:
			Element* m_elem;
		};

		struct ConstIterator
		{
			ConstIterator() : m_elem(nullptr) { }
			ConstIterator(const Element* init) : m_elem(init) { }
			~ConstIterator() = default;
			const KeyValuePair& operator * () const { return m_elem->data; }
			const KeyValuePair* operator -> () const { return &m_elem->data; }
			ConstIterator& operator ++ () { if (m_elem) { m_elem = m_elem->next; } return *this; }
			ConstIterator& operator -- () { if (m_elem) { m_elem = m_elem->prev; } return *this; }
			ConstIterator& operator ++ (int) { if (m_elem) { m_elem = m_elem->next; } return *this; }
			ConstIterator& operator -- (int) { if (m_elem) { m_elem = m_elem->prev; } return *this; }
			bool operator == (const ConstIterator& other) const { return this->m_elem == other.m_elem; }
			bool operator != (const ConstIterator& other) const { return this->m_elem != other.m_elem; }
		private:
			const Element* m_elem;
		};

		HashMap();
		HashMap(int initialCapacity);

		HashMap(const HashMap& other);
		HashMap(HashMap&& other) noexcept;

		HashMap& operator = (const HashMap& other);
		HashMap& operator = (HashMap&& other) noexcept;

		~HashMap();

		/*
		 * Add a pair to the hashmap.
		 */
		void insert(const TKey& key, const TValue& value);
		void insert(const KeyValuePair& pair);

		/*
		 * Erase the entry with this key from the hashmap.
		 */
		void erase(const TKey& key);

		/*
		 * Erase all entries.
		 */
		void clear();

		/*
		 * Retrieve the entry with this key.
		 */
		TValue& get(const TKey& key);
		const TValue& get(const TKey& key) const;

		/*
		 * Check if the hashmap contains this key.
		 */
		bool contains(const TKey& key);

		int getElementCount() const;
		int getCapacity() const;
		bool isEmpty() const;

		Element* first();
		const Element* first() const;
		Element* last();
		const Element* last() const;

		Iterator begin();
		ConstIterator begin() const;
		Iterator end();
		ConstIterator end() const;

		ConstIterator cbegin() const;
		ConstIterator cend() const;

		TValue& operator [] (const TKey& idx);
		const TValue& operator [] (const TKey& idx) const;

	private:
		/*
		 * Calculates the bucket index of the key.
		 */
		int indexOf(const TKey& key) const;

		/*
		 * Allocates the memory for the element buckets.
		 */
		void realloc();

		/*
		 * Re-Aligns the pointers for the first and last entries to ensure
		 * that the iterators still work.
		 */
		void realignPtrs();

		/*
		 * (Internal without pointer re-alignment)
		 * Insert a key-value pair.
		 */
		void _insert(const KeyValuePair& pair);

		Element** m_elements;
		int m_elementCount;
		int m_capacity;
	};

	template <typename TKey, typename TValue>
	HashMap<TKey, TValue>::HashMap()
		: m_elements(nullptr)
		, m_elementCount(0)
		, m_capacity(0)
	{
		realloc();
	}

	template <typename TKey, typename TValue>
	HashMap<TKey, TValue>::HashMap(int initial_capacity)
		: m_elements(nullptr)
		, m_elementCount(0)
		, m_capacity(initial_capacity)
	{
		realloc();
	}

	template <typename TKey, typename TValue>
	HashMap<TKey, TValue>::HashMap(const HashMap& other)
	{
		this->m_elements = nullptr;
		this->m_elementCount = other.m_elementCount;
		this->m_capacity = other.m_capacity;

		realloc();

		if (!other.m_elements) {
			return;
		}

		/*
		 * Add all elements in the other hashmap.
		 */
		for (int i = 0; i < other.m_capacity; i++)
		{
			if (other.m_elements[i])
			{
				Element* elemPtr = other.m_elements[i];

				while (elemPtr) {
					_insert(elemPtr->data);
					elemPtr = elemPtr->next;
				}

				break;
			}
		}

		realignPtrs();
	}

	template <typename TKey, typename TValue>
	HashMap<TKey, TValue>::HashMap(HashMap&& other) noexcept
	{
		this->m_elements = std::move(other.m_elements);
		this->m_elementCount = std::move(other.m_elementCount);
		this->m_capacity = std::move(other.m_capacity);

		other.m_elements = nullptr;
		other.m_elementCount = 0;
		other.m_capacity = 0;
	}

	template <typename TKey, typename TValue>
	HashMap<TKey, TValue>& HashMap<TKey, TValue>::operator = (const HashMap& other)
	{
		this->m_elements = nullptr;
		this->m_elementCount = other.m_elementCount;
		this->m_capacity = other.m_capacity;

		realloc();

		if (!other.m_elements) {
			return *this;
		}

		for (int i = 0; i < other.m_capacity; i++)
		{
			if (other.m_elements[i])
			{
				Element* elemPtr = other.m_elements[i];

				while (elemPtr) {
					_insert(elemPtr->data);
					elemPtr = elemPtr->next;
				}

				break;
			}
		}

		realignPtrs();

		return *this;
	}

	template <typename TKey, typename TValue>
	HashMap<TKey, TValue>& HashMap<TKey, TValue>::operator = (HashMap&& other) noexcept
	{
		this->m_elements = std::move(other.m_elements);
		this->m_elementCount = std::move(other.m_elementCount);
		this->m_capacity = std::move(other.m_capacity);

		other.m_elements = nullptr;
		other.m_elementCount = 0;
		other.m_capacity = 0;

		return *this;
	}

	template <typename TKey, typename TValue>
	HashMap<TKey, TValue>::~HashMap()
	{
		Element* e = m_elements[0];

		while (e)
		{
			Element* next = e->next;
			delete e;
			e = next;
		}

		delete[] m_elements;

		m_capacity = 0;
		m_elementCount = 0;
	}

	template <typename TKey, typename TValue>
	void HashMap<TKey, TValue>::insert(const TKey& key, const TValue& value)
	{
		this->insert(KeyValuePair(key, value));
	}

	template <typename TKey, typename TValue>
	void HashMap<TKey, TValue>::insert(const KeyValuePair& pair)
	{
		_insert(pair);

		realignPtrs();

		m_elementCount++;
	}

	template <typename TKey, typename TValue>
	void HashMap<TKey, TValue>::_insert(const KeyValuePair& pair)
	{
		auto idx = indexOf(pair.first);

		Element* b = m_elements[idx];

		if (b)
		{
			while (b->next) {
				b = b->next;
			}

			b->next = new Element(pair);
			b->next->prev = b;
		}
		else
		{
			m_elements[idx] = new Element(pair);
		}
	}

	template <typename TKey, typename TValue>
	void HashMap<TKey, TValue>::erase(const TKey& key)
	{
		Element* b = m_elements[indexOf(key)];

		while (b)
		{
			if (b->data.first == key)
			{
				if (b == m_elements[indexOf(key)]) {
					m_elements[indexOf(key)] = b->next;
				}

				if (b->next) {
					b->next->prev = b->prev;
				}

				if (b->prev) {
					b->prev->next = b->next;
				}

				::operator delete(b, sizeof(Element));

				m_elementCount--;

				realignPtrs();

				return;
			}

			b = b->next;
		}
	}

	template <typename TKey, typename TValue>
	void HashMap<TKey, TValue>::clear()
	{
		m_elementCount = 0;
	}

	template <typename TKey, typename TValue>
	void HashMap<TKey, TValue>::realloc()
	{
		int oldCapacity = m_capacity;

		if (m_capacity < MIN_CAPACITY) {
			m_capacity = MIN_CAPACITY;
		}

		// keep doubling our capacity until we reach the desired bucket count.
		while (m_elementCount >= m_capacity) {
			m_capacity *= 2;
		}

		Element** newBuffer = new Element*[m_capacity];
		mem::set(newBuffer, 0, sizeof(Element*) * m_capacity);

		if (m_elements)
		{
			for (int i = 0; i < oldCapacity; i++)
			{
				if (m_elements[i])
				{
					int idx = indexOf(m_elements[i]->data.first);
					newBuffer[idx] = new Element(std::move(*m_elements[i]));
				}
			}
		}

		delete[] m_elements;
		m_elements = newBuffer;

		realignPtrs();
	}

	template <typename TKey, typename TValue>
	void HashMap<TKey, TValue>::realignPtrs()
	{
		Element* f = first();
		Element* l = last();

		if (f == nullptr && l == nullptr) {
			return;
		}

		for (int i = 0; i < m_capacity; i++)
		{
			Element* bucket = m_elements[i];

			// make sure we are on a valid bucket
			if (!bucket) {
				continue;
			}

			// do this only if we are not the last element to prevent an infinite loop when iterating
			if (bucket != l)
			{
				// backwards pass
				for (int j = 0; j < m_capacity - 1; j++)
				{
					int checkIdx = (i + j + 1) % m_capacity;

					if (m_elements[checkIdx])
					{
						Element* lastElem = m_elements[i];

						while (lastElem->next) {
							for (int k = 0; k < m_capacity; k++) {
								if (lastElem->next == m_elements[k]) {
									goto nextAlignFound;
								}
							}
							lastElem = lastElem->next;
						}

nextAlignFound:
						lastElem->next = m_elements[checkIdx];
						break;
					}
				}
			}

			// do this only if we are not the first element to prevent an infinite loop when iterating
			if (bucket != f)
			{
				// forwards pass
				for (int j = 0; j < m_capacity - 1; j++)
				{
					int check_idx = m_capacity - 1 - j + i;

					if (check_idx < 0) {
						check_idx += m_capacity;
					} else if (check_idx >= m_capacity) {
						check_idx -= m_capacity;
					}

					if (m_elements[check_idx])
					{
						bucket->prev = m_elements[check_idx];
						break;
					}
				}
			}
		}
	}

	template <typename TKey, typename TValue>
	TValue& HashMap<TKey, TValue>::get(const TKey& key)
	{
		Element* b = m_elements[indexOf(key)];

		while (b)
		{
			if (b->data.first == key) {
				return b->data.second;
			}

			b = b->next;
		}

		LLT_ERROR("[HASHMAP|DEBUG] Could not find bucket matching key.");
		return m_elements[0]->data.second;
	}

	template <typename TKey, typename TValue>
	const TValue& HashMap<TKey, TValue>::get(const TKey& key) const
	{
		Element* b = m_elements[indexOf(key)];

		while (b)
		{
			if (b->data.first == key) {
				return b->data.second;
			}

			b = b->next;
		}

		LLT_ERROR("[HASHMAP|DEBUG] Could not find element matching key.");
		return m_elements[0]->data.second;
	}

	template <typename TKey, typename TValue>
	bool HashMap<TKey, TValue>::contains(const TKey& key)
	{
		Element* b = m_elements[indexOf(key)];

		while (b)
		{
			if (b->data.first == key) {
				return true;
			}

			b = b->next;
		}

		return false;
	}

	template <typename TKey, typename TValue>
	int HashMap<TKey, TValue>::getElementCount() const
	{
		return m_elementCount;
	}

	template <typename TKey, typename TValue>
	int HashMap<TKey, TValue>::getCapacity() const
	{
		return m_capacity;
	}

	template <typename TKey, typename TValue>
	bool HashMap<TKey, TValue>::isEmpty() const
	{
		for (int i = 0; i < m_capacity; i++) {
			if (m_elements[i]) {
				return false;
			}
		}

		return true;
	}

	template <typename TKey, typename TValue>
	int HashMap<TKey, TValue>::indexOf(const TKey& key) const
	{
		return hash::calc(&key) % m_capacity;
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::Element* HashMap<TKey, TValue>::first()
	{
		for (int i = 0; i < m_capacity; i++) {
			if (m_elements[i]) {
				return m_elements[i];
			}
		}

		return nullptr;
	}

	template <typename TKey, typename TValue>
	const typename HashMap<TKey, TValue>::Element* HashMap<TKey, TValue>::first() const
	{
		for (int i = 0; i < m_capacity; i++) {
			if (m_elements[i]) {
				return m_elements[i];
			}
		}

		return nullptr;
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::Element* HashMap<TKey, TValue>::last()
	{
		for (int i = m_capacity - 1; i >= 0; i--) {
			if (m_elements[i]) {
				return m_elements[i];
			}
		}

		return nullptr;
	}

	template <typename TKey, typename TValue>
	const typename HashMap<TKey, TValue>::Element* HashMap<TKey, TValue>::last() const
	{
		for (int i = m_capacity; i >= 0; i--) {
			if (m_elements[i]) {
				return m_elements[i];
			}
		}

		return nullptr;
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::Iterator HashMap<TKey, TValue>::begin()
	{
		return Iterator(first());
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::ConstIterator HashMap<TKey, TValue>::begin() const
	{
		return ConstIterator(first());
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::ConstIterator HashMap<TKey, TValue>::cbegin() const
	{
		return ConstIterator(first());
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::Iterator HashMap<TKey, TValue>::end()
	{
		return Iterator(nullptr);
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::ConstIterator HashMap<TKey, TValue>::end() const
	{
		return ConstIterator(nullptr);
	}

	template <typename TKey, typename TValue>
	typename HashMap<TKey, TValue>::ConstIterator HashMap<TKey, TValue>::cend() const
	{
		return ConstIterator(nullptr);
	}

	template <typename TKey, typename TValue>
	TValue& HashMap<TKey, TValue>::operator [] (const TKey& idx)
	{
		return get(idx);
	}

	template <typename TKey, typename TValue>
	const TValue& HashMap<TKey, TValue>::operator [] (const TKey& idx) const
	{
		return get(idx);
	}
};

#endif // HASH_MAP_H_
