#pragma once

// Resources:
//  * Parallelizing the Naughty Dog engine using fibers by Christian Gyrling
//  * Parallelizing the Physics Solver by Dennis Gustafsson

#include "core/types.h"
#include "container/vector.h"

#if defined(__x86_64__)
#  define JOB_SPIN_PAUSE() _mm_pause()
#else
#  define JOB_SPIN_PAUSE() std::this_thread::yield()
#endif

#define JOB_ENTRY_POINT(fname_) void fname_(uptr param)
#define JOB_PARALLEL_FOR(fname_, index_) void fname_(u32 index_)

namespace job
{
	typedef JOB_ENTRY_POINT(EntryPoint);
	typedef JOB_PARALLEL_FOR(EntryPointParallelFor, index);

	enum JobPriority {
		PRIORITY_LOW,
		PRIORITY_NORMAL,
		PRIORITY_HIGH,
		PRIORITY_MAX_ENUM
	};

	struct JobDecl {
		EntryPoint *entry_point;
		uptr param;
		JobPriority priority;

		JobDecl()
			: entry_point(nullptr)
			, param(0)
			, priority(PRIORITY_NORMAL)
		{
		}

		JobDecl(EntryPoint *entry_point, void *param, JobPriority priority = PRIORITY_NORMAL)
			: entry_point(entry_point)
			, param((uptr)param)
			, priority(priority)
		{
		}
	};
	
	struct JobCounter;

	static constexpr u32 MAX_JOBS_IN_QUEUE = 4096;
	static constexpr u32 MAX_CONCURRENT_FIBERS = 128;

	void init();
	void shutdown();

	JobCounter *alloc_counter(u32 initial_count = 0);
	void free_counter(JobCounter *counter);

	void yield_on_counter(JobCounter *counter, u32 value = 0);
	void yield_on_counter_and_free(JobCounter *counter, u32 value = 0);

	void kick_job(
		const JobDecl &job,
		JobCounter **counter
	);

	void kick_job_batch(
		const JobDecl *jobs, u32 count,
		JobCounter **counter
	);

	void parallel_for(
		u32 count,
		EntryPointParallelFor *fn,
		JobPriority priority = PRIORITY_NORMAL,
		u32 batch_size = 64
	);

	bool is_spin_mode_enabled();
	void set_spin_mode(bool enabled);

	u32 get_current_worker_id();
	bool is_main_thread();

	struct SpinScope {
		SpinScope()
		{
			original_value = is_spin_mode_enabled();
			set_spin_mode(true);
		}

		~SpinScope()
		{
			set_spin_mode(original_value);
		}

		bool original_value;
	};
}

#define JOB_SPIN_MODE_ALLOWED 1

#if JOB_SPIN_MODE_ALLOWED
#  define JOB_SPIN_SCOPE() ::job::SpinScope MCONCAT_EXP(job_spin_scope_, __LINE__)
#else
#  define JOB_SPIN_SCOPE()
#endif
