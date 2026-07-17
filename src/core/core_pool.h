#ifndef CORE_POOL_H
#define CORE_POOL_H

typedef struct SlotPool SlotPool;
struct SlotPool
{
	u32 *free_indices;
	u32 free_index_count;
	u32 capacity;
};

static void SlotPoolInit(SlotPool *pool, Arena *arena, u32 capacity);
static b32 SlotPoolAlloc(SlotPool *pool, u32 *out_index);
static void SlotPoolFree(SlotPool *pool, u32 index);
static u32 SlotPoolLiveCount(const SlotPool *pool);

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

static void DensePoolInit(DensePool *pool, Arena *arena, u32 capacity);
static u32 DensePoolGetStableID(DensePool *pool);
static u32 DensePoolFreeID(DensePool *pool, u32 id);
static u32 DensePoolDenseIndex(const DensePool *pool, u32 id);
static u32 DensePoolLiveCount(const DensePool *pool);

#endif // CORE_POOL_H
