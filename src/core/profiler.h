#ifndef PROFILER_H_
#define PROFILER_H_

#include "third_party/volk.h"

#include "common.h"

#include "vulkan/command_buffer.h"

#include "container/string.h"
#include "container/array.h"
#include "container/vector.h"
#include "container/hash_map.h"

namespace llt
{
	class Profiler;

	class ScopeTimer
	{
	public:
		struct Sample
		{
			String name;
			uint64_t start;
			uint64_t end;
		};

		ScopeTimer(const char *name, CommandBuffer &cmd);
		~ScopeTimer();

	private:
		Sample m_sample;
		CommandBuffer &m_cmd;
	};

	class Profiler
	{
	public:
		Profiler(int perFramePoolSizes);
		~Profiler();

		void getQuerys(CommandBuffer &cmd);

		void storeSample(const ScopeTimer::Sample &sample);

		VkQueryPool getTimerPool() const;

		uint64_t getTimestampID();

		HashMap<String, double> m_timings;

	private:
		float m_period;

		struct QueryFrameState
		{
			Vector<ScopeTimer::Sample> samples;
			VkQueryPool queryPool;
			uint64_t last;
		};

		Array<QueryFrameState, mgc::FRAMES_IN_FLIGHT> m_queryFrames;
	};

	extern Profiler *g_profiler;
}

#endif // PROFILER_H_
