#ifndef V_KEY_H_
#define V_KEY_H_

#include "container/vector.h"

#include "keyboard.h"
#include "mouse.h"
#include "gamepad.h"

namespace llt
{
	/**
	 * Represents a "virtual key", a collection of keyboard, mouse and joystick keys
	 * that all can be treated as a single key so it can all be queried at once.
	 */
	class VirtualKey
	{
	public:
		VirtualKey() = default;
		~VirtualKey() = default;

		bool isDown() const;
		bool isPressed() const;
		bool isReleased() const;

		Vector<KeyboardKey> keyboardKeys;
		Vector<MouseButton> mouseButtons;
		Vector<GamepadButton> gamepadButtons;
	};
}

#endif // V_KEY_H_
