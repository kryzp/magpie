
internal ScratchArena GetScratch(MemoryArena *conflicts, u32 count)
{
	MemoryArena *scratch = core->scratch_arenas;

	for (u32 j = 0; j < count; j++) {
		MemoryArena *arena = conflicts + j;

		for (i32 i = 0; i < 2; i++, scratch += 1) {
			if (scratch != arena) {
				break;
			}
		}
	}

	ScratchArena result = { 0 };
	result.arena = scratch;
	result.pos = scratch->used;

	return result;
}

internal void ReleaseScratch(ScratchArena *scratch)
{
	MemoryArenaPopTo(scratch->arena, scratch->pos);
}
