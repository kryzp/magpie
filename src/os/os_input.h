#ifndef OS_INPUT_H
#define OS_INPUT_H

typedef enum OS_KeyboardKey
{
	OS_KeyboardKey_Unknown = 0,
	OS_KeyboardKey_A = 4,
	OS_KeyboardKey_B = 5,
	OS_KeyboardKey_C = 6,
	OS_KeyboardKey_D = 7,
	OS_KeyboardKey_E = 8,
	OS_KeyboardKey_F = 9,
	OS_KeyboardKey_G = 10,
	OS_KeyboardKey_H = 11,
	OS_KeyboardKey_I = 12,
	OS_KeyboardKey_J = 13,
	OS_KeyboardKey_K = 14,
	OS_KeyboardKey_L = 15,
	OS_KeyboardKey_M = 16,
	OS_KeyboardKey_N = 17,
	OS_KeyboardKey_O = 18,
	OS_KeyboardKey_P = 19,
	OS_KeyboardKey_Q = 20,
	OS_KeyboardKey_R = 21,
	OS_KeyboardKey_S = 22,
	OS_KeyboardKey_T = 23,
	OS_KeyboardKey_U = 24,
	OS_KeyboardKey_V = 25,
	OS_KeyboardKey_W = 26,
	OS_KeyboardKey_X = 27,
	OS_KeyboardKey_Y = 28,
	OS_KeyboardKey_Z = 29,
	OS_KeyboardKey_D1 = 30,
	OS_KeyboardKey_D2 = 31,
	OS_KeyboardKey_D3 = 32,
	OS_KeyboardKey_D4 = 33,
	OS_KeyboardKey_D5 = 34,
	OS_KeyboardKey_D6 = 35,
	OS_KeyboardKey_D7 = 36,
	OS_KeyboardKey_D8 = 37,
	OS_KeyboardKey_D9 = 38,
	OS_KeyboardKey_D0 = 39,
	OS_KeyboardKey_Enter = 40,
	OS_KeyboardKey_Escape = 41,
	OS_KeyboardKey_Backspace = 42,
	OS_KeyboardKey_Tab = 43,
	OS_KeyboardKey_Space = 44,
	OS_KeyboardKey_Minus = 45,
	OS_KeyboardKey_Equals = 46,
	OS_KeyboardKey_LeftBracket = 47,
	OS_KeyboardKey_RightBracket = 48,
	OS_KeyboardKey_Backslash = 49,
	OS_KeyboardKey_NonUSHash = 50,
	OS_KeyboardKey_Semicolon = 51,
	OS_KeyboardKey_Apostrophe = 52,
	OS_KeyboardKey_Grave = 53,
	OS_KeyboardKey_Comma = 54,
	OS_KeyboardKey_Period = 55,
	OS_KeyboardKey_Slash = 56,
	OS_KeyboardKey_Capslock = 57,
	OS_KeyboardKey_F1 = 58,
	OS_KeyboardKey_F2 = 59,
	OS_KeyboardKey_F3 = 60,
	OS_KeyboardKey_F4 = 61,
	OS_KeyboardKey_F5 = 62,
	OS_KeyboardKey_F6 = 63,
	OS_KeyboardKey_F7 = 64,
	OS_KeyboardKey_F8 = 65,
	OS_KeyboardKey_F9 = 66,
	OS_KeyboardKey_F10 = 67,
	OS_KeyboardKey_F11 = 68,
	OS_KeyboardKey_F12 = 69,
	OS_KeyboardKey_PrintScreen = 70,
	OS_KeyboardKey_ScrollLock = 71,
	OS_KeyboardKey_Pause = 72,
	OS_KeyboardKey_Insert = 73,
	OS_KeyboardKey_Home = 74,
	OS_KeyboardKey_PageUp = 75,
	OS_KeyboardKey_Delete = 76,
	OS_KeyboardKey_End = 77,
	OS_KeyboardKey_PageDown = 78,
	OS_KeyboardKey_Right = 79,
	OS_KeyboardKey_Left = 80,
	OS_KeyboardKey_Down = 81,
	OS_KeyboardKey_Up = 82,
	OS_KeyboardKey_Numlock_clear = 83,
	OS_KeyboardKey_KpDivide = 84,
	OS_KeyboardKey_KpMultiply = 85,
	OS_KeyboardKey_KpMinus = 86,
	OS_KeyboardKey_KpPlus = 87,
	OS_KeyboardKey_KpEnter = 88,
	OS_KeyboardKey_Kp1 = 89,
	OS_KeyboardKey_Kp2 = 90,
	OS_KeyboardKey_Kp3 = 91,
	OS_KeyboardKey_Kp4 = 92,
	OS_KeyboardKey_Kp5 = 93,
	OS_KeyboardKey_Kp6 = 94,
	OS_KeyboardKey_Kp7 = 95,
	OS_KeyboardKey_Kp8 = 96,
	OS_KeyboardKey_Kp9 = 97,
	OS_KeyboardKey_Kp0 = 98,
	OS_KeyboardKey_KpPeriod = 99,
	OS_KeyboardKey_NonUSBackslash = 100,
	OS_KeyboardKey_Application = 101,
	OS_KeyboardKey_Power = 102,
	OS_KeyboardKey_KpEquals = 103,
	OS_KeyboardKey_F13 = 104,
	OS_KeyboardKey_F14 = 105,
	OS_KeyboardKey_F15 = 106,
	OS_KeyboardKey_F16 = 107,
	OS_KeyboardKey_F17 = 108,
	OS_KeyboardKey_F18 = 109,
	OS_KeyboardKey_F19 = 110,
	OS_KeyboardKey_F20 = 111,
	OS_KeyboardKey_F21 = 112,
	OS_KeyboardKey_F22 = 113,
	OS_KeyboardKey_F23 = 114,
	OS_KeyboardKey_F24 = 115,
	OS_KeyboardKey_Execute = 116,
	OS_KeyboardKey_Help = 117,
	OS_KeyboardKey_Menu = 118,
	OS_KeyboardKey_Select = 119,
	OS_KeyboardKey_Stop = 120,
	OS_KeyboardKey_Again = 121,
	OS_KeyboardKey_Undo = 122,
	OS_KeyboardKey_Cut = 123,
	OS_KeyboardKey_Copy = 124,
	OS_KeyboardKey_Paste = 125,
	OS_KeyboardKey_Find = 126,
	OS_KeyboardKey_Mute = 127,
	OS_KeyboardKey_VolumeUp = 128,
	OS_KeyboardKey_VolumeDown = 129,
	OS_KeyboardKey_KpComma = 133,
	OS_KeyboardKey_KpEqualsAs400 = 134,
	OS_KeyboardKey_International1 = 135,
	OS_KeyboardKey_International2 = 136,
	OS_KeyboardKey_International3 = 137,
	OS_KeyboardKey_International4 = 138,
	OS_KeyboardKey_International5 = 139,
	OS_KeyboardKey_International6 = 140,
	OS_KeyboardKey_International7 = 141,
	OS_KeyboardKey_International8 = 142,
	OS_KeyboardKey_International9 = 143,
	OS_KeyboardKey_Language1 = 144,
	OS_KeyboardKey_Language2 = 145,
	OS_KeyboardKey_Language3 = 146,
	OS_KeyboardKey_Language4 = 147,
	OS_KeyboardKey_Language5 = 148,
	OS_KeyboardKey_Language6 = 149,
	OS_KeyboardKey_Language7 = 150,
	OS_KeyboardKey_Language8 = 151,
	OS_KeyboardKey_Language9 = 152,
	OS_KeyboardKey_AltErase = 153,
	OS_KeyboardKey_SysReq = 154,
	OS_KeyboardKey_Cancel = 155,
	OS_KeyboardKey_Clear = 156,
	OS_KeyboardKey_Prior = 157,
	OS_KeyboardKey_Return2 = 158,
	OS_KeyboardKey_Seperator = 159,
	OS_KeyboardKey_Out = 160,
	OS_KeyboardKey_Oper = 161,
	OS_KeyboardKey_ClearAgain = 162,
	OS_KeyboardKey_Crsel = 163,
	OS_KeyboardKey_Exsel = 164,
	OS_KeyboardKey_Kp00 = 176,
	OS_KeyboardKey_Kp000 = 177,
	OS_KeyboardKey_ThousandsSeperator = 178,
	OS_KeyboardKey_DecimalSeperator = 179,
	OS_KeyboardKey_CurrencyUnit = 180,
	OS_KeyboardKey_CurrencySubUnit = 181,
	OS_KeyboardKey_KpLeftParent = 182,
	OS_KeyboardKey_KpRightParent = 183,
	OS_KeyboardKey_KpLeftBrace = 184,
	OS_KeyboardKey_KpRightBrace = 185,
	OS_KeyboardKey_KpTab = 186,
	OS_KeyboardKey_KpBackspace = 187,
	OS_KeyboardKey_KpA = 188,
	OS_KeyboardKey_KpB = 189,
	OS_KeyboardKey_KpC = 190,
	OS_KeyboardKey_KpD = 191,
	OS_KeyboardKey_KpE = 192,
	OS_KeyboardKey_KpF = 193,
	OS_KeyboardKey_KpXor = 194,
	OS_KeyboardKey_KpPower = 195,
	OS_KeyboardKey_KpPercent = 196,
	OS_KeyboardKey_KpLess = 197,
	OS_KeyboardKey_KpGreater = 198,
	OS_KeyboardKey_KpAmpersand = 199,
	OS_KeyboardKey_KpDoubleAmpersand = 200,
	OS_KeyboardKey_KpVerticalBar = 201,
	OS_KeyboardKey_KpDoubleVerticalBar = 202,
	OS_KeyboardKey_KpColon = 203,
	OS_KeyboardKey_KpHash = 204,
	OS_KeyboardKey_KpSpace = 205,
	OS_KeyboardKey_KpAt = 206,
	OS_KeyboardKey_KpExclaim = 207,
	OS_KeyboardKey_KpMemStore = 208,
	OS_KeyboardKey_KpMemRecall = 209,
	OS_KeyboardKey_KpMemClear = 210,
	OS_KeyboardKey_KpMemAdd = 211,
	OS_KeyboardKey_KpMemSubtract = 212,
	OS_KeyboardKey_KpMemMultiply = 213,
	OS_KeyboardKey_KpMemDivide = 214,
	OS_KeyboardKey_KpPlusMinus = 215,
	OS_KeyboardKey_KpClear = 216,
	OS_KeyboardKey_KpClearEntry = 217,
	OS_KeyboardKey_KpBinary = 218,
	OS_KeyboardKey_KpOctal = 219,
	OS_KeyboardKey_KpDecimal = 220,
	OS_KeyboardKey_KpHexadecimal = 221,
	OS_KeyboardKey_LeftControl = 224,
	OS_KeyboardKey_LeftShift = 225,
	OS_KeyboardKey_LeftAlt = 226,
	OS_KeyboardKey_LeftSuper = 227,
	OS_KeyboardKey_RightControl = 228,
	OS_KeyboardKey_RightShift = 229,
	OS_KeyboardKey_RightAlt = 230,
	OS_KeyboardKey_RightSuper = 231,
	OS_KeyboardKey_COUNT
}
OS_KeyboardKey;

typedef enum OS_MouseButton
{
	OS_MouseButton_Unknown = 0,
	OS_MouseButton_Left = 1,
	OS_MouseButton_Middle = 2,
	OS_MouseButton_Right = 3,
	OS_MouseButton_SideBottom = 4,
	OS_MouseButton_SideTop = 5,
	OS_MouseButton_COUNT
}
OS_MouseButton;

#define OS_MAX_GAMEPADS 4

typedef enum OS_GamepadButton
{
	OS_GamepadButton_A,
	OS_GamepadButton_B,
	OS_GamepadButton_X,
	OS_GamepadButton_Y,
	OS_GamepadButton_Back,
	OS_GamepadButton_Select,
	OS_GamepadButton_Start,
	OS_GamepadButton_LeftStick,
	OS_GamepadButton_RightStick,
	OS_GamepadButton_LeftShoulder,
	OS_GamepadButton_RightShoulder,
	OS_GamepadButton_Up,
	OS_GamepadButton_Down,
	OS_GamepadButton_Left,
	OS_GamepadButton_Right,
	OS_GamepadButton_COUNT,
	
	// PlayStation Equivalents.
	OS_GamepadButton_PsCross    = OS_GamepadButton_A,
	OS_GamepadButton_PsCircle   = OS_GamepadButton_B,
	OS_GamepadButton_PsSquare   = OS_GamepadButton_X,
	OS_GamepadButton_PsTriangle = OS_GamepadButton_Y
}
OS_GamepadButton;

typedef enum OS_GamepadAxis
{
	OS_GamepadAxis_LeftX,
	OS_GamepadAxis_LeftY,
	OS_GamepadAxis_RightX,
	OS_GamepadAxis_RightY,
	OS_GamepadAxis_TriggerLeft,
	OS_GamepadAxis_TriggerRight,
	OS_GamepadAxis_COUNT
}
OS_GamepadAxis;

typedef enum OS_GamepadType
{
	OS_GamepadType_Standard,
	OS_GamepadType_Xbox360,
	OS_GamepadType_XboxOne,
	OS_GamepadType_Ps3,
	OS_GamepadType_Ps4,
	OS_GamepadType_Ps5,
	OS_GamepadType_NsPro,
	OS_GamepadType_NsJoyconLeft,
	OS_GamepadType_NsJoyconRight,
	OS_GamepadType_NsJoyconPair,
	OS_GamepadType_COUNT
}
OS_GamepadType;

typedef struct OS_GamepadState OS_GamepadState;
struct OS_GamepadState
{
	b32 down[OS_GamepadButton_COUNT];
	b32 pressed[OS_GamepadButton_COUNT];
	b32 released[OS_GamepadButton_COUNT];

	v2 left_stick;
	v2 right_stick;

	f32 left_trigger;
	f32 right_trigger;
};

static void OS_GamepadStateSetAxisValue(OS_GamepadState *st, OS_GamepadAxis axis, f32 value);

typedef struct OS_InputState OS_InputState;
struct OS_InputState
{
	b32 kb_down[OS_KeyboardKey_COUNT];
	b32 kb_pressed[OS_KeyboardKey_COUNT];
	b32 kb_released[OS_KeyboardKey_COUNT];
	
	b32 mb_down[OS_MouseButton_COUNT];
	b32 mb_pressed[OS_MouseButton_COUNT];
	b32 mb_released[OS_MouseButton_COUNT];

	v2 mouse_delta;
	v2 mouse_position;
	v2 mouse_screen_position;
	v2 mouse_wheel;

	OS_GamepadState gamepads[OS_MAX_GAMEPADS];
};

static b32 OS_KbDown     (const OS_InputState *st, OS_KeyboardKey k);
static b32 OS_KbPressed  (const OS_InputState *st, OS_KeyboardKey k);
static b32 OS_KbReleased (const OS_InputState *st, OS_KeyboardKey k);

static b32 OS_MbDown     (const OS_InputState *st, OS_MouseButton b);
static b32 OS_MbPressed  (const OS_InputState *st, OS_MouseButton b);
static b32 OS_MbReleased (const OS_InputState *st, OS_MouseButton b);

static b32 OS_GpDown     (const OS_InputState *st, OS_GamepadButton b, u32 player_index);
static b32 OS_GpPressed  (const OS_InputState *st, OS_GamepadButton b, u32 player_index);
static b32 OS_GpReleased (const OS_InputState *st, OS_GamepadButton b, u32 player_index);

static b32 OS_KbShift    (const OS_InputState *st);
static b32 OS_KbCtrl     (const OS_InputState *st);
static b32 OS_KbAlt      (const OS_InputState *st);

// 0 <= lo, hi <= 1
//static void OS_RumbleGamepad(u32 index, f32 lo, f32 hi, f32 duration_s);

#endif // OS_INPUT_H
