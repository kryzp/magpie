#include "input.h"

using namespace mgp;

InputState::InputState()
	: m_currentState()
	, m_nextState()
	, m_prevState()
{
	mgp_LOG("Input Initialized!");
}

InputState::~InputState()
{
	mgp_LOG("Input Destroyed!");
}

void InputState::update()
{
	// "shuffle" our input state forward
	m_prevState = m_currentState;
	m_currentState = m_nextState;
}

bool InputState::isDown(const KeyboardKey &k) const
{
	return m_currentState.keyboard.down[k];
}

bool InputState::isPressed(const KeyboardKey &k) const
{
	return m_currentState.keyboard.down[k] && !m_prevState.keyboard.down[k];
}

bool InputState::isReleased(const KeyboardKey &k) const
{
	return !m_currentState.keyboard.down[k] && m_prevState.keyboard.down[k];
}

bool InputState::isDown(const MouseButton &mb) const
{
	return m_currentState.mouse.down[mb];
}

bool InputState::isPressed(const MouseButton &mb) const
{
	return m_currentState.mouse.down[mb] && !m_prevState.mouse.down[mb];
}

bool InputState::isReleased(const MouseButton &mb) const
{
	return !m_currentState.mouse.down[mb] && m_prevState.mouse.down[mb];
}

bool InputState::isDown(const GamepadButton &gpb, uint32_t id) const
{
	return m_currentState.gamepads[id].down[gpb];
}

bool InputState::isPressed(const GamepadButton &gpb, uint32_t id) const
{
	return m_currentState.gamepads[id].down[gpb] && !m_prevState.gamepads[id].down[gpb];
}

bool InputState::isReleased(const GamepadButton &gpb, uint32_t id) const
{
	return !m_currentState.gamepads[id].down[gpb] && m_prevState.gamepads[id].down[gpb];
}

glm::vec2 InputState::getMousePosition() const
{
	return m_currentState.mouse.position;
}

glm::vec2 InputState::getMouseScreenPosition() const
{
	return m_currentState.mouse.screenPosition;
}

glm::vec2 InputState::getMouseWheel() const
{
	return m_currentState.mouse.wheel;
}

bool InputState::shift() const
{
	return isDown(KB_KEY_LEFT_SHIFT) || isDown(KB_KEY_RIGHT_SHIFT);
}

bool InputState::ctrl() const
{
	return isDown(KB_KEY_LEFT_CONTROL) || isDown(KB_KEY_RIGHT_CONTROL);
}

bool InputState::alt() const
{
	return isDown(KB_KEY_LEFT_ALT) || isDown(KB_KEY_RIGHT_ALT);
}

const char *InputState::text() const
{
	return m_currentState.keyboard.text;
}

glm::vec2 InputState::getLeftStick(uint32_t id) const
{
	return m_currentState.gamepads[id].leftStick;
}

glm::vec2 InputState::getRightStick(uint32_t id) const
{
	return m_currentState.gamepads[id].rightStick;
}

float InputState::getLeftTrigger(uint32_t id) const
{
	return m_currentState.gamepads[id].leftTrigger;
}

float InputState::getRightTrigger(uint32_t id) const
{
	return m_currentState.gamepads[id].rightTrigger;
}

void InputState::onMouseWheel(float x, float y)
{
	m_nextState.mouse.wheel = glm::vec2(x, y);
}

void InputState::onMouseScreenMove(float x, float y)
{
	m_nextState.mouse.screenPosition = glm::vec2(x, y);
}

void InputState::onMouseMove(float x, float y)
{
	m_nextState.mouse.position = glm::vec2(x, y);
}

void InputState::onMouseDown(uint64_t btn)
{
	m_nextState.mouse.down[btn] = true;
}

void InputState::onMouseUp(uint64_t btn)
{
	m_nextState.mouse.down[btn] = false;
}

void InputState::onKeyDown(uint64_t btn)
{
	m_nextState.keyboard.down[btn] = true;
}

void InputState::onKeyUp(uint64_t btn)
{
	m_nextState.keyboard.down[btn] = false;
}

void InputState::onTextUtf8(const char *text)
{
	cstr::concat(m_nextState.keyboard.text, text, MAX_TEXT_INPUT);
}

void InputState::onGamepadButtonDown(uint64_t btn, int id)
{
	if (id < 0)
		return;

	m_nextState.gamepads[id].down[btn] = true;
}

void InputState::onGamepadButtonUp(uint64_t btn, int id)
{
	if (id < 0)
		return;

	m_nextState.gamepads[id].down[btn] = false;
}

void InputState::onGamepadMotion(int id, GamepadAxis axis, float value)
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
