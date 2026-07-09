
static void R_GeometryFreeListInit(R_GeometryFreeList *list, u64 capacity)
{
	list->blocks[0].offset = 0;
	list->blocks[0].capacity = capacity;
	list->count = 1;
	list->total_free = capacity;
}

static b32 R_GeometryFreeListHasAvailable(const R_GeometryFreeList *list, u64 size)
{
	if (size == 0)
		return true;

	if (list->total_free < size)
		return false;

	for (u32 i = 0; i < list->count; i++)
	{
		if (list->blocks[i].capacity >= size)
			return true;
	}

	return false;
}

static b32 R_GeometryFreeListTryAlloc(R_GeometryFreeList *list, u64 size, u64 *out_index_offset)
{
	if (size == 0)
	{
		*out_index_offset = 0;
		return true;
	}

	for (u32 i = 0; i < list->count; i++)
	{
		if (list->blocks[i].capacity < size)
			continue;

		R_GeometryFreeBlock *block = &list->blocks[i];

		*out_index_offset = block->offset;

		block->offset += size;
		block->capacity -= size;
		
		list->total_free -= size;

		if (block->capacity == 0)
		{
			list->count--;

			// remove block by shifting over it
			for (u32 j = i; j < list->count; j++)
				list->blocks[j] = list->blocks[j + 1];
		}

		return true;
	}

	return false;
}

static void R_GeometryFreeListRelease(R_GeometryFreeList *list, u64 offset, u64 size)
{
	if (size == 0)
		return;

	u32 at = 0;

	while (at < list->count && list->blocks[at].offset < offset)
		at++;

	for (u32 j = list->count; j > at; j--)
		list->blocks[j] = list->blocks[j - 1];

	list->blocks[at].offset = offset;
	list->blocks[at].capacity = size;

	list->count++;

	list->total_free += size;

	if (at + 1 < list->count)
	{
		// merge with block to the right if adjacent

		R_GeometryFreeBlock *curr = &list->blocks[at];
		R_GeometryFreeBlock *next = &list->blocks[at + 1];

		if (curr->offset + curr->capacity == next->offset)
		{
			curr->capacity = curr->capacity + next->capacity;

			list->count--;

			for (u32 j = at + 1; j < list->count; j++)
				list->blocks[j] = list->blocks[j + 1];
		}
	}

	if (at > 0)
	{
		// merge with block to the left if adjacent

		R_GeometryFreeBlock *prev = &list->blocks[at - 1];
		R_GeometryFreeBlock *curr = &list->blocks[at];

		if (prev->offset + prev->capacity == curr->offset)
		{
			prev->capacity = prev->capacity + curr->capacity;

			list->count--;

			for (u32 j = at; j < list->count; j++)
				list->blocks[j] = list->blocks[j + 1];
		}
	}
}
