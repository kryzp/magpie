
internal void G_ResourceListInit(G_ResourceList *list)
{
	list->first_sentinel.next = &list->first_sentinel;
	list->first_sentinel.prev = &list->first_sentinel;
	
	list->free_sentinel.next = &list->free_sentinel;
	list->free_sentinel.prev = &list->free_sentinel;
}

internal G_ResourceKey G_ResourceListPush(G_ResourceList *list, Arena *arena, const G_Resource *resource, G_ResourceKey key)
{
	G_ResourceListNode *node = NULL;

	if (list->free_sentinel.next != &list->free_sentinel)
	{
		node = list->free_sentinel.next;

		node->prev->next = node->next;
		node->next->prev = node->prev;

		MemZeroStruct(node);
	}
	else
	{
		node = ArenaPushArray(arena, G_ResourceListNode, 1);
	}

	node->key = key;
	node->resource = *resource;
	
	node->next = list->first_sentinel.next;
	node->prev = &list->first_sentinel;

	node->next->prev = node;
	node->prev->next = node;
	
	return node->key;
}

internal G_ResourceKey G_ResourceListPushAuto(G_ResourceList *list, Arena *arena, const G_Resource *resource)
{
	G_ResourceListNode *node = NULL;

	if (list->free_sentinel.next != &list->free_sentinel)
	{
		node = list->free_sentinel.next;

		node->prev->next = node->next;
		node->next->prev = node->prev;

		MemZeroStruct(node);
	}
	else
	{
		node = ArenaPushArray(arena, G_ResourceListNode, 1);
	}

	node->key.value = list->first_sentinel.next ? (list->first_sentinel.next->key.value + 1) : 1;
	node->resource = *resource;
	
	node->next = list->first_sentinel.next;
	node->prev = &list->first_sentinel;

	node->next->prev = node;
	node->prev->next = node;
	
	return node->key;
}

internal G_Resource *G_ResourceListGet(const G_ResourceList *list, G_ResourceKey key)
{
	for (G_ResourceListNode *node = list->first_sentinel.next;
		 node != &list->first_sentinel;
		 node = node->next)
	{
		if (G_ResourceKeyMatch(key, node->key))
			return &node->resource;
	}

	return NULL;
}

internal void G_ResourceListReturn(G_ResourceList *list, G_ResourceKey key)
{
	for (G_ResourceListNode *node = list->first_sentinel.next;
		 node != &list->first_sentinel;
		 node = node->next)
	{
		if (!G_ResourceKeyMatch(key, node->key))
			continue;
	
		node->prev->next = node->next;
		node->next->prev = node->prev;

		node->next = list->free_sentinel.next;
		node->prev = &list->free_sentinel;

		node->next->prev = node;
		node->prev->next = node;

		return;
	}
}
