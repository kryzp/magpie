#ifndef OS_WIN32_J_H
#define OS_WIN32_J_H

/*
 * Fiber-Driven Job System.
 * - N worker threads, each run a scheduler loop.
 * - A pool of fibers executes jobs, fibers loop forever and
 *   get re-used via a pool.
 * - Seperate job queue per-priority level.
 * - Don't bother with work-stealing but that might be
 *   something to look into in the future!
 *
 * Resources:
 * - "Parallelizing the Naughty Dog Engine Using Fibers" - Christian Gyrling
 * - "Parallelizing the Physics Solver" - Dennis Gustafsson
 */

#define OS_W32_J_MAX_JOBS_PER_QUEUE         512
#define OS_W32_J_MAX_CONCURRENT_FIBERS      128
#define OS_W32_J_MAX_WORKERS                32
#define OS_W32_J_COUNTER_MAX_WAITING        64
#define OS_W32_J_FIBER_SCRATCH_SIZE         Megabytes(8)
#define OS_W32_J_FIBER_SCRATCH_RING_SIZE    2

typedef struct OS_W32_J_Context OS_W32_J_Context;
struct OS_W32_J_Context
{
	u32 worker_id;
	i32 fiber_id;
};

typedef struct OS_W32_J_Fiber OS_W32_J_Fiber;

typedef struct OS_W32_J_Counter OS_W32_J_Counter;
struct OS_W32_J_Counter
{
	u32 atomic_count;
	u32 atomic_spinlock;

	// Fibers that are blocked waiting on this counter.
	u32 waiting_count;
	OS_W32_J_Fiber *waiting[OS_W32_J_COUNTER_MAX_WAITING];
};

typedef struct OS_W32_J_Fiber OS_W32_J_Fiber;
struct OS_W32_J_Fiber
{
	u32 id;
	
	OS_W32_J_Fiber *next_free;

	OS_Handle handle;

	J_EntryPointFn *EntryPoint;
	void *param;
	
	J_Priority priority;

	OS_W32_J_Counter *counter;
	
	b32 finished;

	Arena scratch_arenas[OS_W32_J_FIBER_SCRATCH_RING_SIZE];
};

typedef struct OS_W32_J_Request OS_W32_J_Request;
struct OS_W32_J_Request
{
	J_EntryPointFn *EntryPoint;
	void *param;
	J_Priority priority;
	J_Flags flags;
	OS_W32_J_Counter *counter;
};

typedef struct OS_W32_J_Queue OS_W32_J_Queue;
struct OS_W32_J_Queue
{
	u32 atomic_spinlock;
	
	OS_W32_J_Request requests[OS_W32_J_MAX_JOBS_PER_QUEUE];
	OS_W32_J_Fiber *waiting[OS_W32_J_MAX_JOBS_PER_QUEUE];

	u32 atomic_taken_task_count;
	u32 atomic_added_task_count;
	
	u32 atomic_taken_waiting_count;
	u32 atomic_added_waiting_count;
};

typedef struct OS_W32_J_Scheduler OS_W32_J_Scheduler;

typedef struct OS_W32_J_Worker OS_W32_J_Worker;
struct OS_W32_J_Worker
{
	u32 id;
	
	OS_Handle thread_handle;
	OS_Handle fiber_handle; // The scheduler fiber for this current worker thread.

	OS_W32_J_Fiber *current_fiber; // The fiber currently executing on this worker.

	// you know what fuck you.
	OS_W32_J_Scheduler *scheduler;
};

typedef struct OS_W32_J_Scheduler OS_W32_J_Scheduler;
struct OS_W32_J_Scheduler
{
	LOG_Channel log_channel;
	
	u32 atomic_running;
	u32 atomic_spin_mode;

	OS_Handle mutex;
	OS_Handle cond_begin;

	OS_W32_J_Queue main_thread_queue;
	OS_W32_J_Queue queues[J_Priority_COUNT];
	
	Arena fallback_scratch_ring[OS_W32_J_FIBER_SCRATCH_RING_SIZE];
	
	u32 worker_count;
	OS_W32_J_Worker workers[OS_W32_J_MAX_WORKERS];

	OS_W32_J_Fiber atomic_fiber_storage[OS_W32_J_MAX_CONCURRENT_FIBERS];

	u32 fiber_pool_spinlock;
	OS_W32_J_Fiber *fiber_pool_head;
	
	void (*OnMainThreadIdle)(void *ctx);
	void *main_thread_idle_ctx;

	//u32 tls_worker_slot;
};


/* ==================================================
   HELPERS
   ================================================== */

static void                OS_W32_J_SpinModeEnable          (OS_W32_J_Scheduler *scheduler);
static void                OS_W32_J_SpinModeDisable         (OS_W32_J_Scheduler *scheduler);

static b32                 OS_W32_J_IsMainThread            (OS_W32_J_Scheduler *scheduler);

static void                OS_W32_J_FiberYield              (OS_W32_J_Scheduler *scheduler);
static void                OS_W32_J_FiberCompleted          (OS_W32_J_Scheduler *scheduler);
static OS_W32_J_Fiber   *OS_W32_J_FiberFetchFree          (OS_W32_J_Scheduler *scheduler);
static void                OS_W32_J_FiberReturn             (OS_W32_J_Scheduler *scheduler, OS_W32_J_Fiber *fiber);
static OS_Handle           OS_W32_J_GetCurrentFiberHandle   (OS_W32_J_Scheduler *scheduler);

static OS_W32_J_Request *OS_W32_J_TryGetRequest           (OS_W32_J_Scheduler *scheduler);
static OS_W32_J_Fiber   *OS_W32_J_TryGetWaitingFiber      (OS_W32_J_Scheduler *scheduler);

static b32                 OS_W32_J_RequestAvailable        (OS_W32_J_Scheduler *scheduler);


/* ==================================================
   CORE
   ================================================== */

static void OS_W32_J_Init     (OS_W32_J_Scheduler *scheduler, LOG_Channel log_channel);
static void OS_W32_J_Shutdown (OS_W32_J_Scheduler *scheduler);

static void OS_W32_J_SchedulerThreadEntry(void *param);
static void OS_W32_J_FiberEntry(void *param);

static void OS_W32_J_Enter (OS_W32_J_Scheduler *scheduler, void (*OnMainThreadIdle)(void *ctx), void *main_thread_idle_ctx);
static void OS_W32_J_Halt  (OS_W32_J_Scheduler *scheduler);

static OS_W32_J_Context OS_W32_J_GetContext(OS_W32_J_Scheduler *scheduler);

/* ==================================================
   COUNTER
   ================================================== */

static void OS_W32_J_CounterInit      (OS_W32_J_Counter *counter, u32 initial_count);
static void OS_W32_J_CounterIncrement (OS_W32_J_Counter *counter, u32 n);
static void OS_W32_J_CounterDecrement (OS_W32_J_Scheduler *scheduler, OS_W32_J_Counter *counter, u32 n);
static u32  OS_W32_J_CounterValue     (OS_W32_J_Counter *counter);


/* ==================================================
   JOBS
   ================================================== */

static void OS_W32_J_Push(OS_W32_J_Scheduler *scheduler, const OS_W32_J_Request *request);
static void OS_W32_J_PushWaitingFiber(OS_W32_J_Scheduler *scheduler, OS_W32_J_Fiber *fiber);

static void OS_W32_J_Yield(OS_W32_J_Scheduler *scheduler, OS_W32_J_Counter *counter, u32 value);

static void OS_W32_J_Kick  (OS_W32_J_Scheduler *scheduler, const J_Decl *decl, OS_W32_J_Counter *counter);
static void OS_W32_J_Batch (OS_W32_J_Scheduler *scheduler, const J_Decl *decls, u32 count, OS_W32_J_Counter *counter);

/*
 * Divides [0, count) into batches of batch_size, and kicks
 * off a basic for-loop job for them all.
 */
static J_ENTRY_POINT_DEF(OS_W32_J_ParallelForBatchEntry);

static void OS_W32_J_For(OS_W32_J_Scheduler *scheduler, u32 count, J_EntryForFn *fn, J_Priority priority, u32 batch_size);


/* ==================================================
   SCRATCH
   ================================================== */

static Arena *OS_W32_J_GetScratch(OS_W32_J_Scheduler *scheduler, Arena * const *conflicts, u32 conflict_count);


#endif // OS_WIN32_J_H
