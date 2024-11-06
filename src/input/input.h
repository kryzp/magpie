#ifndef INPUT_MGR_H_
#define INPUT_MGR_H_

#include <glm/vec2.hpp>

#include "../common.h"

#include "keyboard.h"
#include "mouse.h"
#include "gamepad.h"
#include "v_key.h"

namespace llt
{
	/**
	 * Manages input in the program.
	 */
	class Input
	{
		struct KeyboardState
		{
			bool down[KB_KEY_MAX_ENUM];
			char text[MAX_TEXT_INPUT];
		};

		struct MouseState
		{
			bool down[MBTN_MAX_ENUM];
			glm::vec2 screenPosition;
			glm::vec2 drawPosition;
			glm::vec2 position;
			glm::vec2 wheel;
		};

		struct GamepadState
		{
			bool down[GAMEPAD_BTN_MAX_ENUM];
			glm::vec2 leftStick;
			glm::vec2 rightStick;
			float leftTrigger;
			float rightTrigger;
		};

		struct InputState
		{
			KeyboardState keyboard;
			MouseState mouse;
			GamepadState gamepads[MAX_GAMEPADS];
		};

	public:
		Input();
		~Input();

		void update();

		bool isDown(const VirtualKey& k) const;
		bool isPressed(const VirtualKey& k) const;
		bool isReleased(const VirtualKey& k) const;

		bool isDown(const KeyboardKey& k) const;
		bool isPressed(const KeyboardKey& k) const;
		bool isReleased(const KeyboardKey& k) const;

		bool isDown(const MouseButton& mb) const;
		bool isPressed(const MouseButton& mb) const;
		bool isReleased(const MouseButton& mb) const;

		bool isDown(const GamepadButton& gpb, uint32_t id) const;
		bool isPressed(const GamepadButton& gpb, uint32_t id) const;
		bool isReleased(const GamepadButton& gpb, uint32_t id) const;

		glm::vec2 getMousePosition() const;
		glm::vec2 getMouseScreenPosition() const;
		glm::vec2 getMouseDrawPosition() const;
		glm::vec2 getMouseWheel() const;

		bool shift() const;
		bool ctrl() const;
		bool alt() const;

		const char* text() const;

		glm::vec2 getLeftStick(uint32_t id) const;
		glm::vec2 getRightStick(uint32_t id) const;

		float getLeftTrigger(uint32_t id) const;
		float getRightTrigger(uint32_t id) const;

		void onMouseWheel(float x, float y);
		void onMouseScreenMove(float x, float y);
		void onMouseMove(float x, float y);

		void onMouseDown(uint64_t btn);
		void onMouseUp(uint64_t btn);

		void onKeyDown(uint64_t btn);
		void onKeyUp(uint64_t btn);

		void onTextUtf8(const char* text);

		void onGamepadButtonDown(uint64_t btn, int id);
		void onGamepadButtonUp(uint64_t btn, int id);
		void onGamepadMotion(int id, GamepadAxis axis, float value);

	private:
		InputState m_currentState;
		InputState m_nextState;
		InputState m_prevState;
	};

	extern Input* g_inputState;
}

#endif // INPUT_MGR_H_
