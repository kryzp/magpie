#pragma once

#include <glm/vec2.hpp>

#include "core/common.h"

#include "keyboard.h"
#include "mouse.h"
#include "gamepad.h"

namespace mgp
{
	class InputState
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

		struct InputData
		{
			KeyboardState keyboard;
			MouseState mouse;
			GamepadState gamepads[MAX_GAMEPADS];
		};

	public:
		InputState();
		~InputState();

		void update();

		bool isDown(const KeyboardKey &k) const;
		bool isPressed(const KeyboardKey &k) const;
		bool isReleased(const KeyboardKey &k) const;

		bool isDown(const MouseButton &mb) const;
		bool isPressed(const MouseButton &mb) const;
		bool isReleased(const MouseButton &mb) const;

		bool isDown(const GamepadButton &gpb, uint32_t id) const;
		bool isPressed(const GamepadButton &gpb, uint32_t id) const;
		bool isReleased(const GamepadButton &gpb, uint32_t id) const;

		glm::vec2 getMousePosition() const;
		glm::vec2 getMouseScreenPosition() const;
		glm::vec2 getMouseWheel() const;

		bool shift() const;
		bool ctrl() const;
		bool alt() const;

		const char *text() const;

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

		void onTextUtf8(const char *text);

		void onGamepadButtonDown(uint64_t btn, int id);
		void onGamepadButtonUp(uint64_t btn, int id);
		void onGamepadMotion(int id, GamepadAxis axis, float value);

	private:
		InputData m_currentState;
		InputData m_nextState;
		InputData m_prevState;
	};
}
