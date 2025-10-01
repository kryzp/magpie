#include "core.h"
#include "core_scratch.h"

void core_init(struct core *core, struct memory_arena *arena, u64 scratch_size)
{
	core->scratch_arenas[0] = memory_arena_sub_arena(arena, scratch_size);
	core->scratch_arenas[1] = memory_arena_sub_arena(arena, scratch_size);
	
	scratch_select(core->scratch_arenas, 2);
}

void core_hot_load(struct core *core)
{
	scratch_select(core->scratch_arenas, 2);
}
