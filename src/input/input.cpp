#include "input.h"

llt::Input* llt::g_inputState = nullptr;

using namespace llt;

Input::Input()
	: m_currentState()
	, m_nextState()
	, m_prevState()
{
	LLT_LOG("[INPUT] Initialized!");
}

Input::~Input()
{
	LLT_LOG("[INPUT] Destroyed!");
}

void Input::update()
{
	// "shuffle" our input state forward
	m_prevState = m_currentState;
	m_currentState = m_nextState;
}

bool Input::isDown(const VirtualKey& k) const
{
	for (auto& kb : k.keyboardKeys) {
		if (isDown(kb)) {
			return true;
		}
	}

	for (auto& mb : k.mouseButtons) {
		if (isDown(mb)) {
			return true;
		}
	}

	for (int i = 0; i < MAX_GAMEPADS; i++) {
		for (auto& jsb : k.gamepadButtons) {
			if (isDown(jsb, i)) {
				return true;
			}
		}
	}

	return false;
}

bool Input::isPressed(const VirtualKey& k) const
{
	for (auto& kb : k.keyboardKeys) {
		if (isPressed(kb)) {
			return true;
		}
	}

	for (auto& mb : k.mouseButtons) {
		if (isPressed(mb)) {
			return true;
		}
	}

	for (int i = 0; i < MAX_GAMEPADS; i++) {
		for (auto& jsb : k.gamepadButtons) {
			if (isPressed(jsb, i)) {
				return true;
			}
		}
	}

	return false;
}

bool Input::isReleased(const VirtualKey& k) const
{
	for (auto& kb : k.keyboardKeys) {
		if (isReleased(kb)) {
			return true;
		}
	}

	for (auto& mb : k.mouseButtons) {
		if (isReleased(mb)) {
			return true;
		}
	}

	for (uint32_t i = 0; i < MAX_GAMEPADS; i++) {
		for (auto& jsb : k.gamepadButtons) {
			if (isReleased(jsb, i)) {
				return true;
			}
		}
	}

	return false;
}

bool Input::isDown(const KeyboardKey& k) const
{
	return m_currentState.keyboard.down[k];
}

bool Input::isPressed(const KeyboardKey& k) const
{
	return m_currentState.keyboard.down[k] && !m_prevState.keyboard.down[k];
}

bool Input::isReleased(const KeyboardKey& k) const
{
	return !m_currentState.keyboard.down[k] && m_prevState.keyboard.down[k];
}

bool Input::isDown(const MouseButton& mb) const
{
	return m_currentState.mouse.down[mb];
}

bool Input::isPressed(const MouseButton& mb) const
{
	return m_currentState.mouse.down[mb] && !m_prevState.mouse.down[mb];
}

bool Input::isReleased(const MouseButton& mb) const
{
	return !m_currentState.mouse.down[mb] && m_prevState.mouse.down[mb];
}

bool Input::isDown(const GamepadButton& gpb, uint32_t id) const
{
	return m_currentState.gamepads[id].down[gpb];
}

bool Input::isPressed(const GamepadButton& gpb, uint32_t id) const
{
	return m_currentState.gamepads[id].down[gpb] && !m_prevState.gamepads[id].down[gpb];
}

bool Input::isReleased(const GamepadButton& gpb, uint32_t id) const
{
	return !m_currentState.gamepads[id].down[gpb] && m_prevState.gamepads[id].down[gpb];
}

glm::vec2 Input::getMousePosition() const
{
	return m_currentState.mouse.position;
}

glm::vec2 Input::getMouseScreenPosition() const
{
	return m_currentState.mouse.screenPosition;
}

glm::vec2 Input::getMouseDrawPosition() const
{
	return m_currentState.mouse.drawPosition;
}

glm::vec2 Input::getMouseWheel() const
{
	return m_currentState.mouse.wheel;
}

bool Input::shift() const
{
	return isDown(KEY_LEFT_SHIFT) || isDown(KEY_RIGHT_SHIFT);
}

bool Input::ctrl() const
{
	return isDown(KEY_LEFT_CONTROL) || isDown(KEY_RIGHT_CONTROL);
}

bool Input::alt() const
{
	return isDown(KEY_LEFT_ALT) || isDown(KEY_RIGHT_ALT);
}

const char* Input::text() const
{
	return m_currentState.keyboard.text;
}

glm::vec2 Input::getLeftStick(uint32_t id) const
{
	return m_currentState.gamepads[id].leftStick;
}

glm::vec2 Input::getRightStick(uint32_t id) const
{
	return m_currentState.gamepads[id].rightStick;
}

float Input::getLeftTrigger(uint32_t id) const
{
	return m_currentState.gamepads[id].leftTrigger;
}

float Input::getRightTrigger(uint32_t id) const
{
	return m_currentState.gamepads[id].rightTrigger;
}

void Input::onMouseWheel(float x, float y)
{
	m_nextState.mouse.wheel = glm::vec2(x, y);
}

void Input::onMouseScreenMove(float x, float y)
{
	m_nextState.mouse.screenPosition = glm::vec2(x, y);
}

void Input::onMouseMove(float x, float y)
{
	m_nextState.mouse.position = glm::vec2(x, y);
}

void Input::onMouseDown(uint64_t btn)
{
	m_nextState.mouse.down[btn] = true;
}

void Input::onMouseUp(uint64_t btn)
{
	m_nextState.mouse.down[btn] = false;
}

void Input::onKeyDown(uint64_t btn)
{
	m_nextState.keyboard.down[btn] = true;
}

void Input::onKeyUp(uint64_t btn)
{
	m_nextState.keyboard.down[btn] = false;
}

void Input::onTextUtf8(const char* text)
{
	cstr::concat(m_nextState.keyboard.text, text, MAX_TEXT_INPUT);
}

void Input::onGamepadButtonDown(uint64_t btn, int id)
{
	if (id < 0)
		return;

	m_nextState.gamepads[id].down[btn] = true;
}

void Input::onGamepadButtonUp(uint64_t btn, int id)
{
	if (id < 0)
		return;

	m_nextState.gamepads[id].down[btn] = false;
}

void Input::onGamepadMotion(int id, GamepadAxis axis, float value)
{
	if (id < 0)
		return;

	switch (axis)
	{
		case GAMEPAD_AXIS_LEFT_X:
			m_nextState.gamepads[id].leftStick.x = value;
			break;

		case GAMEPAD_AXIS_LEFT_Y:
			m_nextState.gamepads[id].leftStick.y = value;
			break;

		case GAMEPAD_AXIS_RIGHT_X:
			m_nextState.gamepads[id].rightStick.x = value;
			break;

		case GAMEPAD_AXIS_RIGHT_Y:
			m_nextState.gamepads[id].rightStick.y = value;
			break;

		case GAMEPAD_AXIS_TRIGGER_LEFT:
			m_nextState.gamepads[id].leftTrigger = value;
			break;

		case GAMEPAD_AXIS_TRIGGER_RIGHT:
			m_nextState.gamepads[id].rightTrigger = value;
			break;

		default:
			break;
	}
}
