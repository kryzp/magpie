#ifndef GRAPHICS_PROFILER_H
#define GRAPHICS_PROFILER_H

#define G_PROFILER_MAX_QUERIES_PER_FRAME    512
#define G_PROFILER_MAX_EVENTS               512

typedef enum G_ProfileType
{
	G_ProfileType_Timestamp,
	G_ProfileType_PipelineStatistics,
	G_ProfileType_COUNT
}
G_ProfileType;

typedef struct G_ProfileQuery G_ProfileQuery;
struct G_ProfileQuery
{
	u64 name_hash;
	u64 stat;
};

typedef struct G_ProfileQueryList G_ProfileQueryList;
struct G_ProfileQueryList
{
	G_ProfileQuery queries[G_PROFILER_MAX_QUERIES_PER_FRAME];
	u32 count;
};

typedef struct G_ProfileEvent G_ProfileEvent;
struct G_ProfileEvent
{
	String8 name;
	G_ProfileType type;

	union
	{
		struct
		{
			u64 start;
			u64 end;
		}
		ts;

		u64 query;
	};
};

typedef struct G_ProfilePool G_ProfilePool;
struct G_ProfilePool
{
	VkQueryPool vk_pool;
	G_ProfileEvent events[G_PROFILER_MAX_EVENTS];
	u64 count;
};

typedef struct G_ProfilerFrame G_ProfilerFrame;
struct G_ProfilerFrame
{
	G_ProfilePool pools[G_ProfileType_COUNT];
};

typedef struct G_Profiler G_Profiler;
struct G_Profiler
{
	Arena *arena;
	f32 period;
	G_ProfileQueryList query_lists[G_ProfileType_COUNT];
	G_ProfilerFrame frames[G_FRAMES_IN_FLIGHT];
};

internal void G_ProfilerInitAndSelect(G_Profiler *profiler);
internal void G_ProfilerDestroy(void);
internal void G_ProfilerSelectContext(G_Profiler *profiler);

internal void G_ProfilerGrabQueries(void);

internal void G_ProfilerAddEvent(const G_ProfileEvent *event);
internal VkQueryPool G_ProfilerGetVkPool(G_ProfileType type);
internal u64 G_ProfilerGetNewID(G_ProfileType type);

internal u64 G_ProfilerGetRawStat(G_ProfileType type, String8 name);
internal f64 G_ProfilerGetTimer(String8 name);
internal i32 G_ProfilerGetPipelineStatistics(String8 name);

typedef struct G_ProfileScope G_ProfileScope;
struct G_ProfileScope
{
	const G_CmdBuffer *cmd;
	String8 name;
	u64 id;
};

internal G_ProfileScope G_ProfileBegin(const G_CmdBuffer *cmd, String8 name);
internal void G_ProfileEnd(const G_ProfileScope *scope);

internal G_ProfileScope G_ProfileStatsBegin(const G_CmdBuffer *cmd, String8 name);
internal void G_ProfileStatsEnd(const G_ProfileScope *scope);

#endif // GRAPHICS_PROFILER_H
