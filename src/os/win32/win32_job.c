
static __declspec(thread) OS_W32_J_Worker *job_current_worker;

static void OS_W32_J_SpinModeEnable(OS_W32_J_Scheduler *scheduler)
{
	osapi->AtomicStoreU32(&scheduler->atomic_spin_mode, true);
	//osapi->CondVarBroadcast(scheduler->cond_begin);
}

static void OS_W32_J_SpinModeDisable(OS_W32_J_Scheduler *scheduler)
{
	osapi->AtomicStoreU32(&scheduler->atomic_spin_mode, false);
}

static b32 OS_W32_J_IsMainThread(OS_W32_J_Scheduler *scheduler)
{
	AssertTrue(job_current_worker);
	
	return job_current_worker->id == 0;
}

/*
 * Switch back to the worker's scheduler fiber without marking
 * the job as finished, so the fiber itself won't be returned
 * back to the free fiber pool.
 */
static void OS_W32_J_FiberYield(OS_W32_J_Scheduler *scheduler)
{
	job_current_worker->current_fiber->finished = false;
	
	osapi->SwitchToFiber(job_current_worker->fiber_handle);
}

/*
 * Switch back to the worker's scheduler fiber while marking
 * the job as finished, so the fiber itself is returned
 * back to the free fiber pool for use later.
 */
static void OS_W32_J_FiberCompleted(OS_W32_J_Scheduler *scheduler)
{
	job_current_worker->current_fiber->finished = true;
	
	osapi->SwitchToFiber(job_current_worker->fiber_handle);
}

static OS_W32_J_Fiber *OS_W32_J_FiberFetchFree(OS_W32_J_Scheduler *scheduler)
{
	osapi->SpinLockAcquire(&scheduler->fiber_pool_spinlock);
 
	OS_W32_J_Fiber *fiber = scheduler->fiber_pool_head;
	
	if (fiber)
		scheduler->fiber_pool_head = fiber->next_free;

	osapi->SpinLockRelease(&scheduler->fiber_pool_spinlock);
 
	if (fiber)
		fiber->next_free = NULL;
 
	return fiber;
}

static void OS_W32_J_FiberReturn(OS_W32_J_Scheduler *scheduler, OS_W32_J_Fiber *fiber)
{
	fiber->EntryPoint = NULL;
	fiber->param = NULL;
	
	fiber->counter = NULL;
	fiber->finished = true;
 
	osapi->SpinLockAcquire(&scheduler->fiber_pool_spinlock);
 
	fiber->next_free = scheduler->fiber_pool_head;
	scheduler->fiber_pool_head = fiber;
 
	osapi->SpinLockRelease(&scheduler->fiber_pool_spinlock);
}

static OS_Handle OS_W32_J_GetCurrentFiberHandle(OS_W32_J_Scheduler *scheduler)
{
	if (job_current_worker && job_current_worker->current_fiber)
		return job_current_worker->current_fiber->handle;
	else
		return OS_HandleNull();
}

static OS_W32_J_Request *OS_W32_J_TryGetRequest(OS_W32_J_Scheduler *scheduler)
{
	if (OS_W32_J_IsMainThread(scheduler))
	{
		OS_W32_J_Queue *mtq = &scheduler->main_thread_queue;
		
		if (mtq->atomic_added_task_count > mtq->atomic_taken_task_count)
		{
			osapi->SpinLockAcquire(&mtq->atomic_spinlock);
 
			u32 t = osapi->AtomicLoadU32(&mtq->atomic_taken_task_count);
			u32 a = osapi->AtomicLoadU32(&mtq->atomic_added_task_count);
 
			OS_W32_J_Request *request = NULL;
		
			if (a > t)
			{
				request = &mtq->requests[t % OS_W32_J_MAX_JOBS_PER_QUEUE];
				mtq->atomic_taken_task_count = t + 1;
			}

			osapi->SpinLockRelease(&mtq->atomic_spinlock);
		
			if (request)
				return request;
		}
	}
	
	for (i32 i = J_Priority_COUNT - 1; i >= 0; i--)
	{
		OS_W32_J_Queue *queue = &scheduler->queues[i];
		
		if (queue->atomic_added_task_count <= queue->atomic_taken_task_count)
			continue;

		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_task_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_task_count);
 
		OS_W32_J_Request *request = NULL;
		
		if (a > t)
		{
			request = &queue->requests[t % OS_W32_J_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_task_count = t + 1;
		}

		osapi->SpinLockRelease(&queue->atomic_spinlock);
		
		if (request)
			return request;
	}
 
	return NULL;
}

static OS_W32_J_Fiber *OS_W32_J_TryGetWaitingFiber(OS_W32_J_Scheduler *scheduler)
{
	for (i32 i = J_Priority_COUNT - 1; i >= 0; i--)
	{
		OS_W32_J_Queue *queue = &scheduler->queues[i];
 
		if (queue->atomic_added_waiting_count <= queue->atomic_taken_waiting_count)
			continue;
 
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_waiting_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_waiting_count);
 
		OS_W32_J_Fiber *fiber = NULL;
		
		if (a > t)
		{
			fiber = queue->waiting[t % OS_W32_J_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_waiting_count = t + 1;
		}

		osapi->SpinLockRelease(&queue->atomic_spinlock);
 
		if (fiber)
			return fiber;
	}
 
	return NULL;
}

static b32 OS_W32_J_RequestAvailable(OS_W32_J_Scheduler *scheduler)
{
	for (u32 i = 0; i < J_Priority_COUNT; i++)
	{
		OS_W32_J_Queue *queue = &scheduler->queues[i];
		
		if (osapi->AtomicLoadU32(&queue->atomic_added_task_count) > osapi->AtomicLoadU32(&queue->atomic_taken_task_count))
			return true;
		
		if (osapi->AtomicLoadU32(&queue->atomic_added_waiting_count) > osapi->AtomicLoadU32(&queue->atomic_taken_waiting_count))
			return true;
	}
	
	return false;
}

static void OS_W32_J_Init(OS_W32_J_Scheduler *scheduler, LOG_Channel log_channel)
{
	MemZeroStruct(scheduler);

	scheduler->log_channel = log_channel;
 
	scheduler->mutex	  = osapi->MutexCreate();
	scheduler->cond_begin = osapi->CondVarCreate();
 
	osapi->AtomicStoreU32(&scheduler->atomic_running, true);
	
	for (u32 j = 0; j < OS_W32_J_FIBER_SCRATCH_RING_SIZE; j++)
		scheduler->fallback_scratch_ring[j] = ArenaAlloc(OS_W32_J_FIBER_SCRATCH_SIZE);
 
	for (u32 i = 0; i < OS_W32_J_MAX_CONCURRENT_FIBERS; i++)
	{
		OS_W32_J_Fiber *fiber = &scheduler->atomic_fiber_storage[i];

		// This is purely an aesthetic thing but since we push
		// the fibers to the front, the ones at the front get
		// selected first so we assign id's in reverse so at
		// the end the "front" fiber has id 0, then 1, etc...
		fiber->id = OS_W32_J_MAX_CONCURRENT_FIBERS - i - 1;

		fiber->handle = osapi->FiberCreate(0, OS_W32_J_FiberEntry, scheduler);
 
		for (u32 j = 0; j < OS_W32_J_FIBER_SCRATCH_RING_SIZE; j++)
			fiber->scratch_arenas[j] = ArenaAlloc(OS_W32_J_FIBER_SCRATCH_SIZE);

		// Give the fiber to the freelist.
		OS_W32_J_FiberReturn(scheduler, fiber);
	}

	// Try to leave at least one free core available.
	const u32 desired_workers = MaxValue(1, osapi->GetNumCores() - 1);

	scheduler->worker_count = MinValue(desired_workers, OS_W32_J_MAX_WORKERS);

	scheduler->workers[0].id = 0;
	scheduler->workers[0].thread_handle = osapi->GetCurrentThreadHandle();
	scheduler->workers[0].scheduler = scheduler;
 
	for (u32 i = 1; i < scheduler->worker_count; i++)
	{
		OS_W32_J_Worker *worker = &scheduler->workers[i];
		worker->id = i;
		worker->thread_handle = osapi->ThreadCreate(OS_W32_J_SchedulerThreadEntry, worker);
		worker->scheduler = scheduler;
	}

	//scheduler->tls_worker_slot = osapi->TLSAlloc();
	//osapi->TLSSet(scheduler->tls_worker_slot, NULL);

	DebugLogI(scheduler->log_channel, "Initialized.");
}

static void OS_W32_J_Shutdown(OS_W32_J_Scheduler *scheduler)
{
	//osapi->TLSFree(scheduler->tls_worker_slot);
	
	// worker 0 is the main thread so no join needed.
	for (u32 i = 1; i < scheduler->worker_count; i++)
		osapi->ThreadJoin(scheduler->workers[i].thread_handle);
 
	// release fibers
	for (u32 i = 0; i < OS_W32_J_MAX_CONCURRENT_FIBERS; i++)
	{
		for (u32 j = 0; j < OS_W32_J_FIBER_SCRATCH_RING_SIZE; j++)
			ArenaRelease(&scheduler->atomic_fiber_storage[i].scratch_arenas[j]);

		osapi->FiberDelete(scheduler->atomic_fiber_storage[i].handle);
	}
	
	for (u32 j = 0; j < OS_W32_J_FIBER_SCRATCH_RING_SIZE; j++)
		ArenaRelease(&scheduler->fallback_scratch_ring[j]);

	osapi->MutexDestroy(scheduler->mutex);
	osapi->CondVarDestroy(scheduler->cond_begin);

	DebugLogI(scheduler->log_channel, "Destroyed.");
}

static void OS_W32_J_SchedulerThreadEntry(void *param)
{
	OS_W32_J_Worker *worker = param;
	OS_W32_J_Scheduler *scheduler = worker->scheduler;

	//osapi->TLSSet(scheduler->tls_worker_slot, worker);

	job_current_worker = worker;
	worker->fiber_handle = osapi->ConvertThreadToFiber();
 
	// Force each worker to stay on a single core.
	osapi->ThreadSetAffinity(osapi->GetCurrentThreadHandle(), 1ull << worker->id);
 
	while (osapi->AtomicLoadU32(&scheduler->atomic_running))
	{
		// Check for a fiber to resume.
		OS_W32_J_Fiber *waiting_fiber = OS_W32_J_TryGetWaitingFiber(scheduler);
		if (waiting_fiber)
		{
			worker->current_fiber = waiting_fiber;
			
			osapi->SwitchToFiber(waiting_fiber->handle);
 
			if (worker->current_fiber->finished)
				OS_W32_J_FiberReturn(scheduler, worker->current_fiber);
			
			worker->current_fiber = NULL;

			continue;
		}
 
		// Check for a new job to start.
		OS_W32_J_Request *request = OS_W32_J_TryGetRequest(scheduler);
		if (request)
		{
			OS_W32_J_Fiber *fiber = OS_W32_J_FiberFetchFree(scheduler);
			
			if (!fiber)
			{
				// Spin until there is a free fiber.
				OS_SPIN_PAUSE();
				continue;
			}
 
			fiber->EntryPoint = request->EntryPoint;
			fiber->param      = request->param;
			fiber->priority	  = request->priority;
			fiber->counter    = request->counter;
			fiber->finished   = false;
 
			worker->current_fiber = fiber;
			
			osapi->SwitchToFiber(fiber->handle);
 
			if (worker->current_fiber->finished)
				OS_W32_J_FiberReturn(scheduler, worker->current_fiber);

			worker->current_fiber = NULL;

			continue;
		}
 
		// Wait until we have more work to do...
		
		b32 is_main_thread = OS_W32_J_IsMainThread(scheduler);
		
		if (is_main_thread || osapi->AtomicLoadU32(&scheduler->atomic_spin_mode))
		{
			// High-Perf Spin Mode.
			// Main thread always goes here because it can pump messages.
			
			while (!OS_W32_J_RequestAvailable(scheduler) && osapi->AtomicLoadU32(&scheduler->atomic_running))
			{
				if (scheduler->OnMainThreadIdle && is_main_thread)
					scheduler->OnMainThreadIdle(scheduler->main_thread_idle_ctx);
				
				if (!osapi->AtomicLoadU32(&scheduler->atomic_spin_mode))
					break;
				
				OS_SPIN_PAUSE();
			}
		}
		else
		{
			// No Spin Mode so resort to regular mutex and
			// waiting on a condition variable.
			//
			// Main thread can't sleep on condition variable
			// because it must keep pumping messages.
			if (!OS_W32_J_IsMainThread(scheduler))
			{	
				osapi->MutexLock(scheduler->mutex);
				
				while (!OS_W32_J_RequestAvailable(scheduler) && osapi->AtomicLoadU32(&scheduler->atomic_running))
				{
					osapi->CondVarWait(scheduler->cond_begin, scheduler->mutex);
				}
				
				osapi->MutexUnlock(scheduler->mutex);
			}
		}
	}
 
	osapi->ConvertFiberToThread();
}

/*
 * The entry point for all fibers.
 */
static void OS_W32_J_FiberEntry(void *param)
{
	OS_W32_J_Scheduler *scheduler = param;
	
	for (;;)
	{
		OS_W32_J_Fiber *f = job_current_worker->current_fiber;
		OS_W32_J_Counter *c = f->counter;

		if (f->EntryPoint)
			f->EntryPoint(f->param);
		
		if (c)
			OS_W32_J_CounterDecrement(scheduler, c, 1);

		OS_W32_J_FiberCompleted(scheduler);
	}
}

static void OS_W32_J_Enter(OS_W32_J_Scheduler *scheduler, void (*OnMainThreadIdle)(void *ctx), void *main_thread_idle_ctx)
{
	scheduler->OnMainThreadIdle = OnMainThreadIdle;
	scheduler->main_thread_idle_ctx = main_thread_idle_ctx;
	
	OS_W32_J_SchedulerThreadEntry(&scheduler->workers[0]);
}

static void OS_W32_J_Halt(OS_W32_J_Scheduler *scheduler)
{
	osapi->AtomicStoreU32(&scheduler->atomic_running, false);
	osapi->CondVarBroadcast(scheduler->cond_begin);
}

static OS_W32_J_Context OS_W32_J_GetContext(OS_W32_J_Scheduler *scheduler)
{
	OS_W32_J_Context ctx = {0};
	ctx.worker_id = 0;
	ctx.fiber_id = -1;
	
    OS_W32_J_Worker *worker = job_current_worker;//(OS_W32_J_Worker *)osapi->TLSGet(scheduler->tls_worker_slot);

	if (worker)
	{
		ctx.worker_id = worker->id;
		ctx.fiber_id = worker->current_fiber ? worker->current_fiber->id : -1;
	}

	return ctx;
}

static void OS_W32_J_CounterInit(OS_W32_J_Counter *counter, u32 initial_count)
{
	counter->atomic_count = initial_count;
}

static void OS_W32_J_CounterIncrement(OS_W32_J_Counter *counter, u32 n)
{
	osapi->AtomicAddU32(&counter->atomic_count, n);
}

/*
 * Decrement the counter, but if we hit zero we have to collect all of the
 * waiting fibers and kick them off.
 */
static void OS_W32_J_CounterDecrement(OS_W32_J_Scheduler *scheduler, OS_W32_J_Counter *counter, u32 n)
{
	osapi->SpinLockAcquire(&counter->atomic_spinlock);

	u32 atomic_count = osapi->AtomicLoadU32(&counter->atomic_count);
 
	AssertTrue(atomic_count >= n);

	osapi->AtomicSubU32(&counter->atomic_count, n);

	atomic_count -= n;
 
	u32 kick_count = 0;
	OS_W32_J_Fiber *to_kick[OS_W32_J_COUNTER_MAX_WAITING] = {0};
 
	if (atomic_count == 0 && counter->waiting_count > 0)
	{
		kick_count = counter->waiting_count;
		MemCopy(to_kick, counter->waiting, kick_count * sizeof(OS_W32_J_Fiber *));
		counter->waiting_count = 0;
	}
 
	osapi->SpinLockRelease(&counter->atomic_spinlock);
 
	for (u32 i = 0; i < kick_count; i++)
		OS_W32_J_PushWaitingFiber(scheduler, to_kick[i]);
 
	if (kick_count > 0)
	{
		osapi->MutexLock(scheduler->mutex);
		osapi->CondVarBroadcast(scheduler->cond_begin);
		osapi->MutexUnlock(scheduler->mutex);
	}
}

static u32 OS_W32_J_CounterValue(OS_W32_J_Counter *counter)
{
	return osapi->AtomicLoadU32(&counter->atomic_count);
}

static void OS_W32_J_Push(OS_W32_J_Scheduler *scheduler, const OS_W32_J_Request *request)
{
	OS_W32_J_Queue *queue = &scheduler->queues[request->priority];

	if (request->flags & J_Flag_MainThreadOnly)
		queue = &scheduler->main_thread_queue;
	
	for (;;)
	{
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
		
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_task_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_task_count);
 
		if ((a - t) >= OS_W32_J_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			OS_SPIN_PAUSE();
		}
		else
		{
			queue->requests[a % OS_W32_J_MAX_JOBS_PER_QUEUE] = *request;
			queue->atomic_added_task_count = a + 1;

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

static void OS_W32_J_PushWaitingFiber(OS_W32_J_Scheduler *scheduler, OS_W32_J_Fiber *fiber)
{
	OS_W32_J_Queue *queue = &scheduler->queues[fiber->priority];
 
	for (;;)
	{
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_waiting_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_waiting_count);
 
		if ((a - t) >= OS_W32_J_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			OS_SPIN_PAUSE();
		}
		else
		{
			queue->waiting[a % OS_W32_J_MAX_JOBS_PER_QUEUE] = fiber;
			queue->atomic_added_waiting_count = a + 1;
			
			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

static void OS_W32_J_Yield(OS_W32_J_Scheduler *scheduler, OS_W32_J_Counter *counter, u32 value)
{
	for (;;)
	{
		osapi->SpinLockAcquire(&counter->atomic_spinlock);
 
		if (counter->atomic_count == value)
		{
			osapi->SpinLockRelease(&counter->atomic_spinlock);
			return;
		}
 
		AssertTrue(counter->waiting_count < OS_W32_J_COUNTER_MAX_WAITING);
		
		counter->waiting[counter->waiting_count++] = job_current_worker->current_fiber;
 
		osapi->SpinLockRelease(&counter->atomic_spinlock);
 
		OS_W32_J_FiberYield(scheduler);
	}
}

static void OS_W32_J_Kick(OS_W32_J_Scheduler *scheduler, const J_Decl *decl, OS_W32_J_Counter *counter)
{
	if (counter)
		OS_W32_J_CounterIncrement(counter, 1);
 
	OS_W32_J_Request request = {0};
	request.EntryPoint = decl->EntryPoint;
	request.param      = decl->param;
	request.priority   = decl->priority;
	request.flags      = decl->flags;
	request.counter    = counter;
 
	OS_W32_J_Push(scheduler, &request);

	osapi->MutexLock(scheduler->mutex);
	osapi->CondVarSignal(scheduler->cond_begin);
	osapi->MutexUnlock(scheduler->mutex);
}

static void OS_W32_J_Batch(OS_W32_J_Scheduler *scheduler, const J_Decl *decls, u32 count, OS_W32_J_Counter *counter)
{
	if (counter)
		OS_W32_J_CounterIncrement(counter, count);
 
	for (u32 i = 0; i < count; i++)
	{
		OS_W32_J_Request request = {0};
		request.EntryPoint = decls[i].EntryPoint;
		request.param      = decls[i].param;
		request.priority   = decls[i].priority;
		request.flags      = decls[i].flags;
		request.counter    = counter;
 
		OS_W32_J_Push(scheduler, &request);
	}
 
	osapi->MutexLock(scheduler->mutex);
	osapi->CondVarBroadcast(scheduler->cond_begin);
	osapi->MutexUnlock(scheduler->mutex);
}

typedef struct OS_W32_J_ParallelForParam OS_W32_J_ParallelForParam;
struct OS_W32_J_ParallelForParam
{
	J_EntryForFn *Inner;
	u32 base_index;
	u32 loop_size;
};

static J_ENTRY_POINT_DEF(OS_W32_J_ParallelForBatchEntry)
{
	OS_W32_J_ParallelForParam *p = param;
	
	for (u32 i = 0; i < p->loop_size; i++)
		p->Inner(p->base_index + i);
}

static void OS_W32_J_For(OS_W32_J_Scheduler *scheduler, u32 count, J_EntryForFn *fn, J_Priority priority, u32 batch_size)
{
	if (count == 0 || batch_size == 0)
		return;
 
	u32 job_count = (count + batch_size - 1) / batch_size;
 
	ScratchArena scratch = ScratchBegin(NULL, 0);
 
	OS_W32_J_ParallelForParam *params = ArenaPushArray(scratch.arena, OS_W32_J_ParallelForParam, job_count);
	J_Decl                    *decls  = ArenaPushArray(scratch.arena, J_Decl,                    job_count);
 
	for (u32 i = 0; i < job_count; i++)
	{
		u32 base_index = batch_size * i;
		u32 loop_size  = count - base_index;
		
		if (loop_size > batch_size)
			loop_size = batch_size;
 
		params[i].Inner = fn;
		params[i].base_index = base_index;
		params[i].loop_size = loop_size;
 
		decls[i].EntryPoint	= OS_W32_J_ParallelForBatchEntry;
		decls[i].param = &params[i];
		decls[i].priority = priority;
		decls[i].flags = J_Flag_None;
	}
 
	OS_W32_J_Counter counter = {0};
	OS_W32_J_Batch(scheduler, decls, job_count, &counter);
	OS_W32_J_Yield(scheduler, &counter, 0);
 
	ScratchRelease(&scratch);
}

static Arena *OS_W32_J_GetScratch(OS_W32_J_Scheduler *scheduler, Arena * const *conflicts, u32 conflict_count)
{
	Arena *arena = NULL;
	Arena *ring = NULL;

	if (job_current_worker)
	{
		OS_W32_J_Fiber *fiber = job_current_worker->current_fiber;
		ring = fiber->scratch_arenas;
	}
	else
	{
		ring = scheduler->fallback_scratch_ring;
	}

	for (u32 i = 0; i < OS_W32_J_FIBER_SCRATCH_RING_SIZE; i++, ring++)
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

	DebugLogAssert(scheduler->log_channel, arena, "Must have found scratch arena. This is mathematically impossible to hit, the fuck?");

	return arena;
}
