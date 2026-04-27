
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
ArenaInitArena(Arena *arena, u64 capacity, u64 alignment)
{
	Arena child = {0};
	child.base = ArenaPush(arena, capacity, alignment);
	child.capacity = capacity;
	child.used = 0;
	child.last_alloc_offset = 0;

	return child;
}

internal u64
ArenaSafePartitionSize(const Arena *parent, u32 count, u64 alignment)
{
	AssertTrue(parent->capacity >= parent->used);
	
	u64 left = parent->capacity - parent->used;
	u64 total_padding_reserve = count * alignment;

	AssertTrue(left >= total_padding_reserve);
	
	u64 safe_size = 0;

	if (left > total_padding_reserve)
		safe_size = (left - total_padding_reserve) / count;

	return safe_size;
}

internal void *
ArenaPush(Arena *arena, u64 bytes, u64 alignment)
{
	u64 aligned = MemAlignUp(arena->used, alignment);

	if (aligned + bytes > arena->capacity)
	{
		CoreFatal("Arena (%p) out of space, requested %llu bytes, capacity %llu, used %llu (aligned offset %llu, free %llu).",
				  arena, bytes, arena->capacity, arena->used, aligned, arena->capacity - aligned);
	}
	
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
   	if (arena->used + bytes > arena->capacity)
	{
		CoreFatal("Arena %p out of space, attempted to resize by %llu bytes from %llu exceeding capacity of %llu.",
				  arena, bytes, arena->used, arena->capacity);
	}
	
	arena->used += bytes;
}

internal void
ArenaResizeLastTo(Arena *arena, u64 bytes)
{
	u64 new_used = arena->last_alloc_offset + bytes;

   	if (new_used > arena->capacity)
	{
		CoreFatal("Arena %p out of space, attempted to resize to %llu bytes from %llu exceeding capacity of %llu.",
				  arena, new_used, arena->used, arena->capacity);
	}

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
