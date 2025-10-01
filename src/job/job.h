#ifndef JOB_H
#define JOB_H

#include "core/core_types.h"

#if defined(__x86_64__)
# define JOB_SPIN_PAUSE() _mm_pause()
#elif defined(__aarch64__)
# define JOB_SPIN_PAUSE() __yield()
#else
# define JOB_SPIN_PAUSE()
#endif

struct thread_job;
typedef void (*thread_job_fn_t)(struct thread_job *job);

struct thread_job {
	thread_job_fn_t fn;
	void *data;
	_Atomic(struct thread_job *) next;
	atomic_uint *counter;
};

// TODO: ONLY IMPLEMENT IN THE .C FILE!!!
//       --> THIS DECL IS JUST A REMINDER!
void thread_job_complete_internal(struct thread_job *job);

struct thread_job_pool {
	_Atomic(struct thread_job *) free_list;
	struct thread_job *buffer;
	u64 capacity;
};

struct thread_job *thread_job_pool_alloc(struct thread_job_pool *pool);
void thread_job_pool_free(struct thread_job_pool *pool, struct thread_job_pool *pool);

struct thread_job_counter {
	atomic_uint counter;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

void thread_job_counter_init(struct thread_job_counter *counter, u32 initial);
void thread_job_counter_destroy(struct thread_job_counter *counter);

void thread_job_counter_inc(struct thread_job_counter *counter, u32 i);
void thread_job_counter_dec(struct thread_job_counter *counter, u32 i);

void thread_job_counter_wait(struct thread_job_counter *counter);

enum thread_job_priority {
	THREAD_JOB_PRIORITY_low,
	THREAD_JOB_PRIORITY_medium,
	THREAD_JOB_PRIORITY_high,
	THREAD_JOB_PRIORITY_critical,
	THREAD_JOB_PRIORITY_max_enum,
};

struct thread_worker_deque {
	struct thread_job *buffer[16];

	atomic_uint head; // For stealing.
	atomic_uint tail; // For owner pushes/pops.
	
	pthread_mutext_t park_mutex;
	pthread_cond_t park_cond;

	// Number of parked threads waiting on this deque.
	atomic_uint parked_count;
};

struct thread_worker_deque_set {
	struct thread_worker_deque deques[THREAD_JOB_PRIORITY_max_enum];
};

void thread_worker_deque_init(struct thread_worker_deque *dq);
void thread_worker_deque_destroy(struct thread_worker_deque *dq);

inline u32 thread_worker_deque_size(struct thread_worker_deque *dq);

int thread_worker_deque_push_tail(struct thread_worker_deque *dq, struct thread_job *job);
struct thread_job *thread_worker_deque_pop_tail(struct thread_worker_deque *dq);

struct thread_job *thread_worker_deque_steal_head(struct thread_worker_deque *dq);

struct thread_job_system {
	struct memory_arena *arena;
	int worker_count;
	struct thread_worker_deque_set workers;
	pthread_t *threads;
	atomic_uint shutdown;
	atomic_uint rr_counter; // Round-robin pushes from external threads.
};

void thread_job_system_init(struct thread_job_system *sys, struct memory_arena *arena, int worker_count);
void thread_job_system_shutdown(struct thread_job_system *sys);

void thread_job_system_for(struct thread_job_system *sys,
			   thread_job_fn_t fn,
			   u32 count, void **data);

void thread_job_system_for_range(struct thread_job_system *sys,
				 thread_job_fn_t fn,
				 void *context,
				 u32 start, u32 end, u32 stride);



void thread_job_system_submit(struct thread_job_system *sys, struct thread_job *job);

struct thread_job *thread_job_system_alloc_and_submit(struct thread_job_system *sys,
						      thread_job_fn_t fn,
						      void *data,
						      struct thread_job_counter *counter);

void thread_job_system_get_job_for_worker(struct thread_job_system *sys, int index);

struct thread_worker_thread_state {
	struct thread_job_system *system;
	int worker_index;
};

void *job_worker_thread_entry(void *arg);

#endif // JOB_H
