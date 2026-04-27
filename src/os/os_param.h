#ifndef OS_PARAM_H
#define OS_PARAM_H

// Memory allocated to platform layer.
#define OS_LAYER_MEMORY                Megabytes(512)

// Memory allocated to job system.
#define JOB_LAYER_MEMORY               Gigabytes(3)

#define JOB_MAX_JOBS_PER_QUEUE         512
#define JOB_MAX_CONCURRENT_FIBERS      128
#define JOB_MAX_WORKERS                32
#define JOB_COUNTER_MAX_WAITING        64
#define JOB_FIBER_SCRATCH_SIZE         Megabytes(8)

#define OS_ENGINE_NAME                "Magpie"
#define OS_DEFAULT_WINDOW_TITLE       "Magpie Demo"

#define OS_DEFAULT_WINDOW_WIDTH        1280
#define OS_DEFAULT_WINDOW_HEIGHT       720

#define OS_APP_VERSION_MAJOR           0
#define OS_APP_VERSION_MINOR           1
#define OS_APP_VERSION_PATCH           0

#define OS_ENGINE_VERSION_MAJOR        0
#define OS_ENGINE_VERSION_MINOR        1
#define OS_ENGINE_VERSION_PATCH        0

#endif // OS_PARAM_H
