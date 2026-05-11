#ifndef OS_WIN32_JOB_H
#define OS_WIN32_JOB_H

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

#define OS_W32_JOB_MAX_JOBS_PER_QUEUE         512
#define OS_W32_JOB_MAX_CONCURRENT_FIBERS      128
#define OS_W32_JOB_MAX_WORKERS                32
#define OS_W32_JOB_COUNTER_MAX_WAITING        64
#define OS_W32_JOB_FIBER_SCRATCH_SIZE         Megabytes(8)
#define OS_W32_JOB_FIBER_SCRATCH_RING_SIZE    2

typedef struct OS_W32_JOB_Context OS_W32_JOB_Context;
struct OS_W32_JOB_Context
{
	u32 worker_id;
	i32 fiber_id;
};

typedef struct OS_W32_JOB_Fiber OS_W32_JOB_Fiber;

typedef struct OS_W32_JOB_Counter OS_W32_JOB_Counter;
struct OS_W32_JOB_Counter
{
	u32 atomic_count;
	u32 atomic_spinlock;

	// Fibers that are blocked waiting on this counter.
	u32 waiting_count;
	OS_W32_JOB_Fiber *waiting[OS_W32_JOB_COUNTER_MAX_WAITING];
};

typedef struct OS_W32_JOB_Fiber OS_W32_JOB_Fiber;
struct OS_W32_JOB_Fiber
{
	u32 id;
	
	OS_W32_JOB_Fiber *next_free;

	OS_Handle handle;

	JOB_EntryPointFn *EntryPoint;
	void *param;
	
	JOB_Priority priority;

	OS_W32_JOB_Counter *counter;
	
	b32 finished;

	Arena scratch_arenas[OS_W32_JOB_FIBER_SCRATCH_RING_SIZE];
};

typedef struct OS_W32_JOB_Request OS_W32_JOB_Request;
struct OS_W32_JOB_Request
{
	JOB_EntryPointFn *EntryPoint;
	void *param;
	JOB_Priority priority;
	JOB_Flags flags;
	OS_W32_JOB_Counter *counter;
};

typedef struct OS_W32_JOB_Queue OS_W32_JOB_Queue;
struct OS_W32_JOB_Queue
{
	u32 atomic_spinlock;
	
	OS_W32_JOB_Request requests[OS_W32_JOB_MAX_JOBS_PER_QUEUE];
	OS_W32_JOB_Fiber *waiting[OS_W32_JOB_MAX_JOBS_PER_QUEUE];

	u32 atomic_taken_task_count;
	u32 atomic_added_task_count;
	
	u32 atomic_taken_waiting_count;
	u32 atomic_added_waiting_count;
};

typedef struct OS_W32_JOB_Scheduler OS_W32_JOB_Scheduler;

typedef struct OS_W32_JOB_Worker OS_W32_JOB_Worker;
struct OS_W32_JOB_Worker
{
	u32 id;
	
	OS_Handle thread_handle;
	OS_Handle fiber_handle; // The scheduler fiber for this current worker thread.

	OS_W32_JOB_Fiber *current_fiber; // The fiber currently executing on this worker.

	// you know what fuck you.
	OS_W32_JOB_Scheduler *scheduler;
};

typedef struct OS_W32_JOB_Scheduler OS_W32_JOB_Scheduler;
struct OS_W32_JOB_Scheduler
{
	LOG_Channel log_channel;
	
	u32 atomic_running;
	u32 atomic_spin_mode;

	OS_Handle mutex;
	OS_Handle cond_begin;

	OS_W32_JOB_Queue main_thread_queue;
	OS_W32_JOB_Queue queues[JOB_Priority_COUNT];
	
	Arena fallback_scratch_ring[OS_W32_JOB_FIBER_SCRATCH_RING_SIZE];
	
	u32 worker_count;
	OS_W32_JOB_Worker workers[OS_W32_JOB_MAX_WORKERS];

	OS_W32_JOB_Fiber atomic_fiber_storage[OS_W32_JOB_MAX_CONCURRENT_FIBERS];

	u32 fiber_pool_spinlock;
	OS_W32_JOB_Fiber *fiber_pool_head;
	
	void (*OnMainThreadIdle)(void *ctx);
	void *main_thread_idle_ctx;

	u32 tls_worker_slot;
};


/* ==================================================
   HELPERS
   ================================================== */

internal void                OS_W32_JOB_SpinModeEnable          (OS_W32_JOB_Scheduler *scheduler);
internal void                OS_W32_JOB_SpinModeDisable         (OS_W32_JOB_Scheduler *scheduler);

internal b32                 OS_W32_JOB_IsMainThread            (OS_W32_JOB_Scheduler *scheduler);

internal void                OS_W32_JOB_FiberYield              (OS_W32_JOB_Scheduler *scheduler);
internal void                OS_W32_JOB_FiberCompleted          (OS_W32_JOB_Scheduler *scheduler);
internal OS_W32_JOB_Fiber   *OS_W32_JOB_FiberFetchFree          (OS_W32_JOB_Scheduler *scheduler);
internal void                OS_W32_JOB_FiberReturn             (OS_W32_JOB_Scheduler *scheduler, OS_W32_JOB_Fiber *fiber);
internal OS_Handle           OS_W32_JOB_GetCurrentFiberHandle   (OS_W32_JOB_Scheduler *scheduler);

internal OS_W32_JOB_Request *OS_W32_JOB_TryGetRequest           (OS_W32_JOB_Scheduler *scheduler);
internal OS_W32_JOB_Fiber   *OS_W32_JOB_TryGetWaitingFiber      (OS_W32_JOB_Scheduler *scheduler);

internal b32                 OS_W32_JOB_RequestAvailable        (OS_W32_JOB_Scheduler *scheduler);


/* ==================================================
   CORE
   ================================================== */

internal void OS_W32_JOB_Init     (OS_W32_JOB_Scheduler *scheduler, LOG_Channel log_channel);
internal void OS_W32_JOB_Shutdown (OS_W32_JOB_Scheduler *scheduler);

internal void OS_W32_JOB_SchedulerThreadEntry(void *param);
internal void OS_W32_JOB_FiberEntry(void *param);

internal void OS_W32_JOB_Enter (OS_W32_JOB_Scheduler *scheduler, void (*OnMainThreadIdle)(void *ctx), void *main_thread_idle_ctx);
internal void OS_W32_JOB_Halt  (OS_W32_JOB_Scheduler *scheduler);

internal OS_W32_JOB_Context OS_W32_JOB_GetContext(OS_W32_JOB_Scheduler *scheduler);

/* ==================================================
   COUNTER
   ================================================== */

internal void OS_W32_JOB_CounterInit      (OS_W32_JOB_Counter *counter, u32 initial_count);
internal void OS_W32_JOB_CounterIncrement (OS_W32_JOB_Counter *counter, u32 n);
internal void OS_W32_JOB_CounterDecrement (OS_W32_JOB_Scheduler *scheduler, OS_W32_JOB_Counter *counter, u32 n);
internal u32  OS_W32_JOB_CounterValue     (OS_W32_JOB_Counter *counter);


/* ==================================================
   JOBS
   ================================================== */

internal void OS_W32_JOB_Push(OS_W32_JOB_Scheduler *scheduler, const OS_W32_JOB_Request *request);
internal void OS_W32_JOB_PushWaitingFiber(OS_W32_JOB_Scheduler *scheduler, OS_W32_JOB_Fiber *fiber);

internal void OS_W32_JOB_Yield(OS_W32_JOB_Scheduler *scheduler, OS_W32_JOB_Counter *counter, u32 value);

internal void OS_W32_JOB_Kick  (OS_W32_JOB_Scheduler *scheduler, const JOB_Decl *decl, OS_W32_JOB_Counter *counter);
internal void OS_W32_JOB_Batch (OS_W32_JOB_Scheduler *scheduler, const JOB_Decl *decls, u32 count, OS_W32_JOB_Counter *counter);

/*
 * Divides [0, count) into batches of batch_size, and kicks
 * off a basic for-loop job for them all.
 */
JOB_ENTRY_POINT_DEF(OS_W32_JOB_ParallelForBatchEntry);

internal void OS_W32_JOB_For(OS_W32_JOB_Scheduler *scheduler, u32 count, JOB_EntryForFn *fn, JOB_Priority priority, u32 batch_size);


/* ==================================================
   SCRATCH
   ================================================== */

internal Arena *OS_W32_JOB_GetScratch(OS_W32_JOB_Scheduler *scheduler, Arena * const *conflicts, u32 conflict_count);


#endif // OS_WIN32_JOB_H
