#ifndef CORE_MEMORY_ARENA
#define CORE_MEMORY_ARENA

#include "core_types.h"
#include "core_string.h"

struct memory_arena {
	void *memory;
	u64 size;
	u64 used;
};

struct memory_arena memory_arena_init(void *memory, u64 size);
struct memory_arena memory_arena_sub_arena(struct memory_arena *arena, u64 size);

void *memory_arena_push_no_zero(struct memory_arena *arena, u64 size);
void *memory_arena_push(struct memory_arena *arena, u64 size);

#define memory_arena_array_no_zero(arena, count, elem_size) memory_arena_push_no_zero((arena), (count) * (elem_size))
#define memory_arena_array(arena, count, elem_size) memory_arena_push((arena), (count) * (elem_size))

void memory_arena_pop_to(struct memory_arena *arena, u64 pos);
void memory_arena_pop(struct memory_arena *arena, u64 size);

void memory_arena_clear(struct memory_arena *arena);
void memory_arena_zero(struct memory_arena *arena);

struct string8 memory_arena_allocate_string8(struct memory_arena *arena, u64 length);
struct string8 memory_arena_allocate_string8_copy(struct memory_arena *arena, struct string8 string);

#endif // CORE_MEMORY_ARENA_H
