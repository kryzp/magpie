#ifndef GRAPHICS_PROFILER_H
#define GRAPHICS_PROFILER_H

typedef enum G_ProfileType
{
	G_ProfileType_Timestamp,
	G_ProfileType_PipelineStatistics,
	G_ProfileType_COUNT
}
G_ProfileType;

typedef struct G_ProfileEvent G_ProfileEvent;
struct G_ProfileEvent
{
	G_ProfileEvent *next;
	
	String8 name;
	G_ProfileType type;
	u64 start;
	u64 end;
	u64 query;
};

typedef struct G_ProfilePool G_ProfilePool;
struct G_ProfilePool
{
	G_ProfileEvent *event_head;
	VkQueryPool vk_pool;
	u64 count;
};

typedef struct G_ProfilerFrame G_ProfilerFrame;
struct G_ProfilerFrame
{
	G_ProfilePool pools[G_ProfileType_COUNT];
};

typedef struct G_ProfileQueryNode G_ProfileQueryNode;
struct G_ProfileQueryNode
{
	G_ProfileQueryNode *next;
	u64 key;
	u64 query;
};

typedef struct G_Profiler G_Profiler;
struct G_Profiler
{
	Arena *arena;
	f32 period;
	G_ProfileQueryNode *query_head;
	G_ProfilerFrame frames[G_FRAMES_IN_FLIGHT];
};

#endif // GRAPHICS_PROFILER_H
