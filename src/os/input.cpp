#include "os/input.h"

void os::GamepadState::SetAxisValue(GamepadAxis axis, float value)
{
	switch (axis) {
		case GamepadAxis_LeftX:
			left_stick.x = value;
			break;

		case GamepadAxis_LeftY:
			left_stick.y = value;
			break;

		case GamepadAxis_RightX:
			right_stick.x = value;
			break;

		case GamepadAxis_RightY:
			right_stick.y = value;
			break;

		case GamepadAxis_TriggerLeft:
			left_trigger = value;
			break;

		case GamepadAxis_TriggerRight:
			right_trigger = value;
			break;

		default:
			break;
	}
}

bool os::InputState::KbDown(KeyboardKey k) const
{
	return kb_down[k];
}

bool os::InputState::KbPressed(KeyboardKey k) const
{
	return kb_pressed[k];
}

bool os::InputState::KbReleased(KeyboardKey k) const
{
	return kb_released[k];
}

bool os::InputState::MbDown(MouseButton b) const
{
	return mb_down[b];
}

bool os::InputState::MbPressed(MouseButton b) const
{
	return mb_pressed[b];
}

bool os::InputState::MbReleased(MouseButton b) const
{
	return mb_released[b];
}

bool os::InputState::GpDown(GamepadButton b, unsigned player_index) const
{
	return gamepads[player_index].down[b];
}

bool os::InputState::GpPressed(GamepadButton b, unsigned player_index) const
{
	return gamepads[player_index].pressed[b];
}

bool os::InputState::GpReleased(GamepadButton b, unsigned player_index) const
{
	return gamepads[player_index].released[b];
}

bool os::InputState::KbShift() const
{
	return KbDown(KeyboardKey_LeftShift) || KbDown(KeyboardKey_RightShift);
}

bool os::InputState::KbCtrl() const
{
	return KbDown(KeyboardKey_LeftControl) || KbDown(KeyboardKey_RightControl);
}

bool os::InputState::KbAlt() const
{
	return KbDown(KeyboardKey_LeftAlt) || KbDown(KeyboardKey_RightAlt);
}
