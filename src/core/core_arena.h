#ifndef CORE_ARENA_H
#define CORE_ARENA_H

typedef struct Arena Arena;
struct Arena
{
	void *base;
	u64 capacity;
	u64 used;
	u64 committed;
};

static Arena ArenaAlloc(u64 size);
static void  ArenaRelease(Arena *arena);

/*
 * Partitioning arenas into sub-arenas can be a pain in the ass
 * because memory is aligned upwards to better performance, so
 * just allocating two arenas of size parent->capacity / 2 onto
 * the parent arena can crash it.
 */
static u64 ArenaSafePartitionSize(const Arena *parent, u32 count, u64 alignment);

static void *ArenaPush(Arena *arena, u64 bytes, u64 alignment);

static void ArenaPopTo(Arena *arena, u64 to);
static void ArenaPop(Arena *arena, u64 bytes);

static void ArenaReset(Arena *arena);
static void ArenaResetAndDecommit(Arena *arena);
static void ArenaZero(Arena *arena);

#define ArenaPushArray(arena, type, count) ArenaPush((arena), sizeof(type) * (count), _Alignof(type))

#endif // CORE_ARENA_H
