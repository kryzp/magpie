
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

internal char *
MemoryArenaAllocateCStringCopy(MemoryArena *arena, char *str)
{
    u32 str_length = CalculateCStringLength(str);
    char *str_copy = (char *)MemoryArenaPush(arena, str_length + 1);
    MemoryCopy(str_copy, str, str_length);
    str_copy[str_length] = '\0';
    return str_copy;
}
