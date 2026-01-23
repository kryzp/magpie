#pragma once

#include "core/types.h"

namespace ent
{
	struct Coroutine {
		u64 state = 0;
		float delay = 0.f;
	};
}

#define COROUTINE_BEGIN(coroutine_) switch ((coroutine_)->state) { case 0:

#define COROUTINE_YIELD(coroutine_)			\
	do {									\
		(coroutine_)->state = __LINE__;		\
		return;								\
		case __LINE__:;						\
	} while (0)

#define COROUTINE_DELAY(coroutine_, time_, dt_)			\
	do {												\
		(coroutine_)->state = __LINE__;					\
		(coroutine_)->delay = (time_);					\
		return;											\
		case __LINE__:;									\
			if ((coroutine_)->delay > 0.f) {			\
				(coroutine_)->delay -= (dt_);			\
				return;									\
			}											\
	} while (0)

#define COROUTINE_END(coroutine_) } (coroutine_)->state = ((u64)(-1))

#if 0

COROUTINE_BEGIN(&my_coroutine);
{
	printf("Hello,\n");
	COROUTINE_DELAY(&my_coroutine, 2.5f, delta_time); // Wait for 2.5 seconds to pass.
	printf("World!");
	COROUTINE_YIELD(&my_coroutine); // Resume here next time the function is called.
	printf("Asdf.\n");
}
COROUTINE_END(&my_coroutine);

#endif
