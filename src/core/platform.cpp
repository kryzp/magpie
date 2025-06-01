#include "platform.h"

#include <SDL3/SDL_vulkan.h>

#include "third_party/imgui/imgui_impl_sdl3.h"

#include "common.h"

using namespace mgp;

Platform::Platform(const Config &config)
	: m_window(nullptr)
	, m_gamepads{}
	, m_gamepadCount(0)
{
	uint32_t initFlags =
		SDL_INIT_VIDEO |
		SDL_INIT_AUDIO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMEPAD |
		SDL_INIT_HAPTIC |
		SDL_INIT_EVENTS |
		SDL_INIT_SENSOR |
		SDL_INIT_CAMERA;

	if (SDL_Init(initFlags) == 0)
		MGP_ERROR("Failed to initialize: %s", SDL_GetError());

	uint64_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;

	if (config.hasFlag(CONFIG_FLAG_RESIZABLE_BIT))				flags |= SDL_WINDOW_RESIZABLE;
	if (config.hasFlag(CONFIG_FLAG_HIGH_PIXEL_DENSITY_BIT))		flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

	flags |= SDL_WINDOW_VULKAN;

	m_window = SDL_CreateWindow(config.windowName, config.width, config.height, flags);

	if (!m_window)
		MGP_ERROR("Failed to create window.");

	MGP_LOG("SDL Initialized!");
}

Platform::~Platform()
{
	closeAllGamepads();

	SDL_DestroyWindow(m_window);
	SDL_Quit();

	MGP_LOG("SDL Destroyed!");
}

void Platform::pollEvents(InputState *input)
{
	float spx = 0.0f, spy = 0.0f;
	SDL_Event ev = {};

	while (SDL_PollEvent(&ev))
	{
		ImGui_ImplSDL3_ProcessEvent(&ev);

		switch (ev.type)
		{
			case SDL_EVENT_QUIT:
				if (onExit)
					onExit();
				break;

			case SDL_EVENT_WINDOW_RESIZED:
				if (onWindowResize)
					onWindowResize(ev.window.data1, ev.window.data2);
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				input->onMouseWheel(ev.wheel.x, ev.wheel.y);
				break;

			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetGlobalMouseState(&spx, &spy);
				input->onMouseScreenMove(spx, spy);
				input->onMouseMove(ev.motion.x, ev.motion.y);
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				input->onMouseDown(ev.button.button);
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				input->onMouseUp(ev.button.button);
				break;

			case SDL_EVENT_KEY_DOWN:
				input->onKeyDown(ev.key.scancode);
				break;

			case SDL_EVENT_KEY_UP:
				input->onKeyUp(ev.key.scancode);
				break;

			case SDL_EVENT_TEXT_INPUT:
				input->onTextUtf8(ev.text.text);
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				input->onGamepadButtonDown(ev.gbutton.button, SDL_GetGamepadPlayerIndexForID(ev.gbutton.which));
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				input->onGamepadButtonUp(ev.gbutton.button, SDL_GetGamepadPlayerIndexForID(ev.gbutton.which));
				break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				input->onGamepadMotion(
					SDL_GetGamepadPlayerIndexForID(ev.gaxis.which),
					(GamepadAxis)ev.gaxis.axis,
					(float)(ev.gaxis.value) / (float)(SDL_JOYSTICK_AXIS_MAX - ((ev.gaxis.value >= 0) ? 1.0f : 0.0f))
				);
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				MGP_LOG("Gamepad added, trying to reconnect all gamepads...");
				reconnectAllGamepads();
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				MGP_LOG("Gamepad removed.");
				break;

			case SDL_EVENT_GAMEPAD_REMAPPED:
				MGP_LOG("Gamepad remapped.");
				break;

			default:
				break;
		}
	}
}

void Platform::reconnectAllGamepads()
{
	// if we already have some gamepads connected disconnect them
	if (m_gamepads[0] != nullptr)
		closeAllGamepads();

	// find all connected gamepads
	SDL_JoystickID *gamepadIDs = SDL_GetGamepads(&m_gamepadCount);

	if (m_gamepadCount == 0)
	{
		MGP_LOG("No gamepads found!");
		goto finished;
	}
	else
	{
		MGP_LOG("Found %d gamepads!", m_gamepadCount);
	}

	// iterate through found gamepads
	for (int i = 0; i < m_gamepadCount; i++)
	{
		SDL_JoystickID id = gamepadIDs[i];
		m_gamepads[i] = SDL_OpenGamepad(id); // open the gamepad

		// check if we actually managed to open the gamepad
		if (m_gamepads[i]) {
			MGP_LOG("Opened gamepad with id: %d, internal index: %d, and player index: %d.", id, i, SDL_GetGamepadPlayerIndex(m_gamepads[i]));
		} else {
			MGP_LOG("Failed to open gamepad with id: %d, and internal index: %d.", id, i);
		}
	}

finished:
	SDL_free(gamepadIDs); // finished, free the gamepad_ids array
}

void Platform::closeAllGamepads()
{
	for (int i = 0; i < m_gamepadCount; i++)
	{
		SDL_CloseGamepad(m_gamepads[i]);
		MGP_LOG("Closed gamepad with internal index %d.", i);
	}

	m_gamepadCount = 0;
}

GamepadType Platform::getGamepadType(int player)
{
	return (GamepadType)SDL_GetGamepadType(SDL_GetGamepadFromPlayerIndex(player));
}

void Platform::closeGamepad(int player) const
{
	SDL_CloseGamepad(SDL_GetGamepadFromPlayerIndex(player));
}

std::string Platform::getWindowName() const
{
	return SDL_GetWindowTitle(m_window);
}

void Platform::setWindowName(const std::string &name)
{
	SDL_SetWindowTitle(m_window, name.c_str());
}

glm::ivec2 Platform::getWindowPosition() const
{
	glm::ivec2 result = { 0, 0 };
	SDL_GetWindowPosition(m_window, &result.x, &result.y);
	return result;
}

void Platform::setWindowPosition(const glm::ivec2& position)
{
	SDL_SetWindowPosition(m_window, position.x, position.y);
}

glm::ivec2 Platform::getWindowSize() const
{
	glm::ivec2 result = { 0, 0 };
	SDL_GetWindowSize(m_window, &result.x, &result.y);
	return result;
}

void Platform::setWindowSize(const glm::ivec2& size)
{
	SDL_SetWindowSize(m_window, size.x, size.y);
}

glm::ivec2 Platform::getWindowSizeInPixels() const
{
	glm::ivec2 result = { 0, 0 };
	SDL_GetWindowSizeInPixels(m_window, &result.x, &result.y);
	return result;
}

glm::ivec2 Platform::getScreenSize() const
{
	const SDL_DisplayMode *out = SDL_GetCurrentDisplayMode(1);

	if (out) {
		return { out->w, out->h };
	} else {
		return { 0, 0 };
	}
}

float Platform::getWindowOpacity() const
{
	return SDL_GetWindowOpacity(m_window);
}

void Platform::setWindowOpacity(float opacity) const
{
	SDL_SetWindowOpacity(m_window, opacity);
}

bool Platform::isWindowResizable() const
{
	return SDL_GetWindowFlags(m_window) & SDL_WINDOW_RESIZABLE;
}

void Platform::toggleWindowResizable(bool toggle) const
{
	SDL_SetWindowResizable(m_window, toggle);
}

float Platform::getWindowRefreshRate() const
{
	return SDL_GetCurrentDisplayMode(1)->refresh_rate;
}

float Platform::getWindowPixelDensity() const
{
	return SDL_GetCurrentDisplayMode(1)->pixel_density;
}

bool Platform::isCursorVisible() const
{
	return SDL_CursorVisible();
}

void Platform::setCursorVisible(bool toggle) const
{
	toggle ? SDL_ShowCursor() : SDL_HideCursor();
}

void Platform::lockCursor(bool toggle) const
{
	SDL_SetWindowRelativeMouseMode(m_window, toggle);
}

void Platform::setCursorPosition(int x, int y, InputState *input)
{
	SDL_WarpMouseInWindow(m_window, x, y);

	float spx = 0.0f, spy = 0.0f;
	SDL_GetGlobalMouseState(&spx, &spy);

	if (input)
	{
		input->onMouseScreenMove(spx, spy);
		input->onMouseMove(x, y);
	}
}

WindowMode Platform::getWindowMode() const
{
	auto flags = SDL_GetWindowFlags(m_window);

	if (flags & SDL_WINDOW_FULLSCREEN)
	{
		if (flags & SDL_WINDOW_BORDERLESS)
		{
			return WINDOW_MODE_BORDERLESS_FULLSCREEN;
		}
		else
		{
			return WINDOW_MODE_FULLSCREEN;
		}
	}
	else if (flags & SDL_WINDOW_BORDERLESS)
	{
		return WINDOW_MODE_BORDERLESS;
	}
	else
	{
		return WINDOW_MODE_WINDOWED;
	}
}

void Platform::setWindowMode(WindowMode toggle)
{
	switch (toggle)
	{
		case WINDOW_MODE_FULLSCREEN:
			SDL_SetWindowFullscreen(m_window, true);
			SDL_SetWindowBordered(m_window, false);
			break;

		case WINDOW_MODE_BORDERLESS:
			SDL_SetWindowFullscreen(m_window, false);
			SDL_SetWindowBordered(m_window, false);
			break;

		case WINDOW_MODE_WINDOWED:
			SDL_SetWindowFullscreen(m_window, false);
			SDL_SetWindowBordered(m_window, true);
			break;

		case WINDOW_MODE_BORDERLESS_FULLSCREEN:
			SDL_SetWindowFullscreen(m_window, true);
			SDL_SetWindowBordered(m_window, false);
			break;

		default:
			return;
	}
}

void Platform::sleepFor(uint64_t ms) const
{
	if (ms > 0)
		SDL_Delay(ms);
}

uint64_t Platform::getTicks() const
{
	return SDL_GetTicks();
}

uint64_t Platform::getPerformanceCounter() const
{
	return SDL_GetPerformanceCounter();
}

uint64_t Platform::getPerformanceFrequency() const
{
	return SDL_GetPerformanceFrequency();
}

void *Platform::streamFromFile(const char *filepath, const char *mode) const
{
	return SDL_IOFromFile(filepath, mode);
}

void *Platform::streamFromMemory(void *memory, uint64_t size) const
{
	return SDL_IOFromMem(memory, size);
}

void *Platform::streamFromConstMemory(const void *memory, uint64_t size) const
{
	return SDL_IOFromConstMem(memory, size);
}

int64_t Platform::streamRead(void *stream, void *dst, uint64_t size) const
{
	return SDL_ReadIO((SDL_IOStream *)stream, dst, size);
}

int64_t Platform::streamWrite(void *stream, const void *src, uint64_t size) const
{
	return SDL_WriteIO((SDL_IOStream *)stream, src, size);
}

int64_t Platform::streamSeek(void *stream, int64_t offset) const
{
	return SDL_SeekIO((SDL_IOStream *)stream, offset, SDL_IO_SEEK_SET);
}

int64_t Platform::streamSize(void *stream) const
{
	return SDL_GetIOSize((SDL_IOStream *)stream);
}

int64_t Platform::streamPosition(void *stream) const
{
	return SDL_TellIO((SDL_IOStream *)stream);
}

void Platform::streamClose(void *stream) const
{
	SDL_CloseIO((SDL_IOStream *)stream);
}

void Platform::initImGui()
{
	ImGui_ImplSDL3_InitForVulkan(m_window);
}

const char *const *Platform::vkGetInstanceExtensions(uint32_t *count) const
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

bool Platform::vkCreateSurface(VkInstance instance, VkSurfaceKHR *surface) const
{
	return SDL_Vulkan_CreateSurface(m_window, instance, NULL, surface);
}
