#include "profiler.h"

#include "vulkan/core.h"

// cheers to the vulkan engine guide for helping me with this

llt::Profiler *llt::g_profiler = nullptr;

using namespace llt;

Profiler::Profiler(int perFramePoolSizes)
	: m_period(0.0f)
	, m_queryFrames()
	, m_timings()
{
	m_period = g_vkCore->m_physicalData.properties.properties.limits.timestampPeriod;

	VkQueryPoolCreateInfo queryPoolInfo = {};
	queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	queryPoolInfo.queryCount = perFramePoolSizes;

	for (auto &frame : m_queryFrames)
	{
		vkCreateQueryPool(
			g_vkCore->m_device,
			&queryPoolInfo,
			nullptr,
			&frame.queryPool
		);

		frame.last = 0;
	}
}

Profiler::~Profiler()
{
	for (auto &frame : m_queryFrames)
		vkDestroyQueryPool(g_vkCore->m_device, frame.queryPool, nullptr);
}

void Profiler::getQuerys(CommandBuffer &cmd)
{
	int currentFrame = g_vkCore->getCurrentFrameIdx();

	QueryFrameState &state = m_queryFrames[currentFrame];

	cmd.resetQueryPool(m_queryFrames[currentFrame].queryPool, 0, m_queryFrames[currentFrame].last);

	m_queryFrames[currentFrame].last = 0;
	m_queryFrames[currentFrame].samples.clear();

	Vector<uint64_t> queryState;
	queryState.resize(state.last);

	if (state.last != 0)
	{
		vkGetQueryPoolResults(
			g_vkCore->m_device,
			state.queryPool,
			0,
			state.last,
			queryState.size() * sizeof(uint64_t),
			queryState.data(),
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
		);
	}

	for (auto &timer : state.samples)
	{
		uint64_t begin = queryState[timer.start];
		uint64_t end = queryState[timer.end];

		uint64_t range = end - begin;

		double time = (m_period * (double)range) / 1000000.0;

		m_timings.insert(timer.name, time);
	}
}

void Profiler::storeSample(const ScopeTimer::Sample &sample)
{
	m_queryFrames[g_vkCore->getCurrentFrameIdx()].samples.pushBack(sample);
}

VkQueryPool Profiler::getTimerPool() const
{
	return m_queryFrames[g_vkCore->getCurrentFrameIdx()].queryPool;
}

uint64_t Profiler::getTimestampID()
{
	return m_queryFrames[g_vkCore->getCurrentFrameIdx()].last++;
}

ScopeTimer::ScopeTimer(const char *name, CommandBuffer &cmd)
	: m_sample()
	, m_cmd(cmd)
{
	m_sample.name = name;
	m_sample.start = g_profiler->getTimestampID();
	m_sample.end = 0;

	VkQueryPool pool = g_profiler->getTimerPool();

	m_cmd.writeTimestamp(
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		pool,
		m_sample.start
	);
}

ScopeTimer::~ScopeTimer()
{
	m_sample.end = g_profiler->getTimestampID();

	VkQueryPool pool = g_profiler->getTimerPool();

	m_cmd.writeTimestamp(
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		pool,
		m_sample.end
	);

	g_profiler->storeSample(m_sample);
}
