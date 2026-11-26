#include "os/job.h"
#include "os/fiber.h"
#include "os/platform.h"

#if defined(__linux__) && defined(OS_PIN_WORKER_THREADS)
#include <pthread.h>
#include <sched.h>
#endif

#if defined(_MSC_VER)
#define JOBS_NOINLINE __declspec(noinline)
#else
#define JOBS_NOINLINE __attribute__((noinline))
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
//#define OS_PIN_WORKER_THREADS

static job::Scheduler *scheduler = nullptr;

// FIBERS CAN BE RESUMED BY DIFFERENT WORKER THREADS THAN THE ONES THAT SUSPENDED THEM.
// THIS MEANS THAT THE COMPILER MUST *NEVER*, *EVER*, *EVER*, CACHE THE ADDRESS OF THIS
// ACROSS A FIBER SWITCH.
// THEREFORE, ONLY EVER READ THIS VARIABLE THROUGH GETCURRENTWORKER.
static thread_local job::Worker *current_worker_internal_state = nullptr;

static JOBS_NOINLINE job::Worker *GetCurrentWorker()
{
	job::Worker *worker = current_worker_internal_state;

#if !defined(_MSC_VER)
	// wacky asm trick
	// basically the empty asm stops the compiler from treating the call as pure and merging / hoisting it.
	// cool stuff!!
	__asm__ volatile("" : "+r"(worker));
#endif

	return worker;
}

static void SetCurrentWorker(job::Worker *worker)
{
	current_worker_internal_state = worker;
}

bool job::Queue::TryPopRequest(Request *out)
{
	if (static_cast<unsigned>(added_task_count.load(std::memory_order_acquire)) ==
		static_cast<unsigned>(taken_task_count.load(std::memory_order_acquire))) {
		return false;
	}

	bool popped = false;

	lock.Acquire();
	{
		unsigned t = static_cast<unsigned>(taken_task_count.load(std::memory_order_relaxed));
		unsigned a = static_cast<unsigned>(added_task_count.load(std::memory_order_relaxed));

		if (a != t) {
			*out = requests[t % JOBS_MAX_JOBS_PER_QUEUE];
			taken_task_count.store(static_cast<int>(t + 1), std::memory_order_release);
			popped = true;
		}
	}
	lock.Release();

	return popped;
}

job::JobFiber *job::Queue::TryPopWaiting()
{
	if (added_waiting_count.load(std::memory_order_acquire) ==
		taken_waiting_count.load(std::memory_order_acquire)) {
		return nullptr;
	}

	JobFiber *fiber = nullptr;

	lock.Acquire();
	{
		unsigned t = taken_waiting_count.load(std::memory_order_relaxed);
		unsigned a = added_waiting_count.load(std::memory_order_relaxed);

		if (a != t) {
			fiber = waiting[t % JOBS_MAX_JOBS_PER_QUEUE];
			taken_waiting_count.store(static_cast<int>(t + 1), std::memory_order_release);
		}
	}
	lock.Release();

	return fiber;
}

bool job::Queue::HasWork() const
{
	bool has_tasks =
		added_task_count.load(std::memory_order_acquire) !=
		taken_task_count.load(std::memory_order_acquire);

	bool has_waiting =
		added_waiting_count.load(std::memory_order_acquire) !=
		taken_waiting_count.load(std::memory_order_acquire);

	return has_tasks || has_waiting;
}

job::Counter::Counter(int initial_count)
	: count(initial_count)
	, waiting_count(0)
	, waiting{}
{
}

void job::Counter::Increment(int n)
{
	count.fetch_add(n, std::memory_order_seq_cst);
}

void job::Counter::Decrement(int n)
{
	lock.Acquire();

	int current = count.load(std::memory_order_acquire);

	DebugLogAssert(scheduler->GetLogChannel(), current >= n, "Asking to subtract more than possible.");

	count.fetch_add(-n, std::memory_order_seq_cst);

	current -= n;

	unsigned kick_count = 0;
	JobFiber *to_kick[JOBS_MAX_WAITING];

	if (current == 0 && waiting_count > 0) {
		kick_count = waiting_count;
		memcpy(to_kick, waiting, kick_count * sizeof(JobFiber *));
		waiting_count = 0;
	}

	// note to self:
	// since the counter might live on the stack of the waiter,
	// after this spinlock release it might have gone out of
	// scope so no touching 'this' anymore!
	lock.Release();

	for (unsigned i = 0; i < kick_count; i++) {
		scheduler->PushWaitingFiber(to_kick[i]);
	}

	if (kick_count > 0)
		scheduler->WakeAllWorkers();
}

int job::Counter::Value() const
{
	return count.load(std::memory_order_acquire);
}

void job::Counter::Yield(int value)
{
	for (;;) {
		lock.Acquire();

		if (count.load(std::memory_order_acquire) == value) {
			lock.Release();
			return;
		}

		Worker *worker = GetCurrentWorker();

		DebugLogAssert(scheduler->GetLogChannel(), worker && worker->current_fiber, "Yielding can only be done from inside a job.");
		DebugLogAssert(scheduler->GetLogChannel(), waiting_count < JOBS_MAX_WAITING, "Cannot make more jobs wait on counter!!");

		waiting[waiting_count++] = worker->current_fiber;

		scheduler->SuspendCurrentFiber(&lock);
	}
}

job::FiberMutex::FiberMutex()
	: state(0)
	, waiting_count(0)
{
}

void job::FiberMutex::Lock()
{
	for (;;) {
		int expected = 0;

		if (state.compare_exchange_strong(expected, 1))
			return;

		Worker *worker = GetCurrentWorker();

		if (!worker || !worker->current_fiber) {
			// we are not inside of a job (so in startup, shutdown, or a scheduler loop)
			// therefore there is no fiber to put to sleep, so we'll just spin i guess.

			// TODO is this even necessary??

			OS_SPIN_PAUSE();
			continue;
		}

		lock.Acquire();

		expected = 0;

		if (state.compare_exchange_strong(expected, 1)) {
			lock.Release();
			return;
		}

		DebugLogAssert(scheduler->GetLogChannel(), waiting_count < JOBS_MAX_WAITING, "Cannot make more jobs wait on mutex!!!");

		waiting[waiting_count++] = worker->current_fiber;

		scheduler->SuspendCurrentFiber(&lock);
	}
}

void job::FiberMutex::Unlock()
{
	lock.Acquire();

	JobFiber *next = nullptr;

	if (waiting_count > 0) {
		next = waiting[0];

		for (unsigned i = 1; i < waiting_count; i++)
			waiting[i - 1] = waiting[i];

		waiting_count--;
	}

	state.exchange(0);

	lock.Release();

	if (next) {
		scheduler->PushWaitingFiber(next);
		scheduler->WakeAllWorkers();
	}
}

job::FiberCondVar::FiberCondVar()
	: waiting_count(0)
{
}

void job::FiberCondVar::Wait(FiberMutex &mutex)
{
	Worker *worker = GetCurrentWorker();

	DebugLogAssert(scheduler->GetLogChannel(), worker && worker->current_fiber, "CondVarWait can only be called from inside a job.");

	lock.Acquire();

	DebugLogAssert(scheduler->GetLogChannel(), waiting_count < JOBS_MAX_WAITING, "Cannot wait more jobs on condition variable!!");

	waiting[waiting_count++] = worker->current_fiber;

	// still holding the cv spinlock here, so a signal can't resume us before
	// we've switched away, even though the mutex is already free for others.
	mutex.Unlock();
	scheduler->SuspendCurrentFiber(&lock);
	mutex.Lock();
}

void job::FiberCondVar::Signal()
{
	lock.Acquire();

	JobFiber *next = nullptr;

	if (waiting_count > 0) {
		next = waiting[0];

		for (unsigned i = 1; i < waiting_count; i++) {
			waiting[i - 1] = waiting[i];
		}

		waiting_count--;
	}

	lock.Release();

	if (next) {
		scheduler->PushWaitingFiber(next);
		scheduler->WakeAllWorkers();
	}
}

void job::FiberCondVar::Broadcast()
{
	lock.Acquire();

	unsigned kick_count = waiting_count;
	JobFiber *to_kick[JOBS_MAX_WAITING];

	memcpy(to_kick, waiting, kick_count * sizeof(JobFiber *));

	waiting_count = 0;

	lock.Release();

	for (unsigned i = 0; i < kick_count; i++) {
		scheduler->PushWaitingFiber(to_kick[i]);
	}

	if (kick_count > 0)
		scheduler->WakeAllWorkers();
}

job::Scheduler::Scheduler(LogChannel log_channel_)
	: log_channel(log_channel_)
	, running(1)
	, spin_mode(0)
	, worker_count(0)
	, fiber_pool_head(nullptr)
	, on_main_thread_idle(nullptr)
	, main_thread_idle_ctx(nullptr)
{
	scheduler = this;

	for (unsigned i = 0; i < JOBS_MAX_CONCURRENT_FIBERS; i++) {
		JobFiber *fiber = &fiber_storage[i];

		// This is purely an aesthetic thing but since we push
		// the fibers to the front, the ones at the front get
		// selected first so we assign id's in reverse so at
		// the end the "front" fiber has id 0, then 1, etc...
		fiber->id = JOBS_MAX_CONCURRENT_FIBERS - i - 1;

		fiber->context = new os::Fiber(OS_FIBER_STACK_SIZE, FiberEntry, this);

		DebugLogAssert(log_channel, fiber->context->IsValid(), "Failed to allocate a fiber stack.");

		// move the fiber to the freelist.
		ReturnFiber(fiber);
	}

	// try to leave at least one free core available.
	unsigned cores = os::GetNumCores();
	const unsigned desired_workers = (cores > 1) ? (cores - 1) : 1;

	worker_count = desired_workers < JOBS_MAX_WORKERS ? desired_workers : JOBS_MAX_WORKERS;

	workers[0].id = 0;

	for (unsigned i = 1; i < worker_count; i++) {
		Worker *worker = &workers[i];
		worker->id = i;
		worker->thread = std::thread(ThreadEntry, this, worker);
	}

	DebugLogI(log_channel, "Initialized.");
}

job::Scheduler::~Scheduler()
{
	// worker 0 is the main thread so no join needed.
	for (unsigned i = 1; i < worker_count; i++) {
		if (workers[i].thread.joinable())
			workers[i].thread.join();
	}

	// release fibers
	for (unsigned i = 0; i < JOBS_MAX_CONCURRENT_FIBERS; i++) {
		delete fiber_storage[i].context;
		fiber_storage[i].context = nullptr;
	}

	DebugLogI(log_channel, "Destroyed.");

	scheduler = nullptr;
}

void job::Scheduler::SetSpinMode(bool enabled)
{
	spin_mode.exchange(enabled ? 1 : 0);
}

// NOTE: caller must be owner of unlock_after_switch!
//       unlock_after_switch is released by the scheduler
//       fiber after this fiber's context has been saved, which makes
//       it safe to put the fiber calling this onto the waitlist, and then
//       yield: whoever wants to wake you has to take that same spinlock
//       first, so can't resume you until the switch has full happened.
void job::Scheduler::SuspendCurrentFiber(SpinLock *unlock_after_switch)
{
	Worker *worker = GetCurrentWorker();
	JobFiber *fiber = worker->current_fiber;

	fiber->finished = false;

	worker->pending_unlock = unlock_after_switch;

	os::Fiber::Switch(fiber->context, worker->fiber);
}

void job::Scheduler::MarkCurrentJobAsCompleted()
{
	Worker *worker = GetCurrentWorker();
	JobFiber *fiber = worker->current_fiber;

	fiber->finished = true;

	worker->pending_unlock = nullptr;

	os::Fiber::Switch(fiber->context, worker->fiber);
}

job::JobFiber *job::Scheduler::FetchFreeFiber()
{
	fiber_pool_lock.Acquire();

	JobFiber *fiber = fiber_pool_head;

	if (fiber)
		fiber_pool_head = fiber->next_free;

	fiber_pool_lock.Release();

	if (fiber)
		fiber->next_free = nullptr;

	return fiber;
}

void job::Scheduler::ReturnFiber(JobFiber *fiber)
{
	fiber->entry_point = nullptr;
	fiber->param = nullptr;

	fiber->counter = nullptr;
	fiber->flags = 0;
	fiber->finished = true;

	fiber_pool_lock.Acquire();
	{
		fiber->next_free = fiber_pool_head;
		fiber_pool_head = fiber;
	}
	fiber_pool_lock.Release();
}

bool job::Scheduler::TryGetRequest(bool is_main_thread, Request *out)
{
	if (is_main_thread && main_thread_queue.TryPopRequest(out))
		return true;

	for (int i = Priority_MaxEnum - 1; i >= 0; i--) {
		if (queues[i].TryPopRequest(out))
			return true;
	}

	return false;
}

job::JobFiber *job::Scheduler::TryGetWaitingFiber(bool is_main_thread)
{
	JobFiber *fiber = nullptr;

	if (is_main_thread) {
		fiber = main_thread_queue.TryPopWaiting();

		if (fiber)
			return fiber;
	}

	for (int i = Priority_MaxEnum - 1; i >= 0; i--) {
		fiber = queues[i].TryPopWaiting();

		if (fiber)
			return fiber;
	}

	return nullptr;
}

bool job::Scheduler::RequestAvailable(bool is_main_thread)
{
	if (is_main_thread && main_thread_queue.HasWork())
		return true;

	for (unsigned i = 0; i < Priority_MaxEnum; i++) {
		if (queues[i].HasWork())
			return true;
	}

	return false;
}

void job::Scheduler::WakeAllWorkers()
{
	// mutex because we need to protect against workers that have
	// just checked for work but not yet gone to sleep.

	std::lock_guard<std::mutex> guard(thread_mutex);
	thread_cond_begin.notify_all();
}

void job::Scheduler::WakeOneWorker()
{
	// mutex because we need to protect against workers that have
	// just checked for work but not yet gone to sleep.

	std::lock_guard<std::mutex> guard(thread_mutex);
	thread_cond_begin.notify_one();
}

bool job::Scheduler::TryPush(const Request &request)
{
	Queue *queue = &queues[request.priority];

	if (request.flags & Flag_MainThreadOnly)
		queue = &main_thread_queue;

	bool pushed = false;

	queue->lock.Acquire();
	{
		unsigned t = static_cast<unsigned>(queue->taken_task_count.load(std::memory_order_relaxed));
		unsigned a = static_cast<unsigned>(queue->added_task_count.load(std::memory_order_relaxed));

		if ((a - t) < JOBS_MAX_JOBS_PER_QUEUE) {
			queue->requests[a % JOBS_MAX_JOBS_PER_QUEUE] = request;
			queue->added_task_count.store(static_cast<int>(a + 1), std::memory_order_release);
			pushed = true;
		}
	}
	queue->lock.Release();

	return pushed;
}

void job::Scheduler::Push(const Request &request)
{
	bool woke_workers = false;

	while (!TryPush(request)) {
		if (!woke_workers) {
			WakeAllWorkers();
			woke_workers = true;
		}

		OS_SPIN_PAUSE();
	}
}

void job::Scheduler::PushWaitingFiber(JobFiber *fiber)
{
	Queue *queue = &queues[fiber->priority];

	if (fiber->flags & Flag_MainThreadOnly)
		queue = &main_thread_queue;

	for (;;) {
		queue->lock.Acquire();

		unsigned t = static_cast<unsigned>(queue->taken_waiting_count.load(std::memory_order_relaxed));
		unsigned a = static_cast<unsigned>(queue->added_waiting_count.load(std::memory_order_relaxed));

		bool is_full = (a - t) >= JOBS_MAX_JOBS_PER_QUEUE;

		if (is_full) {
			queue->lock.Release();
			OS_SPIN_PAUSE();
		} else {
			queue->waiting[a % JOBS_MAX_JOBS_PER_QUEUE] = fiber;
			queue->added_waiting_count.store(static_cast<int>(a + 1), std::memory_order_release);

			queue->lock.Release();

			break;
		}
	}
}

void job::Scheduler::KickRaw(const Decl &decl, Counter *counter)
{
	if (counter)
		counter->Increment(1);

	Request request;
	request.entry_point = decl.entry_point;
	request.param = decl.param;
	request.priority = decl.priority;
	request.flags = decl.flags;
	request.counter = counter;

	Push(request);

	WakeOneWorker();
}

void job::Scheduler::BatchRaw(const Decl *decls, unsigned count, Counter *counter)
{
	if (counter)
		counter->Increment(count);

	for (unsigned i = 0; i < count; i++) {
		Request request;
		request.entry_point = decls[i].entry_point;
		request.param = decls[i].param;
		request.priority = decls[i].priority;
		request.flags = decls[i].flags;
		request.counter = counter;

		Push(request);
	}

	WakeAllWorkers();
}

void job::Scheduler::FiberEntry(void *param)
{
	Scheduler *self = static_cast<Scheduler *>(param);

	for (;;) {
		JobFiber *f = GetCurrentWorker()->current_fiber;
		Counter *c = f->counter;

		if (f->entry_point)
			f->entry_point(f->param);

		if (c)
			c->Decrement(1);

		// Comes back here once the scheduler hands this fiber a new job.
		self->MarkCurrentJobAsCompleted();
	}
}

// run a job fiber until it yields or completes, then deal
// with the fallout back on the worker's own scheduler fiber.
void job::Scheduler::RunFiber(Worker *worker, JobFiber *fiber)
{
	worker->current_fiber = fiber;

	os::Fiber::Switch(worker->fiber, fiber->context);

	// back on the scheduler.
	// we need to store everything about the fiber before we can unlock it
	// since right after that another worker will just grab it immediately.

	bool finished = fiber->finished;
	SpinLock *unlock = worker->pending_unlock;

	worker->current_fiber = nullptr;
	worker->pending_unlock = nullptr;

	if (unlock)
		unlock->Release();

	if (finished)
		ReturnFiber(fiber);
}

void job::Scheduler::ThreadEntry(Scheduler *self, Worker *worker)
{
	self->WorkerLoop(worker);
}

void job::Scheduler::WorkerLoop(Worker *worker)
{
	bool is_main_thread = worker->id == 0;

	unsigned starved_spins = 0;
	u64 starved_since_ms = 0;

	SetCurrentWorker(worker);
	worker->fiber = os::Fiber::ConvertThread();

#if defined(__linux__) && defined(OS_PIN_WORKER_THREADS)
	{
		cpu_set_t pinned;
		CPU_ZERO(&pinned);
		CPU_SET(worker->id % os::GetNumCores(), &pinned);
		pthread_setaffinity_np(pthread_self(), sizeof(pinned), &pinned);
	}
#endif

	while (running.load(std::memory_order_acquire)) {
		// Check for a fiber to resume.
		JobFiber *waiting_fiber = TryGetWaitingFiber(is_main_thread);

		if (waiting_fiber) {
			RunFiber(worker, waiting_fiber);
			continue;
		}

		// Check for a new job to start.
		if (RequestAvailable(is_main_thread)) {
			JobFiber *fiber = FetchFreeFiber();

			if (!fiber) {
				// Spin until there is a free fiber.

				if (starved_spins % 5000 == 0) {
					u64 now = os::GetTicks();

					if (starved_since_ms == 0) {
						starved_since_ms = now;
					} else if ((now - starved_since_ms) > JOBS_FIBER_SPIN_STARVE_ALERT_TIME_MS) {
						starved_since_ms = 0;

						DebugLogW(log_channel, "Starving, cannot get a free fiber for the job. Deadlock occuring p.");
					}
				}

				OS_SPIN_PAUSE();

				starved_spins++;

				continue;
			}

			starved_spins = 0;
			starved_since_ms = 0;

			Request request;

			if (!TryGetRequest(is_main_thread, &request)) {
				// another worker got here first, drop the fiber and start again.
				ReturnFiber(fiber);
				continue;
			}

			// FINALLY!!!!

			fiber->entry_point = request.entry_point;
			fiber->param = request.param;
			fiber->priority = request.priority;
			fiber->flags = request.flags;
			fiber->counter = request.counter;
			fiber->finished = false;

			RunFiber(worker, fiber);

			continue;
		}

		// Wait until we have more work to do...
		if (spin_mode.load(std::memory_order_acquire) || is_main_thread) {
			// main thread gets a free pass because
			// it can't sleep on a condition variable since
			// we gotta call on_main_thread_idle.

			while (!RequestAvailable(is_main_thread) && running.load(std::memory_order_acquire)) {
				if (on_main_thread_idle && is_main_thread)
					on_main_thread_idle(main_thread_idle_ctx);

				if (!spin_mode.load(std::memory_order_acquire))
					break;

				OS_SPIN_PAUSE();
			}
		} else {
			// No spin mode so resort to regular mutex and
			// waiting on a condition variable.

			std::unique_lock<std::mutex> guard(thread_mutex);

			while (!RequestAvailable(is_main_thread) && running.load(std::memory_order_acquire)) {
				thread_cond_begin.wait(guard);
			}
		}
	}

	SetCurrentWorker(nullptr);

	os::Fiber::RevertThread();
}

void job::Scheduler::Enter(void (*idle_callback)(void *ctx), void *ctx)
{
	on_main_thread_idle = idle_callback;
	main_thread_idle_ctx = ctx;

	WorkerLoop(&workers[0]);
}

void job::Scheduler::Halt()
{
	std::lock_guard<std::mutex> guard(thread_mutex);

	running.exchange(0);
	thread_cond_begin.notify_all();
}

struct ParallelForParam {
	job::ForFn *inner;
	void *inner_param;
	unsigned base_index;
	unsigned loop_size;
};

static void ParallelForBatchEntry(void *param)
{
	ParallelForParam *p = static_cast<ParallelForParam *>(param);

	for (unsigned i = 0; i < p->loop_size; i++)
		p->inner(p->base_index + i, p->inner_param);
}

void job::Kick(const Decl &decl, Counter *counter)
{
	scheduler->KickRaw(decl, counter);
}

void job::Batch(const Decl *decls, unsigned count, Counter *counter)
{
	scheduler->BatchRaw(decls, count, counter);
}

void job::For(unsigned count, ForFn *fn, void *param, Priority priority, unsigned batch_size)
{
	if (count == 0 || batch_size == 0)
		return;

	unsigned job_count = (count + batch_size - 1) / batch_size;

	Vector<ParallelForParam> params(job_count);

	Counter counter;
	bool pushed_any = false;
	bool woke_workers = false;

	for (unsigned i = 0; i < job_count; i++) {
		unsigned base_index = batch_size * i;
		unsigned loop_size = count - base_index;

		if (loop_size > batch_size)
			loop_size = batch_size;

		params[i].inner = fn;
		params[i].inner_param = param;
		params[i].base_index = base_index;
		params[i].loop_size = loop_size;

		Request request;
		request.entry_point = ParallelForBatchEntry;
		request.param = &params[i];
		request.priority = priority;
		request.flags = Flag_None;
		request.counter = &counter;

		counter.Increment(1);

		if (scheduler->TryPush(request)) {
			pushed_any = true;
			continue;
		}

		// the queue is full.
		// spinning could deadlock of the workers are asleep or we're the only
		// one, so we wake all the workers and then do this batch ourselves
		// to keep ourselves busy.

		counter.count.fetch_add(-1);

		if (!woke_workers) {
			scheduler->WakeAllWorkers();
			woke_workers = true;
		}

		ParallelForBatchEntry(&params[i]);
	}

	if (pushed_any)
		scheduler->WakeAllWorkers();

	counter.Yield(0);
}

bool job::IsMainThread()
{
	Worker *worker = GetCurrentWorker();

	DebugLogAssert(scheduler->GetLogChannel(), worker, "No worker running.");

	return worker->id == 0;
}

job::Context job::GetContext()
{
	Context ctx;
	ctx.worker_id = 0;
	ctx.fiber_id = -1;

	Worker *worker = GetCurrentWorker();

	if (worker) {
		ctx.worker_id = worker->id;
		ctx.fiber_id = worker->current_fiber ? static_cast<int>(worker->current_fiber->id) : -1;
	}

	return ctx;
}

job::Scheduler *job::GetScheduler()
{
	return scheduler;
}
