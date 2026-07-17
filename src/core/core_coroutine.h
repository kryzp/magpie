#ifndef CORE_COROUTINE_H
#define CORE_COROUTINE_H

typedef struct Coroutine Coroutine;
struct Coroutine
{
	u64 state;
	f32 delay;
};

#define CoroutineBegin(cor_) switch ((cor_)->state) { case 0:

#define CoroutineYield(cor_)					\
	do											\
	{											\
		(cor_)->state = __LINE__;				\
		return;									\
		case __LINE__:;							\
	}											\
	while (0)

#define CoroutineDelay(cor_, time_)				\
	do											\
	{											\
		(cor_)->state = __LINE__;				\
		(cor_)->delay = (time_);				\
		return;									\
		case __LINE__:;							\
			if ((cor_)->delay > 0.f)			\
			{									\
				(cor_)->delay -= (/* TODO */);	\
				return;							\
			}									\
	}											\
	while (0)

#define CoroutineEnd(cor_) } (cor_)->state = ((u64)(-1))

#if 0

Coroutine my_coroutine = {0};

// ...

CoroutineBegin(&my_coroutine);
{
	DebugLog("1");
	CoroutineDelay(&my_coroutine, 2.5f); // wait for 2.5 seconds
	DebugLog("2");
	CoroutineYield(&my_coroutine); // resume next time the function is called
	DebugLog("3");
}
CoroutineEnd(&my_coroutine);

#endif

#endif // CORE_COROUTINE_H
