#ifndef OS_SPIN_LOCK_H
#define OS_SPIN_LOCK_H

#include "core/types.h"

#include <atomic>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define OS_SPIN_PAUSE() _mm_pause()
#elif defined(__aarch64__)
#define OS_SPIN_PAUSE() __asm__ volatile("yield" ::: "memory")
#else
#define OS_SPIN_PAUSE() \
	do {                      \
	} while (0)
#endif

class SpinLock {
public:
	SpinLock()
		: locked(0)
	{
	}

	SpinLock(const SpinLock &) = delete;
	SpinLock &operator=(const SpinLock &) = delete;

	void Acquire()
	{
		for (;;) {
			if (locked.exchange(1, std::memory_order_acquire) == 0)
				break;

			while (locked.load(std::memory_order_relaxed) == 1)
				OS_SPIN_PAUSE();
		}
	}

	void Release()
	{
		locked.store(0, std::memory_order_release);
	}

private:
	std::atomic<int> locked;
};

class SpinLockScope {
public:
	explicit SpinLockScope(SpinLock &lock_)
		: lock(lock_)
	{
		lock.Acquire();
	}

	~SpinLockScope()
	{
		lock.Release();
	}

	SpinLockScope(const SpinLockScope &) = delete;
	SpinLockScope &operator=(const SpinLockScope &) = delete;

private:
	SpinLock &lock;
};

#endif // OS_SPIN_LOCK_H
