
struct entity_coroutine {
	u64 state;
	u64 delay;
};

#define COROUTINE_BEGIN(coroutine) switch ((coroutine)->state) { case 0:

#define COROUTINE_YIELD(coroutine)		\
	do {					\
		(coroutine)->state = __LINE__;	\
		return;				\
		case __LINE__:;			\
	} while (0)

#define COROUTINE_DELAY(coroutine, ticks)		\
	do {						\
		(coroutine)->state = __LINE__;		\
		(coroutine)->delay = (ticks);		\
		return;					\
		case __LINE__:;				\
			if ((coroutine)->delay > 0) {	\
				(coroutine)->delay--;	\
				return;			\
			}				\
	} while (0)

#define COROUTINE_END(coroutine) } (coroutine)->state = ((u64)(-1))

#if 0

COROUTINE_BEGIN(&my_coroutine);
{
	printf("Hello,\n");
	COROUTINE_DELAY(&my_coroutine, 50); // Wait for 50 ticks to pass.
	printf("World!");
	COROUTINE_YIELD(&my_coroutine); // Resume here next time the function is called.
	printf("Asdf.\n");
}
COROUTINE_END(&my_coroutine);

#endif
