#ifndef OS_H
#define OS_H

typedef struct OS_Handle OS_Handle;
struct OS_Handle
{
	void *value;
};

inline OS_Handle OS_HandleNull(void)
{
	OS_Handle null_handle = {0};
	return null_handle;
}

inline b32 OS_HandleIsNull(OS_Handle handle)
{
	return handle.value == NULL;
}

inline b32 OS_HandleMatch(OS_Handle a, OS_Handle b)
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

	void        (*Log)(LOG_Level level, LOG_Channel channel, const char *file, i32 line, const char *fn, const char *fmt, ...);
	LOG_Channel (*LogChannelOpen)(String8 name);
	LOG_Channel (*LogChannelOpenFrom)(LOG_Channel parent, String8 name); // create a sub/child channel.
	void        (*LogChannelClose)(LOG_Channel channel);
	
	
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
	
	u32   (*TLSAlloc) (void);
	void  (*TLSFree)  (u32 slot);
	void *(*TLSGet)   (u32 slot);
	void  (*TLSSet)   (u32 slot, void *value);
	

	/* ==================================================
	   ATOMICS
	   ================================================== */

	// TODO: Implement the rest of the functions for signed
	//       integers and also the 8 and 16 bit versions.

	// TODO: Some kind of OS_MemoryOrder parameter?

	u32   (*AtomicLoadU32)   (u32  *ptr);
	u64   (*AtomicLoadU64)   (u64  *ptr);
	void *(*AtomicLoadPtr)   (void *ptr);

	u32   (*AtomicStoreU32)  (u32  *ptr, u32   value);
	u64   (*AtomicStoreU64)  (u64  *ptr, u64   value);
	void *(*AtomicStorePtr)  (void *ptr, void *value);

	// Note the return value is the value of the input atomic BEFORE adding / subtracting.
	u32   (*AtomicAddU32)    (u32 *ptr, u32 delta);
	u64   (*AtomicAddU64)    (u64 *ptr, u64 delta);
	u32   (*AtomicSubU32)    (u32 *ptr, u32 delta);
	u64   (*AtomicSubU64)    (u64 *ptr, u64 delta);

	b32   (*AtomicCASU32)    (u32  *ptr, u32   expected, u32   desired);
	b32   (*AtomicCASU64)    (u64  *ptr, u64   expected, u64   desired);
	b32   (*AtomicCASPtr)    (void *ptr, void *expected, void *desired);
	
	
	/* ==================================================
	   SPINLOCK
	   ================================================== */

	void (*SpinLockAcquire)(u32 *lock);
	void (*SpinLockRelease)(u32 *lock);


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

	OS_Handle (*StreamFromFile)(String8 path, OS_FileAccess access);
	OS_Handle (*StreamFromMemory)(void *memory, u64 bytes);
	OS_Handle (*StreamFromConstMemory)(const void *memory, u64 bytes);

	i64 (*StreamRead)     (OS_Handle handle, void *dst, u64 bytes);
	i64 (*StreamWrite)    (OS_Handle handle, const void *src, u64 bytes);
	i64 (*StreamSeek)     (OS_Handle handle, i64 offset);
	i64 (*StreamSize)     (OS_Handle handle);
	i64 (*StreamPosition) (OS_Handle handle);
	b32 (*StreamClose)    (OS_Handle handle);
	

	/* ==================================================
	   JOBS
	   ================================================== */

	OS_Handle    (*JobCounterAlloc)   (u32 initial_count);
	void         (*JobCounterRelease) (OS_Handle handle);
	void         (*JobCounterInc)     (OS_Handle handle, u32 amount);
	void         (*JobCounterDec)     (OS_Handle handle, u32 amount);
	u32          (*JobCounterValue)   (OS_Handle handle);
	void         (*JobYield)          (OS_Handle handle, u32 value);
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

typedef void *OS_EntryInitFn      (const OS_API *api);
typedef void  OS_EntryDestroyFn   (void *ctx);
typedef b32   OS_EntryTickFn      (void *ctx, const OS_InputState *input);
typedef void  OS_EntryHotLoadFn   (void *ctx, const OS_API *api);
typedef void  OS_EntryHotUnloadFn (void *ctx);

static const OS_API *osapi = NULL; // must be manually set

#endif // OS_H
