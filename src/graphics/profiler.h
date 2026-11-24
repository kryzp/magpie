#ifndef GRAPHICS_PROFILER_H
#define GRAPHICS_PROFILER_H

#include "core/types.h"
#include "graphics/swapchain.h"

#include "container/string.h"

#include <volk/volk.h>

#define GRAPHICS_PROFILER_MAX_QUERIES_PER_FRAME 512
#define GRAPHICS_PROFILER_MAX_EVENTS            512

namespace gfx {

class CmdBuffer;

enum ProfileType {
	ProfileType_Timestamp,
	ProfileType_PipelineStatistics,
	ProfileType_MaxEnum
};

struct ProfileQuery {
	u64 name_hash = 0;
	u64 stat = 0;
};

struct ProfileQueryList {
	ProfileQuery queries[GRAPHICS_PROFILER_MAX_QUERIES_PER_FRAME];
	unsigned count = 0;
};

struct ProfileEvent {
	const char *name = nullptr;
	ProfileType type = ProfileType_Timestamp;
	u64 ts_start = 0;
	u64 ts_end = 0;
	u64 query = 0;
};

struct ProfilePool {
	VkQueryPool vk_pool = VK_NULL_HANDLE;
	ProfileEvent events[GRAPHICS_PROFILER_MAX_EVENTS];
	u64 count = 0;
};

struct ProfilerFrame {
	ProfilePool pools[ProfileType_MaxEnum];
};

struct ProfileScope {
	const CmdBuffer *cmd = nullptr;
	const char *name = nullptr;
	u64 id = 0;
};

class Profiler {
public:
	Profiler();
	~Profiler();

	Profiler(const Profiler &) = delete;
	Profiler &operator=(const Profiler &) = delete;

	static Profiler *Get();

	void GrabQueries();

	void AddEvent(const ProfileEvent &event);
	VkQueryPool GetVkPool(ProfileType type);
	u64 GetNewID(ProfileType type);

	u64 GetRawStat(ProfileType type, const char *name) const;
	double GetTimer(const char *name) const;
	int GetPipelineStatistics(const char *name) const;

	ProfileScope Begin(const CmdBuffer *cmd, const char *name);
	void End(const ProfileScope &scope);

	ProfileScope StatsBegin(const CmdBuffer *cmd, const char *name);
	void StatsEnd(const ProfileScope &scope);

private:
	float period;
	ProfileQueryList query_lists[ProfileType_MaxEnum];
	ProfilerFrame frames[GRAPHICS_FRAMES_IN_FLIGHT];
};

}

#endif // GRAPHICS_PROFILER_H
