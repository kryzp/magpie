#include "memory_arena.h"

#include "math/calc.h"

MemoryArena::MemoryArena()
	: memory(nullptr)
	, memory_size(0)
	, memory_used(0)
{
}

MemoryArena::MemoryArena(void *memory, u64 size)
	: memory(memory)
	, memory_size(size)
	, memory_used(0)
{
}

MemoryArena::MemoryArena(const MemoryArena &other)
{
	this->memory = other.memory;
	this->memory_size = other.memory_size;
	this->memory_used = other.memory_used;
}

MemoryArena::~MemoryArena()
{
}

MemoryArena MemoryArena::sub_arena(u64 size)
{
	return MemoryArena(this->push_bytes(size), size);
}

void *MemoryArena::push_bytes(u64 size)
{
	void *buf = push_bytes_no_zero(size);

	if (buf)
		memory_set(buf, 0, size);

	return buf;
}

void *MemoryArena::push_bytes_no_zero(u64 size)
{
	void *buf = nullptr;

	if (memory_used + size <= memory_size) {
		buf = (void *)((u8 *)memory + memory_used);
		memory_used += size;
	} else {
		debug_log_crash("Ran out of space in memory arena.");
	}

	return buf;
}

void MemoryArena::pop(u64 size)
{
	memory_used = Calc<u64>::max(0, memory_used - size);
}

void MemoryArena::pop_to(u64 pos)
{
	memory_used = pos;
}

void MemoryArena::clear()
{
	reset();
	zero();
}

void MemoryArena::reset()
{
	memory_used = 0;
}

void MemoryArena::zero() const
{
	memory_set(memory, 0, memory_size);
}
