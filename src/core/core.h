#ifndef CORE_H
#define CORE_H

#include "core_memory_arena.h"

struct core {
	struct memory_arena scratch_arenas[2];
};

void core_init(struct core *core, struct memory_arena *arena, u64 scratch_size);
void core_hot_load(struct core *core);

#endif // CORE_H
