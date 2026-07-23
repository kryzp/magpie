
internal void SlotPoolInit(SlotPool *pool, Arena *arena, u32 capacity)
{
	pool->free_indices = ArenaPushArray(arena, u32, capacity);
	pool->capacity = capacity;
	pool->free_index_count = capacity;

	for (u32 i = 0; i < capacity; i++)
		pool->free_indices[i] = capacity - 1 - i;
}

internal b32 SlotPoolAlloc(SlotPool *pool, u32 *out_index)
{
	if (pool->free_index_count == 0)
		return false;

	*out_index = pool->free_indices[--pool->free_index_count];
	return true;
}

internal void SlotPoolFree(SlotPool *pool, u32 index)
{
	AssertTrue(pool->free_index_count < pool->capacity);
	pool->free_indices[pool->free_index_count++] = index;
}

internal u32 SlotPoolLiveCount(const SlotPool *pool)
{
	return pool->capacity - pool->free_index_count;
}

internal void DensePoolInit(DensePool *pool, Arena *arena, u32 capacity)
{
	pool->id_to_dense = ArenaPushArray(arena, u32, capacity);
	pool->dense_to_id = ArenaPushArray(arena, u32, capacity);
	pool->free_ids = ArenaPushArray(arena, u32, capacity);
	pool->capacity = capacity;
	pool->count = 0;
	pool->free_id_count = capacity;

	for (u32 i = 0; i < capacity; i++)
		pool->free_ids[i] = capacity - 1 - i;
}

internal u32 DensePoolGetStableID(DensePool *pool)
{
	AssertTrue(pool->free_id_count > 0);

	u32 id = pool->free_ids[--pool->free_id_count];
	u32 dense = pool->count++;

	pool->id_to_dense[id] = dense;
	pool->dense_to_id[dense] = id;

	return id;
}

internal u32 DensePoolFreeID(DensePool *pool, u32 id)
{
	u32 dense = pool->id_to_dense[id];
	u32 last_dense = --pool->count;
	u32 last_id = pool->dense_to_id[last_dense];

	pool->dense_to_id[dense] = last_id;
	pool->id_to_dense[last_id] = dense;

	pool->free_ids[pool->free_id_count++] = id;

	return last_dense;
}

internal u32 DensePoolDenseIndex(const DensePool *pool, u32 id)
{
	return pool->id_to_dense[id];
}

internal u32 DensePoolLiveCount(const DensePool *pool)
{
	return pool->count;
}
