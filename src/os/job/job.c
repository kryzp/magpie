
global JOB_Scheduler *job_scheduler;
global void (*job_MessagePump)(void);
global thread_local JOB_Worker *job_current_worker;
global b32 job_fiber_pool_locked;
global JOB_Fiber *job_fiber_pool_head;

internal void
JOB_SpinModeEnable(void)
{
	OS_AtomicStoreU32(&job_scheduler->atomic_spin_mode, true);
	//OS_CondVarBroadcast(job_scheduler->cond_begin);
}

internal void
JOB_SpinModeDisable(void)
{
	OS_AtomicStoreU32(&job_scheduler->atomic_spin_mode, false);
}

internal b32
JOB_IsMainThread(void)
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
JOB_FiberYield(void)
{
	job_current_worker->current_fiber->finished = false;
	
	OS_SwitchToFiber(job_current_worker->fiber_handle);
}

/*
 * Switch back to the worker's scheduler fiber while marking
 * the job as finished, so the fiber itself is returned
 * back to the free fiber pool for use later.
 */
internal void
JOB_FiberCompleted(void)
{
	job_current_worker->current_fiber->finished = true;
	
	OS_SwitchToFiber(job_current_worker->fiber_handle);
}

internal JOB_Fiber *
JOB_FiberFetchFree(void)
{
	OS_SpinLockAcquire(&job_fiber_pool_lock);
 
	JOB_Fiber *fiber = job_fiber_pool_head;
	
	if (fiber)
		job_fiber_pool_head = fiber->next;

	OS_SpinLockRelease(&job_fiber_pool_lock);
 
	if (fiber)
		fiber->next = NULL;
 
	return fiber;
}

internal void
JOB_FiberReturn(JOB_Fiber *fiber)
{
	MemZeroStruct(&fiber->job);
	
	fiber->counter	= NULL;
	fiber->finished = true;
 
	OS_SpinLockAcquire(&job_fiber_pool_lock);
 
	fiber->next = job_fiber_pool_head;
	job_fiber_pool_head = fiber;
 
	OS_SpinLockRelease(&job_fiber_pool_lock);
}

internal OS_Handle
JOB_GetCurrentFiberHandle(void)
{
	if (job_current_worker && job_current_worker->current_fiber)
		return job_current_worker->current_fiber->handle;
	else
		return OS_HandleNull();
}

internal JOB_Request *
JOB_TryGetRequest(void)
{
	for (u32 i = JOB_Priority_COUNT - 1; i >= 0; i--)
	{
		JOB_Queue *queue = &job_scheduler->queues[i];
		
		if (queue->atomic_added_task_count <= queue->atomic_taken_task_count)
			continue;

		OS_SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = queue->atomic_taken_task_count;
		u32 a = queue->atomic_added_task_count;
 
		JOB_Request *request = NULL;
		if (a > t)
		{
			request = &queue->requests[t % JOB_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_task_count = t + 1;
		}

		OS_SpinLockRelease(&queue->atomic_spinlock);
		
		if (request)
			return request;
	}
 
	return NULL;
}

internal JOB_Fiber *
JOB_TryGetWaitingFiber(void)
{
	for (u32 i = JOB_Priority_COUNT - 1; i >= 0; i--)
	{
		JOB_Queue *queue = &job_scheduler->queues[i];
 
		if (queue->atomic_added_waiting_count <= queue->atomic_taken_waiting_count)
			continue;
 
		OS_SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = queue->atomic_taken_waiting_count;
		u32 a = queue->atomic_added_waiting_count;
 
		JOB_Fiber *fiber = NULL;
		
		if (a > t)
		{
			fiber = queue->waiting[t % JOB_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_waiting_count = t + 1;
		}

		OS_SpinLockRelease(&queue->atomic_spinlock);
 
		if (fiber)
			return fiber;
	}
 
	return NULL;
}

internal b32
JOB_RequestAvailable(void)
{
	for (u32 i = 0; i < JOB_Priority_COUNT; i++)
	{
		JOB_Queue *queue = &job_scheduler->queues[i];
		
		if (queue->atomic_added_task_count > queue->atomic_taken_task_count)
			return true;
		
		if (queue->atomic_added_waiting_count > queue->atomic_taken_waiting_count)
			return true;
	}
	
	return false;
}

internal void
JOB_InitAndSelect(Arena *arena, JOB_Scheduler *scheduler)
{
	job_scheduler = scheduler;

	MemZeroStruct(scheduler);
 
	scheduler->mutex	  = OS_MutexCreate();
	scheduler->cond_begin = OS_CondVarCreate();
 
	OS_AtomicStoreU32(&scheduler->atomic_running, true);
 
	for (u32 i = 0; i < JOB_MAX_CONCURRENT_FIBERS; i++)
	{
		JOB_Fiber *fiber = &scheduler->atomic_fiber_pool[i];
 
		fiber->handle = OS_FiberCreate(0, JOB_FiberEntry, NULL);
 
		for (u32 j = 0; j < ArraySize(fiber->scratch_arenas); j++)
		{
			Arena *scratch = ArenaPushArray(arena, Arena, 1);
			*scratch = ArenaInitArena(arena, JOB_FIBER_SCRATCH_SIZE);
			fiber->tctx.scratch_ring[j] = scratch;
		}

		// Give the fiber to the freelist.
		JOB_FiberReturn(fiber);
	}

	// Try to leave at least one free core available.
	const u32 desired_workers = MaxValue(1, OS_GetNumCores() - 1);

	scheduler->worker_count = MinValue(desired_workers, JOB_MAX_WORKERS);
 
	scheduler->workers[0].id = 0;
	scheduler->workers[0].thread_handle = OS_GetCurrentThreadHandle();
 
	for (u32 i = 1; i < scheduler->worker_count; i++)
	{
		JOB_Worker *worker = &scheduler->workers[i];
		worker->id = i;
		worker->thread_handle = OS_ThreadCreate(JOB_SchedulerThreadEntry, worker);
	}
}

internal void
JOB_Shutdown(void)
{
	// Join worker threads (worker 0 is the main thread so no join needed).
	for (u32 i = 1; i < job_scheduler->worker_count; i++)
		OS_ThreadJoin(job_scheduler->workers[i].thread_handle);
 
	// Release all fibers.
	for (u32 i = 0; i < JOB_MAX_CONCURRENT_FIBERS; i++)
		OS_FiberDelete(job_scheduler->atomic_fiber_pool[i].handle);

	// Destroy OS synchronisation primitives.
	OS_MutexDestroy(job_scheduler->mutex);
	OS_CondVarDestroy(job_scheduler->cond_begin);

	// Unselect.
	job_scheduler = NULL;
}

internal void
JOB_SchedulerThreadEntry(void *param)
{
	JOB_Worker *worker = param;

	job_current_worker = worker;
	worker->fiber_handle = OS_ConvertThreadToFiber();
 
	// Force each worker to stay on a single core.
	OS_ThreadSetAffinity(worker->thread_handle, 1ull << worker->id);
 
	while (OS_AtomicLoadU32(&job_scheduler->atomic_running))
	{
		if (JOB_IsMainThread())
			job_MessagePump();
 
		// Check for a fiber to resume.
		JOB_Fiber *waiting_fiber = JOB_TryGetWaitingFiber();
		if (waiting_fiber)
		{
			worker->current_fiber = waiting_fiber;
			
			OS_SwitchToFiber(waiting_fiber->handle);
 
			if (worker->current_fiber->finished)
				JOB_FiberReturn(worker->current_fiber);
			
			worker->current_fiber = NULL;

			continue;
		}
 
		// Check for a new job to start.
		JOB_Request *request = JOB_TryGetRequest();
		if (request)
		{
			JOB_Fiber *fiber = JOB_FiberFetchFree();
			if (!fiber)
			{
				// Spin until there is a free fiber.
				JOB_SPIN_PAUSE();
				continue;
			}
 
			fiber->EntryPoint = request->EntryPoint;
			fiber->param      = request->param;
			fiber->priority	  = request->priority;
			fiber->counter    = request->counter;
			fiber->finished   = false;
 
			worker->current_fiber = fiber;

			OS_SwitchToFiber(fiber->handle);
 
			if (worker->current_fiber->finished)
				JOB_FiberReturn(worker->current_fiber);

			worker->current_fiber = NULL;

			continue;
		}
 
		// Wait until we have more work to do.

		// Spin Mode
		if (OS_AtomicLoadU32(&job_scheduler->atomic_spin_mode))
		{
			while (!JOB_RequestAvailable() && OS_AtomicLoadU32(&job_scheduler->atomic_running))
			{
				if (JOB_IsMainThread())
					job_message_pump();
				
				if (!OS_AtomicLoadU32(&job_scheduler->atomic_spin_mode))
					break;
				
				JOB_SPIN_PAUSE();
			}
		}
		else
		{
			// No Spin Mode so resort to regular mutex and
			// waiting on a condition variable.
			//
			// Main thread can't sleep on condition variable
			// because it must keep pumping messages.
			if (!JOB_IsMainThread())
			{	
				OS_MutexLock(job_scheduler->mutex);
				
				if (!JOB_RequestAvailable() && OS_AtomicLoadU32(&job_scheduler->atomic_running))
				{
					OS_CondVarWait(job_scheduler->cond_begin, job_scheduler->mutex);
				}
				
				OS_MutexUnlock(job_scheduler->mutex);
			}
		}
	}
 
	OS_ConvertFiberToThread();
}

/*
 * The entry point for all fibers.
 */
internal void
JOB_FiberEntry(void *param)
{
	while (true)
	{
		JOB_Fiber *f = job_current_worker->current_fiber;
		JOB_Counter *c = f->counter;

		if (f->EntryPoint)
			f->EntryPoint(f->param);

		if (c)
			JOB_CounterDecrement(c, 1);

		JOB_FiberCompleted();
	}
}

internal void
JOB_Enter(void (*MessagePump)(void))
{
	job_MessagePump = MessagePump;
	JOB_SchedulerThreadEntry(&job_scheduler->workers[0]);
}

internal void
JOB_Halt(void)
{
	OS_AtomicStoreU32(&job_scheduler->atomic_running, false);
	OS_CondVarBroadcast(job_scheduler->cond_begin);
}

internal JOB_Counter *
JOB_CounterAlloc(Arena *arena, u32 initial_count)
{
	JOB_Counter *counter = ArenaPushArray(arena, JOB_Counter, 1);
	counter->atomic_count = initial_count;
	
	return counter;
}

internal void
JOB_CounterLock(JOB_Counter *counter)
{
	OS_SpinLockAcquire(&counter->atomic_spinlock);
}

internal void
JOB_CounterUnlock(JOB_Counter *counter)
{
	OS_SpinLockRelease(&counter->atomic_spinlock);
}

internal void
JOB_CounterIncrement(JOB_Counter *counter, u32 n)
{
	OS_AtomicAddU32(&counter->atomic_count, n);
}

/*
 * Decrement the counter, but if we hit zero we have to collect all of the
 * waiting fibers and kick them off.
 */
internal void
JOB_CounterDecrement(JOB_Counter *counter, u32 n)
{
	JOB_CounterLock(counter);

	u32 atomic_count = OS_AtomicLoadU32(&counter->atomic_count);
 
	AssertTrue(atomic_count >= n);

	OS_AtomicSubU32(&counter->atomic_count, n);

	atomic_count -= n;
 
	u32 kick_count = 0;
	JOB_Fiber *to_kick[JOB_COUNTER_MAX_WAITING] = {0};
 
	if (atomic_count == 0 && counter->waiting_count > 0)
	{
		kick_count = counter->waiting_count;
		MemCopy(to_kick, counter->waiting, kick_count * sizeof(JOB_Fiber *));
		counter->waiting_count = 0;
	}
 
	JOB_CounterUnlock(counter);
 
	for (u32 i = 0; i < kick_count; i++)
		JOB_PushWaitingFiber(to_kick[i]);
 
	if (kick_count > 0)
		OS_CondVarBroadcast(job_scheduler->cond_begin);
}

internal void
JOB_Push(const JOB_Request *request)
{
	JOB_Queue *queue = &job_scheduler->queues[request->priority];

	while (true)
	{
		OS_SpinLockAcquire(&queue->atomic_spinlock);
		
		u32 t = queue->atomic_taken_task_count;
		u32 a = queue->atomic_added_task_count;
 
		if ((a - t) >= JOB_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			OS_SpinLockRelease(&queue->atomic_spinlock);
			
			JOB_SPIN_PAUSE();
		}
		else
		{
			queue->requests[a % JOB_MAX_JOBS_PER_QUEUE] = *request;
			queue->atomic_added_task_count = a + 1;

			OS_SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

internal void
JOB_PushWaitingFiber(JOB_Fiber *fiber)
{
	JOB_Queue *queue = &job_scheduler->queues[fiber->priority];
 
	while (true)
	{
		OS_SpinLockAcquire(&queue->atomic_spinlock);
 
		u32 t = queue->atomic_taken_waiting_count;
		u32 a = queue->atomic_added_waiting_count;
 
		if ((a - t) >= JOB_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			OS_SpinLockRelease(&queue->atomic_spinlock);
			
			JOB_SPIN_PAUSE();
		}
		else
		{
			queue->waiting[a % JOB_MAX_JOBS_PER_QUEUE] = fiber;
			queue->atomic_added_waiting_count = a + 1;
			
			OS_SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

internal void
JOB_Yield(JOB_Counter *counter, u32 value)
{
	while (true)
	{
		JOB_CounterLock(counter);
 
		if (counter->atomic_count == value)
		{
			JOB_CounterUnlock(counter);
			return;
		}
 
		AssertTrue(counter->waiting_count < JOB_COUNTER_MAX_WAITING);
		
		counter->waiting[counter->waiting_count++] = job_current_worker->current_fiber;
 
		JOB_CounterUnlock(counter);
 
		JOB_FiberYield();
	}
}

internal void
JOB_Kick(const JOB_Decl *decl, JOB_Counter *counter)
{
	if (counter)
		JOB_CounterIncrement(counter, 1);
 
	JOB_Request request = {0};
	request.EntryPoint  = decl->EntryPoint;
	request.param       = decl->param;
	request.priority    = decl->priority;
	request.counter     = counter;
 
	JOB_Push(&request);
	
	OS_CondVarSignal(job_scheduler->cond_begin);
}

internal void
JOB_Batch(const JOB_Decl *decls, u32 count, JOB_Counter *counter)
{
	if (counter)
		JOB_CounterIncrement(counter, count);
 
	for (u32 i = 0; i < count; i++)
	{
		JOB_Request request = {0};
		request.EntryPoint  = decls[i].EntryPoint;
		request.param       = decls[i].param;
		request.priority    = decls[i].priority;
		request.counter     = counter;
 
		JOB_Push(&request);
	}
 
	OS_CondVarBroadcast(job_scheduler->cond_begin);
}

typedef struct JOB_ParallelForParam JOB_ParallelForParam;
struct JOB_ParallelForParam
{
	JOB_EntryFor *Inner;
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
JOB_For(u32 count, JOB_EntryFor *fn, JOB_Priority priority, u32 batch_size)
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
 
		params[i].Inner      = fn;
		params[i].base_index = base_index;
		params[i].loop_size	 = loop_size;
 
		decls[i].EntryPoint	 = JOB_ParallelForBatchEntry;
		decls[i].param		 = &params[i];
		decls[i].priority	 = priority;
	}
 
	JOB_Counter counter = {0};
	JOB_Batch(decls, job_count, &counter);
	JOB_Yield(&counter, 0);
 
	ScratchRelease(&scratch);
}

internal Arena *
JOB_GetScratch(Arena **conflicts, u32 conflict_count)
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
