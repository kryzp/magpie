#ifndef CORE_POOL_H
#define CORE_POOL_H

typedef struct SlotPool SlotPool;
struct SlotPool
{
	u32 *free_indices;
	u32 free_index_count;
	u32 capacity;
};

internal void SlotPoolInit(SlotPool *pool, Arena *arena, u32 capacity);
internal b32 SlotPoolAlloc(SlotPool *pool, u32 *out_index);
internal void SlotPoolFree(SlotPool *pool, u32 index);
internal u32 SlotPoolLiveCount(const SlotPool *pool);

typedef struct DensePool DensePool;
struct DensePool
{
	u32 *id_to_dense;
	u32 *dense_to_id;

	u32 *free_ids;
	u32 free_id_count;

	u32 count;
	u32 capacity;
};

internal void DensePoolInit(DensePool *pool, Arena *arena, u32 capacity);

internal u32 DensePoolGetStableID(DensePool *pool);

// USAGE:
//   u32 dense = DensePoolDenseIndex(pool, id);
//   u32 moved = DensePoolFreeID(pool, id);
//   data[dense] = data[moved]; // <-- done outside of the pool code, must be done manually!
internal u32 DensePoolFreeID(DensePool *pool, u32 id);

/*
#define DensePoolFree(pool_pointer, id, backing)				\
	do															\
	{															\
		u32 __dense = DensePoolIndexFromID((pool_pointer), (id)); \
		u32 __moved = DensePoolFreeID((pool_pointer), (id));		\
		(backing)[__dense] = (backing)[__moved];					\
	}															\
	while (0);
*/

internal u32 DensePoolIndexFromID(const DensePool *pool, u32 id);
internal u32 DensePoolIDFromIndex(const DensePool *pool, u32 index);

internal u32 DensePoolLiveCount(const DensePool *pool);

#endif // CORE_POOL_H
