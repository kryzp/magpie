
internal const R_ResourceState *
R_ResourceTrackerFindTexture(R_ResourceTracker *tracker, GFX_TextureKey key)
{
	for (u32 i = 0; i < tracker->texture_count; i++)
	{
		R_TrackedTexture *t = &tracker->textures[i];

		if (GFX_TextureKeyMatch(key, t->key))
			return &t->state;
	}

	return NULL;
}

internal const R_ResourceState *
R_ResourceTrackerFindBuffer(R_ResourceTracker *tracker, GFX_BufferKey key)
{
	for (u32 i = 0; i < tracker->buffer_count; i++)
	{
		R_TrackedBuffer *b = &tracker->buffers[i];

		if (GFX_BufferKeyMatch(key, b->key))
			return &b->state;
	}

	return NULL;
}

internal void
R_ResourceTrackerSetTexture(R_ResourceTracker *tracker, GFX_TextureKey key, R_ResourceState state)
{
	for (u32 i = 0; i < tracker->texture_count; i++)
	{
		R_TrackedTexture *t = &tracker->textures[i];

		if (GFX_TextureKeyMatch(key, t->key))
		{
			t->state = state;
			return;
		}
	}

	R_TrackedTexture t = {0};
	t.key = key;
	t.state = state;

	tracker->textures[tracker->texture_count] = t;
	tracker->texture_count++;

	AssertTrue(tracker->texture_count < ArraySize(tracker->textures));
}

internal void
R_ResourceTrackerSetBuffer(R_ResourceTracker *tracker, GFX_BufferKey key,  R_ResourceState state)
{
	for (u32 i = 0; i < tracker->buffer_count; i++)
	{
		R_TrackedBuffer *b = &tracker->buffers[i];

		if (GFX_BufferKeyMatch(key, b->key))
		{
			t->state = state;
			return;
		}
	}

	R_TrackedBuffer b = {0};
	t.key = key;
	t.state = state;

	tracker->buffers[tracker->buffer_count] = b;
	tracker->buffer_count++;

	AssertTrue(tracker->buffer_count < ArraySize(tracker->buffers));
}
