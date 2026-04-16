
internal ScratchArena
ScratchBegin(Arena * const *conflicts, u32 conflict_count)
{
	Arena *arena = osapi->JobGetScratch(conflicts, conflict_count);
	
	ScratchArena scratch = {0};
	scratch.arena = arena; 
	scratch.checkpoint = arena->used;

	return scratch;
}

internal void
ScratchClear(const ScratchArena *scratch)
{
	ArenaPopTo(scratch->arena, scratch->checkpoint);
}

internal void
ScratchRelease(const ScratchArena *scratch)
{
	ArenaPopTo(scratch->arena, scratch->checkpoint);
}
