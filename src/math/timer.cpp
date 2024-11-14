#include "timer.h"
#include "../platform.h"

using namespace llt;

Timer::Timer()
	: m_startTicks(0)
	, m_started(g_platform->getPerformanceCounter())
	, m_pausedTicks(0)
	, m_paused(false)
{
}

void Timer::start()
{
	m_started = true;
	m_paused = false;

	m_startTicks = g_platform->getPerformanceCounter();
	m_pausedTicks = 0;
}

void Timer::stop()
{
	m_started = false;
	m_paused = false;

	m_startTicks = 0;
	m_pausedTicks = 0;
}

void Timer::pause()
{
	if (!m_started || m_paused) {
		return;
	}

	m_paused = true;
	m_pausedTicks = g_platform->getPerformanceCounter() - m_startTicks;

	m_startTicks = 0;
}

void Timer::resume()
{
	if (!m_started || !m_paused) {
		return;
	}

	m_paused = false;
	m_startTicks = g_platform->getPerformanceCounter() - m_pausedTicks;

	m_pausedTicks = 0;
}

double Timer::reset()
{
	double sec = getElapsedSeconds();

	if (m_started) {
		start();
	}

	return sec;
}

double Timer::getElapsedSeconds() const
{
	if (m_started)
	{
		if (m_paused) {
			return m_pausedTicks;
		}

		return static_cast<double>(g_platform->getPerformanceCounter() - m_startTicks) / static_cast<double>(g_platform->getPerformanceFrequency());
	}

	return 0.0;
}

bool Timer::isStarted() const
{
	return m_started;
}

bool Timer::isPaused() const
{
	return m_paused;
}
