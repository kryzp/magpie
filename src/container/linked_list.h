#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <new>

#include "common.h"

namespace rvn
{
	/**
	 * List in which each "link" points to the next and previous link.
	 */
	template <typename T>
	class LinkedList
	{
		struct Link
		{
			Link *next;
			Link *prev;
			T data;
		};

	public:
		struct Iterator
		{
			friend class LinkedList<T>;
		public:
			Iterator() : m_ptr(nullptr) { }
			Iterator(Link *ptr) : m_ptr(ptr) { }
			~Iterator() = default;
			T &operator * () const { return m_ptr->data; }
			T *operator -> () const { return &m_ptr->data; }
			Iterator operator + (int n) { for (int i = 0; i < n - 1; i++) { m_ptr = m_ptr->next; } return Iterator(m_ptr); }
			Iterator operator - (int n) { for (int i = 0; i < n - 0; i++) { m_ptr = m_ptr->prev; } return Iterator(m_ptr); }
			Iterator &operator ++ () { m_ptr = m_ptr->next; return *this; }
			Iterator &operator -- () { m_ptr = m_ptr->prev; return *this; }
			const Iterator operator ++ (int) { m_ptr = m_ptr->next; return *this; }
			const Iterator operator -- (int) { m_ptr = m_ptr->prev; return *this; }
			bool operator == (const Iterator &other) const { return this->m_ptr == other.m_ptr; }
			bool operator != (const Iterator &other) const { return this->m_ptr != other.m_ptr; }
		private:
			Link *m_ptr;
		};

		struct ConstIterator
		{
			friend class LinkedList<T>;
		public:
			ConstIterator() : m_ptr(nullptr) { }
			ConstIterator(Link *ptr) : m_ptr(ptr) { }
			~ConstIterator() = default;
			T &operator * () const { return m_ptr->data; }
			T *operator -> () const { return &m_ptr->data; }
			ConstIterator operator + (int n) { for (int i = 0; i < n - 1; i++) { m_ptr = m_ptr->next; } return ConstIterator(m_ptr); }
			ConstIterator operator - (int n) { for (int i = 0; i < n - 0; i++) { m_ptr = m_ptr->prev; } return ConstIterator(m_ptr); }
			ConstIterator &operator ++ () { m_ptr = m_ptr->next; return *this; }
			ConstIterator &operator -- () { m_ptr = m_ptr->prev; return *this; }
			const ConstIterator operator ++ (int) { m_ptr = m_ptr->next; return *this; }
			const ConstIterator operator -- (int) { m_ptr = m_ptr->prev; return *this; }
			bool operator == (const ConstIterator &other) const { return this->m_ptr == other.m_ptr; }
			bool operator != (const ConstIterator &other) const { return this->m_ptr != other.m_ptr; }
		private:
			Link *m_ptr;
		};

		LinkedList();
		~LinkedList();

		/*
		 * Remove items from the list.
		 */
		void clear();
		void erase(const Iterator &it);
		void remove(const T &item);

		/*
		 * Get the size.
		 */
		uint64_t size() const;

		/*
		 * Check if it is empty.
		 */
		bool empty() const;

		/*
		 * Add a new item at a point.
		 */
		Iterator insert(const T &item, const Iterator &it);

		template <typename... Args>
		Iterator emplace(Args&&... args, const Iterator &it);

		template <typename... Args>
		Iterator emplaceFront(Args&&... args);

		template <typename... Args>
		Iterator emplaceBack(Args&&... args);

		Iterator pushFront(const T &item);
		Iterator pushBack(const T &item);

		void popFront();
		void popBack();

		T &front();
		const T &front() const;
		T &back();
		const T &back() const;

		Iterator begin();
		ConstIterator begin() const;
		Iterator end();
		ConstIterator end() const;

		ConstIterator cbegin() const;
		ConstIterator cend() const;

	private:
		Link *m_root;
		uint64_t m_size;
	};

	template <typename T>
	LinkedList<T>::LinkedList()
		: m_root(nullptr)
		, m_size(0)
	{
		m_root = new Link();
		m_root->next = m_root;
		m_root->prev = m_root;
	}

	template <typename T>
	LinkedList<T>::~LinkedList()
	{
		clear();
	}

	template <typename T>
	void LinkedList<T>::clear()
	{
		Link *curr = m_root;

		for (int i = 0; i < m_size; i++)
		{
			Link *next = curr->next;
			delete curr;
			curr = next;
		}

		m_size = 0;
		m_root = new Link();
		m_root->next = m_root;
		m_root->prev = m_root;
	}

	template <typename T>
	void LinkedList<T>::erase(const Iterator &it)
	{
		it.m_ptr->prev->next = it.m_ptr->next;
		it.m_ptr->next->prev = it.m_ptr->prev;
		delete it.m_ptr;
		m_size--;
	}

	template<class T>
	void LinkedList<T>::remove(const T &item)
	{
		Iterator curr = begin();
		Iterator last = end();
		Iterator next = curr;

		while (curr != last)
		{
			next++;

			if (*curr == item) {
				erase(curr);
			}

			curr = next;
		}
	}

	template <typename T>
	LinkedList<T>::Iterator LinkedList<T>::insert(const T &item, const Iterator &it)
	{
		Link *l = new Link();

		l->data = item;

		l->next = it.m_ptr;
		l->prev = it.m_ptr->prev;

		l->prev->next = l;
		l->next->prev = l;

		it.m_ptr->prev = l;

		m_size++;

		return Iterator(l);
	}

	template <typename T>
	template <typename... Args>
	LinkedList<T>::Iterator LinkedList<T>::emplace(Args&&... args, const Iterator &it)
	{
		Link *l = new Link();

		new (&l->data) T(std::forward<Args>(args)...);

		l->next = it.m_ptr;
		l->prev = it.m_ptr->prev;

		l->prev->next = l;
		l->next->prev = l;

		it.m_ptr->prev = l;

		m_size++;

		return Iterator(l);
	}

	template <typename T>
	template <typename... Args>
	LinkedList<T>::Iterator LinkedList<T>::emplaceFront(Args&&... args)
	{
		return emplace(std::forward<Args>(args)..., begin());
	}

	template <typename T>
	template <typename... Args>
	LinkedList<T>::Iterator LinkedList<T>::emplaceBack(Args&&... args)
	{
		return emplace(std::forward<Args>(args)..., end());
	}

	template <typename T>
	LinkedList<T>::Iterator LinkedList<T>::pushFront(const T &item)
	{
		return insert(item, begin());
	}

	template <typename T>
	LinkedList<T>::Iterator LinkedList<T>::pushBack(const T &item)
	{
		return insert(item, end());
	}

	template <typename T>
	void LinkedList<T>::popFront()
	{
		Link *l = m_root->next;
		m_root->next = m_root->next->next;
		m_root->next->prev = m_root;
		delete l;
		m_size--;
	}

	template <typename T>
	void LinkedList<T>::popBack()
	{
		Link *l = m_root;
		m_root = m_root->prev;
		m_root->next = l->next;
		delete l;
		m_size--;
	}

	template <typename T>
	uint64_t LinkedList<T>::size() const
	{
		return m_size;
	}

	template <typename T>
	bool LinkedList<T>::empty() const
	{
		return m_size == 0;
	}

	template <typename T>
	T &LinkedList<T>::front()
	{
		return m_root->next->data;
	}

	template <typename T>
	const T &LinkedList<T>::front() const
	{
		return m_root->next->data;
	}

	template <typename T>
	T &LinkedList<T>::back()
	{
		return m_root->data;
	}

	template <typename T>
	const T &LinkedList<T>::back() const
	{
		return m_root->data;
	}

	template <typename T>
	typename LinkedList<T>::Iterator LinkedList<T>::begin()
	{
		return Iterator(m_root->next);
	}

	template <typename T>
	typename LinkedList<T>::ConstIterator LinkedList<T>::begin() const
	{
		return ConstIterator(m_root->next);
	}

	template <typename T>
	typename LinkedList<T>::Iterator LinkedList<T>::end()
	{
		return Iterator(m_root);
	}

	template <typename T>
	typename LinkedList<T>::ConstIterator LinkedList<T>::end() const
	{
		return Iterator(m_root);
	}

	template <typename T>
	typename LinkedList<T>::ConstIterator LinkedList<T>::cbegin() const
	{
		return ConstIterator(m_root->next);
	}

	template <typename T>
	typename LinkedList<T>::ConstIterator LinkedList<T>::cend() const
	{
		return Iterator(m_root);
	}
}

#endif // LINKED_LIST_H_
