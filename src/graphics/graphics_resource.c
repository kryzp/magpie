
internal void G_ResourceListInit(G_ResourceList *list)
{
	list->first_sentinel.next = &list->first_sentinel;
	list->first_sentinel.prev = &list->first_sentinel;

	list->free_sentinel.next = &list->free_sentinel;
	list->free_sentinel.prev = &list->free_sentinel;

	list->next_auto_key = 1; // 0 is the null key.
	list->lock = 0;
}

// Caller holds the lock.
internal G_ResourceListNode *G_ResourceListTakeNodeLocked(G_ResourceList *list, Arena *arena)
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

	return node;
}

// Caller holds the lock.
internal void G_ResourceListLinkFrontLocked(G_ResourceList *list, G_ResourceListNode *node)
{
	node->next = list->first_sentinel.next;
	node->prev = &list->first_sentinel;

	node->next->prev = node;
	node->prev->next = node;
}

internal G_ResourceKey G_ResourceListPush(G_ResourceList *list, Arena *arena, const G_Resource *resource, G_ResourceKey key)
{
	osapi->SpinLockAcquire(&list->lock);

	G_ResourceListNode *node = G_ResourceListTakeNodeLocked(list, arena);
	node->key = key;
	node->resource = *resource;
	G_ResourceListLinkFrontLocked(list, node);

	osapi->SpinLockRelease(&list->lock);

	return key;
}

internal G_ResourceKey G_ResourceListPushAuto(G_ResourceList *list, Arena *arena, const G_Resource *resource)
{
	osapi->SpinLockAcquire(&list->lock);

	G_ResourceListNode *node = G_ResourceListTakeNodeLocked(list, arena);
	node->key.value = list->next_auto_key++;
	node->resource = *resource;
	G_ResourceListLinkFrontLocked(list, node);

	G_ResourceKey key = node->key;

	osapi->SpinLockRelease(&list->lock);

	return key;
}

internal G_Resource *G_ResourceListGet(G_ResourceList *list, G_ResourceKey key)
{
	G_Resource *result = NULL;

	osapi->SpinLockAcquire(&list->lock);

	for (G_ResourceListNode *node = list->first_sentinel.next;
		 node != &list->first_sentinel;
		 node = node->next)
	{
		if (G_ResourceKeyMatch(key, node->key))
		{
			result = &node->resource;
			break;
		}
	}

	osapi->SpinLockRelease(&list->lock);

	return result;
}

internal void G_ResourceListReturn(G_ResourceList *list, G_ResourceKey key)
{
	osapi->SpinLockAcquire(&list->lock);

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

		break;
	}

	osapi->SpinLockRelease(&list->lock);
}
