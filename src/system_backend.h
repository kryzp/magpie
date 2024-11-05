#ifndef SDL3_BACKEND_H_
#define SDL3_BACKEND_H_

#include <SDL3/SDL.h>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.h>

#include "container/string.h"
#include "container/array.h"

#include "input/input.h"

#include "app.h"

namespace llt
{
	class SystemBackend
	{
	public:
		SystemBackend(const Config& config);
		~SystemBackend();

		void pollEvents();

		String getWindowName() const;
		void setWindowName(const String& name);

		glm::ivec2 getWindowPosition() const;
		void setWindowPosition(const glm::ivec2& position);

		glm::ivec2 getWindowSize() const;
		void setWindowSize(const glm::ivec2& size);

		glm::ivec2 getScreenSize() const;

		float getWindowOpacity() const;
		void setWindowOpacity(float opacity) const;

		bool isWindowResizable() const;
		void toggleWindowResizable(bool toggle) const;

		float getWindowRefreshRate() const;
		float getWindowPixelDensity() const;

		bool isCursorVisible() const;
		void toggleCursorVisible(bool toggle) const;
		void lockCursor(bool toggle) const;
		void setCursorPosition(int x, int y);

		void reconnectAllGamepads();
		GamepadType getGamepadType(int player);
		void closeGamepad(int player) const;

		WindowMode getWindowMode() const;
		void setWindowMode(WindowMode toggle);

		void sleepFor(uint64_t ms) const;

		uint64_t getTicks() const;
		uint64_t getPerformanceCounter() const;
		uint64_t getPerformanceFrequency() const;

		void* streamFromFile(const char* filepath, const char* mode);
		void* streamFromMemory(void* memory, uint64_t size);
		void* streamFromConstMemory(const void* memory, uint64_t size);
		int64_t streamRead(void* stream, void* ptr, uint64_t size);
		int64_t streamWrite(void* stream, const void* ptr, uint64_t size);
		int64_t streamSeek(void* stream, int64_t offset);
		int64_t streamSize(void* stream);
		int64_t streamPosition(void* stream);
		void streamClose(void* stream);

		const char* const* vkGetInstanceExtensions(uint32_t* count);
		bool vkCreateSurface(VkInstance instance, VkSurfaceKHR* surface);

	private:
		void closeAllGamepads();

		SDL_Window* m_window;

		Array<SDL_Gamepad*, MAX_GAMEPADS> m_gamepads;
		int m_gamepadCount;
	};

	extern SystemBackend* g_systemBackend;
}

#endif // SDL3_BACKEND_H_
