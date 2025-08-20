
typedef struct HashTableNode
{
	struct HashTableNode *next;
	u64 hash;
	void *data;
}
HashTableNode;

typedef struct HashTable
{
	MemoryArena *arena;
	u64 node_size;
	u64 count;
	HashTableNode *buckets[32];
}
HashTable;
