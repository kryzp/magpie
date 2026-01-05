#pragma once

// Credit: https://www.youtube.com/watch?v=Kvsvd67XUKw

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "core/types.h"

#if defined(__x86_64__)
# define JOB_SPIN_PAUSE() _mm_pause()
#else
# define JOB_SPIN_PAUSE() std::this_thread::yield()
#endif

namespace job
{

enum JobPriority {
	PRIORITY_LOW,
	PRIORITY_HIGH,
	PRIORITY_MAX_ENUM
};

// Lockless atomic job counter.
class JobCounter {
public:
	JobCounter(u32 initial_count = 0);
	~JobCounter();

    JobCounter(JobCounter &&other) noexcept;
    JobCounter &operator = (JobCounter &&other) noexcept;
    JobCounter(const JobCounter &) = delete;
    JobCounter &operator = (const JobCounter &) = delete;

	void inc(u32 n = 1);
	void dec(u32 n = 1);

	void wait();
	bool is_complete() const;
	u32 get_count() const;

private:
	std::atomic<u32> count;
};

using EntryPoint = std::function<void(void)>;

struct JobDecl {
	EntryPoint entry_point;
	JobPriority priority;
	JobCounter *counter;
};

class JobList {
	constexpr static u32 MAX_CAPACITY = 512;

public:
	JobList();
	~JobList();

	void add_job(const JobDecl &decl);
	JobDecl *get_job(u32 index);
	JobDecl *peek_job();
	
	u32 get_size() const;

private:
	JobDecl *buffer;
	u32 size;
};

struct JobWorker {
	JobDecl *job;
	std::thread thread;
};

class JobSystem {
public:
	JobSystem();
	~JobSystem();

	void init(u32 initial_worker_count);
	void shutdown();

	bool is_spin_mode_enabled() const;
	void set_spin_mode(bool enabled);

	//void push_spin_mode();
	//void pop_spin_mode();

	u32 get_worker_count() const;
	
	static u32 get_current_worker_id();

	void parallel_for(u32 count, const std::function<void(int)> &fn, JobPriority priority = PRIORITY_LOW);

	void kick_job(const JobDecl &decl);

private:
	void worker_thread(u32 worker_id);

	void wait_job_spin(JobDecl *job);
	void wait_job_lock();
	
	JobDecl *try_get_job();

	thread_local static u32 current_worker_id;

	u32 worker_count;
	JobWorker *workers;

	JobList jobs;

	std::atomic<bool> running;
	std::mutex mutex;
	std::condition_variable cond_begin;

	std::atomic<u32> taken_task_count;
	std::atomic<u32> added_task_count;

	std::atomic<JobDecl *> next_job;

	std::atomic<bool> spin_mode;
};

class SpinScope {
public:
	SpinScope(JobSystem &system)
		: job_system(system)
	{
		original_value = system.is_spin_mode_enabled();
		job_system.set_spin_mode(true);
	}

	~SpinScope()
	{
		job_system.set_spin_mode(original_value);
	}

private:
	JobSystem &job_system;
	bool original_value;
};

}
