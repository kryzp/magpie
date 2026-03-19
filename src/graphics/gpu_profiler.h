#pragma once

// Based off: https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_profiler.h

#include "core/types.h"

#include "container/vector.h"
#include "container/string.h"
#include "container/hash_map.h"

#include "device.h"
#include "command_buffer.h"

namespace gfx
{
	enum GpuProfileType {
		GPU_PROFILE_TIMESTAMP,
		GPU_PROFILE_PIPELINE_STATS,
		GPU_PROFILE_MAX_ENUM
	};

	struct GpuProfileEvent {
		const char *name;
		GpuProfileType type;
		u64 start;
		u64 end;
		u64 query;
	};

	class GpuProfiler {
		constexpr static u32 MAX_QUERY_TIMESTAMPS_PER_FRAME = 512;

	public:
		GpuProfiler();
		~GpuProfiler();

		static GpuProfiler *get_singleton();

		void init(Device *device);
		void destroy();

		void grab_queries(CommandBuffer &cmd);

		template <typename T>
		T get_statistic(GpuProfileType type, const char *name) const
		{
			auto it = data[type].find(name);
			if (it != data[type].end())
				return (T)it->second;
			else
				return (T)0;
		}

		double get_timer(const char *name) const
		{
			double duration = get_statistic<double>(GPU_PROFILE_TIMESTAMP, name);
			return duration * (double)period / 1'000'000.0;
		}

		s32 get_pipeline_stats(const char *name) const
		{
			s32 stats = get_statistic<s32>(GPU_PROFILE_PIPELINE_STATS, name);
			return stats;
		}

		void add_event(const GpuProfileEvent &event);
		VkQueryPool get_pool(GpuProfileType type) const;
		u64 get_new_id(GpuProfileType type);

	private:
		Device *device;

		float period;

		HashMap<String, u64> data[GPU_PROFILE_MAX_ENUM];

		struct PerFrameData {
			struct ProfilePool {
				Vector<GpuProfileEvent> events;
				VkQueryPool vk_pool;
				u64 count = 0;
			};

			ProfilePool pools[GPU_PROFILE_MAX_ENUM];
		};

		PerFrameData frames[FRAMES_IN_FLIGHT];
	};

	class GpuProfileScope {
	public:
		GpuProfileScope(CommandBuffer &cmd, const char *name)
			: cmd(cmd), name(name)
		{
			auto *profiler = GpuProfiler::get_singleton();

			start = profiler->get_new_id(GPU_PROFILE_TIMESTAMP);

			cmd.write_timestamp(VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, profiler->get_pool(GPU_PROFILE_TIMESTAMP), start);
		}

		~GpuProfileScope()
		{
			auto *profiler = GpuProfiler::get_singleton();

			u64 end = profiler->get_new_id(GPU_PROFILE_TIMESTAMP);

			cmd.write_timestamp(VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, profiler->get_pool(GPU_PROFILE_TIMESTAMP), end);

			GpuProfileEvent event = {};
			event.name = name;
			event.type = GPU_PROFILE_TIMESTAMP;
			event.start = start;
			event.end = end;

			profiler->add_event(event);
		}

	private:
		CommandBuffer &cmd;
		const char *name;
		u64 start;
	};

	class GpuPipelineStatScope {
	public:
		GpuPipelineStatScope(CommandBuffer &cmd, const char *name)
			: cmd(cmd), name(name)
		{
			auto *profiler = GpuProfiler::get_singleton();

			query = profiler->get_new_id(GPU_PROFILE_PIPELINE_STATS);

			cmd.begin_query(profiler->get_pool(GPU_PROFILE_PIPELINE_STATS), query, 0);
		}

		~GpuPipelineStatScope()
		{
			auto *profiler = GpuProfiler::get_singleton();

			cmd.end_query(profiler->get_pool(GPU_PROFILE_PIPELINE_STATS), query);

			GpuProfileEvent event = {};
			event.name = name;
			event.type = GPU_PROFILE_PIPELINE_STATS;
			event.query = query;

			profiler->add_event(event);
		}

	private:
		CommandBuffer &cmd;
		const char *name;
		u32 query;
	};
}

#ifdef DEV_PROFILING_ENABLED
#  define GFX_PROFILE_SCOPE(cmd_, name_) ::gfx::GpuProfileScope MCONCAT_EXP(gpu_profile_scope_, __LINE__)(cmd_, name_);
#  define GFX_PROFILE_FUNCTION(cmd_) GFX_PROFILE_SCOPE(cmd_, __FUNCTION__)
#  define GFX_PROFILE_STATS(cmd_, name_) ::gfx::GpuPipelineStatScope MCONCAT_EXP(gpu_profile_stats_, __LINE__)(cmd_, name_);
#  define GFX_PROFILE_COLLECT(cmd_) ::gfx::GpuProfiler::get_singleton()->grab_queries(cmd_)
#else
#  define GFX_PROFILE_SCOPE(cmd_, name_)
#  define GFX_PROFILE_FUNCTION(cmd_)
#  define GFX_PROFILE_STATS(cmd_, name_)
#  define GFX_PROFILE_COLLECT(cmd_)
#endif
