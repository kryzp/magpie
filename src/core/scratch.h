#pragma once

#include "types.h"
#include "memory_arena.h"

class ScratchScope {
public:
	ScratchScope(MemoryArena &arena)
		: arena(arena)
		, marker(arena.marker())
	{
	}

	~ScratchScope()
	{
		arena.rewind(marker);
	}

	MemoryArena &get_arena()
	{
		return arena;
	}

private:
	MemoryArena &arena;
	u64 marker;
};

namespace scratch
{
	static constexpr u32 SCRATCH_MEMORY_SIZE = MEGABYTES(32);

	void init();
	void destroy();

	ScratchScope get(MemoryArena **conflicts, u32 count);

	inline ScratchScope get()
	{
		return get(nullptr, 0);
	}

	inline ScratchScope get(MemoryArena &conflict)
	{
		MemoryArena *ptr = &conflict;
		return get(&ptr, 1);
	}
}
