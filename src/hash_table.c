
// This is not a complicated hash table implementation at all.
// Just a table of linked lists.

internal void HashTableInit(HashTable *table, MemoryArena *arena, u64 node_size)
{
	table->arena = arena;
	table->node_size = node_size;
	table->count = 0;
}

internal void *HashTableAddElement(HashTable *table, u64 hash, void *element)
{
	u32 index = hash % ArraySize(table->buckets);

	HashTableNode *node = MemoryArenaPush(table->arena, sizeof(HashTableNode));
	node->hash = hash;
	node->data = MemoryArenaPush(table->arena, table->node_size);
	node->next = table->buckets[index];

	table->buckets[index] = node;

	MemoryCopy(node->data, element, table->node_size);

	table->count++;

	return table->buckets[index]->data;
}

internal void *HashTableFetchElement(HashTable *table, u64 hash)
{
	u32 index = hash % ArraySize(table->buckets);

	HashTableNode *curr = table->buckets[index];

	while (curr) {
		if (curr->hash == hash)
			return curr->data;

		curr = curr->next;
	}

	return 0;
}
