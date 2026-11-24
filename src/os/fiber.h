#ifndef OS_FIBER_H
#define OS_FIBER_H

#include "core/types.h"

#if defined(_WIN32)

#define OS_FIBER_BACKEND_WIN32          1
#define OS_FIBER_BACKEND_ASM_X64        0

#elif defined(__x86_64__) && defined(__linux__) && !defined(OS_FIBER_FORCE_UCONTEXT)

#define OS_FIBER_BACKEND_WIN32          0
#define OS_FIBER_BACKEND_ASM_X64        1

#else

#define OS_FIBER_BACKEND_WIN32   0
#define OS_FIBER_BACKEND_ASM_X64 0
#include <ucontext.h>

#endif

#define OS_FIBER_STACK_SIZE (1024ull * 1024ull)

namespace os {

typedef void FiberEntryFn(void *param);

class Fiber {
public:
	Fiber(u64 stack_size, FiberEntryFn *entry, void *param);
	~Fiber();

	Fiber(const Fiber &) = delete;
	Fiber &operator=(const Fiber &) = delete;

	[[nodiscard]] bool IsValid() const;

	static void Switch(Fiber *from, Fiber *to);

	// ONLY TO BE USED BY THE THREAD THAT CONVERTED ITSELF
	static Fiber *ConvertThread();
	static bool RevertThread();

private:
	Fiber();

#if OS_FIBER_BACKEND_WIN32
	static void __stdcall Win32Trampoline(void *param);
	void *handle;
#elif OS_FIBER_BACKEND_ASM_X64
	void *sp;
#else
	static void UContextTrampoline(unsigned lo, unsigned hi);
	ucontext_t uctx;
#endif

	// nullptr for fibers that represent a plain thread.
	void *stack_mapping;
	u64 stack_mapping_size;

	FiberEntryFn *entry;
	void *param;
};

}

#endif // OS_FIBER_H
