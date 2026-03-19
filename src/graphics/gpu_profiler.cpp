#include "gpu_profiler.h"
#include "vk_check.h"

#include "core/scratch.h"

using namespace gfx;

GpuProfiler::GpuProfiler()
	: device(nullptr)
	, period(0.f)
	, data{}
	, frames{}
{
}

GpuProfiler::~GpuProfiler()
{
}

GpuProfiler *GpuProfiler::get_singleton()
{
	static GpuProfiler instance;
	return &instance;
}

void GpuProfiler::init(Device *device)
{
	this->device = device;
	this->period = device->get_context().get_physical_properties().limits.timestampPeriod;
	
	VkQueryType pool_types[GPU_PROFILE_MAX_ENUM] = {
		VK_QUERY_TYPE_TIMESTAMP,
		VK_QUERY_TYPE_PIPELINE_STATISTICS
	};

	VkQueryPoolCreateInfo query_pool_info = {};
	query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	query_pool_info.queryCount = MAX_QUERY_TIMESTAMPS_PER_FRAME;
	query_pool_info.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		auto &frame = frames[i];
		
		for (int j = 0; j < GPU_PROFILE_MAX_ENUM; j++) {
			auto &pool = frame.pools[j];

			query_pool_info.queryType = pool_types[j];
		
			GFX_VK_CHECK(
				vkCreateQueryPool(
					device->get_context().get_device(),
					&query_pool_info, nullptr,
					&pool.vk_pool
				),
				"Failed to create query pool."
			);
		}
	}
}

void GpuProfiler::destroy()
{
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		auto &frame = frames[i];

		for (auto &pool : frame.pools)
			device->destroy_query_pool(pool.vk_pool);
	}
}

void GpuProfiler::grab_queries(CommandBuffer &cmd)
{
	ScratchScope scratch = scratch::get();

	auto &frame = frames[device->get_current_frame_index()];

	for (auto &pool : frame.pools) {
		if (pool.count == 0)
			continue;

		u64 *queries = scratch.arena().array<u64>(pool.count);

		vkGetQueryPoolResults(
			device->get_context().get_device(),
			pool.vk_pool,
			0,
			pool.count,
			pool.count * sizeof(u64),
			queries,
			sizeof(u64),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
		);

		for (auto &ev : pool.events) {
			u64 result = 0;

			switch (ev.type) {
				case GPU_PROFILE_TIMESTAMP:
					result = queries[ev.start] - queries[ev.end];
					break;

				case GPU_PROFILE_PIPELINE_STATS:
					result = queries[ev.query];
					break;
			}

			data[ev.type][ev.name] = result;
		}
	}

	for (auto &pool : frame.pools) {
		cmd.reset_queries(pool.vk_pool, 0, MAX_QUERY_TIMESTAMPS_PER_FRAME);
		pool.count = 0;
		pool.events.clear();
	}
}

void GpuProfiler::add_event(const GpuProfileEvent &event)
{
	auto &frame = frames[device->get_current_frame_index()];
	frame.pools[event.type].events.push_back(event);
}

VkQueryPool GpuProfiler::get_pool(GpuProfileType type) const
{
	auto &frame = frames[device->get_current_frame_index()];
	return frame.pools[type].vk_pool;
}

u64 GpuProfiler::get_new_id(GpuProfileType type)
{
	auto &frame = frames[device->get_current_frame_index()];
	u64 id = frame.pools[type].count;
	frame.pools[type].count++;
	return id;
}
