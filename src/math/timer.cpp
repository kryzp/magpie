#include "timer.h"
#include "../system_backend.h"

using namespace llt;

Timer::Timer()
	: m_startTicks(0)
	, m_started(g_systemBackend->getTicks())
	, m_pausedTicks(0)
	, m_paused(false)
{
}

void Timer::start()
{
	m_started = true;
	m_paused = false;
	m_startTicks = g_systemBackend->getTicks();
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
	m_pausedTicks = g_systemBackend->getTicks() - m_startTicks;
	m_startTicks = 0;
}

void Timer::resume()
{
	if (!m_started || !m_paused) {
		return;
	}

	m_paused = false;
	m_startTicks = g_systemBackend->getTicks() - m_pausedTicks;
	m_pausedTicks = 0;
}

double Timer::reset()
{
	uint64_t mil = getMilliseconds();

	if (m_started) {
		start();
	}

	return (double)mil;
}

double Timer::getMilliseconds() const
{
	if (m_started)
	{
		if (m_paused) {
			return m_pausedTicks;
		}

		return g_systemBackend->getTicks() - m_startTicks;
	}

	return 0;
}

double Timer::getSeconds() const
{
	return getMilliseconds() / 1000.0;
}

bool Timer::started() const
{
	return m_started;
}

bool Timer::isPaused() const
{
	return m_paused;
}
