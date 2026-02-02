#pragma once

#include "memory_arena.h"

#include "container/stack.h"

// TODO: Why isn't this in /container/

/*
 * TODO: THIS ISN'T FIXED TO WORK FOR MULTIPLE THREADS YET!!
 *       DO NOT USE ON ANYTHING OTHER THAN THE MAIN THREAD
 *       EACH THREAD SHOULD HAVE ITS OWN SCRATCH ARENA.
 */

class ScratchArena {
public:
	ScratchArena();
	~ScratchArena();

	MemoryArena &get_arena() const
	{
		return *arena;
	}

	static void select(MemoryArena *scratches, u32 count);

private:
	static MemoryArena *scratch_arenas;
	static u32 scratch_count;
	static Stack<MemoryArena *> conflict_stack;

	MemoryArena *arena;
	u64 pos;
};
