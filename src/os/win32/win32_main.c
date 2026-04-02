
/* --- WIN32 PLATFORM LAYER --- */

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
# define W32_SPIN_PAUSE() do { _mm_pause() } while (0)
#else
# define W32_SPIN_PAUSE() do { } while (0)
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "core/core_inc.h"
#include "os/os_inc.h"
#include "input/input_inc.h"

#include "core/core_inc.c"
#include "os/os_inc.c"
#include "input/input_inc.c"

#include "app.h"

#define W32_OBJECT_MEMORY_SIZE Megabytes(16)
#define W32_APP_MEMORY_SIZE Gigabytes(4)

typedef struct W32_Object W32_Object;
struct W32_Object
{
	W32_Object *next_free;
	
	// Regular win32 CreateMutex(...) is more heavy-duty for sync
	// between multiple processes, so just use this instead.
	CRITICAL_SECTION cs; 

	CONDITION_VARIABLE cv;
};

typedef struct W32_Code W32_Code;
struct W32_Code
{
	HMODULE handle;
	FILETIME last_write_time;

	void (*Init)(const W32_BootstrapData *data);
	void (*Destroy)(void);
	b32  (*Tick)(const I_InputSt *input);
	void (*HotLoad)(const OS_BootstrapData *data);
	void (*HotUnload)(void);
};

typedef struct W32_State W32_State;
struct W32_State
{
	Arena object_arena;
	W32_Object *free_objects;

	OS_API api;
	W32_Code code;
	
	SYSTEM_INFO system_info;
	SDL_Window *sdl_window;

	u32 pending_event_count;
	SDL_Event pending_events[512];

	OS_Handle event_mutex;

	u32 gamepad_count;
	SDL_Gamepad *gamepads[I_MAX_GAMEPADS];
};

global W32_State win32_st = {0};

internal W32_Object *
W32_AllocObject(void)
{
	W32_Object *object = win32_st.free_objects;

	if (object)
	{
		win32_st.free_objects = win32_st.free_objects->next;
		MemZeroStruct(object);
	}
	else
	{
		object = ArenaPushArray(&win32_st.object_arena, W32_Object, 1);
	}

	return object;
}

void W32_AppNullStubInit(const OS_BootstrapData *data) { }
void W32_AppNullStubDestroy(void) { }
b32  W32_AppNullStubTick(const I_InputSt *input) { return false; }
void W32_AppNullStubHotLoad(const OS_BootstrapData *data) { }
void W32_AppNullStubHotUnload(void) { }

internal void
W32_UnloadCode(void)
{
	win32_st.code.Init      = W32_AppNullStubInit;
	win32_st.code.Destroy   = W32_AppNullStubDestroy;
	win32_st.code.Tick      = W32_AppNullStubTick;
	win32_st.code.HotLoad   = W32_AppNullStubHotLoad;
	win32_st.code.HotUnload = W32_AppNullStubHotUnload;

	if (win32_st.code.handle)
	{
		FreeLibrary(win32_st.code.handle);
		win32_st.code.handle = NULL;
	}
}

internal void
W32_LoadCode(String8 dll_path)
{
	static const char *dll_path_hot = "build/hot_reload.dll";

	win32_st.code.last_write_time = OS_GetFileLastWriteTime(dll_path);

	CopyFile(dll_path.str, dll_path_hot, FALSE);

	win32_st.code.handle = LoadLibraryA(dll_path_hot);

	if (win32_st.code.handle)
	{
		win32_st.code.Init      = GetProcAddress(win32_st.code.handle, "AppInit");
		win32_st.code.Destroy   = GetProcAddress(win32_st.code.handle, "AppDestroy");
		win32_st.code.Tick      = GetProcAddress(win32_st.code.handle, "AppTick");
		win32_st.code.HotLoad   = GetProcAddress(win32_st.code.handle, "AppHotLoad");
		win32_st.code.HotUnload = GetProcAddress(win32_st.code.handle, "AppHotUnload");
	}
	else
	{
		W32_UnloadCode();
	}
}

internal void
W32_ReturnObject(W32_Object *object)
{
	object->next = win32_st.free_objects;
	win32_st.free_objects = object;
}

internal void *
W32_VirtualReserve(u64 bytes)
{
	return VirtualAlloc(NULL,
						bytes,
						MEM_RESERVE,
						PAGE_READWRITE);
}

internal void
W32_VirtualCommit(void *address, u64 bytes)
{
	VirtualAlloc(address,
				 bytes,
				 MEM_COMMIT,
				 PAGE_READWRITE);
}

internal void
W32_VirtualFree(void *address)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}

internal u64
W32_GetPageSize(void)
{
	return system_info.dwPageSize;
}

internal void
W32_SetWindowTitle(String8 title)
{
	SDL_SetWindowTitle(win32_st.sdl_window, title);
}

internal void
W32_GetWindowSize(u32 *w, u32 *h)
{
	SDL_GetWindowSize(win32_st.sdl_window, width, height);
}

internal void
W32_GetWindowSizeInPixels(u32 *pw, u32 *ph)
{
	SDL_GetWindowSizeInPixels(win32_st.sdl_window, pixel_width, pixel_height);
}

internal void
W32_SetWindowSize(u32 w, u32 h)
{
	SDL_SetWindowSize(win32_st.sdl_window, width, height);
}

internal void
W32_SetWindowFullscreen(b32 fullscreen)
{
	SDL_SetWindowFullscreen(win32_st.sdl_window, b);
}

internal void
W32_SetWindowBorderless(b32 borderless)
{
	SDL_SetWindowBordered(win32_st.sdl_window, !b);
}

internal void
W32_SetWindowOpacity(f32 opacity)
{
	SDL_SetWindowOpacity(win32_st.sdl_window, opacity);
}

internal void
W32_SetMousePosition(f32 x, f32 y)
{
	SDL_WarpMouseInWindow(win32_st.sdl_window, x, y);
}

internal void
W32_SetMouseVisible(b32 visible)
{
	//	ImGui::SetMouseCursor(visible ? ImGuiMouseSource_Mouse : ImGuiMouseCursor_None);

	if (visible)
		SDL_ShowCursor();
	else
		SDL_HideCursor();
}

internal b32
W32_IsMouseVisible(void)
{
	return SDL_CursorVisible();
}

internal void
W32_SetMouseLocked(b32 locked)
{
	SDL_SetWindowRelativeMouseMode(win32_st.sdl_window, locked);
}

internal b32
W32_IsMouseLocked(void)
{
	return SDL_GetWindowRelativeMouseMode(win32_st.sdl_window);
}

internal u64
W32_GetTicks(void)
{
	return SDL_GetTicks();
}

internal u64
W32_GetPerformanceCounter(void)
{
	return SDL_GetPerformanceCounter();
}

internal u64
W32_GetPerformanceFrequency(void)
{
	return SDL_GetPerformanceFrequency();
}

internal u32
W32_GetNumCores(void)
{
	return win32_system_info.dwNumberOfProcessors;
}

typedef struct W32_ThreadStart W32_ThreadStart;
struct W32_ThreadStart
{
	void (*Entry)(void *param);
	void *param;
}

internal DWORD WINAPI
W32_ThreadTrampoline(LPVOID param)
{
	W32_ThreadStart *t = param;
	t->Entry(t->param);
	free(t);
	return 0;
}

internal OS_Handle
W32_ThreadCreate(void (*Entry)(void *param), void *param)
{
	// TODO: switch to arena alloc
	W32_ThreadStart *t = malloc(sizeof(W32_ThreadStart));
	t->entry = Entry;
	t->param = param;

	return CreateThread(NULL, 0, W32_ThreadTrampoline, t, 0, NULL);
}

internal void
W32_ThreadJoin(OS_Handle handle)
{
	WaitForSingleObject(handle.value, INFINITE);
	CloseHandle(handle.value);
}

internal void
W32_ThreadDetach(OS_Handle handle)
{
	CloseHandle(handle.value);
}

internal void
W32_ThreadSetAffinity(OS_Handle handle, u64 mask)
{
	SetThreadAffinityMask(handle, mask);
}

internal OS_Handle
W32_GetCurrentThreadHandle(void)
{
	return GetCurrentThread();
}

internal OS_Handle
W32_FiberCreate(u32 stack_size, void (*Entry)(void *param), void *param)
{
	return CreateFiber(stack_size, entry, param);
}

internal void
W32_FiberDelete(OS_Handle handle)
{
	DeleteFiber(handle);
}

internal void
W32_SwitchToFiber(OS_Handle handle)
{
	SwitchToFiber(handle);
}

internal OS_Handle
OS_ConvertThreadToFiber(void)
{
	return ConvertThreadToFiber(NULL);
}

internal u32
W32_ConvertFiberToThread(void)
{
	return ConvertFiberToThread();
}

internal u32
W32_AtomicLoadU32(u32 *ptr)
{
	return InterlockedCompareExchange((volatile LONG *)ptr, 0, 0);
}

internal u64
W32_AtomicLoadU64(u64 *ptr)
{
	return InterlockedCompareExchange64((volatile LONGLONG *)ptr, 0, 0);
}

internal void *
W32_AtomicLoadPtr(void *ptr)
{
	return InterlockedCompareExchangePointer((PVOID *)ptr, NULL, NULL);
}

internal void
W32_AtomicStoreU32(u32 *ptr, u32 value)
{
	InterlockedExchange((volatile LONG *)ptr, value);
}

internal void
W32_AtomicStoreU64(u64 *ptr, u64 value)
{
	InterlockedExchange64((volatile LONGLONG *)ptr, value);
}

internal void
W32_AtomicStorePtr(void *ptr, void *value)
{
	InterlockedExchangePointer((PVOID *)ptr, value);
}

internal u32
W32_AtomicAddU32(u32 *ptr, u32 delta)
{
	return InterlockedExchangeAdd((volatile LONG *)ptr, delta);
}

internal u64
W32_AtomicAddU64(u64 *ptr, u64 delta)
{
	return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, delta);
}

internal u32
W32_AtomicSubU32(u32 *ptr, u32 delta)
{
	return InterlockedExchangeAdd((volatile LONG *)ptr, -(LONG)delta);
}

internal u64
W32_AtomicSubU64(volatile u64 *ptr, u64 delta)
{
	return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, -(LONGLONG)delta);
}

internal void
W32_AtomicSpinLockAcquire(u32 *lock)
{
	while (W32_AtomicStoreU32(lock))
		W32_SPIN_PAUSE();
}

internal void
W32_AtomicSpinLockRelease(u32 *lock)
{
	W32_AtomicStoreU32(lock, false);
}

internal OS_Handle
W32_MutexCreate(void)
{
	W32_Object *object = W32_AllocObject();
	
	InitializeCriticalSection(&object->cs);
	
	OS_Handle handle = { object };
	return handle;
}

internal void
W32_MutexDestroy(OS_Handle handle)
{
	W32_Object *object = handle.value;
	
	DestroyCriticalSection(&object->cs);

	W32_ReturnObject(object);
}

internal void
W32_MutexLock(OS_Handle handle)
{
	W32_Object *object = handle.value;
	
	EnterCriticalSection(&object->cs);
}

internal void
W32_MutexUnlock(OS_Handle handle)
{
	W32_Object *object = handle.value;
	
	LeaveCriticalSection(&object->cs);
}

internal OS_Handle
W32_CondVarCreate(void)
{
	W32_Object *object = W32_AllocObject();

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
W32_CondVarDestroy(OS_Handle handle)
{
	W32_Object *object = handle.value;	
	W32_ReturnObject(object);
}

internal void
W32_CondVarWait(OS_Handle handle, OS_Handle mutex_handle)
{
	W32_Object *cnd = handle.value;
	W32_Object *mtx = mutex_handle.value;
	
	SleepConditionVariable(&cnd->cv, &mtx->cs, INFINITE);
}

internal void
W32_CondVarSignal(OS_Handle handle)
{
	W32_Object *object = handle.value;

	WakeConditionVariable(&object->cv);
}

internal void
W32_CondVarBroadcast(OS_Handle handle)
{
	W32_Object *object = handle.value;

	WakeAllConditionVariable(&object->cv);
}

internal b32
W32_FileDelete(String8 path)
{
	AssertTrue(DeleteFile(path.str));
}

internal b32
W32_FileExists(String8 path)
{
	DWORD attr = GetFileAttributes(path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

internal u64
W32_GetFileLastWriteTime(String8 path)
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
W32_DirectoryCreate(String8 path)
{
	AssertTrue(CreateDirectory(path.str, NULL));
}

internal b32
W32_DirectoryDelete(String8 path)
{
	AssertTrue(RemoveDirectory(path.str));
}

internal b32
W32_DirectoryExists(String8 path)
{
	DWORD attr = GetFileAttributes(path.str);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false; // Doesn't exist in the filesystem.

	return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

internal OS_Handle
W32_StreamFromFile(String8 path, OS_FileMode mode)
{
	void *h = SDL_IOFromFile(path, mode);

	OS_Handle handle = { h };
	return h;
}

internal OS_Handle
W32_StreamFromMemory(void *memory, u64 bytes)
{
	void *h = SDL_IOFromMem(path, mode);

	OS_Handle handle = { h };
	return h;
}

internal OS_Handle
W32_StreamFromConstMemory(const void *memory, u64 bytes)
{
	void *h = SDL_IOFromConstMem(path, mode);

	OS_Handle handle = { h };
	return h;
}

internal i64
W32_StreamRead(OS_Handle handle, void *dst, u64 bytes)
{
	return SDL_ReadIO(stream.value, dst, size);
}

internal i64
W32_StreamWrite(OS_Handle handle, const void *src, u64 bytes)
{
	return SDL_WriteIO(stream.value, src, size);
}

internal i64
W32_StreamSeek(OS_Handle handle, i64 offset)
{
	return SDL_SeekIO(stream.value, offset, SDL_IO_SEEK_SET);
}

internal u64
W32_StreamSize(OS_Handle handle)
{
	return SDL_GetIOSize(stream.value);
}

internal i64
W32_StreamPosition(OS_Handle handle)
{
	return SDL_TellIO(stream.value);
}

internal b32
W32_StreamClose(OS_Handle handle)
{
	return SDL_CloseIO(stream.value);
}

internal void
W32_OpenInExplorer(String8 path)
{
	ShellExecute(NULL, "open", path.str, NULL, NULL, SW_SHOWDEFAULT);
}

internal b32
W32_VulkanSurfaceCreate(void *instance, void *surface_ptr)
{
	return SDL_Vulkan_CreateSurface(win32_st.sdl_window, instance, NULL, surface_pointer);
}

internal void
W32_VulkanSurfaceDestroy(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface(instance, surface, NULL);
}

internal const char *const *
W32_VulkanGetInstanceExtensions(u32 *count)
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

internal void
W32_CloseAllGamepads(void)
{
	for (u32 i = 0; i < gamepad_count; i++)
	{
		SDL_CloseGamepad(gamepads[i]);
		gamepads[i] = NULL;
	}

	gamepad_count = 0;
}

internal void
W32_ReconnectAllGamepads(void)
{
	if (gamepad_count > 0)
		W32_CloseAllGamepads();

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
W32_InitImGui(void)
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
W32_DestroyImGui(void)
{
	/*
	  ImGui_ImplSDL3_Shutdown();
	  ImGui::DestroyContext();
	*/
}

internal void
W32_MessagePump(void)
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
W32_BindAPI(OS_API *osapi)
{
	osapi->VirtualReserve = W32_VirtualReserve;
	osapi->VirtualCommit = W32_VirtualCommit;
	osapi->VirtualFree = W32_VirtualFree;
	osapi->GetPageSize = W32_GetPageSize;
	osapi->SetWindowTitle = W32_SetWindowTitle;
	osapi->GetWindowSize = W32_GetWindowSize;
	osapi->GetWindowSizeInPixels = W32_GetWindowSizeInPixels;
	osapi->SetWindowSize = W32_SetWindowSize;
	osapi->SetWindowFullscreen = W32_SetWindowFullscreen;
	osapi->SetWindowBorderless = W32_SetWindowBorderless;
	osapi->SetWindowOpacity = W32_SetWindowOpacity;
	osapi->SetMousePosition = W32_SetMousePosition;
	osapi->SetMouseVisible = W32_SetMouseVisible;
	osapi->IsMouseVisible = W32_IsMouseVisible;
	osapi->SetMouseLocked = W32_SetMouseLocked;
	osapi->IsMouseLocked = W32_IsMouseLocked;
	osapi->GetTicks = W32_GetTicks;
	osapi->GetPerformanceCounter = W32_GetPerformanceCounter;
	osapi->GetPerformanceFrequency = W32_GetPerformanceFrequency;
	osapi->GetNumCores = W32_GetNumCores;
	osapi->Handle = W32_Handle;
	osapi->ThreadJoin = W32_ThreadJoin;
	osapi->ThreadDetach = W32_ThreadDetach;
	osapi->ThreadSetAffinity = W32_ThreadSetAffinity;
	osapi->Handle = W32_Handle;
	osapi->Handle = W32_Handle;
	osapi->FiberDelete = W32_FiberDelete;
	osapi->SwitchToFiber = W32_SwitchToFiber;
	osapi->Handle = W32_Handle;
	osapi->ConvertFiberToThread = W32_ConvertFiberToThread;
	osapi->AtomicLoadU32 = W32_AtomicLoadU32;
	osapi->AtomicLoadU64 = W32_AtomicLoadU64;
	osapi->AtomicLoadPtr = W32_AtomicLoadPtr;
	osapi->AtomicStoreU32 = W32_AtomicStoreU32;
	osapi->AtomicStoreU64 = W32_AtomicStoreU64;
	osapi->AtomicStorePtr = W32_AtomicStorePtr;
	osapi->AtomicAddU32 = W32_AtomicAddU32;
	osapi->AtomicAddU64 = W32_AtomicAddU64;
	osapi->AtomicSubU32 = W32_AtomicSubU32;
	osapi->AtomicSubU64 = W32_AtomicSubU64;
	osapi->AtomicSpinLockAcquire = W32_AtomicSpinLockAcquire;
	osapi->AtomicSpinLockRelease = W32_AtomicSpinLockRelease;
	osapi->Handle = W32_Handle;
	osapi->MutexDestroy = W32_MutexDestroy;
	osapi->MutexLock = W32_MutexLock;
	osapi->MutexUnlock = W32_MutexUnlock;
	osapi->Handle = W32_Handle;
	osapi->CondVarDestroy = W32_CondVarDestroy;
	osapi->CondVarWait = W32_CondVarWait;
	osapi->CondVarSignal = W32_CondVarSignal;
	osapi->CondVarBroadcast = W32_CondVarBroadcast;
	osapi->FileDelete = W32_FileDelete;
	osapi->FileExists = W32_FileExists;
	osapi->GetFileLastWriteTime = W32_GetFileLastWriteTime;
	osapi->DirectoryCreate = W32_DirectoryCreate;
	osapi->DirectoryDelete = W32_DirectoryDelete;
	osapi->DirectoryExists = W32_DirectoryExists;
	osapi->Handle = W32_Handle;
	osapi->Handle = W32_Handle;
	osapi->Handle = W32_Handle;
	osapi->StreamRead = W32_StreamRead;
	osapi->StreamWrite = W32_StreamWrite;
	osapi->StreamSeek = W32_StreamSeek;
	osapi->StreamSize = W32_StreamSize;
	osapi->StreamPosition = W32_StreamPosition;
	osapi->StreamClose = W32_StreamClose;
	osapi->JobCounterAlloc = JOB_JobCounterAlloc;
	osapi->JobYield = JOB_JobYield;
	osapi->JobKick = JOB_JobKick;
	osapi->JobBatch = JOB_JobBatch;
	osapi->JobFor = JOB_JobFor;
	osapi->JobIsMainThread = JOB_JobIsMainThread;
	osapi->JobGetScratch = JOB_GetScratch;
	osapi->OpenInExplorer = W32_OpenInExplorer;
	osapi->VulkanSurfaceCreate = W32_VulkanSurfaceCreate;
	osapi->VulkanSurfaceDestroy = W32_VulkanSurfaceDestroy;
	osapi->VulkanGetInstanceExtensions = W32_VulkanGetInstanceExtensions;
}

internal void
W32_ProcessEvents(I_InputSt *input_out)
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
				W32_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				DebugLog("Removed gamepad.");
				W32_ReconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMAPPED:
				SDL_ReloadGamepadMappings();
				W32_ReconnectAllGamepads();
				break;

			default:
				break;
		}
	}
}

JOB_ENTRY_POINT_DEF(W32_FrameJobEntry)
{
	static I_InputSt prev_input_st = {0};

	I_InputSt curr_input_st = prev_input_st;
	W32_ProcessEvents(&curr_input_st);
	prev_input_st = curr_input_st;

	OS_ImGuiNewFrame();

	// ---
	
	if (win32_st.code.Tick(&curr_input_st))
	{
		JOB_Halt();
	}
	else
	{
		JOB_Decl next_frame = {0};
		next_frame.EntryPoint = W32_FrameJobEntry;
		next_frame.priority = JOB_Priority_Normal;

		JOB_Kick(&next_frame, NULL);
	}
}

JOB_ENTRY_POINT_DEF(W32_RootJobEntry)
{
	OS_BootstrapData bootstrap_data = {0};
	bootstrap_data.memory = param;
	bootstrap_data.memory_size = W32_APP_MEMORY_SIZE;
	bootstrap_data.api = &win32_st.api;
	
	win32_st.code.Init(&bootstrap_data);

	JOB_Decl first_frame = {0};
	first_frame.EntryPoint = W32_FrameJobEntry;
	first_frame.priority = JOB_Priority_Normal;

	JOB_Kick(&first_frame, NULL);
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

	W32_BindAPI(&win32_st.api);
	
	W32_InitImGui();
	W32_LoadCode(String8("build/app.dll"));

	u64 job_memory_size =
		JOB_MAX_CONCURRENT_FIBERS * THREAD_CONTEXT_SCRATCH_RING_SIZE * JOB_FIBER_SCRATCH_SIZE +
		Megabytes(1);

	void *object_memory = malloc(W32_OBJECT_MEMORY_SIZE);
	win32_st.object_arena = ArenaInitMemory(object_memory, W32_OBJECT_MEMORY_SIZE);

	void *job_memory = malloc(job_memory_size);
	Arena job_arena = ArenaInitMemory(job_memory, job_memory_size);

	JOB_Scheduler scheduler = {0};	
	JOB_InitAndSelect(&job_arena, &scheduler);

	void *app_memory = malloc(W32_APP_MEMORY_SIZE);
	
	JOB_Decl root_job = {0};
	root_job.EntryPoint = W32_RootJobEntry;
	root_job.param = memory;
	root_job.priority = JOB_Priority_Normal;

	JOB_Kick(&root_job, NULL);
	
	JOB_Enter(W32_MessagePump);
	
	win32_st.code.Destroy();
	JOB_Shutdown();

	free(app_memory);
	free(job_memory);
	free(object_memory);

	W32_DestroyImGui();
	SDL_DestroyWindow(win32_st.sdl_window);
	SDL_Quit();
	
	return 0;
}
