#ifndef CORE_SCRATCH_H
#define CORE_SCRATCH_H

typedef struct ScratchArena ScratchArena;
struct ScratchArena
{
	Arena *arena;
	u64 checkpoint;
};

internal ScratchArena ScratchBegin(Arena **conflicts, u32 conflict_count);
internal void ScratchRelease(const ScratchArena *scratch);

#endif // CORE_SCRATCH_H
