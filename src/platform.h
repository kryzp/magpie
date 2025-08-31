
typedef enum KeyboardKey {
	KeyboardKey_Unknown = 0,
	KeyboardKey_A = 4,
	KeyboardKey_B = 5,
	KeyboardKey_C = 6,
	KeyboardKey_D = 7,
	KeyboardKey_E = 8,
	KeyboardKey_F = 9,
	KeyboardKey_G = 10,
	KeyboardKey_H = 11,
	KeyboardKey_I = 12,
	KeyboardKey_J = 13,
	KeyboardKey_K = 14,
	KeyboardKey_L = 15,
	KeyboardKey_M = 16,
	KeyboardKey_N = 17,
	KeyboardKey_O = 18,
	KeyboardKey_P = 19,
	KeyboardKey_Q = 20,
	KeyboardKey_R = 21,
	KeyboardKey_S = 22,
	KeyboardKey_T = 23,
	KeyboardKey_U = 24,
	KeyboardKey_V = 25,
	KeyboardKey_W = 26,
	KeyboardKey_X = 27,
	KeyboardKey_Y = 28,
	KeyboardKey_Z = 29,
	KeyboardKey_D1 = 30,
	KeyboardKey_D2 = 31,
	KeyboardKey_D3 = 32,
	KeyboardKey_D4 = 33,
	KeyboardKey_D5 = 34,
	KeyboardKey_D6 = 35,
	KeyboardKey_D7 = 36,
	KeyboardKey_D8 = 37,
	KeyboardKey_D9 = 38,
	KeyboardKey_D0 = 39,
	KeyboardKey_Enter = 40,
	KeyboardKey_Escape = 41,
	KeyboardKey_Backspace = 42,
	KeyboardKey_Tab = 43,
	KeyboardKey_Space = 44,
	KeyboardKey_Minus = 45,
	KeyboardKey_Equals = 46,
	KeyboardKey_LeftBracket = 47,
	KeyboardKey_RightBracket = 48,
	KeyboardKey_Backslash = 49,
	KeyboardKey_NonUSHash = 50,
	KeyboardKey_Semicolon = 51,
	KeyboardKey_Apostrophe = 52,
	KeyboardKey_Grave = 53,
	KeyboardKey_Comma = 54,
	KeyboardKey_Period = 55,
	KeyboardKey_Slash = 56,
	KeyboardKey_Capslock = 57,
	KeyboardKey_F1 = 58,
	KeyboardKey_F2 = 59,
	KeyboardKey_F3 = 60,
	KeyboardKey_F4 = 61,
	KeyboardKey_F5 = 62,
	KeyboardKey_F6 = 63,
	KeyboardKey_F7 = 64,
	KeyboardKey_F8 = 65,
	KeyboardKey_F9 = 66,
	KeyboardKey_F10 = 67,
	KeyboardKey_F11 = 68,
	KeyboardKey_F12 = 69,
	KeyboardKey_PrintScreen = 70,
	KeyboardKey_ScrollLock = 71,
	KeyboardKey_Pause = 72,
	KeyboardKey_Insert = 73,
	KeyboardKey_Home = 74,
	KeyboardKey_PageUp = 75,
	KeyboardKey_Delete = 76,
	KeyboardKey_End = 77,
	KeyboardKey_PageDown = 78,
	KeyboardKey_Right = 79,
	KeyboardKey_Left = 80,
	KeyboardKey_Down = 81,
	KeyboardKey_Up = 82,
	KeyboardKey_NumlockClear = 83,
	KeyboardKey_KpDivide = 84,
	KeyboardKey_KpMultiply = 85,
	KeyboardKey_KpMinus = 86,
	KeyboardKey_KpPlus = 87,
	KeyboardKey_KpEnter = 88,
	KeyboardKey_Kp1 = 89,
	KeyboardKey_Kp2 = 90,
	KeyboardKey_Kp3 = 91,
	KeyboardKey_Kp4 = 92,
	KeyboardKey_Kp5 = 93,
	KeyboardKey_Kp6 = 94,
	KeyboardKey_Kp7 = 95,
	KeyboardKey_Kp8 = 96,
	KeyboardKey_Kp9 = 97,
	KeyboardKey_Kp0 = 98,
	KeyboardKey_KpPeriod = 99,
	KeyboardKey_NonUSBackslash = 100,
	KeyboardKey_Corelication = 101,
	KeyboardKey_Power = 102,
	KeyboardKey_KpEquals = 103,
	KeyboardKey_F13 = 104,
	KeyboardKey_F14 = 105,
	KeyboardKey_F15 = 106,
	KeyboardKey_F16 = 107,
	KeyboardKey_F17 = 108,
	KeyboardKey_F18 = 109,
	KeyboardKey_F19 = 110,
	KeyboardKey_F20 = 111,
	KeyboardKey_F21 = 112,
	KeyboardKey_F22 = 113,
	KeyboardKey_F23 = 114,
	KeyboardKey_F24 = 115,
	KeyboardKey_Execute = 116,
	KeyboardKey_Help = 117,
	KeyboardKey_Menu = 118,
	KeyboardKey_Select = 119,
	KeyboardKey_Stop = 120,
	KeyboardKey_Again = 121,
	KeyboardKey_Undo = 122,
	KeyboardKey_Cut = 123,
	KeyboardKey_Copy = 124,
	KeyboardKey_Paste = 125,
	KeyboardKey_Find = 126,
	KeyboardKey_Mute = 127,
	KeyboardKey_VolumeUp = 128,
	KeyboardKey_VolumeDown = 129,
	KeyboardKey_KpComma = 133,
	KeyboardKey_KpEqualsAs400 = 134,
	KeyboardKey_International1 = 135,
	KeyboardKey_International2 = 136,
	KeyboardKey_International3 = 137,
	KeyboardKey_International4 = 138,
	KeyboardKey_International5 = 139,
	KeyboardKey_International6 = 140,
	KeyboardKey_International7 = 141,
	KeyboardKey_International8 = 142,
	KeyboardKey_International9 = 143,
	KeyboardKey_Language1 = 144,
	KeyboardKey_Language2 = 145,
	KeyboardKey_Language3 = 146,
	KeyboardKey_Language4 = 147,
	KeyboardKey_Language5 = 148,
	KeyboardKey_Language6 = 149,
	KeyboardKey_Language7 = 150,
	KeyboardKey_Language8 = 151,
	KeyboardKey_Language9 = 152,
	KeyboardKey_AltErase = 153,
	KeyboardKey_SysReq = 154,
	KeyboardKey_Cancel = 155,
	KeyboardKey_Clear = 156,
	KeyboardKey_Prior = 157,
	KeyboardKey_Return2 = 158,
	KeyboardKey_Seperator = 159,
	KeyboardKey_Out = 160,
	KeyboardKey_Oper = 161,
	KeyboardKey_ClearAgain = 162,
	KeyboardKey_Crsel = 163,
	KeyboardKey_Exsel = 164,
	KeyboardKey_Kp00 = 176,
	KeyboardKey_Kp000 = 177,
	KeyboardKey_ThousandsSeperator = 178,
	KeyboardKey_DecimalSeperator = 179,
	KeyboardKey_CurrencyUnit = 180,
	KeyboardKey_CurrencySubUnit = 181,
	KeyboardKey_KpLeftParent = 182,
	KeyboardKey_KpRightParent = 183,
	KeyboardKey_KpLeftBrace = 184,
	KeyboardKey_KpRightBrace = 185,
	KeyboardKey_KpTab = 186,
	KeyboardKey_KpBackspace = 187,
	KeyboardKey_KpA = 188,
	KeyboardKey_KpB = 189,
	KeyboardKey_KpC = 190,
	KeyboardKey_KpD = 191,
	KeyboardKey_KpE = 192,
	KeyboardKey_KpF = 193,
	KeyboardKey_KpXor = 194,
	KeyboardKey_KpPower = 195,
	KeyboardKey_KpPercent = 196,
	KeyboardKey_KpLess = 197,
	KeyboardKey_KpGreater = 198,
	KeyboardKey_KpAmpersand = 199,
	KeyboardKey_KpDoubleAmpersand = 200,
	KeyboardKey_KpVerticalBar = 201,
	KeyboardKey_KpDoubleVerticalBar = 202,
	KeyboardKey_KpColon = 203,
	KeyboardKey_KpHash = 204,
	KeyboardKey_KpSpace = 205,
	KeyboardKey_KpAt = 206,
	KeyboardKey_KpExclaim = 207,
	KeyboardKey_KpMemStore = 208,
	KeyboardKey_KpMemRecall = 209,
	KeyboardKey_KpMemClear = 210,
	KeyboardKey_KpMemAdd = 211,
	KeyboardKey_KpMemSubtract = 212,
	KeyboardKey_KpMemMultiply = 213,
	KeyboardKey_KpMemDivide = 214,
	KeyboardKey_KpPlusMinus = 215,
	KeyboardKey_KpClear = 216,
	KeyboardKey_KpClearEntry = 217,
	KeyboardKey_KpBinary = 218,
	KeyboardKey_KpOctal = 219,
	KeyboardKey_KpDecimal = 220,
	KeyboardKey_KpHexadecimal = 221,
	KeyboardKey_LeftControl = 224,
	KeyboardKey_LeftShift = 225,
	KeyboardKey_LeftAlt = 226,
	KeyboardKey_LeftSuper = 227,
	KeyboardKey_RightControl = 228,
	KeyboardKey_RightShift = 229,
	KeyboardKey_RightAlt = 230,
	KeyboardKey_RightSuper = 231,
	KeyboardKey_MaxEnum
} KeyboardKey;

typedef enum MouseButton {
	MouseButton_Unknown = 0,
	MouseButton_Left = 1,
	MouseButton_Middle = 2,
	MouseButton_Right = 3,
	MouseButton_SideBottom = 4,
	MouseButton_SideTop = 5,
	MouseButton_MaxEnum
} MouseButton;

#define MAX_GAMEPADS 4

typedef enum GamepadButton {
	GamepadButton_A,
	GamepadButton_B,
	GamepadButton_X,
	GamepadButton_Y,

	GamepadButton_Back,
	GamepadButton_Select,
	GamepadButton_Start,

	GamepadButton_LeftStick,
	GamepadButton_RightStick,

	GamepadButton_LeftShoulder,
	GamepadButton_RightShoulder,

	GamepadButton_Up,
	GamepadButton_Down,
	GamepadButton_Left,
	GamepadButton_Right,

	GamepadButton_MaxEnum
} GamepadButton;

typedef enum GamepadAxis {
	GamepadAxis_LeftX,
	GamepadAxis_LeftY,

	GamepadAxis_RightX,
	GamepadAxis_RightY,

	GamepadAxis_TriggerLeft,
	GamepadAxis_TriggerRight,

	GamepadAxis_MaxEnum
} GamepadAxis;

typedef enum GamepadType {
	GamepadType_Standard,

	GamepadType_Xbox360,
	GamepadType_XboxOne,

	GamepadType_PS3,
	GamepadType_PS4,
	GamepadType_PS5,

	GamepadType_NintendoSwitchPro,
	GamepadType_NintendoSwitchJoyconLeft,
	GamepadType_NintendoSwitchJoyconRight,
	GamepadType_NintendoSwitchJoyconPair,

	GamepadType_MaxEnum
} GamepadType;

typedef struct GamepadState {
	b32 down[GamepadButton_MaxEnum];
	b32 pressed[GamepadButton_MaxEnum];
	b32 released[GamepadButton_MaxEnum];

	v2 left_stick;
	v2 right_stick;

	f32 left_trigger;
	f32 right_trigger;
} GamepadState;

typedef struct Platform {
	void *permanent_memory;
	u64 permanent_memory_size;

	void *transient_memory;
	u64 transient_memory_size;

	void *scratch_memory[2];
	u64 scratch_memory_size;

	i32 window_width;
	i32 window_height;

	i32 window_pixel_width;
	i32 window_pixel_height;

	f32 window_opacity;

	b32 fullscreen;
	b32 borderless;

	f32 target_fps;
	f32 current_time;

	b32 cursor_visible;
	b32 cursor_locked;

	b32 exit;

	b32 kb_down[KeyboardKey_MaxEnum];
	b32 kb_pressed[KeyboardKey_MaxEnum];
	b32 kb_released[KeyboardKey_MaxEnum];

	b32 mb_down[MouseButton_MaxEnum];
	b32 mb_pressed[MouseButton_MaxEnum];
	b32 mb_released[MouseButton_MaxEnum];

	v2 mouse_position;
	v2 mouse_delta;
	v2 mouse_screen_position;
	v2 mouse_wheel;

	GamepadState gamepads[MAX_GAMEPADS];

	const char *const *(*GetVulkanInstanceExtensions)(u32 *count);
	b32 (*CreateVulkanSurface)(void *instance, void *surface);

	u64 (*GetTicks)(void);
	u64 (*GetPerformanceCounter)(void);
	u64 (*GetPerformanceFrequency)(void);
} Platform;
