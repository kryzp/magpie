#pragma once

#include "types.h"
#include "memory_arena.h"

class ScratchScope {
public:
	ScratchScope(ArenaView &arena)
		: arena_ref(arena)
		, marker(arena.marker())
	{
	}

	~ScratchScope()
	{
		arena_ref.rewind(marker);
	}

	ArenaView &arena()
	{
		return arena_ref;
	}

private:
	ArenaView &arena_ref;
	u64 marker;
};

namespace scratch
{
	static constexpr u32 SCRATCH_MEMORY_SIZE = MEGABYTES(32);

	void init(VirtualArena &arena);
	void destroy();

	ScratchScope get(ArenaView **conflicts, u32 count);

	inline ScratchScope get()
	{
		return get(nullptr, 0);
	}

	inline ScratchScope get(ArenaView &conflict)
	{
		ArenaView *ptr = &conflict;
		return get(&ptr, 1);
	}
}
