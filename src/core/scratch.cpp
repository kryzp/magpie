#include "scratch.h"

struct ThreadScratch {
	ArenaView arenas[2];
};

static thread_local ThreadScratch thread_scratch_context;

void scratch::init(VirtualArena &arena)
{
	auto &ctx = thread_scratch_context;

	ctx.arenas[0] = arena.arena(SCRATCH_MEMORY_SIZE);
	ctx.arenas[1] = arena.arena(SCRATCH_MEMORY_SIZE);
}

void scratch::destroy()
{
	auto &ctx = thread_scratch_context;

	ctx.arenas[0].destroy();
	ctx.arenas[1].destroy();
}

ScratchScope scratch::get(ArenaView **conflicts, u32 count)
{
	auto &ctx = thread_scratch_context;

	for (int i = 0; i < 2; i++) {
		bool conflict = false;

		for (int j = 0; j < count; j++) {
			if (&ctx.arenas[i] == conflicts[j]) {
				conflict = true;
				break;
			}
		}

		if (!conflict)
			return ScratchScope(ctx.arenas[i]);
	}

	// This should *provably* never happen. Ever.
	debug_log_crash("All scratch arenas are in conflict.");
	return ScratchScope(ctx.arenas[0]);
}
