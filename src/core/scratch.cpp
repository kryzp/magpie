#include "scratch.h"

MemoryArena *ScratchArena::scratch_arenas = nullptr;
u32 ScratchArena::scratch_count = 0;
Stack<MemoryArena *> ScratchArena::conflict_stack;

ScratchArena::ScratchArena()
{
	MemoryArena *scratch = scratch_arenas;

	if (!conflict_stack.empty()) {
		MemoryArena *conflict = conflict_stack.top();
		for (int i = 0; i < scratch_count; i++, scratch++) {
			if (scratch != conflict)
				break;
		}
	}

	conflict_stack.push(scratch);

	this->arena = scratch;
	this->pos = scratch->get_used_memory();
}

ScratchArena::~ScratchArena()
{
	conflict_stack.pop();
	arena->pop_to(pos);
}

void ScratchArena::select(MemoryArena *scratches, u32 count)
{
	scratch_arenas = scratches;
	scratch_count = count;
}
