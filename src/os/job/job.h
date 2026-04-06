#ifndef JOB_H
#define JOB_H

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

typedef struct JOB_Fiber JOB_Fiber;
struct JOB_Fiber
{
	JOB_Fiber *next_free;

	OS_Handle handle;
	JOB_EntryPoint *EntryPoint;
	void *param;
	JOB_Priority priority;
	JOB_Counter *counter;
	b32 finished;
	Arena *scratch_arenas[2];
};

typedef struct JOB_Counter JOB_Counter;
struct JOB_Counter
{
	u32 atomic_count;
	u32 atomic_spinlock;

	// Fibers that are blocked waiting on this counter.
	u32 waiting_count;
	JOB_Fiber *waiting[JOB_COUNTER_MAX_WAITING];
};

typedef struct JOB_Request JOB_Request;
struct JOB_Request
{
	JOB_EntryPoint *EntryPoint;
	void *param;
	JOB_Priority priority;
	JOB_Counter *counter;
};

typedef struct JOB_Queue JOB_Queue;
struct JOB_Queue
{
	u32 atomic_spinlock;
	
	JOB_Request requests[JOB_MAX_JOBS_PER_QUEUE];
	JOB_Fiber *waiting[JOB_MAX_JOBS_PER_QUEUE];

	u32 atomic_taken_task_count;
	u32 atomic_added_task_count;
	
	u32 atomic_taken_waiting_count;
	u32 atomic_added_waiting_count;
};

typedef struct JOB_Worker JOB_Worker;
struct JOB_Worker
{
	JOB_Worker *next;
	
	u32 id;
	
	OS_Handle thread_handle;
	OS_Handle fiber_handle; // The scheduler fiber for this current worker thread.

	JOB_Fiber *current_fiber; // The fiber currently executing on this worker.
};

typedef struct JOB_Scheduler JOB_Scheduler;
struct JOB_Scheduler
{
	u32 atomic_running;
	u32 atomic_spin_mode;

	OS_Handle mutex;
	OS_Handle cond_begin;

	JOB_Queue queues[JOB_Priority_COUNT];
	
	u32 worker_count;
	JOB_Worker workers[JOB_MAX_WORKERS];

	JOB_Fiber atomic_fiber_storage[JOB_MAX_CONCURRENT_FIBERS];
};


/* ==================================================
   HELPERS
   ================================================== */

internal void JOB_SpinModeEnable(void);
internal void JOB_SpinModeDisable(void);

internal b32 JOB_IsMainThread(void);

internal void         JOB_FiberYield(void);
internal void         JOB_FiberCompleted(void);
internal JOB_Fiber   *JOB_FiberFetchFree(void);
internal void         JOB_FiberReturn(JOB_Fiber *fiber);
internal OS_Handle    JOB_GetCurrentFiberHandle(void);

internal JOB_Request *JOB_TryGetRequest(void);
internal JOB_Fiber   *JOB_TryGetWaitingFiber(void);

internal b32          JOB_RequestAvailable(void);


/* ==================================================
   CORE
   ================================================== */

internal void JOB_InitAndSelect(Arena *arena, JOB_Scheduler *scheduler);
internal void JOB_Shutdown(void);
internal void JOB_SchedulerThreadEntry(void *param);
internal void JOB_FiberEntry(void *param);
internal void JOB_Enter(void (*MessagePump)(void));
internal void JOB_Halt(void);


/* ==================================================
   COUNTER
   ================================================== */

internal JOB_Counter *JOB_CounterAlloc(Arena *arena, u32 initial_count);
internal void         JOB_CounterLock(JOB_Counter *counter);
internal void         JOB_CounterUnlock(JOB_Counter *counter);
internal void         JOB_CounterIncrement(JOB_Counter *counter, u32 n);
internal void         JOB_CounterDecrement(JOB_Counter *counter, u32 n);


/* ==================================================
   JOBS
   ================================================== */

internal void JOB_Push(const JOB_Request *request);
internal void JOB_PushWaitingFiber(JOB_Fiber *fiber);

internal void JOB_Yield(JOB_Counter *counter, u32 value);

internal void JOB_Kick(const JOB_Decl *decl, JOB_Counter *counter);
internal void JOB_Batch(const JOB_Decl *decls, u32 count, JOB_Counter *counter);

/*
 * Parallel for loop utility.
 * Divides [0, count) into batches of batch_size, and kicks
 * off a basic for-loop job for them all.
 */
JOB_ENTRY_POINT_SIG(JOB_ParallelForBatchEntry);

internal void JOB_For(u32 count, JOB_EntryFor *fn, JOB_Priority priority, u32 batch_size);


/* ==================================================
   SCRATCH
   ================================================== */

internal Arena *JOB_GetScratch(Arena **conflicts, u32 conflict_count);


#endif // JOB_H
