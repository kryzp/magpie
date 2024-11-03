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

		String getWindowName();
		void setWindowName(const String& name);

		glm::ivec2 getWindowPosition();
		void setWindowPosition(const glm::ivec2& position);

		glm::ivec2 getWindowSize();
		void setWindowSize(const glm::ivec2& size);

		glm::ivec2 getScreenSize();

		float getWindowOpacity();
		void setWindowOpacity(float opacity);

		bool isWindowResizable();
		void toggleWindowResizable(bool toggle);

		float getWindowRefreshRate();
		float getWindowPixelDensity();

		bool isCursorVisible();
		void toggleCursorVisible(bool toggle);
		void lockCursor(bool toggle);
		void setCursorPosition(int x, int y);

		void reconnectAllGamepads();
		GamepadType getGamepadType(int player);
		void closeGamepad(int player);

		WindowMode getWindowMode();
		void setWindowMode(WindowMode toggle);

		void sleepFor(uint64_t ms) ;
		uint64_t getTicks();

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
