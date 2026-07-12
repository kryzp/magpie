
static __declspec(thread) J_W32_Worker *job_current_worker;

static void J_W32_SpinModeEnable(J_W32_Scheduler *scheduler)
{
	osapi->AtomicStoreI32(&scheduler->atomic_spin_mode, 1);
	//osapi->CondVarBroadcast(scheduler->cond_begin);
}

static void J_W32_SpinModeDisable(J_W32_Scheduler *scheduler)
{
	osapi->AtomicStoreI32(&scheduler->atomic_spin_mode, 0);
}

static b32 J_W32_IsMainThread(J_W32_Scheduler *scheduler)
{
	AssertTrue(job_current_worker);
	
	return job_current_worker->id == 0;
}

/*
 * Switch back to the worker's scheduler fiber without marking
 * the job as finished, so the fiber itself won't be returned
 * back to the free fiber pool.
 */
static void J_W32_FiberYield(J_W32_Scheduler *scheduler)
{
	job_current_worker->current_fiber->finished = false;
	
	osapi->SwitchToFiber(job_current_worker->fiber_handle);
}

/*
 * Switch back to the worker's scheduler fiber while marking
 * the job as finished, so the fiber itself is returned
 * back to the free fiber pool for use later.
 */
static void J_W32_FiberCompleted(J_W32_Scheduler *scheduler)
{
	job_current_worker->current_fiber->finished = true;
	
	osapi->SwitchToFiber(job_current_worker->fiber_handle);
}

static J_W32_Fiber *J_W32_FiberFetchFree(J_W32_Scheduler *scheduler)
{
	osapi->SpinLockAcquire(&scheduler->fiber_pool_spinlock);
 
	J_W32_Fiber *fiber = scheduler->fiber_pool_head;
	
	if (fiber)
		scheduler->fiber_pool_head = fiber->next_free;

	osapi->SpinLockRelease(&scheduler->fiber_pool_spinlock);
 
	if (fiber)
		fiber->next_free = NULL;
 
	return fiber;
}

static void J_W32_FiberReturn(J_W32_Scheduler *scheduler, J_W32_Fiber *fiber)
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

static OS_Handle J_W32_GetCurrentFiberHandle(J_W32_Scheduler *scheduler)
{
	if (job_current_worker && job_current_worker->current_fiber)
		return job_current_worker->current_fiber->handle;
	else
		return OS_HandleNull();
}

static J_W32_Request *J_W32_TryGetRequest(J_W32_Scheduler *scheduler)
{
	if (J_W32_IsMainThread(scheduler))
	{
		J_W32_Queue *mtq = &scheduler->main_thread_queue;
		
		if (mtq->atomic_added_task_count > mtq->atomic_taken_task_count)
		{
			osapi->SpinLockAcquire(&mtq->atomic_spinlock);
 
			i32 t = osapi->AtomicCompareExchangeI32(&mtq->atomic_taken_task_count, 0, 0);
			i32 a = osapi->AtomicCompareExchangeI32(&mtq->atomic_added_task_count, 0, 0);
 
			J_W32_Request *request = NULL;
		
			if (a > t)
			{
				request = &mtq->requests[t % J_W32_MAX_JOBS_PER_QUEUE];
				mtq->atomic_taken_task_count = t + 1;
			}

			osapi->SpinLockRelease(&mtq->atomic_spinlock);
		
			if (request)
				return request;
		}
	}
	
	for (i32 i = J_Priority_COUNT - 1; i >= 0; i--)
	{
		J_W32_Queue *queue = &scheduler->queues[i];
		
		if (queue->atomic_added_task_count <= queue->atomic_taken_task_count)
			continue;

		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		i32 t = osapi->AtomicCompareExchangeI32(&queue->atomic_taken_task_count, 0, 0);
		i32 a = osapi->AtomicCompareExchangeI32(&queue->atomic_added_task_count, 0, 0);
 
		J_W32_Request *request = NULL;
		
		if (a > t)
		{
			request = &queue->requests[t % J_W32_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_task_count = t + 1;
		}

		osapi->SpinLockRelease(&queue->atomic_spinlock);
		
		if (request)
			return request;
	}
 
	return NULL;
}

static J_W32_Fiber *J_W32_TryGetWaitingFiber(J_W32_Scheduler *scheduler)
{
	for (i32 i = J_Priority_COUNT - 1; i >= 0; i--)
	{
		J_W32_Queue *queue = &scheduler->queues[i];
 
		if (queue->atomic_added_waiting_count <= queue->atomic_taken_waiting_count)
			continue;
 
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		i32 t = osapi->AtomicCompareExchangeI32(&queue->atomic_taken_waiting_count, 0, 0);
		i32 a = osapi->AtomicCompareExchangeI32(&queue->atomic_added_waiting_count, 0, 0);
 
		J_W32_Fiber *fiber = NULL;
		
		if (a > t)
		{
			fiber = queue->waiting[t % J_W32_MAX_JOBS_PER_QUEUE];
			queue->atomic_taken_waiting_count = t + 1;
		}

		osapi->SpinLockRelease(&queue->atomic_spinlock);
 
		if (fiber)
			return fiber;
	}
 
	return NULL;
}

static b32 J_W32_RequestAvailable(J_W32_Scheduler *scheduler)
{
	for (u32 i = 0; i < J_Priority_COUNT; i++)
	{
		J_W32_Queue *queue = &scheduler->queues[i];

		i32 added_task_count = osapi->AtomicCompareExchangeI32(&queue->atomic_added_task_count, 0, 0);
		i32 taken_task_count = osapi->AtomicCompareExchangeI32(&queue->atomic_taken_task_count, 0, 0);

		i32 added_waiting_count = osapi->AtomicCompareExchangeI32(&queue->atomic_added_waiting_count, 0, 0);
		i32 taken_waiting_count = osapi->AtomicCompareExchangeI32(&queue->atomic_taken_waiting_count, 0, 0);
		
		if ((added_task_count > taken_task_count) || (added_waiting_count > taken_waiting_count))
			return true;
	}
	
	return false;
}

static void J_W32_Init(J_W32_Scheduler *scheduler, LOG_Channel log_channel)
{
	MemZeroStruct(scheduler);

	scheduler->log_channel = log_channel;
 
	scheduler->thread_mutex = osapi->ThreadMutexCreate();
	scheduler->thread_cond_begin = osapi->ThreadCondVarCreate();
 
	osapi->AtomicStoreI32(&scheduler->atomic_running, 1);
	
	for (u32 j = 0; j < J_W32_FIBER_SCRATCH_RING_SIZE; j++)
		scheduler->fallback_scratch_ring[j] = ArenaAlloc(J_W32_FIBER_SCRATCH_SIZE);
 
	for (u32 i = 0; i < J_W32_MAX_CONCURRENT_FIBERS; i++)
	{
		J_W32_Fiber *fiber = &scheduler->atomic_fiber_storage[i];

		// This is purely an aesthetic thing but since we push
		// the fibers to the front, the ones at the front get
		// selected first so we assign id's in reverse so at
		// the end the "front" fiber has id 0, then 1, etc...
		fiber->id = J_W32_MAX_CONCURRENT_FIBERS - i - 1;

		fiber->handle = osapi->FiberCreate(0, J_W32_FiberEntry, scheduler);
 
		for (u32 j = 0; j < J_W32_FIBER_SCRATCH_RING_SIZE; j++)
			fiber->scratch_arenas[j] = ArenaAlloc(J_W32_FIBER_SCRATCH_SIZE);

		// Give the fiber to the freelist.
		J_W32_FiberReturn(scheduler, fiber);
	}

	// Try to leave at least one free core available.
	const u32 desired_workers = MaxValue(1, osapi->GetNumCores() - 1);

	scheduler->worker_count = MinValue(desired_workers, J_W32_MAX_WORKERS);

	scheduler->workers[0].id = 0;
	scheduler->workers[0].thread_handle = osapi->GetCurrentThreadHandle();
	scheduler->workers[0].scheduler = scheduler;
 
	for (u32 i = 1; i < scheduler->worker_count; i++)
	{
		J_W32_Worker *worker = &scheduler->workers[i];
		worker->id = i;
		worker->thread_handle = osapi->ThreadCreate(J_W32_SchedulerThreadEntry, worker);
		worker->scheduler = scheduler;
	}

	//scheduler->tls_worker_slot = osapi->TLSAlloc();
	//osapi->TLSSet(scheduler->tls_worker_slot, NULL);

	DebugLogI(scheduler->log_channel, "Initialized.");
}

static void J_W32_Shutdown(J_W32_Scheduler *scheduler)
{
	//osapi->TLSFree(scheduler->tls_worker_slot);
	
	// worker 0 is the main thread so no join needed.
	for (u32 i = 1; i < scheduler->worker_count; i++)
		osapi->ThreadJoin(scheduler->workers[i].thread_handle);
 
	// release fibers
	for (u32 i = 0; i < J_W32_MAX_CONCURRENT_FIBERS; i++)
	{
		for (u32 j = 0; j < J_W32_FIBER_SCRATCH_RING_SIZE; j++)
			ArenaRelease(&scheduler->atomic_fiber_storage[i].scratch_arenas[j]);

		osapi->FiberDelete(scheduler->atomic_fiber_storage[i].handle);
	}
	
	for (u32 j = 0; j < J_W32_FIBER_SCRATCH_RING_SIZE; j++)
		ArenaRelease(&scheduler->fallback_scratch_ring[j]);

	osapi->ThreadMutexDestroy(scheduler->thread_mutex);
	osapi->ThreadCondVarDestroy(scheduler->thread_cond_begin);

	DebugLogI(scheduler->log_channel, "Destroyed.");
}

static void J_W32_SchedulerThreadEntry(void *param)
{
	J_W32_Worker *worker = param;
	J_W32_Scheduler *scheduler = worker->scheduler;

	//osapi->TLSSet(scheduler->tls_worker_slot, worker);

	job_current_worker = worker;
	worker->fiber_handle = osapi->ConvertThreadToFiber();
 
	// Force each worker to stay on a single core.
	osapi->ThreadSetAffinity(osapi->GetCurrentThreadHandle(), 1ull << worker->id);
 
	while (osapi->AtomicCompareExchangeI32(&scheduler->atomic_running, 0, 0))
	{
		// Check for a fiber to resume.
		J_W32_Fiber *waiting_fiber = J_W32_TryGetWaitingFiber(scheduler);
		
		if (waiting_fiber)
		{
			worker->current_fiber = waiting_fiber;
			
			osapi->SwitchToFiber(waiting_fiber->handle);
 
			if (worker->current_fiber->finished)
				J_W32_FiberReturn(scheduler, worker->current_fiber);
			
			worker->current_fiber = NULL;

			continue;
		}
 
		// Check for a new job to start.
		J_W32_Request *request = J_W32_TryGetRequest(scheduler);
		
		if (request)
		{
			J_W32_Fiber *fiber = J_W32_FiberFetchFree(scheduler);
			
			if (!fiber)
			{
				// Spin until there is a free fiber.
				OS_SPIN_PAUSE();
				continue;
			}
 
			fiber->EntryPoint = request->EntryPoint;
			fiber->param = request->param;
			fiber->priority	= request->priority;
			fiber->counter = request->counter;
			fiber->finished = false;
 
			worker->current_fiber = fiber;
			
			osapi->SwitchToFiber(fiber->handle);
 
			if (worker->current_fiber->finished)
				J_W32_FiberReturn(scheduler, worker->current_fiber);

			worker->current_fiber = NULL;

			continue;
		}

		// Wait until we have more work to do...
		b32 is_main_thread = J_W32_IsMainThread(scheduler);
		
		if (is_main_thread || osapi->AtomicCompareExchangeI32(&scheduler->atomic_spin_mode, 0, 0))
		{
			// Main thread can't sleep on condition variable
			// because it must keep pumping messages.
			
			while (!J_W32_RequestAvailable(scheduler) && osapi->AtomicCompareExchangeI32(&scheduler->atomic_running, 0, 0))
			{
				if (scheduler->OnMainThreadIdle && is_main_thread)
					scheduler->OnMainThreadIdle(scheduler->main_thread_idle_ctx);
				
				if (!osapi->AtomicCompareExchangeI32(&scheduler->atomic_spin_mode, 0, 0))
					break;
				
				OS_SPIN_PAUSE();
			}
		}
		else
		{
			// No Spin Mode so resort to regular mutex and
			// waiting on a condition variable.
			
			osapi->ThreadMutexLock(scheduler->thread_mutex);
				
			while (!J_W32_RequestAvailable(scheduler) && osapi->AtomicCompareExchangeI32(&scheduler->atomic_running, 0, 0))
			{
				osapi->ThreadCondVarWait(scheduler->thread_cond_begin, scheduler->thread_mutex);
			}
				
			osapi->ThreadMutexUnlock(scheduler->thread_mutex);
		}
	}
 
	osapi->ConvertFiberToThread();
}

/*
 * The entry point for all fibers.
 */
static void J_W32_FiberEntry(void *param)
{
	J_W32_Scheduler *scheduler = param;
	
	for (;;)
	{
		J_W32_Fiber *f = job_current_worker->current_fiber;
		J_W32_Counter *c = f->counter;

		if (f->EntryPoint)
			f->EntryPoint(f->param);
		
		if (c)
			J_W32_CounterDecrement(scheduler, c, 1);

		J_W32_FiberCompleted(scheduler);
	}
}

static void J_W32_Enter(J_W32_Scheduler *scheduler, void (*OnMainThreadIdle)(void *ctx), void *main_thread_idle_ctx)
{
	scheduler->OnMainThreadIdle = OnMainThreadIdle;
	scheduler->main_thread_idle_ctx = main_thread_idle_ctx;
	
	J_W32_SchedulerThreadEntry(&scheduler->workers[0]);
}

static void J_W32_Halt(J_W32_Scheduler *scheduler)
{
	osapi->AtomicStoreI32(&scheduler->atomic_running, false);
	osapi->ThreadCondVarBroadcast(scheduler->thread_cond_begin);
}

static J_W32_Context J_W32_GetContext(J_W32_Scheduler *scheduler)
{
	J_W32_Context ctx = {0};
	ctx.worker_id = 0;
	ctx.fiber_id = -1;
	
	J_W32_Worker *worker = job_current_worker;

	if (worker)
	{
		ctx.worker_id = worker->id;
		ctx.fiber_id = worker->current_fiber ? worker->current_fiber->id : -1;
	}

	return ctx;
}

static void J_W32_CounterInit(J_W32_Counter *counter, i32 initial_count)
{
	counter->atomic_count = initial_count;
}

static void J_W32_CounterIncrement(J_W32_Counter *counter, i32 n)
{
	osapi->AtomicAddI32(&counter->atomic_count, n);
}

/*
 * Decrement the counter, but if we hit zero we have to collect all of the
 * waiting fibers and kick them off.
 */
static void J_W32_CounterDecrement(J_W32_Scheduler *scheduler, J_W32_Counter *counter, i32 n)
{
	osapi->SpinLockAcquire(&counter->atomic_spinlock);

	i32 atomic_count = osapi->AtomicCompareExchangeI32(&counter->atomic_count, 0, 0);
 
	AssertTrue(atomic_count >= n);

	osapi->AtomicAddI32(&counter->atomic_count, -n);

	atomic_count -= n;
 
	u32 kick_count = 0;
	J_W32_Fiber *to_kick[J_W32_COUNTER_MAX_WAITING] = {0};
 
	if (atomic_count == 0 && counter->waiting_count > 0)
	{
		kick_count = counter->waiting_count;
		MemCopy(to_kick, counter->waiting, kick_count * sizeof(J_W32_Fiber *));
		counter->waiting_count = 0;
	}
 
	osapi->SpinLockRelease(&counter->atomic_spinlock);
 
	for (u32 i = 0; i < kick_count; i++)
		J_W32_PushWaitingFiber(scheduler, to_kick[i]);
 
	if (kick_count > 0)
	{
		osapi->ThreadMutexLock(scheduler->thread_mutex);
		osapi->ThreadCondVarBroadcast(scheduler->thread_cond_begin);
		osapi->ThreadMutexUnlock(scheduler->thread_mutex);
	}
}

static u32 J_W32_CounterValue(J_W32_Counter *counter)
{
	return (u32)osapi->AtomicCompareExchangeI32(&counter->atomic_count, 0, 0);
}

static void J_W32_FiberMutexLock(J_W32_Scheduler *scheduler, J_W32_FiberMutex *m)
{
	for (;;)
	{
		if (osapi->AtomicCompareExchangeI32(&m->atomic_mutex_state, 1, 0) == 0)
			return;

		osapi->SpinLockAcquire(&m->atomic_spinlock);

		if (osapi->AtomicCompareExchangeI32(&m->atomic_mutex_state, 1, 0) == 0)
		{
			osapi->SpinLockRelease(&m->atomic_spinlock);
			return;
		}

		AssertTrue(m->waiting_count < J_W32_COUNTER_MAX_WAITING);
		m->waiting[m->waiting_count++] = job_current_worker->current_fiber;

		osapi->SpinLockRelease(&m->atomic_spinlock);

		J_W32_FiberYield(scheduler);
	}
}

static void J_W32_FiberMutexUnlock(J_W32_Scheduler *scheduler, J_W32_FiberMutex *m)
{
	osapi->SpinLockAcquire(&m->atomic_spinlock);

	J_W32_Fiber *next = NULL;

	if (m->waiting_count > 0)
	{
		next = m->waiting[0];

		for (u32 i = 1; i < m->waiting_count; i++)
			m->waiting[i - 1] = m->waiting[i];
		
		m->waiting_count--;
	}

	osapi->AtomicStoreI32(&m->atomic_mutex_state, 0);

	osapi->SpinLockRelease(&m->atomic_spinlock);

	if (next)
	{
		J_W32_PushWaitingFiber(scheduler, next);

		osapi->ThreadMutexLock(scheduler->thread_mutex);
		osapi->ThreadCondVarBroadcast(scheduler->thread_cond_begin);
		osapi->ThreadMutexUnlock(scheduler->thread_mutex);
	}
}

static void J_W32_FiberCondVarWait(J_W32_Scheduler *scheduler, J_W32_FiberCondVar *cv, J_W32_FiberMutex *mutex)
{
	osapi->SpinLockAcquire(&cv->atomic_spinlock);

	AssertTrue(cv->waiting_count < J_W32_COUNTER_MAX_WAITING);
	cv->waiting[cv->waiting_count++] = job_current_worker->current_fiber;

	osapi->SpinLockRelease(&cv->atomic_spinlock);

	J_W32_FiberMutexUnlock(scheduler, mutex);
	J_W32_FiberYield(scheduler);
	J_W32_FiberMutexLock(scheduler, mutex);
}

static void J_W32_FiberCondVarSignal(J_W32_Scheduler *scheduler, J_W32_FiberCondVar *cv)
{
	osapi->SpinLockAcquire(&cv->atomic_spinlock);

	J_W32_Fiber *next = NULL;
	
	if (cv->waiting_count > 0)
	{
		next = cv->waiting[0];
		
		for (u32 i = 1; i < cv->waiting_count; i++)
			cv->waiting[i - 1] = cv->waiting[i];
		
		cv->waiting_count--;
	}

	osapi->SpinLockRelease(&cv->atomic_spinlock);

	if (next)
	{
		J_W32_PushWaitingFiber(scheduler, next);

		osapi->ThreadMutexLock(scheduler->thread_mutex);
		osapi->ThreadCondVarBroadcast(scheduler->thread_cond_begin);
		osapi->ThreadMutexUnlock(scheduler->thread_mutex);
	}
}

static void J_W32_FiberCondVarBroadcast(J_W32_Scheduler *scheduler, J_W32_FiberCondVar *cv)
{
	osapi->SpinLockAcquire(&cv->atomic_spinlock);

	u32 kick_count = cv->waiting_count;
	J_W32_Fiber *to_kick[J_W32_COUNTER_MAX_WAITING] = {0};
	MemCopy(to_kick, cv->waiting, kick_count * sizeof(J_W32_Fiber *));
	cv->waiting_count = 0;

	osapi->SpinLockRelease(&cv->atomic_spinlock);

	for (u32 i = 0; i < kick_count; i++)
		J_W32_PushWaitingFiber(scheduler, to_kick[i]);

	if (kick_count > 0)
	{
		osapi->ThreadMutexLock(scheduler->thread_mutex);
		osapi->ThreadCondVarBroadcast(scheduler->thread_cond_begin);
		osapi->ThreadMutexUnlock(scheduler->thread_mutex);
	}
}

static void J_W32_Push(J_W32_Scheduler *scheduler, const J_W32_Request *request)
{
	J_W32_Queue *queue = &scheduler->queues[request->priority];

	if (request->flags & J_Flag_MainThreadOnly)
		queue = &scheduler->main_thread_queue;
	
	for (;;)
	{
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
		
		i32 t = osapi->AtomicCompareExchangeI32(&queue->atomic_taken_task_count, 0, 0);
		i32 a = osapi->AtomicCompareExchangeI32(&queue->atomic_added_task_count, 0, 0);
 
		if ((a - t) >= J_W32_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			OS_SPIN_PAUSE();
		}
		else
		{
			queue->requests[a % J_W32_MAX_JOBS_PER_QUEUE] = *request;
			queue->atomic_added_task_count = a + 1;

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

static void J_W32_PushWaitingFiber(J_W32_Scheduler *scheduler, J_W32_Fiber *fiber)
{
	J_W32_Queue *queue = &scheduler->queues[fiber->priority];
 
	for (;;)
	{
		osapi->SpinLockAcquire(&queue->atomic_spinlock);
 
		i32 t = osapi->AtomicCompareExchangeI32(&queue->atomic_taken_waiting_count, 0, 0);
		i32 a = osapi->AtomicCompareExchangeI32(&queue->atomic_added_waiting_count, 0, 0);
 
		if ((a - t) >= J_W32_MAX_JOBS_PER_QUEUE)
		{
			// Queue is full.
			// Release and spin until space opens up.

			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			OS_SPIN_PAUSE();
		}
		else
		{
			queue->waiting[a % J_W32_MAX_JOBS_PER_QUEUE] = fiber;
			queue->atomic_added_waiting_count = a + 1;
			
			osapi->SpinLockRelease(&queue->atomic_spinlock);
			
			break;
		}
	}
}

static void J_W32_Yield(J_W32_Scheduler *scheduler, J_W32_Counter *counter, i32 value)
{
	for (;;)
	{
		osapi->SpinLockAcquire(&counter->atomic_spinlock);
 
		if (counter->atomic_count == value)
		{
			osapi->SpinLockRelease(&counter->atomic_spinlock);
			return;
		}
 
		AssertTrue(counter->waiting_count < J_W32_COUNTER_MAX_WAITING);
		
		counter->waiting[counter->waiting_count++] = job_current_worker->current_fiber;
 
		osapi->SpinLockRelease(&counter->atomic_spinlock);
 
		J_W32_FiberYield(scheduler);
	}
}

static void J_W32_Kick(J_W32_Scheduler *scheduler, const J_Decl *decl, J_W32_Counter *counter)
{
	if (counter)
		J_W32_CounterIncrement(counter, 1);
 
	J_W32_Request request = {0};
	request.EntryPoint = decl->EntryPoint;
	request.param = decl->param;
	request.priority = decl->priority;
	request.flags = decl->flags;
	request.counter = counter;
 
	J_W32_Push(scheduler, &request);

	osapi->ThreadMutexLock(scheduler->thread_mutex);
	osapi->ThreadCondVarSignal(scheduler->thread_cond_begin);
	osapi->ThreadMutexUnlock(scheduler->thread_mutex);
}

static void J_W32_Batch(J_W32_Scheduler *scheduler, const J_Decl *decls, u32 count, J_W32_Counter *counter)
{
	if (counter)
		J_W32_CounterIncrement(counter, count);
 
	for (u32 i = 0; i < count; i++)
	{
		J_W32_Request request = {0};
		request.EntryPoint = decls[i].EntryPoint;
		request.param = decls[i].param;
		request.priority = decls[i].priority;
		request.flags = decls[i].flags;
		request.counter = counter;
 
		J_W32_Push(scheduler, &request);
	}
 
	osapi->ThreadMutexLock(scheduler->thread_mutex);
	osapi->ThreadCondVarBroadcast(scheduler->thread_cond_begin);
	osapi->ThreadMutexUnlock(scheduler->thread_mutex);
}

typedef struct J_W32_ParallelForParam J_W32_ParallelForParam;
struct J_W32_ParallelForParam
{
	J_EntryForFn *Inner;
	u32 base_index;
	u32 loop_size;
};

static J_ENTRY_POINT_DEF(J_W32_ParallelForBatchEntry)
{
	J_W32_ParallelForParam *p = param;
	
	for (u32 i = 0; i < p->loop_size; i++)
		p->Inner(p->base_index + i);
}

static void J_W32_For(J_W32_Scheduler *scheduler, u32 count, J_EntryForFn *fn, J_Priority priority, u32 batch_size)
{
	if (count == 0 || batch_size == 0)
		return;
 
	u32 job_count = (count + batch_size - 1) / batch_size;
 
	ScratchArena scratch = ScratchBegin(NULL, 0);
 
	J_W32_ParallelForParam *params = ArenaPushArray(scratch.arena, J_W32_ParallelForParam, job_count);
	J_Decl *decls = ArenaPushArray(scratch.arena, J_Decl, job_count);
 
	for (u32 i = 0; i < job_count; i++)
	{
		u32 base_index = batch_size * i;
		u32 loop_size  = count - base_index;
		
		if (loop_size > batch_size)
			loop_size = batch_size;
 
		params[i].Inner = fn;
		params[i].base_index = base_index;
		params[i].loop_size = loop_size;
 
		decls[i].EntryPoint	= J_W32_ParallelForBatchEntry;
		decls[i].param = &params[i];
		decls[i].priority = priority;
		decls[i].flags = J_Flag_None;
	}
 
	J_W32_Counter counter = {0};
	J_W32_Batch(scheduler, decls, job_count, &counter);
	J_W32_Yield(scheduler, &counter, 0);
 
	ScratchRelease(&scratch);
}

static Arena *J_W32_GetScratch(J_W32_Scheduler *scheduler, Arena * const *conflicts, u32 conflict_count)
{
	Arena *arena = NULL;
	Arena *ring = NULL;

	if (job_current_worker)
	{
		J_W32_Fiber *fiber = job_current_worker->current_fiber;
		ring = fiber->scratch_arenas;
	}
	else
	{
		ring = scheduler->fallback_scratch_ring;
	}

	for (u32 i = 0; i < J_W32_FIBER_SCRATCH_RING_SIZE; i++, ring++)
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
