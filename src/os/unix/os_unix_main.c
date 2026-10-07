
/*
 * magpie unix platform layer
 * 
 * yes this file is a complete mess.
 *
 * but that's okay!
 *
 * as long as that mess is confined to just one file,
 * then I consider that a damn good success for trying
 * to support multiple platforms.
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#if defined(__x86_64__)
# include <immintrin.h>
# define OS_UNIX_SPIN_PAUSE() _mm_pause()
#elif defined(__aarch64__)
# define OS_UNIX_SPIN_PAUSE() __asm__ volatile("yield" ::: "memory")
#else
# define OS_UNIX_SPIN_PAUSE() do { } while (0)
#endif

#include "core/core_inc.h"
#include "os/os_inc.h"
#include "core/core_inc.c"
#include "os/os_inc.c"

#include "io/io_inc.h"
#include "io/io_inc.c"

#include "chrono/chrono_inc.h"
#include "chrono/chrono_inc.c"

#if defined(__APPLE__)
# define OS_UNIX_CODE_EXT "dylib"
#else
# define OS_UNIX_CODE_EXT "so"
#endif

#define OS_UNIX_CODE_PATH      "build/program." OS_UNIX_CODE_EXT
#define OS_UNIX_CODE_HOT_FMT   "build/hot_reload_%u." OS_UNIX_CODE_EXT

#define OS_UNIX_PATH_MAX       4096

#ifndef OS_UNIX_MAX_PENDING_EVENTS
# define OS_UNIX_MAX_PENDING_EVENTS 256
#endif

#ifndef OS_UNIX_JOB_FIBER_STACK_SIZE
# define OS_UNIX_JOB_FIBER_STACK_SIZE (1024ull * 1024ull)
#endif

/*
 * ok so
 * pinning a worker X to a core N is off by default on linux, unlike windows.
 * this means that the affinity mask is inherited by every thread that a pinned thread creates,
 * so other threads which might get spawned from inside a job would all end up stuck
 * on that single core, so we gotta disable this.
 * TODO: do we actually spawn threads from inside jobs?
 */

// uncomment to enable
//#define OS_UNIX_PIN_WORKER_THREADS

#if defined(__x86_64__) && defined(__linux__) && !defined(OS_UNIX_FIBER_FORCE_UCONTEXT)
# define OS_UNIX_FIBER_BACKEND_ASM_X64 1
#else
# define OS_UNIX_FIBER_BACKEND_ASM_X64 0
# include <ucontext.h>
#endif

#define OS_UNIX_NOINLINE __attribute__((noinline))

typedef void OS_UNIX_FiberEntryFn(void *param);

typedef struct OS_UNIX_Fiber OS_UNIX_Fiber;
struct OS_UNIX_Fiber
{
#if OS_UNIX_FIBER_BACKEND_ASM_X64
	void *sp;
#else
	ucontext_t uctx;
#endif

	// NULL for fibers that represent a plain thraed.
	void *stack_mapping;
	u64 stack_mapping_size;

	OS_UNIX_FiberEntryFn *Entry;
	void *param;
};

typedef struct OS_UNIX_JobContext OS_UNIX_JobContext;
struct OS_UNIX_JobContext
{
	u32 worker_id;
	i32 fiber_id;
};

#define OS_UNIX_LOG_MAX_CHANNELS        32
#define OS_UNIX_LOG_LINE_BUFFER_SIZE    4096
#define OS_UNIX_LOG_CHANNEL_COL_ALIGN   10

#define OS_UNIX_LOG_ANSI_RESET    "\x1b[0m"
#define OS_UNIX_LOG_ANSI_DIM      "\x1b[2m"

typedef struct OS_UNIX_LogChannelEntry OS_UNIX_LogChannelEntry;
struct OS_UNIX_LogChannelEntry
{
	String8 name;
	b32 enabled;
	LOG_Channel parent;
};

typedef struct OS_UNIX_Logger OS_UNIX_Logger;
struct OS_UNIX_Logger
{
	Arena *arena;
	
	CH_Timer timer;
	
	OS_Handle mutex;
	OS_Handle file_stream;

	i32 channel_spinlock;
	u32 channel_count;
	OS_UNIX_LogChannelEntry channels[OS_UNIX_LOG_MAX_CHANNELS];

	LOG_Channel null_channel;
	LOG_Channel log_channel;

	// track last message and overwrite in-place if repeated.
	char               dedup_body[OS_UNIX_LOG_LINE_BUFFER_SIZE];
	LOG_Channel        dedup_channel;
	LOG_Level          dedup_level;
	u32                dedup_count;
	b32                dedup_active;
	OS_UNIX_JobContext dedup_job_context;
};

/*
 * - "Parallelizing the Naughty Dog Engine Using Fibers" - Christian Gyrling
 * - "Parallelizing the Physics Solver" - Dennis Gustafsson
 */

#define OS_UNIX_JOB_MAX_JOBS_PER_QUEUE                 512
#define OS_UNIX_JOB_MAX_CONCURRENT_FIBERS              128
#define OS_UNIX_JOB_FIBER_SPIN_STARVE_ALERT_TIME_MS    3000
#define OS_UNIX_JOB_MAX_WORKERS                        32
#define OS_UNIX_JOB_COUNTER_MAX_WAITING                128
#define OS_UNIX_JOB_FIBER_SCRATCH_SIZE                 Megabytes(8)
#define OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE            2

// due to how ive written the queue indicies it needs to be this way :/
_Static_assert((OS_UNIX_JOB_MAX_JOBS_PER_QUEUE & (OS_UNIX_JOB_MAX_JOBS_PER_QUEUE - 1)) == 0, "Queue size must be a power of two.");

typedef struct OS_UNIX_JobFiber OS_UNIX_JobFiber;

typedef struct OS_UNIX_JobCounter OS_UNIX_JobCounter;
struct OS_UNIX_JobCounter
{
	i32 atomic_count;
	i32 atomic_spinlock;

	u32 waiting_count;
	OS_UNIX_JobFiber *waiting[OS_UNIX_JOB_COUNTER_MAX_WAITING];
};

struct OS_UNIX_JobFiber
{
	OS_UNIX_JobFiber *next_free;
	
	u32 id;
	OS_UNIX_Fiber *context;
	
	J_EntryPointFn *EntryPoint;
	void *param;
	J_Priority priority;
	J_Flags flags;

	OS_UNIX_JobCounter *counter;
	b32 finished;
	
	Arena scratch_arenas[OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE];
};

typedef struct OS_UNIX_JobFiberMutex OS_UNIX_JobFiberMutex;
struct OS_UNIX_JobFiberMutex
{
	i32 atomic_mutex_state;
	i32 atomic_spinlock;
	
	u32 waiting_count;
	OS_UNIX_JobFiber *waiting[OS_UNIX_JOB_COUNTER_MAX_WAITING];
};

typedef struct OS_UNIX_JobFiberCondVar OS_UNIX_JobFiberCondVar;
struct OS_UNIX_JobFiberCondVar
{
	i32 atomic_spinlock;

	u32 waiting_count;
	OS_UNIX_JobFiber *waiting[OS_UNIX_JOB_COUNTER_MAX_WAITING];
};

typedef struct OS_UNIX_JobRequest OS_UNIX_JobRequest;
struct OS_UNIX_JobRequest
{
	J_EntryPointFn *EntryPoint;
	void *param;
	J_Priority priority;
	J_Flags flags;
	OS_UNIX_JobCounter *counter;
};

// NOTE: the 4 counts are i32's but are treated like they are free running u32's
//       because they need to keep working when they wrap, so note to self, do
//       NOT compare using < or >, only compare using subtract and !=.
typedef struct OS_UNIX_JobQueue OS_UNIX_JobQueue;
struct OS_UNIX_JobQueue
{
	i32 atomic_spinlock;
	
	OS_UNIX_JobRequest requests[OS_UNIX_JOB_MAX_JOBS_PER_QUEUE];
	OS_UNIX_JobFiber *waiting[OS_UNIX_JOB_MAX_JOBS_PER_QUEUE];

	i32 atomic_taken_task_count;
	i32 atomic_added_task_count;
	
	i32 atomic_taken_waiting_count;
	i32 atomic_added_waiting_count;
};

typedef struct OS_UNIX_JobScheduler OS_UNIX_JobScheduler;

typedef struct OS_UNIX_JobWorker OS_UNIX_JobWorker;
struct OS_UNIX_JobWorker
{
	u32 id;
	
	OS_Handle thread_handle;

	// this worker's thread / scheduler loop.
	OS_UNIX_Fiber *fiber;

	OS_UNIX_JobFiber *current_fiber;

	// a job fiber that goes to sleep on a spinlock-protected waitlist keeps
	// that spinlock held across the switch, and the scheduler releases it
	// once the fiber's context has actually been properly saved.
	// without this, another worker could also pop the fiber off the waitlist
	// and resume it while it is still in the middle of switching away, which
	// would lead to disaster!!
	i32 *pending_unlock;
};

struct OS_UNIX_JobScheduler
{
	LOG_Channel log_channel;

	i32 atomic_running;
	i32 atomic_spin_mode;

	OS_Handle thread_mutex;
	OS_Handle thread_cond_begin;

	OS_UNIX_JobQueue main_thread_queue;
	OS_UNIX_JobQueue queues[J_Priority_COUNT];
	
	Arena fallback_scratch_ring[OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE];
	
	u32 worker_count;
	OS_UNIX_JobWorker workers[OS_UNIX_JOB_MAX_WORKERS];

	OS_UNIX_JobFiber atomic_fiber_storage[OS_UNIX_JOB_MAX_CONCURRENT_FIBERS];

	i32 fiber_pool_spinlock;
	OS_UNIX_JobFiber *fiber_pool_head;
	
	void (*OnMainThreadIdle)(void *ctx);
	void *main_thread_idle_ctx;	
};

typedef struct OS_UNIX_Object OS_UNIX_Object;
struct OS_UNIX_Object
{
	OS_UNIX_Object *next_free;

	union
	{
		pthread_mutex_t tmutex;
		pthread_cond_t tcondvar;
		OS_UNIX_JobCounter counter;
		OS_UNIX_JobFiberMutex fmutex;
		OS_UNIX_JobFiberCondVar fcondvar;
	};
};

typedef struct OS_UNIX_Code OS_UNIX_Code;
struct OS_UNIX_Code
{
	void *lib;

	u64 last_write_time; // mtime

	// only reload once the mtime has stopped changing.
	u64 pending_write_time;
	u64 pending_since_ms;

	u32 generation;
	char hot_path[OS_UNIX_PATH_MAX];

	OS_EntryInitFn        *Init;
	OS_EntryDestroyFn     *Destroy;
	OS_EntryTickFn        *Tick;
	OS_EntryHotLoadFn     *HotLoad;
	OS_EntryHotUnloadFn   *HotUnload;
};

typedef struct OS_UNIX_State OS_UNIX_State;
struct OS_UNIX_State
{
	Arena arena;
	void *app;

	OS_API api;
	OS_UNIX_Code code;

	SDL_Window *sdl_window;

	u64 page_size;
	u32 num_cores;

	OS_UNIX_JobScheduler sched;

	OS_UNIX_Logger logger;
	LOG_Channel log_channel;
	
	i32 object_spinlock;
	OS_UNIX_Object *free_objects;

	OS_Handle event_mutex;
	u32 pending_event_count;
	SDL_Event *pending_events;

	u32 gamepad_count;
	SDL_Gamepad *gamepads[OS_MAX_GAMEPADS];
};

static OS_UNIX_State unix_st = {0};

static OS_UNIX_Logger *unix_logger = NULL;
static OS_UNIX_JobScheduler *unix_job_sched = NULL;

// Only touched by the thread that converted itself, never by migrating fibers.
static __thread OS_UNIX_Fiber *unix_thread_fiber = NULL;

// FIBERS CAN BE RESUMED BY DIFFERENT WORKER THREADS THAN THE ONES THAT SUSPENDED THEM.
// THIS MEANS THAT THE COMPILER MUST *NEVER*, *EVER*, *EVER*, CACHE THE ADDRESS OF THIS
// ACROSS A FIBER SWITCH.
// THEREFORE, ONLY EVER READ THIS VARIABLE THROUGH OS_UNIX_JOBGETCURRENTWORKER.
static __thread OS_UNIX_JobWorker *unix_job_thread_current_worker_internal_state = NULL;

internal OS_UNIX_NOINLINE OS_UNIX_JobWorker *OS_UNIX_JobGetCurrentWorker(void)
{
	OS_UNIX_JobWorker *worker = unix_job_thread_current_worker_internal_state;

	// wacky asm trick
	// basically the empty asm stops the compiler from treating the call as pure and merging / hoisting it.
	// cool stuff!!
	__asm__ volatile("" : "+r"(worker));

	return worker;
}

internal OS_Handle OS_UNIX_FiberMutexCreate(void);
internal void OS_UNIX_FiberMutexDestroy(OS_Handle handle);
internal void OS_UNIX_FiberMutexLock(OS_Handle handle);
internal void OS_UNIX_FiberMutexUnlock(OS_Handle handle);

internal void OS_UNIX_Log(LOG_Level level, LOG_Channel channel,
						  const char *file, i32 line, const char *fn,
						  const char *fmt, ...);

internal OS_UNIX_JobContext OS_UNIX_JobGetContext(void);

internal void OS_UNIX_QuerySystemInfo(void)
{
	long page = sysconf(_SC_PAGESIZE);
	unix_st.page_size = (page > 0) ? (u64)page : 4096;

	u32 cores = 0;

#if defined(__linux__)
	cpu_set_t set;
	CPU_ZERO(&set);

	if (sched_getaffinity(0, sizeof(set), &set) == 0)
		cores = (u32)CPU_COUNT(&set);
#endif

	if (cores == 0)
	{
		long n = sysconf(_SC_NPROCESSORS_ONLN);
		cores = (n > 0) ? (u32)n : 1;
	}

	unix_st.num_cores = cores;
}

internal u64 OS_UNIX_GetPageSize(void)
{
	if (unix_st.page_size == 0)
		OS_UNIX_QuerySystemInfo();

	return unix_st.page_size;
}

internal u32 OS_UNIX_GetNumCores(void)
{
	if (unix_st.num_cores == 0)
		OS_UNIX_QuerySystemInfo();

	return unix_st.num_cores;
}

// string8 isnt guranteed to be null terminated due to substrings and stuff
// but every libc/sdl call wants a cstring, so copy it into a stack buffer
internal b32 OS_UNIX_PathToCStr(char *dst, u64 dst_size, String8 s)
{
	if (s.len >= dst_size)
	{
		DebugLogE(unix_st.log_channel,
				  "Name too long! String8 string: \"%.*s\" is %llu characters long, which is more than dst_size (%llu).",
				  String8VArg(s),
				  s.len,
				  dst_size);

		return false;
	}

	if (s.len > 0)
		memcpy(dst, s.str, s.len);

	dst[s.len] = '\0';

	return true;
}

// note: one page header is added just below address because VirtualRelease()
//       doesn't give the length.

#define OS_UNIX_VM_ALIGNMENT (64ull * 1024ull)

typedef struct OS_UNIX_VirtualHeader OS_UNIX_VirtualHeader;
struct OS_UNIX_VirtualHeader
{
	void *mapping;
	u64 mapping_size;
};

internal void *OS_UNIX_VirtualReserve(u64 bytes)
{
	u64 page = OS_UNIX_GetPageSize();

	u64 size = MemAlignUp(bytes, page);
	u64 total = size + page + OS_UNIX_VM_ALIGNMENT;

	u8 *mapping = mmap(NULL, total, PROT_NONE,
					   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

	if (mapping == MAP_FAILED)
		return NULL;

	u8 *base = (u8 *)MemAlignUp((u64)(uintptr_t)(mapping + page), OS_UNIX_VM_ALIGNMENT);
	u8 *header = base - page;

	if (mprotect(header, page, PROT_READ | PROT_WRITE) != 0)
	{
		munmap(mapping, total);
		return NULL;
	}

	OS_UNIX_VirtualHeader *h = (OS_UNIX_VirtualHeader *)header;
	h->mapping = mapping;
	h->mapping_size = total;

	return base;
}

internal void OS_UNIX_VirtualRelease(void *address)
{
	if (!address)
		return;

	u64 page = OS_UNIX_GetPageSize();

	OS_UNIX_VirtualHeader *h = (OS_UNIX_VirtualHeader *)((u8 *)address - page);

	munmap(h->mapping, h->mapping_size);
}

internal void OS_UNIX_VirtualCommit(void *address, u64 bytes)
{
	u64 page = OS_UNIX_GetPageSize();

	u64 start = MemAlignDown((u64)(uintptr_t)address, page);
	u64 end = MemAlignUp((u64)(uintptr_t)address + bytes, page);

	if (mprotect((void *)(uintptr_t)start, end - start, PROT_READ | PROT_WRITE) != 0)
		fprintf(stderr, "OS_UNIX_VirtualCommit(%p, %llu) failed: %s\n", address, bytes, strerror(errno));
}

internal void OS_UNIX_VirtualDecommit(void *address, u64 bytes)
{
	u64 page = OS_UNIX_GetPageSize();

	u64 start = MemAlignDown((u64)(uintptr_t)address, page);
	u64 end = MemAlignUp((u64)(uintptr_t)address + bytes, page);

	madvise((void *)(uintptr_t)start, end - start, MADV_DONTNEED);
	mprotect((void *)(uintptr_t)start, end - start, PROT_NONE);
}

internal void *OS_UNIX_HeapAlloc(u64 bytes)
{
	return calloc(1, bytes);
}

internal void OS_UNIX_HeapFree(void *address)
{
	free(address);
}

internal void *OS_UNIX_HeapRealloc(void *address, u64 new_bytes)
{
	return realloc(address, new_bytes);
}

internal void OS_UNIX_SetWindowTitle(String8 title)
{
	char buf[1024];

	u64 len = (title.len < sizeof(buf) - 1) ? title.len : sizeof(buf) - 1;
	memcpy(buf, title.str, len);
	buf[len] = '\0';

	SDL_SetWindowTitle(unix_st.sdl_window, buf);
}

internal void OS_UNIX_GetWindowSize(u32 *w, u32 *h)
{
	i32 width;
	i32 height;
	
	SDL_GetWindowSize(unix_st.sdl_window, &width, &height);

	AssertTrue(width >= 0 && height >= 0);

	if (w) *w = width;
	if (h) *h = height;
}

internal void OS_UNIX_GetWindowSizeInPixels(u32 *pw, u32 *ph)
{
	i32 pixel_width;
	i32 pixel_height;

	SDL_GetWindowSizeInPixels(unix_st.sdl_window, &pixel_width, &pixel_height);

	AssertTrue(pixel_width >= 0 && pixel_height >= 0);

	if (pw) *pw = pixel_width;
	if (ph) *ph = pixel_height;
}

internal void OS_UNIX_SetWindowSize(u32 w, u32 h)
{
	SDL_SetWindowSize(unix_st.sdl_window, w, h);
}

internal void OS_UNIX_SetWindowFullscreen(b32 fullscreen)
{
	SDL_SetWindowFullscreen(unix_st.sdl_window, fullscreen);
}

internal void OS_UNIX_SetWindowBorderless(b32 borderless)
{
	SDL_SetWindowBordered(unix_st.sdl_window, !borderless);
}

internal void OS_UNIX_SetWindowOpacity(f32 opacity)
{
	SDL_SetWindowOpacity(unix_st.sdl_window, opacity);
}

internal void OS_UNIX_SetMousePosition(f32 x, f32 y)
{
	SDL_WarpMouseInWindow(unix_st.sdl_window, x, y);
}

internal void OS_UNIX_SetMouseVisible(b32 visible)
{
	//	ImGui::SetMouseCursor(visible ? ImGuiMouseSource_Mouse : ImGuiMouseCursor_None);

	if (visible)
		SDL_ShowCursor();
	else
		SDL_HideCursor();
}

internal b32 OS_UNIX_IsMouseVisible(void)
{
	return SDL_CursorVisible();
}

internal void OS_UNIX_SetMouseLocked(b32 locked)
{
	SDL_SetWindowRelativeMouseMode(unix_st.sdl_window, locked);
}

internal b32 OS_UNIX_IsMouseLocked(void)
{
	return SDL_GetWindowRelativeMouseMode(unix_st.sdl_window);
}

internal u64 OS_UNIX_GetTicks(void)
{
	return SDL_GetTicks();
}

internal u64 OS_UNIX_GetPerformanceCounter(void)
{
	return SDL_GetPerformanceCounter();
}

internal u64 OS_UNIX_GetPerformanceFrequency(void)
{
	return SDL_GetPerformanceFrequency();
}

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

#if OS_UNIX_FIBER_BACKEND_ASM_X64

/*
 * System V x86_64. Only the callee-saved registers (plus the SSE/x87 control
 * words, which the ABI also makes callee-saved) have to be preserved, since
 * the switch looks like an ordinary function call to the compiler.
 *
 * Saved frame, low to high address:
 *    [ mxcsr | fpucw ][ r15 ][ r14 ][ r13 ][ r12 ][ rbx ][ rbp ][ return address ]
 */

void OS_UNIX_FiberAsmSwitch(void **save_sp, void *load_sp);
void OS_UNIX_FiberAsmTrampoline(void);

__asm__(
		".pushsection .text\n"
		".p2align 4\n"
		".globl OS_UNIX_FiberAsmSwitch\n"
		".hidden OS_UNIX_FiberAsmSwitch\n"
		".type OS_UNIX_FiberAsmSwitch, @function\n"
		"OS_UNIX_FiberAsmSwitch:\n"
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
		".size OS_UNIX_FiberAsmSwitch, .-OS_UNIX_FiberAsmSwitch\n"
		"\n"
		// First thing a brand new fiber runs. r12 = entry, r13 = param
		// (loaded from the initial frame built in OS_UNIX_FiberCreate).
		".p2align 4\n"
		".globl OS_UNIX_FiberAsmTrampoline\n"
		".hidden OS_UNIX_FiberAsmTrampoline\n"
		".type OS_UNIX_FiberAsmTrampoline, @function\n"
		"OS_UNIX_FiberAsmTrampoline:\n"
		"	.cfi_startproc\n"
		"	.cfi_undefined rip\n"
		"	movq %r13, %rdi\n"
		"	callq *%r12\n"
		"	ud2\n"
		"	.cfi_endproc\n"
		".size OS_UNIX_FiberAsmTrampoline, .-OS_UNIX_FiberAsmTrampoline\n"
		".popsection\n"
		);

#else

// makecontext only passes ints, so the pointer is split in two.
internal void OS_UNIX_FiberUContextTrampoline(unsigned int lo, unsigned int hi)
{
	OS_UNIX_Fiber *fiber = (OS_UNIX_Fiber *)(((uintptr_t)hi << 32) | (uintptr_t)lo);

	fiber->Entry(fiber->param);

	DebugLogAssert(unix_st.log_channel, false, "The fuck?");
}

#endif

internal OS_UNIX_Fiber *OS_UNIX_FiberCreate(u64 stack_size, OS_UNIX_FiberEntryFn *Entry, void *param)
{
	u64 page = OS_UNIX_GetPageSize();

	if (stack_size == 0)
		stack_size = OS_UNIX_JOB_FIBER_STACK_SIZE;

	stack_size = MemAlignUp(stack_size, page);

	// One extra page at the bottom as a guard so overflowing faults instead
	// of silently stomping on whatever mapping happens to be below.
	// (...which woudl be be bad.)
	u64 mapping_size = stack_size + page;

	void *mapping = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE,
						 MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK, -1, 0);

	if (mapping == MAP_FAILED)
		return NULL;

	mprotect(mapping, page, PROT_NONE);

	OS_UNIX_Fiber *fiber = calloc(1, sizeof(OS_UNIX_Fiber));

	if (!fiber)
	{
		munmap(mapping, mapping_size);
		return NULL;
	}

	fiber->stack_mapping = mapping;
	fiber->stack_mapping_size = mapping_size;
	fiber->Entry = Entry;
	fiber->param = param;

	u8 *stack_top = (u8 *)mapping + mapping_size;

#if OS_UNIX_FIBER_BACKEND_ASM_X64

	// stack_top is page aligned, so 16 byte aligned.
	u64 *frame = (u64 *)(stack_top - 64);

	// default floating point control words:
	const u64 MXCSR_CW = 0x1F80;
	const u64 X87_FPU_CW = 0x037F;
	
	frame[0] = (MXCSR_CW << 0) | (X87_FPU_CW << 32);
	frame[1] = 0;                       // r15
	frame[2] = 0;                       // r14
	frame[3] = (u64)(uptr)param;        // r13
	frame[4] = (u64)(uptr)Entry;        // r12
	frame[5] = 0;                       // rbx
	frame[6] = 0;                       // rbp
	frame[7] = (u64)(uptr)OS_UNIX_FiberAsmTrampoline;

	// After the final ret in the switch, rsp == stack_top,
	// so the trampoline's callq hands Entry the alignment the ABI wants.
	fiber->sp = frame;

#else

	getcontext(&fiber->uctx);

	fiber->uctx.uc_stack.ss_sp = (u8 *)mapping + page;
	fiber->uctx.uc_stack.ss_size = stack_size;
	fiber->uctx.uc_link = NULL;

	uptr p = (uptr)fiber;
	makecontext(&fiber->uctx, (void (*)(void))OS_UNIX_FiberUContextTrampoline, 2, (u32)(p & 0xFFFFFFFFu), (u32)(p >> 32));

#endif

	return fiber;
}

internal void OS_UNIX_FiberDelete(OS_UNIX_Fiber *fiber)
{
	if (!fiber)
		return;

	if (fiber->stack_mapping)
		munmap(fiber->stack_mapping, fiber->stack_mapping_size);

	free(fiber);
}

internal void OS_UNIX_FiberSwitch(OS_UNIX_Fiber *from, OS_UNIX_Fiber *to)
{
#if OS_UNIX_FIBER_BACKEND_ASM_X64
	OS_UNIX_FiberAsmSwitch(&from->sp, to->sp);
#else
	swapcontext(&from->uctx, &to->uctx);
#endif
}

internal OS_UNIX_Fiber *OS_UNIX_FiberConvertThread(void)
{
	if (unix_thread_fiber)
		return unix_thread_fiber;

	unix_thread_fiber = calloc(1, sizeof(OS_UNIX_Fiber));

	return unix_thread_fiber;
}

internal b32 OS_UNIX_FiberRevertThread(void)
{
	if (!unix_thread_fiber)
		return false;

	free(unix_thread_fiber);
	unix_thread_fiber = NULL;

	return true;
}

typedef struct OS_UNIX_ThreadStart OS_UNIX_ThreadStart;
struct OS_UNIX_ThreadStart
{
	void (*Entry)(void *param);
	void *param;
};

internal OS_Handle OS_UNIX_ThreadToHandle(pthread_t thread)
{
	OS_Handle handle = { (void *)(uintptr_t)thread };
	return handle;
}

internal pthread_t OS_UNIX_HandleToThread(OS_Handle handle)
{
	return (pthread_t)(uintptr_t)handle.value;
}

internal void *OS_UNIX_ThreadTrampoline(void *arg)
{
	OS_UNIX_ThreadStart start = *(OS_UNIX_ThreadStart *)arg;

	free(arg);
	
	start.Entry(start.param);
	
	return NULL;
}

internal OS_Handle OS_UNIX_ThreadCreate(void (*Entry)(void *param), void *param)
{
	OS_UNIX_ThreadStart *t = malloc(sizeof(OS_UNIX_ThreadStart));
	t->Entry = Entry;
	t->param = param;

	pthread_t thread;

	if (pthread_create(&thread, NULL, OS_UNIX_ThreadTrampoline, t) != 0)
	{
		free(t);
		return OS_HandleNull();
	}

	return OS_UNIX_ThreadToHandle(thread);
}

internal void OS_UNIX_ThreadJoin(OS_Handle handle)
{
	pthread_join(OS_UNIX_HandleToThread(handle), NULL);
}

internal void OS_UNIX_ThreadDetach(OS_Handle handle)
{
	pthread_detach(OS_UNIX_HandleToThread(handle));
}

internal b32 OS_UNIX_ThreadPinToCore(OS_Handle handle, u32 core_index)
{
#if defined(__linux__)
	
	cpu_set_t allowed;
	CPU_ZERO(&allowed);

	if (sched_getaffinity(0, sizeof(allowed), &allowed) != 0)
		return false;

	i32 allowed_count = CPU_COUNT(&allowed);

	if (allowed_count <= 0)
		return false;

	i32 wanted = (i32)(core_index % (u32)allowed_count);

	cpu_set_t pinned;
	CPU_ZERO(&pinned);

	for (i32 cpu = 0, seen = 0; cpu < CPU_SETSIZE; cpu++)
	{
		if (!CPU_ISSET(cpu, &allowed))
			continue;

		if (seen++ == wanted)
		{
			CPU_SET(cpu, &pinned);
			break;
		}
	}

	return pthread_setaffinity_np(OS_UNIX_HandleToThread(handle), sizeof(pinned), &pinned) == 0;
	
#else
	
	return false;
	
#endif
}

internal OS_Handle OS_UNIX_GetCurrentThreadHandle(void)
{
	return OS_UNIX_ThreadToHandle(pthread_self());
}

internal u32 OS_UNIX_TLSAlloc(void)
{
	pthread_key_t key;

	int result = pthread_key_create(&key, NULL);
	
	DebugLogAssert(unix_st.log_channel, result == 0, "Could not create TLS key.");

	return (u32)key;
}

internal void OS_UNIX_TLSFree(u32 slot)
{
	pthread_key_delete((pthread_key_t)slot);
}

internal void *OS_UNIX_TLSGet(u32 slot)
{
	return pthread_getspecific((pthread_key_t)slot);
}

internal void OS_UNIX_TLSSet(u32 slot, void *value)
{
	pthread_setspecific((pthread_key_t)slot, value);
}

internal i32 OS_UNIX_AtomicLoadI32(const i32 *ptr)
{
	return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
}

internal i64 OS_UNIX_AtomicLoadI64(const i64 *ptr)
{
	return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
}

internal void *OS_UNIX_AtomicLoadPtr(const void **ptr)
{
	return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
}

internal void OS_UNIX_AtomicStoreI32(i32 *ptr, i32 value)
{
	__atomic_store_n(ptr, value, __ATOMIC_RELEASE);
}

internal void OS_UNIX_AtomicStoreI64(i64 *ptr, i64 value)
{
	__atomic_store_n(ptr, value, __ATOMIC_RELEASE);
}

internal void OS_UNIX_AtomicStorePtr(void **ptr, void *value)
{
	__atomic_store_n(ptr, value, __ATOMIC_RELEASE);
}

internal i32 OS_UNIX_AtomicExchangeI32(i32 *ptr, i32 value)
{
	return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

internal i64 OS_UNIX_AtomicExchangeI64(i64 *ptr, i64 value)
{
	return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

internal void *OS_UNIX_AtomicExchangePtr(void **ptr, void *value)
{
	return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

internal i32 OS_UNIX_AtomicCompareExchangeI32(i32 *ptr, i32 exchange, i32 comperand)
{
	__atomic_compare_exchange_n(ptr, &comperand, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return comperand;
}

internal i64 OS_UNIX_AtomicCompareExchangeI64(i64 *ptr, i64 exchange, i64 comperand)
{
	__atomic_compare_exchange_n(ptr, &comperand, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return comperand;
}

internal void *OS_UNIX_AtomicCompareExchangePtr(void **ptr, void *exchange, void *comperand)
{
	__atomic_compare_exchange_n(ptr, &comperand, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return comperand;
}

internal i32 OS_UNIX_AtomicFetchAddI32(i32 *ptr, i32 delta)
{
	return __atomic_fetch_add(ptr, delta, __ATOMIC_SEQ_CST);
}

internal i64 OS_UNIX_AtomicFetchAddI64(i64 *ptr, i64 delta)
{
	return __atomic_fetch_add(ptr, delta, __ATOMIC_SEQ_CST);
}

internal void OS_UNIX_SpinLockAcquire(i32 *lock)
{
	for (;;)
	{
		if (__atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE) == 0)
			break;
		
		while (__atomic_load_n(lock, __ATOMIC_RELAXED) == 1)
			OS_UNIX_SPIN_PAUSE();
	}
}

internal void OS_UNIX_SpinLockRelease(i32 *lock)
{
	__atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

internal OS_UNIX_Object *OS_UNIX_AllocObject(void)
{
	OS_UNIX_Object *object = NULL;

	OS_UNIX_SpinLockAcquire(&unix_st.object_spinlock);
	{
		object = unix_st.free_objects;

		if (object)
			unix_st.free_objects = object->next_free;
		else
			object = ArenaPushArray(&unix_st.arena, OS_UNIX_Object, 1);
	}
	OS_UNIX_SpinLockRelease(&unix_st.object_spinlock);

	MemZeroStruct(object);

	return object;
}

internal void OS_UNIX_ReturnObject(OS_UNIX_Object *object)
{
	OS_UNIX_SpinLockAcquire(&unix_st.object_spinlock);
	{
		object->next_free = unix_st.free_objects;
		unix_st.free_objects = object;
	}
	OS_UNIX_SpinLockRelease(&unix_st.object_spinlock);
}

internal OS_Handle OS_UNIX_ThreadMutexCreate(void)
{
	OS_UNIX_Object *mtx = OS_UNIX_AllocObject();

	// todo: do we need to enable PTHREAD_MUTEX_RECURSIVE?
	
	pthread_mutex_init(&mtx->tmutex, NULL);

	OS_Handle handle = { mtx };
	return handle;
}

internal void OS_UNIX_ThreadMutexDestroy(OS_Handle handle)
{
	OS_UNIX_Object *mtx = handle.value;
	
	pthread_mutex_destroy(&mtx->tmutex);

	OS_UNIX_ReturnObject(mtx);
}

internal void OS_UNIX_ThreadMutexLock(OS_Handle handle)
{
	OS_UNIX_Object *mtx = handle.value;
	
	pthread_mutex_lock(&mtx->tmutex);
}

internal void OS_UNIX_ThreadMutexUnlock(OS_Handle handle)
{
	OS_UNIX_Object *mtx = handle.value;
	
	pthread_mutex_unlock(&mtx->tmutex);
}

internal OS_Handle OS_UNIX_ThreadCondVarCreate(void)
{
	OS_UNIX_Object *cnd = OS_UNIX_AllocObject();

	pthread_cond_init(&cnd->tcondvar, NULL);
	
	OS_Handle handle = { cnd };
	return handle;
}

internal void OS_UNIX_ThreadCondVarDestroy(OS_Handle handle)
{
	OS_UNIX_Object *cnd = handle.value;

	pthread_cond_destroy(&cnd->tcondvar);
	
	OS_UNIX_ReturnObject(cnd);
}

internal void OS_UNIX_ThreadCondVarWait(OS_Handle handle, OS_Handle thread_mutex_handle)
{
	OS_UNIX_Object *cnd = handle.value;
	OS_UNIX_Object *mtx = thread_mutex_handle.value;
	
	pthread_cond_wait(&cnd->tcondvar, &mtx->tmutex);
}

internal void OS_UNIX_ThreadCondVarSignal(OS_Handle handle)
{
	OS_UNIX_Object *cnd = handle.value;

	pthread_cond_signal(&cnd->tcondvar);
}

internal void OS_UNIX_ThreadCondVarBroadcast(OS_Handle handle)
{
	OS_UNIX_Object *cnd = handle.value;

	pthread_cond_broadcast(&cnd->tcondvar);
}

internal b32 OS_UNIX_FileDelete(String8 path)
{
	char p[OS_UNIX_PATH_MAX];

	if (!OS_UNIX_PathToCStr(p, sizeof(p), path))
		return false;

	return unlink(p) == 0;
}

internal b32 OS_UNIX_FileExists(String8 path)
{
	char p[OS_UNIX_PATH_MAX];
	struct stat st;

	if (!OS_UNIX_PathToCStr(p, sizeof(p), path) || stat(p, &st) != 0)
		return false;

	return !S_ISDIR(st.st_mode);
}

internal u64 OS_UNIX_GetFileLastWriteTimeInternal(const char *path)
{
	struct stat st;

	if (stat(path, &st) != 0)
		return 0;

#if defined(__APPLE__)
	struct timespec ts = st.st_mtimespec;
#else
	struct timespec ts = st.st_mtim;
#endif

	return ts.tv_sec;
}

internal u64 OS_UNIX_GetFileLastWriteTime(String8 path)
{
	char p[OS_UNIX_PATH_MAX];

	if (!OS_UNIX_PathToCStr(p, sizeof(p), path))
		return 0;

	return OS_UNIX_GetFileLastWriteTimeInternal(p);
}

internal b32 OS_UNIX_DirectoryCreate(String8 path)
{
	char p[OS_UNIX_PATH_MAX];

	if (!OS_UNIX_PathToCStr(p, sizeof(p), path))
		return false;

	return mkdir(p, 0755) == 0;
}

internal b32 OS_UNIX_DirectoryDelete(String8 path)
{
	char p[OS_UNIX_PATH_MAX];

	if (!OS_UNIX_PathToCStr(p, sizeof(p), path))
		return false;

	return rmdir(p) == 0;
}

internal b32 OS_UNIX_DirectoryExists(String8 path)
{
	char p[OS_UNIX_PATH_MAX];
	struct stat st;

	if (!OS_UNIX_PathToCStr(p, sizeof(p), path) || stat(p, &st) != 0)
		return false;

	return S_ISDIR(st.st_mode);
}

internal OS_Handle OS_UNIX_StreamFromFile(String8 path, OS_FileAccess access)
{
	b32 read	   = access & OS_FileAccess_Read;
	b32 write	   = access & OS_FileAccess_Write;
	b32 create	   = access & OS_FileAccess_CreateIfMissing;
	b32 overwrite  = access & OS_FileAccess_OverwriteIfExists;
	b32 append	   = access & OS_FileAccess_Append;
	b32 excl	   = access & OS_FileAccess_Exclusive;
	b32 non_binary = access & OS_FileAccess_NonBinary;

	AssertTrue(read || write);

	char cpath[OS_UNIX_PATH_MAX];

	if (!OS_UNIX_PathToCStr(cpath, sizeof(cpath), path))
		return OS_HandleNull();

	const char *bin = non_binary ? "" : "b";

	char mode[8] = {0};

	// 'x' (exclusive) has to come last to be valid.
	if (append)
		snprintf(mode, sizeof(mode), "a%s%s", read ? "+" : "", bin);
	else if (overwrite)
		snprintf(mode, sizeof(mode), "w%s%s%s", read ? "+" : "", bin, excl ? "x" : "");
	else
		snprintf(mode, sizeof(mode), "%s%s", write ? "r+" : "r", bin);

	SDL_IOStream *io = SDL_IOFromFile(cpath, mode);

	if (!io && create && !append && !overwrite)
	{
		snprintf(mode, sizeof(mode), "w+%s", bin);
		io = SDL_IOFromFile(cpath, mode);
	}

	OS_Handle handle = { io };
	return handle;
}

internal OS_Handle OS_UNIX_StreamFromMemory(void *memory, u64 bytes)
{
	OS_Handle handle = { SDL_IOFromMem(memory, bytes) };
	return handle;
}

internal OS_Handle OS_UNIX_StreamFromConstMemory(const void *memory, u64 bytes)
{
	OS_Handle handle = { SDL_IOFromConstMem(memory, bytes) };
	return handle;
}

internal i64 OS_UNIX_StreamRead(OS_Handle handle, void *dst, u64 bytes)
{
	return SDL_ReadIO(handle.value, dst, bytes);
}

internal i64 OS_UNIX_StreamWrite(OS_Handle handle, const void *src, u64 bytes)
{
	return SDL_WriteIO(handle.value, src, bytes);
}

internal i64 OS_UNIX_StreamSeek(OS_Handle handle, i64 offset)
{
	return SDL_SeekIO(handle.value, offset, SDL_IO_SEEK_SET);
}

internal i64 OS_UNIX_StreamSize(OS_Handle handle)
{
	return SDL_GetIOSize(handle.value);
}

internal i64 OS_UNIX_StreamPosition(OS_Handle handle)
{
	return SDL_TellIO(handle.value);
}

internal b32 OS_UNIX_StreamFlush(OS_Handle handle)
{
	return SDL_FlushIO(handle.value);
}

internal b32 OS_UNIX_StreamClose(OS_Handle handle)
{
	return SDL_CloseIO(handle.value);
}

internal void OS_UNIX_OpenInExplorer(String8 path)
{
	// TODO: not finished

	DebugLogW(unix_st.log_channel,
			  "Not implemented! But here we should open the following path in the system file explorer: \"%.*s\"",
			  String8VArg(path));
}

internal b32 OS_UNIX_VulkanSurfaceCreate(void *instance, void *surface_ptr)
{
	return SDL_Vulkan_CreateSurface(unix_st.sdl_window, (VkInstance)instance, NULL, (VkSurfaceKHR *)surface_ptr);
}

internal void OS_UNIX_VulkanSurfaceDestroy(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface((VkInstance)instance, (VkSurfaceKHR)surface, NULL);
}

internal const char * const *OS_UNIX_VulkanGetInstanceExtensions(u32 *count)
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

internal inline const char *OS_UNIX_LogLevelToString(LOG_Level level)
{
	switch (level)
	{
		case LOG_Level_Trace:  return "TRACE";
		case LOG_Level_Debug:  return "DEBUG";
		case LOG_Level_Info:   return "INFO ";
		case LOG_Level_Warn:   return "WARN ";
		case LOG_Level_Error:  return "ERROR";
		case LOG_Level_Break:  return "BREAK";
	}

	return "?????";
}

internal inline const char *OS_UNIX_LogLevelAnsi(LOG_Level level)
{
	switch (level)
	{
		case LOG_Level_Trace:  return "\x1b[90m";
		case LOG_Level_Debug:  return "\x1b[36m";
		case LOG_Level_Info:   return "\x1b[32m";
		case LOG_Level_Warn:   return "\x1b[33m";
		case LOG_Level_Error:  return "\x1b[31m";
		case LOG_Level_Break:  return "\x1b[41;30m";
	}

	return "";
}

internal LOG_Channel OS_UNIX_LoggerOpenChannelFrom(LOG_Channel parent, String8 name)
{
	OS_UNIX_SpinLockAcquire(&unix_logger->channel_spinlock);

	AssertTrue(unix_logger->channel_count < ArraySize(unix_logger->channels));

	OS_UNIX_LogChannelEntry *entry = &unix_logger->channels[unix_logger->channel_count];
	entry->name = name;
	entry->enabled = true;
	entry->parent = parent;

	LOG_Channel channel = { unix_logger->channel_count };

	unix_logger->channel_count++;

	OS_UNIX_SpinLockRelease(&unix_logger->channel_spinlock);

	return channel;
}

internal LOG_Channel OS_UNIX_LoggerOpenChannel(String8 name)
{
	// 0 = null channel = no parent
	LOG_Channel no_parent = {0};

	return OS_UNIX_LoggerOpenChannelFrom(no_parent, name);
}

internal void OS_UNIX_LoggerCloseChannel(LOG_Channel channel)
{
	unix_logger->channels[channel.id].enabled = false;
}

internal void OS_UNIX_LoggerMakeDedupBody(char *dst, i32 dst_size, const char *body, u32 count)
{
	if (count > 1)
		snprintf(dst, (usize)dst_size, "%s (%ux)", body, count);
	else
		snprintf(dst, (usize)dst_size, "%s", body);

}

internal void OS_UNIX_LoggerChannelNameResolve(LOG_Channel channel, char *dst, i32 dst_size)
{
	OS_UNIX_LogChannelEntry *entry = &unix_logger->channels[channel.id];

	if (entry->parent.id != 0)
	{
		char parent[64] = {0};
		OS_UNIX_LoggerChannelNameResolve(entry->parent, parent, sizeof(parent));
		snprintf(dst, (usize)dst_size, "%s/%.*s", parent, String8VArg(entry->name));
	}
	else
	{
		snprintf(dst, (usize)dst_size, "%.*s", String8VArg(entry->name));
	}
}

internal i32 OS_UNIX_LoggerFormatLine(char *dst, i32 dst_size,
									  LOG_Level level, LOG_Channel channel,
									  const char *file, i32 line, const char *fn,
									  const char *body,
									  b32 for_file, f32 elapsed,
									  OS_UNIX_JobContext job_context)
{
	const char *level_string = OS_UNIX_LogLevelToString (level);
	const char *level_ansi   = OS_UNIX_LogLevelAnsi     (level);

	i32 len = 0;
	b32 show_callsite = level >= LOG_Level_Error;

	// I just learned about this push/pop_macro stuff man this is so sick.
#pragma push_macro("Append")
#undef Append

#define Append(...)														\
	do																	\
	{																	\
		if (len < dst_size)												\
			len += snprintf(dst + len, (usize)(dst_size - len), __VA_ARGS__); \
	}																	\
	while (0)

	// Timestamp.
	if (for_file)
		Append("[  %7.3f  ] ", elapsed);
	else
		Append(OS_UNIX_LOG_ANSI_DIM "[  %7.3f  ]" OS_UNIX_LOG_ANSI_RESET " ", elapsed);

	// Job Context.
	{
		char worker_str[16] = {0};
		char fiber_str[16] = {0};

		// We could jsut use worker_id = 0 but
		// writing MT makes it more obvious so
		// I'm going with that.
		if (job_context.worker_id == 0)
			snprintf(worker_str, sizeof(worker_str), "MT ");
		else
			snprintf(worker_str, sizeof(worker_str), "%-3u", job_context.worker_id);

		if (job_context.fiber_id == -1)
			snprintf(fiber_str, sizeof(fiber_str), "---");
		else
			snprintf(fiber_str, sizeof(fiber_str), "%-3d", job_context.fiber_id);

		if (for_file)
			Append("[  W:%s F:%s  ] ", worker_str, fiber_str);
		else
			Append(OS_UNIX_LOG_ANSI_DIM "[  W:%s F:%s  ]" OS_UNIX_LOG_ANSI_RESET " ", worker_str, fiber_str);
	}

	// Level.
	if (for_file)
		Append("[  %s  ] ", level_string);

	// Channel.
	char channel_name[512] = {0};
	OS_UNIX_LoggerChannelNameResolve(channel, channel_name, sizeof(channel_name));

	if (for_file)
		Append("[  %-*s  ] ", OS_UNIX_LOG_CHANNEL_COL_ALIGN, channel_name);
	else
		Append("%s[  %-*s  ]" OS_UNIX_LOG_ANSI_RESET " ", level_ansi, OS_UNIX_LOG_CHANNEL_COL_ALIGN, channel_name);

	// Callsite.
	if (show_callsite)
	{
		const char *filename = file;
		for (const char *p = file; *p; p++)
			if (*p == '/' || *p == '\\')
				filename = p + 1;

		if (for_file)
			Append("%s:%d %s: ", filename, line, fn);
		else
			Append(OS_UNIX_LOG_ANSI_DIM "%s:%d %s:" OS_UNIX_LOG_ANSI_RESET " ", filename, line, fn);
	}

	Append("%s\n", body);

#pragma pop_macro("Append")

	if (len >= dst_size)
		len = dst_size - 1;

	return len;

}

internal void OS_UNIX_LoggerWriteToFile(const char *line, i32 len)
{
	if (len <= 0 || OS_HandleIsNull(unix_logger->file_stream))
		return;

	OS_UNIX_StreamWrite(unix_logger->file_stream, line, (u64)len);

	// necessary for linux since on linux it's buffered so after an assert
	// crash the last couple lines would be lost, unlike on windows.
	OS_UNIX_StreamFlush(unix_logger->file_stream);
}

internal void OS_UNIX_LoggerFlushDedupToFile(f32 elapsed)
{
	if (!unix_logger->dedup_active || OS_HandleIsNull(unix_logger->file_stream))
		return;

	if (unix_logger->dedup_count <= 1)
		return;

	char body[OS_UNIX_LOG_LINE_BUFFER_SIZE] = {0};
	OS_UNIX_LoggerMakeDedupBody(body, sizeof(body), unix_logger->dedup_body, unix_logger->dedup_count);

	char file_line[OS_UNIX_LOG_LINE_BUFFER_SIZE] = {0};

	i32 file_len = OS_UNIX_LoggerFormatLine(file_line, sizeof(file_line),
											unix_logger->dedup_level,
											unix_logger->dedup_channel,
											"", 0, "",
											body,
											true, elapsed,
											unix_logger->dedup_job_context);

	OS_UNIX_LoggerWriteToFile(file_line, file_len);
}

internal void OS_UNIX_LoggerInit(OS_UNIX_Logger *logger, String8 sink)
{
	logger->mutex = OS_UNIX_FiberMutexCreate();
	
	CH_TimerStart(&logger->timer);

	unix_logger = logger;
	
	logger->null_channel = OS_UNIX_LoggerOpenChannel(String8Lit("----------"));
	logger->log_channel = OS_UNIX_LoggerOpenChannel(String8Lit("UNIX/LOG")); // yes yes, i know. I'm faking the parent channel for aesthetics because I can't make the OS unix channel until the logging system is created!

	if (sink.len > 0)
	{
		logger->file_stream = OS_UNIX_StreamFromFile(sink, OS_FILE_PRESET_CREATE);
		DebugLogI(logger->log_channel, "Initialized (\"%.*s\" as file output).", String8VArg(sink));
	}
	else
	{
		DebugLogI(logger->log_channel, "Initialized (no file sink).");
	}
}

internal void OS_UNIX_LoggerShutdown(void)
{
	if (unix_logger->dedup_active)
	{
		f32 elapsed = CH_TimerElapsed(&unix_logger->timer);
		OS_UNIX_LoggerFlushDedupToFile(elapsed);
		unix_logger->dedup_active = false;
	}

	DebugLogI(unix_logger->log_channel, "Shutting down...");

	if (!OS_HandleIsNull(unix_logger->file_stream))
	{
		OS_UNIX_StreamClose(unix_logger->file_stream);
		unix_logger->file_stream = OS_HandleNull();
	}

	OS_UNIX_FiberMutexDestroy(unix_logger->mutex);
	unix_logger->mutex = OS_HandleNull();

	unix_logger = NULL;
}

internal void OS_UNIX_LoggerWriteV(OS_UNIX_JobContext job_context,
								   LOG_Level level, LOG_Channel channel,
								   const char *file, i32 line, const char *fn,
								   const char *fmt, va_list args)
{
	AssertTrue(unix_logger);

	if (level < LOG_COMPILE_MIN_FILTER)
		return;

	if (!unix_logger->channels[channel.id].enabled)
		return;

	f32 elapsed = CH_TimerElapsed(&unix_logger->timer);

	char body[OS_UNIX_LOG_LINE_BUFFER_SIZE] = {0};
	vsnprintf(body, sizeof(body), fmt, args);

	OS_UNIX_FiberMutexLock(unix_logger->mutex);
	{
		b32 is_repeated_line =
			unix_logger->dedup_active &&
			unix_logger->dedup_level == level &&
			unix_logger->dedup_job_context.fiber_id == job_context.fiber_id &&
			LOG_ChannelMatch(unix_logger->dedup_channel, channel) &&
			(CStrCompare(unix_logger->dedup_body, body) == 0);

		if (is_repeated_line)
		{
			unix_logger->dedup_count++;

			char dedup_body[OS_UNIX_LOG_LINE_BUFFER_SIZE] = {0};
			OS_UNIX_LoggerMakeDedupBody(dedup_body, sizeof(dedup_body), body, unix_logger->dedup_count);

			char console_line[OS_UNIX_LOG_LINE_BUFFER_SIZE] = {0};

			i32 console_len = OS_UNIX_LoggerFormatLine(console_line, sizeof(console_line),
													   level, channel,
													   file, line, fn,
													   dedup_body,
													   false, elapsed,
													   unix_logger->dedup_job_context);

			if (console_len > 0)
			{
				i32 write_len = console_len;

				if (console_line[write_len - 1] == '\n')
					write_len--;

				if (unix_logger->dedup_count == 2) // jump back to previous line to overwrite
					fwrite("\x1b[1A\r", 1, 5, stdout);
				else
					fwrite("\r", 1, 1, stdout);

				fwrite(console_line, 1, (usize)write_len, stdout);

				fflush(stdout);
			}
		}
		else
		{
			if (unix_logger->dedup_active)
			{
				OS_UNIX_LoggerFlushDedupToFile(elapsed);

				if (unix_logger->dedup_count > 1)
					fwrite("\n", 1, 1, stdout);
			}

			char console_line[OS_UNIX_LOG_LINE_BUFFER_SIZE] = {0};

			i32 console_len = OS_UNIX_LoggerFormatLine(console_line, sizeof(console_line),
													   level, channel,
													   file, line, fn,
													   body,
													   false, elapsed,
													   job_context);

			if (console_len > 0)
			{
				fwrite(console_line, 1, (usize)console_len, stdout);

				if (level >= LOG_Level_Warn)
					fflush(stdout);
			}

			if (!OS_HandleIsNull(unix_logger->file_stream))
			{
				char file_line[OS_UNIX_LOG_LINE_BUFFER_SIZE] = {0};

				i32 file_len = OS_UNIX_LoggerFormatLine(file_line, sizeof(file_line),
														level, channel,
														file, line, fn,
														body,
														true, elapsed,
														job_context);

				OS_UNIX_LoggerWriteToFile(file_line, file_len);
			}

			snprintf(unix_logger->dedup_body, sizeof(unix_logger->dedup_body), "%s", body);
			unix_logger->dedup_level = level;
			unix_logger->dedup_channel = channel;
			unix_logger->dedup_count = 1;
			unix_logger->dedup_active = true;
			unix_logger->dedup_job_context = job_context;
		}
	}
	OS_UNIX_FiberMutexUnlock(unix_logger->mutex);

	if (level == LOG_Level_Break)
	{
		fflush(stdout);

		if (!OS_HandleIsNull(unix_logger->file_stream))
		{
			OS_UNIX_StreamClose(unix_logger->file_stream);
			unix_logger->file_stream = OS_HandleNull();
		}

		AssertTrue(false);
	}
}

internal void OS_UNIX_JobSpinModeEnable(void)
{
	OS_UNIX_AtomicExchangeI32(&unix_job_sched->atomic_spin_mode, 1);
}

internal void OS_UNIX_JobSpinModeDisable(void)
{
	OS_UNIX_AtomicExchangeI32(&unix_job_sched->atomic_spin_mode, 0);
}

internal b32 OS_UNIX_JobIsMainThread(void)
{
	OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();

	DebugLogAssert(unix_job_sched->log_channel, worker, "No worker running.");

	return worker->id == 0;
}

internal OS_UNIX_JobContext OS_UNIX_JobGetContext(void)
{
	OS_UNIX_JobContext ctx = {0};
	ctx.worker_id = 0;
	ctx.fiber_id = -1;
	
	OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();

	if (worker)
	{
		ctx.worker_id = worker->id;
		ctx.fiber_id = worker->current_fiber ? (i32)worker->current_fiber->id : -1;
	}

	return ctx;
}

// NOTE: caller must be owner of unlock_after_switch_spinlock!
//       unlock_after_switch_spinlock is released by the scheduler
//       fiber after this fiber's context has been saved, which makes
//       it safe to put the fiber calling this onto the waitlist, and then
//       yield: whoever wants to wake you has to take that same spinlock
//       first, so can't resume you until the switch has full happened.
internal void OS_UNIX_JobFiberSuspendMyself(i32 *unlock_after_switch_spinlock)
{
	OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();
	OS_UNIX_JobFiber *fiber = worker->current_fiber;

	fiber->finished = false;

	worker->pending_unlock = unlock_after_switch_spinlock;

	OS_UNIX_FiberSwitch(fiber->context, worker->fiber);
}


internal void OS_UNIX_JobFiberMarkMyJobAsCompleted(void)
{
	OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();
	OS_UNIX_JobFiber *fiber = worker->current_fiber;

	fiber->finished = true;
	
	worker->pending_unlock = NULL;

	OS_UNIX_FiberSwitch(fiber->context, worker->fiber);
}

internal OS_UNIX_JobFiber *OS_UNIX_JobFiberFetchFree(void)
{
	OS_UNIX_SpinLockAcquire(&unix_job_sched->fiber_pool_spinlock);
 
	OS_UNIX_JobFiber *fiber = unix_job_sched->fiber_pool_head;
	
	if (fiber)
		unix_job_sched->fiber_pool_head = fiber->next_free;

	OS_UNIX_SpinLockRelease(&unix_job_sched->fiber_pool_spinlock);
 
	if (fiber)
		fiber->next_free = NULL;
 
	return fiber;
}

internal void OS_UNIX_JobFiberReturn(OS_UNIX_JobFiber *fiber)
{
	fiber->EntryPoint = NULL;
	fiber->param = NULL;
	
	fiber->counter = NULL;
	fiber->flags = 0;
	fiber->finished = true;
 
	OS_UNIX_SpinLockAcquire(&unix_job_sched->fiber_pool_spinlock);
	{
		fiber->next_free = unix_job_sched->fiber_pool_head;
		unix_job_sched->fiber_pool_head = fiber;
	}
	OS_UNIX_SpinLockRelease(&unix_job_sched->fiber_pool_spinlock);
}

internal b32 OS_UNIX_JobQueueTryPopRequest(OS_UNIX_JobQueue *queue, OS_UNIX_JobRequest *out)
{
	if ((u32)OS_UNIX_AtomicLoadI32(&queue->atomic_added_task_count) ==
		(u32)OS_UNIX_AtomicLoadI32(&queue->atomic_taken_task_count))
		return false;

	b32 popped = false;

	OS_UNIX_SpinLockAcquire(&queue->atomic_spinlock);
	{
		u32 t = (u32)queue->atomic_taken_task_count;
		u32 a = (u32)queue->atomic_added_task_count;

		if (a != t)
		{
			*out = queue->requests[t % OS_UNIX_JOB_MAX_JOBS_PER_QUEUE];
			OS_UNIX_AtomicStoreI32(&queue->atomic_taken_task_count, (i32)(t + 1));
			popped = true;
		}
	}
	OS_UNIX_SpinLockRelease(&queue->atomic_spinlock);

	return popped;
}

internal OS_UNIX_JobFiber *OS_UNIX_JobQueueTryPopWaiting(OS_UNIX_JobQueue *queue)
{
	if ((u32)OS_UNIX_AtomicLoadI32(&queue->atomic_added_waiting_count) ==
		(u32)OS_UNIX_AtomicLoadI32(&queue->atomic_taken_waiting_count))
		return NULL;

	OS_UNIX_JobFiber *fiber = NULL;

	OS_UNIX_SpinLockAcquire(&queue->atomic_spinlock);
	{
		u32 t = (u32)queue->atomic_taken_waiting_count;
		u32 a = (u32)queue->atomic_added_waiting_count;

		if (a != t)
		{
			fiber = queue->waiting[t % OS_UNIX_JOB_MAX_JOBS_PER_QUEUE];
			OS_UNIX_AtomicStoreI32(&queue->atomic_taken_waiting_count, (i32)(t + 1));
		}
	}
	OS_UNIX_SpinLockRelease(&queue->atomic_spinlock);

	return fiber;
}

internal b32 OS_UNIX_JobQueueHasWork(OS_UNIX_JobQueue *queue)
{
	b32 has_tasks =
		(u32)OS_UNIX_AtomicLoadI32(&queue->atomic_added_task_count) !=
		(u32)OS_UNIX_AtomicLoadI32(&queue->atomic_taken_task_count);

	b32 has_waiting =
		(u32)OS_UNIX_AtomicLoadI32(&queue->atomic_added_waiting_count) !=
		(u32)OS_UNIX_AtomicLoadI32(&queue->atomic_taken_waiting_count);

	return has_tasks || has_waiting;
}

internal b32 OS_UNIX_JobTryGetRequest(b32 is_main_thread, OS_UNIX_JobRequest *out)
{
	if (is_main_thread && OS_UNIX_JobQueueTryPopRequest(&unix_job_sched->main_thread_queue, out))
		return true;
	
	for (i32 i = J_Priority_COUNT - 1; i >= 0; i--)
	{
		if (OS_UNIX_JobQueueTryPopRequest(&unix_job_sched->queues[i], out))
			return true;
	}
 
	return false;
}

internal OS_UNIX_JobFiber *OS_UNIX_JobTryGetWaitingFiber(b32 is_main_thread)
{
	OS_UNIX_JobFiber *fiber = NULL;

	if (is_main_thread)
	{
		fiber = OS_UNIX_JobQueueTryPopWaiting(&unix_job_sched->main_thread_queue);

		if (fiber)
			return fiber;
	}

	for (i32 i = J_Priority_COUNT - 1; i >= 0; i--)
	{
		fiber = OS_UNIX_JobQueueTryPopWaiting(&unix_job_sched->queues[i]);

		if (fiber)
			return fiber;
	}
 
	return NULL;
}

internal b32 OS_UNIX_JobRequestAvailable(b32 is_main_thread)
{
	if (is_main_thread && OS_UNIX_JobQueueHasWork(&unix_job_sched->main_thread_queue))
		return true;

	for (u32 i = 0; i < J_Priority_COUNT; i++)
	{
		if (OS_UNIX_JobQueueHasWork(&unix_job_sched->queues[i]))
			return true;
	}
	
	return false;
}

internal void OS_UNIX_JobWakeAllWorkers(void)
{
	// mutex because we need to protect against workers that have
	// just checked for work but not yet gone to sleep.
	
	OS_UNIX_ThreadMutexLock(unix_job_sched->thread_mutex);
	OS_UNIX_ThreadCondVarBroadcast(unix_job_sched->thread_cond_begin);
	OS_UNIX_ThreadMutexUnlock(unix_job_sched->thread_mutex);
}

internal void OS_UNIX_JobWakeOneWorker(void)
{
	// mutex because we need to protect against workers that have
	// just checked for work but not yet gone to sleep.
	
	OS_UNIX_ThreadMutexLock(unix_job_sched->thread_mutex);
	OS_UNIX_ThreadCondVarSignal(unix_job_sched->thread_cond_begin);
	OS_UNIX_ThreadMutexUnlock(unix_job_sched->thread_mutex);
}

internal b32 OS_UNIX_JobTryPush(const OS_UNIX_JobRequest *request)
{
	OS_UNIX_JobQueue *queue = &unix_job_sched->queues[request->priority];

	if (request->flags & J_Flag_MainThreadOnly)
		queue = &unix_job_sched->main_thread_queue;

	b32 pushed = false;

	OS_UNIX_SpinLockAcquire(&queue->atomic_spinlock);
	{
		u32 t = (u32)queue->atomic_taken_task_count;
		u32 a = (u32)queue->atomic_added_task_count;

		if ((a - t) < OS_UNIX_JOB_MAX_JOBS_PER_QUEUE)
		{
			queue->requests[a % OS_UNIX_JOB_MAX_JOBS_PER_QUEUE] = *request;
			OS_UNIX_AtomicStoreI32(&queue->atomic_added_task_count, (i32)(a + 1));
			pushed = true;
		}
	}
	OS_UNIX_SpinLockRelease(&queue->atomic_spinlock);

	return pushed;
}

internal void OS_UNIX_JobPush(const OS_UNIX_JobRequest *request)
{
	b32 woke_workers = false;

	while (!OS_UNIX_JobTryPush(request))
	{
		if (!woke_workers)
		{
			OS_UNIX_JobWakeAllWorkers();
			woke_workers = true;
		}

		OS_UNIX_SPIN_PAUSE();
	}
}

internal void OS_UNIX_JobPushWaitingFiber(OS_UNIX_JobFiber *fiber)
{
	OS_UNIX_JobQueue *queue = &unix_job_sched->queues[fiber->priority];

	if (fiber->flags & J_Flag_MainThreadOnly)
		queue = &unix_job_sched->main_thread_queue;
 
	for (;;)
	{
		OS_UNIX_SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = (u32)queue->atomic_taken_waiting_count;
		u32 a = (u32)queue->atomic_added_waiting_count;

		b32 is_full = (a - t) >= OS_UNIX_JOB_MAX_JOBS_PER_QUEUE;
		
		if (is_full)
		{
			OS_UNIX_SpinLockRelease(&queue->atomic_spinlock);
			
			OS_UNIX_SPIN_PAUSE();
		}
		else
		{
			queue->waiting[a % OS_UNIX_JOB_MAX_JOBS_PER_QUEUE] = fiber;
			OS_UNIX_AtomicStoreI32(&queue->atomic_added_waiting_count, (i32)(a + 1));
			
			OS_UNIX_SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

internal void OS_UNIX_JobCounterInit(OS_UNIX_JobCounter *counter, i32 initial_count)
{
	counter->atomic_count = initial_count;
}

internal void OS_UNIX_JobCounterIncrement(OS_UNIX_JobCounter *counter, i32 n)
{
	OS_UNIX_AtomicFetchAddI32(&counter->atomic_count, n);
}

/*
 * Decrement the counter, but if we hit zero we have to collect all of the
 * waiting fibers and kick them off.
 */
internal void OS_UNIX_JobCounterDecrement(OS_UNIX_JobCounter *counter, i32 n)
{
	OS_UNIX_SpinLockAcquire(&counter->atomic_spinlock);

	i32 atomic_count = OS_UNIX_AtomicLoadI32(&counter->atomic_count);
 
	DebugLogAssert(unix_job_sched->log_channel, atomic_count >= n, "Asking to subtract more than possible.");

	OS_UNIX_AtomicFetchAddI32(&counter->atomic_count, -n);

	atomic_count -= n;
 
	u32 kick_count = 0;
	OS_UNIX_JobFiber *to_kick[OS_UNIX_JOB_COUNTER_MAX_WAITING] = {0};
 
	if (atomic_count == 0 && counter->waiting_count > 0)
	{
		kick_count = counter->waiting_count;
		MemCopy(to_kick, counter->waiting, kick_count * sizeof(OS_UNIX_JobFiber *));
		counter->waiting_count = 0;
	}
 
	OS_UNIX_SpinLockRelease(&counter->atomic_spinlock);

	// note to self:
	// since the 'counter' might live on the stack of the waiter,
	// after this spinlock release it might have gone out of
	// scope so no touching it anymore!
	counter = NULL;
	
	for (u32 i = 0; i < kick_count; i++)
		OS_UNIX_JobPushWaitingFiber(to_kick[i]);
 
	if (kick_count > 0)
		OS_UNIX_JobWakeAllWorkers();
}

internal i32 OS_UNIX_JobCounterRead(OS_UNIX_JobCounter *counter)
{
	return OS_UNIX_AtomicLoadI32(&counter->atomic_count);
}

internal void OS_UNIX_JobYieldOnCounter(OS_UNIX_JobCounter *counter, i32 value)
{
	for (;;)
	{
		OS_UNIX_SpinLockAcquire(&counter->atomic_spinlock);
 
		if (OS_UNIX_AtomicLoadI32(&counter->atomic_count) == value)
		{
			OS_UNIX_SpinLockRelease(&counter->atomic_spinlock);
			return;
		}

		OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();

		DebugLogAssert(unix_job_sched->log_channel, worker && worker->current_fiber, "Yielding can only be done from inside a job.");
		DebugLogAssert(unix_job_sched->log_channel, counter->waiting_count < OS_UNIX_JOB_COUNTER_MAX_WAITING, "Cannot make more jobs wait on counter!!");
		
		counter->waiting[counter->waiting_count++] = worker->current_fiber;
 
		OS_UNIX_JobFiberSuspendMyself(&counter->atomic_spinlock);
	}
}

internal void OS_UNIX_JobFiberMutexLock(OS_UNIX_JobFiberMutex *m)
{
	for (;;)
	{
		if (OS_UNIX_AtomicCompareExchangeI32(&m->atomic_mutex_state, 1, 0) == 0)
			return;

		OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();

		if (!worker || !worker->current_fiber)
		{
			// we are not inside of a job (so in startup, shutdown, or a scheduler loop)
			// therefore there is no fiber to put to sleep, so we'll just spin i guess.
			
			// TODO is this even necessary??
			
			OS_UNIX_SPIN_PAUSE();
			continue;
		}

		OS_UNIX_SpinLockAcquire(&m->atomic_spinlock);

		if (OS_UNIX_AtomicCompareExchangeI32(&m->atomic_mutex_state, 1, 0) == 0)
		{
			OS_UNIX_SpinLockRelease(&m->atomic_spinlock);
			return;
		}

		DebugLogAssert(unix_job_sched->log_channel, m->waiting_count < OS_UNIX_JOB_COUNTER_MAX_WAITING, "Cannot make more jobs wait on mutex!!!");
		
		m->waiting[m->waiting_count++] = worker->current_fiber;

		OS_UNIX_JobFiberSuspendMyself(&m->atomic_spinlock);
	}
}

internal void OS_UNIX_JobFiberMutexUnlock(OS_UNIX_JobFiberMutex *m)
{
	OS_UNIX_SpinLockAcquire(&m->atomic_spinlock);

	OS_UNIX_JobFiber *next = NULL;

	if (m->waiting_count > 0)
	{
		next = m->waiting[0];

		for (u32 i = 1; i < m->waiting_count; i++)
			m->waiting[i - 1] = m->waiting[i];
		
		m->waiting_count--;
	}

	OS_UNIX_AtomicExchangeI32(&m->atomic_mutex_state, 0);

	OS_UNIX_SpinLockRelease(&m->atomic_spinlock);

	if (next)
	{
		OS_UNIX_JobPushWaitingFiber(next);
		OS_UNIX_JobWakeAllWorkers();
	}
}

internal void OS_UNIX_JobFiberCondVarWait(OS_UNIX_JobFiberCondVar *cv, OS_UNIX_JobFiberMutex *mutex)
{
	OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();

	DebugLogAssert(unix_job_sched->log_channel, worker && worker->current_fiber, "CondVarWait can only be called from inside a job.");

	OS_UNIX_SpinLockAcquire(&cv->atomic_spinlock);

	DebugLogAssert(unix_job_sched->log_channel, cv->waiting_count < OS_UNIX_JOB_COUNTER_MAX_WAITING, "Cannot wait more jobs on condition variable!!");
	
	cv->waiting[cv->waiting_count++] = worker->current_fiber;

	// still holding the cv spinlock here, so a signal can't resume us before
	// we've switched away, even though the mutex is already free for others.
	OS_UNIX_JobFiberMutexUnlock(mutex);
	OS_UNIX_JobFiberSuspendMyself(&cv->atomic_spinlock);
	OS_UNIX_JobFiberMutexLock(mutex);
}

internal void OS_UNIX_JobFiberCondVarSignal(OS_UNIX_JobFiberCondVar *cv)
{
	OS_UNIX_SpinLockAcquire(&cv->atomic_spinlock);

	OS_UNIX_JobFiber *next = NULL;
	
	if (cv->waiting_count > 0)
	{
		next = cv->waiting[0];
		
		for (u32 i = 1; i < cv->waiting_count; i++)
			cv->waiting[i - 1] = cv->waiting[i];
		
		cv->waiting_count--;
	}

	OS_UNIX_SpinLockRelease(&cv->atomic_spinlock);

	if (next)
	{
		OS_UNIX_JobPushWaitingFiber(next);
		OS_UNIX_JobWakeAllWorkers();
	}
}

internal void OS_UNIX_JobFiberCondVarBroadcast(OS_UNIX_JobFiberCondVar *cv)
{
	OS_UNIX_SpinLockAcquire(&cv->atomic_spinlock);

	u32 kick_count = cv->waiting_count;
	OS_UNIX_JobFiber *to_kick[OS_UNIX_JOB_COUNTER_MAX_WAITING] = {0};
	
	MemCopy(to_kick, cv->waiting, kick_count * sizeof(OS_UNIX_JobFiber *));

	cv->waiting_count = 0;

	OS_UNIX_SpinLockRelease(&cv->atomic_spinlock);

	for (u32 i = 0; i < kick_count; i++)
		OS_UNIX_JobPushWaitingFiber(to_kick[i]);

	if (kick_count > 0)
		OS_UNIX_JobWakeAllWorkers();
}

internal void OS_UNIX_JobKickRaw(const J_Decl *decl, OS_UNIX_JobCounter *counter)
{
	if (counter)
		OS_UNIX_JobCounterIncrement(counter, 1);
 
	OS_UNIX_JobRequest request = {0};
	request.EntryPoint = decl->EntryPoint;
	request.param = decl->param;
	request.priority = decl->priority;
	request.flags = decl->flags;
	request.counter = counter;
 
	OS_UNIX_JobPush(&request);

	OS_UNIX_JobWakeOneWorker();
}

internal void OS_UNIX_JobBatchRaw(const J_Decl *decls, u32 count, OS_UNIX_JobCounter *counter)
{
	if (counter)
		OS_UNIX_JobCounterIncrement(counter, count);
 
	for (u32 i = 0; i < count; i++)
	{
		OS_UNIX_JobRequest request = {0};
		request.EntryPoint = decls[i].EntryPoint;
		request.param = decls[i].param;
		request.priority = decls[i].priority;
		request.flags = decls[i].flags;
		request.counter = counter;
 
		OS_UNIX_JobPush(&request);
	}
 
	OS_UNIX_JobWakeAllWorkers();
}

typedef struct OS_UNIX_JobParallelForParam OS_UNIX_JobParallelForParam;
struct OS_UNIX_JobParallelForParam
{
	J_EntryForFn *Inner;
	u32 base_index;
	u32 loop_size;
};

internal J_ENTRY_POINT_DEF(OS_UNIX_JobParallelForBatchEntry)
{
	OS_UNIX_JobParallelForParam *p = param;
	
	for (u32 i = 0; i < p->loop_size; i++)
		p->Inner(p->base_index + i);
}

internal void OS_UNIX_JobFor(u32 count, J_EntryForFn *fn, J_Priority priority, u32 batch_size)
{
	if (count == 0 || batch_size == 0)
		return;
 
	u32 job_count = (count + batch_size - 1) / batch_size;
 
	ScratchArena scratch = ScratchBegin(NULL, 0);
 
	OS_UNIX_JobParallelForParam *params = ArenaPushArray(scratch.arena, OS_UNIX_JobParallelForParam, job_count);
 
	OS_UNIX_JobCounter counter = {0};
	b32 pushed_any = false;
	b32 woke_workers = false;

	for (u32 i = 0; i < job_count; i++)
	{
		u32 base_index = batch_size * i;
		u32 loop_size = count - base_index;
		
		if (loop_size > batch_size)
			loop_size = batch_size;
 
		params[i].Inner = fn;
		params[i].base_index = base_index;
		params[i].loop_size = loop_size;

		OS_UNIX_JobRequest request = {0};
		request.EntryPoint = OS_UNIX_JobParallelForBatchEntry;
		request.param = &params[i];
		request.priority = priority;
		request.flags = J_Flag_None;
		request.counter = &counter;

		OS_UNIX_JobCounterIncrement(&counter, 1);

		if (OS_UNIX_JobTryPush(&request))
		{
			pushed_any = true;
			continue;
		}

		// the queue if full.
		// spinning could deadlock of the workers are asleep or we're the only
		// one, so we wake all the workers and then do this batch ourselves
		// to keep ourselves busy.
		
		OS_UNIX_AtomicFetchAddI32(&counter.atomic_count, -1);

		if (!woke_workers)
		{
			OS_UNIX_JobWakeAllWorkers();
			woke_workers = true;
		}

		OS_UNIX_JobParallelForBatchEntry(&params[i]);
	}

	if (pushed_any)
		OS_UNIX_JobWakeAllWorkers();

	OS_UNIX_JobYieldOnCounter(&counter, 0);
 
	ScratchRelease(&scratch);
}

internal Arena *OS_UNIX_JobGetScratch(Arena * const *conflicts, u32 conflict_count)
{
	Arena *arena = NULL;
	Arena *ring = NULL;

	OS_UNIX_JobWorker *worker = OS_UNIX_JobGetCurrentWorker();

	if (worker && worker->current_fiber)
		ring = worker->current_fiber->scratch_arenas;
	else
		ring = unix_job_sched->fallback_scratch_ring;

	for (u32 i = 0; i < OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE; i++, ring++)
	{
		b32 conflict = false;

		for (u32 j = 0; j < conflict_count; j++)
		{
			if (ring == conflicts[j])
			{
				conflict = true;
				break;
			}
		}

		if (!conflict)
		{
			arena = ring;
			break;
		}
	}

	DebugLogAssert(unix_job_sched->log_channel, arena, "We must have found scratch arena by now. This is mathematically impossible to hit, the fuck?");

	return arena;
}

internal void OS_UNIX_JobFiberEntry(void *param)
{
	for (;;)
	{
		OS_UNIX_JobFiber *f = OS_UNIX_JobGetCurrentWorker()->current_fiber;
		OS_UNIX_JobCounter *c = f->counter;

		if (f->EntryPoint)
			f->EntryPoint(f->param);
		
		if (c)
			OS_UNIX_JobCounterDecrement(c, 1);

		// Comes back here once the scheduler hands this fiber a new job.
		OS_UNIX_JobFiberMarkMyJobAsCompleted();
	}
}

// run a job fiber until it yields or completes, then deal
// with the fallout back on the worker's own scheduler fiber.
internal void OS_UNIX_JobRunFiber(OS_UNIX_JobWorker *worker, OS_UNIX_JobFiber *fiber)
{
	worker->current_fiber = fiber;

	OS_UNIX_FiberSwitch(worker->fiber, fiber->context);

	// back on the scheduler.
	// we need to store everything about the fiber before we can unlock it
	// since right after that another worker will just grab it immediately.
	
	b32 finished = fiber->finished;
	i32 *unlock = worker->pending_unlock;

	worker->current_fiber = NULL;
	worker->pending_unlock = NULL;

	if (unlock)
		OS_UNIX_SpinLockRelease(unlock);

	if (finished)
		OS_UNIX_JobFiberReturn(fiber);
}

internal void OS_UNIX_JobSchedulerThreadEntry(void *param)
{
	OS_UNIX_JobWorker *worker = param;

	b32 is_main_thread = worker->id == 0;

	u32 starved_spins = 0;
	u64 starved_since_ms = 0;

	unix_job_thread_current_worker_internal_state = worker;
	worker->fiber = OS_UNIX_FiberConvertThread();

#ifdef OS_UNIX_PIN_WORKER_THREADS
	OS_UNIX_ThreadPinToCore(OS_UNIX_GetCurrentThreadHandle(), worker->id);
#endif
 
	while (OS_UNIX_AtomicLoadI32(&unix_job_sched->atomic_running))
	{
		// Check for a fiber to resume.
		OS_UNIX_JobFiber *waiting_fiber = OS_UNIX_JobTryGetWaitingFiber(is_main_thread);
		
		if (waiting_fiber)
		{
			OS_UNIX_JobRunFiber(worker, waiting_fiber);
			continue;
		}
 
		// Check for a new job to start.
		if (OS_UNIX_JobRequestAvailable(is_main_thread))
		{
			OS_UNIX_JobFiber *fiber = OS_UNIX_JobFiberFetchFree();
			
			if (!fiber)
			{
				// Spin until there is a free fiber.
				
				if (starved_spins % 5000 == 0)
				{
					u64 now = OS_UNIX_GetTicks();

					if (starved_since_ms == 0)
					{
						starved_since_ms = now;
					}
					else if ((now - starved_since_ms) > OS_UNIX_JOB_FIBER_SPIN_STARVE_ALERT_TIME_MS)
					{
						starved_since_ms = 0;
						
						DebugLogW(unix_job_sched->log_channel, "Starving, cannot get a free fiber for the job. Deadlock occuring p.");
					}
				}

				OS_UNIX_SPIN_PAUSE();
				
				starved_spins++;

				continue;
			}

			starved_spins = 0;
			starved_since_ms = 0;

			OS_UNIX_JobRequest request = {0};

			if (!OS_UNIX_JobTryGetRequest(is_main_thread, &request))
			{
				// another worker got here first, drop the fiber and start again.
				OS_UNIX_JobFiberReturn(fiber);
				continue;
			}
 
			// FINALLY!!!!
			
			fiber->EntryPoint = request.EntryPoint;
			fiber->param = request.param;
			fiber->priority	= request.priority;
			fiber->flags = request.flags;
			fiber->counter = request.counter;
			fiber->finished = false;

			OS_UNIX_JobRunFiber(worker, fiber);

			continue;
		}

		// Wait until we have more work to do...
		if (OS_UNIX_AtomicLoadI32(&unix_job_sched->atomic_spin_mode) || is_main_thread)
		{
			// main thread gets a free pass because
			// it can't sleep on a condition variable since
			// we gotta call OnMainThreadIdle.
			
			while (!OS_UNIX_JobRequestAvailable(is_main_thread) && OS_UNIX_AtomicLoadI32(&unix_job_sched->atomic_running))
			{
				if (unix_job_sched->OnMainThreadIdle && is_main_thread)
					unix_job_sched->OnMainThreadIdle(unix_job_sched->main_thread_idle_ctx);
				
				if (!OS_UNIX_AtomicLoadI32(&unix_job_sched->atomic_spin_mode))
					break;
				
				OS_UNIX_SPIN_PAUSE();
			}
		}
		else
		{
			// No spin mode so resort to regular mutex and
			// waiting on a condition variable.
			
			OS_UNIX_ThreadMutexLock(unix_job_sched->thread_mutex);
				
			while (!OS_UNIX_JobRequestAvailable(is_main_thread) && OS_UNIX_AtomicLoadI32(&unix_job_sched->atomic_running))
			{
				OS_UNIX_ThreadCondVarWait(unix_job_sched->thread_cond_begin, unix_job_sched->thread_mutex);
			}
				
			OS_UNIX_ThreadMutexUnlock(unix_job_sched->thread_mutex);
		}
	}

	unix_job_thread_current_worker_internal_state = NULL;
 
	OS_UNIX_FiberRevertThread();
}

internal void OS_UNIX_JobInit(OS_UNIX_JobScheduler *scheduler, LOG_Channel log_channel)
{
	unix_job_sched = scheduler;
	
	scheduler->log_channel = log_channel;
 
	scheduler->thread_mutex = OS_UNIX_ThreadMutexCreate();
	scheduler->thread_cond_begin = OS_UNIX_ThreadCondVarCreate();
 
	OS_UNIX_AtomicExchangeI32(&scheduler->atomic_running, 1);
	
	for (u32 j = 0; j < OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE; j++)
		scheduler->fallback_scratch_ring[j] = ArenaAlloc(OS_UNIX_JOB_FIBER_SCRATCH_SIZE);
 
	for (u32 i = 0; i < OS_UNIX_JOB_MAX_CONCURRENT_FIBERS; i++)
	{
		OS_UNIX_JobFiber *fiber = &scheduler->atomic_fiber_storage[i];

		// This is purely an aesthetic thing but since we push
		// the fibers to the front, the ones at the front get
		// selected first so we assign id's in reverse so at
		// the end the "front" fiber has id 0, then 1, etc...
		fiber->id = OS_UNIX_JOB_MAX_CONCURRENT_FIBERS - i - 1;

		fiber->context = OS_UNIX_FiberCreate(OS_UNIX_JOB_FIBER_STACK_SIZE, OS_UNIX_JobFiberEntry, scheduler);

		DebugLogAssert(log_channel, fiber->context, "Failed to allocate a fiber stack.");
 
		for (u32 j = 0; j < OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE; j++)
			fiber->scratch_arenas[j] = ArenaAlloc(OS_UNIX_JOB_FIBER_SCRATCH_SIZE);

		// Give the fiber to the freelist.
		OS_UNIX_JobFiberReturn(fiber);
	}

	// Try to leave at least one free core available.
	u32 cores = OS_UNIX_GetNumCores();
	const u32 desired_workers = (cores > 1) ? (cores - 1) : 1;

	scheduler->worker_count = MinValue(desired_workers, OS_UNIX_JOB_MAX_WORKERS);

	scheduler->workers[0].id = 0;
	scheduler->workers[0].thread_handle = OS_UNIX_GetCurrentThreadHandle();
 
	for (u32 i = 1; i < scheduler->worker_count; i++)
	{
		OS_UNIX_JobWorker *worker = &scheduler->workers[i];
		worker->id = i;
		worker->thread_handle = OS_UNIX_ThreadCreate(OS_UNIX_JobSchedulerThreadEntry, worker);
	}

	DebugLogI(scheduler->log_channel, "Initialized.");
}

internal void OS_UNIX_JobShutdown(void)
{
	// worker 0 is the main thread so no join needed.
	for (u32 i = 1; i < unix_job_sched->worker_count; i++)
		OS_UNIX_ThreadJoin(unix_job_sched->workers[i].thread_handle);
 
	// release fibers
	for (u32 i = 0; i < OS_UNIX_JOB_MAX_CONCURRENT_FIBERS; i++)
	{
		for (u32 j = 0; j < OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE; j++)
			ArenaRelease(&unix_job_sched->atomic_fiber_storage[i].scratch_arenas[j]);

		OS_UNIX_FiberDelete(unix_job_sched->atomic_fiber_storage[i].context);
	}
	
	for (u32 j = 0; j < OS_UNIX_JOB_FIBER_SCRATCH_RING_SIZE; j++)
		ArenaRelease(&unix_job_sched->fallback_scratch_ring[j]);

	OS_UNIX_ThreadMutexDestroy(unix_job_sched->thread_mutex);
	OS_UNIX_ThreadCondVarDestroy(unix_job_sched->thread_cond_begin);

	DebugLogI(unix_job_sched->log_channel, "Destroyed.");

	unix_job_sched = NULL;
}

internal void OS_UNIX_JobEnter(void (*OnMainThreadIdle)(void *ctx), void *main_thread_idle_ctx)
{
	unix_job_sched->OnMainThreadIdle = OnMainThreadIdle;
	unix_job_sched->main_thread_idle_ctx = main_thread_idle_ctx;
	
	OS_UNIX_JobSchedulerThreadEntry(&unix_job_sched->workers[0]);
}

internal void OS_UNIX_JobHalt(void)
{
	OS_UNIX_ThreadMutexLock(unix_job_sched->thread_mutex);
	{
		OS_UNIX_AtomicExchangeI32(&unix_job_sched->atomic_running, 0);
		OS_UNIX_ThreadCondVarBroadcast(unix_job_sched->thread_cond_begin);
	}
	OS_UNIX_ThreadMutexUnlock(unix_job_sched->thread_mutex);
}

internal void OS_UNIX_Log(LOG_Level level, LOG_Channel channel,
						  const char *file, i32 line, const char *fn,
						  const char *fmt, ...)
{
	OS_UNIX_JobContext job_context = OS_UNIX_JobGetContext();
	
	va_list args;
	va_start(args, fmt);
	OS_UNIX_LoggerWriteV(job_context, level, channel, file, line, fn, fmt, args);
	va_end(args);
}

internal LOG_Channel OS_UNIX_LogChannelOpen(String8 name)
{
	return OS_UNIX_LoggerOpenChannel(name);
}

internal LOG_Channel OS_UNIX_LogChannelOpenFrom(LOG_Channel parent, String8 name)
{
	return OS_UNIX_LoggerOpenChannelFrom(parent, name);
}

internal void OS_UNIX_LogChannelClose(LOG_Channel channel)
{
	OS_UNIX_LoggerCloseChannel(channel);
}

internal OS_Handle OS_UNIX_JobCounterAlloc(i32 initial_count)
{
	OS_UNIX_Object *counter = OS_UNIX_AllocObject();

	OS_UNIX_JobCounterInit(&counter->counter, initial_count);
	
	OS_Handle handle = { counter };
	return handle;
}

internal void OS_UNIX_JobCounterRelease(OS_Handle handle)
{
	OS_UNIX_Object *obj = handle.value;
	
	OS_UNIX_ReturnObject(obj);
}

internal void OS_UNIX_JobCounterInc(OS_Handle handle, i32 amount)
{
	OS_UNIX_Object *obj = handle.value;
	
	OS_UNIX_JobCounterIncrement(&obj->counter, amount);
}

internal void OS_UNIX_JobCounterDec(OS_Handle handle, i32 amount)
{
	OS_UNIX_Object *obj = handle.value;
	
	OS_UNIX_JobCounterDecrement(&obj->counter, amount);
}

internal i32 OS_UNIX_JobCounterValue(OS_Handle handle)
{
	OS_UNIX_Object *obj = handle.value;
	
	return OS_UNIX_JobCounterRead(&obj->counter);
}

internal void OS_UNIX_JobYield(OS_Handle handle, i32 value)
{
	OS_UNIX_Object *obj = handle.value;
	
	OS_UNIX_JobYieldOnCounter(&obj->counter, value);
}

internal void OS_UNIX_JobKick(const J_Decl *decl, OS_Handle counter_handle)
{
	OS_UNIX_Object *obj = counter_handle.value;
	OS_UNIX_JobKickRaw(decl, obj ? &obj->counter : NULL);
}

internal void OS_UNIX_JobBatch(const J_Decl *decls, u32 count, OS_Handle counter_handle)
{
	OS_UNIX_Object *obj = counter_handle.value;
	
	OS_UNIX_JobBatchRaw(decls, count, obj ? &obj->counter : NULL);
}

internal OS_Handle OS_UNIX_FiberMutexCreate(void)
{
	OS_UNIX_Object *mtx = OS_UNIX_AllocObject();
	
	OS_Handle handle = { mtx };
	return handle;
}

internal void OS_UNIX_FiberMutexDestroy(OS_Handle handle)
{
	OS_UNIX_Object *mtx = handle.value;
	
	OS_UNIX_ReturnObject(mtx);
}

internal void OS_UNIX_FiberMutexLock(OS_Handle handle)
{
	OS_UNIX_Object *mtx = handle.value;
	
	OS_UNIX_JobFiberMutexLock(&mtx->fmutex);
}

internal void OS_UNIX_FiberMutexUnlock(OS_Handle handle)
{
	OS_UNIX_Object *mtx = handle.value;
	
	OS_UNIX_JobFiberMutexUnlock(&mtx->fmutex);
}

internal OS_Handle OS_UNIX_FiberCondVarCreate(void)
{
	OS_UNIX_Object *cv = OS_UNIX_AllocObject();
	
	OS_Handle handle = { cv };
	return handle;
}

internal void OS_UNIX_FiberCondVarDestroy(OS_Handle handle)
{
	OS_UNIX_Object *cv = handle.value;
	
	OS_UNIX_ReturnObject(cv);
}

internal void OS_UNIX_FiberCondVarWait(OS_Handle handle, OS_Handle fiber_mutex_handle)
{
	OS_UNIX_Object *cv = handle.value;
	OS_UNIX_Object *mtx = fiber_mutex_handle.value;
	
	OS_UNIX_JobFiberCondVarWait(&cv->fcondvar, &mtx->fmutex);
}

internal void OS_UNIX_FiberCondVarSignal(OS_Handle handle)
{
	OS_UNIX_Object *cv = handle.value;
	
	OS_UNIX_JobFiberCondVarSignal(&cv->fcondvar);
}

internal void OS_UNIX_FiberCondVarBroadcast(OS_Handle handle)
{
	OS_UNIX_Object *cv = handle.value;
	
	OS_UNIX_JobFiberCondVarBroadcast(&cv->fcondvar);
}

internal void OS_UNIX_BindAPI(OS_API *api)
{
	api->VirtualReserve              = OS_UNIX_VirtualReserve;
	api->VirtualRelease              = OS_UNIX_VirtualRelease;
	api->VirtualCommit               = OS_UNIX_VirtualCommit;
	api->VirtualDecommit             = OS_UNIX_VirtualDecommit;

	api->HeapAlloc                   = OS_UNIX_HeapAlloc;
	api->HeapFree                    = OS_UNIX_HeapFree;
	api->HeapRealloc                 = OS_UNIX_HeapRealloc;
	
	api->GetPageSize                 = OS_UNIX_GetPageSize;

	api->Log                         = OS_UNIX_Log;
	api->LogChannelOpen              = OS_UNIX_LogChannelOpen;
	api->LogChannelOpenFrom          = OS_UNIX_LogChannelOpenFrom;
	api->LogChannelClose             = OS_UNIX_LogChannelClose;

	api->SetWindowTitle              = OS_UNIX_SetWindowTitle;
	api->GetWindowSize               = OS_UNIX_GetWindowSize;
	api->GetWindowSizeInPixels       = OS_UNIX_GetWindowSizeInPixels;
	api->SetWindowSize               = OS_UNIX_SetWindowSize;
	api->SetWindowFullscreen         = OS_UNIX_SetWindowFullscreen;
	api->SetWindowBorderless         = OS_UNIX_SetWindowBorderless;
	api->SetWindowOpacity            = OS_UNIX_SetWindowOpacity;

	api->SetMousePosition            = OS_UNIX_SetMousePosition;
	api->SetMouseVisible             = OS_UNIX_SetMouseVisible;
	api->IsMouseVisible              = OS_UNIX_IsMouseVisible;
	api->SetMouseLocked              = OS_UNIX_SetMouseLocked;
	api->IsMouseLocked               = OS_UNIX_IsMouseLocked;

	api->GetTicks                    = OS_UNIX_GetTicks;
	api->GetPerformanceCounter       = OS_UNIX_GetPerformanceCounter;
	api->GetPerformanceFrequency     = OS_UNIX_GetPerformanceFrequency;

	api->GetNumCores                 = OS_UNIX_GetNumCores;

	api->TLSAlloc                    = OS_UNIX_TLSAlloc;
	api->TLSFree                     = OS_UNIX_TLSFree;
	api->TLSGet                      = OS_UNIX_TLSGet;
	api->TLSSet                      = OS_UNIX_TLSSet;
	
	api->AtomicCompareExchangeI32    = OS_UNIX_AtomicCompareExchangeI32;
	api->AtomicCompareExchangeI64    = OS_UNIX_AtomicCompareExchangeI64;
	api->AtomicCompareExchangePtr    = OS_UNIX_AtomicCompareExchangePtr;
	
	api->AtomicLoadI32               = OS_UNIX_AtomicLoadI32;
	api->AtomicLoadI64               = OS_UNIX_AtomicLoadI64;
	api->AtomicLoadPtr               = OS_UNIX_AtomicLoadPtr;

	api->AtomicStoreI32              = OS_UNIX_AtomicStoreI32;
	api->AtomicStoreI64              = OS_UNIX_AtomicStoreI64;
	api->AtomicStorePtr              = OS_UNIX_AtomicStorePtr;

	api->AtomicExchangeI32           = OS_UNIX_AtomicExchangeI32;
	api->AtomicExchangeI64           = OS_UNIX_AtomicExchangeI64;
	api->AtomicExchangePtr           = OS_UNIX_AtomicExchangePtr;
	
	api->AtomicCompareExchangeI32    = OS_UNIX_AtomicCompareExchangeI32;
	api->AtomicCompareExchangeI64    = OS_UNIX_AtomicCompareExchangeI64;
	api->AtomicCompareExchangePtr    = OS_UNIX_AtomicCompareExchangePtr;

	api->AtomicFetchAddI32           = OS_UNIX_AtomicFetchAddI32;
	api->AtomicFetchAddI64           = OS_UNIX_AtomicFetchAddI64;
	
	api->SpinLockAcquire             = OS_UNIX_SpinLockAcquire;
	api->SpinLockRelease             = OS_UNIX_SpinLockRelease;

	api->FiberMutexCreate            = OS_UNIX_FiberMutexCreate;
	api->FiberMutexDestroy           = OS_UNIX_FiberMutexDestroy;
	api->FiberMutexLock              = OS_UNIX_FiberMutexLock;
	api->FiberMutexUnlock            = OS_UNIX_FiberMutexUnlock;
	
	api->FiberCondVarCreate          = OS_UNIX_FiberCondVarCreate;
	api->FiberCondVarDestroy         = OS_UNIX_FiberCondVarDestroy;
	api->FiberCondVarWait            = OS_UNIX_FiberCondVarWait;
	api->FiberCondVarSignal          = OS_UNIX_FiberCondVarSignal;
	api->FiberCondVarBroadcast       = OS_UNIX_FiberCondVarBroadcast;

	api->ThreadMutexCreate           = OS_UNIX_ThreadMutexCreate;
	api->ThreadMutexDestroy          = OS_UNIX_ThreadMutexDestroy;
	api->ThreadMutexLock             = OS_UNIX_ThreadMutexLock;
	api->ThreadMutexUnlock           = OS_UNIX_ThreadMutexUnlock;
	
	api->ThreadCondVarCreate         = OS_UNIX_ThreadCondVarCreate;
	api->ThreadCondVarDestroy        = OS_UNIX_ThreadCondVarDestroy;
	api->ThreadCondVarWait           = OS_UNIX_ThreadCondVarWait;
	api->ThreadCondVarSignal         = OS_UNIX_ThreadCondVarSignal;
	api->ThreadCondVarBroadcast      = OS_UNIX_ThreadCondVarBroadcast;

	api->FileDelete                  = OS_UNIX_FileDelete;
	api->FileExists                  = OS_UNIX_FileExists;
	api->GetFileLastWriteTime        = OS_UNIX_GetFileLastWriteTime;

	api->DirectoryCreate             = OS_UNIX_DirectoryCreate;
	api->DirectoryDelete             = OS_UNIX_DirectoryDelete;
	api->DirectoryExists             = OS_UNIX_DirectoryExists;

	api->StreamFromFile              = OS_UNIX_StreamFromFile;
	api->StreamFromMemory            = OS_UNIX_StreamFromMemory;
	api->StreamFromConstMemory       = OS_UNIX_StreamFromConstMemory;

	api->StreamRead                  = OS_UNIX_StreamRead;
	api->StreamWrite                 = OS_UNIX_StreamWrite;
	api->StreamSeek                  = OS_UNIX_StreamSeek;
	api->StreamSize                  = OS_UNIX_StreamSize;
	api->StreamPosition              = OS_UNIX_StreamPosition;
	api->StreamClose                 = OS_UNIX_StreamClose;

	api->JobCounterAlloc             = OS_UNIX_JobCounterAlloc;
	api->JobCounterRelease           = OS_UNIX_JobCounterRelease;
	api->JobCounterInc               = OS_UNIX_JobCounterInc;
	api->JobCounterDec               = OS_UNIX_JobCounterDec;
	api->JobCounterValue             = OS_UNIX_JobCounterValue;
	api->JobYield                    = OS_UNIX_JobYield;
	api->JobKick                     = OS_UNIX_JobKick;
	api->JobBatch                    = OS_UNIX_JobBatch;
	api->JobFor                      = OS_UNIX_JobFor;
	api->JobIsMainThread             = OS_UNIX_JobIsMainThread;
	api->JobGetScratch               = OS_UNIX_JobGetScratch;

	api->OpenInExplorer              = OS_UNIX_OpenInExplorer;

	api->VulkanSurfaceCreate         = OS_UNIX_VulkanSurfaceCreate;
	api->VulkanSurfaceDestroy        = OS_UNIX_VulkanSurfaceDestroy;
	api->VulkanGetInstanceExtensions = OS_UNIX_VulkanGetInstanceExtensions;
}

void *OS_UNIX_EntryInitStub(const OS_API *api) { return NULL; }
void OS_UNIX_EntryDestroyStub(void *ctx) { }
b32 OS_UNIX_EntryTickStub(void *ctx, const OS_InputState *input) { return false; }
void OS_UNIX_EntryHotLoadStub(void *ctx, const OS_API *api) { }
void OS_UNIX_EntryHotUnloadStub(void *ctx) { }

internal void OS_UNIX_UnloadCode(void)
{
	unix_st.code.Init      = OS_UNIX_EntryInitStub;
	unix_st.code.Destroy   = OS_UNIX_EntryDestroyStub;
	unix_st.code.Tick      = OS_UNIX_EntryTickStub;
	unix_st.code.HotLoad   = OS_UNIX_EntryHotLoadStub;
	unix_st.code.HotUnload = OS_UNIX_EntryHotUnloadStub;

	if (unix_st.code.lib)
	{
		dlclose(unix_st.code.lib);
		unix_st.code.lib = NULL;
	}

	if (unix_st.code.hot_path[0])
	{
		unlink(unix_st.code.hot_path);
		unix_st.code.hot_path[0] = '\0';
	}
}

internal void OS_UNIX_LoadCode(const char *module_path)
{
	OS_UNIX_UnloadCode();

	unix_st.code.last_write_time = OS_UNIX_GetFileLastWriteTimeInternal(module_path);

	if (unix_st.code.last_write_time == 0)
	{
		DebugLogE(unix_st.log_channel, "Couldn't find hot module \"%s\".", module_path);
		return;
	}

	// we have to generate new paths because reusing an old path could hand
	// back the old library due to some internal optimisation shenanigans.
	snprintf(unix_st.code.hot_path, sizeof(unix_st.code.hot_path), OS_UNIX_CODE_HOT_FMT, unix_st.code.generation++);

	if (!SDL_CopyFile(module_path, unix_st.code.hot_path))
	{
		DebugLogE(unix_st.log_channel, "Failed to copy \"%s\" to \"%s\": %s", module_path, unix_st.code.hot_path, SDL_GetError());
		unix_st.code.hot_path[0] = '\0';
		return;
	}

	unix_st.code.lib = dlopen(unix_st.code.hot_path, RTLD_NOW | RTLD_LOCAL);

	if (!unix_st.code.lib)
	{
		DebugLogE(unix_st.log_channel, "dlopen failed: \"%s\"", dlerror());
		OS_UNIX_UnloadCode();
		return;
	}

	OS_EntryInitFn      *Init      = (OS_EntryInitFn      *)dlsym(unix_st.code.lib, "MagpieInit");
	OS_EntryDestroyFn   *Destroy   = (OS_EntryDestroyFn   *)dlsym(unix_st.code.lib, "MagpieDestroy");
	OS_EntryTickFn      *Tick      = (OS_EntryTickFn      *)dlsym(unix_st.code.lib, "MagpieTick");
	OS_EntryHotLoadFn   *HotLoad   = (OS_EntryHotLoadFn   *)dlsym(unix_st.code.lib, "MagpieHotLoad");
	OS_EntryHotUnloadFn *HotUnload = (OS_EntryHotUnloadFn *)dlsym(unix_st.code.lib, "MagpieHotUnload");

	if (!Init || !Destroy || !Tick || !HotLoad || !HotUnload)
	{
		DebugLogE(unix_st.log_channel, "Hot module is missing one or more Magpie...() entry points:");
		DebugLogE(unix_st.log_channel, " MagpieInit: %p", Init);
		DebugLogE(unix_st.log_channel, " MagpieDestroy: %p", Destroy);
		DebugLogE(unix_st.log_channel, " MagpieTick: %p", Tick);
		DebugLogE(unix_st.log_channel, " MagpieHotLoad: %p", HotLoad);
		DebugLogE(unix_st.log_channel, " MagpieHotUnload: %p", HotUnload);

		OS_UNIX_UnloadCode();
		
		return;
	}

	unix_st.code.Init      = Init;
	unix_st.code.Destroy   = Destroy;
	unix_st.code.Tick      = Tick;
	unix_st.code.HotLoad   = HotLoad;
	unix_st.code.HotUnload = HotUnload;
}

internal b32 OS_UNIX_CodeNeedsReload(void)
{
	// the linker writes the output over a stretch of time, so
	// reacting to the first mtime change means copying a half-written
	// .so, so while it's being relinked the file can briefly not exist at all.
	
	OS_UNIX_Code *code = &unix_st.code;

	u64 mtime = OS_UNIX_GetFileLastWriteTimeInternal(OS_UNIX_CODE_PATH);

	if (mtime == 0 || mtime == code->last_write_time)
	{
		code->pending_write_time = 0;
		return false;
	}

	u64 now = SDL_GetTicks();

	if (mtime != code->pending_write_time)
	{
		code->pending_write_time = mtime;
		code->pending_since_ms = now;
		return false;
	}

	return (now - code->pending_since_ms) >= 100;
}

internal void OS_UNIX_InitImGui(void)
{
#if 0
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplSDL3_InitForVulkan(unix_st.sdl_window);
#endif
}

internal void OS_UNIX_DestroyImGui(void)
{
#if 0
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
#endif
}

internal void OS_UNIX_ImGuiNewFrame(void)
{
	// TODO
}

internal void OS_UNIX_CloseAllGamepads(void)
{
	for (u32 i = 0; i < unix_st.gamepad_count; i++)
	{
		SDL_CloseGamepad(unix_st.gamepads[i]);
		unix_st.gamepads[i] = NULL;
	}

	unix_st.gamepad_count = 0;
}

internal void OS_UNIX_ReconnectAllGamepads(void)
{
	if (unix_st.gamepad_count > 0)
		OS_UNIX_CloseAllGamepads();

	i32 sdl_gp_count = 0;
	
	SDL_JoystickID *ids = SDL_GetGamepads(&sdl_gp_count);

	if (!ids)
		return;

	AssertTrue(sdl_gp_count >= 0);

	if (sdl_gp_count > OS_MAX_GAMEPADS)
		sdl_gp_count = OS_MAX_GAMEPADS;
	
	unix_st.gamepad_count = sdl_gp_count;
	
	for (u32 i = 0; i < unix_st.gamepad_count; i++)
	{
		unix_st.gamepads[i] = SDL_OpenGamepad(ids[i]);

		if (unix_st.gamepads[i])
			DebugLogD(unix_st.log_channel, "Added gamepad with player index: %d.", SDL_GetGamepadPlayerIndex(unix_st.gamepads[i]));
		else
			DebugLogD(unix_st.log_channel, "Failed to open gamepad %u: %s", (u32)ids[i], SDL_GetError());
	}

	SDL_free(ids);
}

internal void OS_UNIX_MessagePump(void *)
{
	SDL_Event local_events[OS_UNIX_MAX_PENDING_EVENTS];
	u32 local_event_count = 0;

	SDL_Event ev;

	while (local_event_count < ArraySize(local_events) && SDL_PollEvent(&ev))
		local_events[local_event_count++] = ev;

	if (local_event_count > 0)
	{
		OS_UNIX_ThreadMutexLock(unix_st.event_mutex);
		{
			DebugLogAssert(unix_st.log_channel, unix_st.pending_event_count <= OS_UNIX_MAX_PENDING_EVENTS, "Ran out of event buffer space SHIT.");
			
			u32 available_space = OS_UNIX_MAX_PENDING_EVENTS - unix_st.pending_event_count;
			u32 copy_count = (local_event_count > available_space) ? available_space : local_event_count;

			MemCopy(unix_st.pending_events + unix_st.pending_event_count, local_events, copy_count * sizeof(SDL_Event));
			unix_st.pending_event_count += copy_count;
		}
		OS_UNIX_ThreadMutexUnlock(unix_st.event_mutex);
	}
}

internal OS_GamepadState *OS_UNIX_GamepadForID(OS_InputState *input, SDL_JoystickID id)
{
	i32 index = SDL_GetGamepadPlayerIndexForID(id);

	if (index < 0 || index >= (i32)OS_MAX_GAMEPADS)
		return NULL;

	return &input->gamepads[index];
}

internal OS_InputState OS_UNIX_ProcessEvents(OS_InputState prev_state)
{
	OS_InputState input_out = prev_state;

	OS_UNIX_MessagePump(NULL);
	
	SDL_Event events[OS_UNIX_MAX_PENDING_EVENTS];
	u32 event_count = 0;

	OS_UNIX_ThreadMutexLock(unix_st.event_mutex);
	{
		MemCopy(events, unix_st.pending_events, unix_st.pending_event_count * sizeof(SDL_Event));
		event_count = unix_st.pending_event_count;
		unix_st.pending_event_count = 0;
	}
	OS_UNIX_ThreadMutexUnlock(unix_st.event_mutex);

	input_out.mouse_delta = v2x(0.f);
	input_out.mouse_wheel = v2x(0.f);

	for (u32 i = 0; i < event_count; i++)
	{
		const SDL_Event *ev = &events[i];

		//ImGui_ImplSDL3_ProcessEvent(ev);

		switch (ev->type)
		{
			case SDL_EVENT_QUIT:
				OS_UNIX_JobHalt();
				break;

			case SDL_EVENT_KEY_DOWN:
				if ((u32)ev->key.scancode < ArraySize(input_out.kb_down))
					input_out.kb_down[ev->key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				if ((u32)ev->key.scancode < ArraySize(input_out.kb_down))
					input_out.kb_down[ev->key.scancode] = false;
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if ((u32)ev->button.button < ArraySize(input_out.mb_down))
					input_out.mb_down[ev->button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				if ((u32)ev->button.button < ArraySize(input_out.mb_down))
					input_out.mb_down[ev->button.button] = false;
				break;

			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetGlobalMouseState(&input_out.mouse_screen_position.x, &input_out.mouse_screen_position.y);
				input_out.mouse_position = v2(ev->motion.x, ev->motion.y);
				input_out.mouse_delta = V2Add(input_out.mouse_delta, v2(ev->motion.xrel, ev->motion.yrel));
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				input_out.mouse_wheel = V2Add(input_out.mouse_wheel, v2(ev->wheel.x, ev->wheel.y));
				break;
					
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				{
					OS_GamepadState *gp = OS_UNIX_GamepadForID(&input_out, ev->gbutton.which);

					if (gp && (u32)ev->gbutton.button < ArraySize(gp->down))
						gp->down[ev->gbutton.button] = true;
				}
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				{
					OS_GamepadState *gp = OS_UNIX_GamepadForID(&input_out, ev->gbutton.which);

					if (gp && (u32)ev->gbutton.button < ArraySize(gp->down))
						gp->down[ev->gbutton.button] = false;
				}
				break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				{
					OS_GamepadState *gp = OS_UNIX_GamepadForID(&input_out, ev->gaxis.which);

					if (!gp)
						break;

					OS_GamepadAxis axis = (OS_GamepadAxis)ev->gaxis.axis;

					f32 normalized_value = (ev->gaxis.value < 0)
						? (f32)ev->gaxis.value / 32768.f
						: (f32)ev->gaxis.value / (f32)SDL_JOYSTICK_AXIS_MAX;
				
					OS_GamepadStateSetAxisValue(gp, axis, normalized_value);
				}
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				OS_UNIX_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				DebugLogD(unix_st.log_channel, "Removed Gamepad.");
				OS_UNIX_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMAPPED:
				SDL_ReloadGamepadMappings();
				OS_UNIX_ReconnectAllGamepads();
				break;

			default:
				break;
		}
	}

	for (u32 i = 0; i < ArraySize(input_out.kb_down); i++)
	{
		input_out.kb_pressed[i] = input_out.kb_down[i] && !prev_state.kb_down[i];
		input_out.kb_released[i] = !input_out.kb_down[i] && prev_state.kb_down[i];
	}

	for (u32 i = 0; i < ArraySize(input_out.mb_down); i++)
	{
		input_out.mb_pressed[i] = input_out.mb_down[i] && !prev_state.mb_down[i];
		input_out.mb_released[i] = !input_out.mb_down[i] && prev_state.mb_down[i];
	}

	for (u32 p = 0; p < OS_MAX_GAMEPADS; p++)
	{
		for (u32 i = 0; i < ArraySize(input_out.gamepads[p].down); i++)
		{
			input_out.gamepads[p].pressed[i] = input_out.gamepads[p].down[i] && !prev_state.gamepads[p].down[i];
			input_out.gamepads[p].released[i] = !input_out.gamepads[p].down[i] && prev_state.gamepads[p].down[i];
		}
	}

	return input_out;
}

internal J_ENTRY_POINT_DEF(OS_UNIX_FrameJobEntry)
{
	if (OS_UNIX_CodeNeedsReload())
	{
		DebugLogI(unix_st.log_channel, "Attempting Hot Reload...");

		unix_st.code.HotUnload(unix_st.app);

		OS_UNIX_UnloadCode();
		OS_UNIX_LoadCode(OS_UNIX_CODE_PATH);

		unix_st.code.HotLoad(unix_st.app, &unix_st.api);

		DebugLogI(unix_st.log_channel, "Hot Reloaded!");
	}

	static OS_InputState prev_input_st = {0};
	OS_InputState curr_input_st = OS_UNIX_ProcessEvents(prev_input_st);
	prev_input_st = curr_input_st;

	OS_UNIX_ImGuiNewFrame();
	
	// ---
	
	if (unix_st.code.Tick(unix_st.app, &curr_input_st))
	{
		unix_st.code.Destroy(unix_st.app);
		OS_UNIX_JobHalt();
		return;
	}
	
	J_Decl next_frame_job = {0};
	next_frame_job.EntryPoint = OS_UNIX_FrameJobEntry;
	next_frame_job.priority = J_Priority_Normal;
	next_frame_job.flags = J_Flag_MainThreadOnly;

	OS_UNIX_JobKickRaw(&next_frame_job, NULL);
}

internal J_ENTRY_POINT_DEF(OS_UNIX_RootJobEntry)
{
	unix_st.app = unix_st.code.Init(&unix_st.api);

	J_Decl frame_job = {0};
	frame_job.EntryPoint = OS_UNIX_FrameJobEntry;
	frame_job.priority = J_Priority_Normal;
	frame_job.flags = J_Flag_MainThreadOnly;

	OS_UNIX_JobKickRaw(&frame_job, NULL);
}

int main(int argc, char **argv)
{
	OS_UNIX_QuerySystemInfo();

	SDL_InitFlags init_flags =
		SDL_INIT_VIDEO |
		SDL_INIT_AUDIO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMEPAD |
		SDL_INIT_HAPTIC |
		SDL_INIT_EVENTS |
		SDL_INIT_SENSOR |
		SDL_INIT_CAMERA;

	if (!SDL_Init(init_flags))
	{
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return -1;
	}

	SDL_WindowFlags window_flags =
		SDL_WINDOW_VULKAN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY;

	unix_st.sdl_window = SDL_CreateWindow(OS_DEFAULT_WINDOW_TITLE,
										  OS_DEFAULT_WINDOW_WIDTH,
										  OS_DEFAULT_WINDOW_HEIGHT,
										  window_flags);

	if (!unix_st.sdl_window)
	{
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	OS_UNIX_BindAPI(&unix_st.api);

	osapi = &unix_st.api;

	unix_st.arena = ArenaAlloc(OS_LAYER_MEMORY);

	OS_UNIX_LoggerInit(&unix_st.logger, String8Lit("log_output.txt"));

	unix_st.log_channel = OS_UNIX_LogChannelOpen(String8Lit("UNIX"));

	DebugLogI(unix_st.log_channel, "Initializing ImGui...");
	
	OS_UNIX_InitImGui();

	DebugLogI(unix_st.log_channel, "Loading hot module...");

	OS_UNIX_LoadCode(OS_UNIX_CODE_PATH);
	
	unix_st.pending_events = ArenaPushArray(&unix_st.arena, SDL_Event, OS_UNIX_MAX_PENDING_EVENTS);
	
	unix_st.event_mutex = OS_UNIX_ThreadMutexCreate();

	OS_UNIX_JobInit(&unix_st.sched, OS_UNIX_LogChannelOpenFrom(unix_st.log_channel, String8Lit("JOB")));

	J_Decl root_job = {0};
	root_job.EntryPoint = OS_UNIX_RootJobEntry;
	root_job.priority = J_Priority_Normal;
	root_job.flags = J_Flag_MainThreadOnly;

	OS_UNIX_JobKickRaw(&root_job, NULL);
	
	OS_UNIX_JobEnter(OS_UNIX_MessagePump, NULL);

	OS_UNIX_JobShutdown();
	
	OS_UNIX_UnloadCode();
	
	OS_UNIX_ThreadMutexDestroy(unix_st.event_mutex);
	
	OS_UNIX_LoggerShutdown();

	OS_UNIX_DestroyImGui();

	ArenaRelease(&unix_st.arena);

	SDL_DestroyWindow(unix_st.sdl_window);
	
	SDL_Quit();
	
	return 0;
}
