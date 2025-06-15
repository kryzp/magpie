#pragma once

#include "core/common.h"

namespace mgp
{
	class PlatformCore;

	class Timer
	{
	public:
		Timer(PlatformCore *platform);

		void start();
		void stop();
		
		void pause();
		void resume();

		double reset();

		double getElapsedSeconds() const;

		bool isStarted() const;
		bool isPaused() const;

	private:
		PlatformCore *m_platform;

		uint64_t m_startTicks;
		bool m_started;

		uint64_t m_pausedTicks;
		bool m_paused;
	};
}
