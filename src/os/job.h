#ifndef OS_JOBS_H
#define OS_JOBS_H

#include "core/types.h"
#include "core/log.h"
#include "os/spin_lock.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "container/vector.h"

/*
 * - "Parallelizing the Naughty Dog Engine Using Fibers" - Christian Gyrling
 * - "Parallelizing the Physics Solver" - Dennis Gustafsson
 */

#define JOBS_MAX_JOBS_PER_QUEUE              512
#define JOBS_MAX_CONCURRENT_FIBERS           128
#define JOBS_FIBER_SPIN_STARVE_ALERT_TIME_MS 3000
#define JOBS_MAX_WORKERS                     32
#define JOBS_MAX_WAITING                     128

// due to how ive written the queue indicies it needs to be this way :/
static_assert((JOBS_MAX_JOBS_PER_QUEUE & (JOBS_MAX_JOBS_PER_QUEUE - 1)) == 0, "Queue size must be a power of two.");

namespace os {
class Fiber;
}

namespace job {

typedef void EntryPointFn(void *param);
typedef void ForFn(unsigned index, void *param);

enum Priority {
	Priority_Low,
	Priority_Normal,
	Priority_High,
	Priority_MaxEnum
};

enum Flags : unsigned {
	Flag_None = 0,
	Flag_MainThreadOnly = 1 << 0
};

struct Decl {
	EntryPointFn *entry_point = nullptr;
	void *param = nullptr;
	Priority priority = Priority_Normal;
	unsigned flags = Flag_None;
};

struct Context {
	unsigned worker_id = 0;
	int fiber_id = -1;
};

struct JobFiber;

class Counter {
public:
	explicit Counter(int initial_count = 0);

	Counter(const Counter &) = delete;
	Counter &operator=(const Counter &) = delete;

	void Increment(int n);
	void Decrement(int n);

	[[nodiscard]] int Value() const;

	void Yield(int value);

private:
	friend void For(unsigned count, ForFn *fn, void *param, Priority priority, unsigned batch_size);

	std::atomic<int> count;
	SpinLock lock;

	unsigned waiting_count;
	JobFiber *waiting[JOBS_MAX_WAITING];
};

class FiberCondVar;

class FiberMutex {
public:
	FiberMutex();

	FiberMutex(const FiberMutex &) = delete;
	FiberMutex &operator=(const FiberMutex &) = delete;

	void Lock();
	void Unlock();

private:
	std::atomic<int> state;
	SpinLock lock;

	unsigned waiting_count;
	JobFiber *waiting[JOBS_MAX_WAITING];
};

class FiberMutexScope {
public:
	explicit FiberMutexScope(FiberMutex &mutex_)
		: mutex(mutex_)
	{
		mutex.Lock();
	}

	~FiberMutexScope()
	{
		mutex.Unlock();
	}

	FiberMutexScope(const FiberMutexScope &) = delete;
	FiberMutexScope &operator=(const FiberMutexScope &) = delete;

private:
	FiberMutex &mutex;
};

class FiberCondVar {
public:
	FiberCondVar();

	FiberCondVar(const FiberCondVar &) = delete;
	FiberCondVar &operator=(const FiberCondVar &) = delete;

	void Wait(FiberMutex &mutex);
	void Signal();
	void Broadcast();

private:
	SpinLock lock;

	unsigned waiting_count;
	JobFiber *waiting[JOBS_MAX_WAITING];
};

struct Request {
	EntryPointFn *entry_point = nullptr;
	void *param = nullptr;
	Priority priority = Priority_Normal;
	unsigned flags = Flag_None;
	Counter *counter = nullptr;
};

// NOTE: the 4 counts are int's but are treated like they are free running unsigned's
//       because they need to keep working when they wrap, so note to self, do
//       NOT compare using < or >, only compare using subtract and !=.
struct Queue {
	SpinLock lock;

	Request requests[JOBS_MAX_JOBS_PER_QUEUE];
	JobFiber *waiting[JOBS_MAX_JOBS_PER_QUEUE];

	std::atomic<int> taken_task_count { 0 };
	std::atomic<int> added_task_count { 0 };

	std::atomic<int> taken_waiting_count { 0 };
	std::atomic<int> added_waiting_count { 0 };

	bool TryPopRequest(Request *out);
	JobFiber *TryPopWaiting();
	bool HasWork() const;
};

struct JobFiber {
	JobFiber *next_free = nullptr;

	unsigned id = 0;
	os::Fiber *context = nullptr;

	EntryPointFn *entry_point = nullptr;
	void *param = nullptr;
	Priority priority = Priority_Normal;
	unsigned flags = Flag_None;

	Counter *counter = nullptr;
	bool finished = true;
};

struct Worker {
	unsigned id = 0;

	std::thread thread;

	// this worker's thread / scheduler loop.
	os::Fiber *fiber = nullptr;

	JobFiber *current_fiber = nullptr;

	// a job fiber that goes to sleep on a spinlock-protected waitlist keeps
	// that spinlock held across the switch, and the scheduler releases it
	// once the fiber's context has actually been properly saved.
	// without this, another worker could also pop the fiber off the waitlist
	// and resume it while it is still in the middle of switching away, which
	// would lead to disaster!!
	SpinLock *pending_unlock = nullptr;
};

class Scheduler {
public:
	explicit Scheduler(LogChannel log_channel);
	~Scheduler();

	Scheduler(const Scheduler &) = delete;
	Scheduler &operator=(const Scheduler &) = delete;

	void Enter(void (*idle_callback)(void *ctx), void *ctx);
	void Halt();

	void SetSpinMode(bool enabled);

	LogChannel GetLogChannel() const
	{
		return log_channel;
	}

	void KickRaw(const Decl &decl, Counter *counter);
	void BatchRaw(const Decl *decls, unsigned count, Counter *counter);

	bool TryPush(const Request &request);
	void Push(const Request &request);
	void PushWaitingFiber(JobFiber *fiber);

	void WakeAllWorkers();
	void WakeOneWorker();

	void SuspendCurrentFiber(SpinLock *unlock_after_switch);

private:
	static void FiberEntry(void *param);
	static void ThreadEntry(Scheduler *scheduler, Worker *worker);

	void WorkerLoop(Worker *worker);

	void MarkCurrentJobAsCompleted();

	JobFiber *FetchFreeFiber();
	void ReturnFiber(JobFiber *fiber);

	bool TryGetRequest(bool is_main_thread, Request *out);
	JobFiber *TryGetWaitingFiber(bool is_main_thread);
	bool RequestAvailable(bool is_main_thread);

	void RunFiber(Worker *worker, JobFiber *fiber);

	LogChannel log_channel;

	std::atomic<int> running;
	std::atomic<int> spin_mode;

	std::mutex thread_mutex;
	std::condition_variable thread_cond_begin;

	Queue main_thread_queue;
	Queue queues[Priority_MaxEnum];

	unsigned worker_count;
	Worker workers[JOBS_MAX_WORKERS];

	JobFiber fiber_storage[JOBS_MAX_CONCURRENT_FIBERS];

	SpinLock fiber_pool_lock;
	JobFiber *fiber_pool_head;

	void (*on_main_thread_idle)(void *ctx);
	void *main_thread_idle_ctx;
};

void Kick(const Decl &decl, Counter *counter);
void Batch(const Decl *decls, unsigned count, Counter *counter);
void For(unsigned count, ForFn *fn, void *param, Priority priority, unsigned batch_size);

bool IsMainThread();
Context GetContext();

Scheduler *GetScheduler();

}

#endif // OS_JOBS_H
