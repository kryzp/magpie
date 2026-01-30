#include "job.h"

/*
 * ATROCIOUS CODE.
 */

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "core/scratch.h"
#include "platform/platform.h"

namespace job
{
	struct JobWorker {
		u32 id;
		void *thread;
		void *scheduler_fiber;
	};

	struct JobFiber {
		void *handle;
		JobDecl *job;
		JobCounter *counter;
		bool is_finished;
		JobFiber *next; // Lockless stack
	};
	
	struct JobCounter {
		std::atomic<u32> count;
		std::atomic_flag locked = ATOMIC_FLAG_INIT;
		Vector<JobFiber *> wait_list;
	};

	struct JobRequest {
		JobDecl decl;
		JobCounter *counter = nullptr;
		JobFiber *fiber = nullptr;
	};
	
	struct JobQueue {
		JobRequest *buffer;
		std::atomic_flag locked = ATOMIC_FLAG_INIT;
		std::atomic<u32> taken_task_count {0};
		std::atomic<u32> added_task_count {0};
	};

	static void lock_counter(JobCounter *counter);
	static void unlock_counter(JobCounter *counter);
	static void inc_counter(JobCounter *counter, u32 n = 1);
	static void dec_counter(JobCounter *counter, u32 n = 1);

	static void push_job(JobPriority priority, const JobRequest &request);

	static void kick_waiting_fiber(JobFiber *fiber);
	static void yield_current_fiber();
	static void return_current_fiber();
	static JobFiber *get_free_fiber();
	static void return_fiber(JobFiber *fiber);

	static JobRequest *try_get_job();
	static bool has_job_available();
}

struct ThreadLocalState {
	u32 current_worker_id = UINT32_MAX;
	job::JobFiber *current_fiber = nullptr;
	job::JobWorker *current_worker = nullptr;
};

static thread_local ThreadLocalState tls;
static u32 worker_count;
static job::JobWorker *workers;
static std::atomic<job::JobFiber *> fiber_pool_head {nullptr};
static job::JobQueue job_queues[job::PRIORITY_MAX_ENUM];
static std::atomic<bool> running;
static std::mutex mutex;
static std::condition_variable cond_begin;
static std::atomic<bool> spin_mode;

job::JobCounter *job::alloc_counter(u32 initial_count)
{
	job::JobCounter *c = new JobCounter();
	c->count.store(initial_count, std::memory_order_relaxed);
	return c;
}

void job::free_counter(JobCounter *counter)
{
	assert(counter->wait_list.empty());
	delete counter;
}

static void job::lock_counter(JobCounter *counter)
{
	while (counter->locked.test_and_set(std::memory_order_acquire))
		JOB_SPIN_PAUSE();
}

static void job::unlock_counter(JobCounter *counter)
{
	counter->locked.clear(std::memory_order_release);
}

static void job::inc_counter(JobCounter *counter, u32 n)
{
	counter->count.fetch_add(n, std::memory_order_seq_cst);
}

static void job::dec_counter(JobCounter *counter, u32 n)
{
	u32 p = counter->count.fetch_sub(n, std::memory_order_acq_rel);

	if (p == n) {
		lock_counter(counter);

		Vector<JobFiber *> wait_list_copy = counter->wait_list;

		counter->wait_list.clear();

		for (JobFiber *f : wait_list_copy)
			job::kick_waiting_fiber(f);

		unlock_counter(counter);
	}
}

void job::yield_on_counter(JobCounter *counter, u32 value)
{
	if (counter->count.load(std::memory_order_acquire) == value)
		return;

	lock_counter(counter);
	counter->wait_list.push_back(tls.current_fiber);
	unlock_counter(counter);

	yield_current_fiber();
}

void job::yield_on_counter_and_free(JobCounter *counter, u32 value)
{
	yield_on_counter(counter, value);
	free_counter(counter);
}

static void job::push_job(JobPriority priority, const JobRequest &request)
{
	JobQueue *queue = &job_queues[priority];

	while (true) {
		while (queue->locked.test_and_set(std::memory_order_acquire))
			JOB_SPIN_PAUSE();
		
		u32 t = queue->taken_task_count.load(std::memory_order_relaxed);
		u32 a = queue->added_task_count.load(std::memory_order_relaxed);

		bool overflow = (a - t) >= MAX_JOBS_IN_QUEUE;

		if (overflow) {
			queue->locked.clear(std::memory_order_release);
			JOB_SPIN_PAUSE();
		} else {
			queue->buffer[a % MAX_JOBS_IN_QUEUE] = request;
			queue->added_task_count.fetch_add(1, std::memory_order_release);
			queue->locked.clear(std::memory_order_release);
			break;
		}
	}
}

static void fiber_entry_point(void *param)
{
	while (true) {
		job::JobDecl *job = tls.current_fiber->job;
		job::JobCounter *counter = tls.current_fiber->counter;

		if (job) {
			job->entry_point(job->param);

			if (counter)
				job::dec_counter(counter);
		}

		job::return_current_fiber();
	}
}

static uptr scheduler_thread(void *param)
{
	u32 *worker_id = (u32 *)param;
	
	void *fiber_handle = platform::convert_thread_to_fiber();

	tls.current_worker_id = *worker_id;
	tls.current_worker = &workers[*worker_id];
	tls.current_worker->scheduler_fiber = fiber_handle;

	// Lock thread to core.
	platform::set_thread_affinity(tls.current_worker->thread, 1ull << tls.current_worker_id);

	while (running) {
		job::JobRequest *request = job::try_get_job();

		if (request) {
			if (request->fiber) {
				tls.current_fiber = request->fiber;
			} else {
				job::JobFiber *fib = job::get_free_fiber();

				if (!fib)
					continue;

				tls.current_fiber = fib;
				tls.current_fiber->job = &request->decl;
				tls.current_fiber->counter = request->counter;
				tls.current_fiber->is_finished = false;
			}

			platform::switch_to_fiber(tls.current_fiber->handle);

			if (tls.current_fiber->is_finished)
				return_fiber(tls.current_fiber);

			tls.current_fiber = nullptr;
		} else {
			if (spin_mode) {
				while (!job::has_job_available()) {
					if (!spin_mode)
						break;
					JOB_SPIN_PAUSE();
				}
			} else {
				std::unique_lock<std::mutex> lock(mutex);
				cond_begin.wait(lock);
			}
		}
	}

	platform::convert_fiber_to_thread();

	return 0;
}

void job::init()
{
	if (running)
		return;

	running = true;

	worker_count = platform::get_num_cores() - 1; // Leave one core free for OS.
	workers = new JobWorker[worker_count];

	for (int i = 0; i < worker_count; i++) {
		workers[i].id = i;
		workers[i].thread = platform::create_thread(scheduler_thread, &workers[i].id);
		workers[i].scheduler_fiber = nullptr;
	}

	for (int i = 0; i < MAX_CONCURRENT_FIBERS; i++) {
		JobFiber *f = new JobFiber();
		f->handle = platform::create_fiber(0, fiber_entry_point, nullptr);
		f->job = nullptr;
		f->is_finished = true;

		return_fiber(f);
	}

	for (int i = 0; i < PRIORITY_MAX_ENUM; i++) {
		job_queues[i].buffer = new JobRequest[MAX_JOBS_IN_QUEUE];
	}
}

void job::shutdown()
{
	if (!running)
		return;

	running = false;

	cond_begin.notify_all();
	
	for (int i = 0; i < worker_count; i++)
		platform::join_thread(workers[i].thread);
	
	for (int i = 0; i < PRIORITY_MAX_ENUM; i++)
		delete[] job_queues[i].buffer;

	delete[] workers;
	worker_count = 0;
	workers = nullptr;

	JobFiber *curr = fiber_pool_head.exchange(nullptr);

	while (curr) {
		JobFiber *next = curr->next;
		platform::delete_fiber(curr->handle);
		delete curr;
		curr = next;
	}
}

void job::kick_job(
	const JobDecl &job,
	JobCounter **counter
)
{
	JobRequest request = {};
	request.decl = job;
	request.counter = nullptr;
	request.fiber = nullptr;

	if (counter) {
		*counter = alloc_counter();
		inc_counter(*counter);
		request.counter = *counter;
	}

	push_job(job.priority, request);

	cond_begin.notify_one();
}

void job::kick_job_batch(
	const JobDecl *jobs, u32 count,
	JobCounter **counter
)
{
	JobCounter *c = nullptr;

	if (counter) {
		*counter = alloc_counter();
		inc_counter(*counter);
		c = *counter;
	}

	for (int i = 0; i < count; i++) {
		JobRequest request = {};
		request.decl = jobs[i];
		request.counter = c;
		request.fiber = nullptr;

		push_job(jobs[i].priority, request);
	}

	cond_begin.notify_all();
}

static void job::kick_waiting_fiber(JobFiber *fiber)
{
	JobRequest request = {};
	request.fiber = fiber;

	push_job(fiber->job->priority, request);

	cond_begin.notify_one();
}

struct ParallelForInternalParam {
	job::EntryPointParallelFor *fn;
	u32 loop_size;
	u32 base_index;
};

static JOB_ENTRY_POINT(parallel_for_batch_internal)
{
	ParallelForInternalParam *data = (ParallelForInternalParam *)param;

	for (u32 i = 0; i < data->loop_size; i++)
		data->fn(data->base_index + i);
}

void job::parallel_for(
	u32 count,
	EntryPointParallelFor *fn,
	JobPriority priority,
	u32 batch_size
)
{
	if (count <= 0)
		return;

	assert(batch_size > 0);

	u32 job_count = count / batch_size + 1;

	ScratchArena scratch;

	ParallelForInternalParam *params = scratch.get_arena().push_array<ParallelForInternalParam>(job_count);
	JobDecl *decls =  scratch.get_arena().push_array<JobDecl>(job_count);

	for (int i = 0; i < job_count; i++) {
		u32 base_index = batch_size * i;
		u32 loop_size = count - base_index;

		if (loop_size > batch_size)
			loop_size = batch_size;

		params[i].fn = fn;
		params[i].loop_size = loop_size;
		params[i].base_index = base_index;

		decls[i] = JobDecl(parallel_for_batch_internal, &params[i], priority);
	}

	JobCounter *counter = nullptr;
	kick_job_batch(decls, job_count, &counter);
	yield_on_counter_and_free(counter);
}

bool job::is_spin_mode_enabled()
{
	return spin_mode;
}

void job::set_spin_mode(bool enabled)
{
	spin_mode.store(enabled);
}

u32 job::get_current_worker_id()
{
	return tls.current_worker_id;
}

bool job::is_main_thread()
{
	return tls.current_worker_id == 0;
}

static void job::yield_current_fiber()
{
	tls.current_fiber->is_finished = false;
	platform::switch_to_fiber(tls.current_worker->scheduler_fiber);
}

static void job::return_current_fiber()
{
	tls.current_fiber->is_finished = true;
	platform::switch_to_fiber(tls.current_worker->scheduler_fiber);
}

static job::JobFiber *job::get_free_fiber()
{
	JobFiber *head = fiber_pool_head.load(std::memory_order_acquire);

	while (head) {
		bool end = fiber_pool_head.compare_exchange_weak(
			head, head->next,
			std::memory_order_release,
			std::memory_order_acquire
		);

		if (end)
			break;
	}

	if (head)
		head->next = nullptr;

	return head;
}

static void job::return_fiber(JobFiber *fiber)
{
	fiber->job = nullptr;
	fiber->is_finished = true;

	fiber->next = fiber_pool_head.load(std::memory_order_relaxed);

	while (!fiber_pool_head.compare_exchange_weak(
		fiber->next, fiber,
		std::memory_order_release,
		std::memory_order_relaxed
	)) {
		JOB_SPIN_PAUSE();
	}
}

// Lockless thread pool.
static job::JobRequest *job::try_get_job()
{
	for (int i = PRIORITY_MAX_ENUM - 1; i >= 0; i--) {
		JobQueue *queue = &job_queues[i];
			
		u32 t = queue->taken_task_count.load(std::memory_order_relaxed);
			
		while (true) {
			u32 a = queue->added_task_count.load(std::memory_order_acquire);

			if ((int)(a - t) <= 0)
				break;
				
			if (queue->taken_task_count.compare_exchange_weak(t, t + 1, std::memory_order_seq_cst))
				return &queue->buffer[t % MAX_JOBS_IN_QUEUE];
		}
	}

	return nullptr;
}

static bool job::has_job_available()
{
	for (int i = PRIORITY_MAX_ENUM - 1; i >= 0; i--) {
		JobQueue *queue = &job_queues[i];

		u32 t = queue->taken_task_count.load(std::memory_order_relaxed);
		u32 a = queue->added_task_count.load(std::memory_order_relaxed);

		if ((int)(a - t) > 0)
			return true;
	}

	return false;
}
