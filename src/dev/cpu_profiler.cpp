#include "cpu_profiler.h"

#include "platform/platform.h"
#include "job/job.h"

using namespace dev;

CpuProfiler *singleton = nullptr;

CpuProfiler::CpuProfiler()
{
	singleton = this;
}

CpuProfiler::~CpuProfiler()
{
	singleton = nullptr;
}

CpuProfiler *CpuProfiler::get_singleton()
{
	return singleton;
}

void CpuProfiler::frame(const char *name, u32 worker_count)
{
	std::lock_guard<std::mutex> lock(mutex);

	per_worker_samples.clear();
	per_worker_samples.resize(worker_count + 1);

	for (auto &v : per_worker_samples)
		v.resize(INITAL_SAMPLE_STORAGE_SIZE);
}

void CpuProfiler::store_sample(const CpuProfileSample &sample)
{
	per_worker_samples[job::get_current_worker_id()].push_back(sample);
}

void CpuProfiler::dump_to_json(const char *filename)
{
}

u64 CpuProfiler::get_timestamp() const
{
	return platform::get_performance_counter();
}
