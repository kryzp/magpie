
internal const R_ResourceState *R_ResourceTrackerFindTexture(R_ResourceTracker *tracker, G_ResourceKey key)
{
	for (u32 i = 0; i < tracker->texture_count; i++)
	{
		R_TrackedTexture *t = &tracker->textures[i];

		if (G_ResourceKeyMatch(key, t->key))
			return &t->state;
	}

	return NULL;
}

internal const R_ResourceState *R_ResourceTrackerFindBuffer(R_ResourceTracker *tracker, G_ResourceKey key)
{
	for (u32 i = 0; i < tracker->buffer_count; i++)
	{
		R_TrackedBuffer *b = &tracker->buffers[i];

		if (G_ResourceKeyMatch(key, b->key))
			return &b->state;
	}

	return NULL;
}

internal void R_ResourceTrackerSetTexture(R_ResourceTracker *tracker, G_ResourceKey key, R_ResourceState state)
{
	for (u32 i = 0; i < tracker->texture_count; i++)
	{
		R_TrackedTexture *t = &tracker->textures[i];

		if (G_ResourceKeyMatch(key, t->key))
		{
			t->state = state;
			return;
		}
	}

	R_TrackedTexture t = {0};
	t.key = key;
	t.state = state;

	AssertTrue(tracker->texture_count < ArraySize(tracker->textures));

	tracker->textures[tracker->texture_count] = t;
	tracker->texture_count++;
}

internal void R_ResourceTrackerSetBuffer(R_ResourceTracker *tracker, G_ResourceKey key,  R_ResourceState state)
{
	for (u32 i = 0; i < tracker->buffer_count; i++)
	{
		R_TrackedBuffer *b = &tracker->buffers[i];

		if (G_ResourceKeyMatch(key, b->key))
		{
			b->state = state;
			return;
		}
	}

	R_TrackedBuffer b = {0};
	b.key = key;
	b.state = state;

	AssertTrue(tracker->buffer_count < ArraySize(tracker->buffers));

	tracker->buffers[tracker->buffer_count] = b;
	tracker->buffer_count++;
}
