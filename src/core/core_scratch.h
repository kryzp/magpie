#ifndef CORE_SCRATCH_H
#define CORE_SCRATCH_H

#include "core_memory_arena.h"

struct scratch_arena {
	struct memory_arena *arena;
	u64 pos;
};

void scratch_select(struct memory_arena *scratches, u32 count);

struct scratch_arena scratch_begin(const struct memory_arena *conflicts, u32 count);
void scratch_release(struct scratch_arena *scratch);

#endif // CORE_SCRATCH_H
