#pragma once

#include "core/common.h"

namespace mgp
{
	class Platform;

	class Timer
	{
	public:
		Timer(Platform *platform);

		void start();
		void stop();
		
		void pause();
		void resume();

		double reset();

		double getElapsedSeconds() const;

		bool isStarted() const;
		bool isPaused() const;

	private:
		uint64_t m_startTicks;
		bool m_started;

		uint64_t m_pausedTicks;
		bool m_paused;

		Platform *m_platform;
	};
}
