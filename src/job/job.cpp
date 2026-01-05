#include "job.h"

using namespace job;

thread_local u32 JobSystem::current_worker_id = UINT32_MAX;

JobCounter::JobCounter(u32 initial_count)
	: count()
{
    count.store(initial_count, std::memory_order::relaxed);
}

JobCounter::~JobCounter()
{
}

JobCounter::JobCounter(JobCounter &&other) noexcept
	: count(other.count.load())
{
	other.count = 0;
}

JobCounter &JobCounter::operator = (JobCounter &&other) noexcept
{
	if (this != &other) {
		count = other.count.load();
		other.count = 0;
	}
    return *this;
}

void JobCounter::inc(u32 n)
{
	count.fetch_add(n, std::memory_order::seq_cst);
}

void JobCounter::dec(u32 n)
{
	count.fetch_sub(n, std::memory_order::acq_rel);
//	u32 prev = count.fetch_sub(n, std::memory_order::acq_rel);
//	if (prev <= n && on_complete_callback)
//		on_complete_callback();
}

void JobCounter::wait()
{
	while (count.load(std::memory_order::acquire) > 0)
		JOB_SPIN_PAUSE();
}

bool JobCounter::is_complete() const
{
	return count.load(std::memory_order::acquire) == 0;
}

u32 JobCounter::get_count() const
{
	return count.load(std::memory_order::acquire);
}

JobList::JobList()
	: buffer(nullptr)
	, size(0)
{
	buffer = new JobDecl[MAX_CAPACITY];
}

JobList::~JobList()
{
	delete[] buffer;
}

void JobList::add_job(const JobDecl &decl)
{
	buffer[size % MAX_CAPACITY] = decl;
	size++;
}

JobDecl *JobList::get_job(u32 index)
{
	return &buffer[index % MAX_CAPACITY];
}

JobDecl *JobList::peek_job()
{
	assert(size > 0);
	return &buffer[(size - 1) % MAX_CAPACITY];
}

u32 JobList::get_size() const
{
	return size;
}

JobSystem::JobSystem()
	: worker_count(0)
	, workers()
	, jobs()
	, mutex()
	, cond_begin()
	, taken_task_count()
	, added_task_count()
	, next_job(nullptr)
	, spin_mode(false)
{
}

JobSystem::~JobSystem()
{
	shutdown();
}

void JobSystem::init(u32 initial_worker_count)
{
	running = true;

	worker_count = initial_worker_count;
	workers = new JobWorker[worker_count];

	for (int i = 0; i < worker_count; i++) {
		workers[i].thread = std::thread(&JobSystem::worker_thread, this, i);
	}
}

void JobSystem::shutdown()
{
	if (!running)
		return;

	running = false;

	next_job = nullptr;
	cond_begin.notify_all();

	for (int i = 0; i < worker_count; i++) {
		if (workers[i].thread.joinable())
			workers[i].thread.join();
	}

	delete[] workers;
	workers = nullptr;
}

bool JobSystem::is_spin_mode_enabled() const
{
	return spin_mode.load();
}

void JobSystem::set_spin_mode(bool enabled)
{
	spin_mode.store(enabled, std::memory_order::relaxed);
}

u32 JobSystem::get_worker_count() const
{
	return worker_count;
}

u32 JobSystem::get_current_worker_id()
{
	return current_worker_id;
}

void JobSystem::parallel_for(u32 count, const std::function<void(int)> &fn, JobPriority priority)
{
	JobCounter counter;

	for (int i = 0; i < count; i++) {
		JobDecl decl = {};
		decl.entry_point = [fn, i] { fn(i); };
		decl.priority = priority;
		decl.counter = &counter;

		kick_job(decl);
	}

	counter.wait();
}

void JobSystem::kick_job(const JobDecl &decl)
{
	jobs.add_job(decl);

	if (decl.counter)
		decl.counter->inc();

	added_task_count.fetch_add(1, std::memory_order::relaxed);
	cond_begin.notify_one();

	next_job = jobs.peek_job();
}

void JobSystem::worker_thread(u32 worker_id)
{
	current_worker_id = worker_id;

	JobWorker &worker = workers[current_worker_id];

	while (running) {
		JobDecl *job = try_get_job();

		if (job) {
			worker.job = job;

			job->entry_point();

			if (job->counter)
				job->counter->dec();
		} else {
			if (spin_mode)
				wait_job_spin(worker.job);
			else
				wait_job_lock();
		}
	}
}

void JobSystem::wait_job_spin(JobDecl *job)
{
	while (next_job == job && spin_mode)
		JOB_SPIN_PAUSE();
}

void JobSystem::wait_job_lock()
{
	std::unique_lock<std::mutex> lock(mutex);
	cond_begin.wait(lock);
}

JobDecl *JobSystem::try_get_job()
{
	// Lockless thread pool.
	while (true) {
		u32 t = taken_task_count.load(std::memory_order::relaxed);
		if (t < added_task_count.load(std::memory_order::acquire)) {
			if (taken_task_count.compare_exchange_weak(t, t + 1, std::memory_order::relaxed))
				return jobs.get_job(t);
		} else if (t == jobs.get_size()) {
			return nullptr;
		}
	}
}
