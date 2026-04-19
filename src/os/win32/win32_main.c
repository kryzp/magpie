
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
# define OS_W32_SPIN_PAUSE() _mm_pause()
#else
# define OS_W32_SPIN_PAUSE() do { } while (0)
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "core/core_inc.h"
#include "os/os_inc.h"
#include "input/input_inc.h"

#include "core/core_inc.c"
#include "os/os_inc.c"
#include "input/input_inc.c"

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

typedef struct OS_W32_State OS_W32_State;
struct OS_W32_State
{
	Arena process_arena;
	
	void *app;
	
	OS_API api;
	OS_W32_Code code;
	
	SYSTEM_INFO system_info;
	SDL_Window *sdl_window;

	OS_W32_Object *free_objects;

	u32 pending_event_count;
	SDL_Event pending_events[512];
	OS_Handle event_mutex;

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
		object = ArenaPushArray(&win32_st.process_arena, OS_W32_Object, 1);
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

	win32_st.code.last_write_time = OS_GetFileLastWriteTime(dll_path);

	CopyFile(dll_path.str, dll_path_hot, FALSE);

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
OS_W32_VirtualCommit(void *address, u64 bytes)
{
	VirtualAlloc(address,
				 bytes,
				 MEM_COMMIT,
				 PAGE_READWRITE);
}

internal void
OS_W32_VirtualFree(void *address)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}

internal u64
OS_W32_GetPageSize(void)
{
	return system_info.dwPageSize;
}

internal void
OS_W32_SetWindowTitle(String8 title)
{
	SDL_SetWindowTitle(win32_st.sdl_window, title);
}

internal void
OS_W32_GetWindowSize(u32 *w, u32 *h)
{
	SDL_GetWindowSize(win32_st.sdl_window, width, height);
}

internal void
OS_W32_GetWindowSizeInPixels(u32 *pw, u32 *ph)
{
	SDL_GetWindowSizeInPixels(win32_st.sdl_window, pixel_width, pixel_height);
}

internal void
OS_W32_SetWindowSize(u32 w, u32 h)
{
	SDL_SetWindowSize(win32_st.sdl_window, width, height);
}

internal void
OS_W32_SetWindowFullscreen(b32 fullscreen)
{
	SDL_SetWindowFullscreen(win32_st.sdl_window, b);
}

internal void
OS_W32_SetWindowBorderless(b32 borderless)
{
	SDL_SetWindowBordered(win32_st.sdl_window, !b);
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
	return win32_system_info.dwNumberOfProcessors;
}

typedef struct OS_W32_ThreadStart OS_W32_ThreadStart;
struct OS_W32_ThreadStart
{
	void (*Entry)(void *param);
	void *param;
}

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
	t->entry = Entry;
	t->param = param;

	return CreateThread(NULL, 0, OS_W32_ThreadTrampoline, t, 0, NULL);
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
	SetThreadAffinityMask(handle, mask);
}

internal OS_Handle
OS_W32_GetCurrentThreadHandle(void)
{
	return GetCurrentThread();
}

internal OS_Handle
OS_W32_FiberCreate(u32 stack_size, void (*Entry)(void *param), void *param)
{
	return CreateFiber(stack_size, entry, param);
}

internal void
OS_W32_FiberDelete(OS_Handle handle)
{
	DeleteFiber(handle);
}

internal void
OS_W32_SwitchToFiber(OS_Handle handle)
{
	SwitchToFiber(handle);
}

internal OS_Handle
OS_ConvertThreadToFiber(void)
{
	return ConvertThreadToFiber(NULL);
}

internal u32
OS_W32_ConvertFiberToThread(void)
{
	return ConvertFiberToThread();
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

internal void
OS_W32_AtomicStoreU32(u32 *ptr, u32 value)
{
	InterlockedExchange((volatile LONG *)ptr, value);
}

internal void
OS_W32_AtomicStoreU64(u64 *ptr, u64 value)
{
	InterlockedExchange64((volatile LONGLONG *)ptr, value);
}

internal void
OS_W32_AtomicStorePtr(void *ptr, void *value)
{
	InterlockedExchangePointer((PVOID *)ptr, value);
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
OS_W32_AtomicSubU64(volatile u64 *ptr, u64 delta)
{
	return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, -(LONGLONG)delta);
}

internal void
OS_W32_SpinLockAcquire(b32 *lock)
{
	while (OS_W32_AtomicStoreU32(lock, true))
		OS_W32_SPIN_PAUSE();
}

internal void
OS_W32_SpinLockRelease(b32 *lock)
{
	OS_W32_AtomicStoreU32(lock, false);
}

internal OS_Handle
OS_W32_MutexCreate(void)
{
	OS_W32_Object *object = OS_W32_AllocObject();
	
	InitializeCriticalSection(&object->cs);
	
	OS_Handle handle = { object };
	return handle;
}

internal void
OS_W32_MutexDestroy(OS_Handle handle)
{
	OS_W32_Object *object = handle.value;
	
	DestroyCriticalSection(&object->cs);

	OS_W32_ReturnObject(object);
}

internal void
OS_W32_MutexLock(OS_Handle handle)
{
	OS_W32_Object *object = handle.value;
	
	EnterCriticalSection(&object->cs);
}

internal void
OS_W32_MutexUnlock(OS_Handle handle)
{
	OS_W32_Object *object = handle.value;
	
	LeaveCriticalSection(&object->cs);
}

internal OS_Handle
OS_W32_CondVarCreate(void)
{
	OS_W32_Object *object = OS_W32_AllocObject();

	InitializeConditionVariable(&object->cv);
	
	OS_Handle handle = { object };
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
	OS_W32_Object *object = handle.value;	
	OS_W32_ReturnObject(object);
}

internal void
OS_W32_CondVarWait(OS_Handle handle, OS_Handle mutex_handle)
{
	OS_W32_Object *cnd = handle.value;
	OS_W32_Object *mtx = mutex_handle.value;
	
	SleepConditionVariable(&cnd->cv, &mtx->cs, INFINITE);
}

internal void
OS_W32_CondVarSignal(OS_Handle handle)
{
	OS_W32_Object *object = handle.value;

	WakeConditionVariable(&object->cv);
}

internal void
OS_W32_CondVarBroadcast(OS_Handle handle)
{
	OS_W32_Object *object = handle.value;

	WakeAllConditionVariable(&object->cv);
}

internal b32
OS_W32_FileDelete(String8 path)
{
	AssertTrue(DeleteFile(path.str));
}

internal b32
OS_W32_FileExists(String8 path)
{
	DWORD attr = GetFileAttributes(path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

internal u64
OS_W32_GetFileLastWriteTime(String8 path)
{
	FILETIME last_write_time = {0};

	WIN32_FIND_DATA find_data = {0};
	HANDLE file_handle = FindFirstFileA(path.str, &find_data);

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
	AssertTrue(CreateDirectory(path.str, NULL));
}

internal b32
OS_W32_DirectoryDelete(String8 path)
{
	AssertTrue(RemoveDirectory(path.str));
}

internal b32
OS_W32_DirectoryExists(String8 path)
{
	DWORD attr = GetFileAttributes(path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

internal OS_Handle
OS_W32_StreamFromFile(String8 path, OS_FileMode mode)
{
	void *h = SDL_IOFromFile(path, mode);

	OS_Handle handle = { h };
	return h;
}

internal OS_Handle
OS_W32_StreamFromMemory(void *memory, u64 bytes)
{
	void *h = SDL_IOFromMem(path, mode);

	OS_Handle handle = { h };
	return h;
}

internal OS_Handle
OS_W32_StreamFromConstMemory(const void *memory, u64 bytes)
{
	void *h = SDL_IOFromConstMem(path, mode);

	OS_Handle handle = { h };
	return h;
}

internal i64
OS_W32_StreamRead(OS_Handle handle, void *dst, u64 bytes)
{
	return SDL_ReadIO(stream.value, dst, size);
}

internal i64
OS_W32_StreamWrite(OS_Handle handle, const void *src, u64 bytes)
{
	return SDL_WriteIO(stream.value, src, size);
}

internal i64
OS_W32_StreamSeek(OS_Handle handle, i64 offset)
{
	return SDL_SeekIO(stream.value, offset, SDL_IO_SEEK_SET);
}

internal u64
OS_W32_StreamSize(OS_Handle handle)
{
	return SDL_GetIOSize(stream.value);
}

internal i64
OS_W32_StreamPosition(OS_Handle handle)
{
	return SDL_TellIO(stream.value);
}

internal b32
OS_W32_StreamClose(OS_Handle handle)
{
	return SDL_CloseIO(stream.value);
}

internal void
OS_W32_OpenInExplorer(String8 path)
{
	ShellExecute(NULL, "open", path.str, NULL, NULL, SW_SHOWDEFAULT);
}

internal b32
OS_W32_VulkanSurfaceCreate(void *instance, void *surface_ptr)
{
	return SDL_Vulkan_CreateSurface(win32_st.sdl_window, instance, NULL, surface_pointer);
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
	for (u32 i = 0; i < gamepad_count; i++)
	{
		SDL_CloseGamepad(gamepads[i]);
		gamepads[i] = NULL;
	}

	gamepad_count = 0;
}

internal void
OS_W32_ReconnectAllGamepads(void)
{
	if (gamepad_count > 0)
		OS_W32_CloseAllGamepads();

	SDL_JoystickID *ids = SDL_GetGamepads(&gamepad_count);

	for (u32 i = 0; i < gamepad_count; i++)
	{
		gamepads[i] = SDL_OpenGamepad(ids[i]);

		if (gamepads[i])
			DebugLogF("Added gamepad with player index: %d", SDL_GetGamepadPlayerIndex(gamepads[i]));
		else
			DebugLogF("Failed to open gamepad: %d.", SDL_GetGamepadPlayerIndex(gamepads[i]));
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

internal void
OS_W32_MessagePump(void)
{
	SDL_Event local_events[MAX_PENDING_EVENTS] = {0};
	u32 local_event_count = 0;

	SDL_Event ev = {0};

	while (SDL_PollEvent(&ev) && local_event_count < array_size(local_events))
		local_events[local_event_count++] = ev;

	if (local_event_count > 0)
	{
		OS_MutexLock(win32_st.event_mutex);
		
		u32 available_space = MAX_PENDING_EVENTS - pending_event_count;
		u32 copy_count = (local_event_count > available_space) ? available_space : local_event_count;

		memory_copy(pending_events + pending_event_count, local_events, copy_count * sizeof(SDL_Event));
		pending_event_count += copy_count;

		OS_MutexUnlock(win32_st.event_mmutex);
	}
}

internal void
OS_W32_BindAPI(OS_API *osapi)
{
	osapi->VirtualReserve = OS_W32_VirtualReserve;
	osapi->VirtualCommit = OS_W32_VirtualCommit;
	osapi->VirtualFree = OS_W32_VirtualFree;
	osapi->GetPageSize = OS_W32_GetPageSize;
	osapi->SetWindowTitle = OS_W32_SetWindowTitle;
	osapi->GetWindowSize = OS_W32_GetWindowSize;
	osapi->GetWindowSizeInPixels = OS_W32_GetWindowSizeInPixels;
	osapi->SetWindowSize = OS_W32_SetWindowSize;
	osapi->SetWindowFullscreen = OS_W32_SetWindowFullscreen;
	osapi->SetWindowBorderless = OS_W32_SetWindowBorderless;
	osapi->SetWindowOpacity = OS_W32_SetWindowOpacity;
	osapi->SetMousePosition = OS_W32_SetMousePosition;
	osapi->SetMouseVisible = OS_W32_SetMouseVisible;
	osapi->IsMouseVisible = OS_W32_IsMouseVisible;
	osapi->SetMouseLocked = OS_W32_SetMouseLocked;
	osapi->IsMouseLocked = OS_W32_IsMouseLocked;
	osapi->GetTicks = OS_W32_GetTicks;
	osapi->GetPerformanceCounter = OS_W32_GetPerformanceCounter;
	osapi->GetPerformanceFrequency = OS_W32_GetPerformanceFrequency;
	osapi->GetNumCores = OS_W32_GetNumCores;
	osapi->Handle = OS_W32_Handle;
	osapi->ThreadJoin = OS_W32_ThreadJoin;
	osapi->ThreadDetach = OS_W32_ThreadDetach;
	osapi->ThreadSetAffinity = OS_W32_ThreadSetAffinity;
	osapi->Handle = OS_W32_Handle;
	osapi->Handle = OS_W32_Handle;
	osapi->FiberDelete = OS_W32_FiberDelete;
	osapi->SwitchToFiber = OS_W32_SwitchToFiber;
	osapi->Handle = OS_W32_Handle;
	osapi->ConvertFiberToThread = OS_W32_ConvertFiberToThread;
	osapi->AtomicLoadU32 = OS_W32_AtomicLoadU32;
	osapi->AtomicLoadU64 = OS_W32_AtomicLoadU64;
	osapi->AtomicLoadPtr = OS_W32_AtomicLoadPtr;
	osapi->AtomicStoreU32 = OS_W32_AtomicStoreU32;
	osapi->AtomicStoreU64 = OS_W32_AtomicStoreU64;
	osapi->AtomicStorePtr = OS_W32_AtomicStorePtr;
	osapi->AtomicAddU32 = OS_W32_AtomicAddU32;
	osapi->AtomicAddU64 = OS_W32_AtomicAddU64;
	osapi->AtomicSubU32 = OS_W32_AtomicSubU32;
	osapi->AtomicSubU64 = OS_W32_AtomicSubU64;
	osapi->SpinLockAcquire = OS_W32_SpinLockAcquire;
	osapi->SpinLockRelease = OS_W32_SpinLockRelease;
	osapi->Handle = OS_W32_Handle;
	osapi->MutexDestroy = OS_W32_MutexDestroy;
	osapi->MutexLock = OS_W32_MutexLock;
	osapi->MutexUnlock = OS_W32_MutexUnlock;
	osapi->Handle = OS_W32_Handle;
	osapi->CondVarDestroy = OS_W32_CondVarDestroy;
	osapi->CondVarWait = OS_W32_CondVarWait;
	osapi->CondVarSignal = OS_W32_CondVarSignal;
	osapi->CondVarBroadcast = OS_W32_CondVarBroadcast;
	osapi->FileDelete = OS_W32_FileDelete;
	osapi->FileExists = OS_W32_FileExists;
	osapi->GetFileLastWriteTime = OS_W32_GetFileLastWriteTime;
	osapi->DirectoryCreate = OS_W32_DirectoryCreate;
	osapi->DirectoryDelete = OS_W32_DirectoryDelete;
	osapi->DirectoryExists = OS_W32_DirectoryExists;
	osapi->Handle = OS_W32_Handle;
	osapi->Handle = OS_W32_Handle;
	osapi->Handle = OS_W32_Handle;
	osapi->StreamRead = OS_W32_StreamRead;
	osapi->StreamWrite = OS_W32_StreamWrite;
	osapi->StreamSeek = OS_W32_StreamSeek;
	osapi->StreamSize = OS_W32_StreamSize;
	osapi->StreamPosition = OS_W32_StreamPosition;
	osapi->StreamClose = OS_W32_StreamClose;
	osapi->JobCounterAlloc = JOB_JobCounterAlloc;
	osapi->JobYield = JOB_JobYield;
	osapi->JobKick = JOB_JobKick;
	osapi->JobBatch = JOB_JobBatch;
	osapi->JobFor = JOB_JobFor;
	osapi->JobIsMainThread = JOB_JobIsMainThread;
	osapi->JobGetScratch = JOB_GetScratch;
	osapi->OpenInExplorer = OS_W32_OpenInExplorer;
	osapi->VulkanSurfaceCreate = OS_W32_VulkanSurfaceCreate;
	osapi->VulkanSurfaceDestroy = OS_W32_VulkanSurfaceDestroy;
	osapi->VulkanGetInstanceExtensions = OS_W32_VulkanGetInstanceExtensions;
}

internal void
OS_W32_ProcessEvents(I_State *input_out)
{
	SDL_Event events[MAX_PENDING_EVENTS] = {0};
	u32 event_count = 0;

	OS_MutexLock(event_mutex);
	{
		MemCpy(events, pending_events, pending_event_count * sizeof(SDL_Event));
		event_count = pending_event_count;
		pending_event_count = 0;
	}
	OS_MutexUnlock(event_mutex);
	
	// Reset button input states.
	MemZeroArray(input_out->kb_pressed);
	MemZeroArray(input_out->kb_released);
	MeMZeroArray(input_out->mb_pressed);
	MemZeroArray(input_out->mb_released);

	for (int i = 0; i < inp::MAX_GAMEPADS; i++) {
		I_GamepadSt *gp = input_out->gamepads[i];
		MemZeroArray(gp->pressed);
		MemZeroArray(gp->released);
	}

	// Reset mouse state.
	MemZeroStruct(&input_out->mouse_delta);
	MemZeroStruct(&input_out->mouse_wheel);

	for (u32 i = 0; i < event_count; i++) {
		const SDL_Event &ev = events[i];

		ImGui_ImplSDL3_ProcessEvent(&ev);

		switch (ev.type) {
			case SDL_EVENT_QUIT:
				JOB_Halt();
				break;

			case SDL_EVENT_KEY_DOWN:
				input_out->kb_down[ev.key.scancode] = true;
				input_out->kb_pressed[ev.key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				input_out->kb_down[ev.key.scancode] = false;
				input_out->kb_released[ev.key.scancode] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				input_out->mb_down[ev.button.button] = true;
				input_out->mb_pressed[ev.button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				input_out->mb_down[ev.button.button] = false;
				input_out->mb_released[ev.button.button] = true;
				break;

			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetGlobalMouseState(&input_out->mouse_screen_position.x, &input_out->mouse_screen_position.y);
				input_out->mouse_position = v2(ev.motion.x, ev.motion.y);
				input_out->mouse_delta = V2Add(input_out->mouse_delta, v2(ev.motion.xrel, ev.motion.yrel));
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				input_out->mouse_wheel += v2(ev.wheel.x, ev.wheel.y);
				break;
					
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev.gbutton.which)].down[ev.gbutton.button] = true;
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev.gbutton.which)].pressed[ev.gbutton.button] = true;
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev.gbutton.which)].down[ev.gbutton.button] = false;
				input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev.gbutton.which)].released[ev.gbutton.button] = true;
				break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				I_GamepadStSetAxisValue(&input_out->gamepads[SDL_GetGamepadPlayerIndexForID(ev.gaxis.which)],
										(I_GamepadAxis)ev.gaxis.axis,
										(f32)ev.gaxis.value / (f32)(SDL_JOYSTICK_AXIS_MAX - ((ev.gaxis.value >= 0.f) ? 1.f : 0.f)));
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				OS_W32_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				DebugLog("Removed gamepad.");
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
}

JOB_ENTRY_POINT_DEF(OS_W32_FrameJobEntry)
{
	static I_State prev_input_st = {0};

	I_State curr_input_st = prev_input_st;
	OS_W32_ProcessEvents(&curr_input_st);
	prev_input_st = curr_input_st;

	OS_ImGuiNewFrame();

	// ---
	
	if (win32_st.code.Tick(win32_st.app, &curr_input_st))
	{
		JOB_Halt();
	}
	else
	{
		JOB_Decl next_frame_job = {0};
		next_frame_job.EntryPoint = OS_W32_FrameJobEntry;
		next_frame_job.priority = JOB_Priority_Normal;

		JOB_Kick(&next_frame_job, NULL);
	}
}

JOB_ENTRY_POINT_DEF(OS_W32_RootJobEntry)
{
	win32_st.app = win32_st.code.Init(&win32_st.process_arena, &win32_st.api);

	JOB_Decl first_frame_job = {0};
	first_frame_job.EntryPoint = OS_W32_FrameJobEntry;
	first_frame_job.priority = JOB_Priority_Normal;

	JOB_Kick(&first_frame_job, NULL);
}

i32
main(void)
{
	GetSystemInfo(&win32_system_info);

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
	
	OS_W32_InitImGui();
	OS_W32_LoadCode(String8Lit("build/app.dll"));

	const u64 process_memory_size = OS_PROCESS_MEMORY;
	void *process_memory = malloc(process_memory_size);
	MemSet(process_memory, 0, process_memory_size);
	win32_st.process_arena = ArenaInitMemory(process_memory, process_memory_size);

	JOB_Scheduler scheduler = {0};	
	JOB_InitAndSelect(&win32_st.process_arena, &scheduler);

	JOB_Decl root_job = {0};
	root_job.EntryPoint = OS_W32_RootJobEntry;
	root_job.priority = JOB_Priority_Normal;

	JOB_Kick(&root_job, NULL);
	
	JOB_Enter(OS_W32_MessagePump);
	
	win32_st.code.Destroy(win32_st.app);
	
	JOB_Shutdown();

	free(process_memory);

	OS_W32_DestroyImGui();
	SDL_DestroyWindow(win32_st.sdl_window);
	SDL_Quit();
	
	return 0;
}
