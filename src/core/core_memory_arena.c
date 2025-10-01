#include "core_memory_arena.h"

struct memory_arena memory_arena_init(void *memory, u64 size)
{
	struct memory_arena arena = {0};
	arena.memory = memory;
	arena.size = size;
	arena.used = 0;

	return arena;
}

struct memory_arena memory_arena_sub_arena(struct memory_arena *arena, u64 size)
{
	return memory_arena_init(memory_arena_push(arena, size), size);
}

void *memory_arena_push_no_zero(struct memory_arena *arena, u64 size)
{
	void *mem = NULL;

	if (arena->used + size <= arena->size) {
		mem = (void *)((u8 *)arena->memory + arena->used);
		arena->used += size;
	} else {
		debug_log_crash("Ran out of space in memory arena.");
	}
	
	return mem;
}

void *memory_arena_push(struct memory_arena *arena, u64 size)
{
	void *mem = memory_arena_push_no_zero(arena, size);

	if (mem)
		memory_set(mem, 0, size);

	return mem;
}

void memory_arena_pop_to(struct memory_arena *arena, u64 pos)
{
	arena->used = pos;
}

void memory_arena_pop(struct memory_arena *arena, u64 size)
{
	arena->used -= size;

	if (arena->used - size < 0)
		arena->used = 0;
}

void memory_arena_clear(struct memory_arena *arena)
{
	arena->used = 0;
}

void memory_arena_zero(struct memory_arena *arena)
{
	memory_set(arena->memory, 0, arena->size);
}

struct string8 memory_arena_allocate_string8(struct memory_arena *arena, u64 length)
{
	struct string8 string = {0};
	string.len = length;
	string.str = memory_arena_push(arena, length + 1);
	string.str[length] = '\0';
	
	return string;
}

struct string8 memory_arena_allocate_string8_copy(struct memory_arena *arena, struct string8 string)
{
	struct string8 copy = memory_arena_allocate_string8(arena, string.len);
	memory_copy(copy.str, string.str, string.len);

	return copy;
}
