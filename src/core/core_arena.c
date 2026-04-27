
internal Arena
ArenaInitReserved(u64 size)
{
	u64 page = osapi->GetPageSize();
	u64 reserve_size = MemAlignUp(size, page);

	Arena arena = {0};
	arena.base = osapi->VirtualReserve(reserve_size);
	arena.capacity = reserve_size;
	arena.used = 0;
	arena.last_alloc_offset = 0;
	arena.committed = 0;
	arena.kind = ArenaKind_Backed;
	
	return arena;
}

internal Arena
ArenaInitMemory(void *memory, u64 capacity)
{
	Arena arena = {0};
	arena.base = memory;
	arena.capacity = capacity;
	arena.used = 0;
	arena.last_alloc_offset = 0;
	arena.kind = ArenaKind_View;

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
	child.kind = ArenaKind_View;

	return child;
}

internal void
ArenaRelease(Arena *arena)
{
	if (arena->kind != ArenaKind_Backed)
		CoreFatal("Arena (%p) attempted to release but not backed by reserved memory.", arena);

	if (!arena->base)
		CoreFatal("Arena (%p) attempted to release by null base pointer.", arena);
	
	osapi->VirtualRelease(arena->base);
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
	u64 new_used = aligned + bytes;
	
	if (new_used > arena->capacity)
	{
		f64 bytes_mb    = (f64)bytes           / (f64)Megabytes(1);
		f64 capacity_mb = (f64)arena->capacity / (f64)Megabytes(1);
		f64 used_mb     = (f64)arena->used     / (f64)Megabytes(1);
		f64 aligned_mb  = (f64)aligned         / (f64)Megabytes(1);

		f32 free_mb = capacity_mb - used_mb;
	
		CoreFatal("Arena (%p) out of space, requested %.2f MB (%llu bytes), capacity %.2f MB (%llu bytes), used %.2f MB (%llu bytes) (aligned to %.2f MB, free %.2f MB).",
				  arena,
				  bytes_mb,    bytes,
				  capacity_mb, arena->capacity,
				  used_mb,     arena->used,
				  aligned_mb, free_mb);
	}

	if (arena->kind == ArenaKind_Backed && new_used > arena->committed)
	{
		u64 page = osapi->GetPageSize();
		u64 commit_to = MemAlignUp(new_used, page);
		u64 commit_bytes = commit_to - arena->committed;

		osapi->VirtualCommit((u8 *)arena->base + arena->committed, commit_bytes);
	}
	
	void *mem = (u8 *)arena->base + aligned;

	MemSet(mem, 0, bytes);

	arena->used = new_used;
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
		CoreFatal("Arena %p out of space, attempted to resize by %llu bytes from %llu exceeding capacity of %llu bytes.",
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
		CoreFatal("Arena %p out of space, attempted to resize to %llu bytes from %llu exceeding capacity of %llu bytes.",
				  arena, new_used, arena->used, arena->capacity);
	}

	arena->used = new_used;
}

internal void
ArenaClear(Arena *arena)
{
	if (arena->kind == ArenaKind_Backed && arena->committed > 0)
	{
		osapi->VirtualDecommit(arena->base, arena->committed);
		arena->committed = 0;
	}
	
	arena->used = 0;
	arena->last_alloc_offset = 0;
}

internal void
ArenaZero(Arena *arena)
{
	MemSet(arena->base, 0, arena->capacity);
}
