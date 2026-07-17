
static ScratchArena ScratchBegin(Arena * const *conflicts, u32 conflict_count)
{
	Arena *arena = osapi->JobGetScratch(conflicts, conflict_count);
	
	ScratchArena scratch = {0};
	scratch.arena = arena; 
	scratch.checkpoint = arena->used;

	return scratch;
}

static void ScratchClear(const ScratchArena *scratch)
{
	ArenaPopTo(scratch->arena, scratch->checkpoint);
}

static void ScratchRelease(ScratchArena *scratch)
{
	ArenaPopTo(scratch->arena, scratch->checkpoint);

	scratch->arena = NULL;
	scratch->checkpoint = 0;
}
