
internal Arena
ArenaInitMemory(void *memory, u64 capacity)
{
	Arena arena = {0};
	arena.base = memory;
	arena.capacity = capacity;
	arena.used = 0;
	arena.last_alloc_offset = 0;

	return arena;
}

internal Arena
ArenaInitArena(Arena *arena, u64 capacity)
{
	Arena child = {0};
	child.base = ArenaPush(arena, capacity, 16);
	child.capacity = capacity;
	child.used = 0;
	child.last_alloc_offset = 0;

	return child;
}

internal void *
ArenaPush(Arena *arena, u64 bytes, u64 alignment)
{
	u64 aligned = MemAlignUp(arena->used, alignment);

	AssertTrue(aligned + bytes <= arena->capacity);

	void *mem = (void *)((u8 *)arena->base + aligned);

	MemSet(mem, 0, bytes);

	arena->used = aligned + bytes;
	arena->last_alloc_offset = aligned;
	
	return mem;
}

internal void
ArenaPopTo(Arena *arena, u64 to)
{
	AssertTrue(to <= arena->capacity);
	arena->used = to;
}

internal void
ArenaPop(Arena *arena, u64 bytes)
{
	if (arena->used <= bytes)
		arena->used = 0;
	else
		arena->used -= bytes;
}

internal void
ArenaResizeLastBy(Arena *arena, u64 bytes)
{
	AssertTrue(arena->used + bytes <= arena->capacity);
	arena->used += bytes;
}

internal void
ArenaResizeLastTo(Arena *arena, u64 bytes)
{
	u64 new_used = arena->last_alloc_offset + bytes;

	AssertTrue(new_used <= arena->capacity);

	arena->used = new_used;
}

internal void
ArenaClear(Arena *arena)
{
	arena->used = 0;
	arena->last_alloc_offset = 0;
}

internal void
ArenaZero(Arena *arena)
{
	MemSet(arena->base, 0, arena->capacity);
}
