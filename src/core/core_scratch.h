#ifndef CORE_SCRATCH_H
#define CORE_SCRATCH_H

typedef struct ScratchArena ScratchArena;
struct ScratchArena
{
	Arena *arena;
	u64 checkpoint;
};

static ScratchArena ScratchBegin(Arena * const *conflicts, u32 conflict_count);
static void ScratchClear(const ScratchArena *scratch);
static void ScratchRelease(ScratchArena *scratch);

#endif // CORE_SCRATCH_H
