
internal ScratchArena
GetScratch(MemoryArena *arena)
{
	MemoryArena *scratch = core->scratch_arenas;
	
	if(arena)
	{
		for(i32 i = 0; i < 2; i++, scratch += 1)
		{
			if(scratch != arena)
				break;
		}
	}
	
	ScratchArena result = {0};
	result.arena = scratch;
	result.pos = scratch->used;
	
	return result;
}

internal void
ReleaseScratch(ScratchArena *scratch)
{
	MemoryArenaPopTo(scratch->arena, scratch->pos);
}
