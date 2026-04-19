#ifndef I_INPUT_H
#define I_INPUT_H

typedef enum I_KeyboardKey
{
	I_KeyboardKey_Unknown = 0,
	I_KeyboardKey_A = 4,
	I_KeyboardKey_B = 5,
	I_KeyboardKey_C = 6,
	I_KeyboardKey_D = 7,
	I_KeyboardKey_E = 8,
	I_KeyboardKey_F = 9,
	I_KeyboardKey_G = 10,
	I_KeyboardKey_H = 11,
	I_KeyboardKey_I = 12,
	I_KeyboardKey_J = 13,
	I_KeyboardKey_K = 14,
	I_KeyboardKey_L = 15,
	I_KeyboardKey_M = 16,
	I_KeyboardKey_N = 17,
	I_KeyboardKey_O = 18,
	I_KeyboardKey_P = 19,
	I_KeyboardKey_Q = 20,
	I_KeyboardKey_R = 21,
	I_KeyboardKey_S = 22,
	I_KeyboardKey_T = 23,
	I_KeyboardKey_U = 24,
	I_KeyboardKey_V = 25,
	I_KeyboardKey_W = 26,
	I_KeyboardKey_X = 27,
	I_KeyboardKey_Y = 28,
	I_KeyboardKey_Z = 29,
	I_KeyboardKey_D1 = 30,
	I_KeyboardKey_D2 = 31,
	I_KeyboardKey_D3 = 32,
	I_KeyboardKey_D4 = 33,
	I_KeyboardKey_D5 = 34,
	I_KeyboardKey_D6 = 35,
	I_KeyboardKey_D7 = 36,
	I_KeyboardKey_D8 = 37,
	I_KeyboardKey_D9 = 38,
	I_KeyboardKey_D0 = 39,
	I_KeyboardKey_Enter = 40,
	I_KeyboardKey_Escape = 41,
	I_KeyboardKey_Backspace = 42,
	I_KeyboardKey_Tab = 43,
	I_KeyboardKey_Space = 44,
	I_KeyboardKey_Minus = 45,
	I_KeyboardKey_Equals = 46,
	I_KeyboardKey_LeftBracket = 47,
	I_KeyboardKey_RightBracket = 48,
	I_KeyboardKey_Backslash = 49,
	I_KeyboardKey_NonUSHash = 50,
	I_KeyboardKey_Semicolon = 51,
	I_KeyboardKey_Apostrophe = 52,
	I_KeyboardKey_Grave = 53,
	I_KeyboardKey_Comma = 54,
	I_KeyboardKey_Period = 55,
	I_KeyboardKey_Slash = 56,
	I_KeyboardKey_Capslock = 57,
	I_KeyboardKey_F1 = 58,
	I_KeyboardKey_F2 = 59,
	I_KeyboardKey_F3 = 60,
	I_KeyboardKey_F4 = 61,
	I_KeyboardKey_F5 = 62,
	I_KeyboardKey_F6 = 63,
	I_KeyboardKey_F7 = 64,
	I_KeyboardKey_F8 = 65,
	I_KeyboardKey_F9 = 66,
	I_KeyboardKey_F10 = 67,
	I_KeyboardKey_F11 = 68,
	I_KeyboardKey_F12 = 69,
	I_KeyboardKey_PrintScreen = 70,
	I_KeyboardKey_ScrollLock = 71,
	I_KeyboardKey_Pause = 72,
	I_KeyboardKey_Insert = 73,
	I_KeyboardKey_Home = 74,
	I_KeyboardKey_PageUp = 75,
	I_KeyboardKey_Delete = 76,
	I_KeyboardKey_End = 77,
	I_KeyboardKey_PageDown = 78,
	I_KeyboardKey_Right = 79,
	I_KeyboardKey_Left = 80,
	I_KeyboardKey_Down = 81,
	I_KeyboardKey_Up = 82,
	I_KeyboardKey_Numlock_clear = 83,
	I_KeyboardKey_KpDivide = 84,
	I_KeyboardKey_KpMultiply = 85,
	I_KeyboardKey_KpMinus = 86,
	I_KeyboardKey_KpPlus = 87,
	I_KeyboardKey_KpEnter = 88,
	I_KeyboardKey_Kp1 = 89,
	I_KeyboardKey_Kp2 = 90,
	I_KeyboardKey_Kp3 = 91,
	I_KeyboardKey_Kp4 = 92,
	I_KeyboardKey_Kp5 = 93,
	I_KeyboardKey_Kp6 = 94,
	I_KeyboardKey_Kp7 = 95,
	I_KeyboardKey_Kp8 = 96,
	I_KeyboardKey_Kp9 = 97,
	I_KeyboardKey_Kp0 = 98,
	I_KeyboardKey_KpPeriod = 99,
	I_KeyboardKey_NonUSBackslash = 100,
	I_KeyboardKey_Application = 101,
	I_KeyboardKey_Power = 102,
	I_KeyboardKey_KpEquals = 103,
	I_KeyboardKey_F13 = 104,
	I_KeyboardKey_F14 = 105,
	I_KeyboardKey_F15 = 106,
	I_KeyboardKey_F16 = 107,
	I_KeyboardKey_F17 = 108,
	I_KeyboardKey_F18 = 109,
	I_KeyboardKey_F19 = 110,
	I_KeyboardKey_F20 = 111,
	I_KeyboardKey_F21 = 112,
	I_KeyboardKey_F22 = 113,
	I_KeyboardKey_F23 = 114,
	I_KeyboardKey_F24 = 115,
	I_KeyboardKey_Execute = 116,
	I_KeyboardKey_Help = 117,
	I_KeyboardKey_Menu = 118,
	I_KeyboardKey_Select = 119,
	I_KeyboardKey_Stop = 120,
	I_KeyboardKey_Again = 121,
	I_KeyboardKey_Undo = 122,
	I_KeyboardKey_Cut = 123,
	I_KeyboardKey_Copy = 124,
	I_KeyboardKey_Paste = 125,
	I_KeyboardKey_Find = 126,
	I_KeyboardKey_Mute = 127,
	I_KeyboardKey_Volume_up = 128,
	I_KeyboardKey_Volume_down = 129,
	I_KeyboardKey_KpComma = 133,
	I_KeyboardKey_KpEqualsAs400 = 134,
	I_KeyboardKey_International1 = 135,
	I_KeyboardKey_International2 = 136,
	I_KeyboardKey_International3 = 137,
	I_KeyboardKey_International4 = 138,
	I_KeyboardKey_International5 = 139,
	I_KeyboardKey_International6 = 140,
	I_KeyboardKey_International7 = 141,
	I_KeyboardKey_International8 = 142,
	I_KeyboardKey_International9 = 143,
	I_KeyboardKey_Language1 = 144,
	I_KeyboardKey_Language2 = 145,
	I_KeyboardKey_Language3 = 146,
	I_KeyboardKey_Language4 = 147,
	I_KeyboardKey_Language5 = 148,
	I_KeyboardKey_Language6 = 149,
	I_KeyboardKey_Language7 = 150,
	I_KeyboardKey_Language8 = 151,
	I_KeyboardKey_Language9 = 152,
	I_KeyboardKey_AltErase = 153,
	I_KeyboardKey_SysReq = 154,
	I_KeyboardKey_Cancel = 155,
	I_KeyboardKey_Clear = 156,
	I_KeyboardKey_Prior = 157,
	I_KeyboardKey_Return2 = 158,
	I_KeyboardKey_Seperator = 159,
	I_KeyboardKey_Out = 160,
	I_KeyboardKey_Oper = 161,
	I_KeyboardKey_ClearAgain = 162,
	I_KeyboardKey_Crsel = 163,
	I_KeyboardKey_Exsel = 164,
	I_KeyboardKey_Kp00 = 176,
	I_KeyboardKey_Kp000 = 177,
	I_KeyboardKey_ThousandsSeperator = 178,
	I_KeyboardKey_DecimalSeperator = 179,
	I_KeyboardKey_CurrencyUnit = 180,
	I_KeyboardKey_CurrencySubUnit = 181,
	I_KeyboardKey_KpLeftParent = 182,
	I_KeyboardKey_KpRightParent = 183,
	I_KeyboardKey_KpLeftBrace = 184,
	I_KeyboardKey_KpRightBrace = 185,
	I_KeyboardKey_KpTab = 186,
	I_KeyboardKey_KpBackspace = 187,
	I_KeyboardKey_KpA = 188,
	I_KeyboardKey_KpB = 189,
	I_KeyboardKey_KpC = 190,
	I_KeyboardKey_KpD = 191,
	I_KeyboardKey_KpE = 192,
	I_KeyboardKey_KpF = 193,
	I_KeyboardKey_KpXor = 194,
	I_KeyboardKey_KpPower = 195,
	I_KeyboardKey_KpPercent = 196,
	I_KeyboardKey_KpLess = 197,
	I_KeyboardKey_KpGreater = 198,
	I_KeyboardKey_KpAmpersand = 199,
	I_KeyboardKey_KpDoubleAmpersand = 200,
	I_KeyboardKey_KpVerticalBar = 201,
	I_KeyboardKey_KpDoubleVerticalBar = 202,
	I_KeyboardKey_KpColon = 203,
	I_KeyboardKey_KpHash = 204,
	I_KeyboardKey_KpSpace = 205,
	I_KeyboardKey_KpAt = 206,
	I_KeyboardKey_KpExclaim = 207,
	I_KeyboardKey_KpMemStore = 208,
	I_KeyboardKey_KpMemRecall = 209,
	I_KeyboardKey_KpMemClear = 210,
	I_KeyboardKey_KpMemAdd = 211,
	I_KeyboardKey_KpMemSubtract = 212,
	I_KeyboardKey_KpMemMultiply = 213,
	I_KeyboardKey_KpMemDivide = 214,
	I_KeyboardKey_KpPlusMinus = 215,
	I_KeyboardKey_KpClear = 216,
	I_KeyboardKey_KpClearEntry = 217,
	I_KeyboardKey_KpBinary = 218,
	I_KeyboardKey_KpOctal = 219,
	I_KeyboardKey_KpDecimal = 220,
	I_KeyboardKey_KpHexadecimal = 221,
	I_KeyboardKey_LeftControl = 224,
	I_KeyboardKey_LeftShift = 225,
	I_KeyboardKey_LeftAlt = 226,
	I_KeyboardKey_LeftSuper = 227,
	I_KeyboardKey_RightControl = 228,
	I_KeyboardKey_RightShift = 229,
	I_KeyboardKey_RightAlt = 230,
	I_KeyboardKey_RightSuper = 231,
	I_KeyboardKey_COUNT
}
I_KeyboardKey;

typedef enum I_MouseButton
{
	I_MouseButton_Unknown = 0,
	I_MouseButton_Left = 1,
	I_MouseButton_Middle = 2,
	I_MouseButton_Right = 3,
	I_MouseButton_SideBottom = 4,
	I_MouseButton_SideTop = 5,
	I_MouseButton_COUNT
}
I_MouseButton;

#define I_MAX_GAMEPADS 4

typedef enum I_GamepadButton
{
	I_GamepadButton_A,
	I_GamepadButton_B,
	I_GamepadButton_X,
	I_GamepadButton_Y,
	I_GamepadButton_Back,
	I_GamepadButton_Select,
	I_GamepadButton_Start,
	I_GamepadButton_LeftStick,
	I_GamepadButton_RightStick,
	I_GamepadButton_LeftShoulder,
	I_GamepadButton_RightShoulder,
	I_GamepadButton_Up,
	I_GamepadButton_Down,
	I_GamepadButton_Left,
	I_GamepadButton_Right,
	I_GamepadButton_COUNT,
	
	// PlayStation Equivalents.
	I_GamepadButton_PsCross    = I_GamepadButton_A,
	I_GamepadButton_PsCircle   = I_GamepadButton_B,
	I_GamepadButton_PsSquare   = I_GamepadButton_X,
	I_GamepadButton_PsTriangle = I_GamepadButton_Y
}
I_GamepadButton;

typedef enum I_GamepadAxis
{
	I_GamepadAxis_LeftX,
	I_GamepadAxis_LeftY,
	I_GamepadAxis_RightX,
	I_GamepadAxis_RightY,
	I_GamepadAxis_TriggerLeft,
	I_GamepadAxis_TriggerRight,
	I_GamepadAxis_COUNT
}
I_GamepadAxis;

typedef enum I_GamepadType
{
	I_GamepadType_Standard,
	I_GamepadType_Xbox360,
	I_GamepadType_XboxOne,
	I_GamepadType_Ps3,
	I_GamepadType_Ps4,
	I_GamepadType_Ps5,
	I_GamepadType_NsPro,
	I_GamepadType_NsJoyconLeft,
	I_GamepadType_NsJoyconRight,
	I_GamepadType_NsJoyconPair,
	I_GamepadType_COUNT
}
I_GamepadType;

typedef struct I_GamepadState I_GamepadState;
struct I_GamepadState
{
	b32 down[I_GamepadButton_COUNT];
	b32 pressed[I_GamepadButton_COUNT];
	b32 released[I_GamepadButton_COUNT];

	v2 left_stick;
	v2 right_stick;

	f32 left_trigger;
	f32 right_trigger;
};

internal void I_GamepadStateSetAxisValue(I_GamepadState *st, I_GamepadAxis axis, f32 value);

typedef struct I_State I_State;
struct I_State
{
	b32 kb_down[I_KeyboardKey_COUNT];
	b32 kb_pressed[I_KeyboardKey_COUNT];
	b32 kb_released[I_KeyboardKey_COUNT];
	
	b32 mb_down[I_MouseButton_COUNT];
	b32 mb_pressed[I_MouseButton_COUNT];
	b32 mb_released[I_MouseButton_COUNT];

	v2 mouse_delta;
	v2 mouse_position;
	v2 mouse_screen_position;
	v2 mouse_wheel;

	I_GamepadState gamepads[I_MAX_GAMEPADS];
};

internal b32 I_KbDown     (const I_State *st, I_KeyboardKey k);
internal b32 I_KbPressed  (const I_State *st, I_KeyboardKey k);
internal b32 I_KbReleased (const I_State *st, I_KeyboardKey k);

internal b32 I_MbDown     (const I_State *st, I_MouseButton b);
internal b32 I_MbPressed  (const I_State *st, I_MouseButton b);
internal b32 I_MbReleased (const I_State *st, I_MouseButton b);

internal b32 I_GpDown     (const I_State *st, I_GamepadButton b, u32 player_index);
internal b32 I_GpPressed  (const I_State *st, I_GamepadButton b, u32 player_index);
internal b32 I_GpReleased (const I_State *st, I_GamepadButton b, u32 player_index);

internal b32 I_Shift (const I_State *st);
internal b32 I_Ctrl  (const I_State *st);
internal b32 I_Alt   (const I_State *st);

// 0 <= lo, hi <= 1
//internal void I_RumbleGamepad(u32 index, f32 lo, f32 hi, f32 duration_s);

#endif // I_INPUT_H
