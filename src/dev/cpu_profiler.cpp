#include "cpu_profiler.h"

#include "platform/platform.h"

using namespace dev;

CpuProfiler::CpuProfiler()
	: events()
	, mutex()
{
}

CpuProfiler::~CpuProfiler()
{
}

CpuProfiler *CpuProfiler::get_singleton()
{
	static CpuProfiler instance;
	return &instance;
}

void CpuProfiler::add_event(const ProfileEvent &event)
{
	std::lock_guard<std::mutex> lock(mutex);
	events.push_back(event);
}

void CpuProfiler::dump_to_file(const char *filepath)
{
	std::lock_guard<std::mutex> lock(mutex);

	FILE *file = fopen(filepath, "w");

	if (!file)
		return;

	fprintf(file, "{\"traceEvents\":[\n");

	for (int i = 0; i < events.size(); i++) {
		const auto& e = events[i];

		fprintf(
			file, 
			"{"
			"\"name\":\"%s\","
			"\"ph\":\"X\","
			"\"pid\":%u,"
			"\"tid\":%u,"
			"\"ts\":%llu,"
			"\"dur\":%llu"
			"}",
			e.name, 1, e.fiber_id, e.start_us, e.get_duration_us()
		);

		if (i < events.size() - 1)
			fprintf(file, ",\n");
	}
	
	fprintf(file, "\n]}\n");

	fclose(file);
}

u64 CpuProfiler::get_timestamp_us() const
{
	return platform::get_performance_counter();
}
