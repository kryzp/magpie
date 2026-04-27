
/*
 * Yes this file is a complete mess.
 *
 * But that's okay!
 *
 * As long as that mess is confined to just one file,
 * then I consider that a damn good success for trying
 * to support multiple platforms.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

// minwindef.h wtf???
// who decided to name these macros ffs
#undef min
#undef max
#undef near
#undef far

#ifdef __x86_64__
# include <immintrin.h>
# define OS_SPIN_PAUSE() _mm_pause()
#else
# define OS_SPIN_PAUSE() do { } while (0)
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "core/core_inc.h"
#include "input/input_inc.h"
#include "os/os_inc.h"
#include "os/job/job_inc.h"

global OS_API *osapi = NULL;

#include "core/core_inc.c"
#include "input/input_inc.c"
#include "os/os_inc.c"
#include "os/job/job_inc.c"

typedef struct OS_W32_Object OS_W32_Object;
struct OS_W32_Object
{
	OS_W32_Object *next_free;
	
	// Regular win32 CreateMutex(...) is more heavy-duty for sync
	// between multiple processes, so just use this instead.
	CRITICAL_SECTION cs; 

	CONDITION_VARIABLE cv;
};

typedef struct OS_W32_Code OS_W32_Code;
struct OS_W32_Code
{
	HMODULE handle;
	FILETIME last_write_time;

	OS_EntryInitFn        *Init;
	OS_EntryDestroyFn     *Destroy;
	OS_EntryTickFn        *Tick;
	OS_EntryHotLoadFn     *HotLoad;
	OS_EntryHotUnloadFn   *HotUnload;
};

#define OS_W32_MAX_PENDING_EVENTS 1024

typedef struct OS_W32_State OS_W32_State;
struct OS_W32_State
{
	Arena process_arena;
	
	void *app;
	
	OS_API api;
	OS_W32_Code code;
	
	SYSTEM_INFO system_info;
	SDL_Window *sdl_window;

	Arena object_arena;
	OS_W32_Object *free_objects;

	OS_Handle event_mutex;
	u32 pending_event_count;
	SDL_Event *pending_events;

	JOB_Scheduler scheduler;

	u32 gamepad_count;
	SDL_Gamepad *gamepads[I_MAX_GAMEPADS];
};

global OS_W32_State win32_st = {0};

internal OS_W32_Object *
OS_W32_AllocObject(void)
{
	OS_W32_Object *object = win32_st.free_objects;

	if (object)
	{
		win32_st.free_objects = win32_st.free_objects->next_free;
		MemZeroStruct(object);
	}
	else
	{
		object = ArenaPushArray(&win32_st.object_arena, OS_W32_Object, 1);
	}

	return object;
}

internal void
OS_W32_ReturnObject(OS_W32_Object *object)
{
	object->next_free = win32_st.free_objects;
	win32_st.free_objects = object;
}

void *OS_W32_EntryInitStub(Arena *arena, const OS_API *api) { return NULL; }
void  OS_W32_EntryDestroyStub(void *ctx) { }
b32   OS_W32_EntryTickStub(void *ctx, const I_State *input) { return false; }
void  OS_W32_EntryHotLoadStub(void *ctx, const OS_API *api) { }
void  OS_W32_EntryHotUnloadStub(void *ctx) { }

internal void
OS_W32_UnloadCode(void)
{
	win32_st.code.Init      = OS_W32_EntryInitStub;
	win32_st.code.Destroy   = OS_W32_EntryDestroyStub;
	win32_st.code.Tick      = OS_W32_EntryTickStub;
	win32_st.code.HotLoad   = OS_W32_EntryHotLoadStub;
	win32_st.code.HotUnload = OS_W32_EntryHotUnloadStub;

	if (win32_st.code.handle)
	{
		FreeLibrary(win32_st.code.handle);
		win32_st.code.handle = NULL;
	}
}

internal void
OS_W32_LoadCode(String8 dll_path)
{
	static const char *dll_path_hot = "build/hot_reload.dll";

	WIN32_FIND_DATA find_data = {0};
	HANDLE file_handle = FindFirstFileA((LPCSTR)dll_path.str, &find_data);

	if (file_handle != INVALID_HANDLE_VALUE)
	{
		FindClose(file_handle);
		win32_st.code.last_write_time = find_data.ftLastWriteTime;
	}
	else
	{
		AssertTrue(false && "FUCK");
	}

	CopyFile((LPCSTR)dll_path.str, dll_path_hot, FALSE);

	win32_st.code.handle = LoadLibraryA(dll_path_hot);

	if (win32_st.code.handle)
	{
		win32_st.code.Init      = (OS_EntryInitFn      *)GetProcAddress(win32_st.code.handle, "AppInit");
		win32_st.code.Destroy   = (OS_EntryDestroyFn   *)GetProcAddress(win32_st.code.handle, "AppDestroy");
		win32_st.code.Tick      = (OS_EntryTickFn      *)GetProcAddress(win32_st.code.handle, "AppTick");
		win32_st.code.HotLoad   = (OS_EntryHotLoadFn   *)GetProcAddress(win32_st.code.handle, "AppHotLoad");
		win32_st.code.HotUnload = (OS_EntryHotUnloadFn *)GetProcAddress(win32_st.code.handle, "AppHotUnload");
	}
	else
	{
		OS_W32_UnloadCode();
	}
}

internal void *
OS_W32_VirtualReserve(u64 bytes)
{
	return VirtualAlloc(NULL,
						bytes,
						MEM_RESERVE,
						PAGE_READWRITE);
}

internal void
OS_W32_VirtualRelease(void *address)
{
	VirtualFree(address, 0, MEM_RELEASE);
}

internal void
OS_W32_VirtualCommit(void *address, u64 bytes)
{
	VirtualAlloc(address,
				 bytes,
				 MEM_COMMIT,
				 PAGE_READWRITE);
}

internal void
OS_W32_VirtualDecommit(void *address, u64 bytes)
{
	VirtualFree(address, bytes, MEM_DECOMMIT);
}

internal u64
OS_W32_GetPageSize(void)
{
	return win32_st.system_info.dwPageSize;
}

internal void
OS_W32_SetWindowTitle(String8 title)
{
	SDL_SetWindowTitle(win32_st.sdl_window, (const char *)title.str);
}

internal void
OS_W32_GetWindowSize(u32 *w, u32 *h)
{
	i32 width,
		height;
	
	SDL_GetWindowSize(win32_st.sdl_window, &width, &height);

	AssertTrue(width >= 0 &&
			   height >= 0);

	if (w) *w = width;
	if (h) *h = height;
}

internal void
OS_W32_GetWindowSizeInPixels(u32 *pw, u32 *ph)
{
	i32 pixel_width,
		pixel_height;
	
	SDL_GetWindowSizeInPixels(win32_st.sdl_window, &pixel_width, &pixel_height);

	AssertTrue(pixel_width >= 0 &&
			   pixel_height >= 0);

	if (pw) *pw = pixel_width;
	if (ph) *ph = pixel_height;
}

internal void
OS_W32_SetWindowSize(u32 w, u32 h)
{
	SDL_SetWindowSize(win32_st.sdl_window, w, h);
}

internal void
OS_W32_SetWindowFullscreen(b32 fullscreen)
{
	SDL_SetWindowFullscreen(win32_st.sdl_window, fullscreen);
}

internal void
OS_W32_SetWindowBorderless(b32 borderless)
{
	SDL_SetWindowBordered(win32_st.sdl_window, !borderless);
}

internal void
OS_W32_SetWindowOpacity(f32 opacity)
{
	SDL_SetWindowOpacity(win32_st.sdl_window, opacity);
}

internal void
OS_W32_SetMousePosition(f32 x, f32 y)
{
	SDL_WarpMouseInWindow(win32_st.sdl_window, x, y);
}

internal void
OS_W32_SetMouseVisible(b32 visible)
{
	//	ImGui::SetMouseCursor(visible ? ImGuiMouseSource_Mouse : ImGuiMouseCursor_None);

	if (visible)
		SDL_ShowCursor();
	else
		SDL_HideCursor();
}

internal b32
OS_W32_IsMouseVisible(void)
{
	return SDL_CursorVisible();
}

internal void
OS_W32_SetMouseLocked(b32 locked)
{
	SDL_SetWindowRelativeMouseMode(win32_st.sdl_window, locked);
}

internal b32
OS_W32_IsMouseLocked(void)
{
	return SDL_GetWindowRelativeMouseMode(win32_st.sdl_window);
}

internal u64
OS_W32_GetTicks(void)
{
	return SDL_GetTicks();
}

internal u64
OS_W32_GetPerformanceCounter(void)
{
	return SDL_GetPerformanceCounter();
}

internal u64
OS_W32_GetPerformanceFrequency(void)
{
	return SDL_GetPerformanceFrequency();
}

internal u32
OS_W32_GetNumCores(void)
{
	return win32_st.system_info.dwNumberOfProcessors;
}

typedef struct OS_W32_ThreadStart OS_W32_ThreadStart;
struct OS_W32_ThreadStart
{
	void (*Entry)(void *param);
	void *param;
};

internal DWORD WINAPI
OS_W32_ThreadTrampoline(LPVOID param)
{
	OS_W32_ThreadStart *t = param;

	t->Entry(t->param);
	
	free(t);
	
	return 0;
}

internal OS_Handle
OS_W32_ThreadCreate(void (*Entry)(void *param), void *param)
{
	// TODO: switch to arena alloc
	OS_W32_ThreadStart *t = malloc(sizeof(OS_W32_ThreadStart));
	t->Entry = Entry;
	t->param = param;

	OS_Handle handle = { CreateThread(NULL, 0, OS_W32_ThreadTrampoline, t, 0, NULL) };
	return handle;
}

internal void
OS_W32_ThreadJoin(OS_Handle handle)
{
	WaitForSingleObject(handle.value, INFINITE);
	CloseHandle(handle.value);
}

internal void
OS_W32_ThreadDetach(OS_Handle handle)
{
	CloseHandle(handle.value);
}

internal void
OS_W32_ThreadSetAffinity(OS_Handle handle, u64 mask)
{
	SetThreadAffinityMask(handle.value, mask);
}

internal OS_Handle
OS_W32_GetCurrentThreadHandle(void)
{
	OS_Handle handle = { GetCurrentThread() };
	return handle;
}

internal OS_Handle
OS_W32_FiberCreate(u32 stack_size, void (*Entry)(void *param), void *param)
{
	OS_Handle handle = { CreateFiber(stack_size, Entry, param) };
	return handle;
}

internal void
OS_W32_FiberDelete(OS_Handle handle)
{
	DeleteFiber(handle.value);
}

internal void
OS_W32_SwitchToFiber(OS_Handle handle)
{
	SwitchToFiber(handle.value);
}

internal OS_Handle
OS_W32_ConvertThreadToFiber(void)
{
	OS_Handle handle = { ConvertThreadToFiber(NULL) };
	return handle;
}

internal b32
OS_W32_ConvertFiberToThread(void)
{
	return ConvertFiberToThread();
}

internal u32
OS_W32_TLSAlloc(void)
{
	u32 slot = TlsAlloc();
	AssertTrue(slot != TLS_OUT_OF_INDEXES);
	return slot;
}

internal void
OS_W32_TLSFree(u32 slot)
{
	TlsFree(slot);
}

internal void *
OS_W32_TLSGet(u32 slot)
{
	SetLastError(0);

	void *value = TlsGetValue(slot);
	
	if (value == NULL && GetLastError() != 0)
		AssertTrue(false && "Error with the call to get TLS.");
	
	return value;
}

internal void
OS_W32_TLSSet(u32 slot, void *value)
{
	TlsSetValue(slot, value);
}

internal u32
OS_W32_AtomicLoadU32(u32 *ptr)
{
	return InterlockedCompareExchange((volatile LONG *)ptr, 0, 0);
}

internal u64
OS_W32_AtomicLoadU64(u64 *ptr)
{
	return InterlockedCompareExchange64((volatile LONGLONG *)ptr, 0, 0);
}

internal void *
OS_W32_AtomicLoadPtr(void *ptr)
{
	return InterlockedCompareExchangePointer((PVOID *)ptr, NULL, NULL);
}

internal u32
OS_W32_AtomicStoreU32(u32 *ptr, u32 value)
{
	return InterlockedExchange((volatile LONG *)ptr, value);
}

internal u64
OS_W32_AtomicStoreU64(u64 *ptr, u64 value)
{
	return InterlockedExchange64((volatile LONGLONG *)ptr, value);
}

internal void *
OS_W32_AtomicStorePtr(void *ptr, void *value)
{
	return InterlockedExchangePointer((PVOID *)ptr, value);
}

internal u32
OS_W32_AtomicAddU32(u32 *ptr, u32 delta)
{
	return InterlockedExchangeAdd((volatile LONG *)ptr, delta);
}

internal u64
OS_W32_AtomicAddU64(u64 *ptr, u64 delta)
{
	return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, delta);
}

internal u32
OS_W32_AtomicSubU32(u32 *ptr, u32 delta)
{
	return InterlockedExchangeAdd((volatile LONG *)ptr, -(LONG)delta);
}

internal u64
OS_W32_AtomicSubU64(u64 *ptr, u64 delta)
{
	return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, -(LONGLONG)delta);
}

internal b32
OS_W32_AtomicCASU32(u32 *ptr, u32 expected, u32 desired)
{
	return InterlockedCompareExchange((volatile LONG *)ptr, (LONG)desired, (LONG)expected) == (LONG)expected;
}

internal b32
OS_W32_AtomicCASU64(u64 *ptr, u64 expected, u64 desired)
{
	return InterlockedCompareExchange64((volatile LONGLONG *)ptr, (LONGLONG)desired, (LONGLONG)expected) == (LONGLONG)expected;
}

internal b32
OS_W32_AtomicCASPtr(void *ptr, void *expected, void *desired)
{
	return InterlockedCompareExchangePointer((volatile void *)ptr, desired, expected) == expected;
}

internal void
OS_W32_SpinLockAcquire(u32 *lock)
{
	for (;;)
	{
		if (InterlockedCompareExchange((volatile LONG *)lock, 1, 0) == 0)
			break;
		
		while (InterlockedCompareExchange((volatile LONG *)lock, 0, 0))
			OS_SPIN_PAUSE();
	}
}

internal void
OS_W32_SpinLockRelease(u32 *lock)
{
	InterlockedExchange((volatile LONG *)lock, 0);
}

internal OS_Handle
OS_W32_MutexCreate(void)
{
	OS_W32_Object *mtx = OS_W32_AllocObject();
	
	InitializeCriticalSection(&mtx->cs);
	
	OS_Handle handle = { mtx };
	return handle;
}

internal void
OS_W32_MutexDestroy(OS_Handle handle)
{
	OS_W32_Object *mtx = handle.value;
	
	DeleteCriticalSection(&mtx->cs);

	OS_W32_ReturnObject(mtx);
}

internal void
OS_W32_MutexLock(OS_Handle handle)
{
	OS_W32_Object *mtx = handle.value;
	
	EnterCriticalSection(&mtx->cs);
}

internal void
OS_W32_MutexUnlock(OS_Handle handle)
{
	OS_W32_Object *mtx = handle.value;
	
	LeaveCriticalSection(&mtx->cs);
}

internal OS_Handle
OS_W32_CondVarCreate(void)
{
	OS_W32_Object *cnd = OS_W32_AllocObject();

	InitializeConditionVariable(&cnd->cv);
	
	OS_Handle handle = { cnd };
	return handle;
}

/*
 * Even though condition variables don't allocate any resources
 * we still need a destroy function to return the object back
 * to the free pool.
 */
internal void
OS_W32_CondVarDestroy(OS_Handle handle)
{
	OS_W32_Object *cnd = handle.value;
	
	OS_W32_ReturnObject(cnd);
}

internal void
OS_W32_CondVarWait(OS_Handle handle, OS_Handle mutex_handle)
{
	OS_W32_Object *cnd = handle.value;
	OS_W32_Object *mtx = mutex_handle.value;
	
	SleepConditionVariableCS(&cnd->cv, &mtx->cs, INFINITE);
}

internal void
OS_W32_CondVarSignal(OS_Handle handle)
{
	OS_W32_Object *cnd = handle.value;

	WakeConditionVariable(&cnd->cv);
}

internal void
OS_W32_CondVarBroadcast(OS_Handle handle)
{
	OS_W32_Object *cnd = handle.value;

	WakeAllConditionVariable(&cnd->cv);
}

internal b32
OS_W32_FileDelete(String8 path)
{
	AssertTrue(DeleteFile((LPCSTR)path.str));
	return true;
}

internal b32
OS_W32_FileExists(String8 path)
{
	DWORD attr = GetFileAttributes((LPCSTR)path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

internal u64
OS_W32_GetFileLastWriteTime(String8 path)
{
	FILETIME last_write_time = {0};

	WIN32_FIND_DATA find_data = {0};
	HANDLE file_handle = FindFirstFileA((LPCSTR)path.str, &find_data);

	if (file_handle != INVALID_HANDLE_VALUE)
	{
		FindClose(file_handle);
		last_write_time = find_data.ftLastWriteTime;
	}

	return ((u64)last_write_time.dwHighDateTime << 32) | last_write_time.dwLowDateTime;
}

internal b32
OS_W32_DirectoryCreate(String8 path)
{
	AssertTrue(CreateDirectory((LPCSTR)path.str, NULL));
	return true;
}

internal b32
OS_W32_DirectoryDelete(String8 path)
{
	AssertTrue(RemoveDirectory((LPCSTR)path.str));
	return true;
}

internal b32
OS_W32_DirectoryExists(String8 path)
{
	DWORD attr = GetFileAttributes((LPCSTR)path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

internal OS_Handle
OS_W32_StreamFromFile(String8 path, OS_FileAccess access)
{
	b32 read	   = access & OS_FileAccess_Read;
	b32 write	   = access & OS_FileAccess_Write;
	b32 create	   = access & OS_FileAccess_CreateIfMissing;
	b32 overwrite  = access & OS_FileAccess_OverwriteIfExists;
	b32 append	   = access & OS_FileAccess_Append;
	b32 excl	   = access & OS_FileAccess_Exclusive;
	b32 non_binary = access & OS_FileAccess_NonBinary;

	AssertTrue(read || write);
	
	char buf[16] = {0};
	int cursor = 0;

	if (append)
	{
		buf[cursor++] = 'a';
	}
	else if (overwrite)
	{
		buf[cursor++] = 'w';
			
		if (excl)
			buf[cursor++] = 'x';
	}
	else if (create)
	{
		SDL_IOStream *io = SDL_IOFromFile((const char *)path.str,
										  (read && write)
										  ? non_binary ? "r+" : "r+b"
										  : non_binary ? "r"  : "rb");

		if (!io)
		{
			io = SDL_IOFromFile((const char *)path.str,
								(read && write)
								? non_binary ? "w+" : "w+b"
								: non_binary ? "w"  : "wb");
		}

		OS_Handle handle = { io };
		return handle;
	}
	else
	{
		buf[cursor++] = 'r';
	}

	if (read && write)
		buf[cursor++] = '+';

	if (!non_binary)
		buf[cursor++] = 'b';

	buf[cursor] = '\0';
	
	OS_Handle handle = { SDL_IOFromFile((const char *)path.str, buf) };
	return handle;
}

internal OS_Handle
OS_W32_StreamFromMemory(void *memory, u64 bytes)
{
	OS_Handle handle = { SDL_IOFromMem(memory, bytes) };
	return handle;
}

internal OS_Handle
OS_W32_StreamFromConstMemory(const void *memory, u64 bytes)
{
	OS_Handle handle = { SDL_IOFromConstMem(memory, bytes) };
	return handle;
}

internal i64
OS_W32_StreamRead(OS_Handle handle, void *dst, u64 bytes)
{
	return SDL_ReadIO(handle.value, dst, bytes);
}

internal i64
OS_W32_StreamWrite(OS_Handle handle, const void *src, u64 bytes)
{
	return SDL_WriteIO(handle.value, src, bytes);
}

internal i64
OS_W32_StreamSeek(OS_Handle handle, i64 offset)
{
	return SDL_SeekIO(handle.value, offset, SDL_IO_SEEK_SET);
}

internal u64
OS_W32_StreamSize(OS_Handle handle)
{
	return SDL_GetIOSize(handle.value);
}

internal i64
OS_W32_StreamPosition(OS_Handle handle)
{
	return SDL_TellIO(handle.value);
}

internal b32
OS_W32_StreamClose(OS_Handle handle)
{
	return SDL_CloseIO(handle.value);
}

internal void
OS_W32_OpenInExplorer(String8 path)
{
	ShellExecute(NULL, "open", (LPCSTR)path.str, NULL, NULL, SW_SHOWDEFAULT);
}

internal b32
OS_W32_VulkanSurfaceCreate(void *instance, void *surface_ptr)
{
	return SDL_Vulkan_CreateSurface(win32_st.sdl_window, instance, NULL, surface_ptr);
}

internal void
OS_W32_VulkanSurfaceDestroy(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface(instance, surface, NULL);
}

internal const char * const *
OS_W32_VulkanGetInstanceExtensions(u32 *count)
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

internal void
OS_W32_CloseAllGamepads(void)
{
	for (u32 i = 0; i < win32_st.gamepad_count; i++)
	{
		SDL_CloseGamepad(win32_st.gamepads[i]);
		win32_st.gamepads[i] = NULL;
	}

	win32_st.gamepad_count = 0;
}

internal void
OS_W32_ReconnectAllGamepads(void)
{
	if (win32_st.gamepad_count > 0)
		OS_W32_CloseAllGamepads();

	i32 sdl_gp_count = 0;
	
	SDL_JoystickID *ids = SDL_GetGamepads(&sdl_gp_count);

	AssertTrue(sdl_gp_count >= 0);
	
	win32_st.gamepad_count = sdl_gp_count;
	
	for (u32 i = 0; i < win32_st.gamepad_count; i++)
	{
		win32_st.gamepads[i] = SDL_OpenGamepad(ids[i]);

		if (win32_st.gamepads[i])
			printf("OS/Win32 -- Added gamepad with player index: %d\n", SDL_GetGamepadPlayerIndex(win32_st.gamepads[i]));
		else
			printf("OS/Win32 -- Failed to open gamepad: %d.\n", SDL_GetGamepadPlayerIndex(win32_st.gamepads[i]));
	}

	SDL_free(ids);
}

internal void
OS_W32_InitImGui(void)
{
	/*
	  IMGUI_CHECKVERSION();

	  ImGui::CreateContext();

	  ImGuiIO& io = ImGui::GetIO();
	  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	  ImGui_ImplSDL3_InitForVulkan(win32_st.sdl_window);
	*/
}

internal void
OS_W32_DestroyImGui(void)
{
	/*
	  ImGui_ImplSDL3_Shutdown();
	  ImGui::DestroyContext();
	*/
}

internal JOB_Counter *
OS_W32_JobCounterAlloc(Arena *arena, u32 initial_count)
{
	return JOB_CounterAlloc(&win32_st.scheduler, arena, initial_count);
}

internal void
OS_W32_JobCounterInc(JOB_Counter *counter, u32 amount)
{
	JOB_CounterIncrement(&win32_st.scheduler, counter, amount);
}

internal void
OS_W32_JobCounterDec(JOB_Counter *counter, u32 amount)
{
	JOB_CounterDecrement(&win32_st.scheduler, counter, amount);
}

internal u32
OS_W32_JobCounterValue(JOB_Counter *counter)
{
	return JOB_CounterValue(&win32_st.scheduler, counter);
}

internal void
OS_W32_JobYield(JOB_Counter *counter, u32 value)
{
	JOB_Yield(&win32_st.scheduler, counter, value);
}

internal void
OS_W32_JobKick(const JOB_Decl *decl, JOB_Counter *counter)
{
	JOB_Kick(&win32_st.scheduler, decl, counter);
}

internal void
OS_W32_JobBatch(const JOB_Decl *decls, u32 count, JOB_Counter *counter)
{
	JOB_Batch(&win32_st.scheduler, decls, count, counter);
}

internal void
OS_W32_JobFor(u32 count, JOB_EntryForFn *fn, JOB_Priority priority, u32 batch_size)
{
	JOB_For(&win32_st.scheduler, count, fn, priority, batch_size);
}

internal b32
OS_W32_JobIsMainThread(void)
{
	return JOB_IsMainThread(&win32_st.scheduler);
}

internal JOB_Context
OS_W32_JobGetContext(void)
{
	return JOB_GetContext(&win32_st.scheduler);
}

internal Arena *
OS_W32_JobGetScratch(Arena * const *conflicts, u32 conflict_count)
{
	return JOB_GetScratch(&win32_st.scheduler, conflicts, conflict_count);
}

internal void
OS_W32_BindAPI(OS_API *api)
{
	api->VirtualReserve              = OS_W32_VirtualReserve;
	api->VirtualRelease              = OS_W32_VirtualRelease;
	api->VirtualCommit               = OS_W32_VirtualCommit;
	api->VirtualDecommit             = OS_W32_VirtualDecommit;

	api->GetPageSize                 = OS_W32_GetPageSize;

	api->SetWindowTitle              = OS_W32_SetWindowTitle;
	api->GetWindowSize               = OS_W32_GetWindowSize;
	api->GetWindowSizeInPixels       = OS_W32_GetWindowSizeInPixels;
	api->SetWindowSize               = OS_W32_SetWindowSize;
	api->SetWindowFullscreen         = OS_W32_SetWindowFullscreen;
	api->SetWindowBorderless         = OS_W32_SetWindowBorderless;
	api->SetWindowOpacity            = OS_W32_SetWindowOpacity;

	api->SetMousePosition            = OS_W32_SetMousePosition;
	api->SetMouseVisible             = OS_W32_SetMouseVisible;
	api->IsMouseVisible              = OS_W32_IsMouseVisible;
	api->SetMouseLocked              = OS_W32_SetMouseLocked;
	api->IsMouseLocked               = OS_W32_IsMouseLocked;

	api->GetTicks                    = OS_W32_GetTicks;
	api->GetPerformanceCounter       = OS_W32_GetPerformanceCounter;
	api->GetPerformanceFrequency     = OS_W32_GetPerformanceFrequency;

	api->GetNumCores                 = OS_W32_GetNumCores;

	api->ThreadCreate                = OS_W32_ThreadCreate;
	api->ThreadJoin                  = OS_W32_ThreadJoin;
	api->ThreadDetach                = OS_W32_ThreadDetach;
	api->ThreadSetAffinity           = OS_W32_ThreadSetAffinity;
	api->GetCurrentThreadHandle      = OS_W32_GetCurrentThreadHandle;

	api->FiberCreate                 = OS_W32_FiberCreate;
	api->FiberDelete                 = OS_W32_FiberDelete;
	api->SwitchToFiber               = OS_W32_SwitchToFiber;

	api->ConvertThreadToFiber        = OS_W32_ConvertThreadToFiber;
	api->ConvertFiberToThread        = OS_W32_ConvertFiberToThread;

	api->TLSAlloc                    = OS_W32_TLSAlloc;
	api->TLSFree                     = OS_W32_TLSFree;
	api->TLSGet                      = OS_W32_TLSGet;
	api->TLSSet                      = OS_W32_TLSSet;
	
	api->AtomicLoadU32               = OS_W32_AtomicLoadU32;
	api->AtomicLoadU64               = OS_W32_AtomicLoadU64;
	api->AtomicLoadPtr               = OS_W32_AtomicLoadPtr;
	
	api->AtomicStoreU32              = OS_W32_AtomicStoreU32;
	api->AtomicStoreU64              = OS_W32_AtomicStoreU64;
	api->AtomicStorePtr              = OS_W32_AtomicStorePtr;
	
	api->AtomicAddU32                = OS_W32_AtomicAddU32;
	api->AtomicAddU64                = OS_W32_AtomicAddU64;
	api->AtomicSubU32                = OS_W32_AtomicSubU32;
	api->AtomicSubU64                = OS_W32_AtomicSubU64;
	
	api->AtomicCASU32                = OS_W32_AtomicCASU32;
	api->AtomicCASU64                = OS_W32_AtomicCASU64;
	api->AtomicCASPtr                = OS_W32_AtomicCASPtr;
	
	api->SpinLockAcquire             = OS_W32_SpinLockAcquire;
	api->SpinLockRelease             = OS_W32_SpinLockRelease;

	api->MutexCreate                 = OS_W32_MutexCreate;
	api->MutexDestroy                = OS_W32_MutexDestroy;
	api->MutexLock                   = OS_W32_MutexLock;
	api->MutexUnlock                 = OS_W32_MutexUnlock;
	
	api->CondVarCreate               = OS_W32_CondVarCreate;
	api->CondVarDestroy              = OS_W32_CondVarDestroy;
	api->CondVarWait                 = OS_W32_CondVarWait;
	api->CondVarSignal               = OS_W32_CondVarSignal;
	api->CondVarBroadcast            = OS_W32_CondVarBroadcast;

	api->FileDelete                  = OS_W32_FileDelete;
	api->FileExists                  = OS_W32_FileExists;
	api->GetFileLastWriteTime        = OS_W32_GetFileLastWriteTime;

	api->DirectoryCreate             = OS_W32_DirectoryCreate;
	api->DirectoryDelete             = OS_W32_DirectoryDelete;
	api->DirectoryExists             = OS_W32_DirectoryExists;

	api->StreamFromFile              = OS_W32_StreamFromFile;
	api->StreamFromMemory            = OS_W32_StreamFromMemory;
	api->StreamFromConstMemory       = OS_W32_StreamFromConstMemory;

	api->StreamRead                  = OS_W32_StreamRead;
	api->StreamWrite                 = OS_W32_StreamWrite;
	api->StreamSeek                  = OS_W32_StreamSeek;
	api->StreamSize                  = OS_W32_StreamSize;
	api->StreamPosition              = OS_W32_StreamPosition;
	api->StreamClose                 = OS_W32_StreamClose;

	api->JobCounterAlloc             = OS_W32_JobCounterAlloc;
	api->JobCounterInc               = OS_W32_JobCounterInc;
	api->JobCounterDec               = OS_W32_JobCounterDec;
	api->JobCounterValue             = OS_W32_JobCounterValue;
	api->JobYield                    = OS_W32_JobYield;
	api->JobKick                     = OS_W32_JobKick;
	api->JobBatch                    = OS_W32_JobBatch;
	api->JobFor                      = OS_W32_JobFor;
	api->JobIsMainThread             = OS_W32_JobIsMainThread;
	api->JobGetContext               = OS_W32_JobGetContext;
	api->JobGetScratch               = OS_W32_JobGetScratch;

	api->OpenInExplorer              = OS_W32_OpenInExplorer;

	api->VulkanSurfaceCreate         = OS_W32_VulkanSurfaceCreate;
	api->VulkanSurfaceDestroy        = OS_W32_VulkanSurfaceDestroy;
	api->VulkanGetInstanceExtensions = OS_W32_VulkanGetInstanceExtensions;
}

internal void
OS_W32_MessagePump(void *ctx)
{
	SDL_Event local_events[OS_W32_MAX_PENDING_EVENTS];
	u32 local_event_count = 0;

	SDL_Event ev = {0};

	while (SDL_PollEvent(&ev) && local_event_count < ArraySize(local_events))
		local_events[local_event_count++] = ev;

	if (local_event_count > 0)
	{
		OS_W32_MutexLock(win32_st.event_mutex);

		// ---
		
		AssertTrue(win32_st.pending_event_count <= OS_W32_MAX_PENDING_EVENTS);
		
		u32 available_space = OS_W32_MAX_PENDING_EVENTS - win32_st.pending_event_count;
		u32 copy_count = (local_event_count > available_space) ? available_space : local_event_count;

		MemCopy(win32_st.pending_events + win32_st.pending_event_count, local_events, copy_count * sizeof(SDL_Event));
		win32_st.pending_event_count += copy_count;

		// ---
		
		OS_W32_MutexUnlock(win32_st.event_mutex);
	}
}

internal void
OS_W32_ProcessEvents(I_State *input_out)
{
	OS_W32_MessagePump(NULL);
	
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	SDL_Event *events = ArenaPushArray(scratch.arena, SDL_Event, OS_W32_MAX_PENDING_EVENTS);
	u32 event_count = 0;

	OS_W32_MutexLock(win32_st.event_mutex);
	{
		MemCopy(events, win32_st.pending_events, win32_st.pending_event_count * sizeof(SDL_Event));
		event_count = win32_st.pending_event_count;
		win32_st.pending_event_count = 0;
	}
	OS_W32_MutexUnlock(win32_st.event_mutex);
	
	// Reset button input states.
	MemZeroArray(input_out->kb_pressed);
	MemZeroArray(input_out->kb_released);
	MemZeroArray(input_out->mb_pressed);
	MemZeroArray(input_out->mb_released);

	for (int i = 0; i < I_MAX_GAMEPADS; i++) {
		I_GamepadState *gp = &input_out->gamepads[i];
		MemZeroArray(gp->pressed);
		MemZeroArray(gp->released);
	}

	// Reset mouse state.
	MemZeroStruct(&input_out->mouse_delta);
	MemZeroStruct(&input_out->mouse_wheel);

	for (u32 i = 0; i < event_count; i++) {
		const SDL_Event *ev = &events[i];

		//ImGui_ImplSDL3_ProcessEvent(ev);

		switch (ev->type) {
			case SDL_EVENT_QUIT:
				JOB_Halt(&win32_st.scheduler);
				break;

			case SDL_EVENT_KEY_DOWN:
				input_out->kb_down[ev->key.scancode] = true;
				input_out->kb_pressed[ev->key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				input_out->kb_down[ev->key.scancode] = false;
				input_out->kb_released[ev->key.scancode] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				input_out->mb_down[ev->button.button] = true;
				input_out->mb_pressed[ev->button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				input_out->mb_down[ev->button.button] = false;
				input_out->mb_released[ev->button.button] = true;
				break;

			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetGlobalMouseState(&input_out->mouse_screen_position.x, &input_out->mouse_screen_position.y);
				input_out->mouse_position = v2(ev->motion.x, ev->motion.y);
				input_out->mouse_delta = V2Add(input_out->mouse_delta, v2(ev->motion.xrel, ev->motion.yrel));
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				input_out->mouse_wheel = V2Add(input_out->mouse_wheel, v2(ev->wheel.x, ev->wheel.y));
				break;
					
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev->gbutton.which)].down[ev->gbutton.button] = true;
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev->gbutton.which)].pressed[ev->gbutton.button] = true;
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev->gbutton.which)].down[ev->gbutton.button] = false;
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev->gbutton.which)].released[ev->gbutton.button] = true;
				break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				I_GamepadStateSetAxisValue(&input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev->gaxis.which)],
										   (I_GamepadAxis)ev->gaxis.axis,
										   (f32)ev->gaxis.value / (f32)(SDL_JOYSTICK_AXIS_MAX - ((ev->gaxis.value >= 0.f) ? 1.f : 0.f)));
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				OS_W32_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				printf("OS/Win32 -- Removed gamepad.\n");
				OS_W32_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMAPPED:
				SDL_ReloadGamepadMappings();
				OS_W32_ReconnectAllGamepads();
				break;

			default:
				break;
		}
	}
	
	ScratchRelease(&scratch);
}

JOB_ENTRY_POINT_DEF(OS_W32_FrameJobEntry)
{
	static I_State prev_input_st = {0};

	I_State curr_input_st = prev_input_st;
	OS_W32_ProcessEvents(&curr_input_st);
	prev_input_st = curr_input_st;

	//OS_ImGuiNewFrame();

	// ---
	
	if (win32_st.code.Tick(win32_st.app, &curr_input_st))
	{
		JOB_Halt(&win32_st.scheduler);
	}
	else
	{
		JOB_Decl next_frame_job = {0};
		next_frame_job.EntryPoint = OS_W32_FrameJobEntry;
		next_frame_job.priority = JOB_Priority_Normal;
		next_frame_job.flags = JOB_Flag_MainThreadOnly;

		JOB_Kick(&win32_st.scheduler, &next_frame_job, NULL);
	}
}

JOB_ENTRY_POINT_DEF(OS_W32_RootJobEntry)
{
	win32_st.app = win32_st.code.Init(&win32_st.process_arena, &win32_st.api);

	JOB_Decl first_frame_job = {0};
	first_frame_job.EntryPoint = OS_W32_FrameJobEntry;
	first_frame_job.priority = JOB_Priority_Normal;
	first_frame_job.flags = JOB_Flag_MainThreadOnly;

	JOB_Kick(&win32_st.scheduler, &first_frame_job, NULL);
}

internal void
OS_W32_CoreFatalHandler(const char *file, i32 line, const char *fn, const char *msg)
{
	printf("OS/W32 -- %s:%d %s: %s\n", file, line, fn, msg);
	AssertTrue(false);
}

i32
main(void)
{
	GetSystemInfo(&win32_st.system_info);

	SDL_InitFlags init_flags =
		SDL_INIT_VIDEO |
		SDL_INIT_AUDIO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMEPAD |
		SDL_INIT_HAPTIC |
		SDL_INIT_EVENTS |
		SDL_INIT_SENSOR |
		SDL_INIT_CAMERA;

	if (!SDL_Init(init_flags))
		return -1;

	SDL_WindowFlags window_flags =
		SDL_WINDOW_VULKAN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY;

	win32_st.sdl_window = SDL_CreateWindow(OS_DEFAULT_WINDOW_TITLE,
										   OS_DEFAULT_WINDOW_WIDTH,
										   OS_DEFAULT_WINDOW_HEIGHT,
										   window_flags);

	OS_W32_BindAPI(&win32_st.api);

	CoreSetFatalHandler(OS_W32_CoreFatalHandler);
	
	osapi = &win32_st.api;

	printf("OS/Win32 -- Initializing ImGui...\n");
	
	OS_W32_InitImGui();

	printf("OS/Win32 -- Loading App DLL...\n");

	OS_W32_LoadCode(String8Lit("build/app.dll"));

	printf("OS/Win32 -- Allocating %.1f GB of memory...\n", (f64)OS_TOTAL_MEMORY / (f64)Gigabytes(1));
	void *process_memory = malloc(OS_TOTAL_MEMORY);
	printf("OS/Win32 -- Allocated!\n");
	
	win32_st.process_arena = ArenaInitMemory(process_memory, OS_TOTAL_MEMORY);
	win32_st.object_arena  = ArenaInitArena(&win32_st.process_arena, OS_LAYER_MEMORY, 8);

	win32_st.pending_events = ArenaPushArray(&win32_st.object_arena, SDL_Event, OS_W32_MAX_PENDING_EVENTS);
	
	win32_st.event_mutex = OS_W32_MutexCreate();

	JOB_Init(&win32_st.process_arena, &win32_st.scheduler);

	JOB_Decl root_job = {0};
	root_job.EntryPoint = OS_W32_RootJobEntry;
	root_job.priority = JOB_Priority_Normal;
	root_job.flags = JOB_Flag_MainThreadOnly;

	JOB_Kick(&win32_st.scheduler, &root_job, NULL);
	
	JOB_Enter(&win32_st.scheduler, OS_W32_MessagePump, NULL);
	
	win32_st.code.Destroy(win32_st.app);
	
	JOB_Shutdown(&win32_st.scheduler);

	OS_W32_MutexDestroy(win32_st.event_mutex);
	
	free(process_memory);

	OS_W32_DestroyImGui();
	SDL_DestroyWindow(win32_st.sdl_window);
	SDL_Quit();
	
	return 0;
}
