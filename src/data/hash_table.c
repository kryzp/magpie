#include "hash_table.h"
#include "core/core_hash.h"

// This is not a complicated hash table implementation at all.
// Just a table of linked lists.

void hash_table_init(struct hash_table *table, struct memory_arena *arena, u64 node_size)
{
	table->arena = arena;
	table->node_size = node_size;
	table->count = 0;
}

void *hash_table_add(struct hash_table *table, u64 hash, void *element)
{
	u32 index = hash % array_size(table->buckets);

	struct hash_table_node *node = memory_arena_push(table->arena, sizeof(struct hash_table_node));
	node->hash = hash;
	node->data = memory_arena_push(table->arena, table->node_size);
	node->next = table->buckets[index];

	table->buckets[index] = node;

	memory_copy(node->data, element, table->node_size);

	table->count++;

	return table->buckets[index]->data;
}

void *hash_table_fetch(struct hash_table *table, u64 hash)
{
	u32 index = hash % array_size(table->buckets);

	struct hash_table_node *curr = table->buckets[index];

	while (curr) {
		if (curr->hash == hash)
			return curr->data;

		curr = curr->next;
	}

	return 0;
}
