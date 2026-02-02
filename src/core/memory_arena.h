#pragma once

#include "types.h"

// TODO: Why isn't this in /container/

class MemoryArena {
public:
	MemoryArena();
	MemoryArena(void *memory, u64 size);
	MemoryArena(const MemoryArena &other);
	~MemoryArena();

	MemoryArena sub_arena(u64 size);

	void *push(u64 size);
	void *push_no_zero(u64 size);

	template <typename T>
	T *push_array(u32 count)
	{
		T *buf = (T *)push(sizeof(T) * count);
		for (int i = 0; i < count; i++)
			new (buf + i) T();
		return buf;
	}

	void pop(u64 size);
	void pop_to(u64 pos);

	void clear();
	void reset();
	void zero() const;

	void *get_memory() const
	{
		return memory;
	}

	u64 get_size() const
	{
		return memory_size;
	}

	u64 get_used_memory() const
	{
		return memory_used;
	}

	u64 get_memory_left() const
	{
		return memory_size - memory_used;
	}

private:
	void *memory;
	u64 memory_size;
	u64 memory_used;
};
