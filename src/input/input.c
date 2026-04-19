
internal void
I_GamepadStSetAxisValue(I_GamepadState *st, I_GamepadAxis axis, f32 value)
{
	switch (axis)
	{
		case I_GamepadAxis_LeftX:
			st->left_stick.x = value;
			break;

		case I_GamepadAxis_LeftY:
			st->left_stick.y = value;
			break;

		case I_GamepadAxis_RightX:
			st->right_stick.x = value;
			break;

		case I_GamepadAxis_RightY:
			st->right_stick.y = value;
			break;

		case I_GamepadAxis_TriggerLeft:
			st->left_trigger = value;
			break;

		case I_GamepadAxis_TriggerRight:
			st->right_trigger = value;
			break;
	}
}

internal b32
I_KbDown(const I_State *st, I_KeyboardKey k)
{
	return st->kb_down[k];
}

internal b32
I_KbPressed(const I_State *st, I_KeyboardKey k)
{
	return st->kb_pressed[k];
}

internal b32
I_KbReleased(const I_State *st, I_KeyboardKey k)
{
	return st->kb_released[k];
}

internal b32
I_MbDown(const I_State *st, I_MouseButton b)
{
	return st->mb_down[b];
}

internal b32
I_MbPressed(const I_State *st, I_MouseButton b)
{
	return st->mb_pressed[b];
}

internal b32
I_MbReleased(const I_State *st, I_MouseButton b)
{
	return st->mb_released[b];
}

internal b32
I_GpDown(const I_State *st, I_GamepadButton b, u32 player_index)
{
	return st->gamepads[player_index].down[b];
}

internal b32
I_GpPressed(const I_State *st, I_GamepadButton b, u32 player_index)
{
	return st->gamepads[player_index].pressed[b];
}

internal b32
I_GpReleased(const I_State *st, I_GamepadButton b, u32 player_index)
{
	return st->gamepads[player_index].released[b];
}

internal b32
I_Shift(const I_State *st)
{
	return I_KbDown(st, I_KeyboardKey_LeftShift) || I_KbDown(st, I_KeyboardKey_RightShift);
}

internal b32
I_Ctrl(const I_State *st)
{
	return I_KbDown(st, I_KeyboardKey_LeftControl) || I_KbDown(st, I_KeyboardKey_RightControl);
}

internal b32
I_Alt(const I_State *st)
{
	return I_KbDown(st, I_KeyboardKey_LeftAlt) || I_KbDown(st, I_KeyboardKey_RightAlt);
}
