#pragma once

#include <mutex>

#include "container/vector.h"
#include "job/job.h"

namespace dev
{
	struct ProfileEvent {
		const char *name;
		u64 start_us;
		u64 end_us;
		u32 worker_id;
		u64 fiber_id;

		u64 get_duration_us() const
		{
			return end_us - start_us;
		}
	};

	class CpuProfiler {
	public:
		CpuProfiler();
		~CpuProfiler();

		static CpuProfiler *get_singleton();

		void add_event(const ProfileEvent &event);

		void dump_to_file(const char *filepath);

		u64 get_timestamp_us() const;

	private:
		std::mutex mutex;
		Vector<ProfileEvent> events;
	};

	class CpuProfileScope {
	public:
		CpuProfileScope(const char *name)
			: name(name)
		{
			start_time_us = CpuProfiler::get_singleton()->get_timestamp_us();
			worker_id = job::get_current_worker_id();
			fiber_id = (u64)job::get_current_fiber_handle();
		}

		~CpuProfileScope()
		{
			u64 end_time_us = CpuProfiler::get_singleton()->get_timestamp_us();

			ProfileEvent event = {};
			event.name = name;
			event.start_us = start_time_us;
			event.end_us = end_time_us;
			event.worker_id = worker_id;
			event.fiber_id = fiber_id;

			CpuProfiler::get_singleton()->add_event(event);
		}

	private:
		const char *name;
		u64 start_time_us;
		u32 worker_id;
		u64 fiber_id;
	};
}

#ifdef DEV_PROFILING_ENABLED
#  define DEV_PROFILE_SCOPE(name_) ::dev::CpuProfileScope MCONCAT_EXP(profile_scope_, __LINE__)(name_);
#  define DEV_PROFILE_FUNCTION() DEV_PROFILE_SCOPE(__FUNCTION__)
#else
#  define DEV_PROFILE_SCOPE(name_)
#  define DEV_PROFILE_FUNCTION()
#endif
