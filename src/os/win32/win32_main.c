
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
#include "os/os_inc.h"
#include "core/core_inc.c"
#include "os/os_inc.c"

#include "io/io_inc.h"
#include "io/io_inc.c"

#include "chrono/chrono_inc.h"
#include "chrono/chrono_inc.c"

#include "win32_job.h"
#include "win32_log.h"
#include "win32_log.c"
#include "win32_job.c"

typedef struct OS_W32_Object OS_W32_Object;
struct OS_W32_Object
{
	OS_W32_Object *next_free;

	union
	{
		// Regular win32 CreateMutex(...) is more heavy-duty for sync
		// between multiple processes, so just use this instead.
		CRITICAL_SECTION cs;

		CONDITION_VARIABLE cv;

		J_W32_Counter counter;
	};
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

	Arena platform_layer_arena;

	J_W32_Scheduler scheduler;

	LOG_W32_Logger logger;
	LOG_Channel log_channel;

	OS_W32_Object *free_objects;

	OS_Handle event_mutex;
	u32 pending_event_count;
	SDL_Event *pending_events;

	u32 gamepad_count;
	SDL_Gamepad *gamepads[OS_MAX_GAMEPADS];
};

static OS_W32_State win32_st = {0};

static OS_W32_Object *OS_W32_AllocObject(void)
{
	OS_W32_Object *object = win32_st.free_objects;

	if (object)
	{
		win32_st.free_objects = win32_st.free_objects->next_free;
		MemZeroStruct(object);
	}
	else
	{
		object = ArenaPushArray(&win32_st.platform_layer_arena, OS_W32_Object, 1);
	}

	return object;
}

static void OS_W32_ReturnObject(OS_W32_Object *object)
{
	object->next_free = win32_st.free_objects;
	win32_st.free_objects = object;
}

void *OS_W32_EntryInitStub      (const OS_API *api) { return NULL; }
void  OS_W32_EntryDestroyStub   (void *ctx) { }
b32   OS_W32_EntryTickStub      (void *ctx, const OS_InputState *input) { return false; }
void  OS_W32_EntryHotLoadStub   (void *ctx, const OS_API *api) { }
void  OS_W32_EntryHotUnloadStub (void *ctx) { }

static void OS_W32_UnloadCode(void)
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

static void OS_W32_LoadCode(String8 dll_path)
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
		DebugLogB(win32_st.log_channel, "FUCK");
	}

	CopyFile((LPCSTR)dll_path.str, dll_path_hot, FALSE);

	win32_st.code.handle = LoadLibraryA(dll_path_hot);

	if (win32_st.code.handle)
	{
		win32_st.code.Init      = (OS_EntryInitFn      *)GetProcAddress(win32_st.code.handle, "MagpieInit");
		win32_st.code.Destroy   = (OS_EntryDestroyFn   *)GetProcAddress(win32_st.code.handle, "MagpieDestroy");
		win32_st.code.Tick      = (OS_EntryTickFn      *)GetProcAddress(win32_st.code.handle, "MagpieTick");
		win32_st.code.HotLoad   = (OS_EntryHotLoadFn   *)GetProcAddress(win32_st.code.handle, "MagpieHotLoad");
		win32_st.code.HotUnload = (OS_EntryHotUnloadFn *)GetProcAddress(win32_st.code.handle, "MagpieHotUnload");
	}
	else
	{
		OS_W32_UnloadCode();
	}
}

static void *OS_W32_VirtualReserve(u64 bytes)
{
	return VirtualAlloc(NULL,
						bytes,
						MEM_RESERVE,
						PAGE_READWRITE);
}

static void OS_W32_VirtualRelease(void *address)
{
	VirtualFree(address, 0, MEM_RELEASE);
}

static void OS_W32_VirtualCommit(void *address, u64 bytes)
{
	VirtualAlloc(address,
				 bytes,
				 MEM_COMMIT,
				 PAGE_READWRITE);
}

static void OS_W32_VirtualDecommit(void *address, u64 bytes)
{
	VirtualFree(address, bytes, MEM_DECOMMIT);
}

static void *OS_W32_HeapAlloc(u64 bytes)
{
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
}

static void OS_W32_HeapFree(void *address)
{
	HeapFree(GetProcessHeap(), 0, address);
}

static void *OS_W32_HeapRealloc(void *address, u64 new_bytes)
{
	return HeapReAlloc(GetProcessHeap(), 0, address, new_bytes);
}

static u64 OS_W32_GetPageSize(void)
{
	return win32_st.system_info.dwPageSize;
}

static void OS_W32_Log(LOG_Level level, LOG_Channel channel,
		   const char *file, i32 line, const char *fn,
		   const char *fmt, ...)
{
	J_W32_Context job_context = J_W32_GetContext(&win32_st.scheduler);
	
	va_list args;
	va_start(args, fmt);
	LOG_W32_WriteV(&win32_st.logger, job_context, level, channel, file, line, fn, fmt, args);
	va_end(args);
}

static LOG_Channel OS_W32_LogChannelOpen(String8 name)
{
	return LOG_W32_OpenChannel(&win32_st.logger, name);
}

static LOG_Channel OS_W32_LogChannelOpenFrom(LOG_Channel parent, String8 name)
{
	return LOG_W32_OpenChannelFrom(&win32_st.logger, parent, name);
}

static void OS_W32_LogChannelClose(LOG_Channel channel)
{
	LOG_W32_CloseChannel(&win32_st.logger, channel);
}

static void OS_W32_SetWindowTitle(String8 title)
{
	SDL_SetWindowTitle(win32_st.sdl_window, (const char *)title.str);
}

static void OS_W32_GetWindowSize(u32 *w, u32 *h)
{
	i32 width,
		height;
	
	SDL_GetWindowSize(win32_st.sdl_window, &width, &height);

	AssertTrue(width >= 0 && height >= 0);

	if (w) *w = width;
	if (h) *h = height;
}

static void OS_W32_GetWindowSizeInPixels(u32 *pw, u32 *ph)
{
	i32 pixel_width,
		pixel_height;
	
	SDL_GetWindowSizeInPixels(win32_st.sdl_window, &pixel_width, &pixel_height);

	AssertTrue(pixel_width >= 0 && pixel_height >= 0);

	if (pw) *pw = pixel_width;
	if (ph) *ph = pixel_height;
}

static void OS_W32_SetWindowSize(u32 w, u32 h)
{
	SDL_SetWindowSize(win32_st.sdl_window, w, h);
}

static void OS_W32_SetWindowFullscreen(b32 fullscreen)
{
	SDL_SetWindowFullscreen(win32_st.sdl_window, fullscreen);
}

static void OS_W32_SetWindowBorderless(b32 borderless)
{
	SDL_SetWindowBordered(win32_st.sdl_window, !borderless);
}

static void OS_W32_SetWindowOpacity(f32 opacity)
{
	SDL_SetWindowOpacity(win32_st.sdl_window, opacity);
}

static void OS_W32_SetMousePosition(f32 x, f32 y)
{
	SDL_WarpMouseInWindow(win32_st.sdl_window, x, y);
}

static void OS_W32_SetMouseVisible(b32 visible)
{
	//	ImGui::SetMouseCursor(visible ? ImGuiMouseSource_Mouse : ImGuiMouseCursor_None);

	if (visible)
		SDL_ShowCursor();
	else
		SDL_HideCursor();
}

static b32 OS_W32_IsMouseVisible(void)
{
	return SDL_CursorVisible();
}

static void OS_W32_SetMouseLocked(b32 locked)
{
	SDL_SetWindowRelativeMouseMode(win32_st.sdl_window, locked);
}

static b32 OS_W32_IsMouseLocked(void)
{
	return SDL_GetWindowRelativeMouseMode(win32_st.sdl_window);
}

static u64 OS_W32_GetTicks(void)
{
	return SDL_GetTicks();
}

static u64 OS_W32_GetPerformanceCounter(void)
{
	return SDL_GetPerformanceCounter();
}

static u64 OS_W32_GetPerformanceFrequency(void)
{
	return SDL_GetPerformanceFrequency();
}

static u32 OS_W32_GetNumCores(void)
{
	return win32_st.system_info.dwNumberOfProcessors;
}

typedef struct OS_W32_ThreadStart OS_W32_ThreadStart;
struct OS_W32_ThreadStart
{
	void (*Entry)(void *param);
	void *param;
};

static DWORD WINAPI OS_W32_ThreadTrampoline(LPVOID param)
{
	OS_W32_ThreadStart *t = param;

	t->Entry(t->param);
	
	free(t);
	
	return 0;
}

static OS_Handle OS_W32_ThreadCreate(void (*Entry)(void *param), void *param)
{
	// TODO: switch to arena alloc
	OS_W32_ThreadStart *t = malloc(sizeof(OS_W32_ThreadStart));
	t->Entry = Entry;
	t->param = param;

	OS_Handle handle = { CreateThread(NULL, 0, OS_W32_ThreadTrampoline, t, 0, NULL) };
	return handle;
}

static void OS_W32_ThreadJoin(OS_Handle handle)
{
	WaitForSingleObject(handle.value, INFINITE);
	CloseHandle(handle.value);
}

static void OS_W32_ThreadDetach(OS_Handle handle)
{
	CloseHandle(handle.value);
}

static void OS_W32_ThreadSetAffinity(OS_Handle handle, u64 mask)
{
	SetThreadAffinityMask(handle.value, mask);
}

static OS_Handle OS_W32_GetCurrentThreadHandle(void)
{
	OS_Handle handle = { GetCurrentThread() };
	return handle;
}

static OS_Handle OS_W32_FiberCreate(u32 stack_size, void (*Entry)(void *param), void *param)
{
	OS_Handle handle = { CreateFiber(stack_size, Entry, param) };
	return handle;
}

static void OS_W32_FiberDelete(OS_Handle handle)
{
	DeleteFiber(handle.value);
}

static void OS_W32_SwitchToFiber(OS_Handle handle)
{
	SwitchToFiber(handle.value);
}

static OS_Handle OS_W32_ConvertThreadToFiber(void)
{
	OS_Handle handle = { ConvertThreadToFiber(NULL) };
	return handle;
}

static b32 OS_W32_ConvertFiberToThread(void)
{
	return ConvertFiberToThread();
}

static u32 OS_W32_TLSAlloc(void)
{
	u32 slot = TlsAlloc();
	AssertTrue(slot != TLS_OUT_OF_INDEXES);
	return slot;
}

static void OS_W32_TLSFree(u32 slot)
{
	TlsFree(slot);
}

static void *OS_W32_TLSGet(u32 slot)
{
	SetLastError(0);

	void *value = TlsGetValue(slot);
	
	if (value == NULL && GetLastError() != 0)
		AssertTrue(false && "Error with the call to get TLS.");
	
	return value;
}

static void OS_W32_TLSSet(u32 slot, void *value)
{
	TlsSetValue(slot, value);
}

static u32 OS_W32_AtomicLoadU32(u32 *ptr)
{
	return InterlockedCompareExchange((volatile LONG *)ptr, 0, 0);
}

static u64 OS_W32_AtomicLoadU64(u64 *ptr)
{
	return InterlockedCompareExchange64((volatile LONGLONG *)ptr, 0, 0);
}

static void *OS_W32_AtomicLoadPtr(void *ptr)
{
	return InterlockedCompareExchangePointer((PVOID *)ptr, NULL, NULL);
}

static u32 OS_W32_AtomicStoreU32(u32 *ptr, u32 value)
{
	return InterlockedExchange((volatile LONG *)ptr, value);
}

static u64 OS_W32_AtomicStoreU64(u64 *ptr, u64 value)
{
	return InterlockedExchange64((volatile LONGLONG *)ptr, value);
}

static void *OS_W32_AtomicStorePtr(void *ptr, void *value)
{
	return InterlockedExchangePointer((PVOID *)ptr, value);
}

static u32 OS_W32_AtomicAddU32(u32 *ptr, u32 delta)
{
	return InterlockedExchangeAdd((volatile LONG *)ptr, delta);
}

static u64 OS_W32_AtomicAddU64(u64 *ptr, u64 delta)
{
	return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, delta);
}

static u32 OS_W32_AtomicSubU32(u32 *ptr, u32 delta)
{
	return InterlockedExchangeAdd((volatile LONG *)ptr, -(LONG)delta);
}

static u64 OS_W32_AtomicSubU64(u64 *ptr, u64 delta)
{
	return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, -(LONGLONG)delta);
}

static b32 OS_W32_AtomicCASU32(u32 *ptr, u32 expected, u32 desired)
{
	return InterlockedCompareExchange((volatile LONG *)ptr, (LONG)desired, (LONG)expected) == (LONG)expected;
}

static b32 OS_W32_AtomicCASU64(u64 *ptr, u64 expected, u64 desired)
{
	return InterlockedCompareExchange64((volatile LONGLONG *)ptr, (LONGLONG)desired, (LONGLONG)expected) == (LONGLONG)expected;
}

static b32 OS_W32_AtomicCASPtr(void *ptr, void *expected, void *desired)
{
	return InterlockedCompareExchangePointer((volatile void *)ptr, desired, expected) == expected;
}

static void OS_W32_SpinLockAcquire(u32 *lock)
{
	for (;;)
	{
		if (InterlockedCompareExchange((volatile LONG *)lock, 1, 0) == 0)
			break;
		
		while (InterlockedCompareExchange((volatile LONG *)lock, 0, 0))
			OS_SPIN_PAUSE();
	}
}

static void OS_W32_SpinLockRelease(u32 *lock)
{
	InterlockedExchange((volatile LONG *)lock, 0);
}

static OS_Handle OS_W32_MutexCreate(void)
{
	OS_W32_Object *mtx = OS_W32_AllocObject();
	
	InitializeCriticalSection(&mtx->cs);
	
	OS_Handle handle = { mtx };
	return handle;
}

static void OS_W32_MutexDestroy(OS_Handle handle)
{
	OS_W32_Object *mtx = handle.value;
	
	DeleteCriticalSection(&mtx->cs);

	OS_W32_ReturnObject(mtx);
}

static void OS_W32_MutexLock(OS_Handle handle)
{
	OS_W32_Object *mtx = handle.value;
	
	EnterCriticalSection(&mtx->cs);
}

static void OS_W32_MutexUnlock(OS_Handle handle)
{
	OS_W32_Object *mtx = handle.value;
	
	LeaveCriticalSection(&mtx->cs);
}

static OS_Handle OS_W32_CondVarCreate(void)
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
static void OS_W32_CondVarDestroy(OS_Handle handle)
{
	OS_W32_Object *cnd = handle.value;
	
	OS_W32_ReturnObject(cnd);
}

static void OS_W32_CondVarWait(OS_Handle handle, OS_Handle mutex_handle)
{
	OS_W32_Object *cnd = handle.value;
	OS_W32_Object *mtx = mutex_handle.value;
	
	SleepConditionVariableCS(&cnd->cv, &mtx->cs, INFINITE);
}

static void OS_W32_CondVarSignal(OS_Handle handle)
{
	OS_W32_Object *cnd = handle.value;

	WakeConditionVariable(&cnd->cv);
}

static void OS_W32_CondVarBroadcast(OS_Handle handle)
{
	OS_W32_Object *cnd = handle.value;

	WakeAllConditionVariable(&cnd->cv);
}

static b32 OS_W32_FileDelete(String8 path)
{
	AssertTrue(DeleteFile((LPCSTR)path.str));
	return true;
}

static b32 OS_W32_FileExists(String8 path)
{
	DWORD attr = GetFileAttributes((LPCSTR)path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static u64 OS_W32_GetFileLastWriteTime(String8 path)
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

static b32 OS_W32_DirectoryCreate(String8 path)
{
	AssertTrue(CreateDirectory((LPCSTR)path.str, NULL));
	return true;
}

static b32 OS_W32_DirectoryDelete(String8 path)
{
	AssertTrue(RemoveDirectory((LPCSTR)path.str));
	return true;
}

static b32 OS_W32_DirectoryExists(String8 path)
{
	DWORD attr = GetFileAttributes((LPCSTR)path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static OS_Handle OS_W32_StreamFromFile(String8 path, OS_FileAccess access)
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
	i32 cursor = 0;

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

static OS_Handle OS_W32_StreamFromMemory(void *memory, u64 bytes)
{
	OS_Handle handle = { SDL_IOFromMem(memory, bytes) };
	return handle;
}

static OS_Handle OS_W32_StreamFromConstMemory(const void *memory, u64 bytes)
{
	OS_Handle handle = { SDL_IOFromConstMem(memory, bytes) };
	return handle;
}

static i64 OS_W32_StreamRead(OS_Handle handle, void *dst, u64 bytes)
{
	return SDL_ReadIO(handle.value, dst, bytes);
}

static i64 OS_W32_StreamWrite(OS_Handle handle, const void *src, u64 bytes)
{
	return SDL_WriteIO(handle.value, src, bytes);
}

static i64 OS_W32_StreamSeek(OS_Handle handle, i64 offset)
{
	return SDL_SeekIO(handle.value, offset, SDL_IO_SEEK_SET);
}

static i64 OS_W32_StreamSize(OS_Handle handle)
{
	return SDL_GetIOSize(handle.value);
}

static i64 OS_W32_StreamPosition(OS_Handle handle)
{
	return SDL_TellIO(handle.value);
}

static b32 OS_W32_StreamClose(OS_Handle handle)
{
	return SDL_CloseIO(handle.value);
}

static void OS_W32_OpenInExplorer(String8 path)
{
	ShellExecute(NULL, "open", (LPCSTR)path.str, NULL, NULL, SW_SHOWDEFAULT);
}

static b32 OS_W32_VulkanSurfaceCreate(void *instance, void *surface_ptr)
{
	return SDL_Vulkan_CreateSurface(win32_st.sdl_window, instance, NULL, surface_ptr);
}

static void OS_W32_VulkanSurfaceDestroy(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface(instance, surface, NULL);
}

static const char * const *OS_W32_VulkanGetInstanceExtensions(u32 *count)
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

static void OS_W32_CloseAllGamepads(void)
{
	for (u32 i = 0; i < win32_st.gamepad_count; i++)
	{
		SDL_CloseGamepad(win32_st.gamepads[i]);
		win32_st.gamepads[i] = NULL;
	}

	win32_st.gamepad_count = 0;
}

static void OS_W32_ReconnectAllGamepads(void)
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
			DebugLogD(win32_st.log_channel, "Added gamepad with player index: %d.", SDL_GetGamepadPlayerIndex(win32_st.gamepads[i]));
		else
			DebugLogD(win32_st.log_channel, "Failed to open gamepad: %d.", SDL_GetGamepadPlayerIndex(win32_st.gamepads[i]));
	}

	SDL_free(ids);
}

static void OS_W32_InitImGui(void)
{
	/*
	  IMGUI_CHECKVERSION();

	  ImGui::CreateContext();

	  ImGuiIO& io = ImGui::GetIO();
	  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	  ImGui_ImplSDL3_InitForVulkan(win32_st.sdl_window);
	*/
}

static void OS_W32_DestroyImGui(void)
{
	/*
	  ImGui_ImplSDL3_Shutdown();
	  ImGui::DestroyContext();
	*/
}

static OS_Handle OS_W32_JobCounterAlloc(u32 initial_count)
{
	OS_W32_Object *counter = OS_W32_AllocObject();

	J_W32_CounterInit(&counter->counter, initial_count);
	
	OS_Handle handle = { counter };
	return handle;
}

static void OS_W32_JobCounterRelease(OS_Handle handle)
{
	OS_W32_Object *obj = handle.value;
	
	OS_W32_ReturnObject(obj);
}

static void OS_W32_JobCounterInc(OS_Handle handle, u32 amount)
{
	OS_W32_Object *obj = handle.value;
	J_W32_Counter *counter = &obj->counter;
	
	J_W32_CounterIncrement(counter, amount);
}

static void OS_W32_JobCounterDec(OS_Handle handle, u32 amount)
{
	OS_W32_Object *obj = handle.value;
	J_W32_Counter *counter = &obj->counter;
	
	J_W32_CounterDecrement(&win32_st.scheduler, counter, amount);
}

static u32 OS_W32_JobCounterValue(OS_Handle handle)
{
	OS_W32_Object *obj = handle.value;
	J_W32_Counter *counter = &obj->counter;
	
	return J_W32_CounterValue(counter);
}

static void OS_W32_JobYield(OS_Handle handle, u32 value)
{
	OS_W32_Object *obj = handle.value;
	J_W32_Counter *counter = &obj->counter;
	
	J_W32_Yield(&win32_st.scheduler, counter, value);
}

static void OS_W32_JobKick(const J_Decl *decl, OS_Handle counter_handle)
{
	OS_W32_Object *obj = counter_handle.value;
	J_W32_Counter *counter = &obj->counter;
	
	J_W32_Kick(&win32_st.scheduler, decl, counter);
}

static void OS_W32_JobBatch(const J_Decl *decls, u32 count, OS_Handle counter_handle)
{
	OS_W32_Object *obj = counter_handle.value;
	J_W32_Counter *counter = &obj->counter;
	
	J_W32_Batch(&win32_st.scheduler, decls, count, counter);
}

static void OS_W32_JobFor(u32 count, J_EntryForFn *fn, J_Priority priority, u32 batch_size)
{
	J_W32_For(&win32_st.scheduler, count, fn, priority, batch_size);
}

static b32 OS_W32_JobIsMainThread(void)
{
	return J_W32_IsMainThread(&win32_st.scheduler);
}

static Arena *OS_W32_JobGetScratch(Arena * const *conflicts, u32 conflict_count)
{
	return J_W32_GetScratch(&win32_st.scheduler, conflicts, conflict_count);
}

static void OS_W32_BindAPI(OS_API *api)
{
	api->VirtualReserve              = OS_W32_VirtualReserve;
	api->VirtualRelease              = OS_W32_VirtualRelease;
	api->VirtualCommit               = OS_W32_VirtualCommit;
	api->VirtualDecommit             = OS_W32_VirtualDecommit;

	api->HeapAlloc                   = OS_W32_HeapAlloc;
	api->HeapFree                    = OS_W32_HeapFree;
	api->HeapRealloc                 = OS_W32_HeapRealloc;
	
	api->GetPageSize                 = OS_W32_GetPageSize;

	api->Log                         = OS_W32_Log;
	api->LogChannelOpen              = OS_W32_LogChannelOpen;
	api->LogChannelOpenFrom          = OS_W32_LogChannelOpenFrom;
	api->LogChannelClose             = OS_W32_LogChannelClose;

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
	api->JobCounterRelease           = OS_W32_JobCounterRelease;
	api->JobCounterInc               = OS_W32_JobCounterInc;
	api->JobCounterDec               = OS_W32_JobCounterDec;
	api->JobCounterValue             = OS_W32_JobCounterValue;
	api->JobYield                    = OS_W32_JobYield;
	api->JobKick                     = OS_W32_JobKick;
	api->JobBatch                    = OS_W32_JobBatch;
	api->JobFor                      = OS_W32_JobFor;
	api->JobIsMainThread             = OS_W32_JobIsMainThread;
	api->JobGetScratch               = OS_W32_JobGetScratch;

	api->OpenInExplorer              = OS_W32_OpenInExplorer;

	api->VulkanSurfaceCreate         = OS_W32_VulkanSurfaceCreate;
	api->VulkanSurfaceDestroy        = OS_W32_VulkanSurfaceDestroy;
	api->VulkanGetInstanceExtensions = OS_W32_VulkanGetInstanceExtensions;
}

static void OS_W32_MessagePump(void *ctx)
{
	SDL_Event local_events[OS_W32_MAX_PENDING_EVENTS];
	u32 local_event_count = 0;

	SDL_Event ev = {0};

	while (SDL_PollEvent(&ev) && local_event_count < ArraySize(local_events))
		local_events[local_event_count++] = ev;

	if (local_event_count > 0)
	{
		OS_W32_MutexLock(win32_st.event_mutex);
		{
			DebugLogAssert(win32_st.log_channel, win32_st.pending_event_count <= OS_W32_MAX_PENDING_EVENTS, "Ran out of event buffer space SHIT.");
			
			u32 available_space = OS_W32_MAX_PENDING_EVENTS - win32_st.pending_event_count;
			u32 copy_count = (local_event_count > available_space) ? available_space : local_event_count;

			MemCopy(win32_st.pending_events + win32_st.pending_event_count, local_events, copy_count * sizeof(SDL_Event));
			win32_st.pending_event_count += copy_count;
		}
		OS_W32_MutexUnlock(win32_st.event_mutex);
	}
}

static OS_InputState OS_W32_ProcessEvents(OS_InputState prev_state)
{
	OS_InputState input_out = prev_state;

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

	input_out.mouse_delta = v2x(0.f);
	input_out.mouse_wheel = v2x(0.f);

	for (u32 i = 0; i < event_count; i++)
	{
		const SDL_Event *ev = &events[i];

		//ImGui_ImplSDL3_ProcessEvent(ev);

		switch (ev->type)
		{
			case SDL_EVENT_QUIT:
				J_W32_Halt(&win32_st.scheduler);
				break;

			case SDL_EVENT_KEY_DOWN:
				input_out.kb_down[ev->key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				input_out.kb_down[ev->key.scancode] = false;
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				input_out.mb_down[ev->button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				input_out.mb_down[ev->button.button] = false;
				break;

			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetGlobalMouseState(&input_out.mouse_screen_position.x, &input_out.mouse_screen_position.y);
				input_out.mouse_position = v2(ev->motion.x, ev->motion.y);
				input_out.mouse_delta = V2Add(input_out.mouse_delta, v2(ev->motion.xrel, ev->motion.yrel));
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				input_out.mouse_wheel = V2Add(input_out.mouse_wheel, v2(ev->wheel.x, ev->wheel.y));
				break;
					
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				input_out.gamepads[SDL_GetGamepadPlayerIndexForID(ev->gbutton.which)].down[ev->gbutton.button] = true;
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				input_out.gamepads[SDL_GetGamepadPlayerIndexForID(ev->gbutton.which)].down[ev->gbutton.button] = false;
				break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				OS_GamepadStateSetAxisValue(&input_out.gamepads[SDL_GetGamepadPlayerIndexForID(ev->gaxis.which)],
											(OS_GamepadAxis)ev->gaxis.axis,
											(f32)ev->gaxis.value / (f32)(SDL_JOYSTICK_AXIS_MAX - ((ev->gaxis.value >= 0.f) ? 1.f : 0.f)));
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				OS_W32_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				DebugLogD(win32_st.log_channel, "Removed Gamepad.");
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

	for (u32 i = 0; i < ArraySize(input_out.kb_down); i++)
	{
		input_out.kb_pressed[i] = input_out.kb_down[i] && !prev_state.kb_down[i];
		input_out.kb_released[i] = !input_out.kb_down[i] && prev_state.kb_down[i];
	}

	for (u32 i = 0; i < ArraySize(input_out.mb_down); i++)
	{
		input_out.mb_pressed[i] = input_out.mb_down[i] && !prev_state.mb_down[i];
		input_out.mb_released[i] = !input_out.mb_down[i] && prev_state.mb_down[i];
	}

	for (u32 p = 0; p < OS_MAX_GAMEPADS; p++)
	{
		for (u32 i = 0; i < ArraySize(input_out.gamepads[p].down); i++)
		{
			input_out.gamepads[p].pressed[i] = input_out.gamepads[p].down[i] && !prev_state.gamepads[p].down[i];
			input_out.gamepads[p].released[i] = !input_out.gamepads[p].down[i] && prev_state.gamepads[p].down[i];
		}
	}
	
	ScratchRelease(&scratch);

	return input_out;
}

static J_ENTRY_POINT_DEF(OS_W32_FrameJobEntry)
{
	FILETIME last_write_time = {0};

	WIN32_FIND_DATA find_data = {0};
	HANDLE file_handle = FindFirstFileA("build/program.dll", &find_data);

	if (file_handle != INVALID_HANDLE_VALUE)
	{
		FindClose(file_handle);
		last_write_time = find_data.ftLastWriteTime;
	}
	
	if (CompareFileTime(&last_write_time, &win32_st.code.last_write_time) != 0)
	{
		DebugLogI(win32_st.log_channel, "Attempting Hot Reload...");

		win32_st.code.HotUnload(win32_st.app);

		OS_W32_UnloadCode();
		OS_W32_LoadCode(String8Lit("build/program.dll"));

		win32_st.code.HotLoad(win32_st.app, &win32_st.api);

		DebugLogI(win32_st.log_channel, "Hot Reloaded!");
	}

	static OS_InputState prev_input_st = {0};
	OS_InputState curr_input_st = OS_W32_ProcessEvents(prev_input_st);
	prev_input_st = curr_input_st;

	//OS_ImGuiNewFrame();

	// ---
	
	if (win32_st.code.Tick(win32_st.app, &curr_input_st))
	{
		win32_st.code.Destroy(win32_st.app);
		J_W32_Halt(&win32_st.scheduler);
	}
	else
	{
		J_Decl next_frame_job = {0};
		next_frame_job.EntryPoint = OS_W32_FrameJobEntry;
		next_frame_job.priority = J_Priority_Normal;
		next_frame_job.flags = J_Flag_MainThreadOnly;

		J_W32_Kick(&win32_st.scheduler, &next_frame_job, NULL);
	}
}

static J_ENTRY_POINT_DEF(OS_W32_RootJobEntry)
{
	win32_st.app = win32_st.code.Init(&win32_st.api);

	J_Decl first_frame_job = {0};
	first_frame_job.EntryPoint = OS_W32_FrameJobEntry;
	first_frame_job.priority = J_Priority_Normal;
	first_frame_job.flags = J_Flag_MainThreadOnly;

	J_W32_Kick(&win32_st.scheduler, &first_frame_job, NULL);
}

i32 main(void)
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

	osapi = &win32_st.api;

	win32_st.platform_layer_arena = ArenaAlloc(OS_LAYER_MEMORY);

	LOG_W32_Init(&win32_st.logger, String8Lit("log_output.txt"));

	win32_st.log_channel = LOG_W32_OpenChannel(&win32_st.logger, String8Lit("WIN32"));

	DebugLogI(win32_st.log_channel, "Initializing ImGui...");
	
	OS_W32_InitImGui();

	DebugLogI(win32_st.log_channel, "Loading DLL...");

	OS_W32_LoadCode(String8Lit("build/program.dll"));
	
	win32_st.pending_events = ArenaPushArray(&win32_st.platform_layer_arena, SDL_Event, OS_W32_MAX_PENDING_EVENTS);
	
	win32_st.event_mutex = OS_W32_MutexCreate();

	LOG_Channel job_log_channel = LOG_W32_OpenChannelFrom(&win32_st.logger, win32_st.log_channel, String8Lit("JOB"));
	J_W32_Init(&win32_st.scheduler, job_log_channel);

	J_Decl root_job = {0};
	root_job.EntryPoint = OS_W32_RootJobEntry;
	root_job.priority = J_Priority_Normal;
	root_job.flags = J_Flag_MainThreadOnly;

	J_W32_Kick(&win32_st.scheduler, &root_job, NULL);
	
	J_W32_Enter(&win32_st.scheduler, OS_W32_MessagePump, NULL);

	J_W32_Shutdown(&win32_st.scheduler);
	
	OS_W32_MutexDestroy(win32_st.event_mutex);
	
	LOG_W32_Shutdown(&win32_st.logger);

	ArenaRelease(&win32_st.platform_layer_arena);

	OS_W32_DestroyImGui();
	SDL_DestroyWindow(win32_st.sdl_window);
	SDL_Quit();
	
	return 0;
}
