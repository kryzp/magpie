#include "system_backend.h"
#include "common.h"
#include "input/input.h"
#include "graphics/backend.h"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

llt::SystemBackend* llt::g_systemBackend = nullptr;

using namespace llt;

SystemBackend::SystemBackend(const Config& config)
	: m_window(nullptr)
	, m_gamepads{}
	, m_gamepadCount(0)
{
	if (!SDL_Init(
		SDL_INIT_TIMER |
		SDL_INIT_AUDIO |
		SDL_INIT_VIDEO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_HAPTIC |
		SDL_INIT_GAMEPAD |
		SDL_INIT_EVENTS |
		SDL_INIT_SENSOR |
		SDL_INIT_CAMERA) != 0)
	{
		LLT_ERROR("[SDL|DEBUG] Failed to initialize: %s", SDL_GetError());
	}

	uint64_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;

	if (config.hasFlag(Config::FLAG_RESIZABLE)) {
		flags |= SDL_WINDOW_RESIZABLE;
	}

	if (config.hasFlag(Config::FLAG_HIGH_PIXEL_DENSITY)) {
		flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	}

	flags |= SDL_WINDOW_VULKAN;

	m_window = SDL_CreateWindow(config.name, config.width, config.height, flags);

	if (!m_window) {
		LLT_ERROR("[SDL|DEBUG] Failed to create window.");
	}

	LLT_LOG("[SDL] Initialized!");
}

SystemBackend::~SystemBackend()
{
	closeAllGamepads();

	SDL_DestroyWindow(m_window);
	SDL_Quit();

	LLT_LOG("[SDL] Destroyed!");
}

void SystemBackend::pollEvents()
{
	float spx = 0.f, spy = 0.f;
	SDL_Event ev = {};

	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_EVENT_QUIT:
				LLT_LOG("[SDL] Detected window close event, quitting...");
				g_app->exit();
				break;

			case SDL_EVENT_WINDOW_RESIZED:
				LLT_LOG("[SDL] Detected window resize!");
				g_vulkanBackend->onWindowResize(ev.window.data1, ev.window.data2);
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				g_inputState->onMouseWheel(ev.wheel.x, ev.wheel.y);
				break;

			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetGlobalMouseState(&spx, &spy);
				g_inputState->onMouseScreenMove(spx, spy);
				g_inputState->onMouseMove(ev.motion.x, ev.motion.y);
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				g_inputState->onMouseDown(ev.button.button);
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				g_inputState->onMouseUp(ev.button.button);
				break;

			case SDL_EVENT_KEY_DOWN:
				g_inputState->onKeyDown(ev.key.scancode);
				break;

			case SDL_EVENT_KEY_UP:
				g_inputState->onKeyUp(ev.key.scancode);
				break;

			case SDL_EVENT_TEXT_INPUT:
				g_inputState->onTextUtf8(ev.text.text);
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				g_inputState->onGamepadButtonDown(ev.gbutton.button, SDL_GetGamepadPlayerIndexForID(ev.gbutton.which));
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				g_inputState->onGamepadButtonUp(ev.gbutton.button, SDL_GetGamepadPlayerIndexForID(ev.gbutton.which));
				break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				g_inputState->onGamepadMotion(
					SDL_GetGamepadPlayerIndexForID(ev.gaxis.which),
					(GamepadAxis)ev.gaxis.axis,
					(float)(ev.gaxis.value) / (float)(SDL_JOYSTICK_AXIS_MAX - ((ev.gaxis.value >= 0) ? 1.0f : 0.0f))
				);
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				LLT_LOG("[SDL] Gamepad added, trying to reconnect all gamepads...");
				reconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				LLT_LOG("[SDL] Gamepad removed.");
				break;

			case SDL_EVENT_GAMEPAD_REMAPPED:
				LLT_LOG("[SDL] Gamepad remapped.");
				break;

			default:
				break;
		}
	}
}

void SystemBackend::reconnectAllGamepads()
{
	// if we already have some gamepads connected disconnect them
	if (m_gamepads[0] != nullptr) {
		closeAllGamepads();
	}

	// find all connected gamepads
	SDL_JoystickID* gamepadIDs = SDL_GetGamepads(&m_gamepadCount);

	if (m_gamepadCount == 0)
	{
		LLT_LOG("[SDL] No gamepads found!");
		goto finished;
	}
	else
	{
		LLT_LOG("[SDL] Found %d gamepads!", m_gamepadCount);
	}

	// iterate through found gamepads
	for (int i = 0; i < m_gamepadCount; i++)
	{
		SDL_JoystickID id = gamepadIDs[i];
		m_gamepads[i] = SDL_OpenGamepad(id); // open the gamepad

		// check if we actually managed to open the gamepad
		if (m_gamepads[i]) {
			LLT_LOG("[SDL] Opened gamepad with id: %d, internal index: %d, and player index: %d.", id, i,
				SDL_GetGamepadPlayerIndex(m_gamepads[i])
			);
		} else {
			LLT_LOG("[SDL] Failed to open gamepad with id: %d, and internal index: %d.", id, i);
		}
	}

finished:
	SDL_free(gamepadIDs); // finished, free the gamepad_ids array
}

void SystemBackend::closeAllGamepads()
{
	for (int i = 0; i < m_gamepadCount; i++) {
		SDL_CloseGamepad(m_gamepads[i]);
		LLT_LOG("[SDL] Closed gamepad with internal index %d.", i);
	}

	m_gamepadCount = 0;
	m_gamepads.clear();
}

GamepadType SystemBackend::getGamepadType(int player)
{
	return (GamepadType)SDL_GetGamepadType(SDL_GetGamepadFromPlayerIndex(player));
}

void SystemBackend::closeGamepad(int player) const
{
	SDL_CloseGamepad(SDL_GetGamepadFromPlayerIndex(player));
}

String SystemBackend::getWindowName() const
{
	return SDL_GetWindowTitle(m_window);
}

void SystemBackend::setWindowName(const String& name)
{
	SDL_SetWindowTitle(m_window, name.cstr());
}

glm::ivec2 SystemBackend::getWindowPosition() const
{
	glm::ivec2 result = { 0, 0 };
	SDL_GetWindowPosition(m_window, &result.x, &result.y);
	return result;
}

void SystemBackend::setWindowPosition(const glm::ivec2& position)
{
	SDL_SetWindowPosition(m_window, position.x, position.y);
}

glm::ivec2 SystemBackend::getWindowSize() const
{
	glm::ivec2 result = { 0, 0 };
	SDL_GetWindowSize(m_window, &result.x, &result.y);
	return result;
}

void SystemBackend::setWindowSize(const glm::ivec2& size)
{
	SDL_SetWindowSize(m_window, size.x, size.y);
}

glm::ivec2 SystemBackend::getScreenSize() const
{
	const SDL_DisplayMode* out = SDL_GetCurrentDisplayMode(1);

	if (out) {
		return { out->w, out->h };
	}

	return { 0, 0 };
}

float SystemBackend::getWindowOpacity() const
{
	return SDL_GetWindowOpacity(m_window);
}

void SystemBackend::setWindowOpacity(float opacity) const
{
	SDL_SetWindowOpacity(m_window, opacity);
}

bool SystemBackend::isWindowResizable() const
{
	return SDL_GetWindowFlags(m_window) & SDL_WINDOW_RESIZABLE;
}

void SystemBackend::toggleWindowResizable(bool toggle) const
{
	SDL_SetWindowResizable(m_window, static_cast<SDL_bool>(toggle));
}

float SystemBackend::getWindowRefreshRate() const
{
	const SDL_DisplayMode* out = SDL_GetCurrentDisplayMode(1);

	if (out) {
		return out->refresh_rate;
	}

	return 0.0f;
}

float SystemBackend::getWindowPixelDensity() const
{
	const SDL_DisplayMode* out = SDL_GetCurrentDisplayMode(1);

	if (out) {
		return out->pixel_density;
	}

	return 0.0f;
}

bool SystemBackend::isCursorVisible() const
{
	return SDL_CursorVisible();
}

void SystemBackend::toggleCursorVisible(bool toggle) const
{
	if (toggle) {
		SDL_ShowCursor();
	} else {
		SDL_HideCursor();
	}
}

void SystemBackend::lockCursor(bool toggle) const
{
	SDL_SetWindowRelativeMouseMode(m_window, toggle);
}

void SystemBackend::setCursorPosition(int x, int y)
{
	SDL_WarpMouseInWindow(m_window, x, y);

	float spx = 0.f, spy = 0.f;
	SDL_GetGlobalMouseState(&spx, &spy);

	g_inputState->onMouseScreenMove(spx, spy);
	g_inputState->onMouseMove(x, y);
}

WindowMode SystemBackend::getWindowMode() const
{
	auto flags = SDL_GetWindowFlags(m_window);

	if (flags & SDL_WINDOW_FULLSCREEN) {
		return WINDOW_MODE_FULLSCREEN;
	} else if (flags & SDL_WINDOW_BORDERLESS) {
		return WINDOW_MODE_BORDERLESS;
	}

	return WINDOW_MODE_WINDOWED;
}

void SystemBackend::setWindowMode(WindowMode toggle)
{
	switch (toggle)
	{
		case WINDOW_MODE_FULLSCREEN:
			SDL_SetWindowFullscreen(m_window, SDL_TRUE);
			SDL_SetWindowBordered(m_window, SDL_FALSE);
			break;

		case WINDOW_MODE_BORDERLESS_FULLSCREEN:
			SDL_SetWindowFullscreen(m_window, SDL_TRUE);
			SDL_SetWindowBordered(m_window, SDL_TRUE);
			break;

		case WINDOW_MODE_BORDERLESS:
			SDL_SetWindowFullscreen(m_window, SDL_FALSE);
			SDL_SetWindowBordered(m_window, SDL_FALSE);
			break;

		case WINDOW_MODE_WINDOWED:
			SDL_SetWindowFullscreen(m_window, SDL_FALSE);
			SDL_SetWindowBordered(m_window, SDL_TRUE);

		default:
			return;
	}
}

void SystemBackend::sleepFor(uint64_t ms) const
{
	if (ms > 0) {
		SDL_Delay(ms);
	}
}

uint64_t SystemBackend::getTicks() const
{
	return SDL_GetTicks();
}

uint64_t SystemBackend::getPerformanceCounter() const
{
	return SDL_GetPerformanceCounter();
}

uint64_t SystemBackend::getPerformanceFrequency() const
{
	return SDL_GetPerformanceFrequency();
}

void* SystemBackend::streamFromFile(const char* filepath, const char* mode)
{
	return SDL_IOFromFile(filepath, mode);
}

void* SystemBackend::streamFromMemory(void* memory, uint64_t size)
{
	return SDL_IOFromMem(memory, size);
}

void* SystemBackend::streamFromConstMemory(const void* memory, uint64_t size)
{
	return SDL_IOFromConstMem(memory, size);
}

int64_t SystemBackend::streamRead(void* stream, void* ptr, uint64_t size)
{
	return SDL_ReadIO((SDL_IOStream*)stream, ptr, size);
}

int64_t SystemBackend::streamWrite(void* stream, const void* ptr, uint64_t size)
{
	return SDL_WriteIO((SDL_IOStream*)stream, ptr, size);
}

int64_t SystemBackend::streamSeek(void* stream, int64_t offset)
{
	return SDL_SeekIO((SDL_IOStream*)stream, offset, SDL_IO_SEEK_SET);
}

int64_t SystemBackend::streamSize(void* stream)
{
	return SDL_GetIOSize((SDL_IOStream*)stream);
}

int64_t SystemBackend::streamPosition(void* stream)
{
	return SDL_TellIO((SDL_IOStream*)stream);
}

void SystemBackend::streamClose(void* stream)
{
	SDL_CloseIO((SDL_IOStream*)stream);
}

const char* const* SystemBackend::vkGetInstanceExtensions(uint32_t* count)
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

bool SystemBackend::vkCreateSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	LLT_LOG("[SDL] Created Vulkan surface!");
	return SDL_Vulkan_CreateSurface(m_window, instance, NULL, surface);
}
