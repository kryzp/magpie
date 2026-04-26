
global __declspec(thread) JOB_Worker *job_current_worker;

internal void
JOB_SpinModeEnable(JOB_Scheduler *scheduler)
{
	osapi->AtomicStoreU32(&scheduler->atomic_spin_mode, true);
	//osapi->CondVarBroadcast(scheduler->cond_begin);
}

internal void
JOB_SpinModeDisable(JOB_Scheduler *scheduler)
{
	osapi->AtomicStoreU32(&scheduler->atomic_spin_mode, false);
}

internal b32
JOB_IsMainThread(JOB_Scheduler *scheduler)
{
	AssertTrue(job_current_worker);
	
	return job_current_worker->id == 0;
}

/*
 * Switch back to the worker's scheduler fiber without marking
 * the job as finished, so the fiber itself won't be returned
 * back to the free fiber pool.
 */
internal void
JOB_FiberYield(JOB_Scheduler *scheduler)
{
	job_current_worker->current_fiber->finished = false;
	
	osapi->SwitchToFiber(job_current_worker->fiber_handle);
}

/*
 * Switch back to the worker's scheduler fiber while marking
 * the job as finished, so the fiber itself is returned
 * back to the free fiber pool for use later.
 */
internal void
JOB_FiberCompleted(JOB_Scheduler *scheduler)
{
	job_current_worker->current_fiber->finished = true;
	
	osapi->SwitchToFiber(job_current_worker->fiber_handle);
}

internal JOB_Fiber *
JOB_FiberFetchFree(JOB_Scheduler *scheduler)
{
	osapi->SpinLockAcquire(&scheduler->fiber_pool_spinlock);
 
	JOB_Fiber *fiber = scheduler->fiber_pool_head;
	
	if (fiber)
		scheduler->fiber_pool_head = fiber->next_free;

	osapi->SpinLockRelease(&scheduler->fiber_pool_spinlock);
 
	if (fiber)
		fiber->next_free = NULL;
 
	return fiber;
}

internal void
JOB_FiberReturn(JOB_Scheduler *scheduler, JOB_Fiber *fiber)
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

internal OS_Handle
JOB_GetCurrentFiberHandle(JOB_Scheduler *scheduler)
{
	if (job_current_worker && job_current_worker->current_fiber)
		return job_current_worker->current_fiber->handle;
	else
		return OS_HandleNull();
}

internal JOB_Request *
JOB_TryGetRequest(JOB_Scheduler *scheduler)
{
	if (JOB_IsMainThread(scheduler))
	{
		JOB_Queue *mtq = &scheduler->main_thread_queue;
		
		if (mtq->atomic_added_task_count > mtq->atomic_taken_task_count)
		{
			osapi->SpinLockAcquire(&mtq->atomic_spinlock);
 
			u32 t = osapi->AtomicLoadU32(&mtq->atomic_taken_task_count);
			u32 a = osapi->AtomicLoadU32(&mtq->atomic_added_task_count);
 
			JOB_Request *request = NULL;
		
			if (a > t)
			{
				request = &mtq->requests[t % JOB_MAX_JOBS_PER_QUEUE];
				mtq->atomic_taken_task_count = t + 1;
			}

			osapi->SpinLockRelease(&mtq->atomic_spinlock);
		
			if (request)
				return request;
		}
	}
	
	for (i32 i = JOB_Priority_COUNT - 1; i >= 0; i--)
	{
		JOB_Queue *queue = &scheduler->queues[i];
		
		if (queue->atomic_added_task_count <= queue->atomic_taken_task_count)
			continue;

		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_task_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_task_count);
 
		JOB_Request *request = NULL;
		
		if (a > t)
		{
			request = &queue->requests[t % JOB_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_task_count = t + 1;
		}

		osapi->SpinLockRelease(&queue->atomic_spinlock);
		
		if (request)
			return request;
	}
 
	return NULL;
}

internal JOB_Fiber *
JOB_TryGetWaitingFiber(JOB_Scheduler *scheduler)
{
	for (i32 i = JOB_Priority_COUNT - 1; i >= 0; i--)
	{
		JOB_Queue *queue = &scheduler->queues[i];
 
		if (queue->atomic_added_waiting_count <= queue->atomic_taken_waiting_count)
			continue;
 
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_waiting_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_waiting_count);
 
		JOB_Fiber *fiber = NULL;
		
		if (a > t)
		{
			fiber = queue->waiting[t % JOB_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_waiting_count = t + 1;
		}

		osapi->SpinLockRelease(&queue->atomic_spinlock);
 
		if (fiber)
			return fiber;
	}
 
	return NULL;
}

internal b32
JOB_RequestAvailable(JOB_Scheduler *scheduler)
{
	for (u32 i = 0; i < JOB_Priority_COUNT; i++)
	{
		JOB_Queue *queue = &scheduler->queues[i];
		
		if (osapi->AtomicLoadU32(&queue->atomic_added_task_count) > osapi->AtomicLoadU32(&queue->atomic_taken_task_count))
			return true;
		
		if (osapi->AtomicLoadU32(&queue->atomic_added_waiting_count) > osapi->AtomicLoadU32(&queue->atomic_taken_waiting_count))
			return true;
	}
	
	return false;
}

internal void
JOB_Init(Arena *arena, JOB_Scheduler *scheduler)
{
	MemZeroStruct(scheduler);
 
	scheduler->mutex	  = osapi->MutexCreate();
	scheduler->cond_begin = osapi->CondVarCreate();
 
	osapi->AtomicStoreU32(&scheduler->atomic_running, true);
 
	for (u32 i = 0; i < JOB_MAX_CONCURRENT_FIBERS; i++)
	{
		JOB_Fiber *fiber = &scheduler->atomic_fiber_storage[i];

		// This is purely an aesthetic thing but since we push
		// the fibers to the front, the ones at the front get
		// selected first so we assign id's in reverse so at
		// the end the "front" fiber has id 0, then 1, etc...
		fiber->id = JOB_MAX_CONCURRENT_FIBERS - i - 1;

		fiber->handle = osapi->FiberCreate(0, JOB_FiberEntry, scheduler);
 
		for (u32 j = 0; j < ArraySize(fiber->scratch_arenas); j++)
		{
			fiber->scratch_arenas[j] = ArenaPushArray(arena, Arena, 1);
			(*fiber->scratch_arenas[j]) = ArenaInitArena(arena, JOB_FIBER_SCRATCH_SIZE, 8);
		}

		// Give the fiber to the freelist.
		JOB_FiberReturn(scheduler, fiber);
	}

	// Try to leave at least one free core available.
	const u32 desired_workers = MaxValue(1, osapi->GetNumCores() - 1);

	scheduler->worker_count = MinValue(desired_workers, JOB_MAX_WORKERS);

	scheduler->workers[0].id = 0;
	scheduler->workers[0].thread_handle = osapi->GetCurrentThreadHandle();
	scheduler->workers[0].scheduler = scheduler;
 
	for (u32 i = 1; i < scheduler->worker_count; i++)
	{
		JOB_Worker *worker = &scheduler->workers[i];
		worker->id = i;
		worker->thread_handle = osapi->ThreadCreate(JOB_SchedulerThreadEntry, worker);
		worker->scheduler = scheduler;
	}

	scheduler->tls_worker_slot = osapi->TLSAlloc();
	osapi->TLSSet(scheduler->tls_worker_slot, NULL);
}

internal void
JOB_Shutdown(JOB_Scheduler *scheduler)
{
	osapi->TLSFree(scheduler->tls_worker_slot);
	
	// Join worker threads (worker 0 is the main thread so no join needed).
	for (u32 i = 1; i < scheduler->worker_count; i++)
		osapi->ThreadJoin(scheduler->workers[i].thread_handle);
 
	// Release all fibers.
	for (u32 i = 0; i < JOB_MAX_CONCURRENT_FIBERS; i++)
		osapi->FiberDelete(scheduler->atomic_fiber_storage[i].handle);

	// Destroy OS synchronisation primitives.
	osapi->MutexDestroy(scheduler->mutex);
	osapi->CondVarDestroy(scheduler->cond_begin);
}

internal void
JOB_SchedulerThreadEntry(void *param)
{
	JOB_Worker *worker = param;
	JOB_Scheduler *scheduler = worker->scheduler;

	osapi->TLSSet(scheduler->tls_worker_slot, worker);

	job_current_worker = worker;
	worker->fiber_handle = osapi->ConvertThreadToFiber();
 
	// Force each worker to stay on a single core.
	osapi->ThreadSetAffinity(osapi->GetCurrentThreadHandle(), 1ull << worker->id);
 
	while (osapi->AtomicLoadU32(&scheduler->atomic_running))
	{
		// Check for a fiber to resume.
		JOB_Fiber *waiting_fiber = JOB_TryGetWaitingFiber(scheduler);
		if (waiting_fiber)
		{
			worker->current_fiber = waiting_fiber;
			
			osapi->SwitchToFiber(waiting_fiber->handle);
 
			if (worker->current_fiber->finished)
				JOB_FiberReturn(scheduler, worker->current_fiber);
			
			worker->current_fiber = NULL;

			continue;
		}
 
		// Check for a new job to start.
		JOB_Request *request = JOB_TryGetRequest(scheduler);
		if (request)
		{
			JOB_Fiber *fiber = JOB_FiberFetchFree(scheduler);
			
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
				JOB_FiberReturn(scheduler, worker->current_fiber);

			worker->current_fiber = NULL;

			continue;
		}
 
		// Wait until we have more work to do...
		
		b32 is_main_thread = JOB_IsMainThread(scheduler);
		
		if (is_main_thread || osapi->AtomicLoadU32(&scheduler->atomic_spin_mode))
		{
			// High-Perf Spin Mode.
			// Main thread always goes here because it can pump messages.
			
			while (!JOB_RequestAvailable(scheduler) && osapi->AtomicLoadU32(&scheduler->atomic_running))
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
			if (!JOB_IsMainThread(scheduler))
			{	
				osapi->MutexLock(scheduler->mutex);
				
				while (!JOB_RequestAvailable(scheduler) && osapi->AtomicLoadU32(&scheduler->atomic_running))
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
internal void
JOB_FiberEntry(void *param)
{
	JOB_Scheduler *scheduler = param;
	
	while (true)
	{
		JOB_Fiber *f = job_current_worker->current_fiber;
		JOB_Counter *c = f->counter;

		if (f->EntryPoint)
			f->EntryPoint(f->param);

		if (c)
			JOB_CounterDecrement(scheduler, c, 1);

		JOB_FiberCompleted(scheduler);
	}
}

internal void
JOB_Enter(JOB_Scheduler *scheduler, void (*OnMainThreadIdle)(void *ctx), void *main_thread_idle_ctx)
{
	scheduler->OnMainThreadIdle = OnMainThreadIdle;
	scheduler->main_thread_idle_ctx = main_thread_idle_ctx;
	
	JOB_SchedulerThreadEntry(&scheduler->workers[0]);
}

internal void
JOB_Halt(JOB_Scheduler *scheduler)
{
	osapi->AtomicStoreU32(&scheduler->atomic_running, false);
	osapi->CondVarBroadcast(scheduler->cond_begin);
}

internal JOB_Context
JOB_GetContext(JOB_Scheduler *scheduler)
{
	JOB_Context ctx = {0};

    JOB_Worker *worker = (JOB_Worker *)osapi->TLSGet(scheduler->tls_worker_slot);

	ctx.worker_id = worker->id;
	ctx.fiber_id = worker->current_fiber ? worker->current_fiber->id : -1;

	return ctx;
}

internal JOB_Counter *
JOB_CounterAlloc(JOB_Scheduler *scheduler, Arena *arena, u32 initial_count)
{
	JOB_Counter *counter = ArenaPushArray(arena, JOB_Counter, 1);
	counter->atomic_count = initial_count;
	
	return counter;
}

internal void
JOB_CounterLock(JOB_Scheduler *scheduler, JOB_Counter *counter)
{
	osapi->SpinLockAcquire(&counter->atomic_spinlock);
}

internal void
JOB_CounterUnlock(JOB_Scheduler *scheduler, JOB_Counter *counter)
{
	osapi->SpinLockRelease(&counter->atomic_spinlock);
}

internal void
JOB_CounterIncrement(JOB_Scheduler *scheduler, JOB_Counter *counter, u32 n)
{
	osapi->AtomicAddU32(&counter->atomic_count, n);
}

/*
 * Decrement the counter, but if we hit zero we have to collect all of the
 * waiting fibers and kick them off.
 */
internal void
JOB_CounterDecrement(JOB_Scheduler *scheduler, JOB_Counter *counter, u32 n)
{
	JOB_CounterLock(scheduler, counter);

	u32 atomic_count = osapi->AtomicLoadU32(&counter->atomic_count);
 
	AssertTrue(atomic_count >= n);

	osapi->AtomicSubU32(&counter->atomic_count, n);

	atomic_count -= n;
 
	u32 kick_count = 0;
	JOB_Fiber *to_kick[JOB_COUNTER_MAX_WAITING] = {0};
 
	if (atomic_count == 0 && counter->waiting_count > 0)
	{
		kick_count = counter->waiting_count;
		MemCopy(to_kick, counter->waiting, kick_count * sizeof(JOB_Fiber *));
		counter->waiting_count = 0;
	}
 
	JOB_CounterUnlock(scheduler, counter);
 
	for (u32 i = 0; i < kick_count; i++)
		JOB_PushWaitingFiber(scheduler, to_kick[i]);
 
	if (kick_count > 0)
	{
		osapi->MutexLock(scheduler->mutex);
		osapi->CondVarBroadcast(scheduler->cond_begin);
		osapi->MutexUnlock(scheduler->mutex);
	}
}

internal void
JOB_Push(JOB_Scheduler *scheduler, const JOB_Request *request)
{
	JOB_Queue *queue = &scheduler->queues[request->priority];

	if (request->flags & JOB_Flag_MainThreadOnly)
		queue = &scheduler->main_thread_queue;
	
	while (true)
	{
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
		
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_task_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_task_count);
 
		if ((a - t) >= JOB_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			OS_SPIN_PAUSE();
		}
		else
		{
			queue->requests[a % JOB_MAX_JOBS_PER_QUEUE] = *request;
			queue->atomic_added_task_count = a + 1;

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

internal void
JOB_PushWaitingFiber(JOB_Scheduler *scheduler, JOB_Fiber *fiber)
{
	JOB_Queue *queue = &scheduler->queues[fiber->priority];
 
	while (true)
	{
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = osapi->AtomicLoadU32(&queue->atomic_taken_waiting_count);
		u32 a = osapi->AtomicLoadU32(&queue->atomic_added_waiting_count);
 
		if ((a - t) >= JOB_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			OS_SPIN_PAUSE();
		}
		else
		{
			queue->waiting[a % JOB_MAX_JOBS_PER_QUEUE] = fiber;
			queue->atomic_added_waiting_count = a + 1;
			
			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

internal void
JOB_Yield(JOB_Scheduler *scheduler, JOB_Counter *counter, u32 value)
{
	while (true)
	{
		JOB_CounterLock(scheduler, counter);
 
		if (counter->atomic_count == value)
		{
			JOB_CounterUnlock(scheduler, counter);
			return;
		}
 
		AssertTrue(counter->waiting_count < JOB_COUNTER_MAX_WAITING);
		
		counter->waiting[counter->waiting_count++] = job_current_worker->current_fiber;
 
		JOB_CounterUnlock(scheduler, counter);
 
		JOB_FiberYield(scheduler);
	}
}

internal void
JOB_Kick(JOB_Scheduler *scheduler, const JOB_Decl *decl, JOB_Counter *counter)
{
	if (counter)
		JOB_CounterIncrement(scheduler, counter, 1);
 
	JOB_Request request = {0};
	request.EntryPoint  = decl->EntryPoint;
	request.param       = decl->param;
	request.priority    = decl->priority;
	request.flags       = decl->flags;
	request.counter     = counter;
 
	JOB_Push(scheduler, &request);

	osapi->MutexLock(scheduler->mutex);
	osapi->CondVarSignal(scheduler->cond_begin);
	osapi->MutexUnlock(scheduler->mutex);
}

internal void
JOB_Batch(JOB_Scheduler *scheduler, const JOB_Decl *decls, u32 count, JOB_Counter *counter)
{
	if (counter)
		JOB_CounterIncrement(scheduler, counter, count);
 
	for (u32 i = 0; i < count; i++)
	{
		JOB_Request request = {0};
		request.EntryPoint  = decls[i].EntryPoint;
		request.param       = decls[i].param;
		request.priority    = decls[i].priority;
		request.flags       = decls[i].flags;
		request.counter     = counter;
 
		JOB_Push(scheduler, &request);
	}
 
	osapi->MutexLock(scheduler->mutex);
	osapi->CondVarBroadcast(scheduler->cond_begin);
	osapi->MutexUnlock(scheduler->mutex);
}

typedef struct JOB_ParallelForParam JOB_ParallelForParam;
struct JOB_ParallelForParam
{
	JOB_EntryForFn *Inner;
	u32 base_index;
	u32 loop_size;
};

JOB_ENTRY_POINT_DEF(JOB_ParallelForBatchEntry)
{
	JOB_ParallelForParam *p = param;
	
	for (u32 i = 0; i < p->loop_size; i++)
		p->Inner(p->base_index + i);
}

internal void
JOB_For(JOB_Scheduler *scheduler, u32 count, JOB_EntryForFn *fn, JOB_Priority priority, u32 batch_size)
{
	if (count == 0 || batch_size == 0)
		return;
 
	u32 job_count = (count + batch_size - 1) / batch_size;
 
	ScratchArena scratch = ScratchBegin(NULL, 0);
 
	JOB_ParallelForParam *params = ArenaPushArray(scratch.arena, JOB_ParallelForParam, job_count);
	JOB_Decl             *decls  = ArenaPushArray(scratch.arena, JOB_Decl,             job_count);
 
	for (u32 i = 0; i < job_count; i++)
	{
		u32 base_index = batch_size * i;
		u32 loop_size  = count - base_index;
		
		if (loop_size > batch_size)
			loop_size = batch_size;
 
		params[i].Inner = fn;
		params[i].base_index = base_index;
		params[i].loop_size = loop_size;
 
		decls[i].EntryPoint	= JOB_ParallelForBatchEntry;
		decls[i].param = &params[i];
		decls[i].priority = priority;
		decls[i].flags = JOB_Flag_None;
	}
 
	JOB_Counter counter = {0};
	JOB_Batch(scheduler, decls, job_count, &counter);
	JOB_Yield(scheduler, &counter, 0);
 
	ScratchRelease(&scratch);
}

internal Arena *
JOB_GetScratch(JOB_Scheduler *scheduler, Arena * const *conflicts, u32 conflict_count)
{
	JOB_Fiber *fiber = job_current_worker->current_fiber;
	
	Arena *arena = NULL;
	Arena **ring = fiber->scratch_arenas;

	for (u32 i = 0; i < ArraySize(fiber->scratch_arenas); i++, ring++)
	{
		b32 conflict = false;

		for (u32 j = 0; j < conflict_count; j++)
		{
			if (*ring == conflicts[j])
			{
				conflict = true;
				break;
			}
		}

		if (!conflict)
		{
			arena = *ring;
			break;
		}
	}

	AssertTrue(arena);

	return arena;
}
