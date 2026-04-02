#ifndef GRAPHICS_PROFILER_H
#define GRAPHICS_PROFILER_H

typedef enum GFX_ProfileType
{
	GFX_ProfileType_Timestamp,
	GFX_ProfileType_PipelineStatistics,
	GFX_ProfileType_COUNT
}
GFX_ProfileType;

typedef struct GFX_ProfileEvent GFX_ProfileEvent;
struct GFX_ProfileEvent
{
	GFX_ProfileEvent *next;
	
	String8 name;
	GFX_ProfileType type;
	u64 start;
	u64 end;
	u64 query;
};

typedef struct GFX_ProfilePool GFX_ProfilePool;
struct GFX_ProfilePool
{
	GFX_ProfileEvent *event_head;
	VkQueryPool vk_pool;
	u64 count;
};

typedef struct GFX_ProfilerFrame GFX_ProfilerFrame;
struct GFX_ProfilerFrame
{
	GFX_ProfilePool pools[GFX_ProfileType_COUNT];
};

typedef struct GFX_ProfileQueryNode GFX_ProfileQueryNode;
struct GFX_ProfileQueryNode
{
	GFX_ProfileQueryNode *next;
	u64 key;
	u64 query;
};

typedef struct GFX_Profiler GFX_Profiler;
struct GFX_Profiler
{
	Arena *arena;
	f32 period;
	GFX_ProfileQueryNode *query_head;
	GFX_ProfilerFrame frames[GFX_FRAMES_IN_FLIGHT];
};

#endif // GRAPHICS_PROFILER_H
