#ifndef CORE_COROUTINE_H
#define CORE_COROUTINE_H

#include "core/types.h"

struct Coroutine {
	int state = 0;
	float delay = 0.f;
};

#define CoroutineBegin(cor_) \
	switch ((cor_)->state) { \
		case 0:

#define CoroutineYield(cor_)      \
	do {                          \
		(cor_)->state = __LINE__; \
		return;                   \
		case __LINE__:;           \
	} while (0)

#define CoroutineDelay(cor_, time_, dt_) \
	do {                                 \
		(cor_)->state = __LINE__;        \
		(cor_)->delay = (time_);         \
		return;                          \
		case __LINE__:;                  \
			if ((cor_)->delay > 0.f) {   \
				(cor_)->delay -= (dt_);  \
				return;                  \
			}                            \
	} while (0)

#define CoroutineEnd(cor_) \
	}                      \
	(cor_)->state = (-1)

#if 0

Coroutine my_coroutine;

// ...

CoroutineBegin(&my_coroutine);
{
	DebugPrintI("1");
	CoroutineDelay(&my_coroutine, 2.5f, dt); // wait for 2.5 seconds
	DebugPrintI("2");
	CoroutineYield(&my_coroutine); // resume next time the function is called
	DebugPrintI("3");
}
CoroutineEnd(&my_coroutine);

#endif

#endif // CORE_COROUTINE_H
