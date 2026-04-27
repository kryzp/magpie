#ifndef CORE_ARENA_H
#define CORE_ARENA_H

typedef enum ArenaKind
{
	ArenaKind_Backed, // Actual memory backed by a reserved allocation.
	ArenaKind_View,   // Arena operating on top of existing reserved memory.
	ArenaKind_COUNT
}
ArenaKind;

typedef struct Arena Arena;
struct Arena
{
	void *base;
	u64 capacity;
	u64 used;
	u64 last_alloc_offset;
	u64 committed; // used for backed memory.
	ArenaKind kind;
};

internal Arena ArenaInitReserved(u64 size);
internal Arena ArenaInitMemory(void *memory, u64 capacity);
internal Arena ArenaInitArena(Arena *arena, u64 capacity, u64 alignment);

internal void ArenaRelease(Arena *arena);

/*
 * Partitioning arenas into sub-arenas can be a pain in the ass
 * because memory is aligned upwards to better performance, so
 * just allocating two arenas of size parent->capacity / 2 onto
 * the parent arena can crash it.
 */
internal u64 ArenaSafePartitionSize(const Arena *parent, u32 count, u64 alignment);

internal void *ArenaPush(Arena *arena, u64 bytes, u64 alignment);

internal void ArenaPopTo(Arena *arena, u64 to);
internal void ArenaPop(Arena *arena, u64 bytes);

internal void ArenaResizeLastBy(Arena *arena, u64 bytes);
internal void ArenaResizeLastTo(Arena *arena, u64 bytes);

internal void ArenaClear(Arena *arena);
internal void ArenaZero(Arena *arena);

#define ArenaPushArray(arena, type, count) ArenaPush((arena), sizeof(type) * (count), _Alignof(type))

#endif // CORE_ARENA_H
