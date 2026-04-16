#ifndef OS_H
#define OS_H

// total amount of allocated memory to
// the entire process, everything.
#define OS_PROCESS_MEMORY           Gigabytes(8)

#define OS_ENGINE_NAME             "Magpie"
#define OS_DEFAULT_WINDOW_TITLE    "Magpie Demo"

#define OS_DEFAULT_WINDOW_WIDTH     1280
#define OS_DEFAULT_WINDOW_HEIGHT    720

#define OS_APP_VERSION_MAJOR        0
#define OS_APP_VERSION_MINOR        1
#define OS_APP_VERSION_PATCH        0

#define OS_ENGINE_VERSION_MAJOR     0
#define OS_ENGINE_VERSION_MINOR     1
#define OS_ENGINE_VERSION_PATCH     0

typedef enum OS_FileMode
{
	OS_FileMode_Open,      // Open and Append if exists.
	OS_FileMode_OpenRW,    // Open and Read + Append if exixts.
	OS_FileMode_Create,    // Create and Overwrite if exists.
	OS_FileMode_CreateRW,  // Create and Read + Overwrite if exists.
	OS_FileMode_COUNT
}
OS_FileMode;

typedef struct OS_Handle OS_Handle;
struct OS_Handle
{
	void *value;
};

inline OS_Handle
OS_HandleNull()
{
	OS_Handle null_handle = {0};
	return null_handle;
}

inline b32
OS_HandleMatch(OS_Handle a, OS_Handle b)
{
	return a.value == b.value;
}

#define JOB_MAX_JOBS_PER_QUEUE     512
#define JOB_MAX_CONCURRENT_FIBERS  128
#define JOB_MAX_WORKERS            32
#define JOB_COUNTER_MAX_WAITING    64
#define JOB_FIBER_SCRATCH_SIZE     Megabytes(2)

#define JOB_ENTRY_POINT_SIG(fn) void fn(void *param)
#define JOB_ENTRY_POINT_DEF(fn) internal JOB_ENTRY_POINT_SIG(fn)

#define JOB_PARALLEL_FOR_SIG(fn) void fn(u32 index)
#define JOB_PARALLEL_FOR_DEF(fn) internal JOB_PARALLEL_FOR_SIG(fn)

typedef JOB_ENTRY_POINT_SIG(JOB_EntryPoint);
typedef JOB_PARALLEL_FOR_SIG(JOB_EntryFor);

typedef enum JOB_Priority
{
	JOB_Priority_Low,
	JOB_Priority_Normal,
	JOB_Priority_High,
	JOB_Priority_COUNT
}
JOB_Priority;

typedef struct JOB_Decl JOB_Decl;
struct JOB_Decl
{
	JOB_EntryPoint *EntryPoint;
	JOB_Priority priority;
	void *param;
};

typedef struct JOB_Counter JOB_Counter;

typedef struct OS_API OS_API;
struct OS_API
{
	/* ==================================================
	   MEMORY
	   ================================================== */

	void *(*VirtualReserve)(u64 bytes);
	void  (*VirtualCommit)(void *address, u64 bytes);
	void  (*VirtualFree)(void *address);

	u64 (*GetPageSize)(void);


	/* ==================================================
	   WINDOW
	   ================================================== */

	void (*SetWindowTitle)(String8 title);

	void (*GetWindowSize)(u32 *w, u32 *h);
	void (*GetWindowSizeInPixels)(u32 *pw, u32 *ph);

	void (*SetWindowSize)(u32 w, u32 h);

	void (*SetWindowFullscreen)(b32 fullscreen);
	void (*SetWindowBorderless)(b32 borderless);
	void (*SetWindowOpacity)(f32 opacity);


	/* ==================================================
	   MOUSE
	   ================================================== */

	void (*SetMousePosition)(f32 x, f32 y);
	void (*SetMouseVisible)(b32 visible);
	b32  (*IsMouseVisible)(void);
	void (*SetMouseLocked)(b32 locked);
	b32  (*IsMouseLocked)(void);


	/* ==================================================
	   TICKS
	   ================================================== */

	u64 (*GetTicks)(void);
	u64 (*GetPerformanceCounter)(void);
	u64 (*GetPerformanceFrequency)(void);


	/* ==================================================
	   THREADING
	   ================================================== */

	u32 (*GetNumCores)(void);

	OS_Handle (*ThreadCreate)(void (*Entry)(void *param), void *param);
	void      (*ThreadJoin)(OS_Handle handle);
	void      (*ThreadDetach)(OS_Handle handle);
	void      (*ThreadSetAffinity)(OS_Handle handle, u64 mask);
	OS_Handle (*GetCurrentThreadHandle)(void);


	/* ==================================================
	   FIBERS
	   ================================================== */

	OS_Handle (*FiberCreate)(u32 stack_size, void (*Entry)(void *param), void *param);
	void      (*FiberDelete)(OS_Handle handle);
	void      (*SwitchToFiber)(OS_Handle handle);

	OS_Handle (*ConvertThreadToFiber)(void);
	u32       (*ConvertFiberToThread)(void);


	/* ==================================================
	   ATOMICS
	   ================================================== */

	// TODO: Implement the rest of the functions for signed
	//       integers and also the 8 and 16 bit versions.

	// TODO: Some kind of OS_MemoryOrder parameter?

	u32   (*AtomicLoadU32)   (u32  *ptr);
	u64   (*AtomicLoadU64)   (u64  *ptr);
	void *(*AtomicLoadPtr)   (void *ptr);

	void  (*AtomicStoreU32)  (u32  *ptr, u32   value);
	void  (*AtomicStoreU64)  (u64  *ptr, u64   value);
	void  (*AtomicStorePtr)  (void *ptr, void *value);

	// Note the return value is the value of the input atomic BEFORE adding / subtracting.
	u32   (*AtomicAddU32)    (u32 *ptr, u32 delta);
	u64   (*AtomicAddU64)    (u64 *ptr, u64 delta);
	u32   (*AtomicSubU32)    (u32 *ptr, u32 delta);
	u64   (*AtomicSubU64)    (u64 *ptr, u64 delta);

	/* ==================================================
	   SPINLOCK
	   ================================================== */

	void (*SpinLockAcquire)(b32 *lock);
	void (*SpinLockRelease)(b32 *lock);


	/* ==================================================
	   SYNCHRONISATION PRIMITIVES
	   ================================================== */

	OS_Handle (*MutexCreate)  (void);
	void      (*MutexDestroy) (OS_Handle handle);
	void      (*MutexLock)    (OS_Handle handle);
	void      (*MutexUnlock)  (OS_Handle handle);

	OS_Handle (*CondVarCreate)    (void);
	void      (*CondVarDestroy)   (OS_Handle handle);
	void      (*CondVarWait)      (OS_Handle handle, OS_Handle mutex_handle);
	void      (*CondVarSignal)    (OS_Handle handle);
	void      (*CondVarBroadcast) (OS_Handle handle);


	/* ==================================================
	   FILE SYSTEM
	   ================================================== */

	b32 (*FileDelete)(String8 path);
	b32 (*FileExists)(String8 path);

	u64 (*GetFileLastWriteTime)(String8 path);

	b32 (*DirectoryCreate)(String8 path);
	b32 (*DirectoryDelete)(String8 path);
	b32 (*DirectoryExists)(String8 path);


	/* ==================================================
	   STREAMS
	   ================================================== */

	OS_Handle (*StreamFromFile)(String8 path, OS_FileMode mode);
	OS_Handle (*StreamFromMemory)(void *memory, u64 bytes);
	OS_Handle (*StreamFromConstMemory)(const void *memory, u64 bytes);

	i64 (*StreamRead)     (OS_Handle handle, void *dst, u64 bytes);
	i64 (*StreamWrite)    (OS_Handle handle, const void *src, u64 bytes);
	i64 (*StreamSeek)     (OS_Handle handle, i64 offset);
	u64 (*StreamSize)     (OS_Handle handle);
	i64 (*StreamPosition) (OS_Handle handle);
	b32 (*StreamClose)    (OS_Handle handle);
	

	/* ==================================================
	   JOBS
	   ================================================== */

	JOB_Counter *(*JobCounterAlloc)(Arena *arena, u32 initial_count);
	void         (*JobYield)(JOB_Counter *counter, u32 value);
	void         (*JobKick)(const JOB_Decl *decl, JOB_Counter *counter);
	void         (*JobBatch)(const JOB_Decl *decls, u32 count, JOB_Counter *counter);
	void         (*JobFor)(u32 count, JOB_EntryFor *fn, JOB_Priority priority, u32 batch_size);
	b32          (*JobIsMainThread)(void);
	Arena       *(*JobGetScratch)(Arena * const *conflicts, u32 conflict_count);

	
	/* ==================================================
	   EXPLORER
	   ================================================== */

	void (*OpenInExplorer)(String8 path);
	

	/* ==================================================
	   VULKAN
	   ================================================== */

	// TODO: When /graphics/ is split into a backend (G_), frontend (R_), and
	//       scene (RS_), replace the, void *'s here with G_Handle.
	b32  (*VulkanSurfaceCreate)(void *instance, void *surface_ptr);
	void (*VulkanSurfaceDestroy)(void *instance, void *surface);
	const char * const *(*VulkanGetInstanceExtensions)(u32 *count);
};

#endif // OS_H
