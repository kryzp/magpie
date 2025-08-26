
internal MemoryArena
MemoryArenaInit(void *memory, u64 size)
{
    MemoryArena arena = {0};
    arena.memory = memory;
    arena.size = size;
    arena.used = 0;
    
    return arena;
}

internal void *
MemoryArenaPushNoZero(MemoryArena *arena, u64 size)
{
    void *mem = 0;
    
    if(arena->used + size <= arena->size)
    {
        mem = (void *)((u8 *)arena->memory + arena->used);
        arena->used += size;
    }
    
    return mem;
}

internal void *
MemoryArenaPush(MemoryArena *arena, u64 size)
{
    void *mem = MemoryArenaPushNoZero(arena, size);
    
    if(mem)
	{
        MemorySet(mem, 0, size);
    }
	
    return mem;
}

// NOTE(kp): Create an arena within an arena.
internal MemoryArena
MemoryArenaSubArena(MemoryArena *arena, u64 size)
{
	return MemoryArenaInit(MemoryArenaPush(arena, size), size);
}

internal void
MemoryArenaPopTo(MemoryArena *arena, u64 pos)
{
    arena->used = pos;
}

internal void
MemoryArenaPop(MemoryArena *arena, u64 size)
{
	arena->used -= size;
	
	if (arena->used - size < 0)
	{
		arena->used = 0;
	}
}

internal void
MemoryArenaClear(MemoryArena *arena)
{
    arena->used = 0;
}

internal void
MemoryArenaZero(MemoryArena *arena)
{
    MemorySet(arena->memory, 0, arena->size);
}

internal String8
MemoryArenaAllocateString8(MemoryArena *arena, u64 length)
{
	String8 string = {0};
	string.len = length;
	string.str = MemoryArenaPush(arena, length + 1);
	
	string.str[length] = '\0';
	
	return string;
}

internal String8
MemoryArenaAllocateString8Copy(MemoryArena *arena, String8 string)
{
	String8 copy = MemoryArenaAllocateString8(arena, string.len);
	MemoryCopy(copy.str, string.str, string.len);
	
	return copy;
}
