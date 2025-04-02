#pragma once

#include <array>
#include <functional>

#include <SDL3/SDL.h>
#include <glm/vec2.hpp>

#include "third_party/volk.h"

#include "input/input.h"

#include "app.h"

namespace mgp
{
	class Platform
	{
	public:
		Platform(const Config &config);
		~Platform();

		void pollEvents(InputState *input);

		std::string getWindowName() const;
		void setWindowName(const std::string &name);

		glm::ivec2 getWindowPosition() const;
		void setWindowPosition(const glm::ivec2& position);

		glm::ivec2 getWindowSize() const;
		void setWindowSize(const glm::ivec2& size);

		glm::ivec2 getWindowSizeInPixels() const;

		glm::ivec2 getScreenSize() const;

		float getWindowOpacity() const;
		void setWindowOpacity(float opacity) const;

		bool isWindowResizable() const;
		void toggleWindowResizable(bool toggle) const;

		float getWindowRefreshRate() const;
		float getWindowPixelDensity() const;

		bool isCursorVisible() const;
		void setCursorVisible(bool toggle) const;
		void lockCursor(bool toggle) const;
		void setCursorPosition(int x, int y, InputState *input);

		void reconnectAllGamepads();
		GamepadType getGamepadType(int player);
		void closeGamepad(int player) const;

		WindowMode getWindowMode() const;
		void setWindowMode(WindowMode toggle);

		void sleepFor(uint64_t ms) const;

		uint64_t getTicks() const;
		uint64_t getPerformanceCounter() const;
		uint64_t getPerformanceFrequency() const;

		void *streamFromFile(const char *filepath, const char *mode) const;
		void *streamFromMemory(void *memory, uint64_t size) const;
		void *streamFromConstMemory(const void *memory, uint64_t size) const;

		int64_t streamRead(void *stream, void *dst, uint64_t size) const;
		int64_t streamWrite(void *stream, const void *src, uint64_t size) const;
		int64_t streamSeek(void *stream, int64_t offset) const;
		int64_t streamSize(void *stream) const;
		int64_t streamPosition(void *stream) const;
		void streamClose(void *stream) const;

		// os-generic file open, close, write, read, etc... functions
		// (streamX functions only work on windows :( )

		const char *const *vkGetInstanceExtensions(uint32_t *count) const;
		bool vkCreateSurface(VkInstance instance, VkSurfaceKHR *surface) const;

		void initImGui();

		std::function<void(void)> onExit;
		std::function<void(int, int)> onWindowResize;

	private:
		void closeAllGamepads();

		SDL_Window *m_window;

		std::array<SDL_Gamepad*, MAX_GAMEPADS> m_gamepads;
		int m_gamepadCount;
	};
}
