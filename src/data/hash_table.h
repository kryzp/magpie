#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "core/core_memory_arena.h"
#include "core/core_types.h"

struct hash_table_node {
	struct hash_table_node *next;
	u64 hash;
	void *data;
};

struct hash_table {
	struct memory_arena *arena;
	u64 node_size;
	u64 count;
	struct hash_table_node *buckets[32];
};

void hash_table_init(struct hash_table *table, struct memory_arena *arena, u64 node_size);
void *hash_table_add(struct hash_table *table, u64 hash, void *element);
void *hash_table_fetch(struct hash_table *table, u64 hash);

#endif // HASH_TABLE_H
