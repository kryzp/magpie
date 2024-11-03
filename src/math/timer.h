#ifndef TIMER_H_
#define TIMER_H_

#include "../common.h"

namespace llt
{
	class Timer
	{
	public:
		Timer();

		void start();
		void stop();
		void pause();
		void resume();
		double reset();

		double milliseconds() const;
		double seconds() const;

		bool started() const;
		bool paused() const;

	private:
		uint64_t m_startTicks;
		bool m_started;

		uint64_t m_pausedTicks;
		bool m_paused;
	};
}

#endif // TIMER_H_
