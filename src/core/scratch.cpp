#include "scratch.h"

#include <mutex>

struct ThreadScratch {
	MemoryArena arenas[2];
	std::mutex mutex;
};

static thread_local ThreadScratch thread_scratch_context;

void scratch::init()
{
	auto &ctx = thread_scratch_context;

	std::lock_guard<std::mutex> lock(ctx.mutex);

	ctx.arenas[0] = global_arena.arena(SCRATCH_MEMORY_SIZE);
	ctx.arenas[1] = global_arena.arena(SCRATCH_MEMORY_SIZE);
}

void scratch::destroy()
{
	auto &ctx = thread_scratch_context;

	std::lock_guard<std::mutex> lock(ctx.mutex);

	ctx.arenas[0].destroy();
	ctx.arenas[1].destroy();
}

ScratchScope scratch::get(MemoryArena **conflicts, u32 count)
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
