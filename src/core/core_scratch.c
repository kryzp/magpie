#include "core_scratch.h"
#include "core.h"

static struct memory_arena *scratch_arenas = NULL;
static u32 scratch_count = 0;

void scratch_select(struct memory_arena *scratches, u32 count)
{
	scratch_arenas = scratches;
	scratch_count = count;
}

struct scratch_arena scratch_begin(const struct memory_arena *conflicts, u32 count)
{
	struct memory_arena *scratch = scratch_arenas;
	
	for (int j = 0; j < count; j++) {
		const struct memory_arena *arena = conflicts + j;
		if (!arena)
			continue;
		for (int i = 0; i < scratch_count; i++, scratch++) {
			if (scratch != arena)
				break;
		}
	}

	struct scratch_arena result = {0};
	result.arena = scratch;
	result.pos = scratch->used;

	return result;
}

void scratch_release(struct scratch_arena *scratch)
{
	memory_arena_pop_to(scratch->arena, scratch->pos);
}
