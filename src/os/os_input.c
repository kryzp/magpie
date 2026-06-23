
static void OS_GamepadStateSetAxisValue(OS_GamepadState *st, OS_GamepadAxis axis, f32 value)
{
	switch (axis)
	{
		case OS_GamepadAxis_LeftX:
			st->left_stick.x = value;
			break;

		case OS_GamepadAxis_LeftY:
			st->left_stick.y = value;
			break;

		case OS_GamepadAxis_RightX:
			st->right_stick.x = value;
			break;

		case OS_GamepadAxis_RightY:
			st->right_stick.y = value;
			break;

		case OS_GamepadAxis_TriggerLeft:
			st->left_trigger = value;
			break;

		case OS_GamepadAxis_TriggerRight:
			st->right_trigger = value;
			break;
	}
}

static b32 OS_KbDown(const OS_InputState *st, OS_KeyboardKey k)
{
	return st->kb_down[k];
}

static b32 OS_KbPressed(const OS_InputState *st, OS_KeyboardKey k)
{
	return st->kb_pressed[k];
}

static b32 OS_KbReleased(const OS_InputState *st, OS_KeyboardKey k)
{
	return st->kb_released[k];
}

static b32 OS_MbDown(const OS_InputState *st, OS_MouseButton b)
{
	return st->mb_down[b];
}

static b32 OS_MbPressed(const OS_InputState *st, OS_MouseButton b)
{
	return st->mb_pressed[b];
}

static b32 OS_MbReleased(const OS_InputState *st, OS_MouseButton b)
{
	return st->mb_released[b];
}

static b32 OS_GpDown(const OS_InputState *st, OS_GamepadButton b, u32 player_index)
{
	return st->gamepads[player_index].down[b];
}

static b32 OS_GpPressed(const OS_InputState *st, OS_GamepadButton b, u32 player_index)
{
	return st->gamepads[player_index].pressed[b];
}

static b32 OS_GpReleased(const OS_InputState *st, OS_GamepadButton b, u32 player_index)
{
	return st->gamepads[player_index].released[b];
}

static b32 OS_KbShift(const OS_InputState *st)
{
	return OS_KbDown(st, OS_KeyboardKey_LeftShift) || OS_KbDown(st, OS_KeyboardKey_RightShift);
}

static b32 OS_KbCtrl(const OS_InputState *st)
{
	return OS_KbDown(st, OS_KeyboardKey_LeftControl) || OS_KbDown(st, OS_KeyboardKey_RightControl);
}

static b32 OS_KbAlt(const OS_InputState *st)
{
	return OS_KbDown(st, OS_KeyboardKey_LeftAlt) || OS_KbDown(st, OS_KeyboardKey_RightAlt);
}
