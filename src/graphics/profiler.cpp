#include "graphics/profiler.h"
#include "graphics/device.h"
#include "graphics/cmd_buffer.h"
#include "core/hash.h"

#include "container/vector.h"

static gfx::Profiler *selected_profiler = nullptr;

gfx::Profiler *gfx::Profiler::Get()
{
	return selected_profiler;
}

gfx::Profiler::Profiler()
{
	Device *device = Device::Get();

	period = device->vk_physical_device_properties.properties.limits.timestampPeriod;

	static const VkQueryType query_types[] = {
		VK_QUERY_TYPE_TIMESTAMP,          // ProfileType_Timestamp
		VK_QUERY_TYPE_PIPELINE_STATISTICS // ProfileType_PipelineStatistics
	};

	for (unsigned i = 0; i < GRAPHICS_FRAMES_IN_FLIGHT; i++) {
		for (unsigned j = 0; j < ProfileType_MaxEnum; j++) {
			VkQueryPipelineStatisticFlags pipeline_stat_flags = 0;

			if (query_types[j] == VK_QUERY_TYPE_PIPELINE_STATISTICS)
				pipeline_stat_flags = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;

			frames[i].pools[j].vk_pool = device->QueryPoolCreate(
				GRAPHICS_PROFILER_MAX_QUERIES_PER_FRAME,
				query_types[j],
				pipeline_stat_flags
			);
		}
	}

	selected_profiler = this;
}

gfx::Profiler::~Profiler()
{
	Device *device = Device::Get();

	for (unsigned i = 0; i < GRAPHICS_FRAMES_IN_FLIGHT; i++) {
		for (unsigned j = 0; j < ProfileType_MaxEnum; j++) {
			device->QueryPoolDestroy(frames[i].pools[j].vk_pool);
		}
	}

	if (selected_profiler == this)
		selected_profiler = nullptr;
}

void gfx::Profiler::GrabQueries()
{
	Device *device = Device::Get();

	ProfilerFrame *current_frame = &frames[device->GetFrameInFlightIndex()];

	for (int i = 0; i < ProfileType_MaxEnum; i++) {
		ProfilePool *pool = &current_frame->pools[i];
		ProfileQueryList *query_list = &query_lists[i];

		if (pool->count <= 0)
			continue;

		Vector<u64> queries(pool->count);

		vkGetQueryPoolResults(
			device->vk_device,
			pool->vk_pool,
			0,
			pool->count,
			pool->count * sizeof(u64),
			queries.data(),
			sizeof(u64),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
		);

		for (int j = 0; j < pool->count; j++) {
			const ProfileEvent &ev = pool->events[j];

			u64 stat = 0;

			switch (ev.type) {
				case ProfileType_Timestamp:
					stat = queries[ev.ts_end] - queries[ev.ts_start];
					break;

				case ProfileType_PipelineStatistics:
					stat = queries[ev.query];
					break;

				default:
					break;
			}

			AssertTrue(query_list->count < ArraySize(query_list->queries));

			query_list->queries[query_list->count].name_hash = hash::CStr(ev.name);
			query_list->queries[query_list->count].stat = stat;

			query_list->count++;
		}

		pool->count = 0;
	}
}

void gfx::Profiler::AddEvent(const ProfileEvent &event)
{
	ProfilerFrame *current_frame = &frames[Device::Get()->GetFrameInFlightIndex()];
	ProfilePool *current_pool = &current_frame->pools[event.type];

	AssertTrue(current_pool->count < ArraySize(current_pool->events));

	current_pool->events[current_pool->count++] = event;
}

VkQueryPool gfx::Profiler::GetVkPool(ProfileType type)
{
	ProfilerFrame *current_frame = &frames[Device::Get()->GetFrameInFlightIndex()];
	return current_frame->pools[type].vk_pool;
}

u64 gfx::Profiler::GetNewID(ProfileType type)
{
	ProfilerFrame *current_frame = &frames[Device::Get()->GetFrameInFlightIndex()];
	return current_frame->pools[type].count++;
}

u64 gfx::Profiler::GetRawStat(ProfileType type, const char *name) const
{
	const ProfileQueryList *list = &query_lists[type];

	u64 name_hash = hash::CStr(name);

	for (unsigned i = 0; i < list->count; i++) {
		const ProfileQuery *query = &list->queries[i];

		if (query->name_hash == name_hash)
			return query->stat;
	}

	return 0;
}

double gfx::Profiler::GetTimer(const char *name) const
{
	double dur = (double)GetRawStat(ProfileType_Timestamp, name);
	return dur * (double)period / 1000000.0;
}

int gfx::Profiler::GetPipelineStatistics(const char *name) const
{
	int pipeline_stat = (int)GetRawStat(ProfileType_PipelineStatistics, name);
	return pipeline_stat;
}

gfx::ProfileScope gfx::Profiler::Begin(const CmdBuffer *cmd, const char *name)
{
	ProfileScope scope;
	scope.cmd = cmd;
	scope.name = name;
	scope.id = GetNewID(ProfileType_Timestamp);

	cmd->WriteTimestamp(
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		GetVkPool(ProfileType_Timestamp),
		scope.id
	);

	return scope;
}

void gfx::Profiler::End(const ProfileScope &scope)
{
	u64 end_id = GetNewID(ProfileType_Timestamp);

	scope.cmd->WriteTimestamp(
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		GetVkPool(ProfileType_Timestamp),
		end_id
	);

	ProfileEvent event;
	event.name = scope.name;
	event.type = ProfileType_Timestamp;
	event.ts_start = scope.id;
	event.ts_end = end_id;

	AddEvent(event);
}

gfx::ProfileScope gfx::Profiler::StatsBegin(const CmdBuffer *cmd, const char *name)
{
	ProfileScope scope;
	scope.cmd = cmd;
	scope.name = name;
	scope.id = GetNewID(ProfileType_PipelineStatistics);

	cmd->BeginQuery(
		GetVkPool(ProfileType_PipelineStatistics),
		scope.id, 0
	);

	return scope;
}

void gfx::Profiler::StatsEnd(const ProfileScope &scope)
{
	scope.cmd->EndQuery(
		GetVkPool(ProfileType_PipelineStatistics),
		scope.id
	);

	ProfileEvent event;
	event.name = scope.name;
	event.type = ProfileType_PipelineStatistics;
	event.query = scope.id;

	AddEvent(event);
}
