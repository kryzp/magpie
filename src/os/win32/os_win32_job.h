#ifndef OS_WIN32_J_H
#define OS_WIN32_J_H

/*
 * - "Parallelizing the Naughty Dog Engine Using Fibers" - Christian Gyrling
 * - "Parallelizing the Physics Solver" - Dennis Gustafsson
 */

// TODO: this all needs to be reworked its a complete mess, but there are good fundamentals i think
// I'm going to me moving to linux soon anyway so that'll be an opportunity to start from scratch.

#define J_W32_MAX_JOBS_PER_QUEUE         512
#define J_W32_MAX_CONCURRENT_FIBERS      128
#define J_W32_MAX_WORKERS                32
#define J_W32_COUNTER_MAX_WAITING        128
#define J_W32_FIBER_SCRATCH_SIZE         Megabytes(8)
#define J_W32_FIBER_SCRATCH_RING_SIZE    2

typedef struct J_W32_Context J_W32_Context;
struct J_W32_Context
{
	u32 worker_id;
	i32 fiber_id;
};

typedef struct J_W32_Fiber J_W32_Fiber;

typedef struct J_W32_Counter J_W32_Counter;
struct J_W32_Counter
{
	i32 atomic_count;
	i32 atomic_spinlock;

	// Fibers that are blocked waiting on this counter.
	u32 waiting_count;
	J_W32_Fiber *waiting[J_W32_COUNTER_MAX_WAITING];
};

typedef struct J_W32_Fiber J_W32_Fiber;
struct J_W32_Fiber
{
	u32 id;
	J_W32_Fiber *next_free;
	OS_Handle handle;
	J_EntryPointFn *EntryPoint;
	void *param;
	J_Priority priority;
	J_W32_Counter *counter;
	b32 finished;
	Arena scratch_arenas[J_W32_FIBER_SCRATCH_RING_SIZE];
};

typedef struct J_W32_FiberMutex J_W32_FiberMutex;
struct J_W32_FiberMutex
{
	i32 atomic_mutex_state;
	i32 atomic_spinlock;
	u32 waiting_count;
	J_W32_Fiber *waiting[J_W32_COUNTER_MAX_WAITING];
};

typedef struct J_W32_FiberCondVar J_W32_FiberCondVar;
struct J_W32_FiberCondVar
{
	i32 atomic_spinlock;
	u32 waiting_count;
	J_W32_Fiber *waiting[J_W32_COUNTER_MAX_WAITING];
};

typedef struct J_W32_Request J_W32_Request;
struct J_W32_Request
{
	J_EntryPointFn *EntryPoint;
	void *param;
	J_Priority priority;
	J_Flags flags;
	J_W32_Counter *counter;
};

typedef struct J_W32_Queue J_W32_Queue;
struct J_W32_Queue
{
	i32 atomic_spinlock;
	
	J_W32_Request requests[J_W32_MAX_JOBS_PER_QUEUE];
	J_W32_Fiber *waiting[J_W32_MAX_JOBS_PER_QUEUE];

	i32 atomic_taken_task_count;
	i32 atomic_added_task_count;
	
	i32 atomic_taken_waiting_count;
	i32 atomic_added_waiting_count;
};

typedef struct J_W32_Scheduler J_W32_Scheduler;

typedef struct J_W32_Worker J_W32_Worker;
struct J_W32_Worker
{
	u32 id;
	
	OS_Handle thread_handle;
	OS_Handle fiber_handle; // The scheduler fiber for this current worker thread.

	J_W32_Fiber *current_fiber; // The fiber currently executing on this worker.

	// you know what fuck you.
	J_W32_Scheduler *scheduler;
};

typedef struct J_W32_Scheduler J_W32_Scheduler;
struct J_W32_Scheduler
{
	LOG_Channel log_channel;
	
	i32 atomic_running;
	i32 atomic_spin_mode;

	OS_Handle thread_mutex;
	OS_Handle thread_cond_begin;

	J_W32_Queue main_thread_queue;
	J_W32_Queue queues[J_Priority_COUNT];
	
	Arena fallback_scratch_ring[J_W32_FIBER_SCRATCH_RING_SIZE];
	
	u32 worker_count;
	J_W32_Worker workers[J_W32_MAX_WORKERS];

	J_W32_Fiber atomic_fiber_storage[J_W32_MAX_CONCURRENT_FIBERS];

	i32 fiber_pool_spinlock;
	J_W32_Fiber *fiber_pool_head;
	
	void (*OnMainThreadIdle)(void *ctx);
	void *main_thread_idle_ctx;

	//u32 tls_worker_slot;
};


/* ==================================================
   HELPERS
   ================================================== */

internal void J_W32_SpinModeEnable(J_W32_Scheduler *scheduler);
internal void J_W32_SpinModeDisable(J_W32_Scheduler *scheduler);

internal b32 J_W32_IsMainThread(J_W32_Scheduler *scheduler);

internal void J_W32_FiberYield(J_W32_Scheduler *scheduler);
internal void J_W32_FiberCompleted(J_W32_Scheduler *scheduler);
internal J_W32_Fiber *J_W32_FiberFetchFree(J_W32_Scheduler *scheduler);
internal void J_W32_FiberReturn(J_W32_Scheduler *scheduler, J_W32_Fiber *fiber);
internal OS_Handle J_W32_GetCurrentFiberHandle(J_W32_Scheduler *scheduler);

internal J_W32_Request *J_W32_TryGetRequest(J_W32_Scheduler *scheduler);
internal J_W32_Fiber *J_W32_TryGetWaitingFiber(J_W32_Scheduler *scheduler);

internal b32 J_W32_RequestAvailable(J_W32_Scheduler *scheduler);


/* ==================================================
   CORE
   ================================================== */

internal void J_W32_Init(J_W32_Scheduler *scheduler, LOG_Channel log_channel);
internal void J_W32_Shutdown(J_W32_Scheduler *scheduler);

internal void J_W32_SchedulerThreadEntry(void *param);
internal void J_W32_FiberEntry(void *param);

internal void J_W32_Enter(J_W32_Scheduler *scheduler, void (*OnMainThreadIdle)(void *ctx), void *main_thread_idle_ctx);
internal void J_W32_Halt(J_W32_Scheduler *scheduler);

internal J_W32_Context J_W32_GetContext(J_W32_Scheduler *scheduler);

/* ==================================================
   SYNCHRONISATION
   ================================================== */

internal void J_W32_CounterInit(J_W32_Counter *counter, i32 initial_count);
internal void J_W32_CounterIncrement(J_W32_Counter *counter, i32 n);
internal void J_W32_CounterDecrement(J_W32_Scheduler *scheduler, J_W32_Counter *counter, i32 n);
internal u32  J_W32_CounterValue(J_W32_Counter *counter);

internal void J_W32_FiberMutexLock(J_W32_Scheduler *scheduler, J_W32_FiberMutex *m);
internal void J_W32_FiberMutexUnlock(J_W32_Scheduler *scheduler, J_W32_FiberMutex *m);

internal void J_W32_FiberCondVarWait(J_W32_Scheduler *scheduler, J_W32_FiberCondVar *cv, J_W32_FiberMutex *mutex);
internal void J_W32_FiberCondVarSignal(J_W32_Scheduler *scheduler, J_W32_FiberCondVar *cv);
internal void J_W32_FiberCondVarBroadcast(J_W32_Scheduler *scheduler, J_W32_FiberCondVar *cv);


/* ==================================================
   JOBS
   ================================================== */

internal void J_W32_Push(J_W32_Scheduler *scheduler, const J_W32_Request *request);
internal void J_W32_PushWaitingFiber(J_W32_Scheduler *scheduler, J_W32_Fiber *fiber);

internal void J_W32_Yield(J_W32_Scheduler *scheduler, J_W32_Counter *counter, i32 value);

internal void J_W32_Kick  (J_W32_Scheduler *scheduler, const J_Decl *decl, J_W32_Counter *counter);
internal void J_W32_Batch (J_W32_Scheduler *scheduler, const J_Decl *decls, u32 count, J_W32_Counter *counter);

/*
 * Divides [0, count) into batches of batch_size, and kicks
 * off a basic for-loop job for them all.
 */
internal J_ENTRY_POINT_DEF(J_W32_ParallelForBatchEntry);

internal void J_W32_For(J_W32_Scheduler *scheduler, u32 count, J_EntryForFn *fn, J_Priority priority, u32 batch_size);


/* ==================================================
   SCRATCH
   ================================================== */

internal Arena *J_W32_GetScratch(J_W32_Scheduler *scheduler, Arena * const *conflicts, u32 conflict_count);


#endif // OS_WIN32_J_H
