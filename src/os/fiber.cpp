#include "os/fiber.h"
#include "os/platform.h"
#include "core/log.h"

#if OS_FIBER_BACKEND_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <cstdlib>

/*
 * THIS WAS SUCH A PAIN IN THE ASS TO GET WORKING.
 *
 * OK SO BASICALLY.
 *
 * UNIX DOESNT HAVE THE SAME INFRASTRUCTURE FOR FIBERS THAT
 * WINDOWS HAS, AT LEAST TO MY KNOWLEDGE.
 *
 * THEREFORE, WE GOTTA ESSENTIALLY IMPLEMENT OUR OWN
 * VERSION OF CONTEXT SWITCHING, BY MANUALLY
 * WRITING TO THE REGISTERS AND STUFF.
 *
 * I CAN'T BELIEVE IT WORKS.
 */

#if OS_FIBER_BACKEND_ASM_X64

/*
 * Only the callee-saved registers (plus the SSE/x87 control
 * words, which the ABI also makes callee-saved) have to be preserved, since
 * the switch looks like an ordinary function call to the compiler:
 *
 * Saved frame, low to high address:
 *    [ mxcsr | fpucw ][ r15 ][ r14 ][ r13 ][ r12 ][ rbx ][ rbp ][ return address ]
 */

extern "C" void PlatformFiberAsmSwitch(void **save_sp, void *load_sp);
extern "C" void PlatformFiberAsmTrampoline(void);

__asm__(
	".pushsection .text\n"
	".p2align 4\n"
	".globl PlatformFiberAsmSwitch\n"
	".hidden PlatformFiberAsmSwitch\n"
	".type PlatformFiberAsmSwitch, @function\n"
	"PlatformFiberAsmSwitch:\n"
	"	pushq %rbp\n"
	"	pushq %rbx\n"
	"	pushq %r12\n"
	"	pushq %r13\n"
	"	pushq %r14\n"
	"	pushq %r15\n"
	"	subq $8, %rsp\n"
	"	stmxcsr (%rsp)\n"
	"	fnstcw 4(%rsp)\n"
	"	movq %rsp, (%rdi)\n"
	"	movq %rsi, %rsp\n"
	"	ldmxcsr (%rsp)\n"
	"	fldcw 4(%rsp)\n"
	"	addq $8, %rsp\n"
	"	popq %r15\n"
	"	popq %r14\n"
	"	popq %r13\n"
	"	popq %r12\n"
	"	popq %rbx\n"
	"	popq %rbp\n"
	"	ret\n"
	".size PlatformFiberAsmSwitch, .-PlatformFiberAsmSwitch\n"
	"\n"
	".p2align 4\n"
	".globl PlatformFiberAsmTrampoline\n"
	".hidden PlatformFiberAsmTrampoline\n"
	".type PlatformFiberAsmTrampoline, @function\n"
	"PlatformFiberAsmTrampoline:\n"
	"	.cfi_startproc\n"
	"	.cfi_undefined rip\n"
	"	movq %r13, %rdi\n"
	"	callq *%r12\n"
	"	ud2\n"
	"	.cfi_endproc\n"
	".size PlatformFiberAsmTrampoline, .-PlatformFiberAsmTrampoline\n"
	".popsection\n"
);

#endif

static thread_local os::Fiber *thread_fiber = nullptr;

#if OS_FIBER_BACKEND_WIN32

void __stdcall os::Fiber::Win32Trampoline(void *param)
{
	Fiber *fiber = (Fiber *)param;
	fiber->entry(fiber->param);
	DebugPrintAssert(false, "The fuck?");
}

#elif !OS_FIBER_BACKEND_ASM_X64

// makecontext only passes ints, so the pointer is split in two.
void os::Fiber::UContextTrampoline(unsigned lo, unsigned hi)
{
	Fiber *fiber = (Fiber *)(((uptr)hi << 32) | (uptr)lo);
	fiber->entry(fiber->param);
	DebugPrintAssert(false, "The fuck?");
}

#endif

os::Fiber::Fiber()
	: stack_mapping(nullptr)
	, stack_mapping_size(0)
	, entry(nullptr)
	, param(nullptr)
{
#if OS_FIBER_BACKEND_WIN32
	handle = nullptr;
#elif OS_FIBER_BACKEND_ASM_X64
	sp = nullptr;
#endif
}

os::Fiber::Fiber(u64 stack_size, FiberEntryFn *entry_, void *param_)
	: Fiber()
{
	entry = entry_;
	param = param_;

	if (stack_size == 0)
		stack_size = OS_FIBER_STACK_SIZE;

#if OS_FIBER_BACKEND_WIN32

	handle = CreateFiber(stack_size, (LPFIBER_START_ROUTINE)Win32Trampoline, this);

#else

	u64 page = GetPageSize();

	stack_size = MemAlignUp(stack_size, page);

	// leave one extra page at the bottom as a guard so that overflowing will fault instead
	// of silently stomping on whatever memory happens to be below which would suck dick.
	u64 mapping_size = stack_size + page;

	void *mapping = mmap(
		nullptr, mapping_size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK, -1, 0
	);

	if (mapping == MAP_FAILED)
		return;

	mprotect(mapping, page, PROT_NONE);

	stack_mapping = mapping;
	stack_mapping_size = mapping_size;

	u8 *stack_top = (u8 *)mapping + mapping_size;

#if OS_FIBER_BACKEND_ASM_X64

	// stack_top is page aligned, so 16 byte aligned.
	u64 *frame = (u64 *)(stack_top - 64);

	// default floating point control words:
	const u64 MXCSR_CW = 0x1F80;
	const u64 X87_FPU_CW = 0x037F;

	frame[0] = (MXCSR_CW << 0) | (X87_FPU_CW << 32);
	frame[1] = 0;                // r15
	frame[2] = 0;                // r14
	frame[3] = (u64)(uptr)param; // r13
	frame[4] = (u64)(uptr)entry; // r12
	frame[5] = 0;                // rbx
	frame[6] = 0;                // rbp
	frame[7] = (u64)(uptr)PlatformFiberAsmTrampoline;

	// after the final return in the switch, rsp == stack_top so the trampoline's callq hands Entry the alignment the ABI wants
	sp = frame;

#else

	getcontext(&uctx);

	uctx.uc_stack.ss_sp = (u8 *)mapping + page;
	uctx.uc_stack.ss_size = stack_size;
	uctx.uc_link = nullptr;

	uptr p = (uptr)this;
	makecontext(&uctx, (void (*)(void))UContextTrampoline, 2, (unsigned)(p & 0xFFFFFFFFu), (unsigned)(p >> 32));

#endif

#endif
}

os::Fiber::~Fiber()
{
#if OS_FIBER_BACKEND_WIN32
	if (handle && entry)
		DeleteFiber(handle);
#else
	if (stack_mapping)
		munmap(stack_mapping, stack_mapping_size);
#endif
}

bool os::Fiber::IsValid() const
{
#if OS_FIBER_BACKEND_WIN32
	return handle != nullptr;
#else
	return stack_mapping != nullptr;
#endif
}

void os::Fiber::Switch(Fiber *from, Fiber *to)
{
#if OS_FIBER_BACKEND_WIN32
	(void)from;
	SwitchToFiber(to->handle);
#elif OS_FIBER_BACKEND_ASM_X64
	PlatformFiberAsmSwitch(&from->sp, to->sp);
#else
	swapcontext(&from->uctx, &to->uctx);
#endif
}

os::Fiber *os::Fiber::ConvertThread()
{
	if (thread_fiber)
		return thread_fiber;

	thread_fiber = new Fiber();

#if OS_FIBER_BACKEND_WIN32
	thread_fiber->handle = ConvertThreadToFiber(nullptr);
#endif

	return thread_fiber;
}

bool os::Fiber::RevertThread()
{
	if (!thread_fiber)
		return false;

#if OS_FIBER_BACKEND_WIN32
	ConvertFiberToThread();
#endif

	delete thread_fiber;
	thread_fiber = nullptr;

	return true;
}
