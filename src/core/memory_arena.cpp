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
	return allocate_no_lock(size, alignment);
}

void *VirtualArena::allocate_no_lock(u64 size, u64 alignment)
{
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
	memory_used = 0;
	memory_reserved = 0;
	memory_committed = 0;
}

ArenaView VirtualArena::view(u64 size, u64 alignment)
{
	return ArenaView(allocate(size, alignment), size);
}

u64 VirtualArena::align_to_page(u64 size)
{
	return memory_align_up(size, platform::get_page_size());
}

ArenaView::ArenaView()
	: memory(0)
	, memory_used(0)
	, memory_size(0)
	, destructor_head(nullptr)
{
}

ArenaView::ArenaView(void *memory, u64 size)
	: memory((uptr)memory)
	, memory_used(0)
	, memory_size(size)
	, destructor_head(nullptr)
{
}

ArenaView::ArenaView(ArenaView &&other) noexcept
	: memory(other.memory)
	, memory_used(other.memory_used)
	, memory_size(other.memory_size)
	, destructor_head(other.destructor_head)
{
	other.memory = 0;
	other.memory_used = 0;
	other.memory_size = 0;
	other.destructor_head = nullptr;
}

ArenaView &ArenaView::operator = (ArenaView &&other) noexcept
{
	if (this != &other) {
		this->memory = other.memory;
		this->memory_used = other.memory_used;
		this->memory_size = other.memory_size;
		this->destructor_head = other.destructor_head;

		other.memory = 0;
		other.memory_used = 0;
		other.memory_size = 0;
		other.destructor_head = nullptr;
	}

	return *this;
}

ArenaView::~ArenaView()
{
}

void ArenaView::init(void *memory, u64 size)
{
	this->memory = (uptr)memory;
	this->memory_used = 0;
	this->memory_size = size;
	this->destructor_head = nullptr;
}

void ArenaView::destroy()
{
	if (!memory)
		return;
	
	invoke_destructors(0);

	memory = 0;
	memory_used = 0;
	memory_size = 0;
	destructor_head = nullptr;
}

ArenaView ArenaView::sub_arena(u64 size, u64 alignment)
{
	return ArenaView(push_bytes(size, alignment), size);
}

void *ArenaView::push_bytes(u64 size, u64 alignment)
{
	void *mem = push_bytes_no_zero(size, alignment);
	assert(mem);
	memory_set(mem, 0, size);
	return mem;
}

void *ArenaView::push_bytes_no_zero(u64 size, u64 alignment)
{
	uptr mem = 0;

	u64 aligned = memory_align_up(memory_used, alignment);

	if (aligned + size <= memory_size) {
		mem = memory + aligned;
		memory_used = aligned;
		memory_used += size;
	}

	assert(mem);

	return (void *)mem;
}

void ArenaView::rewind(u64 marker)
{
	invoke_destructors(marker);
	memory_used = marker;
}

void ArenaView::reset()
{
	invoke_destructors(0);
	memory_used = 0;
}

void ArenaView::clear()
{
	reset();
	memory_set((void *)memory, 0, memory_size);
}

void *ArenaView::buffer() const
{
	return (void *)memory;
}

u64 ArenaView::capacity() const
{
	return memory_size;
}

u64 ArenaView::marker() const
{
	return memory_used;
}

u64 ArenaView::remaining_space() const
{
	return memory_size - memory_used;
}

void ArenaView::invoke_destructors(u64 target_marker)
{
	uptr target_address = memory + target_marker;

	while (destructor_head) {
		if ((uptr)destructor_head < target_address)
			break;
		for (int i = 0; i < destructor_head->count; i++)
			destructor_head->destroy((void *)((uptr)destructor_head->object + i*destructor_head->stride));
		destructor_head = destructor_head->next;
	}
}
