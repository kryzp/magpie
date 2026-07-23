#ifndef RENDER_GRAPH_TRACKER_H
#define RENDER_GRAPH_TRACKER_H

#define R_TRACKER_MAX_TRACKED 128

typedef struct R_TrackedTexture R_TrackedTexture;
struct R_TrackedTexture
{
	G_ResourceKey key;
	R_ResourceState state;
};

typedef struct R_TrackedBuffer R_TrackedBuffer;
struct R_TrackedBuffer
{
	G_ResourceKey key;
	R_ResourceState state;
};

typedef struct R_ResourceTracker R_ResourceTracker;
struct R_ResourceTracker
{
	u32 texture_count;
	R_TrackedTexture textures[R_TRACKER_MAX_TRACKED];
	
	u32 buffer_count;
	R_TrackedBuffer buffers[R_TRACKER_MAX_TRACKED];
};

// return NULL if not tracked!!
internal const R_ResourceState *R_ResourceTrackerFindTexture (R_ResourceTracker *tracker, G_ResourceKey key);
internal const R_ResourceState *R_ResourceTrackerFindBuffer  (R_ResourceTracker *tracker, G_ResourceKey key);

internal void R_ResourceTrackerSetTexture (R_ResourceTracker *tracker, G_ResourceKey key, R_ResourceState state);
internal void R_ResourceTrackerSetBuffer  (R_ResourceTracker *tracker, G_ResourceKey key,  R_ResourceState state);

#endif // RENDER_GRAPH_TRACKER_H
