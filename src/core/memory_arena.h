#pragma once

#include <type_traits>
#include <utility>

#include "types.h"

class MemoryArena;

class VirtualArena {
public:
	VirtualArena();
	~VirtualArena();

	void reserve(u64 size);
	void commit(u64 size);
	void *allocate(u64 size, u64 alignment = 16);
	
	void destroy();

	MemoryArena arena(u64 size, u64 alignment = 16);

private:
	u64 align_to_page(u64 size);

	uptr memory;
	u64 memory_used;
	u64 memory_reserved;
	u64 memory_committed;
};

extern VirtualArena global_arena;

class MemoryArena {
public:
	MemoryArena();
	MemoryArena(void *memory, u64 size);

	MemoryArena(const MemoryArena &other);
	MemoryArena &operator = (const MemoryArena &other);
	
	~MemoryArena();

	void init(void *memory, u64 size);
	void destroy();

	void *push_bytes(u64 size, u64 alignment = 16);
	void *push_bytes_no_zero(u64 size, u64 alignment = 16);

	template <typename T, typename ...Args>
	T *push_array(u32 count, Args &&...args)
	{
		T *buf = (T *)push_bytes(sizeof(T) * count, alignof(T));
		
		for (int i = 0; i < count; i++)
			new (buf + i) T(std::forward<Args>(args)...);

		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (int i = 0; i < count; i++)
				register_destructor<T>(buf + i);
		}

		return buf;
	}

	template <typename T, typename ...Args>
	T *push(Args &&...args)
	{
		T *buf = (T *)push_bytes(sizeof(T), alignof(T));
		new (buf) T(std::forward<Args>(args)...);

		if constexpr (!std::is_trivially_destructible_v<T>)
			register_destructor<T>(buf);

		return buf;
	}

	void rewind(u64 marker);

	void reset();
	void clear();

	void *buffer() const;
	u64 capacity() const;
	u64 marker() const;
	u64 remaining_space() const;

private:
	struct DestructorNode {
		void (*destroy)(void *);
		void *object;
		DestructorNode *next;
	};

	template <typename T>
	void register_destructor(T *object)
	{
		// Allocate the node onto ourselves 'cuz we're cool like that.
		DestructorNode *node = (DestructorNode *)push_bytes_no_zero(sizeof(DestructorNode), alignof(DestructorNode));
		node->destroy = [](void *ptr) { static_cast<T *>(ptr)->~T(); };
		node->object = object;
		node->next = destructor_head;

		destructor_head = node;
	}

	void invoke_destructors(u64 target_marker);

	uptr memory;
	u64 memory_used;
	u64 memory_size;

	DestructorNode *destructor_head;
};
