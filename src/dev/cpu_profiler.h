#pragma once

#include <mutex>

#include "core/types.h"
#include "container/stack.h"
#include "container/vector.h"

namespace dev
{
	struct CpuProfileSample {
		const char *name;
		u64 t_start;
		u64 t_end;

		constexpr u64 get_duration() const
		{
			return t_end - t_start;
		}
	};

	class CpuProfiler {
		constexpr static u32 INITAL_SAMPLE_STORAGE_SIZE = 1024;

	public:
		CpuProfiler();
		~CpuProfiler();

		static CpuProfiler *get_singleton();

		void frame(const char *name, u32 worker_count);

		void store_sample(const CpuProfileSample &sample);
	
		void dump_to_json(const char *filename);

		u64 get_timestamp() const;

	private:
		std::mutex mutex;
		Vector<Vector<CpuProfileSample>> per_worker_samples;
	};

	struct CpuProfileScope {
		CpuProfileScope(const char *name)
			: name(name)
		{
			start_time = CpuProfiler::get_singleton()->get_timestamp();
		}

		~CpuProfileScope()
		{
			CpuProfileSample sample = {};
			sample.name = name;
			sample.t_start = start_time;
			sample.t_end = CpuProfiler::get_singleton()->get_timestamp();

			CpuProfiler::get_singleton()->store_sample(sample);
		}

		const char *name;
		u32 start_time;
	};
}

#define DEV_PROFILE_ENABLED 1

#if DEV_PROFILE_ENABLED
#  define DEV_PROFILE_SCOPE(name_) ::dev::CpuProfileScope MCONCAT_EXP(profile_scope_, __LINE__)(name_);
#  define DEV_PROFILE_FUNCTION() DEV_PROFILE_SCOPE(__FUNCTION__)
#else
#  define DEV_PROFILE_SCOPE(name_)
#  define DEV_PROFILE_FUNCTION()
#endif
