#include "render/geometry.h"

rnd::GeometryFreeList::GeometryFreeList()
	: count(0)
	, total_free(0)
{
}

rnd::GeometryFreeList::GeometryFreeList(u64 capacity)
	: count(1)
	, total_free(capacity)
{
	blocks[0].offset = 0;
	blocks[0].capacity = capacity;
}

bool rnd::GeometryFreeList::HasAvailable(u64 size) const
{
	if (size == 0)
		return true;

	if (total_free < size)
		return false;

	for (unsigned i = 0; i < count; i++) {
		if (blocks[i].capacity >= size)
			return true;
	}

	return false;
}

bool rnd::GeometryFreeList::TryAlloc(u64 size, u64 *out_index_offset)
{
	if (size == 0) {
		*out_index_offset = 0;
		return true;
	}

	for (unsigned i = 0; i < count; i++) {
		if (blocks[i].capacity < size)
			continue;

		GeometryFreeBlock *block = &blocks[i];

		*out_index_offset = block->offset;

		block->offset += size;
		block->capacity -= size;

		total_free -= size;

		if (block->capacity == 0) {
			count--;

			// remove block by shifting over it
			for (unsigned j = i; j < count; j++)
				blocks[j] = blocks[j + 1];
		}

		return true;
	}

	return false;
}

void rnd::GeometryFreeList::Release(u64 offset, u64 size)
{
	if (size == 0)
		return;

	unsigned at = 0;

	while (at < count && blocks[at].offset < offset)
		at++;

	for (unsigned j = count; j > at; j--)
		blocks[j] = blocks[j - 1];

	blocks[at].offset = offset;
	blocks[at].capacity = size;

	count++;

	total_free += size;

	if (at + 1 < count) {
		// merge with block to the right if adjacent

		GeometryFreeBlock *curr = &blocks[at];
		GeometryFreeBlock *next = &blocks[at + 1];

		if (curr->offset + curr->capacity == next->offset) {
			curr->capacity = curr->capacity + next->capacity;

			count--;

			for (unsigned j = at + 1; j < count; j++)
				blocks[j] = blocks[j + 1];
		}
	}

	if (at > 0) {
		// merge with block to the left if adjacent

		GeometryFreeBlock *prev = &blocks[at - 1];
		GeometryFreeBlock *curr = &blocks[at];

		if (prev->offset + prev->capacity == curr->offset) {
			prev->capacity = prev->capacity + curr->capacity;

			count--;

			for (unsigned j = at; j < count; j++)
				blocks[j] = blocks[j + 1];
		}
	}
}
