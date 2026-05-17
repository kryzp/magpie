
internal b32
GFX_CmdPoolHasEmptyAcquireQueue(const GFX_CmdPool *pool)
{
	return pool->acquire_queue_front == -1;
}

internal b32
GFX_CmdPoolHasEmptyReleaseQueue(const GFX_CmdPool *pool)
{
	return pool->release_queue_front == -1;
}

internal b32
GFX_CmdPoolHasFullAcquireQueue(const GFX_CmdPool *pool)
{
	return ((pool->acquire_queue_back + 1) % ArraySize(pool->acquire_queue)) == pool->acquire_queue_front;
}

internal b32
GFX_CmdPoolHasFullReleaseQueue(const GFX_CmdPool *pool)
{
	return ((pool->release_queue_back + 1) % ArraySize(pool->release_queue)) == pool->release_queue_front;
}
