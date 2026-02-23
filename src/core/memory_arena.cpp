#include "memory_arena.h"

#include "platform/platform.h"

VirtualArena::VirtualArena()
	: memory(0)
	, memory_used(0)
	, memory_reserved(0)
	, memory_committed(0)
	, allocation_mutex()
{
}

VirtualArena::~VirtualArena()
{
}

void VirtualArena::reserve(u64 size)
{
	memory = (uptr)platform::virtual_reserve(size);
	memory_used = 0;
	memory_reserved = size;
	memory_committed = 0;
}

void VirtualArena::commit(u64 size)
{
	size = align_to_page(size);
	platform::virtual_commit((void *)(memory + memory_committed), size);
	memory_committed += size;
}

void *VirtualArena::allocate(u64 size, u64 alignment)
{
	std::lock_guard<std::mutex> lock(allocation_mutex);

	memory_used = memory_align_up(memory_used, alignment);

	uptr mem = memory + memory_used;

	memory_used += size;

	if (memory_used > memory_committed)
		commit(memory_used - memory_committed);

	return (void *)mem;
}

void VirtualArena::destroy()
{
	if (!memory)
		return;

	platform::virtual_free((void *)memory);
	memory = 0;
}

MemoryArena VirtualArena::arena(u64 size, u64 alignment)
{
	return MemoryArena(allocate(size, alignment), size);
}

u64 VirtualArena::align_to_page(u64 size)
{
	return memory_align_up(size, platform::get_page_size());
}

MemoryArena::MemoryArena()
	: memory(0)
	, memory_used(0)
	, memory_size(0)
	, destructor_head(nullptr)
	, allocation_mutex()
{
}

MemoryArena::MemoryArena(void *memory, u64 size)
	: memory((uptr)memory)
	, memory_used(0)
	, memory_size(size)
	, destructor_head(nullptr)
	, allocation_mutex()
{
}

MemoryArena::MemoryArena(const MemoryArena &other)
{
	this->memory = other.memory;
	this->memory_used = other.memory_used;
	this->memory_size = other.memory_size;
	this->destructor_head = other.destructor_head;
}

MemoryArena &MemoryArena::operator = (const MemoryArena &other)
{
	this->memory = other.memory;
	this->memory_used = other.memory_used;
	this->memory_size = other.memory_size;
	this->destructor_head = other.destructor_head;

	return *this;
}

MemoryArena::~MemoryArena()
{
}

void MemoryArena::init(void *memory, u64 size)
{
	this->memory = (uptr)memory;
	this->memory_used = 0;
	this->memory_size = size;
	this->destructor_head = nullptr;
}

void MemoryArena::destroy()
{
	if (!memory)
		return;
	
	invoke_destructors(0);

	memory = 0;
	memory_used = 0;
	memory_size = 0;
	destructor_head = nullptr;
}

MemoryArena MemoryArena::sub_arena(u64 size, u64 alignment)
{
	return MemoryArena(push_bytes(size, alignment), size);
}

void *MemoryArena::push_bytes(u64 size, u64 alignment)
{
	void *mem = push_bytes_no_zero(size, alignment);
	assert(mem);
	memory_set(mem, 0, size);
	return mem;
}

void *MemoryArena::push_bytes_no_zero(u64 size, u64 alignment)
{
	std::lock_guard<std::mutex> lock(allocation_mutex);

	uptr mem = 0;

	memory_used = memory_align_up(memory_used, alignment);

	if (memory_used + size <= memory_size) {
		mem = memory + memory_used;
		memory_used += size;
	}

	assert(mem);
	return (void *)mem;
}

void MemoryArena::rewind(u64 marker)
{
	invoke_destructors(marker);
	memory_used = marker;
}

void MemoryArena::reset()
{
	invoke_destructors(0);
	memory_used = 0;
}

void MemoryArena::clear()
{
	reset();
	memory_set((void *)memory, 0, memory_size);
}

void *MemoryArena::buffer() const
{
	return (void *)memory;
}

u64 MemoryArena::capacity() const
{
	return memory_size;
}

u64 MemoryArena::marker() const
{
	return memory_used;
}

u64 MemoryArena::remaining_space() const
{
	return memory_size - memory_used;
}

void MemoryArena::invoke_destructors(u64 target_marker)
{
	uptr target_address = memory + target_marker;

	while (destructor_head) {
		if ((uptr)destructor_head < target_address)
			break;
		destructor_head->destroy(destructor_head->object);
		destructor_head = destructor_head->next;
	}
}
