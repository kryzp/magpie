#ifndef OS_H
#define OS_H

typedef struct OS_Handle OS_Handle;
struct OS_Handle
{
	void *value;
};

internal inline OS_Handle OS_HandleNull(void)
{
	OS_Handle null_handle = {0};
	return null_handle;
}

internal inline b32 OS_HandleIsNull(OS_Handle handle)
{
	return handle.value == NULL;
}

internal inline b32 OS_HandleMatch(OS_Handle a, OS_Handle b)
{
	return a.value == b.value;
}

typedef struct OS_API OS_API;
struct OS_API
{
	/* ==================================================
	   MEMORY
	   ================================================== */

	void *(*VirtualReserve)(u64 bytes);
	void  (*VirtualRelease)(void *address);
	void  (*VirtualCommit)(void *address, u64 bytes);
	void  (*VirtualDecommit)(void *address, u64 bytes);

	void *(*HeapAlloc)(u64 bytes);
	void  (*HeapFree)(void *address);
	void *(*HeapRealloc)(void *address, u64 new_bytes);
	
	u64 (*GetPageSize)(void);


	/* ==================================================
	   LOG
	   ================================================== */

	void (*Log)(LOG_Level level, LOG_Channel channel, const char *file, i32 line, const char *fn, const char *fmt, ...);
	LOG_Channel (*LogChannelOpen)(String8 name);
	LOG_Channel (*LogChannelOpenFrom)(LOG_Channel parent, String8 name); // create a sub/child channel.
	void (*LogChannelClose)(LOG_Channel channel);
	
	
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
	b32       (*ConvertFiberToThread)(void);


	/* ==================================================
	   TLS
	   ================================================== */
	
	u32   (*TLSAlloc)(void);
	void  (*TLSFree)(u32 slot);
	void *(*TLSGet)(u32 slot);
	void  (*TLSSet)(u32 slot, void *value);
	

	/* ==================================================
	   ATOMICS
	   ================================================== */

	// TODO: Some kind of OS_MemoryOrder parameter?

	i32   (*AtomicCompareExchangeI32)(i32 *ptr, i32 exchange, i32 comperand);
	i64   (*AtomicCompareExchangeI64)(i64 *ptr, i64 exchange, i64 comperand);
	void *(*AtomicCompareExchangePtr)(void *ptr, void *exchange, void *comperand);

	i32   (*AtomicStoreI32)(i32 *ptr, i32 value);
	i64   (*AtomicStoreI64)(i64 *ptr, i64 value);
	void *(*AtomicStorePtr)(void *ptr, void *value);

	i32   (*AtomicAddI32)(i32 *ptr, i32 delta);
	i64   (*AtomicAddI64)(i64 *ptr, i64 delta);

	
	/* ==================================================
	   SPINLOCK
	   ================================================== */

	void (*SpinLockAcquire)(i32 *lock);
	void (*SpinLockRelease)(i32 *lock);


	/* ==================================================
	   SYNCHRONISATION PRIMITIVES
	   ================================================== */

	OS_Handle (*FiberMutexCreate)       (void);
	void      (*FiberMutexDestroy)      (OS_Handle handle);
	void      (*FiberMutexLock)         (OS_Handle handle);
	void      (*FiberMutexUnlock)       (OS_Handle handle);

	OS_Handle (*FiberCondVarCreate)     (void);
	void      (*FiberCondVarDestroy)    (OS_Handle handle);
	void      (*FiberCondVarWait)       (OS_Handle handle, OS_Handle fiber_mutex_handle);
	void      (*FiberCondVarSignal)     (OS_Handle handle);
	void      (*FiberCondVarBroadcast)  (OS_Handle handle);

	// NOTE TO SELF: BE CAREFUL WITH USING THESE!!!!
	//               THE ENTIRE ENGINE IS RAN ON FIBERS
	//               THEREFORE A REGULAR THREAD MUTEX OR CONDVAR
	//               CAN SERIOUSLY FUCK UP SYNCHRONISATION,
	//               AS YOU CAN END UP WITH A MUTEX BEING
	//               RELEASED ON A THREAD THAT DIDNT LOCK IT.
	//               >> ONLY USE IF CERTAIN THE SYNCHRONISED BLOCK WILL /NOT/
	//                  USE FIBER/JOB-SPECIFIC FUNCTIONALITY SUCH AS YIELDING!!
	OS_Handle (*ThreadMutexCreate)      (void);
	void      (*ThreadMutexDestroy)     (OS_Handle handle);
	void      (*ThreadMutexLock)        (OS_Handle handle);
	void      (*ThreadMutexUnlock)      (OS_Handle handle);

	OS_Handle (*ThreadCondVarCreate)    (void);
	void      (*ThreadCondVarDestroy)   (OS_Handle handle);
	void      (*ThreadCondVarWait)      (OS_Handle handle, OS_Handle thread_mutex_handle);
	void      (*ThreadCondVarSignal)    (OS_Handle handle);
	void      (*ThreadCondVarBroadcast) (OS_Handle handle);


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

	OS_Handle (*StreamFromFile)(String8 path, OS_FileAccess access);
	OS_Handle (*StreamFromMemory)(void *memory, u64 bytes);
	OS_Handle (*StreamFromConstMemory)(const void *memory, u64 bytes);

	i64 (*StreamRead)(OS_Handle handle, void *dst, u64 bytes);
	i64 (*StreamWrite)(OS_Handle handle, const void *src, u64 bytes);
	i64 (*StreamSeek)(OS_Handle handle, i64 offset);
	i64 (*StreamSize)(OS_Handle handle);
	i64 (*StreamPosition)(OS_Handle handle);
	b32 (*StreamClose)(OS_Handle handle);
	

	/* ==================================================
	   JOBS
	   ================================================== */

	OS_Handle    (*JobCounterAlloc)   (i32 initial_count);
	void         (*JobCounterRelease) (OS_Handle handle);
	void         (*JobCounterInc)     (OS_Handle handle, i32 amount);
	void         (*JobCounterDec)     (OS_Handle handle, i32 amount);
	u32          (*JobCounterValue)   (OS_Handle handle);
	void         (*JobYield)          (OS_Handle handle, i32 value);
	void         (*JobKick)           (const J_Decl *decl, OS_Handle counter_handle);
	void         (*JobBatch)          (const J_Decl *decls, u32 count, OS_Handle counter_handle);
	void         (*JobFor)            (u32 count, J_EntryForFn *fn, J_Priority priority, u32 batch_size);
	b32          (*JobIsMainThread)   (void);
	Arena       *(*JobGetScratch)     (Arena * const *conflicts, u32 conflict_count);

	
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

typedef void *OS_EntryInitFn(const OS_API *api);
typedef void  OS_EntryDestroyFn(void *ctx);
typedef b32   OS_EntryTickFn(void *ctx, const OS_InputState *input);
typedef void  OS_EntryHotLoadFn(void *ctx, const OS_API *api);
typedef void  OS_EntryHotUnloadFn(void *ctx);

static const OS_API *osapi = NULL; // must be manually set

#endif // OS_H
